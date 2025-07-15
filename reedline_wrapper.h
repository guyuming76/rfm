#ifndef REEDLINE_WRAPPER_H
#define REEDLINE_WRAPPER_H
typedef void ReedlineHandle;
// 函数指针类型定义，对应函数 rfm_filename_completion
typedef const char** (*GetCompletionFn)(const char* input, int *count);
// 创建Reedline实例
ReedlineHandle* reedline_create(GetCompletionFn rfm_filename_completion_fn);
// 读取一行输入 (返回的字符串需用 string_free 释放)
char* reedline_readline(ReedlineHandle* handle, const char* prompt);
// 释放字符串内存
void string_free(char* s);
// 销毁Reedline实例
void reedline_free(ReedlineHandle* handle);
#endif
