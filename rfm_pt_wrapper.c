// rfm_pt_wrapper.c
// GPL-3 by guyuming
//
// Python prompt_toolkit 与 rfm C 代码之间的胶水层。
// 负责：
//   1. 初始化嵌入式 Python 并导入 rfm_prompt_toolkit 模块
//   2. 将 C 补全函数指针通过 PyCapsule 传递给 Python
//   3. 管理 GIL：所有 Python 调用通过 PyGILState_Ensure/Release
//   4. C ↔ Python 字符串转换

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "prompt_toolkit_wrapper.h"

// string_free — 释放由 pt_readline 返回的字符串（与 reedline_wrapper 一致）
void string_free(char *s) {
    if (s) free(s);
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

// 为 C 补全函数创建 PyCapsule 包装器
// Python 侧通过 ctypes 将其转换为可调用对象
static PyObject* _make_completion_capsule(GetCompletionFn fn) {
    if (!fn) Py_RETURN_NONE;
    return PyCapsule_New((void*)fn, "rfm_completion_fn", NULL);
}

static PyObject* _make_escape_capsule(EscapeHandler fn) {
    if (!fn) Py_RETURN_NONE;
    return PyCapsule_New((void*)fn, "rfm_escape_fn", NULL);
}

static PyObject* _make_toggle_recursive_capsule(ToggleRecursiveCompletionFn fn) {
    if (!fn) Py_RETURN_NONE;
    return PyCapsule_New((void*)fn, "rfm_toggle_recursive_fn", NULL);
}

// 将 Python 函数调用结果转换为 C 字符串 (malloc, caller frees with string_free)
static char* _py_unicode_to_cstr(PyObject *obj) {
    if (!obj || obj == Py_None) return NULL;
    if (PyUnicode_Check(obj)) {
        const char *utf8 = PyUnicode_AsUTF8(obj);
        if (utf8) return strdup(utf8);
    }
    // 尝试 str() 转换
    PyObject *str_obj = PyObject_Str(obj);
    if (str_obj) {
        const char *s = PyUnicode_AsUTF8(str_obj);
        char *result = s ? strdup(s) : NULL;
        Py_DECREF(str_obj);
        return result;
    }
    return NULL;
}

// 安全调用 Python 无参函数
static void _call_python_void(PyObject *func) {
    if (!func) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *result = PyObject_CallObject(func, NULL);
    if (!result) PyErr_Print();
    Py_XDECREF(result);
    PyGILState_Release(gstate);
}

// ── 公开接口 ──────────────────────────────────────────────

PtHandle* pt_create(const rfmFunctions *rfm_functions) {
    // Python 应该已经在 rfm 启动时由 startPythonEmbedding() 初始化
    // 这里不再重复 Py_Initialize
    if (!Py_IsInitialized()) {
        g_warning("pt_create: Python not initialized, call startPythonEmbedding() first");
        return NULL;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    // 1. 确保 rfm_prompt_toolkit 模块所在路径在 sys.path 中
    //    模块安装在 ${PREFIX}/bin/rfm_prompt_toolkit
    //    其父目录 ${PREFIX}/bin 需要加入 sys.path
    //    rfmBinPath 是编译期宏（如 "/usr/local/bin/"），去掉尾部斜杠
    {
        const char *bin_dir = rfmBinPath;
        // 去掉尾部斜杠得到目录路径
        gchar *parent_dir = g_strndup(bin_dir, strlen(bin_dir) > 1 && bin_dir[strlen(bin_dir)-1] == '/' ? strlen(bin_dir)-1 : strlen(bin_dir));
        gchar *path_cmd = g_strdup_printf(
            "import sys\n"
            "p = '%s'\n"
            "if p not in sys.path: sys.path.insert(0, p)\n",
            parent_dir);
        PyRun_SimpleString(path_cmd);
        g_free(path_cmd);
        g_free(parent_dir);
    }

    // 2. 导入模块
    if (!pt_module) {
        pt_module = PyImport_ImportModule("rfm_prompt_toolkit.core");
        if (!pt_module) {
            PyErr_Print();
            g_warning("pt_create: failed to import rfm_prompt_toolkit.core");
            PyGILState_Release(gstate);
            return NULL;
        }
    }

    // 3. 注入 C 函数指针（通过 PyCapsule）
    if (rfm_functions) {
        PyObject *capsule;
        capsule = _make_completion_capsule(rfm_functions->get_completion_fn);
        PyObject_SetAttrString(pt_module, "_c_get_completion_fn_capsule", capsule);
        Py_DECREF(capsule);

        capsule = _make_completion_capsule(rfm_functions->get_menu_completion_fn);
        PyObject_SetAttrString(pt_module, "_c_get_menu_completion_fn_capsule", capsule);
        Py_DECREF(capsule);

        capsule = _make_escape_capsule(rfm_functions->g_idle_add_once_handle_escape);
        PyObject_SetAttrString(pt_module, "_c_handle_escape_fn_capsule", capsule);
        Py_DECREF(capsule);

        capsule = _make_toggle_recursive_capsule(rfm_functions->toggle_recursive_completion_fn);
        PyObject_SetAttrString(pt_module, "_c_toggle_recursive_completion_capsule", capsule);
        Py_DECREF(capsule);
    }

    // 4. 缓存 Python 函数引用
    pt_readline_func = PyObject_GetAttrString(pt_module, "readline");
    pt_interrupt_func = PyObject_GetAttrString(pt_module, "interrupt");
    pt_request_repaint_func = PyObject_GetAttrString(pt_module, "request_repaint");
    pt_set_selection_star_func = PyObject_GetAttrString(pt_module, "set_selection_star");
    pt_save_history_func = PyObject_GetAttrString(pt_module, "save_history");
    pt_free_handle_func = PyObject_GetAttrString(pt_module, "free_handle");

    // 5. 初始化 session（创建 PromptSession）
    PyObject *create_handle = PyObject_GetAttrString(pt_module, "create_handle");
    if (create_handle) {
        const char *home = getenv("HOME");
        if (!home) home = "/tmp";
        gchar *history_path = g_build_filename(home, ".rfm_prompt_toolkit_history.txt", NULL);
        PyObject *args = Py_BuildValue("(s)", history_path);
        PyObject *result = PyObject_CallObject(create_handle, args);
        if (!result) PyErr_Print();
        Py_XDECREF(result);
        Py_DECREF(args);
        g_free(history_path);
        Py_DECREF(create_handle);
    }

    PyGILState_Release(gstate);

    // 返回非 NULL 句柄（用于验证已初始化）
    static int dummy_handle = 1;
    return (PtHandle*)&dummy_handle;
}

char* pt_readline(PtHandle* handle, const char* prompt, const rfmFunctions *rfm_functions) {
    (void)handle;
    if (!pt_readline_func) {
        g_warning("pt_readline: module not initialized");
        return NULL;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    // 注意：在 prompt_toolkit 阻塞等待输入时，GIL 会被自动释放
    // （CPython 的 select/poll 调用前会释放 GIL）

    PyObject *args = Py_BuildValue("(s)", prompt ? prompt : "");
    PyObject *result = PyObject_CallObject(pt_readline_func, args);
    Py_DECREF(args);

    if (!result) {
        // Python 异常（如 Ctrl+C 被捕获）
        if (PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt) ||
                PyErr_ExceptionMatches(PyExc_EOFError)) {
                PyErr_Clear();
            } else {
                PyErr_Print();
            }
        }
        PyGILState_Release(gstate);
        return NULL;
    }

    char *c_result = _py_unicode_to_cstr(result);
    Py_DECREF(result);
    PyGILState_Release(gstate);
    return c_result;
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
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *args = Py_BuildValue("(i)", show ? 1 : 0);
    PyObject *result = PyObject_CallObject(pt_set_selection_star_func, args);
    if (!result) PyErr_Print();
    Py_XDECREF(result);
    Py_DECREF(args);
    PyGILState_Release(gstate);

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

    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_XDECREF(pt_readline_func); pt_readline_func = NULL;
    Py_XDECREF(pt_interrupt_func); pt_interrupt_func = NULL;
    Py_XDECREF(pt_request_repaint_func); pt_request_repaint_func = NULL;
    Py_XDECREF(pt_set_selection_star_func); pt_set_selection_star_func = NULL;
    Py_XDECREF(pt_save_history_func); pt_save_history_func = NULL;
    Py_XDECREF(pt_free_handle_func); pt_free_handle_func = NULL;
    Py_XDECREF(pt_module); pt_module = NULL;
    PyGILState_Release(gstate);
}

int rfm_editor_edit_file(const rfmFunctions *rfm_functions, const gchar *filename, gchar **error) {
    if (!pt_module) {
        if (error) *error = g_strdup("prompt_toolkit module not initialized");
        return 0;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *edit_func = PyObject_GetAttrString(pt_module, "edit_file");
    if (!edit_func) {
        PyErr_Print();
        PyGILState_Release(gstate);
        if (error) *error = g_strdup("edit_file function not found");
        return 0;
    }

    PyObject *args = Py_BuildValue("(s)", filename);
    PyObject *result = PyObject_CallObject(edit_func, args);
    Py_DECREF(args);
    Py_DECREF(edit_func);

    int ret = 0;
    if (result && PyLong_Check(result)) {
        ret = (int)PyLong_AsLong(result);
    } else if (!result) {
        if (PyErr_Occurred()) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue && error) {
                PyObject *str = PyObject_Str(pvalue);
                if (str) {
                    *error = g_strdup(PyUnicode_AsUTF8(str));
                    Py_DECREF(str);
                }
            }
            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        } else if (error) {
            *error = g_strdup("edit_file failed");
        }
    }

    Py_XDECREF(result);
    PyGILState_Release(gstate);
    return ret;
}
