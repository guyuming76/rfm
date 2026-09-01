# rfm_pybridge.py
# GPL-3 by guyuming
#
# 公共 Python 侧胶水层，供嵌入式 Python 模块（如 rfm_prompt_toolkit.core）复用：
#   1. GPtrArray ctypes 结构体（GLib struct _GPtrArray）
#   2. 实时读取 C 侧 setenv 的环境变量（绕过 os.environ 快照）
#   3. PyCapsule → ctypes 可调用对象转换

import ctypes as ct


# ── GPtrArray ctypes 结构体 ─────────────────────────────────
# GLib struct _GPtrArray { gpointer *pdata; guint len; };

class GPtrArray(ct.Structure):
    _fields_ = [("pdata", ct.POINTER(ct.c_void_p)), ("len", ct.c_uint)]


# ── 实时读取环境变量 ─────────────────────────────────────────
# 注意：在 Python 嵌入模式下，os.getenv / os.environ 是 CPython 在 os 模块
# 首次使用时对 libc environ 的一次性快照。C 侧在 Python 初始化之后通过
# g_setenv() 做的修改，无法被 os.getenv 感知（实测如此）。
# 因此这里用 ctypes 直接调用 libc 的 getenv()，它始终读取当前进程的实时 environ。
_libc = ct.CDLL(None)
_libc.getenv.restype = ct.c_char_p
_libc.getenv.argtypes = [ct.c_char_p]


def getenv(name: str):
    """读取当前进程实时环境变量（绕过 os.environ 快照）。"""
    try:
        val = _libc.getenv(name.encode("utf-8"))
    except Exception:
        return None
    return val.decode("utf-8") if val else None


def capsule_to_cfunc(module, capsule_attr_name: str, cfunc_type):
    """从 module 的 PyCapsule 属性中提取函数指针并转换为 ctypes 可调用对象。

    参数:
        module: 被 C 侧注入 PyCapsule 属性的模块对象（通常是 sys.modules[__name__]）。
        capsule_attr_name: PyCapsule 属性名。
        cfunc_type: ct.CFUNCTYPE 类型（函数签名）。

    返回:
        可调用的 ctypes 函数对象；失败返回 None。
    """
    capsule = getattr(module, capsule_attr_name, None)
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
    except Exception:
        return None
    return None
