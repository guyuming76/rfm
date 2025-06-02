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

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <rfm_addon.h>

extern char rfm_historyFile_ParentDirectory[PATH_MAX];
extern void load_history_for_prompt();
extern void save_current_history();
extern const stdin_cmd_interpretor stdin_cmd_interpretors[];
extern void add_toolbar_buttons(GtkWidget ***buttons, const RFM_ToolButton button_definitions[], const size_t size);
extern void add_file_menuitems(GtkWidget ***menuitems, const RFM_MenuItem menuitem_definitions[], const size_t size);
extern void remove_toolbar_buttons(GtkWidget ***buttons, const size_t size);
extern void remove_file_menuitems(GtkWidget ***menuitems, const size_t size);

RFM_ADDON_RESOURCE_TYPE addon_resource = {0};
const enum RFM_ADDON_TYPE addon_type = Directory;

static const char *rfmTODO[] = { NULL, "find . -name '*.rfmTODO.*'" RFM_SearchResultTypeNamePrefix "multitype", NULL };
static const char *testcmd[] = { "echo","test local file menu command",NULL };
static char *saved_history_dir = NULL;


RFM_ToolButton tool_buttons_local[] = {
   /* name           icon                       function    		RunCmd      SearchResultView   DirectoryView   Accel                 tooltip                showCondition*/
   { "rfmTODO",      NULL,                      NULL,                   rfmTODO,             FALSE,        TRUE,      0,                     NULL,                    NULL},
};

RFM_MenuItem file_menus_local[] = {
   /* name           mime root        mime sub type            filenameSuffix           OrSearchResultType	func		runCmd            		showCondition	*/
   { "test cmd",     "*",              "*",                    NULL,			NULL,			NULL,		testcmd,             		NULL },
};

void rfm_local_enter() {
     fprintf(stderr,"rfm_local_enter called!\n");
     saved_history_dir=strdup(rfm_historyFile_ParentDirectory);
     save_current_history();
     if (getcwd(rfm_historyFile_ParentDirectory,sizeof(rfm_historyFile_ParentDirectory))==NULL) fprintf(stderr,"getcwd return NULL");
     load_history_for_prompt();

     addon_resource.toolbar_button_definitions_local = tool_buttons_local;
     addon_resource.toolbar_button_definitions_local_array_size = G_N_ELEMENTS(tool_buttons_local);
     addon_resource.file_menuitem_definitions_local = file_menus_local;
     addon_resource.file_menuitem_definitions_local_array_size = G_N_ELEMENTS(file_menus_local);
     rfmTODO[0] = stdin_cmd_interpretors[0].name;
     add_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),tool_buttons_local, addon_resource.toolbar_button_definitions_local_array_size);
     add_file_menuitems(&(addon_resource.menuItems_local), file_menus_local, addon_resource.file_menuitem_definitions_local_array_size);
}

void rfm_local_leave() {
     fprintf(stderr,"rfm_local_leave called!\n");
     if (saved_history_dir) {
         save_current_history();
         sprintf(rfm_historyFile_ParentDirectory,"%s",saved_history_dir);
         free(saved_history_dir);
         load_history_for_prompt();
     }

     remove_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),addon_resource.toolbar_button_definitions_local_array_size);
     remove_file_menuitems(&(addon_resource.menuItems_local), addon_resource.file_menuitem_definitions_local_array_size);
}
