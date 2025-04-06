#ifdef RFM_LOCAL_C_HASH
// 使用编译时定义的宏 RFM_LOCAL_C_HASH 来初始化这个常量指针
// 这个变量会成为 .so 文件中的一个符号，可以被 dlsym 找到
const char * const rfm_local_c_file_hash = RFM_LOCAL_C_HASH;
#else
// 如果编译时没有定义 RFM_LOCAL_C_HASH 宏 (例如手动编译时忘了加 -D 参数)
// 就发出警告，并将符号定义为 NULL，这样 rfm.c 加载时能发现哈希缺失
#warning "RFM_LOCAL_C_HASH is not defined during compilation. Hash checking willlikely fail."
const char * const rfm_local_c_file_hash = NULL;
#endif

// #include <gtk/gtk.h>

void rfm_local_enter() {
     printf("rfm_local_enter called!\n");
}

void rfm_local_leave() {
     printf("rfm_local_leave called!\n");
}
