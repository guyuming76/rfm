// 【已废弃的老机制】曾经用 RFM_LOCAL_C_HASH 在这里定义 rfm_local_c_file_hash 符号。
// 现在插件 .c 与 rfm_addon.h 的 hash 由 rfm 编译插件时用 -DRFM_ADDON_C_HASH / -DRFM_ADDON_H_HASH 传入，
// 并在 rfm_addon.h 里定义成符号 rfm_addon_c_file_hash / rfm_addon_h_file_hash 供 rfm.c 用 dlsym 读取。
// 下面只保留"手动编译时忘了加 -D 参数"的提醒。
#ifndef RFM_ADDON_C_HASH
#warning "RFM_ADDON_C_HASH is not defined during compilation. Hash checking will likely fail."
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <rfm_addon.h>

// 以下宿主函数/变量的声明已统一写入 rfm_addon.h（编译插件时由 -include 自动引入）。
// 这里不要再手写 extern：手写的声明一旦与宿主不一致，编译期就会报 conflicting types 并给出新旧类型。


RFM_ADDON_RESOURCE_TYPE addon_resource = {0};
const enum RFM_ADDON_TYPE addon_type = DirectoryTree;

static const char *rfmTODO[] = { NULL, "find . \\( -name \"rfmTODO.*\" -o -name \"*.rfmTODO.*\" -o -name \"*.rfmTODO\" -o -name \"rfmTODO\" \\)" RFM_SearchResultTypeNamePrefix "multitype", NULL };
static const char *testcmd[] = { "echo","test local file menu command",NULL };
static char *saved_history_dir = NULL;


RFM_ToolButton tool_buttons_local[] = {
   /* name           icon                       function    		RunCmd      SearchResultView   DirectoryView   Accel                 tooltip                showCondition*/
   { "rfmTODO",      NULL,                      NULL,                   rfmTODO,             FALSE,        TRUE,      0,                     NULL,                    NULL},
};

RFM_MenuItem file_menus_local[] = {
   /* name           mime root        mime sub type            filenameSuffix           OrSearchResultType	func		runCmd            		showCondition	*/
   //{ "test cmd",     "*",              "*",                    NULL,			NULL,			NULL,		testcmd,             		NULL },
};

void rfm_local_enter(char * Addon_dir) {
     fprintf(stderr,"rfm_local_enter called for %s\n", Addon_dir);
     saved_history_dir=strdup(rfm_historyFile_ParentDirectory);
     save_current_history();
     snprintf(rfm_historyFile_ParentDirectory,sizeof(rfm_historyFile_ParentDirectory),Addon_dir);
     load_history_for_prompt();
     addon_resource.addon_dir = strdup(Addon_dir);
     addon_resource.toolbar_button_definitions_local = tool_buttons_local;
     addon_resource.toolbar_button_definitions_local_array_size = G_N_ELEMENTS(tool_buttons_local);
     addon_resource.file_menuitem_definitions_local = file_menus_local;
     addon_resource.file_menuitem_definitions_local_array_size = G_N_ELEMENTS(file_menus_local);
     rfmTODO[0] = stdin_cmd_interpretors[0].name;
     add_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),tool_buttons_local, addon_resource.toolbar_button_definitions_local_array_size);
     add_file_menuitems(&(addon_resource.menuItems_local), file_menus_local, addon_resource.file_menuitem_definitions_local_array_size);
}

void rfm_local_leave() {
     fprintf(stderr,"rfm_local_leave called for %s\n", addon_resource.addon_dir);
     if (saved_history_dir) {
         save_current_history();
         sprintf(rfm_historyFile_ParentDirectory,"%s",saved_history_dir);
         free(saved_history_dir);
         load_history_for_prompt();
     }

     remove_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),addon_resource.toolbar_button_definitions_local_array_size);
     remove_file_menuitems(&(addon_resource.menuItems_local), addon_resource.file_menuitem_definitions_local_array_size);
}
