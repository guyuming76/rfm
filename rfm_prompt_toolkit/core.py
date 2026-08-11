# rfm_prompt_toolkit/core.py
# GPL-3 by guyuming
#
# 核心 readline 模块，在 Python 嵌入线程中运行，
# 通过 PyCapsule → ctypes 桥接调用 C 侧补全函数。
#
# 注意：本模块被 rfm_pt_wrapper.c 通过 PyImport_ImportModule 导入，
# C 侧在导入后会通过 PyObject_SetAttrString 注入以下 PyCapsule 对象：
#   _c_get_completion_fn_capsule
#   _c_get_menu_completion_fn_capsule
#   _c_handle_escape_fn_capsule
#   _c_toggle_recursive_completion_capsule

import os
import sys
import ctypes as ct
import logging

try:
    from prompt_toolkit.completion import Completer
except ImportError:
    Completer = object

logger = logging.getLogger(__name__)
if not logger.handlers:
    _h = logging.StreamHandler(sys.stderr)
    _h.setFormatter(logging.Formatter("[%(name)s] %(message)s"))
    logger.addHandler(_h)
    logger.setLevel(logging.DEBUG)

# ── 常量 ────────────────────────────────────────────────────
REEDLINE_INTERRUPTED = "reedline_interrupted"

# ── 全局状态 ────────────────────────────────────────────────
_selection_star = False
_completion_mode = "tab"  # "tab" | "menu" | "shift_tab"


def set_selection_star(show: bool):
    """C 接口：设置选中标记"""
    global _selection_star
    _selection_star = show


# ── PyCapsule → ctypes 可调用对象转换 ────────────────────────

# 这些在第一次使用时从注入的 PyCapsule 中提取
_c_get_completion_callable = None
_c_get_menu_completion_callable = None
_c_handle_escape_callable = None
_c_toggle_recursive_callable = None

# C 函数签名
# GetCompletionFn: f(const char* text, int* count) -> GPtrArray*
_GET_COMPLETION_CFUNC = ct.CFUNCTYPE(
    ct.c_void_p,  # GPtrArray*
    ct.c_char_p,   # const char* text
    ct.POINTER(ct.c_int),  # int* count
)
_ESCAPE_CFUNC = ct.CFUNCTYPE(None)
_TOGGLE_RECURSIVE_CFUNC = ct.CFUNCTYPE(None)


def _capsule_to_cfunc(capsule_attr_name, cfunc_type):
    """从 PyCapsule 属性中提取函数指针并转换为 ctypes 可调用对象"""
    import sys
    core_mod = sys.modules.get("rfm_prompt_toolkit.core")
    if core_mod is None:
        return None

    capsule = getattr(core_mod, capsule_attr_name, None)
    if capsule is None:
        return None

    # 通过 ctypes 调用 CPython C API: PyCapsule_GetPointer
    try:
        pyapi = ct.pythonapi
        pyapi.PyCapsule_GetPointer.argtypes = [ct.py_object, ct.c_char_p]
        pyapi.PyCapsule_GetPointer.restype = ct.c_void_p
        ptr_val = pyapi.PyCapsule_GetPointer(capsule, None)
        if ptr_val:
            return ct.cast(ptr_val, cfunc_type)
    except Exception as e:
        logger.debug("Failed to extract capsule %s: %s", capsule_attr_name, e)
    return None


def _get_completion_fn():
    global _c_get_completion_callable
    if _c_get_completion_callable is None:
        _c_get_completion_callable = _capsule_to_cfunc(
            "_c_get_completion_fn_capsule", _GET_COMPLETION_CFUNC
        )
    return _c_get_completion_callable


def _get_menu_completion_fn():
    global _c_get_menu_completion_callable
    if _c_get_menu_completion_callable is None:
        _c_get_menu_completion_callable = _capsule_to_cfunc(
            "_c_get_menu_completion_fn_capsule", _GET_COMPLETION_CFUNC
        )
    return _c_get_menu_completion_callable


def _get_escape_fn():
    global _c_handle_escape_callable
    if _c_handle_escape_callable is None:
        _c_handle_escape_callable = _capsule_to_cfunc(
            "_c_handle_escape_fn_capsule", _ESCAPE_CFUNC
        )
    return _c_handle_escape_callable


def _get_toggle_recursive_fn():
    global _c_toggle_recursive_callable
    if _c_toggle_recursive_callable is None:
        _c_toggle_recursive_callable = _capsule_to_cfunc(
            "_c_toggle_recursive_completion_capsule", _TOGGLE_RECURSIVE_CFUNC
        )
    return _c_toggle_recursive_callable


# ── prompt_toolkit 导入检测 ──


def _ensure_prompt_toolkit():
    """检查 prompt_toolkit 是否可用"""
    try:
        import prompt_toolkit
        v = tuple(int(x) for x in prompt_toolkit.__version__.split(".")[:2])
        if v < (3, 0):
            raise ImportError(f"prompt_toolkit >= 3.0 required, found {prompt_toolkit.__version__}")
    except ImportError:
        print(
            "[rfm] prompt_toolkit not found. Install it with: pip install prompt_toolkit",
            file=sys.stderr,
        )
        raise


# ── RfmCompleter ────────────────────────────────────────────────

class RfmCompleter(Completer):
    """prompt_toolkit Completer，桥接 C 补全函数。

    补全模式由全局 _completion_mode 控制：
    - "tab"       → get_completion_fn（文件名/搜索结果/维度）
    - "menu"      → get_menu_completion_fn（菜单/工具栏/内置命令）
    - "shift_tab" → toggle_recursive + get_completion_fn + toggle_recursive
    """

    def get_completions(self, document, complete_event):
        from prompt_toolkit.completion import Completion

        global _completion_mode

        mode = _completion_mode
        _completion_mode = "tab"  # 重置

        logger.debug("get_completions: mode=%s, text=%r, cursor=%d",
                     mode, document.text_before_cursor, document.cursor_position)

        if mode == "shift_tab":
            toggle_fn = _get_toggle_recursive_fn()
            if toggle_fn:
                toggle_fn()
            completion_fn = _get_completion_fn()
            if completion_fn:
                yield from self._do_complete(document, completion_fn, is_menu=False)
            if toggle_fn:
                toggle_fn()
        elif mode == "menu":
            completion_fn = _get_menu_completion_fn()
            if completion_fn:
                yield from self._do_complete(document, completion_fn, is_menu=True)
        else:
            # "tab" — 先 C 补全，fallback 到 WordCompleter
            completion_fn = _get_completion_fn()
            logger.debug("get_completions: completion_fn=%s", completion_fn is not None)
            if completion_fn:
                yielded = False
                for c in self._do_complete(document, completion_fn, is_menu=False):
                    yielded = True
                    yield c
                if yielded:
                    logger.debug("get_completions: C completer yielded, skip fallback")
                    return
            # Fallback: 文件路径补全（prompt_toolkit 内置）
            logger.debug("get_completions: falling back to PathCompleter")
            from prompt_toolkit.completion import PathCompleter
            _path_completer = PathCompleter(expanduser=True)
            yield from _path_completer.get_completions(document, complete_event)

    def _do_complete(self, document, completion_fn, is_menu: bool):
        from prompt_toolkit.completion import Completion

        line = document.text_before_cursor
        pos = document.cursor_position

        # 确定补全前缀（模拟 reedline 的 context_for_cursor 逻辑）
        before_cursor = line[:pos]
        if before_cursor:
            last_space = before_cursor.rfind(" ")
            if last_space >= 0:
                start_pos = last_space + 1
                word = before_cursor[start_pos:]
            else:
                start_pos = 0
                word = before_cursor
        else:
            start_pos = pos
            word = ""

        logger.debug("_do_complete: word=%r, is_menu=%s", word, is_menu)

        # 编码为 bytes 传给 C
        text_bytes = word.encode("utf-8") + b"\x00"
        count = ct.c_int(0)

        # ★ GIL 安全：ctypes 调用 C 函数时自动释放 GIL
        #    rfm_menu_completion 内部通过 g_cond_wait 等待 GTK 线程完成，
        #    如果持有 GIL 会导致死锁。ctypes 自动处理这一点。
        try:
            result_ptr = completion_fn(text_bytes, ct.byref(count))
        except Exception as e:
            logger.debug("C completion call failed: %s", e)
            return

        logger.debug("_do_complete: result_ptr=%s, count=%d",
                     hex(result_ptr) if result_ptr else "NULL", count.value)

        if not result_ptr:
            logger.debug("_do_complete: NULL result_ptr, returning")
            return

        # 解析 GPtrArray
        # GPtrArray 内存布局 (glib garray.h):
        #   offset 0 = pdata (gpointer*, 8 bytes on 64-bit)
        #   offset 8 = len   (guint,    4 bytes)
        # 注意：C 函数 rfm_filename_completion 的 else 分支未设置 *count，
        #   因此以 GPtrArray 自身的 len 为准。
        try:
            pdata = ct.cast(result_ptr, ct.POINTER(ct.POINTER(ct.c_char)))
            gp_array_len = ct.cast(result_ptr + 8, ct.POINTER(ct.c_uint))[0]
        except Exception as e:
            logger.debug("_do_complete: GPtrArray parse failed: %s", e)
            return

        logger.debug("_do_complete: gp_array_len=%d, count=%d", gp_array_len, count.value)

        # count.value 仅在 ItemSelected 分支被设置；else 分支未设置。
        # 优先使用 GPtrArray 自身的 len，其次用 *count。
        num = gp_array_len if gp_array_len > 0 else count.value
        if num <= 0:
            logger.debug("_do_complete: num=%d, no items", num)
            return

        logger.debug("_do_complete: iterating %d items", num)
        for i in range(num):
            item_ptr = pdata[i]
            if not item_ptr:
                continue
            try:
                raw = ct.cast(item_ptr, ct.c_char_p).value
                if not raw:
                    continue
                result_str = raw.decode("utf-8", errors="replace")
                if not result_str:
                    continue

                logger.debug("_do_complete: item[%d]=%r", i, result_str)
                display, style = _parse_completion_color(result_str)
                # prompt_toolkit Completion
                # start_position: 负偏移量，表示从光标前几个字符开始替换
                yield Completion(
                    display,
                    start_position=-len(word) if word else 0,
                    style=style,
                )
            except Exception:
                continue


def _parse_completion_color(result_str: str):
    """根据前缀解析补全项颜色和显示文本，与 reedline lib.rs 逻辑一致"""
    if result_str.startswith("|"):
        return result_str[1:], "fg:ansired"
    elif result_str.startswith("}"):
        return " " + result_str[1:], "fg:ansiyellow"
    elif result_str.startswith("="):
        return result_str, "fg:ansiyellow"
    elif result_str.startswith("*"):
        return result_str[1:], "fg:ansiyellow"
    elif result_str.startswith("#"):
        return result_str[1:], "fg:ansigreen"
    else:
        return result_str, "fg:ansiblue"


# ── RfmPromptToolkit 主类 ──────────────────────────────────


class RfmPromptToolkit:
    """包装 prompt_toolkit.PromptSession"""

    def __init__(self, history_path: str):
        from prompt_toolkit.history import FileHistory
        from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
        from prompt_toolkit.styles import Style

        self._history_path = history_path

        try:
            self._history = FileHistory(history_path)
        except Exception:
            self._history = None

        self._key_bindings = self._create_key_bindings()
        self._completer = RfmCompleter()

        self._style = Style.from_dict({
            "completion-menu.completion": "bg:#333333 #ffffff",
            "completion-menu.completion.current": "bg:#666666 #000000",
            "suggestion": "#666666 italic",
            # 颜色类样式通过 Completion.style 字段设置
        })

        from prompt_toolkit.shortcuts import PromptSession
        self._session = PromptSession(
            history=self._history,
            key_bindings=self._key_bindings,
            completer=self._completer,
            auto_suggest=AutoSuggestFromHistory(),
            style=self._style,
            complete_while_typing=False,
            multiline=True,
            enable_history_search=True,
            wrap_lines=False,
        )

    def _create_key_bindings(self):
        from prompt_toolkit.key_binding import KeyBindings
        from prompt_toolkit.keys import Keys

        kb = KeyBindings()

        @kb.add(Keys.ControlM, eager=True)
        def _(event):
            """Enter → 提交"""
            text = event.current_buffer.text
            if text.rstrip().endswith("\\"):
                event.current_buffer.insert_text("\n")
            else:
                event.current_buffer.validate_and_handle()

        @kb.add(Keys.Escape, Keys.ControlM, eager=True)
        def _(event):
            """Alt+Enter → 插入换行"""
            event.current_buffer.insert_text("\n")

        @kb.add(Keys.Tab)
        def _(event):
            """Tab → C 补全"""
            global _completion_mode
            _completion_mode = "tab"
            b = event.current_buffer
            if b.complete_state:
                b.complete_next()
            else:
                b.start_completion(select_first=False)

        @kb.add(Keys.BackTab)
        def _(event):
            """Shift+Tab → 切换递归 + 补全"""
            global _completion_mode
            _completion_mode = "shift_tab"
            b = event.current_buffer
            if b.complete_state:
                b.complete_previous()
            else:
                b.start_completion(select_first=False)

        @kb.add(Keys.F2)
        def _(event):
            """F2 → 菜单命令补全"""
            global _completion_mode
            _completion_mode = "menu"
            b = event.current_buffer
            b.start_completion(select_first=False)

        @kb.add(Keys.Escape)
        def _(event):
            """Esc → C 回调 cmd_handle_escape_key"""
            fn = _get_escape_fn()
            if fn:
                fn()

        @kb.add(Keys.Escape, Keys.Up, eager=True)
        def _(event):
            """Alt+Up → rfm Up (返回上级目录)"""
            event.app.exit(result="u")

        @kb.add(Keys.Escape, Keys.Left, eager=True)
        def _(event):
            """Alt+Left → rfm Previous"""
            event.app.exit(result="l")

        @kb.add(Keys.Escape, Keys.Right, eager=True)
        def _(event):
            """Alt+Right → rfm Next"""
            event.app.exit(result="r")

        return kb

    def readline(self, prompt_text: str) -> str:
        """阻塞读取一行。返回值: 字符串 / None (Ctrl+C/D) / HostCommand"""

        # 使用可调用对象而非静态字符串，使 _selection_star 变化后
        # invalidate() → 重绘时能动态反映 * 的显示/隐藏
        def _dynamic_prompt():
            return self._build_prompt(prompt_text)

        try:
            result = self._session.prompt(
                _dynamic_prompt,
                key_bindings=self._key_bindings,
            )
            # result 可能是字符串（用户输入）或 HostCommand（app.exit(result=...)）
            if result is None:
                return None
            return str(result)

        except KeyboardInterrupt:
            return None
        except EOFError:
            return None

    def interrupt(self):
        """发送 SIGINT 以中断阻塞的 prompt()
        注意：模块级 interrupt() 已设置 _sigint_received 标志，此处只负责发信号。"""
        import os
        import signal
        os.kill(os.getpid(), signal.SIGINT)

    def request_repaint(self):
        """请求重绘"""
        try:
            if hasattr(self._session, 'app') and self._session.app:
                self._session.app.invalidate()
        except Exception:
            pass

    def save_history(self):
        """保存历史"""
        try:
            if self._history:
                # FileHistory 的 save 在 prompt_toolkit 中通过 flush 实现
                pass
        except Exception:
            pass

    def _build_prompt(self, base_prompt: str) -> str:
        """动态插入选中星号"""
        global _selection_star
        if not _selection_star or not base_prompt:
            return base_prompt
        last_char = base_prompt[-1]
        if last_char in (">", "]"):
            return base_prompt[:-1] + "*" + last_char
        return base_prompt


# ── 全局实例 ────────────────────────────────────────────────

_session_instance: RfmPromptToolkit = None

# SIGINT 标志：由 signal handler 设置，readline() 读取后重置
# signal handler 在 create_handle() 中安装（运行在主线程）
_sigint_received = False


def _sigint_handler(signum, frame):
    """SIGINT 处理器 — 只设置标志位，不抛异常。
    KeyboardInterrupt 由 CPython C 层 eval breaker 机制负责触发。"""
    global _sigint_received
    _sigint_received = True


# ── C 可调用接口 ────────────────────────────────────────────


def create_handle(history_path: str):
    """初始化 session。
    在主线程中调用（pt_create → GTK main thread），在此安装 SIGINT handler。"""
    global _session_instance
    _ensure_prompt_toolkit()
    _session_instance = RfmPromptToolkit(history_path)

    # 在主线程安装 SIGINT handler（signal.signal 主线程限制）
    import signal
    try:
        signal.signal(signal.SIGINT, _sigint_handler)
    except ValueError:
        # 如果不在主线程（异常情况），忽略
        logger.warning("Cannot install SIGINT handler (not in main thread)")

    return _session_instance


def readline(prompt_text: str):
    """阻塞读取一行。
    在 readline 线程中调用，不直接操作 signal。
    通过模块级 _sigint_received 标志判断是否被 interrupt() 中断。"""
    global _session_instance, _sigint_received
    if _session_instance is None:
        raise RuntimeError("RfmPromptToolkit not initialized. Call create_handle() first.")

    _sigint_received = False

    try:
        result = _session_instance.readline(prompt_text)
        if _sigint_received:
            return REEDLINE_INTERRUPTED
        if result is None:
            return None
        return result
    except KeyboardInterrupt:
        if _sigint_received:
            return REEDLINE_INTERRUPTED
        return None


def interrupt():
    """中断当前 readline — 从 GTK 主线程调用。
    通过 os.kill 发送 SIGINT 到进程，C 层信号处理器触发 eval breaker，
    使 readline 线程中的 prompt() 抛出 KeyboardInterrupt。"""
    global _sigint_received
    _sigint_received = True
    import os
    import signal
    os.kill(os.getpid(), signal.SIGINT)


def request_repaint():
    """请求重绘"""
    global _session_instance
    if _session_instance:
        _session_instance.request_repaint()


def save_history():
    """保存历史"""
    global _session_instance
    if _session_instance:
        _session_instance.save_history()


def free_handle():
    """释放资源"""
    global _session_instance
    if _session_instance:
        _session_instance.save_history()
        _session_instance = None


def edit_file(filename: str) -> int:
    """编辑文件（rfmEdit）"""
    _ensure_prompt_toolkit()

    from prompt_toolkit.shortcuts import PromptSession
    from prompt_toolkit.key_binding import KeyBindings
    from prompt_toolkit.keys import Keys
    from prompt_toolkit.history import InMemoryHistory

    try:
        with open(filename, "r") as f:
            content = f.read()
    except FileNotFoundError:
        content = ""
    except Exception as e:
        logger.error("Failed to read %s: %s", filename, e)
        return 0

    saved = False

    kb = KeyBindings()

    @kb.add(Keys.ControlO)
    def _(event):
        nonlocal saved
        try:
            with open(filename, "w") as f:
                f.write(event.current_buffer.text)
            saved = True
            event.app.exit()
        except Exception as e:
            logger.error("Save failed: %s", e)

    @kb.add(Keys.ControlX)
    def _(event):
        event.app.exit()

    @kb.add(Keys.Escape)
    def _(event):
        fn = _get_escape_fn()
        if fn:
            fn()

    edit_session = PromptSession(
        history=InMemoryHistory(),
        key_bindings=kb,
        multiline=True,
    )

    print("\x1b[33mCtrl+o save and exit; Ctrl+x exit without saving\x1b[0m")

    try:
        edit_session.prompt("[rfmEdit] ", default=content, key_bindings=kb)
    except (KeyboardInterrupt, EOFError):
        pass

    return 2 if saved else 1
