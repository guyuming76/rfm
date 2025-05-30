//rfm项目里目前有两个.h文件： rfm_local.h 和 config.h.
//最初只有config.h,用于suckless风格的定制，自定义文件菜单之类
//rfm_local.h的加入是为了满足 rfm_local.c 的动态编译。rfm_local.h 要在 rfm.c 和 rfm_local.c 之间共享
//事实上，config.h里面的定义也是依赖rfm_local.h内数据类型定义的，但config.h内无需#include rfm_local.h,这时因为在rfm.c里面对于config.h的引用出现在对rfm_local.h的应用的后面，c语言编译器似乎可以处理这种情况

#define FREE //indicate that it is freed here
#define FREE_FULL //indicate that it is freed full here
#define MEM_ALLOC  //indicate that returned value is allocated memory here
#define MEM_ALLOC_FULL //indicate that returned value is allocated full memory here
#define CALLER_OWN //indicate that caller owns the pointer and should take care of freeing
#define NO_FREE  //indicate that no need to free, for example, a static array compiled in code
#define OWNER_CHANGE //indicate that ownership changed after function calll, need to pay attention.

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

typedef struct {
  gchar *calling_script;
  gchar *host_url;
  gchar *model_name;
  gchar *term_prompt; // This is the prompt such b> shown in term, not the prompt for LLM
  gchar *activationKey;
  gboolean InNewTerm;
  gchar *model_temporature;
  gchar *API_key;
} AI_model_parameter;

typedef struct RFM_ChildAttributes{
   gchar *name;
   const gchar **RunCmd;
   GSpawnFlags  runOpts;
   gchar** env;
   GPid  pid;
   gint  stdIn_fd;
   gint  stdOut_fd;
   gint  stdErr_fd;
   char *stdOut;
   char *stdErr;
   int   status;
   void (*customCallBackFunc)(struct RFM_ChildAttributes*); //In Searchresultviewinsteadofdirectoryview situation, after runAction such as Move, i need to fill_store to reflect remove of files, so, i need a callback function. Rodney's original code only deals with working directory, and use INotify to reflect the change.
   gpointer customCallbackUserData; //this is not freed in free_child_attribs, user should free it.
   gboolean spawn_async;
   gint exitcode;
   gboolean output_read_by_program;
} RFM_ChildAttribs;


enum RFM_ADDON_TYPE {
  Directory,
  DirectoryTree,
  Global,
};

#define RFM_LOG_ADDON "rfm_addon"
