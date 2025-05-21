//rfm项目里目前有两个.h文件： rfm_local.h 和 config.h.
//最初只有config.h,用于suckless风格的定制，自定义文件菜单之类
//rfm_local.h的加入是为了满足 rfm_local.c 的动态编译。rfm_local.h 要在 rfm.c 和 rfm_local.c 之间共享
//事实上，config.h里面的定义也是依赖rfm_local.h内数据类型定义的，但config.h内无需#include rfm_local.h,这时因为在rfm.c里面对于config.h的引用出现在对rfm_local.h的应用的后面，c语言编译器似乎可以处理这种情况

typedef struct {  /* Update free_fileAttributes() and malloc_fileAttributes() if new items are added */
   guint id; // usually, path can be unique id for fileAttributelist, however, for searchresult from grep, same file can appear more than once, so we need a id as key for grepMatch_hash and others.
   gchar *path; // absolute path
   gchar *file_name;
   gchar *file_name_escaped_for_iconview_markup_column;
  //gchar *display_name;
   gboolean is_dir;
   gboolean is_mountPoint;
   gchar *icon_name;
   GdkPixbuf *pixbuf;
   gchar *mime_root;
   gchar *mime_sub_type;
   gboolean is_symlink;
   GDateTime* file_mtime;
   gchar *owner;
   gchar *group;
   GDateTime* file_atime;
   GDateTime* file_ctime;
   guint32 file_mode;
   gchar * file_mode_str;
   guint64 file_size;
   gchar * mime_sort;
   gchar * link_target_filename;
   gchar * mtime;
   gchar * atime;
   gchar * ctime;
} RFM_FileAttributes;

typedef struct {
  gchar* name;
  gchar* activationKey;
  gchar* prompt;
  gchar** (*cmdTransformer)(gchar *, gboolean inNewVT);
  gchar* HistoryFilename;
} stdin_cmd_interpretor;

typedef struct {
   gchar *buttonName;
   gchar *buttonIcon;
   void (*func)(gpointer);
   const gchar **RunCmd;
   gboolean showInSearchResultView;
   gboolean showInDirectoryView;
   guint Accel;
   gchar *tooltip;
   gboolean (*showCondition)();
} RFM_ToolButton;

typedef struct {
   gchar *runName;
   gchar *runRoot;
   gchar *runSub;
   gchar *filenameSuffix;
   gchar *OrSearchResultType;
   void (*func)(gpointer);
   const gchar **runCmd;
   gboolean (*showCondition)(RFM_FileAttributes*);
} RFM_MenuItem;

typedef struct {
  GtkWidget **tool_bar_buttons_local;
  RFM_ToolButton *toolbar_button_definitions_local;//静态数组在rfm_local.c中定义，但是通过本指针访问
  size_t toolbar_button_definitions_local_array_size;
  GtkWidget **menuItems_local;
  RFM_MenuItem *file_menuitem_definitions_local;
  size_t file_menuitem_definitions_local_array_size;
} RFM_ADDON_RESOURCE_TYPE;

enum RFM_ADDON_TYPE {
  Directory,
  DirectoryTree,
  Global,
};
