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

// 以下宿主函数/变量的声明已统一写入 rfm_addon.h（编译插件时由 -include 自动引入）。
// 这里不要再手写 extern：手写的声明一旦与宿主不一致，编译期就会报 conflicting types 并给出新旧类型。


RFM_ADDON_RESOURCE_TYPE addon_resource = {0};
const enum RFM_ADDON_TYPE addon_type = Global;

static const char *start_menu[] = { NULL, "find /usr/share/applications -name \"*.desktop\">.desktop", NULL };
static const char *launch_desktop[] = { "dex", "--term", "$RFM_TERM", NULL };

RFM_ToolButton tool_buttons_local[] = {
   /* name           icon                       function    		RunCmd      SearchResultView   DirectoryView   Accel                 tooltip                showCondition*/
   { START_MENU,     NULL,                      NULL,                   start_menu,        TRUE,          TRUE,      0,                      NULL,                  NULL},
};

RFM_MenuItem file_menus_local[] = {
   /* name           mime root        mime sub type            filenameSuffix           OrSearchResultType	func		runCmd            		showCondition	*/
   { "Open",         "application",    "x-desktop",            NULL,                    NULL,               NULL,    launch_desktop,         NULL },
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

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
        return 0;
    }
    
    gchar *absolute_address = g_canonicalize_filename(oneline, rfm_SearchResultPath);
    GDesktopAppInfo *app_info = g_desktop_app_info_new_from_filename(absolute_address);
    if (!app_info) {
        g_warning("ProcessKeyValuePairInDesktopFilesFromSearchResult: failed to create GDesktopAppInfo for %s", absolute_address);
        g_free(absolute_address);
        return 0;
    }
    
    char currentColumnOriginalTitle[10];
    snprintf(currentColumnOriginalTitle, 10, "C%d", currentPageStartingColumnTitleIndex);
       
    // 要提取的常用 desktop entry 键（不含语言标记）
    static const gchar *common_keys[] = {
        "Name", "Comment", "GenericName", "Exec", "Icon",
        "Categories", "Type", "Terminal", "NoDisplay", "Hidden",
        "OnlyShowIn", "NotShowIn", "MimeType", "Keywords",
        "StartupWMClass", "StartupNotify", "Actions",
        NULL
    };
    
    for (int i = 0; common_keys[i] != NULL; i++) {
        const gchar *key = strdup(common_keys[i]);
        gchar *value = g_desktop_app_info_get_string(app_info, key);
        if (value) ProcessKeyValuePair((gchar*)key, value, currentColumnOriginalTitle);
    }
    
    g_object_unref(app_info);
    g_free(absolute_address);
    return 0;
}
#pragma GCC diagnostic pop
static char *desktop_showcolumn[] = {",缩略图,Name,Exec,Comment,Categories,Terminal", NULL};
RFM_SearchResultType searchresultTypes[] = {
//name	              SearchResultLineProcessingFunc                  showcolumn     SearchResultColumnSeperator  cmdTemplate                                Description
  {".desktop", ProcessKeyValuePairInDesktopFilesFromSearchResult,     desktop_showcolumn, NULL,                        NULL,                            "parse .desktop file"},
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
     char* column=calloc(100,sizeof(char));
     sprintf(column,",%d,Name,Exec,Comment,Categories,Terminal",COL_PIXBUF);
     desktop_showcolumn[0] = column;
}

void rfm_local_leave() {
     fprintf(stderr,"rfm_local_leave called for %s\n", addon_resource.addon_dir);
}
