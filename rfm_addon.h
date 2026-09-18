// RFM_INCLUDED_HEADER_HASHES_BEGIN
//   本区块由 config.mk 调用 scripts/rfmSyncAddonIncludedHeaderHashes.sh 自动生成，请勿手工编辑。
//   作用：这里列出的头文件一旦变化，rfm_addon.h 的内容就变化 ⇒ RFM_AddOn(=rfm_addon.h 的 md5) 变
//         ⇒ 重编 rfm ⇒ 运行时 rfm 用新 md5 与各插件 .so 里记录的旧值比对 ⇒ 触发所有插件重编。
// RFM_INCLUDED_HEADER_HASH: rfmLanguage.h 1ced54b200e46a2d992e2b8ecaed1743    <- Chinese.h
// RFM_INCLUDED_HEADER_HASH: rfm_pybridge.h 7a5aa8def4270765b44f9d3b855a1a02    <- rfm_pybridge.h
// RFM_INCLUDED_HEADER_HASHES_END
#ifndef RFM_ADDON_H
#define RFM_ADDON_H

#ifdef PythonEmbedded
#include "rfm_pybridge.h"
#endif

#ifdef RFM_ADDON_H_HASH
#include "rfmLanguage.h"
#endif

//最初只有config.h,用于suckless风格的定制，自定义文件菜单之类
//rfm_addon.h的加入是为了满足 rfm_addon.c 的动态编译。rfm_addon.h 要在 rfm.c 和 rfm_addon.c 之间共享
//事实上，config.h里面的定义也是依赖rfm_addon.h内数据类型定义的，但config.h内无需#include rfm_local.h,这时因为在rfm.c里面对于config.h的引用出现在对rfm_local.h的应用的后面，c语言编译器似乎可以处理这种情况
#include <gtk-3.0/gtk/gtk.h>
#include <wordexp.h>
#include <linux/limits.h> //PATH_MAX，rfm.c 也用它；插件里要声明 rfm_historyFile_ParentDirectory[PATH_MAX]，所以本头文件必须自足
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
  gchar** (*cmdTransformer)(RFM_ChildAttribs*);//这个函数，1. 最初用于为用户输入的命令套壳，套壳后的命令作为g_spawn_wrapper 系列函数的参数，用于生成rfm的子进程执行. 比如b]提示符下的bash命令解释器，用户输入的命令无论是否带&后缀，都会g_spawn出子进程执行，区别只是根据 childattribs->execstdincmdinnewvt的值生成不同的命令套壳。2. 后来，对于TinyAgent这样的rfm进程内命令处理程序，这个函数专名处理其用户输入的带&后缀的命令。
  gchar* HistoryFilename;
  void (*customCallBackFunc)(RFM_ChildAttribs*);//这个函数用于：1. rfm进程内的命令处理程序，如TinyAgent用户输入的不带&后缀的命令。2. 之所以函数名叫这个，是因为最初用于 rfmAskOpenRouter.sh 等脚本AI 完成后的回调，这类脚本会在rfm 个gpawn出的子进程中执行，执行完调用此rfm进程内回调函数，处理一些rfm GUI显示上的任务，比如把最新保存的会话历史记录添加进rfm查询结果窗口。和上面cmdTransformer 函数类似，对于不同的命令解释程序，这个函数可以有不同的用法.
  //总结一下：命令解释器入口无非工作在rfm进程内或g_spawn出的子进程。1. 在进程内时，需要customCallBackFunc作为入口(这种情况下，这个指针名的确有些令人困惑)，并且这种情况下，&后缀的命令是不可能在rfm进程内执行的，可以把cmdTransformer用做套壳命令生成器入口，而这两个指针实际指向命令解释器的同一入口函数。**这里两个函数指针指向同一个函数是作为命令解释器的这种工作模式的识别标志**2. 作为rfm g_spawn出的子进程时， cmdTransformer 函数被用于生成g_spawn需要的命令套壳，无论命令是否带&后缀，而 customCallBackFunc 正好被用来作为回调函数(这时候这个指针名就不会显得名不副实了)，执行一些必须在rfm进程内完成的动作.
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

/* ══════════════════════════════════════════════════════════════════════════
 * 可供插件调用的宿主函数与全局变量（本区是全项目里这些符号的【唯一权威声明】）
 *
 * 为什么放在这里：
 *  1. rfm.c 也 #include 本头文件（rfm.c 顶部），所以宿主里的定义若与这里不一致，
 *     宿主编译就会报 "conflicting types"——"签名变了"会在编译期被抓住，
 *     而不是等插件加载后崩在调用点（动态加载只按符号名解析，C 的符号名不含类型）。
 *  2. 插件里不要再手写 extern。手写的 extern 一旦过期，gcc 会报
 *     "conflicting types for 'xxx'" 并给出新旧两个类型，明确指出是哪个函数/变量对不上。
 *  3. 声明集中在文件末尾，是因为必须在所有 typedef 之后（RFM_FileAttributes/
 *     RFM_ChildAttribs/RfmFifoSet/RFM_ToolButton/RFM_MenuItem 等）。
 *  4. 【重要】全局变量只在这里写 extern 声明（extern 只是声明，不产生实例）；
 *     真正的定义仍在 rfm.c / config.h 里。宿主侧目前只有 rfm.c 一个 TU 包含本头文件
 *     （rfm_pybridge.c 只包含 rfm_pybridge.h）。将来若想在这里写"定义"而不是 extern，
 *     必须保证宿主侧只有一个 TU 包含本头：一旦多出第二个（比如新增 rfm_xxx.c 也包含本头），
 *     立刻就是链接期 multiple definition。所以现阶段请一律写 extern。
 *  5. 关于"rfm.c 里定义了某个变量，又在同一个 TU 里看到它的 extern 声明"是否矛盾：
 *     不矛盾，也不是笔误。extern 表示"这个名字具有外部链接、此处（在该点）不是它的定义"，
 *     并不要求定义在别的文件里；同一个 TU 里先 `extern int x;` 再 `int x = 5;` 是标准写法
 *     （C11 6.2.2/6.9.2），本文件上面的 rfm_addon_register_dir_change_callback、
 *     以及所有 C 库的"头文件里声明 + .c 里定义（.c 包含自己的头）"都是这个模式。
 *     它的价值恰恰在于：声明与定义一旦不一致，编译期就报 conflicting types。
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── 全局变量（定义在 rfm.c / config.h；插件通过这里的 extern 引用同一个实例） ── */
extern char rfm_historyFile_ParentDirectory[PATH_MAX];              //rfm.c
extern const stdin_cmd_interpretor stdin_cmd_interpretors[];        //config.h（用户定制）
extern stdin_cmd_interpretor *current_stdin_cmd_interpretor_pointer;//rfm.c
extern gchar *Addon_Prompt;                                         //rfm.c，addon 可临时改写提示符
extern gboolean exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread;//config.h（用户定制）
extern gchar *stdin_cmd_template_bash[];                            //rfm.c
extern gchar *stdin_cmd_template_bash_newVT_nonHold[];              //rfm.c
extern gchar *stdin_cmd_template_bash_newVT_nonHold_with_e[];       //rfm.c
extern GtkIconTheme *icon_theme;                                    //rfm.c
extern int RFM_ICON_SIZE;                                           //config.h（用户定制）
extern int currentPageStartingColumnTitleIndex;                     //rfm.c
extern gchar *rfm_SearchResultPath;                                 //rfm.c，创建查询结果时的 rfm_curPath

/* ── 历史记录 / 命令行 ── */
void load_history_for_prompt();                                                                                                                             //rfm.c
void save_current_history();                                                                                                                                //rfm.c
gchar** replace_shell_cmd_buffer(gchar **strv, gchar *buffer);                                                                                               //rfm.c
NO_FREE gchar** stdin_command_bash(RFM_ChildAttribs* childAttribs);                                                                                          //rfm.c
gboolean parse_and_exec_command_in_gtk_thread (FREE gchar * readlineResult, gboolean const from_stdin_readline, NO_FREE const stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs* childAttribs); //rfm.c

/* ── 菜单 / 工具栏 ── */
void add_toolbar_buttons(GtkWidget ***buttons, const RFM_ToolButton button_definitions[], const size_t size);      //rfm.c
void add_file_menuitems(GtkWidget ***menuitems, const RFM_MenuItem menuitem_definitions[], const size_t size);     //rfm.c
void remove_toolbar_buttons(FREE GtkWidget ***buttons, const size_t size);                                         //rfm.c
void remove_file_menuitems(FREE GtkWidget ***menuitems, const size_t size);                                        //rfm.c

/* ── 查询结果处理 ──
 * 注意 ProcessJsonInCmdOutputFromSearchResult 等的参数必须是 const 的：它们的地址
 * 会赋给 RFM_SearchResultType.SearchResultLineProcessingFunc（见上），那个成员的类型
 * 就是 const char** / const RFM_FileAttributes*；非 const 的签名会被 gcc 判为
 * incompatible pointer type（插件编译命令里没有 -Wno-error=incompatible-pointer-types）。 */
const char* rfm_get_search_result_column_seperator(const RFM_ChildAttribs* ca);                                     //rfm.c
void update_SearchResultFileNameList_and_refresh_store(RFM_ChildAttribs *childAttribs);                             //rfm.c
void CallDefaultSearchResultFunctionForNewSearch(CALLER_OWN char *one_line_in_search_result, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs); //rfm.c
int ProcessKeyValuePairInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdtemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* childAttribs); //rfm.c
int ProcessJsonInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdtemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* child_attribs);         //rfm.c
void ProcessKeyValuePair(CALLER_OWN gchar *key, OWNER_CHANGE gchar* currentExtColumnValue, CALLER_OWN char* currentColumnOriginalTitle); //rfm.c
void addToSearchResult(FREE char *fileFullpath);                                                                   //rfm.c
void writeSearchResultIntoFD_in_gtk_thread(gint fd, gboolean exportFilePathOnly, RFM_ChildAttribs* childAttribs);    //rfm.c，TinyAgent 插件用 memfd 取查询结果文本

/* ── 子进程 / 命名管道 ── */
gboolean g_spawn_wrapper(CALLER_OWN const char **action, CALLER_OWN GList *file_list, const int run_opts, FREE const gchar** env, const char *dest_path, const gboolean async,const void(*callbackfunc)(gpointer),CALLER_OWN const gpointer callbackfuncUserData,const gboolean output_read_by_program); //rfm.c
gboolean g_spawn_wrapper_(CALLER_OWN GList *file_list, CALLER_OWN const char *dest_path, FREE_FULL RFM_ChildAttribs * childAttribs); //rfm.c
RfmFifoSet *rfm_fifo_set_create(const char *dir_template, int pipe_mask);                                          //rfm.c
gboolean rfm_fifo_open_read(RfmFifoSet *fset);                                                                     //rfm.c
gboolean rfm_fifo_open_write(RfmFifoSet *fset, gboolean nonblock);                                                 //rfm.c
void rfm_fifo_poll_read(RfmFifoSet *fset,
    RfmFifoDataCallback on_stdout, RfmFifoDataCallback on_stderr,
    void *stdout_ud, void *stderr_ud, int timeout_ms);                                                               //rfm.c
void rfm_fifo_set_cleanup(RfmFifoSet *fset);                                                                       //rfm.c
void g_string_append_len_callback(const char *data, int len, void *user_data);                                     //rfm.c

#define RFM_LOG_ADDON "rfm-addon"

// 使用编译时gcc -DRFM_ADDON_C_HASH=xxxxxx 传入的值来初始化这个常量指针
// 这个变量会成为 .so 文件中的一个符号，可以被 dlsym 找到
#ifdef RFM_ADDON_C_HASH
  const char *const rfm_addon_c_file_hash = RFM_ADDON_C_HASH;
#endif
#ifdef RFM_ADDON_H_HASH
  const char *const rfm_addon_h_file_hash = RFM_ADDON_H_HASH;
#endif
#endif
