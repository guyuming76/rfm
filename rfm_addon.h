#ifndef RFM_ADDON_H
#define RFM_ADDON_H

#ifdef PythonEmbedded
/*
 * GPL-3 by guyuming
 * 目标：消除 rfm.c / rfm_pt_wrapper.c / 各 .rfm_addon.c 插件里大量重复的
 * GIL 管理、引用计数、import/getattr/call/错误处理样板代码。
 *
 * 用法：
 *   1. 函数体开头写 RFM_PY_GIL_SCOPE;（之后任意 return 都会自动释放 GIL）
 *   2. 用 RFM_PY_AUTO(x) 声明临时 PyObject*，作用域结束自动 Py_DECREF
 *   3. 调用 rfm_py_import() / rfm_py_call_attr() 等高阶函数
 *
 * 依赖：GCC/Clang 的 __attribute__((cleanup))（Linux 平台可用）。
 *
 * 本头文件会被 rfm.c 和各个插件 .c 直接包含。
 * 非 inline 的函数实现在 rfm_pybridge.c 中，编译进 rfm 主程序；
 * 插件通过 -rdynamic 导出的符号在 dlopen 时解析（与 add_toolbar_buttons 等一致）。
 */


/* Python.h 先于 glib.h 包含，避免 _POSIX_C_SOURCE 重定义警告 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <glib.h>
/* ── 1. GIL 作用域守卫 ─────────────────────────────────────────────── */
/* 进入作用域自动 PyGILState_Ensure()，离开作用域（含任意 return）自动 Release。 */
typedef struct { PyGILState_STATE state; } RfmPyGil;

static inline RfmPyGil rfm_py_gil_acquire(void) {
    RfmPyGil g = { PyGILState_Ensure() };
    return g;
}
static inline void rfm_py_gil_release(RfmPyGil *g) {
    if (g) PyGILState_Release(g->state);
}

#define RFM_PY_GIL_SCOPE \
    RfmPyGil __rfm_py_gil_ __attribute__((cleanup(rfm_py_gil_release))) = rfm_py_gil_acquire()

/* ── 2. PyObject* 自动 DECREF ──────────────────────────────────────── */
static inline void rfm_py_decref_cleanup(PyObject **pp) {
    if (pp && *pp) { Py_DECREF(*pp); *pp = NULL; }
}

/* 声明一个作用域内自动释放的临时 PyObject*，例如：
 *   RFM_PY_AUTO PyObject *result = rfm_py_call_attr(...);
 * 离开作用域时自动 Py_DECREF（无论正常返回还是提前 return）。
 */
#define RFM_PY_AUTO __attribute__((cleanup(rfm_py_decref_cleanup)))

/* ── 3. 统一错误打印 + 清除 ────────────────────────────────────────── */
/* 返回 FALSE，便于调用方直接 return。当前无异常时不打印。 */
static inline gboolean rfm_py_clear_error(const char *where) {
    if (PyErr_Occurred()) {
        if (where) g_warning("%s:", where);
        PyErr_Print();   /* PyErr_Print 会清除错误指示器 */
    }
    PyErr_Clear();
    return FALSE;
}

/* ── 4. 高阶辅助函数（实现在 rfm_pybridge.c） ──────────────────────── */

/* 导入 Python 模块；path_dir 非 NULL 时先把它插入 sys.path[0]（若不存在）。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_import(const char *module_name, const char *path_dir);

/* 取属性，返回 new reference（等价 PyObject_GetAttrString 但带 NULL 检查）。 */
PyObject *rfm_py_getattr(PyObject *obj, const char *name);

/* 调用 callable(args, kwargs)；args/kwargs 均可为 NULL（NULL 表示空）。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_call(PyObject *callable, PyObject *args, PyObject *kwargs);

/* 调用 obj.method(args, kwargs)；args/kwargs 均可为 NULL。 */
PyObject *rfm_py_call_method_obj(PyObject *obj, const char *method, PyObject *args, PyObject *kwargs);

/* 调用 obj.attr(fmt, ...)。
 * fmt 遵循 Py_BuildValue 格式：可直接写 "(s)"、"(O)"、"(OO)" 产出一个 tuple，
 * 也可写单个格式单元（如 "s"、"O"），此时会自动包成 1-tuple（兼容 PyObject_CallMethod 的写法）；
 * fmt 为 NULL 表示无参调用。
 * 返回 new reference，失败返回 NULL 并设置 Python 异常。 */
PyObject *rfm_py_call_attr(PyObject *obj, const char *attr, const char *fmt, ...);

/* 调用 obj.method(fmt, ...)，与 rfm_py_call_attr 语义一致。 */
PyObject *rfm_py_call_method(PyObject *obj, const char *method, const char *fmt, ...);

/* 把 Python 对象转成 C 字符串（g_malloc，调用方 g_free）。
 * obj 为 NULL 或 Py_None 时返回 NULL；失败时设置 Python 异常。 */
gchar *rfm_py_to_cstr(PyObject *obj);

#endif //PythonEmbedded



//rfm项目里目前有两个.h文件： rfm_local.h 和 config.h.
//最初只有config.h,用于suckless风格的定制，自定义文件菜单之类
//rfm_local.h的加入是为了满足 rfm_local.c 的动态编译。rfm_local.h 要在 rfm.c 和 rfm_local.c 之间共享
//事实上，config.h里面的定义也是依赖rfm_local.h内数据类型定义的，但config.h内无需#include rfm_local.h,这时因为在rfm.c里面对于config.h的引用出现在对rfm_local.h的应用的后面，c语言编译器似乎可以处理这种情况
#include <gtk-3.0/gtk/gtk.h>
#include <wordexp.h>
//在C语言中，使用`-include`参数包含的头文件会出现在所有其他头文件之前。因此，如果在rfm_addon.h中没有包含`glib.h`，那么就会导致这些类型未知。之前在 rfm_addon.c (rfm_local.c) 中，rfm_addon.h是在gtk.h后面include的，现在通过 -include 参数传递给gcc来编译rfm_addon.c, 就不得不在这里加 gtk.h
#define FREE //indicate that it is freed here
#define FREE_FULL //indicate that it is freed full here
#define MEM_ALLOC  //indicate that returned value is allocated memory here
#define MEM_ALLOC_FULL //indicate that returned value is allocated full memory here
#define CALLER_OWN //indicate that caller owns the pointer and should take care of freeing
#define NO_FREE  //indicate that no need to free, for example, a static array compiled in code
#define OWNER_CHANGE //indicate that ownership changed after function calll, need to pay attention.

#define RFM_LOG_DATA "rfm-data"
#define RFM_LOG_DATA_EXT "rfm-data-ext"
#define RFM_LOG_DATA_THUMBNAIL "rfm-data-thumbnail"
#define RFM_LOG_DATA_SEARCH "rfm-data-search"
#define RFM_LOG_DATA_SORT "rfm-data-sort"
#ifdef GitIntegration
#define RFM_LOG_DATA_GIT "rfm-data-git"
#endif
#define RFM_LOG_GSPAWN "rfm-gspawn"
#define RFM_LOG_GSPAWN_READ_PIPE "rfm-gspawn-readpipe"
#define RFM_LOG_COLUMN "rfm-column"
#define RFM_LOG_COLUMN_VERBOSE "rfm-column-verbose"
#define RFM_LOG_GTK "rfm-gtk"
#define RFM_LOG_READLINE "rfm-readline"
#define RFM_LOG_DIMENSION "rfm-dimension"

typedef struct RFM_ChildAttributes RFM_ChildAttribs;
struct RFM_ToolButton;
struct RFM_MenuItem;

typedef struct {
  gchar *cmd;
  void (*action)(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution, RFM_ChildAttribs* childAttribs);
  gchar *help_msg;
} RFM_builtinCMD;

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
  gchar* (*ValueFunc)(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex);
  gchar* MIME_root;
  gchar* MIME_sub;
  gboolean iconview_markup;
  gboolean iconview_tooltip;
  gchar *filenameSuffix;
} RFM_treeviewColumn;

typedef struct {
   gchar *buttonName;
   gchar *buttonIcon;
   void (*func)(struct RFM_ToolButton *);//虽然目前我config.h以及rfm_addon.c中用到的函数参数实际上都是空，但这里传入RFM_ToolButton* 才比较合理，以便将来在rfm_addon里可以动态生成一些按钮，自定义函数可以根据参数判断哪个button被按下。
   const gchar **RunCmd;
   gboolean showInSearchResultView;
   gboolean showInDirectoryView;
   guint Accel;
   gchar *tooltip;
   gboolean (*showCondition)();
} RFM_ToolButton;

/* deepseek-r1>〉我代码中函数指针调用传递一个参数 func(file_menu_item_definition); */
/* ::: 函数指针定义也是需要一个参数 void (*func)(struct RFM_MenuItem *); */
/* ::: 但实际赋值给函数指针的函数定义参数为零，但编译执行都没报错，这和你前面说的不一致啊？ */
/* /home/guyuming/rfm1/rfmAI_deepseek-reasoner___20260115_145409.rfmAI.md */

/* 确认上传这些对话历史吗? [Y/n]  */
/* 从对话记录文件中读取历史消息: /home/guyuming/rfm1/rfmAI_deepseek-reasoner___20260115_145409.rfmAI.md */
/* # 思考: */
/* 您提到的情况确实是一个常见的误解点，让我详细解释一下。 */

/* 首先，根据C语言标准，函数指针的类型必须与所指向的函数类型严格匹配，包括参数个数和类型。如果不匹配，那么行为是未定义（undefined behavior）的。但是，在某些编译环境下，可能会出现您描述的情况：编译通过且运行时没有立即出错。这并不代表代码正确，而是未定义行为的一种表现，可能在某些平台上恰好能运行，但随时可能出错。 */

/* 让我们分析一下您提到的例子： */

/* 1. 函数指针定义：`void (*func)(struct RFM_MenuItem *);` */
/*    - 这是一个指向函数的指针，该函数接受一个指向`struct RFM_MenuItem`的指针作为参数，返回类型为`void`。 */

/* 2. 您将函数指针`func`指向了一个实际函数，但该实际函数的定义参数为零（即没有参数）。假设这个实际函数声明为：`void actual_func();`（注意：这里没有参数列表，或者可能是`void actual_func(void)`？） */

/* 3. 然后通过函数指针调用：`func(file_menu_item_definition);`，传递了一个参数。 */

/* 根据C标准，这是不允许的。但是，为什么编译没有报错呢？可能有以下几种原因： */

/* - **旧式函数声明**：如果实际函数的声明是旧式声明（即没有指定参数列表），例如`void actual_func();`，那么在C语言中，这个声明表示该函数可以接受任意数量的参数（但参数类型和个数都不进行检查）。如果实际函数的定义也是旧式定义（K&R风格），那么编译器可能不会严格检查。 */

/* - **编译器宽松检查**：默认情况下，某些编译器（尤其是GCC）可能会允许这种不匹配，只是给出警告而不是错误。如果您没有开启严格的编译选项（如`-Wall -Wextra -Werror`），那么编译器可能会放行。 */

/* - **调用约定和参数传递**：在x86等架构上，函数参数的传递通常是通过栈或寄存器。如果调用时传递了一个参数，而函数定义中没有参数，那么被调用的函数不会使用这个参数，但调用者仍然会传递。这可能导致栈不平衡吗？不一定，因为调用者会清理栈（取决于调用约定）。但是，如果函数定义中使用了这个参数（而实际上没有传递），就会读取到错误的值。在您的情况下，是调用时传递了参数但函数定义没有参数，那么函数定义中不会使用这个参数，而调用者传递的参数可能仍然在栈上（或寄存器中），但函数不会去使用它。这可能会造成资源浪费，但不一定立即崩溃。 */

/* 然而，请注意，如果实际函数的声明是`void actual_func(void);`，那么它明确表示不接受任何参数。在这种情况下，将函数指针类型为`void (*)(struct RFM_MenuItem *)`指向`actual_func`会导致类型不匹配，编译器应该产生警告或错误。 */


typedef struct {
   gchar *runName;
   gchar *runRoot;
   gchar *runSub;
   gchar *filenameSuffix;
   gchar *OrSearchResultType;
   void (*func)(struct RFM_MenuItem *); //Although i use void here, RFM_MenuItem * might be used in some addon for dynmaically created menuitem
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
   gchar *path;
   gchar *thumb_name;
   gchar *md5;
   gchar *uri;
   gint64 mtime_file;
   gint t_idx;
   pid_t rfm_pid;
   gint thumb_size;
   guint g_source_id_for_load_thumbnail;
} RFM_ThumbQueueData;

typedef struct {
   gchar *thumbRoot;
   gchar *thumbSub;
   gchar *filenameSuffix;
  //GdkPixbuf *(*func)(RFM_ThumbQueueData * thumbData);
   const gchar **thumbCmd;
  gboolean check_tEXt;
} RFM_Thumbnailer;

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
  // 新增：thumbnail 处理相关字段
  //RFM_Thumbnailer *thumbnailer_definitions_local;
  //size_t thumbnailer_definitions_local_array_size;
  GdkPixbuf* (*thumbLoadFunc)(RFM_FileAttributes *fileAttributes);//这个函数运行在gtk主线程，直接根据文件返回图像
  stdin_cmd_interpretor *stdin_cmd_interpretors;
  size_t stdin_cmd_interpretors_array_size;
  stdin_cmd_interpretor*(*check_cmd_interpretor_switch_or_effective_cmd_interpretor)(gchar**, stdin_cmd_interpretor** , gboolean* , gchar**, gchar**);
  RFM_builtinCMD *builtinCMD_definitions_local;
  size_t builtinCMD_definitions_local_array_size;
  char * addon_dir; //TODO:有必要加这个字段吗？通过获取RFM_ADDON指针，然后使用local_c_path的目录不行吗？不安全？
} RFM_ADDON_RESOURCE_TYPE;


typedef struct RFM_ChildAttributes{
   gchar *name;
   const gchar **RunCmd;
   stdin_cmd_interpretor *cmd_interpretor;
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
   gboolean g_idle_add_once_to_run_CallBackFunc;//如果此值设为TRUE,则注意本结构要使用calloc/malloc在堆上分配，而不要直接在函数体内部栈上申明，因为在栈上申明，在callbackfunc后free_childAttribs的时候，childAttribs在栈上可能已经失效了
   gboolean no_free;
   gboolean parse_and_exec_stdin_command_in_gtk_thread_called_nested_so_do_not_new_readlineThread_afterwards;
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
   gboolean execStdinCmdInNewVT;
   gboolean newVT_with_proxy;
   RFM_SearchResultType* SearchResultTypePointer;
   gboolean SearchResultAsInput;
   // used by exec_stdin_command and exec_stdin_command_builtin to share status
   gboolean stdin_cmd_ending_space;
   gboolean stdin_cmd_leading_space;
} RFM_ChildAttribs; //data needed to spawn child process

// ── RfmFifoSet: shared named-pipe utility ────────────────────────────
// Temp directory + up to 3 named pipes.  Indices: 0=stdin, 1=stdout, 2=stderr.
// Used by NewTermProxy (rfm.c) and SubAgentProxy (addon).
#define FIFO_STDIN  0
#define FIFO_STDOUT 1
#define FIFO_STDERR 2

typedef void (*RfmFifoDataCallback)(const char *data, int len, void *user_data);

typedef struct RfmFifoSet {
    char *dir;                    // temp directory, e.g. /tmp/rfm_proxy_XXXXXX
    char *pipe_path[3];           // FIFO_STDIN / FIFO_STDOUT / FIFO_STDERR, NULL if not created
    int   pipe_fd[3];             // opened fds, -1 if not opened
} RfmFifoSet;


enum RFM_ADDON_TYPE {
  Directory,
  DirectoryTree,
  Global,
};

/* ── 插件回调注册 ────────────────────────────────────────────────────
 * rfm 主程序维护一个以插件 id 为 key 的目录切换回调 hashtable。
 * 插件在 rfm_addon_activating() 里调用下面的注册函数；rfm 在
 * set_rfm_curPath_internal() 中遍历并调用这些回调。
 * 推荐使用 rfm_addon_c_file_hash（插件 .c 文件 hash）作为 addon_id，
 * 该值与 rfm 卸载插件时用于删除记录的 key 一致。 */
typedef void (*RFM_DirChangeCallback)(gchar *new_path);

void rfm_addon_register_dir_change_callback(const gchar *addon_id, RFM_DirChangeCallback callback);

#define RFM_LOG_ADDON "rfm-addon"

// 使用编译时gcc -DRFM_ADDON_C_HASH=xxxxxx 传入的值来初始化这个常量指针
// 这个变量会成为 .so 文件中的一个符号，可以被 dlsym 找到
#ifdef RFM_ADDON_C_HASH
  const char *const rfm_addon_c_file_hash = RFM_ADDON_C_HASH;
#endif
#ifdef RFM_ADDON_H_HASH
  const char *const rfm_addon_h_file_hash = RFM_ADDON_H_HASH;
#endif

#endif /* RFM_ADDON_H */
