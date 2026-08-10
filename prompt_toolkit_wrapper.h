// prompt_toolkit_wrapper.h
// GPL-3 by guyuming
//
// 这个文件被 rfm.c 引用，作为调用 Python prompt_toolkit 的接口。
// 接口签名与 reedline_wrapper.h 保持一致，以便在 rfm.c 中用条件编译切换。

#ifndef PROMPT_TOOLKIT_WRAPPER_H
#define PROMPT_TOOLKIT_WRAPPER_H

#include <glib.h>

typedef void PtHandle;

// 函数指针类型定义，对应 C 补全函数（与 reedline_wrapper.h 一致）
typedef const GPtrArray* (*GetCompletionFn)(const char* input, int *count);
typedef const void (*EscapeHandler)();
typedef void (*ToggleRecursiveCompletionFn)();

typedef struct {
    GetCompletionFn get_completion_fn;
    GetCompletionFn get_menu_completion_fn;
    EscapeHandler g_idle_add_once_handle_escape;
    ToggleRecursiveCompletionFn toggle_recursive_completion_fn;
} rfmFunctions;

// 创建 prompt_toolkit 实例
PtHandle* pt_create(const rfmFunctions * rfm_functions);

// 读取一行输入
char* pt_readline(PtHandle* handle, const char* prompt, const rfmFunctions * rfm_functions);

// 中断 readline
void pt_interrupt(PtHandle* handle);

// 异步请求重绘提示符
void pt_request_repaint(PtHandle* handle);

// 设置选中星号
void pt_set_selection_star(gboolean show);

// 释放字符串内存
void string_free(char* s);

// 销毁实例
void pt_free(PtHandle* handle);

// 保存历史记录
void pt_save_history(PtHandle* handle);

// 必须与 Python 侧 REEDLINE_INTERRUPTED 保持一致
#define REEDLINE_INTERRUPTED "reedline_interrupted"

// rfmEdit 编辑器
typedef void RfmEditor;
int rfm_editor_edit_file(const rfmFunctions * rfm_functions, const gchar *filename, gchar **error);

#endif
