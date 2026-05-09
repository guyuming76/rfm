#!/usr/bin/env python3
"""
rfm-specific tools for the OpenHarness agent.

These tools allow the AI agent to query the host rfm process's internal state:
- current directory
- selected files
- file attributes (size, mtime, etc.)
"""

from __future__ import annotations

from typing import Any

# ── shared state (updated by rfmOpenHarness.set_rfm_state) ────────────

# These are module-level globals set by rfmOpenHarness before each turn.
# The tools read them to provide rfm context to the agent.
_cur_path: str = "/"
_selected_files: list[str] = []


def update_rfm_state(cur_path: str, selected_files: list[str]) -> None:
    """Update the shared rfm state (called by rfmOpenHarness)."""
    global _cur_path, _selected_files
    _cur_path = cur_path
    _selected_files = list(selected_files) if selected_files else []


def register_tools(tool_registry) -> None:
    """Register all rfm-specific tools into the given ToolRegistry."""
    from openharness.tools.base import BaseTool, ToolExecutionContext, ToolResult
    from pydantic import BaseModel, Field

    # ── rfm_current_dir ──────────────────────────────────────────────
    class RfmCurrentDirInput(BaseModel):
        pass

    class RfmCurrentDirTool(BaseTool):
        name = "rfm_current_dir"
        description = "Get the current working directory of the rfm file manager host process."

        input_model = RfmCurrentDirInput

        async def execute(
            self, arguments: RfmCurrentDirInput, context: ToolExecutionContext
        ) -> ToolResult:
            return ToolResult(output=_cur_path)

        def is_read_only(self, arguments: RfmCurrentDirInput) -> bool:
            return True

    # ── rfm_selected_files ───────────────────────────────────────────
    class RfmSelectedFilesInput(BaseModel):
        """Get the list of currently selected files in rfm.

        Optionally filter by extension or glob pattern.
        """
        extension: str | None = Field(default=None, description="Filter by file extension, e.g. '.c', '.py'")
        max_files: int | None = Field(default=None, description="Maximum number of files to return")

    class RfmSelectedFilesTool(BaseTool):
        name = "rfm_selected_files"
        description = (
            "Get the list of files currently selected in the rfm file manager. "
            "You can filter by file extension or limit the number of results. "
            "Each file path is absolute. Use this to understand what the user is working on."
        )

        input_model = RfmSelectedFilesInput

        async def execute(
            self, arguments: RfmSelectedFilesInput, context: ToolExecutionContext
        ) -> ToolResult:
            import fnmatch
            import os

            files = _selected_files
            if arguments.extension:
                ext = arguments.extension
                if not ext.startswith("."):
                    ext = "." + ext
                files = [f for f in files if f.endswith(ext)]
            if arguments.max_files is not None and arguments.max_files > 0:
                files = files[: arguments.max_files]

            if not files:
                return ToolResult(output="No files currently selected in rfm.")

            # Get file sizes
            lines = [f"Selected files in rfm ({len(files)} total):"]
            for f in files:
                try:
                    st = os.stat(f)
                    size = st.st_size
                    if size < 1024:
                        size_str = f"{size} B"
                    elif size < 1024 * 1024:
                        size_str = f"{size / 1024:.1f} KB"
                    else:
                        size_str = f"{size / (1024 * 1024):.1f} MB"
                    lines.append(f"  {f}  ({size_str})")
                except OSError:
                    lines.append(f"  {f}")
            return ToolResult(output="\n".join(lines))

        def is_read_only(self, arguments: RfmSelectedFilesInput) -> bool:
            return True

    # ── rfm_list_dir ─────────────────────────────────────────────────
    class RfmListDirInput(BaseModel):
        """List directory contents (uses rfm's current dir by default)."""
        path: str | None = Field(default=None, description="Directory path; defaults to rfm's current directory")
        show_hidden: bool = Field(default=False, description="Include hidden files (dotfiles)")
        max_entries: int = Field(default=100, description="Maximum number of entries to return")

    class RfmListDirTool(BaseTool):
        name = "rfm_list_dir"
        description = (
            "List the contents of the current rfm directory (or a specified path). "
            "Returns file names with type indicators (d=directory, f=file, l=symlink)."
        )

        input_model = RfmListDirInput

        async def execute(
            self, arguments: RfmListDirInput, context: ToolExecutionContext
        ) -> ToolResult:
            import os

            target = arguments.path or _cur_path
            if not os.path.isdir(target):
                return ToolResult(output=f"Not a directory: {target}", is_error=True)

            try:
                entries = sorted(os.listdir(target))
            except PermissionError:
                return ToolResult(output=f"Permission denied: {target}", is_error=True)

            if not arguments.show_hidden:
                entries = [e for e in entries if not e.startswith(".")]

            lines = [f"Contents of {target}:"]
            for entry in entries[: arguments.max_entries]:
                full = os.path.join(target, entry)
                try:
                    if os.path.islink(full):
                        prefix = "l"
                    elif os.path.isdir(full):
                        prefix = "d"
                    else:
                        prefix = "f"
                except OSError:
                    prefix = "?"
                lines.append(f"  [{prefix}] {entry}")
            if len(entries) > arguments.max_entries:
                lines.append(f"  ... and {len(entries) - arguments.max_entries} more entries")
            return ToolResult(output="\n".join(lines))

        def is_read_only(self, arguments: RfmListDirInput) -> bool:
            return True

    # ── Register all tools ───────────────────────────────────────────
    tool_registry.register(RfmCurrentDirTool())
    tool_registry.register(RfmSelectedFilesTool())
    tool_registry.register(RfmListDirTool())
