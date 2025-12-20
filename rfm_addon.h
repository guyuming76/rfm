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

typedef struct RFM_ChildAttributes RFM_ChildAttribs;

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
  void (*customCallBackFunc)(RFM_ChildAttribs*);
} stdin_cmd_interpretor;

enum RFM_treeviewCol{
   COL_ICONVIEW_MARKUP,
   COL_ICONVIEW_TOOLTIP,
   COL_PIXBUF,
   COL_MODE_STR,
   //COL_DISPLAY_NAME,
   COL_FILENAME,
   COL_LINK_TARGET,
   COL_FULL_PATH,
   COL_MTIME,
   COL_MTIME_STR,
   COL_SIZE,
   COL_SIZE_DISPLAY,
   COL_ATTR,
   COL_OWNER,
   COL_GROUP,
   COL_MIME_ROOT,
   COL_MIME_SUB,
   COL_MIME_SORT, //mime root + sub for listview sort
   COL_ATIME_STR,
   COL_CTIME_STR,
#ifdef GitIntegration
   COL_GIT_STATUS_STR,
   COL_GIT_COMMIT_MSG,
   COL_GIT_AUTHOR,
   COL_GIT_COMMIT_DATE,
   COL_GIT_COMMIT_ID,
#endif
   COL_Ext1,COL_Ext2,COL_Ext3,COL_Ext4,COL_Ext5,COL_Ext6,COL_Ext7,COL_Ext8,COL_Ext9,
   COL_Ext10,COL_Ext11,COL_Ext12,COL_Ext13,COL_Ext14,COL_Ext15,COL_Ext16,COL_Ext17,COL_Ext18,COL_Ext19,
   COL_Ext20,COL_Ext21,COL_Ext22,COL_Ext23,COL_Ext24,COL_Ext25,COL_Ext26,COL_Ext27,COL_Ext28,COL_Ext29,
   COL_Ext30,COL_Ext31,COL_Ext32,COL_Ext33,COL_Ext34,COL_Ext35,COL_Ext36,COL_Ext37,COL_Ext38,COL_Ext39,
   COL_Ext40,COL_Ext41,COL_Ext42,COL_Ext43,COL_Ext44,COL_Ext45,COL_Ext46,COL_Ext47,COL_Ext48,COL_Ext49,
   COL_Ext50,COL_Ext51,COL_Ext52,COL_Ext53,COL_Ext54,COL_Ext55,COL_Ext56,COL_Ext57,COL_Ext58,COL_Ext59,
   COL_Ext60,
   NUM_COLS,
};
#define NUM_Ext_Columns (NUM_COLS - COL_Ext1)

#define ColumnTitleMaxLen 256
typedef struct {
  gchar title[ColumnTitleMaxLen];
  enum RFM_treeviewCol enumCol;
  gboolean Show;
  GtkTreeViewColumn* gtkCol;
  gboolean (*showCondition)(RFM_FileAttributes * fileAttributes);
  enum RFM_treeviewCol enumSortCol;
  gchar* ValueCmd;
  gchar* (*ValueFunc)(guint,...); //参见 static gchar* getExtColumnValueFromHashTable(guint fileAttributeId, guint ExtColumnHashTableIndex)
  gchar* MIME_root;
  gchar* MIME_sub;
  gboolean iconview_markup;
  gboolean iconview_tooltip;
  gchar *filenameSuffix;
} RFM_treeviewColumn;

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
   void (*func)(gpointer); //参数指针传入的是没用的rfmCtx，也可以使用不带参数的函数，执行时根据当前selection获得文件
   const gchar **runCmd;
   gboolean (*showCondition)(RFM_FileAttributes*);
} RFM_MenuItem;

typedef struct {
  gchar* name;
  int (*SearchResultLineProcessingFunc)(gchar* oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs); //when new_search is TRUE, fileattributes will be NULL. Note that not all ProcessingFunc has these four parameters defined, mostly not. 参见 ProcessCmdOutputFromSearchResultWithFunction
  char** showcolumn; //the same type of string used in showcolumn builtin command. 参见 showSearchResultExtColumnsBasedOnHashTableValues 函数。事实上，查询结果标准输出里面出现的列(default SearchResultLineProcessingFunc 读取解析，保存在hashtable里的列)，以及其他非默认 SearchResultLineProcessingFunc 从查询结果里解析出来，保存在hashtable里的列，都不用在这里指明，都会自动显示。需要在这里指明的一种是那种“单独定义”的列。那些自动显示的扩展列会统一使 getExtColumnValueFromHashTable 取值函数，而这里需要单独指定的列使用自定义的函数或命令取值. 另一种需要在这里指明的是诸如文件大小日期等常规列(当自动显示包含至少一个使用getExtColumnValueFromHashTable的扩展列的时候),如果查询结果不包含使用getExtColumnValueFromHashTable的扩展列,则会自动显示那些文件大小日期等常规列,如同目录视图一样. 注意，当是multitype时，showcolumn就不再是char**，而被赋值为GPtrArray*,为了方便动态加入字符串元素
  const char* SearchResultColumnSeperator;
  const char** cmdTemplate;//used by ProcessKeyValuePairInCmdOutputFromSearchResult
  const char* Description;
} RFM_SearchResultType;

typedef struct {
  RFM_treeviewColumn *treeviewColumns;
  size_t treeviewColumns_array_size;
  GtkWidget **tool_bar_buttons_local;
  RFM_ToolButton *toolbar_button_definitions_local;//静态数组在rfm_local.c中定义，但是通过本指针访问
  size_t toolbar_button_definitions_local_array_size;
  GtkWidget **menuItems_local;
  RFM_MenuItem *file_menuitem_definitions_local;
  size_t file_menuitem_definitions_local_array_size;
  RFM_SearchResultType *searchresulttypes;
  size_t searchresulttypes_array_size;
  stdin_cmd_interpretor **stdin_cmd_interpretors;
  size_t stdin_cmd_interpretors_array_size;
  stdin_cmd_interpretor*(*check_cmd_interpretor_switch_or_effective_cmd_interpretor)(gchar**, stdin_cmd_interpretor** , gboolean* , gchar**, gchar**);
  char * addon_dir; //TODO:有必要加这个字段吗？通过获取RFM_ADDON指针，然后使用local_c_path的目录不行吗？不安全？
} RFM_ADDON_RESOURCE_TYPE;


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
   void (*customCallBackFunc)(RFM_ChildAttribs*); //In Searchresultviewinsteadofdirectoryview situation, after runAction such as Move, i need to fill_store to reflect remove of files, so, i need a callback function. Rodney's original code only deals with working directory, and use INotify to reflect the change.
   gpointer customCallbackUserData; //this is not freed in free_child_attribs, user should free it.
   gboolean g_idle_add_once_to_run_CallBackFunc;
   gboolean no_free;
   gboolean spawn_async;
   gint exitcode;
   gboolean output_read_by_program;
   gboolean keep_total_buffer_in_show_child_output;
   char* total_stdOut;
   char* total_stdErr;
   gchar *workingDir;//一条命令可以涉及到多个addon,所以下面添加了按类型分的_addon_dir属性，到命令g_spawn 的时候再根据具体情况设置g_spawn的工作目录参数
   gchar *toolbar_button_or_filemenu_addon_dir;
   gchar *searchresulttype_addon_dir;
   gchar *cmd_interpretor_addon_dir;
   GString *readlineResultString;
   gchar *rfmEditFilePath;
   gboolean setsid;
   GList* fullpath_list_as_RunCmd_parameters;
   gboolean exec_stdin_command_in_gtk_thread;
} RFM_ChildAttribs;


enum RFM_ADDON_TYPE {
  Directory,
  DirectoryTree,
  Global,
};

#define RFM_LOG_ADDON "rfm_addon"
