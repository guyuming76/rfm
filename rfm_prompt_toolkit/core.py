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
    logger.setLevel(logging.WARNING)


# ── GPtrArray ctypes 结构体 ─────────────────────────────────
# GLib struct _GPtrArray { gpointer *pdata; guint len; };

class _GPtrArray(ct.Structure):
    _fields_ = [("pdata", ct.POINTER(ct.c_void_p)), ("len", ct.c_uint)]

# ── 常量 ────────────────────────────────────────────────────
REEDLINE_INTERRUPTED = "reedline_interrupted"

# ── 实时读取环境变量 ─────────────────────────────────────────
# 注意：在 Python 嵌入模式下，os.getenv / os.environ 是 CPython 在 os 模块
# 首次使用时对 libc environ 的一次性快照。C 侧在 Python 初始化之后通过
# g_setenv("rfm_curPath", ...) 做的修改，无法被 os.getenv 感知（实测如此）。
# 因此这里用 ctypes 直接调用 libc 的 getenv()，它始终读取当前进程的实时 environ。
_libc = ct.CDLL(None)
_libc.getenv.restype = ct.c_char_p
_libc.getenv.argtypes = [ct.c_char_p]


def _getenv(name: str):
    """读取当前进程实时环境变量（绕过 os.environ 快照）。"""
    try:
        val = _libc.getenv(name.encode("utf-8"))
    except Exception:
        return None
    return val.decode("utf-8") if val else None

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

    relative_to: 可选目录路径。当传入时（rfmEdit 场景），文件路径补全结果
    会被转换为相对于该目录的路径，与 reedline 版 CCompleter 的
    use_relative_path 行为一致。
    """

    def __init__(self, relative_to: str = None):
        self._relative_to = relative_to
        super().__init__()

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

        # 确定补全前缀（模拟 reedline 的 context_for_cursor 逻辑：
        # 先定位到光标所在的当前行，再取当前行内最后一个空格之后的部分）。
        # rfmEdit 是多行编辑器，若用整段文本 rfind(" ") 会误取上一行的空格，
        # 导致换行后按 TAB 无法触发"补全当前选中文件"（word 不为空）。
        last_newline = line.rfind("\n")
        current_line = line[last_newline + 1:] if last_newline >= 0 else line
        last_space = current_line.rfind(" ")
        if last_space >= 0:
            word = current_line[last_space + 1:]
        else:
            word = current_line

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

        # 解析 GPtrArray — 使用 ctypes Structure 精确定义
        # GLib struct _GPtrArray { gpointer *pdata; guint len; };
        try:
            parray = ct.cast(result_ptr, ct.POINTER(_GPtrArray)).contents
            gp_array_len = parray.len
            pdata_ptr = parray.pdata  # gpointer* → 数组首地址
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
            item_ptr = pdata_ptr[i]  # c_void_p，即文件中某个 gpointer 值
            if not item_ptr:
                continue
            try:
                raw = ct.cast(item_ptr, ct.c_char_p).value
                if not raw:
                    continue
                logger.debug("_do_complete: item[%d] raw bytes=%r", i, raw)
                result_str = raw.decode("utf-8", errors="replace")
                if not result_str:
                    continue

                logger.debug("_do_complete: item[%d]=%r", i, result_str)
                # rfmEdit 场景：文件路径补全结果（无 | } = * # 前缀）转换为
                # 相对于被编辑文件所在目录的路径，与 reedline 版
                # CCompleter 的 use_relative_path 行为一致。
                # 注意：仅转换绝对路径；相对路径（如 "./foo"）保持原样，
                # 与 reedline 的 pathdiff::diff_paths 行为一致（相对/绝对
                # 不一致时返回 None → 保留原值）。
                if self._relative_to and os.path.isabs(result_str) \
                        and not result_str.startswith(("|", "}", "=", "*", "#")):
                    try:
                        result_str = os.path.relpath(result_str, self._relative_to)
                    except ValueError:
                        pass  # 路径跨设备等情况，保留原值
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

        from prompt_toolkit.shortcuts import PromptSession, CompleteStyle
        self._session = PromptSession(
            history=self._history,
            key_bindings=self._key_bindings,
            completer=self._completer,
            auto_suggest=AutoSuggestFromHistory(),
            style=self._style,
            complete_while_typing=False,
            #complete_style=CompleteStyle.READLINE_LIKE,
            multiline=True,
            enable_history_search=True,
            wrap_lines=True,
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
        @kb.add(Keys.F16)  # Menu 键 → ^[[29~ → prompt_toolkit 映射为 F16
        def _(event):
            """F2 / Menu 键 → 菜单命令补全"""
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
        global _current_app

        # 使用可调用对象而非静态字符串，使 _selection_star 变化后
        # invalidate() → 重绘时能动态反映 * 的显示/隐藏
        def _dynamic_prompt():
            return self._build_prompt(prompt_text)

        def _capture_app():
            # pre_run 在 app 的事件循环线程（readline 线程）内、set_app 之后执行，
            # 此时 get_app() 可返回正在运行的 Application。
            global _current_app, _sigint_received
            from prompt_toolkit.application.current import get_app
            app = get_app()
            _current_app = app
            # 补漏：若 interrupt() 在本线程进入事件循环之前已被调用（此时
            # _current_app 尚未捕获，interrupt() 无法通过 call_soon_threadsafe 唤醒），
            # 在这里主动注入 send_sigint 触发 KeyboardInterrupt，保证不漏中断。
            # 用 call_soon 而非直接调用，确保事件循环完全就绪（input attach 等）后再处理。
            if _sigint_received:
                app.loop.call_soon(app.key_processor.send_sigint)

        def _bottom_toolbar():
            if _getenv("RFM_showHelpOnStart") == "false":
                # -n 启动时 C 侧会 setenv("RFM_showHelpOnStart", "false")，此时显示当前路径
                return _getenv("rfm_curPath") or ""
            # 默认（环境变量不存在或不为 "false"）显示帮助提示
            return "[Shift+]TAB[包含子目录]补全 Alt+Enter多行编辑添加行 F2工具栏|文件菜单命令 Ctrl+r在历史输入中搜索"

        try:
            # 在 -n 模式（RFM_showHelpOnStart=false）下，用 rprompt 在输入行右侧显示
            # 当前路径，而不是用 bottom_toolbar 在屏幕底部绘制。
            # bottom_toolbar 依赖 CPR + 精确光标跟踪，且要在屏幕底部“向下滚动”绘制；
            # 在 Wayland(foot) 下窗口 resize（dwl 的 master/stack 切换）时，foot 会先
            # reflow 网格，而 prompt_toolkit 的事件循环跑在非主线程（readline 线程），
            # SIGWINCH 被禁用，只能靠 0.5s 轮询发现尺寸变化，此时内部光标位置已与终端
            # 实际位置不一致，erase/redraw 会漏擦底部那行路径，导致每切换一次就残留一行
            # 重复的“路径 + 提示符”。改为 rprompt（画在输入行右侧）即可绕开这个脆弱路径。
            _show_path_rprompt = (_getenv("RFM_showHelpOnStart") == "false")
            result = self._session.prompt(
                _dynamic_prompt,
                key_bindings=self._key_bindings,
                pre_run=_capture_app,
                rprompt=(_getenv("rfm_curPath") or "") if _show_path_rprompt else None,
                bottom_toolbar=None if _show_path_rprompt else _bottom_toolbar(),
            )
            # result 可能是字符串（用户输入）或 HostCommand（app.exit(result=...)）
            if result is None:
                return None
            return str(result)

        except KeyboardInterrupt:
            return None
        except EOFError:
            return None
        finally:
            # prompt() 返回后 app.loop 已被 set_loop 清空，避免 interrupt()
            # 继续持有一个已结束的 Application。
            _current_app = None

    def interrupt(self):
        """中断阻塞的 prompt()。实际逻辑见模块级 interrupt()。"""
        interrupt()

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

# 运行中的 prompt_toolkit Application 引用。
# 由 readline() 的 pre_run 回调在 readline 线程（事件循环线程）中捕获，
# interrupt()（运行在 GTK 主线程）通过它调用 loop.call_soon_threadsafe
# 向事件循环注入 send_sigint，可靠地唤醒阻塞在 select() 上的 readline 线程。
# （简单的对象引用赋值/读取在 GIL 下是原子的；interrupt() 经由 pt_interrupt
#   通过 PyGILState_Ensure 持有 GIL 调用，故不存在竞态。）
_current_app = None


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

    # 若 interrupt() 在本次 readline 真正开始前已被调用（例如 readline 线程
    # 尚在启动、还未进入 prompt() 时，GTK 线程就执行了 CancelReadInput→pt_interrupt），
    # 则立即返回 REEDLINE_INTERRUPTED 并消费该标志，避免后续 g_thread_join 死等。
    if _sigint_received:
        _sigint_received = False
        return REEDLINE_INTERRUPTED

    try:
        result = _session_instance.readline(prompt_text)
        if _sigint_received:
            _sigint_received = False
            return REEDLINE_INTERRUPTED
        if result is None:
            return None
        return result
    except KeyboardInterrupt:
        if _sigint_received:
            _sigint_received = False
            return REEDLINE_INTERRUPTED
        return None


def interrupt():
    """中断当前 readline — 从 GTK 主线程调用。

    通过 app.loop.call_soon_threadsafe 向 readline 线程的事件循环注入
    key_processor.send_sigint —— 这正是 prompt_toolkit 自身在主线程收到
    SIGINT 时做的事。call_soon_threadsafe 会向 asyncio 的 self-pipe 写字节，
    从而唤醒阻塞在 select() 上的 readline 线程，随后 <sigint> 键绑定
    （PromptSession 内置）调用 app.exit(exception=KeyboardInterrupt)，
    使 prompt() 抛出 KeyboardInterrupt。

    不能用 os.kill(SIGINT)：Python 的 signal handler 只能在主线程执行，
    而 readline 线程阻塞在 select() 时 SIGINT 只会设置 eval breaker 标志，
    既不唤醒 select（PEP 475 会让 select 重试），也不抛 KeyboardInterrupt，
    这正是 g_thread_join 长时间阻塞的根因。
    """
    global _sigint_received, _current_app
    _sigint_received = True

    app = _current_app
    if app is None:
        return
    loop = getattr(app, "loop", None)
    if loop is None or loop.is_closed():
        return
    try:
        loop.call_soon_threadsafe(app.key_processor.send_sigint)
    except (RuntimeError, AttributeError):
        # 事件循环已停止或 app 结构异常时忽略
        pass


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

    @kb.add(Keys.Tab)
    def _(event):
        """Tab → C 文件补全（补全当前 rfm GUI 选中的文件）"""
        global _completion_mode
        _completion_mode = "tab"
        b = event.current_buffer
        if b.complete_state:
            b.complete_next()
        else:
            b.start_completion(select_first=False)

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

    # 补全器：桥接 C 侧文件名补全，并把补全结果转为相对于被编辑文件
    # 所在目录的路径（与 reedline 版 RfmEditor 的 use_relative_path 一致）
    relative_to = os.path.dirname(os.path.abspath(filename))
    completer = RfmCompleter(relative_to=relative_to)

    edit_session = PromptSession(
        history=InMemoryHistory(),
        key_bindings=kb,
        completer=completer,
        multiline=True,
        bottom_toolbar="Ctrl+x不保存退出 Ctrl+o保存退出 Ctrl+(r|s)(后向|前向)搜索",
    )

    #print("\x1b[33mCtrl+o save and exit; Ctrl+x exit without saving\x1b[0m")

    try:
        edit_session.prompt("[rfmEdit] ", default=content, key_bindings=kb)
    except (KeyboardInterrupt, EOFError):
        pass

    return 2 if saved else 1
