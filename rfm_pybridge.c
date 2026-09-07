/*
 * rfm_pybridge.c — 公共 C↔Python 嵌入胶水层实现
 * GPL-3 by guyuming
 *
 * 这些函数被编译进 rfm 主程序，插件通过 -rdynamic 导出的符号在 dlopen 时解析。
 */
#include <stdarg.h>
#include "rfm_addon.h"

PyObject *rfm_py_import(const char *module_name, const char *path_dir) {
    if (path_dir && path_dir[0]) {
        PyObject *sys_path = PySys_GetObject("path"); /* borrowed */
        if (sys_path) {
            PyObject *p = PyUnicode_FromString(path_dir);
            if (!p) return NULL;
            int present = PySequence_Contains(sys_path, p);
            if (present < 0) { Py_DECREF(p); return NULL; }
            if (!present) {
                if (PyList_Insert(sys_path, 0, p) != 0) { Py_DECREF(p); return NULL; }
            }
            Py_DECREF(p);
        }
    }
    return PyImport_ImportModule(module_name);
}

PyObject *rfm_py_getattr(PyObject *obj, const char *name) {
    if (!obj) {
        PyErr_SetString(PyExc_SystemError, "rfm_py_getattr: NULL object");
        return NULL;
    }
    return PyObject_GetAttrString(obj, name);
}

PyObject *rfm_py_call(PyObject *callable, PyObject *args, PyObject *kwargs) {
    if (!callable) {
        PyErr_SetString(PyExc_SystemError, "rfm_py_call: NULL callable");
        return NULL;
    }

    if (args == NULL && kwargs == NULL) {
        return PyObject_CallNoArgs(callable);
    }

    PyObject *empty = NULL;
    if (args == NULL) {
        /* PyObject_Call 要求 args 非 NULL，无位置参数时补一个空元组 */
        empty = PyTuple_New(0);
        if (!empty) return NULL;
        args = empty;
    }

    PyObject *result = PyObject_Call(callable, args, kwargs);
    Py_XDECREF(empty);
    return result;   /* NULL on error; caller checks PyErr */
}

/* 用 Py_BuildValue 格式构建参数 tuple。
 * Py_BuildValue 对单个格式单元（如 "s"、"O"）返回单个对象而非 tuple，
 * 而 PyObject_Call 要求 args 必须是 tuple，这里统一补一层 tuple。
 * 这样调用方既可写 "(s)" 也可写 "s"，与 PyObject_CallMethod 的行为兼容。 */
static PyObject *_build_args_tuple(const char *fmt, va_list va) {
    PyObject *args = Py_VaBuildValue(fmt, va);
    if (!args) return NULL;
    if (!PyTuple_Check(args)) {
        PyObject *wrapped = PyTuple_Pack(1, args);
        Py_DECREF(args);
        return wrapped;   /* may be NULL on error */
    }
    return args;
}

PyObject *rfm_py_call_attr(PyObject *obj, const char *attr, const char *fmt, ...) {
    if (!obj || !attr) {
        PyErr_SetString(PyExc_SystemError, "rfm_py_call_attr: NULL obj/attr");
        return NULL;
    }

    PyObject *callable = PyObject_GetAttrString(obj, attr);
    if (!callable) return NULL;

    PyObject *result;
    if (fmt) {
        va_list va;
        va_start(va, fmt);
        PyObject *args = _build_args_tuple(fmt, va);
        va_end(va);
        if (!args) { Py_DECREF(callable); return NULL; }
        result = PyObject_Call(callable, args, NULL);
        Py_DECREF(args);
    } else {
        /* PyObject_Call 要求 args 非 NULL，无参调用用 PyObject_CallNoArgs */
        result = PyObject_CallNoArgs(callable);
    }

    Py_DECREF(callable);
    return result;   /* NULL on error; caller checks PyErr */
}

PyObject *rfm_py_call_method(PyObject *obj, const char *method, const char *fmt, ...) {
    if (!obj || !method) {
        PyErr_SetString(PyExc_SystemError, "rfm_py_call_method: NULL obj/method");
        return NULL;
    }

    PyObject *callable = PyObject_GetAttrString(obj, method);
    if (!callable) return NULL;

    PyObject *result;
    if (fmt) {
        va_list va;
        va_start(va, fmt);
        PyObject *args = _build_args_tuple(fmt, va);
        va_end(va);
        if (!args) { Py_DECREF(callable); return NULL; }
        result = PyObject_Call(callable, args, NULL);
        Py_DECREF(args);
    } else {
        result = PyObject_CallNoArgs(callable);
    }

    Py_DECREF(callable);
    return result;
}


PyObject *rfm_py_call_method_obj(PyObject *obj, const char *method, PyObject *args, PyObject *kwargs) {
    PyObject *callable = rfm_py_getattr(obj, method);
    if (!callable) return NULL;
    PyObject *result = rfm_py_call(callable, args, kwargs);
    Py_DECREF(callable);
    return result;
}

gchar *rfm_py_to_cstr(PyObject *obj) {
    if (!obj || obj == Py_None) return NULL;
    if (PyUnicode_Check(obj)) {
        const char *s = PyUnicode_AsUTF8(obj);
        return s ? g_strdup(s) : NULL;
    }
    PyObject *str = PyObject_Str(obj);
    if (!str) return NULL;
    const char *s = PyUnicode_AsUTF8(str);
    gchar *r = s ? g_strdup(s) : NULL;
    Py_DECREF(str);
    return r;
}
