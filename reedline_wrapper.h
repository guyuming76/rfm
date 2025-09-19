//GPL-3 by guyuming

// 这个文件被 rfm.c 引用，作为调用rust写的reedline_wrapper库接口

#ifndef REEDLINE_WRAPPER_H
#define REEDLINE_WRAPPER_H
typedef void ReedlineHandle;
// 函数指针类型定义，对应函数 rfm_filename_completion
typedef const char** (*GetCompletionFn)(const char* input, int *count);
// 定义与Rust中RfmFunctions匹配的结构体
typedef struct {
    GetCompletionFn get_completion_fn;
    // 其他函数指针可以在这里添加
} rfmFunctions;

// 创建Reedline实例
ReedlineHandle* reedline_create(const rfmFunctions *);
// 读取一行输入 (返回的字符串需用 string_free 释放)
char* reedline_readline(ReedlineHandle* handle, const char* prompt, const rfmFunctions *);
// 释放字符串内存
void string_free(char* s);
// 销毁Reedline实例
void reedline_free(ReedlineHandle* handle);
void reedline_save_history(ReedlineHandle* handle);


typedef void RfmEditor;
int rfm_editor_edit_file(GetCompletionFn rfm_filename_completion_fn, const gchar *filename, gchar **error);
#endif
