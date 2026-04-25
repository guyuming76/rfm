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
#include <gio/gdesktopappinfo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef rfmChinese
   #define START_MENU "开始菜单"
#else
   #define START_MENU "Start menu"
#endif

extern char rfm_historyFile_ParentDirectory[PATH_MAX];
extern void load_history_for_prompt();
extern void save_current_history();
extern const stdin_cmd_interpretor stdin_cmd_interpretors[];
extern void add_toolbar_buttons(GtkWidget ***buttons, const RFM_ToolButton button_definitions[], const size_t size);
extern void add_file_menuitems(GtkWidget ***menuitems, const RFM_MenuItem menuitem_definitions[], const size_t size);
extern void remove_toolbar_buttons(GtkWidget ***buttons, const size_t size);
extern void remove_file_menuitems(GtkWidget ***menuitems, const size_t size);

extern GtkIconTheme *icon_theme;
extern int RFM_ICON_SIZE;
extern int currentPageStartingColumnTitleIndex;
extern gchar *rfm_SearchResultPath;
extern void ProcessKeyValuePair(CALLER_OWN gchar *key, OWNER_CHANGE gchar* currentExtColumnValue, CALLER_OWN char* currentColumnOriginalTitle);
extern void CallDefaultSearchResultFunctionForNewSearch(CALLER_OWN char *one_line_in_search_result, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);

RFM_ADDON_RESOURCE_TYPE addon_resource = {0};
const enum RFM_ADDON_TYPE addon_type = Global;

static const char *start_menu[] = { NULL, "cd /usr/share/applications", NULL };
static const char *launch_desktop[] = { "dex", "--term", "$RFM_TERM", NULL };

RFM_ToolButton tool_buttons_local[] = {
   /* name           icon                       function    		RunCmd      SearchResultView   DirectoryView   Accel                 tooltip                showCondition*/
   { START_MENU,     NULL,                      NULL,                   start_menu,        FALSE,          TRUE,      0,                      NULL,                  NULL},
};

RFM_MenuItem file_menus_local[] = {
   /* name           mime root        mime sub type            filenameSuffix           OrSearchResultType	func		runCmd            		showCondition	*/
   { "Open",         "application",    "x-desktop",            NULL,                    NULL,               NULL,    launch_desktop,         NULL },
};

// .desktop 文件的 thumbnail 加载函数
static GdkPixbuf *desktop_thumbLoadFunc(RFM_FileAttributes *fileAttributes) {
  if (strcmp(fileAttributes->mime_sub_type, "x-desktop") == 0 &&
      strcmp(fileAttributes->mime_root, "application") == 0) {
    GDesktopAppInfo *app_info =
        g_desktop_app_info_new_from_filename(fileAttributes->path);
    if (app_info) {
      GIcon *appIcon = g_app_info_get_icon(app_info);
      if (appIcon) {
        const gchar *type_name = G_OBJECT_TYPE_NAME(appIcon);
        g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG,
              "create %s for GDesktopAppInfo:%s",
              type_name ? type_name : "NULL",
              g_desktop_app_info_get_filename(app_info));
        if (strcmp(type_name, "GThemedIcon") == 0) {
          char **icon_names = g_themed_icon_get_names(appIcon);
          if (icon_names) {
            GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(
                icon_theme, icon_names[0], RFM_ICON_SIZE,
                GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
            g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG,
                  "create GdkPixbuf for themed icon:%s", icon_names[0]);
            return pixbuf;
          }
        }
      }else g_warning("Failed to create GIcon for desktop file:%s",
                fileAttributes->path);
    } else
      g_warning("Failed to create GDesktopAppInfo for desktop file:%s",
                fileAttributes->path);
  }
  return NULL;
}

// 处理 .desktop 文件，直接解析文件内容
int ProcessKeyValuePairInDesktopFilesFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
    if (new_search){
      CallDefaultSearchResultFunctionForNewSearch(oneline, TRUE, cmdTemplate, fileAttributes, childAttribs);
      return 0;// 对于new_search, 调用本方法的目的仅仅是调用default的ProcessOnelineForSearchResult
    }
 
    gchar * absolute_address = g_canonicalize_filename(oneline, rfm_SearchResultPath);
    GError *error=NULL;
 
    // 直接读取并解析 .desktop 文件，避免使用 GDesktopAppInfo 可能的语言问题
    GKeyFile *keyfile = g_key_file_new();
    if (!g_key_file_load_from_file(keyfile, absolute_address, G_KEY_FILE_NONE, &error)){
      g_warning("ProcessKeyValuePairInDesktopFilesFromSearchResult. error loading desktop file %s, error code: %d, message:%s", oneline, error==NULL?0:error->code, error==NULL?"":error->message);
      g_error_free(error);
    } else {
      // 处理常用的 .desktop 文件键
      char currentColumnOriginalTitle[10];
      snprintf(currentColumnOriginalTitle, 10, "C%d", currentPageStartingColumnTitleIndex);
 
      // 处理函数：安全地处理字符串并传递给 ProcessKeyValuePair
      void process_string(const gchar *key, const gchar *value) {
        if (value) {
          // 确保字符串是有效的 UTF-8
          gchar *utf8_value = NULL;
          if (g_utf8_validate(value, -1, NULL)) {
            // 有效的 UTF-8 字符串，直接使用
            utf8_value = g_strdup(value);
          } else {
            // 尝试转换为 UTF-8
            utf8_value = g_locale_to_utf8(value, -1, NULL, NULL, NULL);
          }
          
          if (utf8_value) {
            // 再次验证转换后的字符串
            if (g_utf8_validate(utf8_value, -1, NULL)) {
              ProcessKeyValuePair((gchar*)key, utf8_value, currentColumnOriginalTitle);
            } else {
              // 仍然无效，使用空字符串
              ProcessKeyValuePair((gchar*)key, (gchar*)"", currentColumnOriginalTitle);
            }
            g_free(utf8_value);
          } else {
            // 转换失败，使用空字符串
            ProcessKeyValuePair((gchar*)key, (gchar*)"", currentColumnOriginalTitle);
          }
        }
      }
 
      // 处理常用键
      gchar **keys = g_key_file_get_keys(keyfile, "Desktop Entry", NULL, NULL);
      if (keys) {
        for (int i = 0; keys[i]; i++) {
          gchar *key = keys[i];
          // 只处理常用键，跳过带语言下标的键
          if (strchr(key, '[') == NULL) {
            gchar *value = g_key_file_get_string(keyfile, "Desktop Entry", key, NULL);
            if (value) {
              process_string(key, value);
              g_free(value);
            }
          }
        }
        g_strfreev(keys);
      }
 
      g_key_file_free(keyfile);
    }
 
    g_free(absolute_address);
    return 0;
}

RFM_SearchResultType searchresultTypes[] = {
//name	              SearchResultLineProcessingFunc                  showcolumn     SearchResultColumnSeperator  cmdTemplate                                Description
  {".desktop", ProcessKeyValuePairInDesktopFilesFromSearchResult,     NULL,          NULL,                        NULL,                            "parse .desktop file"},
};

void rfm_local_enter(char * Addon_dir) {
     fprintf(stderr,"rfm_local_enter called for %s\n", Addon_dir);
     addon_resource.addon_dir = strdup(Addon_dir);
     addon_resource.toolbar_button_definitions_local = tool_buttons_local;
     addon_resource.toolbar_button_definitions_local_array_size = G_N_ELEMENTS(tool_buttons_local);
     addon_resource.file_menuitem_definitions_local = file_menus_local;
     addon_resource.file_menuitem_definitions_local_array_size = G_N_ELEMENTS(file_menus_local);
     addon_resource.searchresulttypes = searchresultTypes;
     addon_resource.searchresulttypes_array_size = G_N_ELEMENTS(searchresultTypes);
     start_menu[0] = stdin_cmd_interpretors[0].name;
     addon_resource.thumbLoadFunc = desktop_thumbLoadFunc;
     add_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),tool_buttons_local, addon_resource.toolbar_button_definitions_local_array_size);
     add_file_menuitems(&(addon_resource.menuItems_local), file_menus_local, addon_resource.file_menuitem_definitions_local_array_size);
}

void rfm_local_leave() {
     fprintf(stderr,"rfm_local_leave called for %s\n", addon_resource.addon_dir);
     remove_toolbar_buttons(&(addon_resource.tool_bar_buttons_local),addon_resource.toolbar_button_definitions_local_array_size);
     remove_file_menuitems(&(addon_resource.menuItems_local), addon_resource.file_menuitem_definitions_local_array_size);
}
