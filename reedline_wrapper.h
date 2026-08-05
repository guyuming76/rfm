//GPL-3 by guyuming

// 这个文件被 rfm.c 引用，作为调用rust写的reedline_wrapper库接口

#ifndef REEDLINE_WRAPPER_H
#define REEDLINE_WRAPPER_H
typedef void ReedlineHandle;
// 函数指针类型定义，对应函数 rfm_filename_completion
typedef const GPtrArray* (*GetCompletionFn)(const char* input, int *count);
typedef const void (*EscapeHandler)();
typedef void (*ToggleRecursiveCompletionFn)();
// 定义与Rust中RfmFunctions匹配的结构体
typedef struct {
    GetCompletionFn get_completion_fn;
    GetCompletionFn get_menu_completion_fn;
    // 其他函数指针可以在这里添加
    EscapeHandler g_idle_add_once_handle_escape;
    ToggleRecursiveCompletionFn toggle_recursive_completion_fn;
} rfmFunctions;

// 创建Reedline实例
ReedlineHandle* reedline_create(const rfmFunctions * rfm_functions);
// 读取一行输入 (返回的字符串需用 string_free 释放)
char* reedline_readline(ReedlineHandle* handle, const char* prompt, const rfmFunctions * rfm_functions);
void reedline_interrupt(ReedlineHandle* handle);
// 异步请求 reedline 重绘提示符（用于 GTK 线程中选中状态变化后更新提示符的 * 标记）
void reedline_request_repaint(ReedlineHandle* handle);
// 设置提示符中是否显示选中标记 * （TRUE=显示, FALSE=不显示）
void reedline_set_selection_star(gboolean show);
// 释放字符串内存
void string_free(char* s);
// 销毁Reedline实例
void reedline_free(ReedlineHandle* handle);
void reedline_save_history(ReedlineHandle* handle);
// 注意：必须与 Rust 代码中的 REEDLINE_INTERRUPTED 保持一致
#define REEDLINE_INTERRUPTED "reedline_interrupted"

typedef void RfmEditor;
int rfm_editor_edit_file(const rfmFunctions * rfm_functions, const gchar *filename, gchar **error);
#endif
