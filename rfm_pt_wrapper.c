// rfm_pt_wrapper.c
// GPL-3 by guyuming
//
// Python prompt_toolkit 与 rfm C 代码之间的胶水层。
// 负责：
//   1. 导入 rfm_prompt_toolkit 模块
//   2. 将 C 补全函数指针通过 PyCapsule 传递给 Python
//   3. GIL 管理与 C↔Python 调用（复用 rfm_pybridge 公共胶水层）

#include "rfm_pybridge.h"
#include "prompt_toolkit_wrapper.h"
#include <stdlib.h>
#include <string.h>

// string_free — 释放由 pt_readline 返回的字符串
// （pt_readline 现在返回 g_strdup 分配的字符串，因此用 g_free）
void string_free(char *s) {
    g_free(s);
}

// ── 全局 Python 对象引用 ──────────────────────────────────

static PyObject *pt_module = NULL;

// readline 相关函数
static PyObject *pt_readline_func = NULL;
static PyObject *pt_interrupt_func = NULL;
static PyObject *pt_request_repaint_func = NULL;
static PyObject *pt_set_selection_star_func = NULL;
static PyObject *pt_save_history_func = NULL;
static PyObject *pt_free_handle_func = NULL;

// ── 内部辅助 ──────────────────────────────────────────────

// 在持有 GIL 的前提下，把 C 函数指针以 PyCapsule 注入 module
static void _set_capsule_attr(PyObject *module, const char *name, void *fn) {
    RFM_PY_AUTO PyObject *capsule = rfm_py_capsule_from_fn(fn);
    if (PyObject_SetAttrString(module, name, capsule) != 0)
        rfm_py_clear_error(name);
}

// 安全调用 Python 无参函数
static void _call_python_void(PyObject *func) {
    if (!func) return;
    RFM_PY_GIL_SCOPE;
    RFM_PY_AUTO PyObject *result = rfm_py_call(func, NULL, NULL);
    rfm_py_clear_error(NULL);
}

// ── 公开接口 ──────────────────────────────────────────────

PtHandle* pt_create(const rfmFunctions *rfm_functions) {
    // Python 应该已经在 rfm 启动时由 startPythonEmbedding() 初始化
    if (!Py_IsInitialized()) {
        g_warning("pt_create: Python not initialized, call startPythonEmbedding() first");
        return NULL;
    }

    RFM_PY_GIL_SCOPE;

    // 1. 确保 rfm_prompt_toolkit 模块所在路径在 sys.path 中并导入
    //    rfmBinPath 是编译期宏（如 "/usr/local/bin/"），去掉尾部斜杠
    if (!pt_module) {
        const char *bin_dir = rfmBinPath;
        gchar *parent_dir = g_strndup(bin_dir, strlen(bin_dir) > 1 && bin_dir[strlen(bin_dir)-1] == '/' ? strlen(bin_dir)-1 : strlen(bin_dir));
        pt_module = rfm_py_import("rfm_prompt_toolkit.core", parent_dir);
        g_free(parent_dir);
        if (!pt_module) {
            rfm_py_clear_error("pt_create: failed to import rfm_prompt_toolkit.core");
            return NULL;
        }
    }

    // 2. 注入 C 函数指针（通过 PyCapsule，Python 侧用 ctypes 提取）
    if (rfm_functions) {
        _set_capsule_attr(pt_module, "_c_get_completion_fn_capsule", (void*)rfm_functions->get_completion_fn);
        _set_capsule_attr(pt_module, "_c_get_menu_completion_fn_capsule", (void*)rfm_functions->get_menu_completion_fn);
        _set_capsule_attr(pt_module, "_c_handle_escape_fn_capsule", (void*)rfm_functions->g_idle_add_once_handle_escape);
        _set_capsule_attr(pt_module, "_c_toggle_recursive_completion_capsule", (void*)rfm_functions->toggle_recursive_completion_fn);
    }

    // 3. 缓存 Python 函数引用
    pt_readline_func = rfm_py_getattr(pt_module, "readline");
    pt_interrupt_func = rfm_py_getattr(pt_module, "interrupt");
    pt_request_repaint_func = rfm_py_getattr(pt_module, "request_repaint");
    pt_set_selection_star_func = rfm_py_getattr(pt_module, "set_selection_star");
    pt_save_history_func = rfm_py_getattr(pt_module, "save_history");
    pt_free_handle_func = rfm_py_getattr(pt_module, "free_handle");

    // 4. 初始化 session（创建 PromptSession）
    {
        const char *home = getenv("HOME");
        if (!home) home = "/tmp";
        gchar *history_path = g_build_filename(home, ".rfm_prompt_toolkit_history.txt", NULL);
        RFM_PY_AUTO PyObject *result = rfm_py_call_attr(pt_module, "create_handle", "(s)", history_path);
        rfm_py_clear_error("pt_create: create_handle");
        g_free(history_path);
    }

    // 返回非 NULL 句柄（用于验证已初始化）
    static int dummy_handle = 1;
    return (PtHandle*)&dummy_handle;
}

char* pt_readline(PtHandle* handle, const char* prompt, const rfmFunctions *rfm_functions) {
    (void)handle;
    (void)rfm_functions;
    if (!pt_readline_func) {
        g_warning("pt_readline: module not initialized");
        return NULL;
    }

    RFM_PY_GIL_SCOPE;

    // 注意：在 prompt_toolkit 阻塞等待输入时，GIL 会被自动释放
    // （CPython 的 select/poll 调用前会释放 GIL）
    RFM_PY_AUTO PyObject *args = Py_BuildValue("(s)", prompt ? prompt : "");
    if (!args) {
        rfm_py_clear_error("pt_readline: build args");
        return NULL;
    }

    RFM_PY_AUTO PyObject *result = rfm_py_call(pt_readline_func, args, NULL);
    if (!result) {
        // Python 异常（如 Ctrl+C 被捕获）
        if (PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt) ||
                PyErr_ExceptionMatches(PyExc_EOFError)) {
                PyErr_Clear();
            } else {
                PyErr_Print();
                PyErr_Clear();
            }
        }
        return NULL;
    }

    return rfm_py_to_cstr(result);
}

void pt_interrupt(PtHandle* handle) {
    (void)handle;
    _call_python_void(pt_interrupt_func);
}

void pt_request_repaint(PtHandle* handle) {
    (void)handle;
    _call_python_void(pt_request_repaint_func);
}

void pt_set_selection_star(gboolean show) {
    if (!pt_set_selection_star_func) return;
    RFM_PY_GIL_SCOPE;
    RFM_PY_AUTO PyObject *args = Py_BuildValue("(i)", show ? 1 : 0);
    RFM_PY_AUTO PyObject *result = rfm_py_call(pt_set_selection_star_func, args, NULL);
    rfm_py_clear_error("pt_set_selection_star");

    // 如果正在 readline 中，请求重绘
    _call_python_void(pt_request_repaint_func);
}

void pt_save_history(PtHandle* handle) {
    (void)handle;
    _call_python_void(pt_save_history_func);
}

void pt_free(PtHandle* handle) {
    (void)handle;
    _call_python_void(pt_free_handle_func);

    RFM_PY_GIL_SCOPE;
    Py_CLEAR(pt_readline_func);
    Py_CLEAR(pt_interrupt_func);
    Py_CLEAR(pt_request_repaint_func);
    Py_CLEAR(pt_set_selection_star_func);
    Py_CLEAR(pt_save_history_func);
    Py_CLEAR(pt_free_handle_func);
    Py_CLEAR(pt_module);
}

int rfm_editor_edit_file(const rfmFunctions *rfm_functions, const gchar *filename, gchar **error) {
    (void)rfm_functions;
    if (!pt_module) {
        if (error) *error = g_strdup("prompt_toolkit module not initialized");
        return 0;
    }

    RFM_PY_GIL_SCOPE;

    RFM_PY_AUTO PyObject *result = rfm_py_call_attr(pt_module, "edit_file", "(s)", filename);

    int ret = 0;
    if (result && PyLong_Check(result)) {
        ret = (int)PyLong_AsLong(result);
    } else if (!result) {
        if (PyErr_Occurred()) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue && error) {
                gchar *msg = rfm_py_to_cstr(pvalue);
                if (msg) *error = msg;
            }
            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        } else if (error) {
            *error = g_strdup("edit_file failed");
        }
    }

    return ret;
}
