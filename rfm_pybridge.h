/*
 * rfm_pybridge.h — 公共 C↔Python 嵌入胶水层
 * GPL-3 by guyuming
 *
 * 目标：消除 rfm.c / rfm_pt_wrapper.c / 各 .rfm_addon.c 插件里大量重复的
 * GIL 管理、引用计数、import/getattr/call/错误处理样板代码。
 *
 * 用法：
 *   1. 函数体开头写 RFM_PY_GIL_SCOPE;（之后任意 return 都会自动释放 GIL）
 *   2. 用 RFM_PY_AUTO(x) 声明临时 PyObject*，作用域结束自动 Py_DECREF
 *   3. 调用 rfm_py_import() / rfm_py_call_attr() 等高阶函数
 *
 * 依赖：GCC/Clang 的 __attribute__((cleanup))（Linux 平台可用）。
 *
 * 本头文件会被 rfm.c 和各个插件 .c 直接包含。
 * 非 inline 的函数实现在 rfm_pybridge.c 中，编译进 rfm 主程序；
 * 插件通过 -rdynamic 导出的符号在 dlopen 时解析（与 add_toolbar_buttons 等一致）。
 */
#ifndef RFM_PYBRIDGE_H
#define RFM_PYBRIDGE_H

/* Python.h 先于 glib.h 包含，避免 _POSIX_C_SOURCE 重定义警告 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <glib.h>

/* ── 1. GIL 作用域守卫 ─────────────────────────────────────────────── */
/* 进入作用域自动 PyGILState_Ensure()，离开作用域（含任意 return）自动 Release。 */
typedef struct { PyGILState_STATE state; } RfmPyGil;

static inline RfmPyGil rfm_py_gil_acquire(void) {
    RfmPyGil g = { PyGILState_Ensure() };
    return g;
}
static inline void rfm_py_gil_release(RfmPyGil *g) {
    if (g) PyGILState_Release(g->state);
}

#define RFM_PY_GIL_SCOPE \
    RfmPyGil __rfm_py_gil_ __attribute__((cleanup(rfm_py_gil_release))) = rfm_py_gil_acquire()

/* ── 2. PyObject* 自动 DECREF ──────────────────────────────────────── */
static inline void rfm_py_decref_cleanup(PyObject **pp) {
    if (pp && *pp) { Py_DECREF(*pp); *pp = NULL; }
}

/* 声明一个作用域内自动释放的临时 PyObject*，例如：
 *   RFM_PY_AUTO PyObject *result = rfm_py_call_attr(...);
 * 离开作用域时自动 Py_DECREF（无论正常返回还是提前 return）。
 */
#define RFM_PY_AUTO __attribute__((cleanup(rfm_py_decref_cleanup)))

/* ── 3. 统一错误打印 + 清除 ────────────────────────────────────────── */
/* 返回 FALSE，便于调用方直接 return。当前无异常时不打印。 */
static inline gboolean rfm_py_clear_error(const char *where) {
    if (PyErr_Occurred()) {
        if (where) g_warning("%s:", where);
        PyErr_Print();   /* PyErr_Print 会清除错误指示器 */
    }
    PyErr_Clear();
    return FALSE;
}

/* ── 4. 高阶辅助函数（实现在 rfm_pybridge.c） ──────────────────────── */

/* 导入 Python 模块；path_dir 非 NULL 时先把它插入 sys.path[0]（若不存在）。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_import(const char *module_name, const char *path_dir);

/* 取属性，返回 new reference（等价 PyObject_GetAttrString 但带 NULL 检查）。 */
PyObject *rfm_py_getattr(PyObject *obj, const char *name);

/* 调用 callable(args, kwargs)；args/kwargs 均可为 NULL（NULL 表示空）。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_call(PyObject *callable, PyObject *args, PyObject *kwargs);

/* 调用 obj.method(args, kwargs)；args/kwargs 均可为 NULL。 */
PyObject *rfm_py_call_method_obj(PyObject *obj, const char *method, PyObject *args, PyObject *kwargs);

/* 调用 obj.attr(fmt, ...)。
 * fmt 遵循 Py_BuildValue 格式：可直接写 "(s)"、"(O)"、"(OO)" 产出一个 tuple，
 * 也可写单个格式单元（如 "s"、"O"），此时会自动包成 1-tuple（兼容 PyObject_CallMethod 的写法）；
 * fmt 为 NULL 表示无参调用。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_call_attr(PyObject *obj, const char *attr, const char *fmt, ...);

/* 调用 obj.method(fmt, ...)，与 rfm_py_call_attr 语义一致。 */
PyObject *rfm_py_call_method(PyObject *obj, const char *method, const char *fmt, ...);

/* 把 Python 对象转成 C 字符串（g_malloc，调用方 g_free）。
 * obj 为 NULL 或 Py_None 时返回 NULL；失败时设置 Python 异常。 */
gchar *rfm_py_to_cstr(PyObject *obj);

#endif /* RFM_PYBRIDGE_H */
