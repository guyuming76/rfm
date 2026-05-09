#!/usr/bin/env python3
"""
rfm OpenHarness Agent — runs OpenHarness in-process inside rfm via embedded Python.

Architecture:
    rfm (C/GTK)  --PyGILState-->  rfmOpenHarness (sync facade)
                                       │
                                       │  asyncio.run_coroutine_threadsafe()
                                       │  or loop.run_until_complete()
                                       ▼
                                  OpenHarness Runtime
                                  (build_runtime / handle_line)

Usage from rfm stdin:
    oh 帮我分析这些C文件           # single-turn
    oh -c                          # continuous conversation (REPL)
    oh -c 重构这个函数             # continuous with initial prompt
"""

from __future__ import annotations

import asyncio
import json
import os
import sys
from pathlib import Path
from typing import Any, Callable

# ── ensure OpenHarness is importable ──────────────────────────────────
# When rfm embeds Python, sys.path includes rfm's scripts/ dir.
# We need to add the OpenHarness source tree so that `from openharness import ...` works.
_OH_SRC = os.environ.get(
    "OPENHARNESS_SRC",
    os.path.expanduser("~/OpenHarness/src"),
)
if _OH_SRC not in sys.path:
    sys.path.insert(0, _OH_SRC)

# rfm embeds the system Python, but OpenHarness deps are in a venv.
# Add the venv's site-packages to sys.path.
_OH_VENV = os.environ.get(
    "OPENHARNESS_VENV",
    os.path.expanduser("~/OpenHarness/.venv"),
)
if _OH_VENV and os.path.isdir(_OH_VENV):
    import site
    # Determine site-packages dir for the venv's Python version
    _venv_site = os.path.join(
        _OH_VENV,
        "lib",
        f"python{sys.version_info.major}.{sys.version_info.minor}",
        "site-packages",
    )
    if os.path.isdir(_venv_site) and _venv_site not in sys.path:
        sys.path.insert(0, _venv_site)
        # Also add the venv bin to PATH so subprocess tools find executables
        _venv_bin = os.path.join(_OH_VENV, "bin")
        if os.path.isdir(_venv_bin):
            os.environ["PATH"] = _venv_bin + os.pathsep + os.environ.get("PATH", "")


# ── permission prompt (terminal-based, since rfm inherits stdin/stdout) ─
async def _rfm_permission_prompt(tool_name: str, reason: str) -> bool:
    """Ask user for permission via terminal — works because rfm inherits stdin/stdout."""
    # Print a clear prompt
    sys.stderr.write(f"\n🔐 Allow tool?  tool='{tool_name}'  reason='{reason}'\n")
    sys.stderr.flush()
    try:
        answer = input("  [y/N] → ").strip().lower()
    except EOFError:
        return False
    return answer in ("y", "yes")


async def _rfm_ask_user(question: str) -> str:
    """Ask the user a question via terminal."""
    sys.stderr.write(f"\n❓ {question}\n")
    sys.stderr.flush()
    try:
        return input("  > ").strip()
    except EOFError:
        return ""


class _StreamRenderer:
    """Collects streaming output from the agent, prints to stdout."""

    def __init__(self) -> None:
        self.collected_text = ""

    def reset(self) -> None:
        self.collected_text = ""

    async def __call__(self, event: Any) -> None:
        from openharness.engine.stream_events import (
            AssistantTextDelta,
            AssistantTurnComplete,
            ErrorEvent,
            StatusEvent,
            ToolExecutionStarted,
            ToolExecutionCompleted,
        )
        if isinstance(event, AssistantTextDelta):
            self.collected_text += event.text
            sys.stdout.write(event.text)
            sys.stdout.flush()
        elif isinstance(event, AssistantTurnComplete):
            sys.stdout.write("\n")
            sys.stdout.flush()
        elif isinstance(event, ErrorEvent):
            sys.stderr.write(f"\n❌ {event.message}\n")
            sys.stderr.flush()
        elif isinstance(event, StatusEvent) and event.message:
            sys.stderr.write(f"  … {event.message}\n")
            sys.stderr.flush()
        elif isinstance(event, ToolExecutionStarted):
            sys.stderr.write(f"  🔧 {event.tool_name} ...\n")
            sys.stderr.flush()
        elif isinstance(event, ToolExecutionCompleted):
            if event.is_error:
                sys.stderr.write(f"  ❌ {event.tool_name} failed\n")
            sys.stderr.flush()


class RFMOpenHarness:
    """Singleton manager for an in-process OpenHarness session.

    Designed to be called from rfm's C code via Python's C API.
    """

    def __init__(self) -> None:
        self._bundle = None
        self._loop: asyncio.AbstractEventLoop | None = None
        self._renderer = _StreamRenderer()
        self._model: str | None = None
        self._api_key: str | None = None
        self._base_url: str | None = None
        self._api_format: str | None = None
        self._base_system_prompt: str = ""  # cached after init, without dynamic state
        self._rfm_cur_path: str = "/"
        self._rfm_selected_files: list[str] = []

    # ── public API (called from C) ───────────────────────────────────

    def init(
        self,
        cur_path: str,
        model: str | None = None,
        api_key: str | None = None,
        base_url: str | None = None,
        api_format: str | None = None,
    ) -> bool:
        """Initialise OpenHarness runtime.  Returns True on success."""
        if self._loop is not None:
            return True  # already initialised

        self._model = model
        self._api_key = api_key
        self._base_url = base_url
        self._api_format = api_format

        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_until_complete(self._async_init(cur_path))
        except Exception:
            import traceback
            traceback.print_exc(file=sys.stderr)
            self._loop.close()
            self._loop = None
            return False
        return True

    def set_rfm_state(self, cur_path: str, selected_files: list[str] | None = None) -> None:
        """Update the rfm context that the agent can see.

        Called before every ask() to keep context current.
        """
        if self._bundle is None:
            return
        self._rfm_cur_path = cur_path
        self._rfm_selected_files = selected_files or []
        # Update engine cwd so tools see the right directory
        self._bundle.engine.cwd = cur_path
        # Sync state to rfmOH_tools module so rfm_* tools can see it
        try:
            from rfmOH_tools import update_rfm_state
            update_rfm_state(cur_path, self._rfm_selected_files)
        except ImportError:
            pass

    def ask(self, prompt: str, continuous: bool = False) -> str:
        """Send a prompt to the agent and return the response text.

        Args:
            prompt: User prompt (ignored if continuous mode reads from stdin).
            continuous: If True, enter a mini-REPL loop inside rfm.

        Returns:
            The final collected response text.
        """
        if self._loop is None or self._bundle is None:
            return "Error: OpenHarness not initialised. Call init() first."

        if continuous:
            return self._loop.run_until_complete(self._async_repl(prompt))
        else:
            return self._loop.run_until_complete(self._async_ask(prompt))

    def shutdown(self) -> None:
        """Clean up the OpenHarness runtime."""
        if self._loop is None or self._bundle is None:
            return
        try:
            self._loop.run_until_complete(self._async_shutdown())
        except Exception:
            import traceback
            traceback.print_exc(file=sys.stderr)
        finally:
            self._loop.close()
            self._loop = None
            self._bundle = None

    # ── async internals ──────────────────────────────────────────────

    async def _async_init(self, cur_path: str) -> None:
        from openharness.ui.runtime import (
            RuntimeBundle,
            build_runtime,
            start_runtime,
        )
        import openharness

        # Build runtime with default system prompt (no override)
        self._bundle = await build_runtime(
            cwd=cur_path,
            model=self._model,
            api_key=self._api_key,
            base_url=self._base_url,
            api_format=self._api_format,
            permission_prompt=_rfm_permission_prompt,
            ask_user_prompt=_rfm_ask_user,
            permission_mode="default",
        )
        await start_runtime(self._bundle)

        # Now append rfm context to the existing system prompt
        rfm_context = _build_rfm_system_prompt_extension(cur_path)
        current_prompt = self._bundle.engine.system_prompt
        self._base_system_prompt = current_prompt + "\n\n" + rfm_context
        self._bundle.engine.set_system_prompt(self._base_system_prompt)

        # Register rfm-specific tools
        _register_rfm_tools(self._bundle.tool_registry)

        sys.stderr.write(f"🤖 OpenHarness ready (model={self._bundle.engine.model})\n")
        sys.stderr.flush()

    async def _async_ask(self, prompt: str) -> str:
        """Single-turn: inject rfm state, ask, return response."""
        from openharness.ui.runtime import handle_line
        from openharness.engine.query import MaxTurnsExceeded

        self._renderer.reset()

        # Update system prompt: base (cached at init) + dynamic rfm state
        if self._bundle is not None:
            rfm_state = _build_rfm_state_section(
                getattr(self, "_rfm_cur_path", self._bundle.cwd),
                getattr(self, "_rfm_selected_files", []),
            )
            self._bundle.engine.set_system_prompt(self._base_system_prompt + rfm_state)

        async def _print_system(msg: str) -> None:
            sys.stderr.write(f"{msg}\n")
            sys.stderr.flush()

        async def _clear() -> None:
            pass

        try:
            await handle_line(
                self._bundle,
                prompt,
                print_system=_print_system,
                render_event=self._renderer,
                clear_output=_clear,
            )
        except MaxTurnsExceeded:
            sys.stderr.write("⏱️  Max turns exceeded.\n")
            sys.stderr.flush()

        return self._renderer.collected_text

    async def _async_repl(self, initial_prompt: str = "") -> str:
        """Continuous conversation loop.  Reads from stdin until /quit."""
        from openharness.ui.runtime import handle_line
        from openharness.engine.query import MaxTurnsExceeded

        sys.stderr.write("┌─ OpenHarness continuous mode ──────────────┐\n")
        sys.stderr.write("│  Type /quit or /exit to return to rfm      │\n")
        sys.stderr.write("└────────────────────────────────────────────┘\n")
        sys.stderr.flush()

        prompt = initial_prompt.strip()
        collected = ""

        async def _print_system(msg: str) -> None:
            sys.stderr.write(f"{msg}\n")
            sys.stderr.flush()

        async def _clear() -> None:
            pass

        while True:
            if prompt:
                self._renderer.reset()
                try:
                    await handle_line(
                        self._bundle,
                        prompt,
                        print_system=_print_system,
                        render_event=self._renderer,
                        clear_output=_clear,
                    )
                except MaxTurnsExceeded:
                    sys.stderr.write("⏱️  Max turns exceeded.\n")
                    sys.stderr.flush()
                collected += self._renderer.collected_text

            # Read next prompt from user via stdin
            try:
                next_input = input("oh> ").strip()
            except (EOFError, KeyboardInterrupt):
                sys.stderr.write("\n👋 Returning to rfm\n")
                sys.stderr.flush()
                break

            if next_input in ("/quit", "/exit", "/q"):
                sys.stderr.write("👋 Returning to rfm\n")
                sys.stderr.flush()
                break

            if next_input:
                prompt = next_input
            else:
                prompt = ""  # empty → skip turn

        return collected

    async def _async_shutdown(self) -> None:
        from openharness.ui.runtime import close_runtime
        if self._bundle is not None:
            await close_runtime(self._bundle)
        sys.stderr.write("🤖 OpenHarness shut down.\n")
        sys.stderr.flush()


# ── singleton ────────────────────────────────────────────────────────

_instance: RFMOpenHarness | None = None


def get_instance() -> RFMOpenHarness:
    """Return the singleton RFMOpenHarness instance (called from C)."""
    global _instance
    if _instance is None:
        _instance = RFMOpenHarness()
    return _instance


# ── rfm system-prompt context ────────────────────────────────────────

def _build_rfm_system_prompt_extension(cur_path: str) -> str:
    """Build system prompt extension that tells the agent it's inside rfm."""
    return f"""# Host Environment
You are running as an embedded AI agent inside **rfm**, a GTK-based file manager.
The current working directory is: {cur_path}

You have full access to the filesystem through your standard tools (read_file,
write_file, edit_file, bash, etc.).  You can help the user with file management,
code analysis, git operations, and any other task.

# Important
- The user interacts with you through rfm's stdin prompt.
- You can ask for permission to run tools — the user will be prompted in the terminal.
- Be concise and efficient since the user is in a file manager context.
"""


def _build_rfm_state_section(cur_path: str, selected_files: list[str]) -> str:
    """Build a section describing current rfm state."""
    lines = ["\n# Current rfm State"]
    lines.append(f"- Current directory: {cur_path}")
    if selected_files:
        lines.append(f"- Selected files ({len(selected_files)}):")
        for f in selected_files[:20]:  # cap at 20
            lines.append(f"  - {f}")
        if len(selected_files) > 20:
            lines.append(f"  ... and {len(selected_files) - 20} more")
    else:
        lines.append("- No files currently selected")
    return "\n".join(lines) + "\n"


# ── rfm-specific tools ───────────────────────────────────────────────

def _register_rfm_tools(tool_registry) -> None:
    """Register tools that let the agent query rfm's internal state."""
    try:
        from rfmOH_tools import register_tools
        register_tools(tool_registry)
    except ImportError:
        pass  # rfmOH_tools.py is optional
