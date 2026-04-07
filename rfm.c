/* RFM - Rod's File Manager,  enhancements made by guyuming, and license changed from MIT to GPL by guyuming
 * See LICENSE file for copyright and license details.
 * Edit config.h to define run actions and set options
 * Compile with the supplied Makefile.
 */

#include "gdk/gdkkeysyms.h"
#include "pytypedefs.h"
#include <linux/limits.h>
#include <stddef.h>
#define _GNU_SOURCE

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <gio/gunixmounts.h>
#include <glib-unix.h>
#include <mntent.h>
#include <icons.h>
#include <readline/readline.h> //在使用 reedline 时，我们还是会使用gnu readline 的filename completion, 不考虑 linenoise, 所以我们总是加上gnu readline
#ifdef GNU_readline
#include <readline/history.h>
#endif
#ifdef reedline
#include "reedline_wrapper.h"
#endif
#include <wordexp.h>
#include <stdarg.h>
#include <regex.h>
#include <gio/gdesktopappinfo.h>
#include <json-glib/json-glib.h>
#ifdef RFM_AddOn
#include <dlfcn.h>
#include <glib/gchecksum.h> // For file hashing
#endif
#include <rfm_addon.h>

#define INOTIFY_MASK IN_MOVE|IN_CREATE|IN_CLOSE_WRITE|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF
#define PIPE_SZ 65535      /* Kernel pipe size */

static RFM_ChildAttribs *childAttribsForCurrentDisplayingSearchResult=NULL;//这个用来保持查询视图当前查询运行时的值，查询刷新时使用
static GUnixMountMonitor *rfm_mountMonitor=NULL;   /* Reference for monitor mount events */
static guint delayedRefresh_GSourceID=0;  /* Main loop source ID for refresh_store() delayed refresh timer */
static gchar*  PROG_NAME = NULL;
static gchar *rfm_homePath;         /* Users home dir */
static char *initDir=NULL;
static char cwd[PATH_MAX];
static struct sigaction ignore_SIGINT;

static GtkWidget *icon_or_tree_view = NULL; //this should be in the gtk ui section, however, it does not compile if moved there
static gboolean treeview=FALSE; //在commit 4960886ed663d74170940ea2ff299ba5ba0199cc 里，可能就是为了代码好看，把这一行随手挪下面去了，结果rfmFileChooser.h就编译不了了
static void die(const char *errstr, ...);
static int setup();
static void cleanup();
gpointer transferPointerOwnership(gpointer *newOwner, gpointer *oldOwner);
gboolean str_regex_replace(char **str, const regex_t * restrict pattern_compiled, const char *replacement);
static gchar *rfm_curPath=NULL;  /* The current directory */
/****************************************************/
/******Terminal Emulator related definitions*********/
#ifdef PythonEmbedded
#define PY_SSIZE_T_CLEAN
#include <Python.h>
/*https://docs.python.org/3.10/extending/embedding.html*/
static  wchar_t *pyProgramName=NULL;
static void startPythonEmbedding(){
  pyProgramName = Py_DecodeLocale(PROG_NAME, NULL);
  if (pyProgramName == NULL) {
        g_warning("Py_DecodeLocale: cannot decode argv[0]");
	return;
  }
  Py_SetProgramName(pyProgramName);  /* optional but recommended */
  Py_InitializeEx(0);//参数`0`禁用Python的信号处理器，减少意外退出的风险
  if (!Py_IsInitialized()) {
            g_warning("Python embedding initialization failed. Continuing without Python.");
            if (PyErr_Occurred()) PyErr_Print();
  } else {
    g_debug("Python embeding initialized successfully");// 添加详细诊断信息
    PyEval_InitThreads(); // 启用线程支持,没这个新线程运行pyftpdlib 服务客户端就连不上
    PyEval_SaveThread(); // 释放GIL
  }
}

static void endPythonEmbedding(){
    if (Py_IsInitialized() && Py_FinalizeEx() < 0) {
        g_warning("Py_FinalizeEx()<0");
	return;
    }
    PyMem_RawFree(pyProgramName);
    pyProgramName=NULL;
}

static PyObject *py_rfmShareDir_module=NULL;
static PyObject *pFactoryClass=NULL;
static PyObject *p_ftp_manager=NULL;
static PyObject *p_http_manager=NULL;
static void cmdStartShare(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution) {
    gint port = -1;
    gchar password[20] = {'p','a','s','s','w','o','r','d','\0'};
    gchar perm[20] = {'e','l','r','a','d','f','m','w','\0'};
    gboolean ftp =False;
    gboolean http=False;
    gboolean *no_firewall = False;
    GError *error = NULL;
    GOptionEntry entries[] = {
      {"port", 'p', 0, G_OPTION_ARG_INT, &port, "FTP port", "PORT"},
      {"password", 'w', 0, G_OPTION_ARG_STRING, &password, "FTP password", "PWD"},
      {"perm", 'r', 0, G_OPTION_ARG_STRING, &perm, "user permission default elradfmw", "permission default elradfmw"},
      {"no_firewall", 'n', 0, G_OPTION_ARG_NONE, &no_firewall, "without firewall command", NULL},
      {"ftp", 'f', 0, G_OPTION_ARG_NONE, &ftp, "ftp server", NULL},
      {"http", 'h', 0, G_OPTION_ARG_NONE, &http, "http server", NULL},
      {NULL}
    };
    GOptionContext *context = g_option_context_new("ShareDir");
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse_strv(context, &(parsed_msg->we_wordv), &error)) {
      g_warning("错误: %s", error->message);
      g_error_free(error);
      return;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if (py_rfmShareDir_module==NULL){
      // 添加当前执行文件绝对路径到sys.path
      char buf[PATH_MAX];
      char *program_dir = NULL;
      ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX - 1);
      if (len != -1) {
        buf[len] = '\0';                       // 确保字符串正确终止
        program_dir = g_path_get_dirname(buf); // 获取目录部分
      } else {
        g_debug("readlink /proc/self/exe return -1");
        return;
      }
      PyRun_SimpleString("import sys, os");
      gchar *insert_program_dir_to_path = g_strdup_printf("sys.path.insert(0, '%s')", program_dir);
      PyRun_SimpleString(insert_program_dir_to_path);
      g_free(insert_program_dir_to_path);
      // 导入自定义模块
      py_rfmShareDir_module = PyImport_ImportModule("rfmShareDir");
    }
    if (py_rfmShareDir_module != NULL) {
      // ensure ServiceManagerFactory类
      if (!pFactoryClass && !(pFactoryClass = PyObject_GetAttrString(py_rfmShareDir_module, "ServiceManagerFactory"))){
        if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}
        Py_DECREF(py_rfmShareDir_module); py_rfmShareDir_module = NULL;
        goto quit;
      }
      if (!PyObject_HasAttrString(pFactoryClass, "create_manager")) {
	Py_DECREF(pFactoryClass); pFactoryClass = NULL;
        Py_DECREF(py_rfmShareDir_module); py_rfmShareDir_module = NULL;
	g_warning("no create_manager method for ServiceManagerFactory");
        goto quit;
      }

      if (ftp){
      if ((p_ftp_manager || (p_ftp_manager = PyObject_CallMethod(pFactoryClass, "create_manager", "s", "ftp")))){
	PyObject *start_server_method = NULL;
        if ((start_server_method = PyObject_GetAttrString(p_ftp_manager, "start_server"))) {
	    PyObject *args = Py_BuildValue("sisssO", "0.0.0.0",port,"rfm", password, rfm_curPath, (no_firewall ? Py_False : Py_True)); // 构建位置参数元组
	    PyObject *kwargs = Py_BuildValue("{s:s}", "perm", perm); //构建关键字参数字典
            PyObject *pResult = PyObject_Call(start_server_method, args,kwargs);
            if (PyErr_Occurred()) {
              PyErr_Print(); // 打印异常信息
              PyErr_Clear(); // 清除异常状态，以便程序继续运行（可选）
            } else Py_XDECREF(pResult);
	    Py_XDECREF(start_server_method);
	    Py_XDECREF(args);
	    Py_XDECREF(kwargs);
        }else{ if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}}
      }else { if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}}
      }//ftp

      if (http){
      if ((p_http_manager || (p_http_manager = PyObject_CallMethod(pFactoryClass, "create_manager", "s", "http")))){
        if (PyObject_HasAttrString(p_http_manager, "start_server")) {
            PyObject *pResult = PyObject_CallMethod(p_http_manager, "start_server", "siOssO","0.0.0.0",port,Py_None,password,rfm_curPath,no_firewall?Py_False:Py_True);
            if (PyErr_Occurred()) { PyErr_Print(); PyErr_Clear();
            } else Py_XDECREF(pResult);
        }else{ g_warning("no start_server method for http_manager");}
      }else {if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}}
      }//http
      if (!ftp && !http) g_warning("add --ftp or --http or both as parameter, at least one of them");
    }else{ PyErr_Print();PyErr_Clear();} //py_rfmShareDir_module == NULL
 quit:
    PyGILState_Release(gstate);
    g_option_context_free(context);
}

static void cmdStopShare(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  if (p_ftp_manager){
    if (PyObject_HasAttrString(p_ftp_manager, "stop_server")) {
    PyObject *pResult = PyObject_CallMethod(p_ftp_manager, "stop_server", NULL);
    if (PyErr_Occurred()) { PyErr_Print();PyErr_Clear();
    } else {
      if (pResult == Py_True){
	Py_XDECREF(p_ftp_manager);
	p_ftp_manager = NULL;
      }
      Py_XDECREF(pResult); // 确保释放 result（如果非空）
    }
    }else g_warning("no stop_server method for ftp_manager");
  }else {g_debug("no ftp_manager");}
  
  if (p_http_manager){
    if (PyObject_HasAttrString(p_http_manager, "stop_server")) {
    PyObject *pResult = PyObject_CallMethod(p_http_manager, "stop_server", NULL);
    if (PyErr_Occurred()) { PyErr_Print(); PyErr_Clear();
    } else {
      if (pResult == Py_True){
	Py_XDECREF(p_http_manager);
	p_http_manager = NULL;
      }
      Py_XDECREF(pResult); // 确保释放 result（如果非空）
    }
    }else g_warning("no stop_server method for http_manager");
  }else g_debug("no http_manager");
  PyGILState_Release(gstate);
}

static void change_ftp_server_root(gchar *newroot) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  if (p_ftp_manager){
    if (PyObject_HasAttrString(p_ftp_manager, "change_root_directory")) {
      PyObject *pResult = PyObject_CallMethod(p_ftp_manager, "change_root_directory", "s", rfm_curPath);
      if (PyErr_Occurred()) { PyErr_Print(); PyErr_Clear();
      } else g_debug("ftp server root changed to %s",rfm_curPath);
      Py_XDECREF(pResult); // 确保释放 result（如果非空）
    }else g_warning("no change_root_directory method for ftp_manager");
  }else g_debug("no ftp_manager");
  if (p_http_manager){
    if (PyObject_HasAttrString(p_http_manager, "change_root_directory")) {
      PyObject *pResult = PyObject_CallMethod(p_http_manager, "change_root_directory", "s", rfm_curPath);
      if (PyErr_Occurred()) { PyErr_Print(); PyErr_Clear();
      } else g_debug("http server root changed to %s",rfm_curPath);
      Py_XDECREF(pResult); // 确保释放 result（如果非空）
    }else g_warning("no change_root_directory method for http_manager");
  }else g_debug("no http_manager");
  PyGILState_Release(gstate);
}  
#endif

enum rfmTerminal {
  NO_TERMINAL,
  NEW_TERMINAL,
  INHERIT_TERMINAL,
};

typedef struct {
  gchar* pattern;
  gchar* replacement;
  void (*action)(void);
  regex_t * restrict pattern_compiled;
} RFM_cmd_regex_rule;

typedef struct {
  gchar *cmd;
  void (*action)(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution, RFM_ChildAttribs* childAttribs);//this point to function which takes three parameters, but in config.h, i assign it with function taking not parameter, it still works! Refer to comments in rfm_addon.h for explanation which mentions that GCC allows this.
  //childAttribs参数是后加的，按理说可以把前两个参数合并到childAttribs成员里，这里只要一个参数就可以了。但这样要改现有的buildin_cmd，特别是我还不知道有没有别人在用这一块，所以就扩充一个参数，暂时不管冗余了
  gchar *help_msg;
} RFM_builtinCMD;


static char* pipefd="0";
static FILE *pipeStream=NULL;
// I need a method to show in stdin prompt whether there are selected files in
// gtk view. if there are, *> is prompted, otherwise, just prompt >
static gint ItemSelected = 0;
static char *rfm_selection_completion = NULL;
static GMutex rfm_selection_completion_lock;
static char selected_filename_placeholder_in_space[34];       //" %s "  by default
static char selected_filename_placeholder_in_quotation[34];   //"'%s'"  by default

static gboolean execStdinCmdInNewVT = FALSE;
// since it's may be complicated if possible to update stdin prompt whenever the terminal window get focus, i just show ItemSelected prompt in new prompt, and user press two times to refresh gtk view.So, I need a way to recognize consecutive enter press.
static time_t lastEnter;

static gboolean stdin_cmd_leading_space = FALSE;
static enum rfmTerminal rfmStartWithVirtualTerminal = INHERIT_TERMINAL;
static guint stdin_command_Scheduler=0;
static GThread * readlineThread=NULL;
/*keep the original user inputs for add_history*/
static gchar *OriginalReadlineResult = NULL;
static guint history_entry_added=0;
char rfm_historyFile_ParentDirectory[PATH_MAX] = { '\0' };
static char *rfm_historyFileLocation;
static char *rfm_historyDirectory_FileLocation;//used by script rfmReadlineWithSpecificHistoryFile.sh 
static gboolean showHelpOnStart = TRUE;
static char* InheritedPrompt = "";
#ifdef reedline
static ReedlineHandle *reedline_handle = NULL;
static rfmFunctions functionPointersForReedlineWrapper;
#endif

#ifdef RFM_AddOn
static gchar* Addon_Prompt = NULL;
static NO_FREE char *AddOnStatus(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex);
static void unload_addon_filemenu_func();
#endif
#ifdef reedline
static GPtrArray *rfm_filename_completion(const char *text, int *count);
#endif
#ifdef GNU_readline
extern int rl_done;
int rl_event_hook_to_help_terminate_readline(void){return 0;}
static char **rfm_filename_completion(const char *text, int *count);
static char** gnu_readline_completion(const char *text, int start, int end);
static char** (*OLD_rl_attempted_completion_function)(const char *text, int start, int end);
#endif
void handle_commandline_argument(char* arg);
int readlink_proc_self_fd_pipefd_and_confirm_pipefd_is_used_by_pipeline();
void ensure_fd0_is_opened_for_stdin_inherited_from_parent_process_instead_of_used_by_pipeline_any_more();
static gboolean startWithTerm();
static void readlineInSeperateThread(RFM_ChildAttribs *childAttribs);
static void ensure_foreground_process(void);
static void ReadFromPipeStdinIfAny(char *fd);
static void exec_stdin_command_in_new_VT(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *child_attribs);
static gboolean exec_stdin_command(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *childAttribs);
static void g_spawn_async_callback_to_new_readline_thread(RFM_ChildAttribs *child_attribs);
static void regcomp_for_regex_rules();
static void set_stdin_cmd_leading_space_true();
static NO_FREE stdin_cmd_interpretor* match_cmd_str_array_with_interpretors(gchar** RunCmd);
static void combine_command_with_selected_files_and_setenv(CALLER_OWN GString* readlineResultString, gboolean cmd_end_with_space, gchar** *env, GList* fullpath_list_as_parameter);
gboolean parse_and_exec_command_in_gtk_thread (FREE gchar * readlineResult, gboolean const from_stdin_readline, NO_FREE const stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs* childAttribs);
static void parse_and_exec_stdin_command_in_gtk_thread(gchar *readlineResult);
static void parse_and_exec_cmd_in_gtk_thread (gchar * cmd);
static gboolean parse_and_exec_stdin_builtin_command_in_gtk_thread(wordexp_t * parsed_msg, GString* readline_result_string, RFM_ChildAttribs* childAttribs);
static void toggle_exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread();
static void add_history_timestamp();
static void add_history_after_stdin_command_execution(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
void load_history_for_prompt();
void save_current_history();
static void stdin_command_help(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution);
static void cmd_glog(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution);
static void cmd_setenv(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution);
#ifdef reedline
static gboolean In_rfm_editor = FALSE;
static GPtrArray* rfm_editor_completion_based_on_search_result(char *text);
static void rfmEdit(gchar* filepath);
static gboolean parse_rfmEdit(GString *readline_result_string_after_file_name_substitution,RFM_ChildAttribs *childAttribs);
#endif
gchar** replace_shell_cmd_buffer(gchar **strv, gchar *buffer);
gchar* stdin_cmd_template_bash[]={"/bin/bash","-i","-c", "shell_cmd_buffer", NULL};
static gchar* stdin_cmd_template_nu[]={"nu","-c", "shell_cmd_buffer", NULL};
gchar* stdin_cmd_template_bash_newVT_nonHold[] = { "$RFM_TERM", "/bin/bash", "-i", "-c", "shell_cmd_buffer", NULL };
gchar* stdin_cmd_template_bash_newVT_nonHold_with_e[] = { "$RFM_TERM", "$RFM_TERM_E" ,"/bin/bash", "-i", "-c", "shell_cmd_buffer", NULL };
static gchar* stdin_cmd_template_nu_newVT_nonHold[] = { "$RFM_TERM", "nu", "-i", "-c", "shell_cmd_buffer", NULL };
static gchar* stdin_cmd_template_nu_newVT_nonHold_with_e[] = { "$RFM_TERM", "$RFM_TERM_E" ,"nu", "-i", "-c", "shell_cmd_buffer", NULL };
static NO_FREE gchar** stdin_command_bash(CALLER_OWN gchar* user_input_cmd, gboolean inNewVT);
static NO_FREE gchar** stdin_command_nu(CALLER_OWN gchar* user_input_cmd, gboolean inNewVT);

static void null_log_handler(const gchar *log_domain,GLogLevelFlags log_level,const gchar *message,gpointer user_data){
}

/******Terminal Emulator related definitions end*****/
/****************************************************/
/******Process spawn related definitions*************/
static GList *rfm_childList = NULL;
static int rfm_childListLength = 0;
static void free_child_attribs(FREE_FULL RFM_ChildAttribs *child_attribs);
static gchar *choose_working_directory_for_g_spawn(RFM_ChildAttribs *childAttribs);
static void g_spawn_wrapper_for_selected_fileList_(FREE_FULL RFM_ChildAttribs *childAttribs);
/* instantiate childAttribs and call g_spawn_wrapper_ */
/* caller should g_list_free(file_list), but usually not g_list_free_full, since the file char* is usually owned by rfm_fileattributes */
/* callbackfuncUserData can be pointer of any type, but callbackfunc should take RFM_ChildAttribs* as input parameter, and retrieve callbackfuncUserData with (RFM_ChildAttribs *)->customCallbackUserData; */
gboolean g_spawn_wrapper(CALLER_OWN const char **action, CALLER_OWN GList *file_list, const int run_opts, FREE const gchar** env, const char *dest_path, const gboolean async,const void(*callbackfunc)(gpointer),CALLER_OWN const gpointer callbackfuncUserData,const gboolean output_read_by_program);
gboolean g_spawn_wrapper_wordexp(CALLER_OWN const char *cmd, CALLER_OWN GList *file_list, const int run_opts, FREE const gchar** env, const char *dest_path, const gboolean async,const void(*callbackfunc)(gpointer),CALLER_OWN const gpointer callbackfuncUserData,const gboolean output_read_by_program);
/* call build_cmd_vector to create the argv parameter for g_spawn_* */
/* call different g_spawn_* functions based on child_attribs->spawn_async and child_attribs->runOpts */
gboolean g_spawn_wrapper_(CALLER_OWN GList *file_list, CALLER_OWN const char *dest_path, FREE_FULL RFM_ChildAttribs * childAttribs);
/* create argv parameter for g_spawn functions  */
/* The char* in file_list will be used (WITHOUT strdup) in returned gchar** v and owned by rfm_FileAttributelist before */
static MEM_ALLOC gchar **build_cmd_vector(CALLER_OWN const char **cmd, CALLER_OWN GList *file_list, CALLER_OWN const char *dest_path);
//caller will free v, child_attribs will free in ExecCallback_freeChildAttribs function if return TRUE, otherwize caller free child_attribs
static gboolean g_spawn_async_with_pipes_wrapper(CALLER_OWN gchar **v, RFM_ChildAttribs *child_attribs);
static gboolean g_spawn_async_with_pipes_wrapper_child_supervisor(gpointer user_data);
static void set_finished_status_for_child_supervisor(const GPid pid, const gint status, CALLER_OWN RFM_ChildAttribs *child_attribs);
static gboolean ExecCallback_freeChildAttribs(FREE_FULL RFM_ChildAttribs * child_attribs);
static void check_wait_status_and_set_exit_code(CALLER_OWN RFM_ChildAttribs *child_attribs);
static void show_child_output(CALLER_OWN RFM_ChildAttribs *child_attribs);
static int read_char_pipe(gint fd, ssize_t block_size, char **buffer);
static void GSpawnChildSetupFunc_setenv(gpointer user_data);
static void set_env_to_pass_into_child_process_for_view_columns_when_ItemSelected_equals_one(const GtkTreeIter *iter, gchar*** env_for_g_spawn);
/******Process spawn related definitions end*********/
/****************************************************/
/******Thumbnail related definitions*****************/
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
   GDesktopAppInfo *appInfoFromDesktopFile;
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
   GdkPixbuf *file, *dir;
   GdkPixbuf *symlinkDir;
   GdkPixbuf *symlinkFile;
   GdkPixbuf *unmounted;
   GdkPixbuf *mounted;
   GdkPixbuf *symlink;
   GdkPixbuf *broken;
   GdkPixbuf *dimensionDir;
   GdkPixbuf *symlinkDimensionDir; //指向dimensionDir的符号链接
} RFM_defaultPixbufs;

static RFM_defaultPixbufs *defaultPixbufs=NULL;
static GtkIconTheme *icon_theme;
static gchar *rfm_thumbDir;         /* Users thumbnail directory */
static gint rfm_do_thumbs;          /* Show thumbnail images of files: 0: disabled; 1: enabled; 2: disabled for current dir */
static GList *rfm_thumbQueue=NULL;
static guint rfm_thumbLoadScheduler = 0; // this used to be a gsourceid, but now, only a normal flag
static GThread *mkThumb_thread=NULL;
static gboolean mkThumb_stop=FALSE;
static GtkTreeIter thumbnail_load_iter;
static GHashTable *thumb_hash=NULL; /* Thumbnails in the current view */
static char thumbnailsize_str[8];
#ifdef RFM_CACHE_THUMBNAIL_IN_MEM
static GHashTable *pixbuf_hash = NULL;
#endif
static void load_thumbnail_or_enqueue_thumbQueue_for_store_row(GtkTreeIter *iter);
static RFM_ThumbQueueData *get_thumbData(GtkTreeIter *iter);
static gint find_thumbnailer(gchar *mime_root, gchar *mime_sub_type, gchar *filepath);
static int load_thumbnail(RFM_ThumbQueueData *thumbdata, gboolean show_Thumbnail_Itself_InsteadOf_As_Thumbnail_For_Original_Picture, gboolean check_tEXt);
static int load_thumbnail_as_asyn_callback(RFM_ChildAttribs *childAttribs);
static int load_thumbnail_when_idle_in_gtk_main_loop(RFM_ThumbQueueData *thumbdata);
static void rfm_saveThumbnail(GdkPixbuf *thumb, RFM_ThumbQueueData *thumbData);
static gboolean mkThumb();
static void mkThumbLoop();
static void free_thumbQueueData(RFM_ThumbQueueData *thumbData);
static RFM_defaultPixbufs *load_default_pixbufs(void);
static void free_default_pixbufs(RFM_defaultPixbufs *defaultPixbufs);
static void refreshThumbnail();
static void cmdThumbnailsize(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution);
/******Thumbnail related definitions end*************/
/****************************************************/
/******Dimemsion related definitions*****************/
typedef struct {
  gchar *ID;
  gchar *name;
  gchar *RemoteAddress;
  gchar *localAddress;
  gboolean use_getExtColumnValueFromHashTable;
} DimensionProperties;
static enum RFM_treeviewCol FirstDimensionCol = NUM_COLS;
static GHashTable* DimensionPropertiesHashTable_name_asKey;// Key:ExtColumn Name. Value:struct DimensionProperties
static GHashTable* DimensionPropertiesHashTable_localAddress_asKey;// Key:ExtColumn localAddress. Value:struct DimensionProperties
static void refresh_DimensionPropertiesHashTable();
static gboolean SetViewColumnForDimension(DimensionProperties* dimProps);
static void load_DimensionPropertiesHashTable_Callback(RFM_ChildAttribs* child_attribs);
static void load_ExtColumnHashTable_for_one_Dimension_for_GtkTreeIter(RFM_ChildAttribs* child_attribs);
static void load_ExtColumnHashTables_for_all_Dimensions_for_GtkTreeIter(GtkTreeIter* iter_for_file_in_store);
static void load_AllDimensionValues_for_DimensionColumns();
static void load_AllDimensionValues_for_one_DimensionColumn(RFM_ChildAttribs* childAttribs);
static gchar* getDimensionValueFromHashTable(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex);
static void free_DimensionProperties(DimensionProperties *dimProp);
static void call_g_dir_read_name_recursivly_to_get_dir_names_as_dimension_members(GSList **list, const char *base, const char *path);
static char **get_rfm_dimension_members_for_one_dimension(const char *dimension_localAddress, int start, int end);
static void cmd_set_dimension_value_for_selected_files(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution);
static void set_dimension_value_by_creating_soft_links_to_selected_files(gchar* dimNameAndValue[2]);
#ifdef reedline
static GPtrArray *rfm_completion_based_on_dimensions(char *text);
#endif
/******Dimension related definitions end******/
/****************************************************/
/******GtkListStore and refresh definitions**********/

//TODO: keep GtkTreeIter somewhere such as in fileAttribute, so that we can try load gitmsg and extcolumns with spawn async instead of sync. However, that can be complicated, what if we have spawned so many processes and user clicked refresh? we have to build Stop mechanism into it. Simple strategy is not to load this slow columns unless user configures to show them.
typedef struct {
  GtkTreeIter * iter;
  gint store_column;
  gboolean iconview_markup;
  gboolean iconview_tooltip;
  DimensionProperties *dimProperties;
} RFM_store_cell;

static GtkListStore *store=NULL;
static GtkTreeModel *treemodel=NULL;
// use clear_store() to free
// in searchresultview, this list only contains files on current view page.
// The SearchResultViewFileNameList contains files for the whole result.
static GList *rfm_fileAttributeList=NULL;

//hash table to store string shown in search result, with fileAttributeid as key
static GHashTable* ExtColumnHashTable[NUM_Ext_Columns + 1];
// For ExtColumn value from pipeline stdin, the total result batch is kept in
// hashtable and value not refreshed during refresh_store or turn_page. 
// For ExtColumn value from file parser, the hashtable keep only the values for the current page, and it will be refreshed during each refresh_store or turn_page. This is for performance.
static gboolean ExtColumnHashTable_keep_during_refresh[NUM_Ext_Columns + 1];
static gboolean SearchResultContainNonFilenameColumn = FALSE;
// if true, means that rfm read file names in following way:
//      ls|xargs realpath|rfm
// or
//      locate blablablaa |rfm
// , instead of from a directory
static unsigned int SearchResultViewInsteadOfDirectoryView=0;
static gboolean InSearchResultView(){ return SearchResultViewInsteadOfDirectoryView; }//c 语言有没有啥办法把变量当作函数调用或把函数当变量引用?这样我就用不着为了在ShowCondition里应用变量定义一个函数了
static gboolean InDirectoryView(){return SearchResultViewInsteadOfDirectoryView^1;}
static gboolean In_refresh_store=FALSE;
static gboolean insert_fileAttributes_into_store_one_by_one=FALSE;
static guint rfm_readDirSheduler=0;
static int rfm_inotify_fd;
static int rfm_curPath_wd = -1;    /* Current path (rfm_curPath) watch */
static gboolean pauseInotifyHandler = FALSE;
static gchar *rfm_prePath=NULL;  /* keep previous rfm_curPath, so that it will be autoselected in view. But manual selection change such as user pressing ESC in view will set it to null. Rodney invented this to autoselect child directory after going to parent directory*/
static GList *rfm_path_history_current = NULL;
static int read_one_file_couter = 0;//this is useless except for logging information
//TODO: i added the following two schedulers so that i can put off the loading of these slow columns after file list appears in the view first. But have not implemented them yet. Anyway, if user choose to show this slow columns, they have to wait to the end, show files slowly one by one may be better.
static guint rfm_extColumnScheduler = 0;
static RFM_FileAttributes *malloc_fileAttributes(void);
static void free_fileAttributes(RFM_FileAttributes *fileAttributes);
char* quote_path(char** path);
char * get_drwxrwxrwx(mode_t st_mode);
static RFM_FileAttributes *get_fileAttributes_for_a_file(const gchar *name, guint64 mtimeThreshold, GHashTable *mount_hash);
static void refresh_store();
static void refresh_store_on_searchresult();
static void clear_store(void);
static void rfm_stop_all();
static gboolean fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store(RFM_ChildAttribs* childAttribs);
static gboolean read_one_DirItem_into_fileAttributeList_and_insert_into_store_if_onebyone(GDir *dir);
static void toggle_insert_fileAttributes_into_store_one_by_one();
static void Iterate_through_fileAttribute_list_to_insert_into_store();
static void Insert_fileAttributes_into_store_and_set_GtkTreeIter(RFM_FileAttributes *fileAttributes,GtkTreeIter *iter);
static void DirectoryView_onebyon_On_Insert_fileAttributes_into_store_with_thumbnail_and_more(RFM_FileAttributes* fileAttributes);
//called in dir view when onebyone off
static gboolean iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_one_by_one_when_idle(GtkTreeIter *iter);
//called in search result view
static void SearchResultView_iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration(void);
static GHashTable *get_mount_points(void);
static gboolean mounts_handler(GUnixMountMonitor *monitor);
static gboolean init_inotify();
static gboolean inotify_handler(gint fd, GIOCondition condition);
static void inotify_insert_item(gchar *name, gboolean is_dir);
static gboolean delayed_refreshAll();
static void Update_Store_ExtColumns(RFM_ChildAttribs *childAttribs);
static gchar* getExtColumnValueFromHashTable(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex);
static NO_FREE RFM_FileAttributes *get_fileAttributes_byId(guint fileAttributeId);
static void set_rfm_curPath(CALLER_OWN gchar *path);
static void set_rfm_curPath_internal(CALLER_OWN const gchar* path);
static void cmd_cd(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution, RFM_ChildAttribs* childAttribs);
#ifdef GitIntegration
// value " M " for modified
// value "M " for staged
// value "MM" for both modified and staged
// value "??" for untracked
// the same as git status --porcelain
static GHashTable *gitTrackedFiles;
static GHashTable *gitDeletedDirs;
static gchar * git_root = NULL;//git_root non-NULL means current path is in git repository
static gchar * curPathRelativeToGitRoot = NULL;
static gchar * git_current_branch = NULL;
static guint rfm_gitCommitMsgScheduler = 0;
static GtkTreeIter gitMsg_load_iter;
static void get_git_root_with_rev_parse(gpointer *child_attribs);
static gboolean show_gitColumns(RFM_FileAttributes * fileAttributes);
static gboolean show_gitMenus(RFM_FileAttributes * fileAttributes);
static void readLatestGitLogForFileAndUpdateStore(RFM_ChildAttribs * childAttribs);
static void GitStatusForDirectory(gchar * filename, const char* status);
static void insert_deleted_for_git(gchar *fullpath, gchar *filename, gchar *status, gchar *mime_root, gchar *mime_sub);
static void read_git_ls_files_cmd_output_into_hashtable(gpointer *child_attribs);
static void read_git_status_porcelain_cmd_output_into_hashtable(gpointer *child_attribs);
static void load_gitTrackedFiles_HashTable();
static void load_latest_git_log_for_store_row(GtkTreeIter *iter);
//called in dir view when onebyone off
static gboolean DirectoryView_onebyone_Off_iterate_through_store_to_load_latest_git_log_one_by_one_when_idle(GtkTreeIter *iter);
#endif
static void set_terminal_window_title(CALLER_OWN const char* title);
static void set_Titles(CALLER_OWN const gchar * title);
static void cmdToggleInotifyHandler(wordexp_t *parsed_msg,GString *readline_result_string_after_file_name_substitution);
/******GtkListStore and refresh definitions end******/
/****************************************************/
/******Search Result related definitions*************/
static gchar *rfm_SearchResultPath=NULL; /*keep the rfm_curPath value when SearchResult was created */
static GList *SearchResultFileNameList = NULL;
// in search result view, same file can appear more than once(for example, in
// grep result), which means there can be duplicate files in fileAttribute list,
// so we use this id as unique key field for fileAttribute list
// In searchresultview, this value equals currentFileNum
static guint fileAttributeID=0;
static gint SearchResultFileNameListLength=0;
//the number shown in upper left button in search result view.
static gint currentFileNum=0;
static GList *CurrentPage_SearchResultView=NULL;
static gint PageSize_SearchResultView=100;
static RFM_SearchResultType* SearchResultTypePointer = NULL; // we need to pass this status from exec_stdin_command to readlineInSeperatedThread, however, we can only pass one parameter in g_thread_new, so, i use a global variable here.
static RFM_SearchResultType* SearchResultTypePointerForCurrentExistingSearchResult = NULL; //The value above will be override by next command, which can be a non-search command, we need this variable to preserve the current displaying searchresulttype
//如要分配新的自定义列,此变量记录分配的列名,按C1,C2..顺序,分配后,用键值替换原始的Cx列名
static int currentSearchResultTypeStartingColumnTitleIndex;
static int currentPageStartingColumnTitleIndex;
static int FileNameIndexInColumnTitles=FALSE;
static gchar** search_result_type_withtitle_column_title_array = NULL;
static gchar* stdout_read_by_search_result=NULL;
static gboolean showSearchResultExtColumnsBasedOnHashTableValues_has_already_been_called_for_searchresult = FALSE;//方便用showcolumn命令手工调整SearchResult的列布局
static void update_SearchResultFileNameList_and_refresh_store(RFM_ChildAttribs *childAttribs);
static void showSearchResultExtColumnsBasedOnHashTableValues();
static int ProcessOnelineForSearchResult(CALLER_OWN gchar *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);
static int ProcessOnelineForSearchResult_internel(CALLER_OWN char* oneline, gboolean new_search, int first_line_column_title);
static int ProcessOnelineForSearchResult_with_title(CALLER_OWN char* oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);
static void CallDefaultSearchResultFunctionForNewSearch(CALLER_OWN char *one_line_in_search_result, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);
int ProcessKeyValuePairInFilesFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);
int ProcessKeyValuePairInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, char **cmdtemplate, RFM_FileAttributes* fileAttributes,RFM_ChildAttribs *childAttribs);
int ProcessJsonInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, char** cmdtemplate, RFM_FileAttributes* fileAttributes,RFM_ChildAttribs* child_attribs);
static int ProcessCmdOutputFromSearchResultWithFunction(CALLER_OWN char *oneline, gboolean new_search, char** cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* child_attribs, int (*func)(RFM_ChildAttribs*));
static int ProcessKeyValuePairInData(GKeyFile* keyfile, char *groupname);
static void ProcessKeyValuePair(CALLER_OWN gchar *key, OWNER_CHANGE gchar* currentExtColumnValue, CALLER_OWN char* currentColumnOriginalTitle);
static int ProcessGKeyfileStrWrapper(RFM_ChildAttribs *child_attribs);
static int ProcessGKeyfileStr(CALLER_OWN gchar *cmdOutput);
static int ProcessJsonStrWrapper(RFM_ChildAttribs *child_attribs);
static int ProcessJsonStr(CALLER_OWN gchar* json_data);
static int ProcessJsonFileFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs);
static int CallMatchingProcessorForSearchResultLine(char *oneline, gboolean new_search, char *cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* childAttribs);
static void cmdSearchResultColumnSeperator(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution);
//matched search result type suffix will be removed from readlineResults
static RFM_SearchResultType* MatchSearchResultType(gchar *readlineResult, RFM_ChildAttribs *childattribs);
static void set_pagesize_for_searchresultview(const char *pagesize);
static void cmdPagesize(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution);
void addToSearchResult(FREE char *fileFullpath);
static void cmd_addToSearchResult(CALLER_OWN wordexp_t *parsed_msg,CALLER_OWN  GString *readline_result_string_after_file_name_substitution);
static void writeSearchResultIntoFD_in_gtk_thread(gint fd, gboolean exportFilePathOnly);
static void exec_stdin_command_with_SearchResultAsInput(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, gboolean from_stdin_readline, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *childAttribs);
/******Search Result related definitions end*********/
/****************************************************/
/******TreeView column related definitions***********/
static gchar* treeviewcolumn_init_order_sequence = NULL;
static gboolean do_not_show_VALUE_MAY_NOT_LOADED_message_because_we_will_add_GtkTreeViewColumn_later = FALSE;
void move_array_item_a_after_b(void * array, int index_b, int index_a, uint32_t array_item_size, uint32_t array_length);
static int get_treeviewColumnsIndexByEnum(enum RFM_treeviewCol col);
static RFM_treeviewColumn* get_treeviewcolumnByGtkTreeviewcolumn(GtkTreeViewColumn *gtkCol);
static RFM_treeviewColumn* get_treeviewColumnByEnum(enum RFM_treeviewCol col);
static RFM_treeviewColumn* get_treeviewColumnByTitle(char* title);
static enum RFM_treeviewCol get_available_ExtColumn(enum RFM_treeviewCol col);
static enum RFM_treeviewCol get_available_ExtColumn_from_end(enum RFM_treeviewCol col);
static MEM_ALLOC gchar* get_current_column_layout(gboolean title);
static MEM_ALLOC gchar* get_showcolumn_cmd_from_currently_displaying_columns();
static void show_hide_treeview_columns_in_order(CALLER_OWN gchar *order_sequence);
static void show_hide_treeview_columns_enum(int count, ...);
static void hide_column_menuitem_activated(GtkMenuItem *menuitem, RFM_treeviewColumn *col);
static void createCandidateColumnsMenu(GtkMenuItem* parent, RFM_treeviewColumn *col);
static void insert_column_menuitem_callback(GtkMenuItem *menuitem,char *order_seq);
static void cmd_showcolumn(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution);
static void init_ext_columns_in_treeviewColumns();
/******TreeView column related definitions end*******/
/****************************************************/
/******View Selection related definitions************/
// keep previous selection when go back from cd directory to search result.
// two elements, one for search result view, the other for directory view
// filepath string in this list is created with strdup.
static GList * filepath_lists_for_selection_on_view[2] = {NULL,NULL};
static GList * filepath_lists_for_selection_on_view_clone;
static GList * stdin_cmd_selection_list=NULL; //selected files used in stdin cmd expansion(or we call it substitution) which replace ending space and %s with selected file names
static GtkTreeIter iter_selectionChanged;
static gchar* selected_file_mode_str = NULL;
static GList* get_view_selection_list(GtkWidget * view, gboolean treeview, GtkTreeModel ** model);
static void selectionChanged(GtkWidget *view, gpointer user_data);
static void set_view_selection(GtkWidget* view, gboolean treeview, GtkTreePath* treePath);
static void set_view_selection_list(GtkWidget *view, gboolean treeview,GList *selectionList);
static gboolean path_is_selected(GtkWidget *widget, gboolean treeview, GtkTreePath *path);
/******View Selection related definitions end********/
/****************************************************/
/******View sort related definitions*****************/
static GtkSortType current_sorttype=GTK_SORT_ASCENDING;
static gint current_sort_column_id = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
static void sort_on_column_header(GtkMenuItem *menuitem, RFM_treeviewColumn *rfmCol);
static void sort_on_column(wordexp_t * parsed_msg);
static gboolean view_column_header_clicked(GtkButton* column_header, GdkEvent *event, GtkWidget *gtkCol);
static gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
static void cmdSort(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution);
/******View sort related definitions end*************/
/****************************************************/
/******FileChooser related definitions***************/
static gboolean StartedAs_rfmFileChooser = FALSE;
static gchar *rfmFileChooserReturnSelectionIntoFilename = NULL;
#ifdef RFM_FILE_CHOOSER
static void (*FileChooserClientCallback)(char **) = NULL;
GList *str_array_ToGList(char *a[]);
char **GList_to_str_array(GList *l, int count);
static void rfmFileChooserResultReader(RFM_ChildAttribs *child_attribs);
GList* rfmFileChooser_glist(enum rfmTerminal startWithVirtualTerminal, char* search_cmd, gboolean async, GList** fileChooserSelectionListAddress, void (*fileChooserClientCallback)(char **));
char** rfmFileChooser(enum rfmTerminal startWithVirtualTerminal, char* search_cmd, gboolean async, char *fileSelectionStringArray[], void (*fileChooserClientCallback)(char**));
#include "rfmFileChooser.h"
static gchar** rfmFileChooser_CMD(enum rfmTerminal startWithVT, gchar* search_cmd, gchar** defaultFileSelection, gchar* rfmFileChooserReturnSelectionIntoFilename);
#endif
/******FileChooser related definitions end***********/
/********************************************************************/
/**************gtk UI, filemenu, toolbar*****************************/
typedef struct {
    gchar *msg;
    gchar *title;
    gint type;
} MsgBoxData;

static gboolean no_window=FALSE;    // for the --no-gui command line parameter
static GtkWidget *window=NULL;      /* Main window */
static GtkWidget *rfm_main_box;
static GtkWidget *scroll_window = NULL;
static GtkWidget * PathAndRepositoryNameDisplay;
static GtkWidget *tool_bar = NULL;
static GtkWidget **tool_bar_buttons = NULL;
static GtkWidget *menu = NULL;
static GtkWidget **menuItem = NULL;
static GtkWidget *columnHeaderMenu = NULL;
static GtkWidget *HideColumnMenuItem = NULL;
static GtkWidget *InsertBeforeMenuItem = NULL;
static GtkWidget *InsertAfterMenuItem = NULL;
static void up_clicked();
static void prev_path_clicked() {set_rfm_curPath("<");}
static void next_path_clicked() {set_rfm_curPath(">");}
static void home_clicked(gpointer user_data);
static void PreviousPage();
static void NextPage();
static void FirstPage();
static void TurnPage(gboolean next);
static void info_clicked(gpointer user_data);
static void copy_curPath_to_clipboard(GtkWidget *menuitem, gpointer user_data);
static void refresh_view(gboolean switch_view);
static void switch_iconview_treeview();
gboolean InIconView(){ return !treeview; }
static gboolean InTreeView(){ return treeview; }
static void msgbox_response_handler(GtkDialog *dialog, gint response_id, gpointer user_data);
static gboolean show_msgbox_in_main_thread(MsgBoxData *msg_data);
static void show_msgbox(gchar *msg, gchar *title, gint type);
static void Switch_SearchResultView_DirectoryView();
/* callback function for toolbar buttons. Its possible to make function such as home_clicked as callback directly, instead of use toolbar_button_exec as a wrapper. However,i would add GtkToolItems as first parameter for so many different callback functions then. */
/* since g_spawn_wrapper will free child_attribs, and we don't want the childAttribs object associated with UI interface item to be freed, we duplicate childAttribs here. */
/* tool_buttons actions are basically defined at current directory level, or current view level, for example: to go to parent directory, or to switch between icon or tree view */
static void toolbar_button_exec(GtkToolItem *item, RFM_ToolButton *button_definition);
/* callback function for contextual file menu, which appear after mouse right click on selected file, or the after the menu key on keyboard pressed */
/* since g_spawn_wrapper will free child_attribs, and we don't want the childAttribs object associated with UI interface item to be freed, we duplicate childAttribs here. */
static void file_menu_exec(GtkMenuItem *menuitem, RFM_MenuItem *file_menu_item_definition);
/* Every action defined in config.h is added to the menu here; each menu item is associated with
 * the corresponding index in the run_actions_array.
 * Items are shown or hidden in popup_file_menu() depending on the selected files mime types.
 */
void add_file_menuitems(GtkWidget ***menuitems, const RFM_MenuItem menuitem_definitions[], const size_t size);
/* show/hide a filemenu based on mime type of the files selected and other critierias
 * If all files are the same type selection_mimeRoot and selection_mimeSub are set
 * If all files are the same mime root but different sub, then selection_mimeSub is set to NULL
 * If all the files are completely different types, both root and sub types are set to NULL
 */
static gboolean popup_file_menu(GdkEvent *event);
static void show_hide_file_menu_item(GtkWidget ***menuitems, RFM_MenuItem menuitem_definitions[],size_t menuitem_definitions_size, gboolean match_mimeRoot, gboolean match_mimeSub, RFM_FileAttributes* first_selection_fileAttributes, GList *selectionList);

static gboolean mountpoint_mounted(RFM_FileAttributes *fileAttributes);
static gboolean mountpoint_unmounted(RFM_FileAttributes *fileAttributes);
static void cmd_handle_escape_key();
static void g_idle_add_once_handle_escape_key(){ g_idle_add_once(cmd_handle_escape_key,NULL);}
static gboolean window_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
#include "config.h"

static stdin_cmd_interpretor * current_stdin_cmd_interpretor_pointer = &(stdin_cmd_interpretors[0]);
static stdin_cmd_interpretor * effective_cmd_interpretor_pointer = &(stdin_cmd_interpretors[0]);  //keep the init value the same as that of current_stdin_cmd_interpretor so that these two would be same for the first run of readlineInseperatethread

static GtkWidget *add_view();
void add_toolbar_buttons(GtkWidget ***buttons, const RFM_ToolButton button_definitions[], const size_t size);
static void add_toolbar(GtkWidget *rfm_main_box, RFM_defaultPixbufs *defaultPixbufs);
static void refresh_toolbar();
static gboolean view_key_press(GtkWidget *widget, GdkEvent *event);
static gboolean view_button_press(GtkWidget *widget, GdkEvent *event);
static gboolean on_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
static void item_activated(GtkWidget *icon_view, GtkTreePath *tree_path, gpointer user_data);
static void row_activated(GtkTreeView *tree_view, GtkTreePath *tree_path,GtkTreeViewColumn *col, gpointer user_data);

// 对于在下面几行代码运行git blame 显示commit msg
// 里面提到的是否准备支持多搜索结果问题,我想我是有答案的:不准备支持
// rfm在平铺窗口下运行比较好,用多个rfm实例来支持多搜索结果优于在一个实例里面支持多结果
static RFM_treeviewColumn *ViewColumnsLayouts[2];
static size_t TREEVIEW_COLUMNS_array_size = G_N_ELEMENTS(treeviewColumns);
#define DirectoryViewColumnsLayout ViewColumnsLayouts[0]
#define SearchResultViewColumnsLayout ViewColumnsLayouts[1]
#define TREEVIEW_COLUMNS ViewColumnsLayouts[SearchResultViewInsteadOfDirectoryView]

#define SHOW_TOOLBAR_BUTTON(x,i) (((SearchResultViewInsteadOfDirectoryView && x[i].showInSearchResultView) || (!SearchResultViewInsteadOfDirectoryView && x[i].showInDirectoryView)) && (x[i].showCondition == NULL || x[i].showCondition()))
/**************gtk UI, filemenu, toolbar end**************************/
/************** Addon begin ******************************************/

#ifdef RFM_AddOn
typedef struct {
  RFM_ADDON_RESOURCE_TYPE * resource_managed_by_addon;
  enum RFM_ADDON_TYPE addon_type;
  void *addon_so_handle;
  void (*addon_enter_func)(gchar*);
  void (*addon_leave_func)(void);
  gchar *addon_c_fullpath;
  gchar *addon_so_fullpath;
  gchar *addon_hash;
  gchar *addon_base_filename;
} RFM_ADDON;

static gboolean disable_addon = FALSE;
void remove_toolbar_buttons(FREE GtkWidget ***buttons, const size_t size);
void remove_file_menuitems(FREE GtkWidget ***menuitems, const size_t size);

static void free_addon(RFM_ADDON *addon);
static GHashTable* addons; // RFM_ADDON as value
static GHashTable* all_addon_definitions_by_dir; // addon definition file directory as key, nested ghashtable as value; in nested ghashtable, addon definition file fullpath as key, addon type definition str such as "const enum RFM_ADDON_TYPE addon_type = DirectoryTree;" as value
static GHashTable* addons_hash; //use addon hash as key, addon pointer as value to detect duplicate load
static GHashTable* addons_filename; //use filename as key, to detect duplicate load, such as different versions of the same addon
static void locate_all_addons_callback(RFM_ChildAttribs* child_attribs);
static void load_addon(CALLER_OWN char* addon_fullpath);
static void load_addon_dir(char* addon_dir);
static void append_addon_treeviewColumns(RFM_ADDON *addon,gboolean searchresultview, gboolean dirview);
static void remove_addon_treeviewColumns(RFM_ADDON *addon);
static void unload_local_extension(RFM_ADDON *addon);
static gboolean compile_local_extension(RFM_ADDON *addon);
static gchar* calculate_file_hash(const gchar *filepath);
static void find_and_unload_addons_before_leaving_current_directory(CALLER_OWN const gchar* path);
#endif

/**************Addon end********************************/

char* quote_path(char** path) {
    if (path == NULL || *path == NULL) return NULL;

    // 检查路径是否包含空格
    int need_quotes = (strchr(*path, ' ') != NULL);

    if (need_quotes) {
        // 计算新字符串长度 = 原长度 + 2个引号 + 结束符
        size_t len = strlen(*path);
        char* new_path = malloc(len + 3);
        if (!new_path) return NULL;
        sprintf(new_path, "\"%s\"", *path); // 添加双引号
	free(*path);
	*path = new_path;
        return new_path;
    } else {
        // 无空格时返回原字符串的副本
        return *path;
    }
}

char * get_drwxrwxrwx(mode_t st_mode){
    char * ret=calloc(11,sizeof(char));
    //文件类型
    if(S_ISDIR(st_mode))//目录文件
      ret[0]='d'; // ls -l /sys/dev/char 会显示类型为l, 为啥这里 S_ISDIR 会返回true? 还是说我应该把后面的S_ISLNK 判断放在前面? anyway,这里没研究清楚前暂时不动, 放在获取文件信息里判断 is_symlink 后再修正这个值. strmode是我当时想显示mode字符串时百度来的, fileAttributes->is_symlink=g_file_info_get_is_symlink(info) 这一句是从Rodney继承的
    else if(S_ISREG(st_mode))//普通文件
      ret[0]='-';
    else if(S_ISCHR(st_mode))//字符文件
      ret[0]='c';
    else if(S_ISBLK(st_mode))//块文件
      ret[0]='b';
    else if(S_ISFIFO(st_mode))//管道文件
      ret[0]='p';
    else if(S_ISLNK(st_mode))//链接文件
      ret[0]='l';
    else if(S_ISSOCK(st_mode))//套接字文件
      ret[0]='s';
    //code copied from https://blog.csdn.net/xieeryihe/article/details/121715202
    else ret[0]='?';

    //文件所有者权限
    if(st_mode&S_IRUSR) ret[1]='r';
    else ret[1]='-';
    if(st_mode&S_IWUSR) ret[2]='w';
    else ret[2]='-';
    //https://blog.csdn.net/u013769350/article/details/88903671
    if(st_mode&S_IXUSR){
      if (st_mode & S_ISUID ) ret[3]='s';
      else ret[3]='x';
    }else{
      if (st_mode & S_ISUID ) ret[3]='S';
      else ret[3]='-';
    }
    
    //用户组权限
    if(st_mode&S_IRGRP) ret[4]='r';
    else ret[4]='-';
    if(st_mode&S_IWGRP) ret[5]='w';
    else ret[5]='-';
    //https://blog.csdn.net/u013769350/article/details/88909329
    if(st_mode&S_IXGRP){
      if (st_mode & S_ISGID ) ret[6]='s';
      else ret[6]='x';
    }else{
      if (st_mode & S_ISGID ) ret[6]='S';
      else ret[6]='-';
    }

    //其他用户权限
    if(st_mode&S_IROTH) ret[7]='r';
    else ret[7]='-';
    if(st_mode&S_IWOTH) ret[8]='w';
    else ret[8]='-';
    //https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html
    //https://blog.csdn.net/u013769350/article/details/88915759
    if(st_mode&S_IXOTH){
      if (st_mode & S_ISVTX ) ret[9]='s';
      else ret[9]='x';
    }else{
      if (st_mode & S_ISVTX ) ret[9]='S';
      else ret[9]='-';
    }

    ret[10]=0;
    return ret;
}

static void free_thumbQueueData(RFM_ThumbQueueData *thumbData)
{
   if (thumbData->g_source_id_for_load_thumbnail>0) g_source_remove(thumbData->g_source_id_for_load_thumbnail);
   if (thumbData->appInfoFromDesktopFile) g_free(thumbData->appInfoFromDesktopFile);
   g_free(thumbData->uri);
   g_free(thumbData->path);
   g_free(thumbData->md5);
   g_free(thumbData->thumb_name);
   free(thumbData);
}

static void free_child_attribs(FREE_FULL RFM_ChildAttribs *child_attribs)
{
   g_log(RFM_LOG_GSPAWN,G_LOG_LEVEL_DEBUG,"RFM_childAttribs %p free %s",child_attribs, child_attribs->no_free? "with no_free":"");
   if(child_attribs->no_free) return;
   if (child_attribs->RunCmd) g_strfreev(child_attribs->RunCmd);
   free(child_attribs->stdOut); child_attribs->stdOut = NULL;
   free(child_attribs->stdErr); child_attribs->stdErr = NULL;
   free(child_attribs->total_stdOut); //TODO: 这里向别的地方一样，应该设成NULL的。当时我添加这行的时候，想不起来其他地方为啥还要置NULL,今天从上面日志发现同一个child_attribs 地址，有反复free的现象（从日志上下文看，应该是读取git信息相关操作），但只是同一个地址，还不能确定是同一个，否则下面 g_free(child_attribs) 为啥没有double free 错？
   free(child_attribs->total_stdErr);
   g_free(child_attribs->name);   child_attribs->name = NULL;
   g_strfreev(child_attribs->env);child_attribs->env = NULL;
   g_list_free_full(child_attribs->fullpath_list_as_RunCmd_parameters, (GDestroyNotify)g_free);
   //how can we memcpy the content of customCallbackUserData and free it here?
   //g_free(child_attribs->customCallbackUserData);
   g_free(child_attribs->workingDir); child_attribs->workingDir = NULL;
   g_free(child_attribs->toolbar_button_or_filemenu_addon_dir); child_attribs->toolbar_button_or_filemenu_addon_dir = NULL;
   g_free(child_attribs->cmd_interpretor_addon_dir); child_attribs->cmd_interpretor_addon_dir = NULL;
   g_free(child_attribs->searchresulttype_addon_dir); child_attribs->searchresulttype_addon_dir = NULL;
   g_free(child_attribs->rfmEditFilePath); child_attribs->rfmEditFilePath = NULL;
   g_free(child_attribs);
   //child_attribs->readlineResultString 在 add_history_after_stdin_command_execution 函数内 free, 因为child_attribs 在g_spawn_wrapper执行玩命令后就要free,但命令执行玩还要 add_history 呢。
}

static void rfm_stop_all() {
   if (delayedRefresh_GSourceID > 0)
      g_source_remove(delayedRefresh_GSourceID);

   if (rfm_readDirSheduler>0)
      g_source_remove(rfm_readDirSheduler);

   if (rfm_extColumnScheduler>0) g_source_remove(rfm_extColumnScheduler);
#ifdef GitIntegration
   if (rfm_gitCommitMsgScheduler>0) g_source_remove(rfm_gitCommitMsgScheduler);
#endif

   delayedRefresh_GSourceID=0;
   rfm_readDirSheduler=0;
   rfm_thumbLoadScheduler=0;
   rfm_extColumnScheduler=0;
#ifdef GitIntegration
   rfm_gitCommitMsgScheduler=0;
#endif

   if (mkThumb_thread) {
     mkThumb_stop = TRUE;
     g_thread_join(mkThumb_thread);
     mkThumb_thread=NULL;
     mkThumb_stop = FALSE;
   }
   if (rfm_thumbQueue){
     rfm_thumbQueue = g_list_first(rfm_thumbQueue);
     g_list_free_full(rfm_thumbQueue, (GDestroyNotify)free_thumbQueueData);
     rfm_thumbQueue=NULL;
   }

   if (!no_window) gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
}


static gboolean ExecCallback_freeChildAttribs(FREE_FULL RFM_ChildAttribs * child_attribs){
   if(child_attribs->exitcode==0 && (child_attribs->customCallBackFunc)!=NULL){
       (child_attribs->customCallBackFunc)(child_attribs);
   }

   // Restart readline if this was a search command from readlineinput and it failed
   if ((child_attribs->exitcode == -1 && child_attribs->customCallBackFunc == g_spawn_async_callback_to_new_readline_thread)
       || (child_attribs->spawn_async && child_attribs->keep_total_buffer_in_show_child_output && child_attribs->customCallBackFunc)){ //关于这一行条件，参见parse_and_exec_command_in_gtk_thread 和 exec_stdin_command 里的注释
       readlineThread = g_thread_new("readline", readlineInSeperateThread, NULL); //SearchResultAsInput 时，命令执行失败，从这里新建readline.
   }

   free_child_attribs(child_attribs);
   return TRUE;
}

static void toggle_insert_fileAttributes_into_store_one_by_one(){
  insert_fileAttributes_into_store_one_by_one = !insert_fileAttributes_into_store_one_by_one;
  if (insert_fileAttributes_into_store_one_by_one) printf("onebyone on\n");
  else printf("onebyone off\n");
}

/* Supervise the children to prevent blocked pipes */
static gboolean g_spawn_async_with_pipes_wrapper_child_supervisor(gpointer user_data)
{
   RFM_ChildAttribs *child_attribs=(RFM_ChildAttribs*)user_data;

/* https://docs.gtk.org/glib/func.spawn_async_with_pipes_and_fds.html */
/* g_spawn_async_with_pipes_and_fds(${1:const gchar *working_directory}, ${2:const gchar *const *argv}, ${3:const gchar *const *envp}, ${4:GSpawnFlags flags}, ${5:GSpawnChildSetupFunc child_setup}, ${6:gpointer user_data}, ${7:gint stdin_fd}, ${8:gint stdout_fd}, ${9:gint stderr_fd}, ${10:const gint *source_fds}, ${11:const gint *target_fds}, ${12:gsize n_fds}, ${13:GPid *child_pid_out}, ${14:gint *stdin_pipe_out}, ${15:gint *stdout_pipe_out}, ${16:gint *stderr_pipe_out}, ${17:GError **error}) */

/* G_SPAWN_STDOUT_TO_DEV_NULL means that the child's standard output will be discarded (by default, it goes to the same location as the parent's standard output). G_SPAWN_CHILD_INHERITS_STDOUT explicitly imposes the default behavior. Both flags cannot be enabled at the same time and, in both cases, the stdout_pipe_out argument is ignored. */
   
/* If stdout_pipe_out is NULL, the child's standard output goes to the same location as the parent's standard output unless G_SPAWN_STDOUT_TO_DEV_NULL is set. */

/* https://docs.gtk.org/glib/func.spawn_async_with_pipes.html */
/* g_spawn_async_with_pipes(${1:const gchar *working_directory}, ${2:gchar **argv}, ${3:gchar **envp}, ${4:GSpawnFlags flags}, ${5:GSpawnChildSetupFunc child_setup}, ${6:gpointer user_data}, ${7:GPid *child_pid}, ${8:gint *standard_input}, ${9:gint *standard_output}, ${10:gint *standard_error}, ${11:GError **error}) */
/* Identical to g_spawn_async_with_pipes_and_fds() but with n_fds set to zero, so no FD assignments are used. */
   
   if (child_attribs->output_read_by_program || (((child_attribs->runOpts & G_SPAWN_CHILD_INHERITS_STDOUT)!=G_SPAWN_CHILD_INHERITS_STDOUT) && ((child_attribs->runOpts & G_SPAWN_STDOUT_TO_DEV_NULL)!=G_SPAWN_STDOUT_TO_DEV_NULL)))//rfm代码处理输出，或者不继承（父进程，即终端）输出
     if (read_char_pipe(child_attribs->stdOut_fd, PIPE_SZ, &child_attribs->stdOut)<0) g_log(RFM_LOG_GSPAWN_READ_PIPE, G_LOG_LEVEL_DEBUG,"read_char_pipe failed, child_attribs->stdOut_fd:%d", child_attribs->stdOut_fd);

   if ((((child_attribs->runOpts) & G_SPAWN_CHILD_INHERITS_STDERR)!=G_SPAWN_CHILD_INHERITS_STDERR) && (((child_attribs->runOpts) & G_SPAWN_STDERR_TO_DEV_NULL)!=G_SPAWN_STDERR_TO_DEV_NULL))
     if (read_char_pipe(child_attribs->stdErr_fd, PIPE_SZ, &child_attribs->stdErr)<0) g_log(RFM_LOG_GSPAWN_READ_PIPE, G_LOG_LEVEL_DEBUG,"read_char_pipe failed, child_attribs->stdErr_fd:%d", child_attribs->stdErr_fd);
   
   if (!child_attribs->output_read_by_program && ((child_attribs->stdOut && strlen(child_attribs->stdOut)>0) || (child_attribs->stdErr && strlen(child_attribs->stdErr)>0))) show_child_output(child_attribs);
   //TODO: devPicAndVideo submodule commit f746eaf096827adda06cb2a085787027f1dca027 的错误起源于上面一行代码和RFM_EXEC_FILECHOOSER 的引入。
   if (child_attribs->status==-1) return G_SOURCE_CONTINUE;

   if ((((child_attribs->runOpts) & G_SPAWN_CHILD_INHERITS_STDOUT)!=G_SPAWN_CHILD_INHERITS_STDOUT) && (((child_attribs->runOpts) & G_SPAWN_STDOUT_TO_DEV_NULL)!=G_SPAWN_STDOUT_TO_DEV_NULL)){
     g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "child_supervisor for PID %d, will close stdOut_fd:%d",child_attribs->pid,child_attribs->stdOut_fd);
     close(child_attribs->stdOut_fd);
   }
   if ((((child_attribs->runOpts) & G_SPAWN_CHILD_INHERITS_STDERR)!=G_SPAWN_CHILD_INHERITS_STDERR) && (((child_attribs->runOpts) & G_SPAWN_STDERR_TO_DEV_NULL!=G_SPAWN_STDERR_TO_DEV_NULL))){
     g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "child_supervisor for PID %d, will close stdErr_fd:%d",child_attribs->pid,child_attribs->stdErr_fd);
     close(child_attribs->stdErr_fd);
   }
   g_spawn_close_pid(child_attribs->pid);

   //之前, stdErr 都是child结束后g_warning 显示的, 但现在, 在!child_attribs->output_read_by_program 的情况下, stdErr 是在 show_child_output 里和output 类似处理的, 但output_read_by_program的情况,暂时还是保留之前处理方式吧, 有啥别的办法? 
   if (child_attribs->output_read_by_program && child_attribs->stdErr!=NULL && strlen(child_attribs->stdErr)>0) g_debug("g_spawn_async_with_wrapper_child_supervisor: %s",child_attribs->stdErr); // output_read_by_program 的情况下，如检查当前仓库是否是git仓库，如果不是，命令会返回错误，exitcode 128, 如果这种情况也显示warning,对用户干扰太大。//TODO:在 childattribs里加选项控制是waring,还是debug显示错误消息，奇怪的是今天测试才发现这个错误显示，之前一直没看见，为啥？这两天的代码改动不影响这一块啊
   
   rfm_childList=g_list_remove(rfm_childList, child_attribs);
   rfm_childListLength--;
   /* if (rfm_childList==NULL) */
   /*    gtk_widget_set_sensitive(GTK_WIDGET(info_button), FALSE); */

   /* GError *err=NULL; */
   /* char * msg=NULL; */

   /* /\* if (g_spawn_check_wait_status(child_attribs->status, &err) && child_attribs->runOpts==RFM_EXEC_MOUNT) *\/ */
   /* /\*    set_rfm_curPath(RFM_MOUNT_MEDIA_PATH); *\/ */
   /* //https://docs.gtk.org/glib/func.spawn_check_wait_status.html */
   /* if (!g_spawn_check_wait_status(child_attribs->status, &err)){; //TRUE if child exited successfully, FALSE otherwise (and error will be set). */
   /* //TODO: after i change runOpts to GSpawnFlags, RFM_EXEC_MOUNT won't work, i don't understand what it for, maybe later, we can use fields such as runCmd or some naming conventions in runaction or childAttribs to indicate this type. */
   /*   if (err!=NULL) { */
   /*     if (g_error_matches(err, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED)) { //If the process was terminated by some means other than an exit status (for example if it was killed by a signal), the domain will be G_SPAWN_ERROR and the code will be G_SPAWN_ERROR_FAILED. */
   /* 	 child_attribs->exitcode = -1; // to prevent customcallbackfunc from execution */
   /* 	 msg=g_strdup_printf("%s (pid: %i): stopped: %s\n", child_attribs->name, child_attribs->pid, err->message); */
   /*     }else { //The special semantics are that the actual exit code will be the code set in error, and the domain will be G_SPAWN_EXIT_ERROR. This allows you to differentiate between different exit codes. */
   /* 	 child_attribs->exitcode = err->code; */
   /* 	 msg=g_strdup_printf("%s (pid: %i): finished with exit code %i.\n%s\n", child_attribs->name, child_attribs->pid, child_attribs->exitcode, child_attribs->stdErr); */
   /*     } */
       
   /*     if (startWithVT()){ */
   /* 	 fprintf(stderr,"%s", msg); */
   /*     }else{ */
   /* 	 if (strlen(msg) > RFM_MX_MSGBOX_CHARS){ */
   /* 	   memcpy(msg + (RFM_MX_MSGBOX_CHARS - 35),"......message too long to show\0",31); */
   /* 	 } */
   /* 	 show_msgbox(msg, child_attribs->name, GTK_MESSAGE_ERROR); */
   /*     } */

   /*     g_free(msg); */
   /*     g_error_free(err); */
   /*   }else g_warning("g_spawn_check_wait_status return FALSE but GError NULL"); */
   /* } */
   check_wait_status_and_set_exit_code(child_attribs);
   if (child_attribs->g_idle_add_once_to_run_CallBackFunc) g_idle_add_once(ExecCallback_freeChildAttribs, child_attribs);
   else ExecCallback_freeChildAttribs(child_attribs);
   return G_SOURCE_REMOVE;
}

static void set_finished_status_for_child_supervisor(const GPid pid, const gint status, CALLER_OWN RFM_ChildAttribs *child_attribs)
{
   int oldstatus = child_attribs->status;
   child_attribs->status=status; /* show_child_output() is called from child_supervisor() in case there is any data left in the pipes */
   g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "child Pid %i status changed from %d to %d", child_attribs->pid, oldstatus, child_attribs->status);
}

//显示完 child_attribs->stdOut, stdErr后，会将其置为空字符串
static void show_child_output(CALLER_OWN RFM_ChildAttribs *child_attribs)
{
   gchar *msg=NULL;
   
   /* Show any output we have regardless of error status */
   if (child_attribs->stdOut!=NULL && child_attribs->stdOut[0]!=0) {
     if (startWithTerm()){
       printf("%s",child_attribs->stdOut);
       g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "output of strlen %ld from child PID %d shown", strlen(child_attribs->stdOut), child_attribs->pid);
       //TODO: if not startwithVT(), user won't see printf, but user also will not see all those g_warnings. maybe we can add some indicator on UI and guide user to some log file. When not startwithVT, redirect stdout and stderr to some log file
     }else if (strlen(child_attribs->stdOut) <= RFM_MX_MSGBOX_CHARS)
         show_msgbox(child_attribs->stdOut, child_attribs->name, GTK_MESSAGE_INFO);
     else{
         msg=g_strdup_printf("output of strlen %ld from child PID %d greater that RFM_MX_MSGBOX_CHARS %d, cannot show in msgbox", strlen(child_attribs->stdOut), child_attribs->pid,RFM_MX_MSGBOX_CHARS);
	 show_msgbox(msg, child_attribs->name, GTK_MESSAGE_WARNING);
	 g_free(msg);
     }
     if (child_attribs->keep_total_buffer_in_show_child_output){
       if (child_attribs->total_stdOut)
         child_attribs->total_stdOut=g_strconcat(child_attribs->total_stdOut,child_attribs->stdOut, NULL);
       else
	 child_attribs->total_stdOut=g_strdup(child_attribs->stdOut);
     }
     child_attribs->stdOut[0]=0;
   }else{ g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "empty child_attribs->stdOut in show_child_output"); }

   if (child_attribs->stdErr!=NULL && child_attribs->stdErr[0]!=0) {
     if (startWithTerm()){
       fprintf(stderr, "%s",child_attribs->stdErr);
       g_log(RFM_LOG_GSPAWN,
	     ((child_attribs->runOpts & G_SPAWN_CHILD_INHERITS_STDIN)==G_SPAWN_CHILD_INHERITS_STDIN) ? G_LOG_LEVEL_DEBUG : G_LOG_LEVEL_WARNING // bash read 命令会把输出的提示通过stderr显示，而不是stdout, 若要使用read, 应该是inherit_stdin的，通过这个判断,防止在read的时候输出warning
	       ,"error of strlen %ld from child PID %d shown", strlen(child_attribs->stdErr), child_attribs->pid);
       //TODO: if not startwithVT(), user won't see printf, but user also will not see all those g_warnings. maybe we can add some indicator on UI and guide user to some log file. When not startwithVT, redirect stdout and stderr to some log file
     }else if (strlen(child_attribs->stdErr) <= RFM_MX_MSGBOX_CHARS)
         show_msgbox(child_attribs->stdErr, child_attribs->name, GTK_MESSAGE_INFO);
     else{
         msg=g_strdup_printf("error of strlen %ld from child PID %d greater that RFM_MX_MSGBOX_CHARS %d, cannot show in msgbox", strlen(child_attribs->stdErr), child_attribs->pid,RFM_MX_MSGBOX_CHARS);
	 show_msgbox(msg, child_attribs->name, GTK_MESSAGE_WARNING);
	 g_free(msg);
     }
     if (child_attribs->keep_total_buffer_in_show_child_output){
       if (child_attribs->total_stdErr)
         child_attribs->total_stdErr=g_strconcat(child_attribs->total_stdErr,child_attribs->stdErr, NULL);
       else
	 child_attribs->total_stdErr=g_strdup(child_attribs->stdErr);
     }
     child_attribs->stdErr[0]=0;
   }else{ g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "empty child_attribs->stdErr in show_child_output"); }
}

static int read_char_pipe(gint fd, ssize_t block_size, char **buffer)
{
   char *txt=NULL;
   char *txtNew=NULL;
   ssize_t read_size=-1;
   ssize_t txt_size, i;

   txt=calloc((block_size+1),sizeof(char));
   if (txt==NULL) return -1;

   read_size=read(fd, txt, block_size);
   if (read_size < 1) {
      free(txt);
      return read_size;
   }
   g_debug("read_char_pipe fd:%d read_size:%ld",fd,read_size);
   txt[read_size]='\0';

   txt_size=strlen(txt);   /* Remove embedded null characters */
   if (txt_size<read_size) {
      for (i=txt_size; i<read_size; i++) {
         if (txt[i]=='\0') txt[i]='?';
      }
   }

   if (*buffer==NULL)
      *buffer=txt;
   else {
      txtNew=realloc(*buffer, (strlen(*buffer)+read_size+1)*sizeof(char));
      if (txtNew!=NULL) {
         *buffer=txtNew;
         strcat(*buffer, txt);
      }
      g_free(txt);
   }

   return read_size;
}

static RFM_defaultPixbufs *load_default_pixbufs(void)
{
   GdkPixbuf *umount_pixbuf;
   GdkPixbuf *mount_pixbuf;
   RFM_defaultPixbufs *defaultPixbufs=NULL;
   
   if(!(defaultPixbufs = calloc(1, sizeof(RFM_defaultPixbufs))))
      return NULL;
   defaultPixbufs->file=gtk_icon_theme_load_icon(icon_theme, "application-octet-stream", RFM_ICON_SIZE, 0, NULL);
   defaultPixbufs->dir=gtk_icon_theme_load_icon(icon_theme, "folder", RFM_ICON_SIZE, 0, NULL);
   defaultPixbufs->symlink=gtk_icon_theme_load_icon(icon_theme, "emblem-symbolic-link", RFM_ICON_SIZE/2, 0, NULL);
   defaultPixbufs->broken=gtk_icon_theme_load_icon(icon_theme, "emblem-unreadable", RFM_ICON_SIZE/2, 0, NULL);

   umount_pixbuf=gdk_pixbuf_new_from_xpm_data(RFM_icon_unmounted);
   mount_pixbuf=gdk_pixbuf_new_from_xpm_data(RFM_icon_mounted);

   if (defaultPixbufs->file==NULL) defaultPixbufs->file=gdk_pixbuf_new_from_xpm_data(RFM_icon_file);
   if (defaultPixbufs->dir==NULL) defaultPixbufs->dir=gdk_pixbuf_new_from_xpm_data(RFM_icon_folder);
   if (defaultPixbufs->symlink==NULL) defaultPixbufs->symlink=gdk_pixbuf_new_from_xpm_data(RFM_icon_symlink);
   if (defaultPixbufs->broken==NULL) defaultPixbufs->symlink=gdk_pixbuf_new_from_xpm_data(RFM_icon_broken);

   /* Composite images */
   defaultPixbufs->symlinkDir=gdk_pixbuf_copy(defaultPixbufs->dir);// 符号链接文件夹：在普通文件夹图标上叠加符号链接标志
   gdk_pixbuf_composite(defaultPixbufs->symlink,defaultPixbufs->symlinkDir,0,0,gdk_pixbuf_get_width(defaultPixbufs->symlink),gdk_pixbuf_get_height(defaultPixbufs->symlink),0,0,1,1,GDK_INTERP_NEAREST,200);
   defaultPixbufs->symlinkFile=gdk_pixbuf_copy(defaultPixbufs->file);
   gdk_pixbuf_composite(defaultPixbufs->symlink,defaultPixbufs->symlinkFile,0,0,gdk_pixbuf_get_width(defaultPixbufs->symlink),gdk_pixbuf_get_height(defaultPixbufs->symlink),0,0,1,1,GDK_INTERP_NEAREST,200);
   defaultPixbufs->unmounted=gdk_pixbuf_copy(defaultPixbufs->dir);
   gdk_pixbuf_composite(umount_pixbuf,defaultPixbufs->unmounted,1,1,gdk_pixbuf_get_width(umount_pixbuf),gdk_pixbuf_get_height(umount_pixbuf),0,0,1,1,GDK_INTERP_NEAREST,100);
   defaultPixbufs->mounted=gdk_pixbuf_copy(defaultPixbufs->dir);
   gdk_pixbuf_composite(mount_pixbuf,defaultPixbufs->mounted,1,1,gdk_pixbuf_get_width(mount_pixbuf),gdk_pixbuf_get_height(mount_pixbuf),0,0,1,1,GDK_INTERP_NEAREST,100);

   defaultPixbufs->dimensionDir = gdk_pixbuf_copy(defaultPixbufs->dir);// 创建 dimensionDir（目录中显示符号号链接，表示这是一个包含符号链接的目录，也就是dimension的直观表示）
   g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG, "DimensionDir 尺寸: %dx%d, Symlink 尺寸: %dx%d", gdk_pixbuf_get_width(defaultPixbufs->dimensionDir), gdk_pixbuf_get_height(defaultPixbufs->dimensionDir), gdk_pixbuf_get_width(defaultPixbufs->symlink), gdk_pixbuf_get_height(defaultPixbufs->symlink));
    // 直接复制符号链接小图标到指定位置
   gdk_pixbuf_copy_area(defaultPixbufs->symlink,
                        0, 0,  // 源位置
                        gdk_pixbuf_get_width(defaultPixbufs->symlink),
                        gdk_pixbuf_get_height(defaultPixbufs->symlink),
                        defaultPixbufs->dimensionDir,
                        (gdk_pixbuf_get_height(defaultPixbufs->dimensionDir) - gdk_pixbuf_get_height(defaultPixbufs->symlink))/2 , (gdk_pixbuf_get_height(defaultPixbufs->dimensionDir) - gdk_pixbuf_get_height(defaultPixbufs->symlink))*2/3);  // 目标位置

   defaultPixbufs->symlinkDimensionDir = gdk_pixbuf_copy(defaultPixbufs->dimensionDir);
   gdk_pixbuf_composite(defaultPixbufs->symlink,defaultPixbufs->symlinkDimensionDir,0,0,gdk_pixbuf_get_width(defaultPixbufs->symlink),gdk_pixbuf_get_height(defaultPixbufs->symlink),0,0,1,1,GDK_INTERP_NEAREST,200);// 在dimensionDir的左上角再加个symlink就表示这是指向dimensionDir的symlink

   g_object_unref(umount_pixbuf);
   g_object_unref(mount_pixbuf);
  
   return defaultPixbufs;
}


// 对话框关闭时的回调函数，增加计数
static void msgbox_response_handler(GtkDialog *dialog, gint response_id, gpointer user_data) {
    RFM_msgbox_throttle++;
    if (RFM_msgbox_throttle == 0) RFM_msgbox_throttle++;
    g_debug("Message box closed, current RFM_msgbox_throttle: %d", RFM_msgbox_throttle);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean show_msgbox_in_main_thread(MsgBoxData *msg_data) {
    GtkWidget *dialog = NULL;
    if (window && RFM_msgbox_throttle > 0) {
        dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, msg_data->type, GTK_BUTTONS_OK, "%s", msg_data->msg);
        g_signal_connect(dialog, "response", G_CALLBACK(msgbox_response_handler), NULL);
    }else {
      g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s", msg_data->msg);//TODO: 把原本的LOG DOMAIN 和LOG_LEVEL 传进来，把g_log和show_msgboxt统一起来。比如加一个g_log_wrapper函数
      if (window && RFM_msgbox_throttle == 0) {
          dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, RFM_MSGBOX_THROTTLE_WARNING);
          g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
      }
    }
    if (dialog){
        RFM_msgbox_throttle--;
        gtk_dialog_run(GTK_DIALOG(dialog));
    }
    g_free(msg_data->msg); g_free(msg_data->title); g_free(msg_data);
    return FALSE;
}

static void show_msgbox(gchar *msg, gchar *title, gint type)
{
  MsgBoxData *data = g_new(MsgBoxData, 1);
  data->msg = g_strdup(msg);
  data->title = g_strdup(title);
  data->type = type;
  // 检查是否在主线程中
  if (g_main_context_is_owner(g_main_context_default())) show_msgbox_in_main_thread(data);
  else g_idle_add(show_msgbox_in_main_thread, data);
}


static gboolean g_spawn_async_with_pipes_wrapper(CALLER_OWN gchar **v, RFM_ChildAttribs *child_attribs)
{
   gboolean rv=FALSE;
   if (child_attribs!=NULL) {
      child_attribs->pid=-1;
      GError* err = NULL;
/* https://docs.gtk.org/glib/func.spawn_async_with_pipes_and_fds.html */
/*       If non-NULL, the stdin_pipe_out, stdout_pipe_out, stderr_pipe_out locations will be filled with file descriptors for writing to the child's standard input or reading from its standard output or standard error. The caller of g_spawn_async_with_pipes() must close these file descriptors when they are no longer in use. If these parameters are NULL, the corresponding pipe won't be created. */
      rv=g_spawn_async_with_pipes(choose_working_directory_for_g_spawn(child_attribs), v, child_attribs->env, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH | child_attribs->runOpts,
				  //本commit 是revert 3d53359fdb97498a895a0cec97e3076558908a02 消除冲突后的结果，注意上一行|child_attribs->runOpts 是保留3d53359fdb97498a895a0cec97e3076558908a02改动的

				  GSpawnChildSetupFunc_setenv,child_attribs,
                                  &child_attribs->pid,
				  (child_attribs->stdIn_fd==0 ? NULL: &(child_attribs->stdIn_fd)), //we use calloc to new child_attribs, stdIn_fd will default to 0
				  ((child_attribs->stdOut_fd<=0)? &child_attribs->stdOut_fd: NULL), // stdOut_fd>0, means we use existing fd to read result from, so we pass in NULL, otherwise stdout_fd will be overwriten. For example, we use existing fd in rfmFileChooser
                                  &child_attribs->stdErr_fd, &err);
      
      if (rv==TRUE) {
	 g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "child pid:%d, child stdIn_fd:%d, child stdOut_fd:%d, child stdErr_fd:%d",child_attribs->pid, child_attribs->stdIn_fd,child_attribs->stdOut_fd,child_attribs->stdErr_fd);
         /* Don't block on read if nothing in pipe */
         if (! g_unix_set_fd_nonblocking(child_attribs->stdOut_fd, TRUE, NULL))
	    g_warning("Can't set child stdout to non-blocking mode, PID:%d,FD:%d",child_attribs->pid,child_attribs->stdOut_fd);
         if (! g_unix_set_fd_nonblocking(child_attribs->stdErr_fd, TRUE, NULL))
	    g_warning("Can't set child stderr to non-blocking mode, PID:%d,FD:%d",child_attribs->pid,child_attribs->stdErr_fd);

         if(child_attribs->name==NULL) child_attribs->name=g_strdup(v[0]);
         child_attribs->status=-1;  /* -1 indicates child is running; set to wait wstatus on exit */

         g_timeout_add(100, (GSourceFunc)g_spawn_async_with_pipes_wrapper_child_supervisor, (void*)child_attribs);
         g_child_watch_add(child_attribs->pid, (GChildWatchFunc)set_finished_status_for_child_supervisor, child_attribs);
         rfm_childList=g_list_prepend(rfm_childList, child_attribs);
	 rfm_childListLength++;
         //gtk_widget_set_sensitive(GTK_WIDGET(info_button), TRUE);
      }else{
	g_warning("g_spawn_async_with_pipe error:%s",err->message);
	g_error_free(err);
      }
   }
   return rv;
}


static MEM_ALLOC gchar **build_cmd_vector(CALLER_OWN const char **cmd, CALLER_OWN GList *file_list, CALLER_OWN const char *dest_path)
{
   long j=0;
   gchar **v=NULL;
   file_list=g_list_first(file_list);
  
   if((v=calloc((RFM_MX_ARGS), sizeof(gchar*)))==NULL)
      return NULL;

   while (cmd[j]!=NULL && j<RFM_MX_ARGS) {
     if (strcmp(cmd[j],RFM_selected_filename_placeholder)==0 && (file_list||dest_path)){//如果file_list为NULL,而cmd里面有 RFM_selected_filename_picker, 如%s,并且dest_path 不为空，可以用dest_path来替换%s，相当于dest_path 作为最后一个元素加入file_list
       // before this commit, file_list and dest_path are all appended after cmd, but commands like ffmpeg to create thumbnail need to have file name in the middle of the argv, appended at the end won't work. So i modify the rule here so that if we have placeholder in cmd, we replace it with item in file_list. So, file_list can work as generic argument list later, not necessarily the filename. And replacing empty string place holders in cmd with items in file_list can be something like printf.
       if (file_list) {
	 v[j]=file_list->data; //this data is owned by rfm_FileAttributes and was not freed before, but now, v and file_list share the reference.  g_list_free_full never called on file_list, and filename char* is not freed when free(v),  but freed with rfm_FileAttributes.
	 file_list=g_list_next(file_list);
       }else{
	 v[j]=dest_path; dest_path=NULL;
       }
     }else if (cmd[j][0]=='$' && strlen(cmd[j])>1){
       const char* envName = cmd[j] + sizeof(char); //remove leading $ in cmd[j] to get envName
       char* envValue = getenv(envName);
       if (envValue==NULL){
	 g_warning("environment variable name %s is used, but NULL returned for value.",envName);
	 free(v);
	 return NULL;
       }else if (strlen(envValue)>0){
	 v[j]=strdup(envValue);
       }else { continue; } //当环境变量值为空值“”，非NULL时,如RFM_TERM_E,跳过，也不增加j，否则 foot "" ls 这样的命令会报错
     }else if (strcmp(cmd[j],"")!=0){
       v[j]=(gchar*)cmd[j]; /* FIXME: gtk spawn functions require gchar*, but we have const gchar*; should probably g_strdup() and create a free_cmd_vector() function */
     }//else { continue; } //走到这里表示cmd[j] 等于 ""，按照前面环境变量值为空非NULL时直接跳过并不增加j，这里似乎也应该这样处理，但有没有可能用户就是要放一个空字符串在这里？
     j++;
   }
   if (j==RFM_MX_ARGS) {
      g_warning("build_cmd_vector: %s: RFM_MX_ARGS exceeded. Check config.h!",v[0]);
      free(v);
      return NULL;
   }
   //把没用完的file_list成员append到命令结尾
   while (file_list!=NULL) {
      v[j]=file_list->data;
      j++;
      file_list=g_list_next(file_list);
   }

   v[j]=dest_path; /* This may be NULL anyway */
   v[++j]=NULL;

   return v;
}

static void GSpawnChildSetupFunc_setenv(gpointer user_data) {
      RFM_ChildAttribs* child_attribs=(RFM_ChildAttribs*) user_data;
      //if (rfm_curPath!=NULL) setenv("PWD",rfm_curPath,TRUE);
      //working_directory won't update child process PWD env, which inherits parents PWD env,why?
      if (child_attribs->setsid) setsid();//防止子进程和readline线程争夺终端控制权，产生SIGTTIN 信号，导致readline所在进程组被放入后台。
}

static void check_wait_status_and_set_exit_code(CALLER_OWN RFM_ChildAttribs *child_attribs) {
	            GError *wait_err = NULL;
	            child_attribs->exitcode = -1; // Default to error unless status check passes
		    
		    if (!g_spawn_check_wait_status(child_attribs->status, &wait_err)) {
	                //g_log(RFM_LOG_GSPAWN,child_attribs->output_read_by_program? G_LOG_LEVEL_DEBUG: G_LOG_LEVEL_WARNING,"Child process exited with code %d: %s", WEXITSTATUS(child_attribs->status), wait_err ? wait_err->message : "Unknown reason"); 当初这么写可能是为了防止太多的warning，影响用户，但 SearchResult|nocmd>0 这样的找不到命令错也吞了会给用户更大困惑。这里也就不判断情况了，统一简化为warning, 使用中有问题再改。参见 g_spawn_async_with_pipes_wrapper_child_supervisor 中关于child_attribs->stderr 的注释，现在这里可以看到warning和error code 了，但 '未找到命令' 这样的错误消息还是debug显示
		        g_warning("Child process exited with code %d: %s", WEXITSTATUS(child_attribs->status), wait_err ? wait_err->message : "Unknown reason");
	                if (wait_err) g_error_free(wait_err);
	                // Keep exitcode as -1
	            } else {
	                child_attribs->exitcode = WEXITSTATUS(child_attribs->status);
	                g_log(RFM_LOG_GSPAWN, G_LOG_LEVEL_DEBUG, "Child process finished with exit code %d.", child_attribs->exitcode);
	            }
}

static gchar *choose_working_directory_for_g_spawn(RFM_ChildAttribs *childAttribs) {
  if (childAttribs->workingDir) return childAttribs->workingDir;//workingDir 是强制指定的，有最高优先级
  if (childAttribs->searchresulttype_addon_dir) return childAttribs->searchresulttype_addon_dir;
  if (childAttribs->cmd_interpretor_addon_dir) return childAttribs->cmd_interpretor_addon_dir;
  if (childAttribs->toolbar_button_or_filemenu_addon_dir) return childAttribs->toolbar_button_or_filemenu_addon_dir;
  return rfm_curPath;
}

//caller should g_list_free(file_list), but usually not g_list_free_full, since the file char* is usually owned by rfm_fileattributes
//TODO：file_list 和 dest_path 这两个参数我都是从Rodney那里继承过来的，我猜最初可能是为了匹配cp,mv这一类的文件命令。我改动了 build_cmd_vector 这个方法。我原先一直觉得dest_path这个变量有些多余，但现在突然发觉，file_list是个list,如果只有一个文件名，造个list比较麻烦，用dest_path参数反倒比较好，这里我们还要处理一下 build_cmd_vector 的TODO,来应对dest_path 不是追加到命令结尾，而是替换命令中间%s的情况
gboolean g_spawn_wrapper_(CALLER_OWN GList *file_list, CALLER_OWN const char *dest_path, FREE_FULL RFM_ChildAttribs * child_attribs)
{
   gchar **v=NULL;
   gboolean ret=TRUE;

   v=build_cmd_vector(child_attribs->RunCmd, file_list, dest_path);
   if (v != NULL) {
      setenv("RFM_DIMENSION_PROPERTIES_FILE_NAME",RFM_DIMENSION_PROPERTIES_FILE_NAME, 1);
      setenv("RFM_SearchResultColumnSeperator",SearchResultColumnSeperator,1);
      //上面setenv原本放在 GSpawnChildSetupFunc_setenv 里的，但是测试发觉不稳定，有时子进程里没设置
      gchar * argv=g_strjoinv(" ", v);
      gchar* workingDir = choose_working_directory_for_g_spawn(child_attribs);
      if (child_attribs->spawn_async){
	       g_log(RFM_LOG_GSPAWN,G_LOG_LEVEL_DEBUG,"g_spawn_async_with_pipes_wrapper, workingdir:%s, argv:%s",workingDir, argv);
	       if (!g_spawn_async_with_pipes_wrapper(v, child_attribs)) {
                   g_warning("g_spawn_async_with_pipes_wrapper: %s failed to execute. Check command in config.h!", argv);
                   free_child_attribs(child_attribs); //这里是失败的异步，成功的异步会在  child_supervisor_to_ReadStdout_ShowOutput_ExecCallback 里面 free
	           ret = FALSE;
               };
      } else {
	       child_attribs->status=-1; // Initialize status
	       GError *spawn_err = NULL;
	       g_log(RFM_LOG_GSPAWN,G_LOG_LEVEL_DEBUG,"g_spawn_sync, workingdir:%s, argv:%s",workingDir, argv);
	       if (!g_spawn_sync(workingDir, v, child_attribs->env, G_SPAWN_SEARCH_PATH|child_attribs->runOpts, GSpawnChildSetupFunc_setenv,child_attribs,((child_attribs->runOpts & G_SPAWN_STDOUT_TO_DEV_NULL) || (child_attribs->runOpts & G_SPAWN_CHILD_INHERITS_STDOUT))? NULL : &child_attribs->stdOut, (child_attribs->runOpts & G_SPAWN_CHILD_INHERITS_STDERR)? NULL : &child_attribs->stdErr,&child_attribs->status,&spawn_err)){
	            g_warning("g_spawn_sync %s failed to execute: %s. Check command in config.h!", argv, spawn_err ? spawn_err->message : "Unknown error");
	            if (spawn_err) g_error_free(spawn_err);
	            free_child_attribs(child_attribs);
	            ret = FALSE;
	       } else {
		    //如果是output_read_by_program, 则stdErr和stdOut都在Callback里处理。为啥不在这里显示 stdout 呢？因为如果 stdErr 和 stdout同时不为空，show_child_output 就把stdout也消耗了，callback 就没得 stdout 读了。
   		    if (child_attribs->output_read_by_program){
		      if (child_attribs->stdErr && strlen(child_attribs->stdErr)>0){
			g_debug("g_spawn_wrapper_ sync branch, with output_read_by_program: %s",child_attribs->stdErr);
			if (child_attribs->setsid && (strstr(child_attribs->stdErr, "inappropriate ioctl for device") ||strstr(child_attribs->stdErr,"对设备不适当的 ioctl 操作"))){//TODO:这个错误难道没个编号吗？如果其他语言咋办？
			  g_free(child_attribs->stdErr); child_attribs->stdErr=NULL;
			  if (child_attribs->stdOut) {g_free(child_attribs->stdOut); child_attribs->stdOut=NULL;};
			  child_attribs->status=-1;
			  child_attribs->setsid=FALSE;
			  child_attribs->runOpts = (child_attribs->runOpts | G_SPAWN_CHILD_INHERITS_STDIN) & (~G_SPAWN_STDIN_FROM_DEV_NULL);
			  g_debug("g_spawn_wrapper_, re-run g_spawn_wrapper with setsid false for child that requires stdin");
			  // fzf>0 这样的命令需要读取标准输入， setsid 要设为FALSE, 而 locate AI 对话工具栏命令等后台长时间运行的命令需要setsid为TRUE，参见commit ba793c2c82166dca837b6e72c5f63a72476b8243. TODO:如何才能区分这两种命令呢？ 目前我没法事前区分，只有在报错后改变 setsid及runOpts 的值，然后重新执行一下命令。
			  return g_spawn_wrapper_(file_list, dest_path, child_attribs);
			}
		      }
	            }else if ((child_attribs->stdOut && strlen(child_attribs->stdOut)>0) || (child_attribs->stdErr && strlen(child_attribs->stdErr)>0)) show_child_output(child_attribs);

		    check_wait_status_and_set_exit_code(child_attribs);
		    ret = (child_attribs->exitcode==0);
		    if (child_attribs->g_idle_add_once_to_run_CallBackFunc) g_idle_add_once(ExecCallback_freeChildAttribs, child_attribs);
	            else ExecCallback_freeChildAttribs(child_attribs);
	       }
      }
      g_free(argv);
      free(v); //only free v, but char* data such as filename in command not freed here.
   }
   else{
      g_warning("g_spawn_wrapper_: %s failed to execute: build_cmd_vector() returned NULL.",child_attribs->RunCmd[0]);
      free_child_attribs(child_attribs);
      ret = FALSE;
   }
   return ret;
}

gboolean g_spawn_wrapper(CALLER_OWN const char **action, CALLER_OWN GList *file_list, const int run_opts, FREE const gchar** env, const char *dest_path, const gboolean async,const void(*callbackfunc)(gpointer),CALLER_OWN const gpointer callbackfuncUserData, const gboolean output_read_by_program){
  RFM_ChildAttribs *child_attribs=calloc(1,sizeof(RFM_ChildAttribs));
  child_attribs->customCallBackFunc=callbackfunc;
  child_attribs->customCallbackUserData=callbackfuncUserData;
  child_attribs->runOpts=run_opts;
  child_attribs->RunCmd = g_strdupv(action);
  child_attribs->stdOut = NULL;
  child_attribs->stdErr = NULL;
  child_attribs->spawn_async = async;
  child_attribs->name=g_strdup(action[0]);
  child_attribs->output_read_by_program=output_read_by_program;
  child_attribs->env = env;
	
  return g_spawn_wrapper_(file_list,dest_path,child_attribs);
}

gboolean g_spawn_wrapper_wordexp(CALLER_OWN const char *cmd, CALLER_OWN GList *file_list, const int run_opts, FREE const gchar** env, const char *dest_path, const gboolean async,const void(*callbackfunc)(gpointer),CALLER_OWN const gpointer callbackfuncUserData,const gboolean output_read_by_program){
    gboolean success = FALSE;
    wordexp_t parsed_msg;
    int wordexp_retval = wordexp(cmd,&parsed_msg,0);
    if (wordexp_retval==0){
      success = g_spawn_wrapper(parsed_msg.we_wordv, file_list, run_opts, env, dest_path, async, callbackfunc, callbackfuncUserData, output_read_by_program);
      wordfree(&parsed_msg);
    }else g_warning("g_spawn_wrapper_wordexp: wordexp return not 0 for %s",cmd);
    return success;
}

//g_spawn 异步调用回调函数只支持一个参数,就做了这个wrapper;并且作为异步调用thumbData->thumbnail可能在本函数工作前就跟随thumbqueue被free了,这里thumbnail是strdup进来的,要释放
static int load_thumbnail_as_asyn_callback(RFM_ChildAttribs *childAttribs){
  RFM_ThumbQueueData *thumbdata = childAttribs->customCallbackUserData;
  thumbdata->g_source_id_for_load_thumbnail = g_idle_add_once(load_thumbnail_when_idle_in_gtk_main_loop, thumbdata);
}

static int load_thumbnail_when_idle_in_gtk_main_loop(RFM_ThumbQueueData *thumbdata){
  return load_thumbnail(thumbdata, FALSE, FALSE);
}

/* Load and update a thumbnail from disk cache */
static int load_thumbnail(RFM_ThumbQueueData* thumbdata, gboolean show_Thumbnail_Itself_InsteadOf_As_Thumbnail_For_Original_Picture, gboolean check_tEXt)
{
   GtkTreeIter iter;
   GdkPixbuf *pixbuf=NULL;
   GtkTreePath *treePath;
   gchar *thumb_path;
   const gchar *tmp=NULL;
   GtkTreeRowReference *reference;
   gint64 mtime_file=0;
   gint64 mtime_thumb=1;

   thumbdata->g_source_id_for_load_thumbnail=0;
   
   reference=g_hash_table_lookup(thumb_hash, thumbdata->thumb_name);
   if (reference==NULL){
     g_warning("load_thumbnail failed for %s: thumb_hash key not found",thumbdata->thumb_name);
     return 1;  /* Key not found */
   }
   treePath=gtk_tree_row_reference_get_path(reference);
   if (treePath == NULL){
     g_warning("load_thumbnail failed for %s: GtkTreePath not found",thumbdata->thumb_name);
     return 2;   /* Tree path not found */
   }
   gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treePath);
   gtk_tree_path_free(treePath);
   thumb_path=g_build_filename(rfm_thumbDir, thumbdata->thumb_name, NULL);
#ifdef RFM_CACHE_THUMBNAIL_IN_MEM
   pixbuf=g_hash_table_lookup(pixbuf_hash, key);
#endif
   if (pixbuf==NULL){
      pixbuf=gdk_pixbuf_new_from_file(thumb_path, NULL);
      if (pixbuf==NULL){
	g_free(thumb_path);
	g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG, "load_thumbnail failed for %s: gdk_pixbuf_new_from_file returned NULL",thumbdata->thumb_name);
	return 3;   /* Can't load thumbnail */
      }
#ifdef RFM_CACHE_THUMBNAIL_IN_MEM
      g_hash_table_insert(pixbuf_hash, key, pixbuf);
#endif
   }

   if (!show_Thumbnail_Itself_InsteadOf_As_Thumbnail_For_Original_Picture && check_tEXt){ //显示thumbnail本身,就不要考虑过期问题了,否则为thumbnail再生成thumbnail有些奇怪
     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_MTIME, &mtime_file, -1);
     tmp=gdk_pixbuf_get_option(pixbuf, "tEXt::Thumb::MTime");
     g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG,"tEXt::Thumb::MTime for %s:%s",thumb_path,(tmp?tmp:"NULL"));
     if (tmp!=NULL) mtime_thumb=g_ascii_strtoll(tmp, NULL, 10); /* Convert to gint64 */
     if (mtime_file!=mtime_thumb) {
#ifdef Allow_Thumbnail_Without_tExtThumbMTime
       if (tmp != NULL) {
#endif

#ifdef RFM_CACHE_THUMBNAIL_IN_MEM
	 g_hash_table_remove(pixbuf_hash, key);
#endif
	 g_object_unref(pixbuf);
	 g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG, "load_thumbnail failed for %s: Thumbnail out of date",thumbdata->thumb_name);
	 return 4; /* Thumbnail out of date */
#ifdef Allow_Thumbnail_Without_tExtThumbMTime
       }
#endif
     }
   }
   g_free(thumb_path);
   gtk_list_store_set (store, &iter, COL_PIXBUF, pixbuf, -1);
#ifndef RFM_CACHE_THUMBNAIL_IN_MEM
   g_object_unref(pixbuf);
#endif
   return 0;
}

/* Return the index of a defined thumbnailer which will handle the filename suffix and mime type */
static gint find_thumbnailer(gchar *mime_root, gchar *mime_sub_type, gchar *filepath)
{
   gint filenameSuffix_root_sub_idx=-1;
   gint filenameSuffix_root_idx=-1;
   gint filenameSuffix_idx=-1;
   gint root_sub_idx=-1;
   gint root_idx=-1;

   for(guint i=0;i<G_N_ELEMENTS(thumbnailers);i++) {
     if (thumbnailers[i].filenameSuffix){
       if (g_str_has_suffix(filepath, thumbnailers[i].filenameSuffix)){
	 if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strcmp(mime_sub_type, thumbnailers[i].thumbSub)==0) {
	   filenameSuffix_root_sub_idx=i;
	   break;
	 }else if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strncmp("*", thumbnailers[i].thumbSub, 1)==0)
	   filenameSuffix_root_idx=i;
	 else if (strcmp("*", thumbnailers[i].thumbRoot)==0 && strncmp("*", thumbnailers[i].thumbSub, 1)==0)
	   filenameSuffix_idx=1;
       }
     }else{
       if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strcmp(mime_sub_type, thumbnailers[i].thumbSub)==0) {
         root_sub_idx=i;
       }else if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strncmp("*", thumbnailers[i].thumbSub, 1)==0)
         root_idx=i;
     }
   }

   if (filenameSuffix_root_sub_idx!=-1) return filenameSuffix_root_sub_idx;
   else if (filenameSuffix_root_idx!=-1) return filenameSuffix_root_idx;
   else if (root_sub_idx!=-1) return root_sub_idx;
   else if (root_idx!=-1) return root_idx;
   else if (filenameSuffix_idx!=-1) return filenameSuffix_idx;

   return -1;
}

static void rfm_saveThumbnail(GdkPixbuf *thumb, RFM_ThumbQueueData *thumbData)
{
   GdkPixbuf *thumbAlpha=NULL;
   gchar *mtime;
   gchar *thumb_path;
   thumbAlpha=gdk_pixbuf_add_alpha(thumb, FALSE, 0, 0, 0);  /* Add a transparency channel: some software expects this */
   thumb_path=g_build_filename(rfm_thumbDir, thumbData->thumb_name, NULL);
   if (thumbAlpha!=NULL) {
      mtime=g_strdup_printf("%"G_GINT64_FORMAT, thumbData->mtime_file);
      if (mtime!=NULL) {
	if (gdk_pixbuf_save(thumbAlpha, thumb_path, "png", NULL,
            "tEXt::Thumb::MTime", mtime,
            "tEXt::Thumb::URI", thumbData->uri,
            "tEXt::Software", PROG_NAME,
			    NULL)){
	  if (chmod(thumb_path, S_IRUSR | S_IWUSR)==-1) g_warning("rfm_saveThumbnail: Failed to chmod %s\n", thumb_path);
	  else thumbData->g_source_id_for_load_thumbnail = g_idle_add_once(load_thumbnail_when_idle_in_gtk_main_loop, thumbData);
	}else g_warning("rfm_saveThumbnail: gdk_pixbuf_save failed for %s\n", thumb_path);
      }else g_warning("rfm_saveThumbnail: mtime null for %s\n", thumb_path);
      
      g_free(thumb_path);
      g_free(mtime);
      g_object_unref(thumbAlpha);
   }else g_warning("rfm_saveThumbnail: add Alpha failed for %s\n", thumb_path);
}

static void mkThumbLoop(){
  while(!mkThumb_stop && mkThumb());
}

//运行在 mkThumbnail 线程里，rfm_thumbQueue 在gtk maim loop 里面被添加，在这里使用， 但 mkThumbnail 线程只有在 rfm_thumbQueue 被添加完成后才会被创建，而且在rfm_stop_all里会被清除。 也就是说不会存在 gtk main loop 和 mkThumbnail 线程对 rfm_thumbQueue 的竞争访问
static gboolean mkThumb()
{
   RFM_ThumbQueueData *thumbData;
   GdkPixbuf *thumb=NULL;
   GError *pixbufErr=NULL;

   if (rfm_thumbQueue==NULL) return G_SOURCE_REMOVE;
   thumbData=(RFM_ThumbQueueData*)rfm_thumbQueue->data;

   if (thumbnailers[thumbData->t_idx].thumbCmd==NULL){
     thumb=gdk_pixbuf_new_from_file_at_scale(thumbData->path, thumbData->thumb_size, thumbData->thumb_size, TRUE, &pixbufErr);
      if (thumb!=NULL) {
	rfm_saveThumbnail(thumb, thumbData);
	g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG, "thumbnail saved for %s",thumbData->thumb_name);
	g_object_unref(thumb);
      }else{
	if (pixbufErr==NULL)
	  g_warning("thumbnail null returned by gdk_pixbuf_new_from_file_at_scale for %s",thumbData->thumb_name);
	else{
	  g_warning("thumbnail null returned by gdk_pixbuf_new_from_file_at_scale for %s. GError code:%d, GError msg:%s",thumbData->thumb_name,pixbufErr->code,pixbufErr->message);
	  g_error_free(pixbufErr);
	}
      }
   }else {
      gchar *thumb_path=g_build_filename(rfm_thumbDir, thumbData->thumb_name, NULL);
      GList * input_files=NULL;
      input_files=g_list_prepend(input_files, g_strdup(thumbData->path));
      g_spawn_wrapper(thumbnailers[thumbData->t_idx].thumbCmd, input_files, G_SPAWN_STDOUT_TO_DEV_NULL, NULL, thumb_path, FALSE, load_thumbnail_as_asyn_callback,thumbData,FALSE);//当async参数设为TRUE的时候,运行多个ffmpeg实例,会产生很多g_warning的错误,包含ffmpeg运行的信息.在之前branch里mkThumb运行于gtk main thread 的时候,也能看到这些g_warning,但没看到对rfm本身造成啥问题.但现在mkThumb运行在单独线程里,会造成rfm 读取标准输入命令出错, 会不停显示新的命令提示符,有时直接就rfm退出了,我不知道为啥.这里async改为false,就没看到这个问题了.
      g_list_free_full(input_files, (GDestroyNotify)g_free);
      g_free(thumb_path);
   }

   if (rfm_thumbQueue->next!=NULL) {   /* More items in queue */
      rfm_thumbQueue=g_list_next(rfm_thumbQueue);
      g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG,"mkThumb return TRUE after:%s",thumbData->thumb_name);
      return G_SOURCE_CONTINUE;
   }
   g_debug("mkThumb return FALSE,which means mkThumb finished");
   return G_SOURCE_REMOVE;  /* Finished thumb queue */
}

static RFM_ThumbQueueData *get_thumbData(GtkTreeIter *iter)
{
   GtkTreePath *treePath=NULL;
   RFM_ThumbQueueData *thumbData;
   RFM_FileAttributes *fileAttributes;

   gtk_tree_model_get(GTK_TREE_MODEL(store), iter, COL_ATTR, &fileAttributes, -1);

   if (fileAttributes->is_dir) return NULL;
   
   thumbData=calloc(1, sizeof(RFM_ThumbQueueData));
   if (thumbData==NULL) {
     g_warning("calloc for RFM_ThumbQueueData failed");
     return NULL;
   }

   if (strcmp(fileAttributes->mime_sub_type,"x-desktop")==0 && strcmp(fileAttributes->mime_root,"application")==0){
     thumbData->appInfoFromDesktopFile = g_desktop_app_info_new (fileAttributes->file_name);
     if (thumbData->appInfoFromDesktopFile)
       g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG, "create GDesktopAppInfo for desktop file:%s", fileAttributes->path);
     else{
       g_warning("Failed to create GDesktopAppInfo for desktop file:%s", fileAttributes->path);
       //g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG, "Failed to create GDesktopAppInfo for desktop file:%s", fileAttributes->path);
       free(thumbData);
       thumbData=NULL;
     }
     return thumbData;
   }

   thumbData->t_idx=find_thumbnailer(fileAttributes->mime_root, fileAttributes->mime_sub_type, fileAttributes->path);
   if (thumbData->t_idx==-1) {
      free(thumbData);
      return NULL;  /* Don't show thumbnails for files types with no thumbnailer */
   }

   thumbData->thumb_size = RFM_THUMBNAIL_SIZE;
   thumbData->path=canonicalize_file_name(fileAttributes->path);
   thumbData->mtime_file = fileAttributes->file_mtime==NULL? 0 : g_date_time_to_unix(fileAttributes->file_mtime);
   thumbData->uri=g_filename_to_uri(thumbData->path, NULL, NULL);
   //don't generate thumbnail for thumbnail, show itself for picture in rfm_thumbDir
   if (strncmp(rfm_thumbDir, thumbData->path, strlen(rfm_thumbDir))!=0){
     thumbData->md5=g_compute_checksum_for_string(G_CHECKSUM_MD5, thumbData->uri, -1);
     thumbData->thumb_name=g_strdup_printf("%s_%d.png", thumbData->md5, thumbData->thumb_size);
   }else{
     thumbData->thumb_name=g_strdup(thumbData->path + strlen(rfm_thumbDir) + 1); //get the thumbnail filename without rfm_thumbDir path,  rfm_thumbDir does not have ending '/', so +1
   }
   thumbData->rfm_pid=getpid();  /* pid is used to generate a unique temporary thumbnail name */

   /* Map thumb path to model reference for inotify */
   treePath=gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
   g_hash_table_insert(thumb_hash, g_strdup(thumbData->thumb_name), gtk_tree_row_reference_new(GTK_TREE_MODEL(store), treePath));
   g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG, "thumbname %s inserted into thumb_hash for %s",thumbData->thumb_name, thumbData->path);
   gtk_tree_path_free(treePath);

   return thumbData;
}

static void refreshThumbnail() {
  GList* selection_GtkTreePath_list = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  GtkTreeIter iter;
  GList *listElement = g_list_first(selection_GtkTreePath_list);
  while(listElement!=NULL) {
	 gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data); //GtkTreePath to GtkTreeIter
	 RFM_ThumbQueueData* thumbdata = get_thumbData(&iter);
	 rfm_thumbQueue = g_list_append(rfm_thumbQueue, thumbdata);
	 g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG,"thumbnail %s creation enqueued for %s; manual refresh",thumbdata->thumb_name,thumbdata->path);

         listElement=g_list_next(listElement);
  }
  g_list_free_full(selection_GtkTreePath_list, (GDestroyNotify)gtk_tree_path_free);
  if (rfm_thumbQueue && mkThumb_thread==NULL) mkThumb_thread = g_thread_new("mkThumbnail", mkThumbLoop, NULL);
}

static void free_fileAttributes(RFM_FileAttributes *fileAttributes) {
   g_free(fileAttributes->path);
   g_free(fileAttributes->file_name);
   g_free(fileAttributes->file_name_escaped_for_iconview_markup_column);
   g_free(fileAttributes->link_target_filename);
   //g_free(fileAttributes->display_name);
   g_clear_object(&(fileAttributes->pixbuf));
   g_free(fileAttributes->mime_root);
   g_free(fileAttributes->mime_sub_type);
   g_free(fileAttributes->icon_name);
   g_free(fileAttributes->owner);
   g_free(fileAttributes->group);
   g_free(fileAttributes->file_mode_str);
   g_free(fileAttributes->mime_sort);
   g_free(fileAttributes->mtime);
   g_free(fileAttributes->atime);
   g_free(fileAttributes->ctime);
   if (fileAttributes->file_mtime!=NULL) g_date_time_unref(fileAttributes->file_mtime);
   if (fileAttributes->file_atime!=NULL) g_date_time_unref(fileAttributes->file_atime);
   if (fileAttributes->file_ctime!=NULL) g_date_time_unref(fileAttributes->file_ctime);
   g_free(fileAttributes);
}

static RFM_FileAttributes *malloc_fileAttributes(void)
{
   RFM_FileAttributes *fileAttributes=calloc(1,sizeof(RFM_FileAttributes));
   if (fileAttributes==NULL) g_warning("calloc failed!");
   return fileAttributes;
}

static gboolean mountpoint_mounted(RFM_FileAttributes *fileAttributes) { return fileAttributes->is_mountPoint && fileAttributes->pixbuf==defaultPixbufs->mounted;}

static gboolean mountpoint_unmounted(RFM_FileAttributes *fileAttributes) { return fileAttributes->is_mountPoint && fileAttributes->pixbuf==defaultPixbufs->unmounted;}

static RFM_FileAttributes *get_fileAttributes_for_a_file(const gchar *name, guint64 mtimeThreshold, GHashTable *mount_hash)
{
   GFile *file=NULL;
   GFileInfo *info=NULL;
   gchar *attibuteList="*";
   GFileType fileType;
   //RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");
   gchar *mime_type=NULL;
   //gchar *utf8_display_name=NULL;
   gchar *is_mounted=NULL;
   size_t i;
   RFM_FileAttributes *fileAttributes=malloc_fileAttributes();
   if (fileAttributes==NULL) return NULL;

   fileAttributes->id = fileAttributeID;
   gchar *absoluteaddr = SearchResultViewInsteadOfDirectoryView ? g_canonicalize_filename(name, rfm_SearchResultPath) : g_canonicalize_filename(name, rfm_curPath); //address returned by find can be ./blabla, and g_build_filename return something like /rfm/./blabla, so, need to be canonicalized here

   fileAttributes->path=absoluteaddr;
   g_log(RFM_LOG_DATA,G_LOG_LEVEL_DEBUG,"fileAttributes->path:%s",fileAttributes->path);
   //attibuteList=g_strdup_printf("%s,%s,%s,%s",G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK);
   file=g_file_new_for_path(fileAttributes->path);
   GError *err=NULL;
   info=g_file_query_info(file, attibuteList, G_FILE_QUERY_INFO_NONE, NULL, &err);
   //g_free(attibuteList);
   g_object_unref(file); file=NULL;
   if (err) {
#ifdef GitIntegration
     if ((err->code == 1) && git_root){
       gchar* gitstatus = g_hash_table_lookup(gitTrackedFiles, fileAttributes->path);
       if (gitstatus && strchr(gitstatus,'D')) {
	 fileAttributes->file_name=g_strdup(name);
	 fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
	 fileAttributes->mime_root=g_strdup("na");
	 fileAttributes->mime_sub_type=g_strdup("na");
	 return fileAttributes;
       }
     }
#endif
     g_warning("g_file_query_info error %d, %s",err->code,err->message);
     g_error_free(err);
   }
   if (info == NULL ) {
      g_warning("g_file_query_info return NULL for %s",fileAttributes->path);
      g_free(fileAttributes->path);
      g_free(fileAttributes);
      return NULL;
   }
   fileAttributes->file_mtime=g_file_info_get_modification_date_time(info);
   fileAttributes->file_atime=g_file_info_get_access_date_time(info);
   fileAttributes->file_ctime=g_file_info_get_creation_date_time(info);
   fileAttributes->file_mode=g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_MODE);
   fileAttributes->file_mode_str=get_drwxrwxrwx(fileAttributes->file_mode);
   fileAttributes->is_symlink=g_file_info_get_is_symlink(info); // if the strmode function return l correctly for symlink instead of d as currently, does it mean that we don't need to call this function anymore, we can use fileAttributes->file_mode_str[0] directly?
   if (fileAttributes->is_symlink){
     //fileAttributes->file_mode_str[0]='l';
     fileAttributes->link_target_filename = g_strdup(g_file_info_get_symlink_target(info));
   }
   fileAttributes->file_name=g_strdup(name);
   fileAttributes->owner = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_OWNER_USER);
   fileAttributes->group = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_OWNER_GROUP);
   
   if (canonicalize_file_name(absoluteaddr)==NULL){
     g_warning("invalid address:%s",name);
     fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
     fileAttributes->mime_root=g_strdup("na");
     fileAttributes->mime_sub_type=g_strdup("na");
     return fileAttributes;
   }

   fileAttributes->file_size=g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
   fileType=g_file_info_get_file_type(info);

   switch (fileType) {
      case G_FILE_TYPE_DIRECTORY:
         fileAttributes->is_dir=TRUE;
         fileAttributes->mime_root=g_strdup("inode");
         if (fileAttributes->is_symlink) {
            fileAttributes->mime_sub_type=g_strdup("symlink");
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->symlinkDir);
         }else if (g_hash_table_lookup_extended(mount_hash, fileAttributes->path, NULL, (gpointer)&is_mounted)) {
            fileAttributes->mime_sub_type=g_strdup("mount-point");
            fileAttributes->is_mountPoint=TRUE;
            if (is_mounted[0]=='0')
               fileAttributes->pixbuf=g_object_ref(defaultPixbufs->unmounted);
            else
               fileAttributes->pixbuf=g_object_ref(defaultPixbufs->mounted);
         }else {
            fileAttributes->mime_sub_type=g_strdup("directory");
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->dir);
         }
	 // 指向dimension目录的symlink也应该显示为dimensionDir,所以下面不用else if
	if (g_hash_table_lookup(DimensionPropertiesHashTable_localAddress_asKey, fileAttributes->path)) {
	  if (fileAttributes->is_symlink) {
	    fileAttributes->pixbuf = g_object_ref(defaultPixbufs->symlinkDimensionDir);
	    g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"pixbuf set to symlinkDimensionDir for %s", fileAttributes->path);
	  }else{
	    fileAttributes->pixbuf=g_object_ref(defaultPixbufs->dimensionDir);
	    g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"pixbuf set to dimensionDir for %s", fileAttributes->path);
	  }
	}
      break;
      case G_FILE_TYPE_SPECIAL: /* socket, fifo, block device, or character device */
      case G_FILE_TYPE_REGULAR:
         mime_type=g_strdup(g_file_info_get_content_type(info));
         for (i=0; i<strlen(mime_type); i++) {
            if (mime_type[i]=='/') {
               mime_type[i]='\0';
               fileAttributes->mime_root=g_strdup(mime_type);
               fileAttributes->mime_sub_type=g_strdup(mime_type+i+1);
               mime_type[i]='-';
               fileAttributes->icon_name=mime_type; /* icon_name takes ref */
               break;
            }
         }
         if (fileAttributes->is_symlink)
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->symlinkFile);
         else
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->file);
      break;
      default:
         fileAttributes->mime_root=g_strdup("application");
         fileAttributes->mime_sub_type=g_strdup("octet-stream");
         fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
      break;
   }

   g_object_unref(info); info=NULL;
   return fileAttributes;
}



static gboolean iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_one_by_one_when_idle(GtkTreeIter *iter){
  load_thumbnail_or_enqueue_thumbQueue_for_store_row(iter);
  if (gtk_tree_model_iter_next(treemodel, iter)) return G_SOURCE_CONTINUE;
  else {
    if (rfm_thumbQueue && mkThumb_thread==NULL) mkThumb_thread = g_thread_new("mkThumbnail", mkThumbLoop, NULL);
    rfm_thumbLoadScheduler=0;
#ifdef GitIntegration
    if (rfm_gitCommitMsgScheduler==0){
#endif
      In_refresh_store = FALSE;
      if (!no_window) gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
#ifdef GitIntegration
    }
#endif
    return G_SOURCE_REMOVE;
  }
}

#ifdef GitIntegration
static gboolean DirectoryView_onebyone_Off_iterate_through_store_to_load_latest_git_log_one_by_one_when_idle(GtkTreeIter *iter)
{
     load_latest_git_log_for_store_row(iter);
     if ((get_treeviewColumnByEnum(COL_GIT_COMMIT_MSG)->Show ||get_treeviewColumnByEnum(COL_GIT_COMMIT_ID)->Show || get_treeviewColumnByEnum(COL_GIT_COMMIT_DATE)->Show || get_treeviewColumnByEnum(COL_GIT_AUTHOR)->Show) && gtk_tree_model_iter_next(treemodel, iter)) return G_SOURCE_CONTINUE;
     else {
       rfm_gitCommitMsgScheduler=0;
       if (rfm_thumbLoadScheduler==0){
	 In_refresh_store = FALSE;
	 if (!no_window) gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
       }
       return G_SOURCE_REMOVE;
     }
}
#endif


static void SearchResultView_iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration(void)
{
   GtkTreeIter iter;
   gboolean valid;

   valid=gtk_tree_model_get_iter_first(treemodel, &iter);
   while (valid) {
     load_thumbnail_or_enqueue_thumbQueue_for_store_row(&iter);
#ifdef GitIntegration
     if (get_treeviewColumnByEnum(COL_GIT_COMMIT_MSG)->Show ||get_treeviewColumnByEnum(COL_GIT_COMMIT_ID)->Show || get_treeviewColumnByEnum(COL_GIT_COMMIT_DATE)->Show || get_treeviewColumnByEnum(COL_GIT_AUTHOR)->Show) load_latest_git_log_for_store_row(&iter);
#endif
     valid=gtk_tree_model_iter_next(treemodel, &iter);
   }

   if (rfm_thumbQueue && mkThumb_thread==NULL) mkThumb_thread = g_thread_new("mkThumbnail", mkThumbLoop, NULL);
}


static void load_thumbnail_or_enqueue_thumbQueue_for_store_row(GtkTreeIter *iter){
      RFM_ThumbQueueData *thumbData=NULL;
      thumbData=get_thumbData(iter); /* Returns NULL if thumbnail not handled */

      if (thumbData && thumbData->appInfoFromDesktopFile) {
	GIcon *appIcon = g_app_info_get_icon( thumbData->appInfoFromDesktopFile );
	if (appIcon){
	  const gchar *type_name = G_OBJECT_TYPE_NAME(appIcon);
	  g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG, "create %s for GDesktopAppInfo:%s", type_name? type_name:"NULL", g_desktop_app_info_get_filename(thumbData->appInfoFromDesktopFile));
	  if (strcmp(type_name,"GThemedIcon")==0){
	    char **icon_names = g_themed_icon_get_names(appIcon);
	    if (icon_names){
	      GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_names[0], RFM_ICON_SIZE, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
	      g_log(RFM_LOG_DATA_THUMBNAIL, G_LOG_LEVEL_DEBUG, "create GdkPixbuf for themed icon:%s", icon_names[0]);
	      if (pixbuf){
		gtk_list_store_set (store, iter, COL_PIXBUF, pixbuf, -1);
		g_object_unref(pixbuf);
	      }
	    }
	  }
	}
	free_thumbQueueData(thumbData);

      }else if (thumbData){
         /* Try to load any existing thumbnail */
	int ld=load_thumbnail(thumbData, (strncmp(rfm_thumbDir, thumbData->path, strlen(rfm_thumbDir))==0), thumbnailers[thumbData->t_idx].check_tEXt); //如同get_thumbData函数里面同样的逻辑, thumbData->path 在 rfm_thumbDir, 就视为显示thumbnail本身,而不是将其视作其他图片的thumbnail
	 if ( ld == 0) { /* Success: thumbnail exists in cache and is valid */
	   g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG,"thumbnail %s exists for %s",thumbData->thumb_name, thumbData->path);
           free_thumbQueueData(thumbData);
         } else { /* Thumbnail doesn't exist or is out of date */
           //rfm_thumbQueue = g_list_prepend(rfm_thumbQueue, thumbData);
	   //my test is that prepend and append does not make much difference in performance here
	   rfm_thumbQueue = g_list_append(rfm_thumbQueue, thumbData);
	   g_log(RFM_LOG_DATA_THUMBNAIL,G_LOG_LEVEL_DEBUG,"thumbnail %s creation enqueued for %s; load_thumbnail failure code:%d.",thumbData->thumb_name,thumbData->path,ld);
         }
      }
}

#ifdef GitIntegration
static void load_latest_git_log_for_store_row(GtkTreeIter *iter){
      if (git_root){
	GValue full_path=G_VALUE_INIT;
	gtk_tree_model_get_value(treemodel, iter, COL_FULL_PATH, &full_path);
	GList *file_list = NULL;
	file_list = g_list_append(file_list,g_value_get_string(&full_path));
	GtkTreeIter **iterPointerPointer=calloc(1, sizeof(GtkTreeIter**));//This will be passed into childAttribs, which will be freed in g_spawn_wrapper. but we shall not free iter, so i use pointer to pointer here.
	*iterPointerPointer=iter;
	if(!g_spawn_wrapper(git_latest_log_cmd, file_list, G_SPAWN_DEFAULT, NULL, NULL, FALSE, readLatestGitLogForFileAndUpdateStore, iterPointerPointer,TRUE)){
	  //if false returned here, g_warning would have been called in g_spawn_wrapper, so i write no log entry here
	}
	g_list_free(file_list);
      }
}
#endif


static void Update_Store_ExtColumns(RFM_ChildAttribs *childAttribs) {
   gchar * value=childAttribs->stdOut;
   value[strcspn(value, "\n")] = 0;
   g_log(RFM_LOG_DATA_EXT,G_LOG_LEVEL_DEBUG,"ExtColumn Value:%s",value);//TODO: add store column name?
   RFM_store_cell* cell= *(RFM_store_cell**)(childAttribs->customCallbackUserData);
   if (cell->store_column>=0 && cell->store_column<NUM_COLS)  gtk_list_store_set(store,cell->iter, cell->store_column, value, -1);
   if (g_strcmp0(value,"")!=0){
     if (cell->iconview_markup) {
       gchar * markup_column_value_escaped=g_markup_escape_text(value, -1);
       gtk_list_store_set(store, cell->iter, COL_ICONVIEW_MARKUP, markup_column_value_escaped, -1);
       g_free(markup_column_value_escaped);
     }
     if (cell->iconview_tooltip) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_TOOLTIP, value, -1);
   }
   g_free(cell);
}

static void load_ExtColumns_and_iconview_markup_tooltip(RFM_FileAttributes* fileAttributes, GtkTreeIter *iter){
      g_log(RFM_LOG_DATA_EXT,G_LOG_LEVEL_DEBUG,"load_ExtColumns for %s",fileAttributes->path);
      for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
	if ((TREEVIEW_COLUMNS[i].Show || TREEVIEW_COLUMNS[i].iconview_markup || TREEVIEW_COLUMNS[i].iconview_tooltip)
	    && (g_strcmp0(TREEVIEW_COLUMNS[i].MIME_root, "*")==0 || g_strcmp0(fileAttributes->mime_root, TREEVIEW_COLUMNS[i].MIME_root)==0) //treeviewColumns[i].MIME_root 为*, 或者和当前文件相同
	    && (TREEVIEW_COLUMNS[i].showCondition==NULL || TREEVIEW_COLUMNS[i].showCondition(fileAttributes))
	    && TREEVIEW_COLUMNS[i].enumCol!=NUM_COLS
	    ){
	  //下面的if语句为啥不合并到上面的if,用&&连起来?
	  if (g_strcmp0(TREEVIEW_COLUMNS[i].MIME_sub, "*") || g_strcmp0(TREEVIEW_COLUMNS[i].MIME_sub, fileAttributes->mime_sub_type)){ //treeviewColumns[i].MIME_sub 为*, 或者和当前文件相同
	    if (TREEVIEW_COLUMNS[i].filenameSuffix && !g_str_has_suffix(fileAttributes->path, TREEVIEW_COLUMNS[i].filenameSuffix)) continue;//如果filenameSuffix不为NULL,则需匹配
	    
	    RFM_store_cell* cell=malloc(sizeof(RFM_store_cell));
	    cell->iter = iter;
	    cell->store_column = TREEVIEW_COLUMNS[i].enumCol;
	    cell->iconview_markup = TREEVIEW_COLUMNS[i].iconview_markup;
	    cell->iconview_tooltip = TREEVIEW_COLUMNS[i].iconview_tooltip;
	    //虽然所有的列都出现在treeviewColumns数组里,但只有满足如下条件之一的才会被load
	    if (TREEVIEW_COLUMNS[i].ValueCmd!=NULL){
              gchar* ExtColumn_cmd = g_strdup_printf(TREEVIEW_COLUMNS[i].ValueCmd, fileAttributes->path);
	      gchar* ExtColumn_cmd_template[] = {"/bin/bash", "-c", ExtColumn_cmd, NULL};
	      //TODO: 列表视图里自定义列的ValueCmd目前还只是用 bash 解析，将来也许可以考虑扩展成指定 cmd_interpretor , 比如用 AI 来执行
	      g_spawn_wrapper(ExtColumn_cmd_template, NULL, G_SPAWN_DEFAULT, NULL, NULL, FALSE, Update_Store_ExtColumns, &cell, TRUE);
	    }else if (TREEVIEW_COLUMNS[i].ValueFunc==getExtColumnValueFromHashTable){ //TODO: TreeView中只显示第一行不应该绑定在getExtColumnValueFromHashTable函数上，可以考虑给Column加个属性，但这样一来config.h中Column属性又要变宽了，所以可以考虑定义一个getExtColumnFirstLineValueFromHashTable函数用在这里，然后在定制代码中决定是否用这个函数
	      //ExtColumnValueInHashTable 可能包含多行字符串，\n分割，这里，用第一行更新store. TODO:也序定制 gtkCellRenderer或实现一个自定义的Renderer更好。
	      gchar *ExtColumnValueInHashTable=TREEVIEW_COLUMNS[i].ValueFunc(fileAttributes,TREEVIEW_COLUMNS[i].enumCol-COL_Ext1);
	      if (ExtColumnValueInHashTable){
		ExtColumnValueInHashTable = strdup(ExtColumnValueInHashTable);
		char *firstline = strtok(ExtColumnValueInHashTable, "\n");
		if (firstline) gtk_list_store_set(store,cell->iter, cell->store_column, firstline, -1);
		else gtk_list_store_set(store,cell->iter, cell->store_column, ExtColumnValueInHashTable, -1);
		g_free(ExtColumnValueInHashTable);
	      }
	    }else if (TREEVIEW_COLUMNS[i].ValueFunc!=NULL){
	      gtk_list_store_set(store,cell->iter, cell->store_column, TREEVIEW_COLUMNS[i].ValueFunc(fileAttributes, TREEVIEW_COLUMNS[i].enumCol-COL_Ext1), -1);
	      //for grepMatch column, fileAttributes->file_name is added as key into grepMatch_hash, however, we suppose grep output absoluteaddr, so filenamne equals fileattributes->path here
	    }else if ((cell->iconview_markup) || (cell->iconview_tooltip)){
	      GValue enumColValue = G_VALUE_INIT;
	      gtk_tree_model_get_value(treemodel, iter, TREEVIEW_COLUMNS[i].enumCol, &enumColValue);;
	      if (cell->iconview_markup) {
		gchar * markup_column_value=g_value_get_string(&enumColValue);
		gchar * markup_column_value_escaped=g_markup_escape_text(markup_column_value, -1);
		gtk_list_store_set(store, cell->iter, COL_ICONVIEW_MARKUP, markup_column_value_escaped, -1);
		g_free(markup_column_value_escaped);
		g_free(markup_column_value);
	      }
	      if (cell->iconview_tooltip) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_TOOLTIP, &enumColValue, -1);
	    }
	  }//endif treeviewColumns[i].MIME_sub 为*, 或者和当前文件相同
	}
      }  
}

//called in dir view when onebyone off or in searchresultview
static void Iterate_through_fileAttribute_list_to_insert_into_store()
{
   GList *listElement;
   RFM_FileAttributes *fileAttributes;
   GtkTreeIter iter;
   if (rfm_fileAttributeList==NULL) return;
   listElement=g_list_first(rfm_fileAttributeList);
   while (listElement != NULL) {
      fileAttributes=(RFM_FileAttributes*)listElement->data;
      Insert_fileAttributes_into_store_and_set_GtkTreeIter(fileAttributes, &iter);
      load_ExtColumnHashTables_for_all_Dimensions_for_GtkTreeIter(&iter);
      load_ExtColumns_and_iconview_markup_tooltip(fileAttributes, &iter);
   listElement=g_list_next(listElement);
   }
}

// fileAttributes is the collection of attributes of one file
static void Insert_fileAttributes_into_store_and_set_GtkTreeIter(RFM_FileAttributes *fileAttributes, GtkTreeIter *iter)
{
      GtkTreePath *treePath=NULL;
      GdkPixbuf *theme_pixbuf=NULL;
      //RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");

      if (fileAttributes->icon_name!=NULL) {
         /* Fall back to generic icon if possible: GTK_ICON_LOOKUP_GENERIC_FALLBACK doesn't always work, e.g. flac files */
         if (!gtk_icon_theme_has_icon(icon_theme, fileAttributes->icon_name)) {
            g_free(fileAttributes->icon_name);
            fileAttributes->icon_name=g_strjoin("-", fileAttributes->mime_root, "x-generic", NULL);
         }

         theme_pixbuf=gtk_icon_theme_load_icon(icon_theme, fileAttributes->icon_name, RFM_ICON_SIZE, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
         if (theme_pixbuf!=NULL) {
            g_object_unref(fileAttributes->pixbuf);
            if (fileAttributes->is_symlink) {
               fileAttributes->pixbuf=gdk_pixbuf_copy(theme_pixbuf);
               gdk_pixbuf_composite(defaultPixbufs->symlink, fileAttributes->pixbuf, 0,0, gdk_pixbuf_get_width(defaultPixbufs->symlink), gdk_pixbuf_get_height(defaultPixbufs->symlink), 0, 0, 1, 1, GDK_INTERP_NEAREST, 200);
            }
            else
               fileAttributes->pixbuf=g_object_ref(theme_pixbuf);
            g_object_unref(theme_pixbuf);
         }
      }

#ifdef GitIntegration
      gchar * gitStatus=g_strdup(g_hash_table_lookup(gitTrackedFiles, fileAttributes->path));
      //gchar * git_commit_msg=g_hash_table_lookup(gitCommitMsg,fileAttributes->path);
#endif
      fileAttributes->mime_sort=g_strjoin(NULL,fileAttributes->mime_root,fileAttributes->mime_sub_type,NULL);
      fileAttributes->mtime= fileAttributes->file_mtime==NULL ? NULL : g_date_time_format(fileAttributes->file_mtime,RFM_DATETIME_FORMAT);
      fileAttributes->atime= fileAttributes->file_atime==NULL ? NULL : g_date_time_format(fileAttributes->file_atime,RFM_DATETIME_FORMAT);
      fileAttributes->ctime= fileAttributes->file_ctime==NULL ? NULL : g_date_time_format(fileAttributes->file_ctime,RFM_DATETIME_FORMAT);
      fileAttributes->file_name_escaped_for_iconview_markup_column = g_markup_escape_text(fileAttributes->file_name, -1);
      gtk_list_store_insert_with_values(store, iter, -1,
                          COL_MODE_STR, fileAttributes->file_mode_str,
					//COL_DISPLAY_NAME, fileAttributes->display_name,
			  COL_FILENAME,fileAttributes->file_name,
			  COL_FULL_PATH,fileAttributes->path,
                          COL_PIXBUF, fileAttributes->pixbuf,
			  COL_MTIME, fileAttributes->file_mtime==NULL? 0 : g_date_time_to_unix(fileAttributes->file_mtime),
			  COL_MTIME_STR,fileAttributes->mtime,
             		  COL_SIZE,fileAttributes->file_size,
			  COL_SIZE_DISPLAY,g_format_size_full(fileAttributes->file_size, G_FORMAT_SIZE_LONG_FORMAT),
                          COL_ATTR, fileAttributes,
                          COL_OWNER,fileAttributes->owner,
			  COL_GROUP,fileAttributes->group,
                          COL_MIME_ROOT,fileAttributes->mime_root,
			  COL_MIME_SUB,fileAttributes->mime_sub_type,
			  COL_LINK_TARGET,fileAttributes->link_target_filename,
			  COL_ATIME_STR,fileAttributes->atime,
                          COL_CTIME_STR,fileAttributes->ctime,
#ifdef GitIntegration
                          COL_GIT_STATUS_STR,gitStatus,
					//COL_GIT_COMMIT_MSG,git_commit_msg,
#endif
			  COL_MIME_SORT,fileAttributes->mime_sort,
			  COL_ICONVIEW_MARKUP, fileAttributes->file_name_escaped_for_iconview_markup_column,
			  COL_ICONVIEW_TOOLTIP,NULL,
                          -1);

      g_log(RFM_LOG_DATA,G_LOG_LEVEL_DEBUG,"Inserted into store:%s",fileAttributes->file_name);
      
      if (keep_selection_on_view_across_refresh) {
	GList * selection_filepath_list = g_list_first(filepath_lists_for_selection_on_view_clone);
	while(selection_filepath_list!=NULL){
	  if (g_strcmp0(fileAttributes->path, selection_filepath_list->data)==0){
	    g_debug("re-select file during refresh:%s",fileAttributes->path);
	    treePath=gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	    set_view_selection(icon_or_tree_view, treeview, treePath);
	    gtk_tree_path_free(treePath);
	    //once item in filepath_lists_for_selection_on_view_clone matches and set_view_selection on view, it is removed from the list so that it will not be in the loop of comparison for the next file to Insert into store. This will decrease the total number of comparison and improve performance, but we have to re-clone the list at the begin of refresh.
	    filepath_lists_for_selection_on_view_clone = g_list_remove_link(filepath_lists_for_selection_on_view_clone, selection_filepath_list);
	    g_list_free_full(selection_filepath_list,(GDestroyNotify)g_free);
	    break;
	  }
	  selection_filepath_list = g_list_next(selection_filepath_list);
	}
     }
}


#ifdef GitIntegration
static void readLatestGitLogForFileAndUpdateStore(RFM_ChildAttribs * childAttribs){
   gchar * cmdOutput=childAttribs->stdOut;
   if (strlen(cmdOutput)<=7) return;
   gchar * commitid=cmdOutput+7; //"commit " takes 7 char
   size_t commitid_end_offset=strcspn(commitid, "\n");
   gchar * Author=commitid + commitid_end_offset + 8; //"Auther: " takes 8 char
   size_t Author_end_offset=strcspn(Author, "\n");
   gchar * Date=Author + Author_end_offset + 8; // "Date:   " takes 8 char
   size_t Date_end_offset=strcspn(Date, "\n");
   gchar * commitMsg=Date + Date_end_offset + 1; // "\n"  takes 2 char

   GtkTreeIter *iter= *(GtkTreeIter**)(childAttribs->customCallbackUserData);

   gtk_list_store_set(store,iter,COL_GIT_COMMIT_MSG, commitMsg, -1);
   *(Date + Date_end_offset)=0;
   gtk_list_store_set(store,iter,COL_GIT_COMMIT_DATE, Date, -1);
   *(Author + Author_end_offset)=0;
   gtk_list_store_set(store,iter,COL_GIT_AUTHOR, Author, -1);
   *(commitid + commitid_end_offset)=0;
   gtk_list_store_set(store,iter,COL_GIT_COMMIT_ID, commitid, -1);
   
   g_log(RFM_LOG_DATA_GIT,G_LOG_LEVEL_DEBUG,"gitCommitid:%s",commitid);
}

//git本身只会给文件设置状态，不考虑目录，这里根据目录中文件的状态为目录也设置一个状态，方便了解哪些子目录有文件变动，需要stage等
//filename为git ls-files输出格式
static void GitStatusForDirectory(gchar * filename, const char* status) {
      gchar *savedptr1=NULL;
      if (ignored_filename(basename(filename))) return;//防止子目录里添加的隐藏文件影响目录的gitstatus列，引起误解
      //TODO:如果文件本身不是隐藏文件，但是位于一个隐藏的子目录，怎么办?得识别路径上各层目录有无隐藏，不过这种情况我这里暂时没遇到
      gchar *status_=g_strdup(status);
      gchar *dirname=strtok_r(filename,"/",&savedptr1);//TODO:这里找第一个/符号，实际上找的是当前目录下第一层子目录，再下层的子目录不会显示在当前目录视图里，也就没必要更新gitstatus列了。这里主要是考虑目录视图，搜索结果视图(如git ls-files>0)一般无需显示目录，这里暂不考虑，如果将来有需求，需要找第二，第三个/符号,更新各级目录
      if (dirname!=NULL){
	gchar *dir_fullpath=curPathRelativeToGitRoot ? g_build_filename(git_root,curPathRelativeToGitRoot,dirname,NULL) : g_build_filename(git_root,dirname,NULL);
	gchar* existingStatus=g_hash_table_lookup(gitTrackedFiles, dir_fullpath);
	if (existingStatus){
          for (int i = 0; i <= 1; i++) {
            if (existingStatus[i] != status_[i]) { // 相同就不用调整了
              if (existingStatus[i] == ' ') continue; // 任何字符都覆盖空格，也就是说status这一位不用调整
              if (status_[i] == ' ') { status_[i] = existingStatus[i];continue;} // 如果新的status为空格，则保留existing status, 这一种情况应该不会发生，因为我们是先读取git tracked file,也就是先读取空格status,后续读取的status应该都不会是空格
	      if (i==0){  // git命令输出的??状态实际上应该理解为" ?"更合理，所以"??"+"M " 应该得出"M?" 而不是 "*?" 更合理
		  if (existingStatus[i]=='?') continue; // 任何字符覆盖第一位?
		  if (status_[i]=='?') { status_[i] = existingStatus[i]; continue; } //第一位?不改变任何现有字符
	      }
              status_[i] = '*'; // 不同且不为空，就设为"*"
            }
          }
	  g_hash_table_replace(gitTrackedFiles, dir_fullpath, status_);
        }else g_hash_table_insert(gitTrackedFiles, dir_fullpath, status_);

	insert_deleted_for_git(g_strdup(dir_fullpath), dirname, status, "inode", "directory");
        g_log(RFM_LOG_DATA_GIT,G_LOG_LEVEL_DEBUG,"gitTrackedFile dir:%s, status:%s, status_:%s,%s",dirname, status, status_,dir_fullpath);
      }
}

static void read_git_ls_files_cmd_output_into_hashtable(gpointer *child_attribs) {
  char *child_stdout=(char*)(((RFM_ChildAttribs *)child_attribs)->stdOut);
  char *status=(char*)(((RFM_ChildAttribs *)child_attribs)->customCallbackUserData);
  gchar * savedptr=NULL;
  gchar * oneline=strtok_r(child_stdout,"\n",&savedptr);
  while (oneline!=NULL){
      gchar * fullpath = curPathRelativeToGitRoot? g_build_filename(git_root,curPathRelativeToGitRoot, oneline,NULL) : g_build_filename(git_root,oneline,NULL);
      g_hash_table_insert(gitTrackedFiles,fullpath,g_strdup(status));
      g_log(RFM_LOG_DATA_GIT,G_LOG_LEVEL_DEBUG,"gitTrackedFile status:%s,%s",status,fullpath);
      GitStatusForDirectory(oneline,status);
      oneline=strtok_r(NULL, "\n",&savedptr);
  }
}

static void load_gitTrackedFiles_HashTable()
{
  g_spawn_wrapper(git_ls_files_cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, FALSE, read_git_ls_files_cmd_output_into_hashtable, strdup("  "), TRUE);

  g_spawn_wrapper(git_ls_files_untracked_cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, FALSE, read_git_ls_files_cmd_output_into_hashtable, strdup("??"), TRUE);

  g_spawn_wrapper(git_modified_staged_info_cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, FALSE, read_git_status_porcelain_cmd_output_into_hashtable,NULL, TRUE);
}

static void read_git_status_porcelain_cmd_output_into_hashtable(gpointer *child_attribs){
    char *child_stdout=(char*)(((RFM_ChildAttribs *)child_attribs)->stdOut);
    gchar *savedptr=NULL;
    gchar *oneline=strtok_r(child_stdout,"\n",&savedptr);
    while (oneline!=NULL && strlen(oneline)>3){
      //first 2 char of oneline is git status
      gchar *status=g_strndup(oneline, 2);
      //copy rest of oneline into filename
      gchar *beginfilename = oneline + 3;
      //if you run git mv filename1 filename2
      //git status --porcelain returns
      //R  filename1 -> filename2
      //that is, there are two filenames in oneline, we need to split it
      //and insert into gitTrackedfiles two lines:
      //D filename1
      //A filename2
      gchar** filenames=calloc(sizeof(gchar*), 2);
      if (g_strcmp0(status, "R ")==0){
	filenames = g_strsplit(beginfilename, " -> ", 2);
      }else{
	filenames[0]=g_strndup(beginfilename,strlen(oneline)-3);
	filenames[1]=NULL;
      }

      for (int i=0;i<=1 && filenames[i];i++){ //如果 filenames[1]==NULL, 循环一次,否则循环两次
	if (filenames[1]){//上面 status 为"R "的情况
	  if (i==0) sprintf(status,"D ");
	  else status=strdup("A ");
	}
	if (!SearchResultViewInsteadOfDirectoryView && curPathRelativeToGitRoot && !g_str_has_prefix(filenames[i],curPathRelativeToGitRoot)) continue;//只处理当前目录树
      //TODO: remove ending "/"
      //when i mkdir test in gitreporoot
      //cd test
      //touch testfile.md
      //i can see ?? test/ in git status --porcelain result
      //but the same file path in fileattributes does not have ending /, so , they won't match
      //BTW, git status --porcelain don't have something like ?? test/testfile.md, is it a git issue?
      gchar *fullpath=g_build_filename(git_root,filenames[i],NULL);//TODO:感觉这个fullpath没释放啊
      g_log(RFM_LOG_DATA_GIT,G_LOG_LEVEL_DEBUG,"gitTrackedFile Status:%s,%s",status,fullpath);

      gchar* existingStatus=g_hash_table_lookup(gitTrackedFiles, fullpath);
      //git status --porcelain return two lines for a single file if both staged and unstaged changes included. But we use the fullpath as hashtable key here, so i have to merge these two lines into single line and combine the status.
      // TODO: this may also considered issue of git status instead of mine, but i provide a quick fix here.
      if (existingStatus!=NULL){
	// "D "+"??"="D?"
	if (g_strcmp0(status, "??")==0 && g_strcmp0(existingStatus, "D ")==0){
	  status[0]='D';
	}else if (g_strcmp0(status, "D ")==0 && g_strcmp0(existingStatus, "??")==0){
	  status[1]='?';
	}
	g_hash_table_replace(gitTrackedFiles, g_strdup(fullpath), status);
	//TODO: can there be something like "A "+" D"="AD"?
      }else g_hash_table_insert(gitTrackedFiles,g_strdup(fullpath),status);

      insert_deleted_for_git(fullpath, filenames[i], status, "na", "na");
      GitStatusForDirectory(curPathRelativeToGitRoot?(filenames[i]+strlen(curPathRelativeToGitRoot)):filenames[i], status);//git status --porcelain 输出的文件名格式同git ls-files --full-name， 所以这里要去掉curPathRelativeToGitRoot前缀。并且上面代码已经过滤掉了输出里面不包含curPathRelativeToGitRoot前缀的路径了
      }
      
      g_strfreev(filenames);
      oneline=strtok_r(NULL, "\n",&savedptr);
    }
}

static void insert_deleted_for_git(gchar *fullpath, gchar *filename, gchar *status, gchar *mime_root, gchar *mime_sub) {
      struct stat statbuff;
      if ((SearchResultViewInsteadOfDirectoryView^1)
	  && (status[1]=='D' || status[0]=='D')
	  && stat(fullpath, &statbuff)!=0 //file not exists
	  && g_strcmp0(g_path_get_dirname(fullpath), rfm_curPath)==0){
	//add item into fileattributelist so that user can git stage on it
	RFM_FileAttributes *fileAttributes=malloc_fileAttributes();
	if (fileAttributes!=NULL){//if the file is rfm ignord file, we still add it into display here,but this may be changed.
	  fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
	  fileAttributes->file_name=g_strdup(filename);
	  //fileAttributes->display_name=g_strdup(filename);
	  fileAttributes->path=fullpath;
	  fileAttributes->mime_root=g_strdup(mime_root);
	  fileAttributes->mime_sub_type=g_strdup(mime_sub);
	  fileAttributes->is_dir=(strcmp(fileAttributes->mime_root, "inode")==0 && strcmp(fileAttributes->mime_sub_type, "directory")==0);

	  if (fileAttributes->is_dir){
	    if (g_hash_table_lookup(gitDeletedDirs, fullpath)){
	      g_hash_table_replace(gitDeletedDirs, g_strdup(fullpath), g_strdup(status));
	      return; //防止下面重复往 rfm_fileAttributelist 里插入
	    }else{
	      g_hash_table_insert(gitDeletedDirs, g_strdup(fullpath), g_strdup(status));
	    }
	  }
	  //rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	  if (insert_fileAttributes_into_store_one_by_one) DirectoryView_onebyon_On_Insert_fileAttributes_into_store_with_thumbnail_and_more(fileAttributes);
	  else rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	  //TODO: after i run `git rm -r --cached video`, rfm will show video subdir as shown on devPicAndVideo/20231210_15h14m08s_grim.png
	  //so we should not insert fileattributes if the file is still in directory, and it is just removed from git track, not deleted
	  //or we can insert fileattributes here, but update it when read fileattribute later from directory, instead of insert it again.
	  //Furthur, think about we need to load extended attribs for files, such as those in email, shall we consider general multiple pass fileattributes loading framework?
	}
      }
}
#endif

static void DirectoryView_onebyon_On_Insert_fileAttributes_into_store_with_thumbnail_and_more(RFM_FileAttributes* fileAttributes){
	    GtkTreeIter iter;
            rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	    Insert_fileAttributes_into_store_and_set_GtkTreeIter(fileAttributes,&iter);
	    load_ExtColumnHashTables_for_all_Dimensions_for_GtkTreeIter(&iter);
	    load_ExtColumns_and_iconview_markup_tooltip(fileAttributes, &iter);
	    if (rfm_do_thumbs==1 && g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR)){
	      load_thumbnail_or_enqueue_thumbQueue_for_store_row(&iter);
#ifdef GitIntegration
              if (get_treeviewColumnByEnum(COL_GIT_COMMIT_MSG)->Show ||get_treeviewColumnByEnum(COL_GIT_COMMIT_ID)->Show || get_treeviewColumnByEnum(COL_GIT_COMMIT_DATE)->Show || get_treeviewColumnByEnum(COL_GIT_AUTHOR)->Show) load_latest_git_log_for_store_row(&iter);
#endif
	    }
}

static gboolean read_one_DirItem_into_fileAttributeList_and_insert_into_store_if_onebyone(GDir *dir) {
   const gchar *name=NULL;
   time_t mtimeThreshold=time(NULL)-RFM_MTIME_OFFSET;
   RFM_FileAttributes *fileAttributes;
   GHashTable *mount_hash=get_mount_points();

   name=g_dir_read_name(dir);
   if (name!=NULL) {
     g_log(RFM_LOG_DATA, G_LOG_LEVEL_DEBUG, "read_one_file_in_g_idle_start, counter:%d", read_one_file_couter);
     if (!ignored_filename(name)) {
         fileAttributes=get_fileAttributes_for_a_file(name, mtimeThreshold, mount_hash);
	 fileAttributeID++;//fileAttributeID 目的是为了应对查询结果里面同一个文件返回多条记录的情况,如不同行,在当前目录视图里面是没多大用的,但我们还是累加下
         if (fileAttributes!=NULL){
	   if (insert_fileAttributes_into_store_one_by_one) DirectoryView_onebyon_On_Insert_fileAttributes_into_store_with_thumbnail_and_more(fileAttributes);
	   else rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	 }
      }
      g_hash_table_destroy(mount_hash);
      g_log(RFM_LOG_DATA, G_LOG_LEVEL_DEBUG, "read_one_file_in_g_idle_end, counter:%d", read_one_file_couter);
      read_one_file_couter++;
      return G_SOURCE_CONTINUE;   /* Return TRUE if more items */
   }
   else if (insert_fileAttributes_into_store_one_by_one) {
       if (rfm_thumbQueue && mkThumb_thread==NULL) mkThumb_thread = g_thread_new("mkThumbnail", mkThumbLoop, NULL);
       In_refresh_store = FALSE;
       if (!no_window) gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
   }

   rfm_readDirSheduler=0;
   g_hash_table_destroy(mount_hash);
   return G_SOURCE_REMOVE;
}

static void TurnPage(gboolean next) {
  GList *name =CurrentPage_SearchResultView;
  gint i=0;
  while (name != NULL && i < PageSize_SearchResultView) {
    if (next)
      name=g_list_next(name);
    else
      name=g_list_previous(name);
    i++;
  }
  if (name != NULL && name!=CurrentPage_SearchResultView) {
    CurrentPage_SearchResultView = name;
    if (next) currentFileNum=currentFileNum + PageSize_SearchResultView;
    else currentFileNum=currentFileNum - PageSize_SearchResultView;
    refresh_store();
  }
}

static void NextPage() { TurnPage(TRUE); }

static void PreviousPage() { TurnPage(FALSE); }

static void FirstPage(){
  CurrentPage_SearchResultView = SearchResultFileNameList;
  currentFileNum = 1;
  refresh_store();
}

static void set_pagesize_for_searchresultview(const char *pagesize){
  if (pagesize){
    int ps = atoi(pagesize);
    if (ps>0){
      PageSize_SearchResultView = ps;
      setenv("RFM_PAGESIZE", pagesize, 1);
      if (SearchResultViewInsteadOfDirectoryView && SearchResultFileNameList!=NULL) FirstPage();
    }else g_warning("invalid parameter:%s",pagesize);
  }
}


static gboolean fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store(RFM_ChildAttribs *childAttribs) {
  time_t mtimeThreshold=time(NULL)-RFM_MTIME_OFFSET;
  RFM_FileAttributes *fileAttributes;
  GHashTable *mount_hash=get_mount_points();

  gint i=0;
  GList *name=CurrentPage_SearchResultView;
  while (name!=NULL && i<PageSize_SearchResultView){
    if (fileAttributeID==1 && FileNameIndexInColumnTitles) fileAttributeID++;
    
    fileAttributes = get_fileAttributes_for_a_file(name->data, mtimeThreshold, mount_hash);
    if (fileAttributes != NULL) {
      rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
      g_log(RFM_LOG_DATA,G_LOG_LEVEL_DEBUG,"appended into rfm_fileAttributeList:%s", (char*)name->data);
      if (SearchResultViewInsteadOfDirectoryView && SearchResultTypePointerForCurrentExistingSearchResult)//本函数仅在refresh_store()searchresultview分支调用，因此判断条件中SearchResultViewInsteadOfDirectoryView其实可以不要
	SearchResultTypePointerForCurrentExistingSearchResult->SearchResultLineProcessingFunc((gchar*)(name->data), FALSE, SearchResultTypePointerForCurrentExistingSearchResult->cmdTemplate, fileAttributes, childAttribs);
    }
    fileAttributeID++;//上面SearchResultLineProcessingFunc里面会使用fileAttributeID作为hashtable的key, commit ab8f8c2f832bc32b431ced50bda21de36b9314d5 之前是分两次从fileAttributeID=1开始遍历文件名列表的,但commit ab8f8c2f832bc32b431ced50bda21de36b9314d5 和并成了现在的一次遍历, 造成了一个bug: 之前在 get_fileAttributes_for_a_file 调用fileAttributeID++, 后面的SearchResultLineProcessingFunc里fileAttributeID值就错了,所以现在改在这里调用 fileAttributeID++
    name=g_list_next(name);
    i++;
  }
  rfm_fileAttributeList=g_list_reverse(rfm_fileAttributeList);

  if (SearchResultTypePointerForCurrentExistingSearchResult && !showSearchResultExtColumnsBasedOnHashTableValues_has_already_been_called_for_searchresult) showSearchResultExtColumnsBasedOnHashTableValues();

  icon_or_tree_view = add_view();//add view have to be called after showSearchResultExtColumnsBasedOnHashTableValues()
  //commit ab8f8c2f832bc32b431ced50bda21de36b9314d5 里, 我发现 add_view 需要在showSearchResultExtColumnsBasedOnHashTableValues()后面调用,就在那个commit 里面移动了add_view 的位置,否则会出现一个问题:运行 locate .rfmTODO.md>TODO.md 后,只有filename列显示,需要连续回车刷新下才会显示ext列. 究其原因,可能和第 3334 行代码有关, 也就是说,在调用 show_hide_treeview_column 时,需要 icon_or_tree_view值有效,否则gtkcolumn不会被设置成显示
  //但commit ab8f8c2f832bc32b431ced50bda21de36b9314d5 造成了新的问题, Iterate_through_fileAttribute_list_to_insert_into_store() 调用会自动选择刷新前选中的行,这功能需要 icon_or_tree_view 变量值有效,也就是add_view 要发生在Iterate_through_fileAttribute_list_to_insert_into_store()调用前
  gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), current_sort_column_id, current_sorttype);

  load_AllDimensionValues_for_DimensionColumns();
  //We can load thumbnail and gitCommitMsg one by one right after one row of store inserted, as we do in read_one_DirItem_into_fileAttributeList_and_insert_into_store_in_each_cal. But since for pipeline, we have the paging mechanism, we won't have too much rows loaded in a batch, so, we keep the old way of loading thumbnails and gitCommitMsg in batch here. 
  Iterate_through_fileAttribute_list_to_insert_into_store();
  if (rfm_do_thumbs == 1 && g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR))
    SearchResultView_iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration();

  g_hash_table_destroy(mount_hash);
  return TRUE;
}

/* store hold references to rfm_fileAttributeList: these two must be freed together */
static void clear_store(void)
{
   g_hash_table_remove_all(thumb_hash);
   gtk_list_store_clear(store); /* This will g_free and g_object_unref */
   ItemSelected=0;
#ifdef GitIntegration
   g_hash_table_remove_all(gitTrackedFiles);
   g_hash_table_remove_all(gitDeletedDirs);
  //g_hash_table_remove_all(gitCommitMsg);
#endif
   g_list_free_full(rfm_fileAttributeList, (GDestroyNotify)free_fileAttributes);
   rfm_fileAttributeList=NULL;
}

static gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
   /* Sort directories first, then on-disk filename */
   RFM_FileAttributes *fileAttributesA, *fileAttributesB;
   int rv;

   gtk_tree_model_get(model, a, COL_ATTR, &fileAttributesA, -1);
   gtk_tree_model_get(model, b, COL_ATTR, &fileAttributesB, -1);
   

   if (!fileAttributesA->is_dir && fileAttributesB->is_dir) rv=1;
   else if (fileAttributesA->is_dir && !fileAttributesB->is_dir) rv=-1;
   else  rv=strcmp(fileAttributesA->file_name, fileAttributesB->file_name);

   g_log(RFM_LOG_DATA_SORT, G_LOG_LEVEL_DEBUG, (rv>0? "%s > %s": (rv==0? "%s == %s" : "%s < %s")), fileAttributesA->file_name, fileAttributesB->file_name);
   return rv;
}

static void set_Titles(CALLER_OWN const gchar * title){
   if (title==NULL || no_window) return;
   gboolean title_is_rfm_curPath = (title[0]=='/');
   if (title_is_rfm_curPath) gtk_window_set_title(GTK_WINDOW(window), title);
   gtk_tool_button_set_label(GTK_TOOL_BUTTON(PathAndRepositoryNameDisplay), title);
   if (title_is_rfm_curPath && startWithTerm() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)) set_terminal_window_title(title);
}


static void refresh_store()
{
   In_refresh_store = TRUE;
   rfm_stop_all();

   /* if (icon_or_tree_view!=NULL && viewSelectionChangedSignalConnection!=0) { */
   /*   g_signal_handler_disconnect((treeview? gtk_tree_view_get_selection(GTK_TREE_VIEW(icon_or_tree_view)) : icon_or_tree_view), viewSelectionChangedSignalConnection); */
   /*   viewSelectionChangedSignalConnection=0; */
   /* } */
   if (!no_window && keep_selection_on_view_across_refresh){
     //clone filepath_lists_for_selection_on_view
     g_list_free_full(g_list_first(filepath_lists_for_selection_on_view_clone),(GDestroyNotify)g_free);
     filepath_lists_for_selection_on_view_clone = g_list_copy_deep(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_strdup,NULL);
     g_debug("Number of view selection copied:%d",g_list_length(filepath_lists_for_selection_on_view_clone));
     if (RFM_AUTOSELECT_OLDPWD_IN_VIEW && rfm_prePath!=NULL)
       filepath_lists_for_selection_on_view_clone = g_list_prepend(filepath_lists_for_selection_on_view_clone, g_strdup(rfm_prePath));
   }

   if (!no_window){
   gtk_widget_hide(rfm_main_box);
   if (scroll_window) {gtk_widget_destroy(scroll_window); icon_or_tree_view=NULL;} 
   gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, FALSE);
   }

   clear_store();

#ifdef GitIntegration
   g_spawn_wrapper(git_root_cmd, NULL,G_SPAWN_DEFAULT, NULL, NULL, FALSE, get_git_root_with_rev_parse, NULL,TRUE);
   if (git_root) load_gitTrackedFiles_HashTable();
#endif

   refresh_DimensionPropertiesHashTable();

   gchar * title;
   if (SearchResultViewInsteadOfDirectoryView) {
     if (SearchResultTypePointerForCurrentExistingSearchResult){
         for(int i=0;i<=NUM_Ext_Columns;i++) {
	   if (!ExtColumnHashTable_keep_during_refresh[i] && ExtColumnHashTable[i]!=NULL){
	     g_hash_table_destroy(ExtColumnHashTable[i]);//TODO: 这里为啥要把hashtable里值全部销毁然后用newsearch false 参数重新调用SearchResultTypeProcessingFunc呢？这样文件里面的数据更新是刷新了，但查询命令本身并没有被重新执行，也就是文件结果集没有变化。这牵涉到究竟如何定义查询刷新，如果只是UI重建，保留所有的hashtable,也不用重新调用SearchResultTypeProcessingFunc就可以了，但这样数据没有变化的刷新意义何在？ 这个git branch的重构考虑到把Search命令的ChildAttribs,包括Search命令本身保存在SearchResultTypeProcessingContext 里面，如此，就可以在刷新的时候重新执行整个search命令了。但现有的代码，Update_searchresult里面调用了一次SearchProcessingFunc, newsearch 为true, refresh_store 的时候再调用一次， newsearch为false. 将来的完整刷新模式下似乎应该是showSearchResult负责用hasttable的值填充store，而现有的update_searchresult功能应该并入完整刷新。但问题的另一面是，完整的查询刷新意义又何在呢？作为命令重新执行的快捷方式？
	     ExtColumnHashTable[i]=NULL;
	   }
	 }
     }
     fileAttributeID=currentFileNum;
     title=g_strdup_printf(PipeTitle, currentFileNum,SearchResultFileNameListLength,PageSize_SearchResultView);
     fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store(childAttribsForCurrentDisplayingSearchResult);

     In_refresh_store=FALSE;
     if (!no_window) gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
   } else {
     icon_or_tree_view = add_view();//call add_view earlier for directory view so that user won't see a blank window for long.
     if (current_sort_column_id == GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID) current_sort_column_id = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
     gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), sort_func, NULL, NULL);
     gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), current_sort_column_id, current_sorttype);
     fileAttributeID=1;
     GDir *dir=NULL;
     dir=g_dir_open(rfm_curPath, 0, NULL);
     if (!dir) return;
#ifdef GitIntegration
     title=git_current_branch? g_strdup_printf("%s [%s]",rfm_curPath,git_current_branch) : g_strdup(rfm_curPath);
#else
     title=g_strdup(rfm_curPath);
#endif
     load_AllDimensionValues_for_DimensionColumns();
     read_one_file_couter = 0;
     if (insert_fileAttributes_into_store_one_by_one)
       rfm_readDirSheduler=g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)read_one_DirItem_into_fileAttributeList_and_insert_into_store_if_onebyone, dir, (GDestroyNotify)g_dir_close);
     else{
       while (read_one_DirItem_into_fileAttributeList_and_insert_into_store_if_onebyone(dir)!=G_SOURCE_REMOVE){};
       g_dir_close(dir);

       Iterate_through_fileAttribute_list_to_insert_into_store();

#ifdef GitIntegration
       if ((get_treeviewColumnByEnum(COL_GIT_COMMIT_MSG)->Show ||get_treeviewColumnByEnum(COL_GIT_COMMIT_ID)->Show || get_treeviewColumnByEnum(COL_GIT_COMMIT_DATE)->Show || get_treeviewColumnByEnum(COL_GIT_AUTHOR)->Show)
	   && gtk_tree_model_get_iter_first(treemodel, &gitMsg_load_iter)) rfm_gitCommitMsgScheduler=g_idle_add((GSourceFunc)DirectoryView_onebyone_Off_iterate_through_store_to_load_latest_git_log_one_by_one_when_idle, &gitMsg_load_iter);
#endif

       if (rfm_do_thumbs == 1 && g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR) && gtk_tree_model_get_iter_first(treemodel, &thumbnail_load_iter)){
	 //rfm_thumbLoadScheduler=g_idle_add((GSourceFunc)iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_one_by_one_when_idle, &thumbnail_load_iter);
	 rfm_thumbLoadScheduler=1;
	 while(iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_one_by_one_when_idle(&thumbnail_load_iter)){};
       }
     }
  }
   set_Titles(title);
   g_free(title);

   if(!no_window){
   gtk_widget_show_all(window);
   refresh_toolbar();
   }
}

//在目录视图，添加删除文件会有inotify handler促发刷新，在searchresultview才需要作为动作的后续调用刷新
static void refresh_store_on_searchresult(){
  if (SearchResultViewInsteadOfDirectoryView) refresh_store();
}


//echo -en "\033]0;title\a"
static void set_terminal_window_title(CALLER_OWN const char * title)
{
  char cmd_to_set_terminal_title[PATH_MAX];
  snprintf(cmd_to_set_terminal_title,PATH_MAX, "echo -en \"\\033]0;%s\\a\"", title);
  char* cmd[]={ "bash", "-c", cmd_to_set_terminal_title,NULL};
  g_spawn_sync(rfm_curPath, cmd, NULL,
                           G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN |
                               G_SPAWN_CHILD_INHERITS_STDOUT |
                               G_SPAWN_CHILD_INHERITS_STDERR,
	       NULL, NULL, NULL, NULL, NULL, NULL);
}


#ifdef GitIntegration
//TODO:strtok命令会跳过连续的\n,也就是说，如果 git rev-parse --is-inside-work-tree --show-toplevel --abbrev-ref HEAD --show-prefix 4行结果中倒数第三行如果是空行得话数据就会出错，所以要确认 --abbrev-ref HEAD 不返回空
static void get_git_root_with_rev_parse(gpointer *child_attribs)
{
  if (git_root) { g_free(git_root); git_root=NULL; setenv("RFM_GIT_ROOT","/",1);} //RFM_GIT_ROOT 作为find命令的scope使用，当前不是git仓库时，就全系统从根查找
  if (curPathRelativeToGitRoot) {g_free(curPathRelativeToGitRoot); curPathRelativeToGitRoot=NULL;}
  if (git_current_branch) {g_free(git_current_branch); git_current_branch=NULL; }
  char *child_StdOut=((RFM_ChildAttribs *)child_attribs)->stdOut;
  if (child_StdOut){
    char *savedptr=NULL;
    char *firstline=strtok_r(child_StdOut,"\n",&savedptr);
    if(firstline!=NULL && strcmp(firstline, "true")==0){
      char *secondline=strtok_r(NULL,"\n",&savedptr);
      if (secondline!=NULL) {
	git_root=g_strdup(secondline);
	setenv("RFM_GIT_ROOT",git_root,1);
	char *thirdline=strtok_r(NULL,"\n",&savedptr);
	git_current_branch = thirdline? g_strdup(thirdline):NULL;
	char *fourthline=strtok_r(NULL,"\n",&savedptr);
        curPathRelativeToGitRoot=fourthline? g_strdup(fourthline):NULL;
	g_debug("git root:%s, current branch:%s, curPathRelativeToGitRoot:%s",git_root, git_current_branch, curPathRelativeToGitRoot);
      }else g_warning("git_root NULL for git repo");
    }
  }else g_warning("git_root_cmd in config.h return NULL");
}

static gboolean show_gitMenus(RFM_FileAttributes *fileAttributes) {
  return (SearchResultViewInsteadOfDirectoryView ^ 1) && git_root;
}

static gboolean show_gitColumns(RFM_FileAttributes * fileAttributes) {
  return git_root!=NULL;
}

#endif


static void set_rfm_curPath_internal(CALLER_OWN const gchar* path){
#ifdef RFM_AddOn
  find_and_unload_addons_before_leaving_current_directory(path);
#endif
  //https://blog.csdn.net/CaspianSea/article/details/78817778
       if (rfm_curPath != path) {
	 if (rfm_curPath!=NULL){
	   g_free(rfm_prePath);
	   rfm_prePath=rfm_curPath;
	   setenv("OLDPWD", rfm_prePath, 1);
	 }
         rfm_curPath = g_strdup(path);
	 chdir(rfm_curPath);//TODO: i don't know whether we really need this
	 g_setenv("rfm_curPath", rfm_curPath, 1);

         //g_setenv("PWD", rfm_curPath, 1);
	 //TODO: i am not sure wether i shold set PWD env value here too, and remove the builtin command setpwd
   //when we run commands such as echo $PWD, we spawn child process with working directory rfm_curPath, so, even we do not set $PWD, we get result as expected with this kind of commands.
   //so, we should first know the relationship between working directory parameter in g_spawn and PWD env value inheritence before we do anything furthur here.
   //anyway, setting OLDPWD is necessary for us here.
#ifdef PythonEmbedded
	 change_ftp_server_root(rfm_curPath);
#endif
#ifdef RFM_AddOn
	 if (!disable_addon){
	   gchar * addon_path = g_strdup(rfm_curPath);
	   gboolean is_root_dir;
	   do {
	     if (access(addon_path, R_OK) == 0); load_addon_dir(addon_path);
	     gchar *addon_path_parent = g_path_get_dirname(addon_path);
	     is_root_dir = (g_strcmp0(addon_path, addon_path_parent)==0); // g_path_get_dirname("/") return "/"
	     g_free(addon_path); addon_path = addon_path_parent;
	   }while (!is_root_dir);
	   g_free(addon_path);
	 }
#endif
      }
}

/* caller owns path, free(rfm_prePath), rfm_curPath assigned to rfm_prePath; strdup(path) will be assigned to rfm_curPath,*/
static void set_rfm_curPath(CALLER_OWN gchar* path)
{
   char *msg;
   gchar * realpath;
   int rfm_new_wd;
   //int e;

   if (path==NULL) return;
   if (g_strcmp0(path, ">")==0){
     if (rfm_path_history_current && rfm_path_history_current->prev){
       rfm_path_history_current = g_list_previous(rfm_path_history_current);
       path = rfm_path_history_current->data;
     }else return;
   }else if (g_strcmp0(path, "<")==0){
     if (rfm_path_history_current && rfm_path_history_current->next){
       rfm_path_history_current = g_list_next(rfm_path_history_current);
       path = rfm_path_history_current->data;
     }else return;
   }else{
     if (rfm_path_history_current) rfm_path_history_current = g_list_first(rfm_path_history_current);
     rfm_path_history_current = g_list_prepend(rfm_path_history_current, g_strdup(path));
     
#ifdef GNU_readline
   add_history(g_strconcat("cd ", path, NULL));
   add_history_timestamp();
#endif
   history_entry_added++;
   //TODO: i want to add this path into ~/.rfm_historyDirectory file as well as current history list, so that it can be use by 'read -e' in bash scripts. How?
   //TODO: in future if we need some directory specific history file, we may check that if the directory contains .rfm_history file, it will use this local history, otherwise, use the default global history file.
   /* if (rfm_curPath!=NULL && (e=append_history(history_entry_added, g_build_filename(rfm_curPath,".rfm_history", NULL)))) */
   /*     g_warning("failed to append_history(%d,%s) error code:%d",history_entry_added,g_build_filename(rfm_curPath,".rfm_history", NULL),e); */
     }


     if (UseTargetAddressWhenEnterSymbloicLinkForDir) realpath=canonicalize_file_name(path);
     else realpath = path;
     rfm_new_wd=inotify_add_watch(rfm_inotify_fd, realpath, INOTIFY_MASK);
     if (rfm_new_wd < 0) {
       g_warning("set_rfm_curPath(): inotify_add_watch() failed for %s",realpath);
#ifdef GitIntegration
       if (g_hash_table_lookup(gitDeletedDirs, path)) msg=g_strdup_printf(MSG_cannot_enter_deleted_directory, realpath);
       else
#endif
	 msg=g_strdup_printf(MSG_cannot_enter_directory, realpath);
       show_msgbox(msg, "Warning", GTK_MESSAGE_WARNING);
       g_free(msg);
     } else {
       set_rfm_curPath_internal(realpath);

       // inotify_rm_watch will trigger refresh_store in inotify_handler
       // and it will destory and recreate view base on conditions such as whether curPath_is_git_repo
       inotify_rm_watch(rfm_inotify_fd, rfm_curPath_wd);
       rfm_curPath_wd = rfm_new_wd;
       if (pauseInotifyHandler) refresh_store();
     }

     if (UseTargetAddressWhenEnterSymbloicLinkForDir) free(realpath);
   /* clear_history(); */
   /* if (e=read_history(g_build_filename(rfm_curPath,".rfm_history", NULL))) */
   /*     g_warning("failed to read_history(%s) error code:%d. it's normal if you enter this directory for the first time with rfm.",g_build_filename(rfm_curPath,".rfm_history", NULL),e); */
   /* history_entry_added=0; */
}


/*why i maintain this ItemSelected value here instead of getting it on need?*/
/*because it is used in the readline thread, and i don't think the get_view_selection_list is thread safe*/
static void selectionChanged(GtkWidget *view, gpointer user_data)
{
  GList *newSelectionFilePathList=NULL;
  if (stdin_cmd_selection_list) g_list_free_full(stdin_cmd_selection_list, (GDestroyNotify)gtk_tree_path_free);
  stdin_cmd_selection_list=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  int count=0; //TODO: why not remove this and use ItemSelected directly?
  if (stdin_cmd_selection_list==NULL){
    ItemSelected=0;
  }else{
    GList * selectionListElement = g_list_first(stdin_cmd_selection_list);
    while(selectionListElement!=NULL){
	GValue fullpath = G_VALUE_INIT;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter_selectionChanged, selectionListElement->data);
	gtk_tree_model_get_value(treemodel, &iter_selectionChanged, COL_FULL_PATH, &fullpath);
	newSelectionFilePathList = g_list_prepend(newSelectionFilePathList, g_strdup(g_value_get_string(&fullpath)));
	g_debug("selected file before refresh:%s",(char *)newSelectionFilePathList->data);
	count++;
	selectionListElement=g_list_next(selectionListElement);
    }
    g_mutex_lock(&rfm_selection_completion_lock);
    free(rfm_selection_completion);
    rfm_selection_completion = g_strdup(newSelectionFilePathList->data);
    ItemSelected=count;
    g_mutex_unlock(&rfm_selection_completion_lock);
    if (ItemSelected==1){
      GValue file_mode_str = G_VALUE_INIT;
      gtk_tree_model_get_value(treemodel, &iter_selectionChanged, COL_MODE_STR, &file_mode_str);
      g_free(selected_file_mode_str);
      selected_file_mode_str = g_strdup(g_value_get_string(&file_mode_str));
    }
  }

  g_list_free_full(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_free);
  filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = newSelectionFilePathList;
  g_debug("Number of view selection:%d",count);

  if (rfm_prePath!=NULL) { g_free(rfm_prePath); rfm_prePath=NULL; } //可能是因为 refresh_store 里 RFM_AUTOSELECT_OLDPWD_IN_VIEW 的情况，这里要清空。而 cd - 用的是环境变量OLDPWD,所以这里不影响。
}

// establish environmental varables for view columns
// 为把列表试图字段值通过同名环境变量传递给子进程作准备，以便在脚本中使用
static void set_env_to_pass_into_child_process_for_view_columns_when_ItemSelected_equals_one(const GtkTreeIter *iter, gchar*** env_for_g_spawn){
  g_log(RFM_LOG_GSPAWN,G_LOG_LEVEL_DEBUG,"set_env_to_pass_into_child_process");
  for (enum RFM_treeviewCol c=0;c<NUM_COLS;c++){
    if (gtk_tree_model_get_column_type(GTK_TREE_MODEL(store), c)!=G_TYPE_STRING) continue;
    //TODO: deal with types such as G_TYPE_UINt64
    int i=get_treeviewColumnsIndexByEnum(c);
    if (i>0 && TREEVIEW_COLUMNS[i].Show){
      gchar* env_var_name = g_strconcat(RFM_ENV_VAR_NAME_PREFIX, TREEVIEW_COLUMNS[i].title,NULL);
      gchar* env_var_value = NULL;
      gtk_tree_model_get(GTK_TREE_MODEL(store), iter, c, &env_var_value, -1);
      if(env_var_value!=NULL){
	 g_log(RFM_LOG_GSPAWN,G_LOG_LEVEL_DEBUG,"g_environ_setenv %s %s",env_var_name,env_var_value);
         gchar ** updated_env_for_g_spawn = g_environ_setenv(*env_for_g_spawn, env_var_name, env_var_value, 1);//TODO:按文档上说,caller应该g_strfreev(*env_for_g_spawn),但gdb调试这一句过后,**env_for_g_spawn就内存不可访问了,这一句前还是有效值的,为啥?是文档错了吗?
	 //free(*env_for_g_spawn); 这里无论用free还是g_strfreev在我机器上都会导致程序崩溃退出
	 *env_for_g_spawn = updated_env_for_g_spawn;
	 g_free(env_var_value);env_var_value=NULL;
      }
      g_free(env_var_name);
    }
  }
}

//called for example when user press Enter on selected file or double click on it.
static void item_activated(GtkWidget *icon_view, GtkTreePath *tree_path, gpointer user_data)
{
   GtkTreeIter iter;
   guint i;
   long int index_of_default_file_activation_action_based_on_mime_or_searchresultType=-1;
   gchar *msg;
   RFM_FileAttributes *fileAttributes;

   gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, tree_path);
   gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);

   if (!fileAttributes->is_dir) {

      for(i=0; i<G_N_ELEMENTS(run_actions); i++) {
	 if (SearchResultViewInsteadOfDirectoryView && g_strcmp0(run_actions[i].OrSearchResultType, SearchResultTypePointerForCurrentExistingSearchResult->name)==0){
	    index_of_default_file_activation_action_based_on_mime_or_searchresultType = i;
	    break;
	 }
         if (strcmp(fileAttributes->mime_root, run_actions[i].runRoot)==0) {
            if (strcmp(fileAttributes->mime_sub_type, run_actions[i].runSub)==0 || strncmp("*", run_actions[i].runSub, 1)==0) { /* Exact match */
               index_of_default_file_activation_action_based_on_mime_or_searchresultType = i;
               break;
            }
         }
      }

      if (index_of_default_file_activation_action_based_on_mime_or_searchresultType != -1){
	file_menu_exec(NULL, &(run_actions[index_of_default_file_activation_action_based_on_mime_or_searchresultType]));
      }else {
         msg=g_strdup_printf("No default file activation action defined for mime type:\n %s/%s\n", fileAttributes->mime_root, fileAttributes->mime_sub_type);
         show_msgbox(msg, "Run Action", GTK_MESSAGE_INFO);
         g_free(msg);
      }
   }else{ /* If dir, reset rfm_curPath and fill the model */
      SearchResultViewInsteadOfDirectoryView=0; //let the following set_rfm_curPath to tigger refresh_store from inotify_handler
      set_rfm_curPath(fileAttributes->path);
   }
}

static void row_activated(GtkTreeView *tree_view, GtkTreePath *tree_path,GtkTreeViewColumn *col, gpointer user_data)
{
  item_activated(GTK_WIDGET(tree_view),tree_path,user_data);
}


/*Helper function to get selected items from iconview or treeview*/
/*return GList* of GtkTreePath* */
static GList* get_view_selection_list(GtkWidget * view, gboolean treeview, GtkTreeModel ** model)
{
   if (view==NULL) return NULL;
   if (treeview) {
     return gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), model);
   } else {
     return gtk_icon_view_get_selected_items(GTK_ICON_VIEW(view));
   }
}

/*Helper function to set selected item from iconview or treeview*/
static void set_view_selection(GtkWidget* view, gboolean treeview, GtkTreePath* treePath){
    if (treeview){
      gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),treePath);
      //gtk_tree_view_set_cursor(GTK_TREE_VIEW(icon_or_tree_view),treePath,NULL,FALSE); //TODO: if i use this as Rodney did, i can press enter directly to activate it. However, in multiselect situation, other rows will get unselected.
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(icon_or_tree_view),treePath,NULL,FALSE,0,0);
    }else{
      gtk_icon_view_select_path(GTK_ICON_VIEW(view),treePath);
      gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(icon_or_tree_view), treePath,TRUE, 1.0, 1.0);
    }
}

/*Helper function to set selected items from iconview or treeview*/
static void set_view_selection_list(GtkWidget *view, gboolean treeview,GList *selectionList) {
  selectionList=g_list_first(selectionList);
  while (selectionList != NULL) {
    set_view_selection(view, treeview, (GtkTreePath*)(selectionList->data));
    selectionList = g_list_next(selectionList);
  }
}

static void up_clicked()
{
   gchar *dir_name = g_path_get_dirname(rfm_curPath);
   set_rfm_curPath(dir_name);
   g_free(dir_name);
}

static void home_clicked()
{
   set_rfm_curPath(rfm_homePath);
}

static void info_clicked()
{
   GList *listElement=NULL;
   GString   *string=NULL;
   RFM_ChildAttribs *child_attribs=NULL;
   gchar *msg=NULL;

   string=g_string_new("Running processes:\npid: name\n");
   
   listElement=g_list_first(rfm_childList);

   while (listElement != NULL) {
      child_attribs=(RFM_ChildAttribs*)listElement->data;
      g_string_append_printf(string, "%i: ", child_attribs->pid);
      g_string_append(string, child_attribs->name);
      g_string_append(string, "\n");
      listElement=g_list_next(listElement);
   }

   msg=g_string_free(string,FALSE);
   printf("%s\n",msg);
   g_free(msg);
}


/* Helper function to get treepath from treeview or iconview. This return value needs to be freed */
static void get_path_at_view_pos(GtkWidget* view, gboolean treeview,gint x,gint y, GtkTreePath ** retval)
{
  if (treeview) {
    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view), x, y, retval, NULL, NULL,NULL);
  } else
    *retval = gtk_icon_view_get_path_at_pos(GTK_ICON_VIEW(view), x, y);
}

//helper function due to different selection APIs of treeview and iconview
static gboolean path_is_selected(GtkWidget *widget, gboolean treeview, GtkTreePath *path) {
  if (treeview)
    return gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), path);
  else
    return gtk_icon_view_path_is_selected(GTK_ICON_VIEW(widget), path);
}

static void copy_curPath_to_clipboard(GtkWidget *menuitem, gpointer user_data)
{
   /* Clipboards: GDK_SELECTION_CLIPBOARD, GDK_SELECTION_PRIMARY, GDK_SELECTION_SECONDARY */
   //https://specifications.freedesktop.org/clipboards-spec/clipboards-0.1.txt
   /* No one ever does anything interesting with SECONDARY as far as I can
tell.

The current consensus is that interpretation b) is correct. Qt 3 and
GNU Emacs 21 will use interpretation b), changing the behavior of
previous versions.

The correct behavior can be summarized as follows: CLIPBOARD works
just like the clipboard on Mac or Windows; it only changes on explicit
cut/copy. PRIMARY is an "easter egg" for expert users, regular users
can just ignore it; it's normally pastable only via
middle-mouse-click.  */
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
   gtk_clipboard_set_text(clipboard, rfm_curPath, -1);
}

static void g_spawn_wrapper_for_selected_fileList_(FREE_FULL RFM_ChildAttribs *childAttribs)
{
  //selectionChanged(NULL,NULL); //TODO: 如果cpu资源不够，gtk有没有可能漏掉触发，要不要强制调用下？
  if (filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]!=NULL) {
      GList *filepath_list_for_selection_on_view_clone = g_list_copy_deep(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_strdup,NULL);
      if (ItemSelected==1){
	childAttribs->env=g_get_environ();
	set_env_to_pass_into_child_process_for_view_columns_when_ItemSelected_equals_one(&iter_selectionChanged,&(childAttribs->env));
      }
      g_spawn_wrapper_(filepath_list_for_selection_on_view_clone, NULL,childAttribs);
      g_list_free_full(filepath_list_for_selection_on_view_clone,(GDestroyNotify)g_free);
  }
}

//TODO: 目前这里config.h里面的bash,nu,python等interpreter,稍后再尝试 zsh等,都无需在AddOn里面. 如果需要用AI来解析命令,须参照parse_and_exec_command_in_gtk_thread 里面代码扩充此部分
static NO_FREE stdin_cmd_interpretor* match_cmd_str_array_with_interpretors(gchar** RunCmd){
  for (int i=0; i<G_N_ELEMENTS(stdin_cmd_interpretors); i++){
    if (RunCmd[0] == stdin_cmd_interpretors[i].name)//注意这里比较的是两个字符串的地址，而不是用strcmp比较内容，这样不会因为恰巧相同的字符串造成误判
      return &(stdin_cmd_interpretors[i]);
  }
  return NULL;
}

static void toolbar_button_exec(GtkToolItem *item, RFM_ToolButton *button_definition)
{
   if (button_definition->RunCmd==NULL) /* Argument is an internal function */
     button_definition->func(button_definition);
   else { /* Assume argument is a shell exec */
     RFM_ChildAttribs *child_attribs=calloc(1, sizeof(RFM_ChildAttribs));
     child_attribs->RunCmd = g_strdupv(button_definition->RunCmd);
     if (!exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread) child_attribs->spawn_async = TRUE;//到目前为止，我日常很少通过 toggleBlockGUI 功能，只是在功能添加之初测试过，所以改这一句对我现有常用功能应该没啥影响。设计的出发点在于在执行命令的时候锁死GUI界面，究竟async 好还是sync 好，当时应该也比较随意。也就是当时应该也没明确定义是否要block到自进程结束。显然对于耗时较长的 search ，async 不会让终端输入等待太久，所以之前一直 async 执行子进程。到目前为止，子进程bash脚本执行过程都无需终端窗口输入。需要交互输入的常用命令如AI addon的对话，是通过在 exec_stdin_command 中另外的方式处理同步的，会和cmd_interpretor 提示符关联，好处是可以在 config.h, rfm_addon.c 里面为命令设定好cmd_interpretor.这次新加AI对话历史相似度查询功能，犹豫是不是新加一个cmd_interpretor 。问题是interpretor类型太多用户也不好记。所以做了这个commit 中 exec_stdin_command 中的修改，以便用户在切换到 exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread 状态后可以运行需要提示输入的脚本且不和readlineThread冲突，TODO: 但问题是这个现在要提示用户手工切换。
     child_attribs->name = g_strdup(button_definition->buttonName);
     child_attribs->customCallBackFunc = button_definition->func;
     child_attribs->customCallbackUserData = button_definition;
#ifdef RFM_AddOn
     GHashTableIter ht_iter;
     gchar *key;
     RFM_ADDON *value;
     g_hash_table_iter_init(&ht_iter, addons);
     while (g_hash_table_iter_next(&ht_iter, &key, &value)){
       if ((button_definition >= value->resource_managed_by_addon->toolbar_button_definitions_local)
	   && button_definition < value->resource_managed_by_addon->toolbar_button_definitions_local + (value->resource_managed_by_addon->toolbar_button_definitions_local_array_size)*sizeof(RFM_ToolButton)){ //根据地址判断button_definition 指向某个静态数组.TODO:建立一个反向索引hashtable,以resource_managed_by_addon成员地址为key,addon为value,这样就不用循环了，直接lookup。问题是建一个hashtable呢还是每种成员建一个hashtable，如果是每种成员建一个hashtable，需要iterate时，每次只需要在特定hashtable上iterate就可以了
	 child_attribs->toolbar_button_or_filemenu_addon_dir = g_path_get_dirname(value->addon_c_fullpath);
	 break;
       }
     }
#endif
     stdin_cmd_interpretor *cmd_interpretor_pointer = match_cmd_str_array_with_interpretors(button_definition->RunCmd);//match_cmd函数里面会使用地址相等比较，所以要用原始静态字符串数组，而不是g_strdupv深度复制的
     if (cmd_interpretor_pointer){
       gchar* cmd = strdup(child_attribs->RunCmd[1]);
       if (startWithTerm()) printf("\n\r%s\033[32m%s%s%s\033[0m`%s`\r\n",InheritedPrompt, cmd_interpretor_pointer->prompt,(ItemSelected==0)?"":"*",exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread?"]":">",cmd);
       parse_and_exec_command_in_gtk_thread(cmd, FALSE, cmd_interpretor_pointer, child_attribs);
     }else g_spawn_wrapper_(NULL, NULL, child_attribs);
   }
}

static void file_menu_exec(GtkMenuItem *menuitem, RFM_MenuItem *file_menu_item_definition)
{ 
  if (file_menu_item_definition->runCmd==NULL) //如果runCmd为空，则直接执行func,否则，先执行runCmd,后通过callbackfunc指针执行func
     file_menu_item_definition->func(file_menu_item_definition);
  else{
     RFM_ChildAttribs *child_attribs=calloc(1, sizeof(RFM_ChildAttribs));
     child_attribs->RunCmd = g_strdupv(file_menu_item_definition->runCmd);
     if (!exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread) child_attribs->spawn_async = TRUE;
     child_attribs->name = g_strdup(file_menu_item_definition->runName);
     child_attribs->customCallBackFunc = file_menu_item_definition->func;
     child_attribs->customCallbackUserData = file_menu_item_definition;
#ifdef RFM_AddOn
     GHashTableIter ht_iter;
     gchar *key;
     RFM_ADDON *value;
     g_hash_table_iter_init(&ht_iter, addons);
     while (g_hash_table_iter_next(&ht_iter, &key, &value)){
       if ((file_menu_item_definition >= value->resource_managed_by_addon->file_menuitem_definitions_local)
	   && file_menu_item_definition < value->resource_managed_by_addon->file_menuitem_definitions_local + (value->resource_managed_by_addon->file_menuitem_definitions_local_array_size)*sizeof(RFM_MenuItem)){ //根据地址判断button_definition 指向某个静态数组.TODO:建立一个反向索引hashtable,以resource_managed_by_addon成员地址为key,addon为value,这样就不用循环了，直接lookup。问题是建一个hashtable呢还是每种成员建一个hashtable，如果是每种成员建一个hashtable，需要iterate时，每次只需要在特定hashtable上iterate就可以了
	 child_attribs->toolbar_button_or_filemenu_addon_dir = g_path_get_dirname(value->addon_c_fullpath);
	 break;
       }
     }
#endif
     stdin_cmd_interpretor *cmd_interpretor_pointer = match_cmd_str_array_with_interpretors(file_menu_item_definition->runCmd);//match_cmd函数里面会使用地址相等比较，所以要用原始静态字符串数组，而不是g_strdupv深度复制的
     if (cmd_interpretor_pointer){
       gchar* cmd = strdup(child_attribs->RunCmd[1]);
       if (startWithTerm()) printf("\n\r%s\033[32m%s%s%s\033[0m`%s`\r\n",InheritedPrompt, cmd_interpretor_pointer->prompt,(ItemSelected==0)?"":"*",exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread?"]":">",cmd);
       parse_and_exec_command_in_gtk_thread(cmd, FALSE, cmd_interpretor_pointer, child_attribs);
     }else g_spawn_wrapper_for_selected_fileList_(child_attribs);
  }
}

void add_file_menuitems(GtkWidget ***menuitems, const RFM_MenuItem menuitem_definitions[], const size_t size){
     if (no_window) return;
     assert(*menuitems==NULL);
     *menuitems = calloc(size + 1, sizeof(GtkWidget*));
     for(int i=0; i<size; i++) {
       (*menuitems)[i]=gtk_menu_item_new_with_label(menuitem_definitions[i].runName);
       gtk_menu_shell_append(GTK_MENU_SHELL(menu), (*menuitems)[i]);
       g_signal_connect((*menuitems)[i], "activate", G_CALLBACK(file_menu_exec), &(menuitem_definitions[i]));
     }
}

#ifdef RFM_AddOn
void remove_file_menuitems(FREE GtkWidget ***menuitems, const size_t size) {
  if (no_window) return;
  for (int i=0; i<size; i++) if ((*menuitems)[i]) gtk_widget_destroy((*menuitems)[i]);
  free(*menuitems); *menuitems=NULL;
}
#endif

static void show_hide_file_menu_item(GtkWidget ***menuitems, RFM_MenuItem menuitem_definitions[],size_t menuitem_definitions_size, gboolean match_mimeRoot, gboolean match_mimeSub, RFM_FileAttributes* first_selection_fileAttributes, GList *selectionList) {
   int *showMenuItem = calloc(menuitem_definitions_size, sizeof(int));
   GtkTreeIter iter;
   RFM_FileAttributes *fileAttributes;

   for(int i=0; i<menuitem_definitions_size; i++) {
      if (match_mimeSub) { /* All selected files are the same type */
         if (strcmp(first_selection_fileAttributes->mime_root, menuitem_definitions[i].runRoot)==0 && strcmp(first_selection_fileAttributes->mime_sub_type, menuitem_definitions[i].runSub)==0)
            showMenuItem[i]++; /* Exact match */
      }
      if (match_mimeRoot) {
         if (strcmp(first_selection_fileAttributes->mime_root, menuitem_definitions[i].runRoot)==0 && strncmp("*", menuitem_definitions[i].runSub, 1)==0)
            showMenuItem[i]++; /* Selected files have the same mime root type */
      }
      if (strncmp("*", menuitem_definitions[i].runRoot, 1)==0)
         showMenuItem[i]++;   /* Run actions to show for all files */
      if (SearchResultViewInsteadOfDirectoryView && g_strcmp0(menuitem_definitions[i].OrSearchResultType, SearchResultTypePointerForCurrentExistingSearchResult->name)==0) showMenuItem[i]++;
      if (menuitem_definitions[i].filenameSuffix){
	 GList *listElement=g_list_first(selectionList);
	 while (listElement) {
	    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
	    gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
            if (!g_str_has_suffix(fileAttributes->path, menuitem_definitions[i].filenameSuffix)){
	      showMenuItem[i]=0;
	      break;
	    }
	    listElement=g_list_next(listElement);
	 }
      }
   }

   for(int i=0; i<menuitem_definitions_size; i++) {
      if (showMenuItem[i]>0 && (menuitem_definitions[i].showCondition == NULL || menuitem_definitions[i].showCondition(fileAttributes)))
         gtk_widget_show((*menuitems)[i]);
      else
         gtk_widget_hide((*menuitems)[i]);
   }

   free(showMenuItem);
}

static gboolean popup_file_menu(GdkEvent *event)
{
   GtkTreeIter iter;
   GList *selectionList;
   GList *listElement;
   
   RFM_FileAttributes *fileAttributes_of_the_first_selected_file, *fileAttributes;
   gboolean match_mimeRoot=TRUE, match_mimeSub=TRUE;

   selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
   if (selectionList==NULL)
      return FALSE;

   listElement=g_list_first(selectionList);
   if (listElement!=NULL) {   /* Get file attributes for first item in the list */
      gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes_of_the_first_selected_file, -1);
      listElement=g_list_next(listElement);
   }
   while(listElement!=NULL) { /* Compare first item with any other selected items */
      gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
      if (strcmp(fileAttributes_of_the_first_selected_file->mime_root, fileAttributes->mime_root)!=0) {
         match_mimeRoot=FALSE;
         break;   /* Finish here: files are different */
      }
      if (strcmp(fileAttributes_of_the_first_selected_file->mime_sub_type, fileAttributes->mime_sub_type)!=0)
         match_mimeSub=FALSE;

      listElement=g_list_next(listElement);
   }

   show_hide_file_menu_item(&menuItem, run_actions, G_N_ELEMENTS(run_actions), match_mimeRoot, match_mimeSub, fileAttributes_of_the_first_selected_file, selectionList);
#ifdef RFM_AddOn
   GHashTableIter ht_iter;
   gchar *key;
   RFM_ADDON *value;
   g_hash_table_iter_init(&ht_iter, addons);
   while (g_hash_table_iter_next(&ht_iter, &key, &value)){
     RFM_ADDON_RESOURCE_TYPE* addon_resource = value->resource_managed_by_addon;
     if (addon_resource->file_menuitem_definitions_local_array_size>0) show_hide_file_menu_item(&(addon_resource->menuItems_local), addon_resource->file_menuitem_definitions_local, addon_resource->file_menuitem_definitions_local_array_size, match_mimeRoot, match_mimeSub, fileAttributes_of_the_first_selected_file, selectionList);
   }
#endif
   gtk_menu_popup_at_pointer(GTK_MENU(menu), event);
   g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
   return TRUE;
}

static void sort_on_column_header(GtkMenuItem *menuitem, RFM_treeviewColumn* rfmCol){
  if (rfmCol==NULL) return;
  gint sort_column_id_for_clicked_column_id=rfmCol->enumSortCol==NUM_COLS? rfmCol->enumCol:rfmCol->enumSortCol;
  if (sort_column_id_for_clicked_column_id==COL_FILENAME) sort_column_id_for_clicked_column_id=GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
  //gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(treemodel), &current_sort_column_id, &current_sorttype);
      
  if (current_sort_column_id==sort_column_id_for_clicked_column_id){
    gint new_sorttype=current_sorttype==GTK_SORT_ASCENDING? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treemodel), sort_column_id_for_clicked_column_id, new_sorttype);
    g_log(RFM_LOG_DATA_SORT, G_LOG_LEVEL_DEBUG,"clicked column %d, sort column for clicked %d == current sort column %d, current order %d, new order %d",rfmCol->enumCol,rfmCol->enumSortCol,current_sort_column_id,current_sorttype,new_sorttype);
    current_sorttype = new_sorttype;
  }else{
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treemodel), sort_column_id_for_clicked_column_id, GTK_SORT_ASCENDING);
    g_log(RFM_LOG_DATA_SORT, G_LOG_LEVEL_DEBUG,"clicked column %d, sort column for clicked %d <> current sort column %d, new sort column %d, current order %d, order %d",rfmCol->enumCol,rfmCol->enumSortCol,current_sort_column_id,sort_column_id_for_clicked_column_id, current_sorttype,GTK_SORT_ASCENDING);
    current_sorttype = GTK_SORT_ASCENDING;
  }
  current_sort_column_id = sort_column_id_for_clicked_column_id;
}

static void hide_column_menuitem_activated(GtkMenuItem *menuitem, RFM_treeviewColumn *col) {
        char cmd[256+1];
        snprintf(cmd, sizeof(cmd),"-%s",col->title);
        if (startWithTerm()) printf("\n\r%s\033[32m%s%s%s`\033[0mshowcolumn %s`\r\n",InheritedPrompt, current_stdin_cmd_interpretor_pointer->prompt,(ItemSelected==0)?"":"*",exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread?"]":">",cmd);
        show_hide_treeview_columns_in_order(cmd);
}

static void insert_column_menuitem_callback(GtkMenuItem *menuitem,char *order_seq) {
  if (startWithTerm()) printf("\n\r%s\033[32m%s%s%s`\033[0mshowcolumn %s`\r\n",InheritedPrompt, current_stdin_cmd_interpretor_pointer->prompt,(ItemSelected==0)?"":"*",exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread?"]":">",order_seq);
      show_hide_treeview_columns_in_order(order_seq);
}

static void createCandidateColumnsMenu(GtkMenuItem* parent, RFM_treeviewColumn *col) {
  GtkWidget* candidateColumnsMenu = gtk_menu_item_get_submenu(parent);
  if (candidateColumnsMenu && GTK_IS_WIDGET(candidateColumnsMenu)){
      gtk_widget_destroy(candidateColumnsMenu);
      gtk_menu_item_set_submenu(parent, NULL);
  }
  //TODO:上面是删除上次生成的旧的，下面是根据当前 TREEVIEW_COLUMNS 建新的. 上面的代码是否可以删除,让gtk自动释放旧的
  candidateColumnsMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(parent, candidateColumnsMenu);
  char menuTitle[256+10];
  for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
    GtkMenuItem* menuItemForColumn = NULL;
    if (TREEVIEW_COLUMNS[i].enumCol<NUM_COLS){
      snprintf(menuTitle, sizeof(menuTitle),"%4d: %s%s", TREEVIEW_COLUMNS[i].Show? TREEVIEW_COLUMNS[i].enumCol : (-1)*TREEVIEW_COLUMNS[i].enumCol,TREEVIEW_COLUMNS[i].title, (current_sort_column_id==TREEVIEW_COLUMNS[i].enumCol?(current_sorttype==GTK_SORT_ASCENDING?"{v}":"{^}"):""));
      menuItemForColumn = gtk_menu_item_new_with_label(menuTitle);
    }else if (TREEVIEW_COLUMNS[i].ValueFunc!=getExtColumnValueFromHashTable && TREEVIEW_COLUMNS[i].ValueFunc!=getDimensionValueFromHashTable){
      g_assert(!TREEVIEW_COLUMNS[i].Show);
      snprintf(menuTitle,sizeof(menuTitle),"    : %s",TREEVIEW_COLUMNS[i].title);
      menuItemForColumn = gtk_menu_item_new_with_label(menuTitle);
    }
    if (menuItemForColumn){
      gtk_menu_shell_append(GTK_MENU_SHELL(candidateColumnsMenu), menuItemForColumn);
      
      char order_seq[256*2+1];
      if (parent==InsertAfterMenuItem)
	snprintf(order_seq, sizeof(order_seq), "%s,%s",col->title,TREEVIEW_COLUMNS[i].title);
      else{//在此列前插入的情况比较复杂，需要分析此列 col 的前一列的各种情况
	int colIndexOfClickedHeader = get_treeviewColumnsIndexByEnum(col->enumCol);
	if (colIndexOfClickedHeader<0)
	  g_warning("get_treeviewColumnsIndexByEnum(%d) return %d", col->enumCol, colIndexOfClickedHeader);
	else if (colIndexOfClickedHeader==0) //点击的是第一列的标题
	  snprintf(order_seq, sizeof(order_seq), ",%s",TREEVIEW_COLUMNS[i].title);
	else if ((colIndexOfClickedHeader-1)==i) //出现类似 showcolumn -AiPrompt,AiPrompt 这样的情况，直接变为 showcolumn AiPrompt
	  snprintf(order_seq, sizeof(order_seq), "%s",TREEVIEW_COLUMNS[i].title);
	else{
	  if (TREEVIEW_COLUMNS[colIndexOfClickedHeader-1].Show)//显示在点击列的前一列后面，但要注意保持前一列的显隐状态
	    snprintf(order_seq, sizeof(order_seq), "%s,%s",TREEVIEW_COLUMNS[colIndexOfClickedHeader-1].title, TREEVIEW_COLUMNS[i].title);
	  else
	    snprintf(order_seq, sizeof(order_seq), "-%s,%s",TREEVIEW_COLUMNS[colIndexOfClickedHeader-1].title, TREEVIEW_COLUMNS[i].title);
	}
      }
      g_signal_connect_data(menuItemForColumn, "activate", G_CALLBACK(insert_column_menuitem_callback), g_strdup(order_seq),g_free,G_CONNECT_DEFAULT);
    }
  }
}

static gboolean view_column_header_clicked(GtkButton* column_header, GdkEvent *event, GtkWidget *gtkCol){
  GdkEventButton *bevent = (GdkEventButton *)event;
  RFM_treeviewColumn *col = get_treeviewcolumnByGtkTreeviewcolumn(gtkCol);
  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 3) { // Right click
    if (HideColumnMenuItem) gtk_widget_destroy(HideColumnMenuItem);
    HideColumnMenuItem = gtk_menu_item_new_with_label(MENU_hideThisColumn);
    gtk_menu_shell_append(GTK_MENU_SHELL(columnHeaderMenu), HideColumnMenuItem);
    g_signal_connect(HideColumnMenuItem, "activate", G_CALLBACK(hide_column_menuitem_activated), col);
    createCandidateColumnsMenu(InsertBeforeMenuItem, col);
    createCandidateColumnsMenu(InsertAfterMenuItem, col);
    gtk_widget_show_all(columnHeaderMenu);
    gtk_menu_popup_at_pointer(columnHeaderMenu, event);
    return TRUE;
  }else if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1) { // Left click
    sort_on_column_header(NULL, col);
    return TRUE;
  }
  return FALSE;
}

static void cmd_handle_escape_key() {
    if(treeview) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(icon_or_tree_view)));
    else gtk_icon_view_unselect_all(GTK_ICON_VIEW(icon_or_tree_view));
}

static gboolean view_key_press(GtkWidget *widget, GdkEvent *event) {
  GdkEventKey *ek=(GdkEventKey *)event;
  if (ek->keyval==GDK_KEY_Menu){
    if (!popup_file_menu(event)){
      //TODO: 当没有文件选中，popup_file_menu返回false. 找到tool_bar 中包含的因为宽度不够而显示的溢出按钮，并调用点击。但问题是，工具栏（GtkToolbar）内部有一个容器（可能是GtkBox）来存放工具项，而溢出按钮可能是在这个容器内，但并不是通过gtk_container_get_children()返回的直接子控件。deepseek给出的代码使用gtk_contain_get_children返回工具栏的children,但总是找不到这个按钮。问题是 gtk Inspector 为啥可以列出这个按钮，也许要查阅gtk Inspector源代码. 另一方面，gtk4 好像对工具栏改动比较大
    }else return TRUE;
  } else if (ek->keyval==GDK_KEY_q){
    cleanup();
    return TRUE;
  } else if (ek->keyval == GDK_KEY_Escape) {
    cmd_handle_escape_key();
    return TRUE;
  } else if (ek->keyval == GDK_KEY_Tab){
    g_message("This is the command window");
  } else {
    for (int i = 0; i < G_N_ELEMENTS(tool_buttons); i++) {
      if (tool_buttons[i].Accel &&
	  ek->keyval == tool_buttons[i].Accel &&
          (ek->state & GDK_MOD1_MASK) == GDK_MOD1_MASK &&
          SHOW_TOOLBAR_BUTTON(tool_buttons,i)) {
        tool_buttons[i].func(&(tool_buttons[i]));
        return TRUE;
      }
    }
  }

  return FALSE;
}

static gboolean on_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data) {
    if (keyboard_mode) return FALSE; // 忽略键盘模式

    GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
    GtkTreePath *path=NULL;
    GtkTreeViewColumn *column=NULL;
    RFM_treeviewColumn * col=NULL;

    gint bx,by;
    gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &bx, &by);
    // 获取鼠标位置对应的路径和列
    if (gtk_tree_view_get_path_at_pos(tree_view, bx, by, &path, &column, NULL, NULL)) {
        GtkTreeIter iter;
        GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

        if (path && gtk_tree_model_get_iter(model, &iter, path)) {
	    if (column) col = get_treeviewcolumnByGtkTreeviewcolumn(column);
	    if (col==NULL || col->ValueFunc!=getExtColumnValueFromHashTable) return FALSE;//Dimension 列內容通常不會很長，能顯示完整，無需tooltip
	    RFM_FileAttributes *fileAttributes;
            gtk_tree_model_get(model, &iter, COL_ATTR, &fileAttributes, -1);
	    gchar *cell_text = getExtColumnValueFromHashTable(fileAttributes, col->enumCol-COL_Ext1);//TODO:如果添加getExtColumnFirstLineValueFromHashTable函数，这里也要改
            if (cell_text && cell_text[0] != '\0') {
                // 设置Tooltip文本
                gtk_tooltip_set_text(tooltip, cell_text);
                gtk_tree_path_free(path);
                return TRUE;
            }
        }
        if (path) gtk_tree_path_free(path);
    }
    return FALSE; // 未处理Tooltip
}

static gboolean view_button_press(GtkWidget *widget, GdkEvent *event)
{
   GtkTreePath *tree_path=NULL;
   gboolean ret_val=FALSE;
   GdkEventButton *eb=(GdkEventButton*)event;

   if (eb->type!=GDK_BUTTON_PRESS)
      return FALSE;  /* Only accept single clicks here: item_activated() handles double click */

   if (eb->state&GDK_CONTROL_MASK || eb->state&GDK_SHIFT_MASK)
      return FALSE;  /* If CTRL or SHIFT is down, let icon view handle selections */

   get_path_at_view_pos(widget,treeview,eb->x,eb->y,&tree_path);
   switch (eb->button) {
      case 3:  /* Button 3 selections */
         if (tree_path) {
	   if (! path_is_selected(widget,treeview, tree_path)) {
             if (treeview) {
               gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
               gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), tree_path,NULL,FALSE);
             } else {
               gtk_icon_view_unselect_all(GTK_ICON_VIEW(widget));
               gtk_icon_view_select_path(GTK_ICON_VIEW(widget), tree_path);
             }
            }
            popup_file_menu(event);
            ret_val=TRUE;
         }
         else {
	    if(treeview)
	      gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
	    else
              gtk_icon_view_unselect_all(GTK_ICON_VIEW(widget));
            ret_val=TRUE;
         }
      break;
      default:
         ret_val=FALSE;
      break;
   }
   gtk_tree_path_free(tree_path);
   return ret_val;
}

static void refresh_toolbar()
{
  if (no_window) return;
   gchar *tooltip = SearchResultViewInsteadOfDirectoryView? g_strdup_printf("rfm_curPath:%s\nrfm_SearchResultPath:%s\n%s",rfm_curPath,rfm_SearchResultPath,TOOL_TIP_PathAndRepositoryNameDisplay):g_strdup(TOOL_TIP_PathAndRepositoryNameDisplay);
   gtk_tool_item_set_tooltip_text(PathAndRepositoryNameDisplay, tooltip);
   g_free(tooltip);
   gtk_widget_show(PathAndRepositoryNameDisplay);
   for (guint i = 0; i < G_N_ELEMENTS(tool_buttons); i++) {
     if (SHOW_TOOLBAR_BUTTON(tool_buttons,i)) gtk_widget_show(tool_bar_buttons[i]);
     else gtk_widget_hide(tool_bar_buttons[i]);
   }
#ifdef RFM_AddOn
   GHashTableIter iter;
   gchar *key;
   RFM_ADDON *value;
   g_hash_table_iter_init(&iter, addons);
   while (g_hash_table_iter_next(&iter, &key, &value)) {
     RFM_ADDON_RESOURCE_TYPE* addon_resource = value->resource_managed_by_addon;
     for (guint i = 0; i < addon_resource->toolbar_button_definitions_local_array_size; i++) {
       if (SHOW_TOOLBAR_BUTTON(addon_resource->toolbar_button_definitions_local, i))
         gtk_widget_show(addon_resource->tool_bar_buttons_local[i]);
       else
         gtk_widget_hide(addon_resource->tool_bar_buttons_local[i]);
     }
   }
#endif
}

#ifdef RFM_AddOn
void remove_toolbar_buttons(FREE GtkWidget ***buttons, const size_t size) {
  if (no_window) return;
  for (int i=0; i<size; i++) if ((*buttons)[i]) gtk_widget_destroy((*buttons)[i]);
  free(*buttons); *buttons=NULL;
}
#endif

void add_toolbar_buttons(GtkWidget ***buttons, const RFM_ToolButton button_definitions[], const size_t size) {
  if (no_window) return;
   GtkWidget *buttonImage=NULL; /* Temp store for button image; gtk_tool_button_new() appears to take the reference */
   assert(*buttons==NULL);
   *buttons=calloc(size + 1, sizeof(GtkWidget*));

   for (guint i = 0; i < size; i++) {
       GdkPixbuf *buttonIcon=NULL;
       if (button_definitions[i].buttonIcon!=NULL) buttonIcon=gtk_icon_theme_load_icon(icon_theme, button_definitions[i].buttonIcon, RFM_TOOL_SIZE, 0, NULL);
       if (buttonIcon==NULL)
	 buttonImage=NULL;
       else{
	 buttonImage=gtk_image_new_from_pixbuf(buttonIcon);
	 g_object_unref(buttonIcon);
       }

       (*buttons)[i]=gtk_tool_button_new(buttonImage, button_definitions[i].buttonName);
       gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), (*buttons)[i], -1);
       if(button_definitions[i].tooltip!=NULL) gtk_tool_item_set_tooltip_text((*buttons)[i],button_definitions[i].tooltip);

      g_signal_connect((*buttons)[i], "clicked", G_CALLBACK(toolbar_button_exec), &(button_definitions[i]));
   }
}

static void add_toolbar(GtkWidget *rfm_main_box, RFM_defaultPixbufs *defaultPixbufs)
{
   tool_bar=gtk_toolbar_new();
   gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar), GTK_TOOLBAR_ICONS);
   gtk_box_pack_start(GTK_BOX(rfm_main_box), tool_bar, FALSE, FALSE, 0);

   //we add the following toolbutton here instead of define in config.def.h because we need its global reference.
   PathAndRepositoryNameDisplay = gtk_tool_button_new(NULL,rfm_curPath);
   gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_BUTTON(PathAndRepositoryNameDisplay), -1);
   g_signal_connect(PathAndRepositoryNameDisplay, "clicked", G_CALLBACK(Switch_SearchResultView_DirectoryView),NULL);
   
   add_toolbar_buttons(&tool_bar_buttons,tool_buttons,G_N_ELEMENTS(tool_buttons));
}


static GtkWidget *add_view()
{
   GtkWidget *_view;
   if (no_window) return NULL;
  
   scroll_window=gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start(GTK_BOX(rfm_main_box), scroll_window, TRUE, TRUE, 0);

   if (treeview) {
     _view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
     GtkCellRenderer  * renderer  =  gtk_cell_renderer_text_new();
     GtkCellRendererPixbuf *pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
     gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(_view)),GTK_SELECTION_MULTIPLE);
     gtk_tree_view_set_headers_clickable(_view, TRUE);
     
     for(size_t i=0; i<TREEVIEW_COLUMNS_array_size;i++){
       if (!TREEVIEW_COLUMNS[i].Show){
	 TREEVIEW_COLUMNS[i].gtkCol=NULL;
	 continue;
       }
       g_assert(TREEVIEW_COLUMNS[i].enumCol!=NUM_COLS);
       TREEVIEW_COLUMNS[i].gtkCol = gtk_tree_view_column_new_with_attributes(TREEVIEW_COLUMNS[i].title , (TREEVIEW_COLUMNS[i].enumCol == COL_PIXBUF ? pixbuf_renderer : renderer),(TREEVIEW_COLUMNS[i].enumCol == COL_PIXBUF ? "pixbuf" : "text") ,  TREEVIEW_COLUMNS[i].enumCol , NULL);
       gtk_tree_view_column_set_resizable(TREEVIEW_COLUMNS[i].gtkCol,TRUE);
       gtk_tree_view_append_column(GTK_TREE_VIEW(_view),TREEVIEW_COLUMNS[i].gtkCol);
       gtk_tree_view_column_set_visible(TREEVIEW_COLUMNS[i].gtkCol, TREEVIEW_COLUMNS[i].Show && ((TREEVIEW_COLUMNS[i].showCondition==NULL) ? TRUE: TREEVIEW_COLUMNS[i].showCondition(NULL)));
       gtk_tree_view_column_set_sort_column_id(TREEVIEW_COLUMNS[i].gtkCol, TREEVIEW_COLUMNS[i].enumSortCol==NUM_COLS? TREEVIEW_COLUMNS[i].enumCol: TREEVIEW_COLUMNS[i].enumSortCol);
       g_signal_connect(gtk_tree_view_column_get_button(TREEVIEW_COLUMNS[i].gtkCol), "button-press-event", G_CALLBACK(view_column_header_clicked),TREEVIEW_COLUMNS[i].gtkCol);
     }
     gtk_widget_set_has_tooltip(GTK_WIDGET(_view), TRUE);
     g_signal_connect(_view, "query-tooltip", G_CALLBACK(on_query_tooltip), NULL);
   } else {
     _view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(store));
     gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(_view),GTK_SELECTION_MULTIPLE);
     gtk_icon_view_set_markup_column(GTK_ICON_VIEW(_view),COL_ICONVIEW_MARKUP);
     gtk_icon_view_set_tooltip_column(GTK_ICON_VIEW(_view), COL_ICONVIEW_TOOLTIP);
     gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(_view), COL_PIXBUF);
     gtk_icon_view_set_item_width(GTK_ICON_VIEW(_view), RFM_THUMBNAIL_SIZE);
   }
   #ifdef RFM_SINGLE_CLICK
   gtk_icon_view_set_activate_on_single_click(GTK_ICON_VIEW(icon_view), TRUE);
   #endif
   g_signal_connect(_view, "button-press-event", G_CALLBACK(view_button_press), NULL);
   g_signal_connect(_view, "key-press-event", G_CALLBACK(view_key_press), NULL);

   if (treeview){
     g_signal_connect(_view, "row-activated", G_CALLBACK(row_activated), NULL);
     g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(_view)), "changed", G_CALLBACK(selectionChanged), NULL);
   }
   else{
     g_signal_connect(_view, "item-activated", G_CALLBACK(item_activated), NULL);
     g_signal_connect(_view, "selection-changed", G_CALLBACK(selectionChanged), NULL);
   }
   
   gtk_container_add(GTK_CONTAINER(scroll_window), _view);
   gtk_widget_grab_focus(_view);

   if (!treeview) {
     g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG, "gtk_icon_view_get_column_spacing:%d",gtk_icon_view_get_column_spacing((GtkIconView *)_view));
     g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG,"gtk_icon_view_get_item_padding:%d", gtk_icon_view_get_item_padding((GtkIconView *)_view));
     g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG,"gtk_icon_view_get_spacing:%d", gtk_icon_view_get_spacing((GtkIconView *)_view));
     g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG,"gtk_icon_view_get_item_width:%d", gtk_icon_view_get_item_width((GtkIconView *)_view));
     g_log(RFM_LOG_GTK, G_LOG_LEVEL_DEBUG,"gtk_icon_view_get_margin:%d", gtk_icon_view_get_margin((GtkIconView *)_view));
   }else g_debug("add_view for treeview");
   
   return _view;
}

static void refresh_view(gboolean switch_view) {
  if (no_window) return;
  GList *  selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  gtk_widget_hide(rfm_main_box);
  gtk_widget_destroy(scroll_window);
  if (switch_view) treeview=!treeview;
  icon_or_tree_view=add_view();
  gtk_widget_show_all(window);
  refresh_toolbar();
  set_view_selection_list(icon_or_tree_view, treeview, selectionList);
  g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
}

static void switch_iconview_treeview() {
  refresh_view(TRUE);
}

static void Switch_SearchResultView_DirectoryView()
{
    SearchResultViewInsteadOfDirectoryView=SearchResultViewInsteadOfDirectoryView^1;
    refresh_store();
}

static void inotify_insert_item(gchar *name, gboolean is_dir)
{
   //RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");
   //gchar *utf8_display_name=NULL;
   GtkTreeIter iter;
   RFM_FileAttributes *fileAttributes=malloc_fileAttributes();

   if (fileAttributes==NULL) return;
   if (ignored_filename(name)) return;

   //utf8_display_name=g_filename_to_utf8(name, -1, NULL, NULL, NULL);
   fileAttributes->file_name=g_strdup(name);

   if (is_dir) {
      fileAttributes->mime_root=g_strdup("inode");
      fileAttributes->mime_sub_type=g_strdup("directory");
      fileAttributes->pixbuf=g_object_ref(defaultPixbufs->dir);
      //fileAttributes->display_name=g_markup_printf_escaped("<b>%s</b>", utf8_display_name);
   }
   else {   /* A new file was added, but has not completed copying, or is still open: add entry; inotify will call refresh_store when complete */
      fileAttributes->mime_root=g_strdup("application");
      fileAttributes->mime_sub_type=g_strdup("octet-stream");
      fileAttributes->pixbuf=g_object_ref(defaultPixbufs->file);
      //fileAttributes->display_name=g_markup_printf_escaped("<i>%s</i>", utf8_display_name);
   }
   //g_free(utf8_display_name);
   
   fileAttributes->is_dir=is_dir;
   fileAttributes->path=g_build_filename(rfm_curPath, name, NULL);
   fileAttributes->file_mtime= g_date_time_new_from_unix_local(time(NULL)); /* time() returns a type time_t */
   rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);

   //char * c_time=ctime((time_t*)(&(fileAttributes->file_mtime)));
   // c_time[strcspn(c_time, "\n")] = 0;
   fileAttributes->mime_sort=g_strjoin(NULL,fileAttributes->mime_root,fileAttributes->mime_sub_type,NULL);
   fileAttributes->file_name_escaped_for_iconview_markup_column = g_markup_escape_text(fileAttributes->file_name, -1);
   gtk_list_store_insert_with_values(store, &iter, -1,
				     //                     COL_MODE_STR, fileAttributes->file_mode_str,
				     //sCOL_DISPLAY_NAME, fileAttributes->display_name,
		       COL_FILENAME,fileAttributes->file_name,
		       COL_FULL_PATH,fileAttributes->path,
                       COL_PIXBUF, fileAttributes->pixbuf,
                       COL_MTIME, fileAttributes->file_mtime,
				     //		       COL_MTIME_STR,yyyymmddhhmmss(fileAttributes->file_mtime),
				     //	               COL_SIZE,fileAttributes->file_size,
                       COL_ATTR, fileAttributes,
				     //                     COL_OWNER,fileAttributes->owner,
				     //                       COL_GROUP,fileAttributes->group,
                       COL_MIME_ROOT,fileAttributes->mime_root,
                       COL_MIME_SUB,fileAttributes->mime_sub_type,
				     //		       COL_ATIME_STR,yyyymmddhhmmss(fileAttributes->file_atime),
				     //                       COL_CTIME_STR,yyyymmddhhmmss(fileAttributes->file_ctime),
		       COL_MIME_SORT,fileAttributes->mime_sort,
		       COL_ICONVIEW_MARKUP, fileAttributes->file_name_escaped_for_iconview_markup_column,
                       -1);
}

static gboolean delayed_refreshAll()
{
  refresh_store();
  delayedRefresh_GSourceID=0;
  return G_SOURCE_REMOVE;
}

static gboolean inotify_handler(gint fd, GIOCondition condition)
{
   char buffer[(sizeof(struct inotify_event)+16)*1024];
   int len=0, i=0;
   int eventCount=0;
   gboolean refreshImediately=FALSE;
   gboolean refreshDelayed=FALSE;

   len=read(fd, buffer, sizeof(buffer));
   if (len<0) {
      g_warning("inotify_handler: inotify read error\n");
      return TRUE;
   }

   if (pauseInotifyHandler) return TRUE;
   
   while (i<len) {
      struct inotify_event *event=(struct inotify_event *) (buffer+i);

      if (SearchResultViewInsteadOfDirectoryView) {
	g_debug("Inotify event unhandled in search result view.");
	return TRUE;
      }
      
      if (event->len && !ignored_filename(event->name)) {
            if (event->mask & IN_CREATE)
               inotify_insert_item(event->name, event->mask & IN_ISDIR);
            else
               refreshDelayed=TRUE; /* Item IN_CLOSE_WRITE, deleted or moved */
      }
      if (event->mask & IN_IGNORED) /* Watch changed i.e. rfm_curPath changed */
         refreshImediately=TRUE;

      if (event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF) {
         show_msgbox("Parent directory deleted!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         refreshImediately=TRUE;
      }
      if (event->mask & IN_UNMOUNT) {
         show_msgbox("Parent directory unmounted!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         refreshImediately=TRUE;
      }
      if (event->mask & IN_Q_OVERFLOW) {
         show_msgbox("Inotify event queue overflowed!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         refreshImediately=TRUE;         
      }
      i+=sizeof(*event)+event->len;
      eventCount++;
   }

   g_debug("inotify_handler, refreshDelay:%d, refreshImediately:%d in %d events.",refreshDelayed,refreshImediately,eventCount);
   if (refreshDelayed){  /* Delayed refresh: rate-limiter */
         if (delayedRefresh_GSourceID>0)
            g_source_remove(delayedRefresh_GSourceID);
	 //TODO: Why shall we remove the existing timer here? support there are regular inotify event at a constant interval less then the RFM_INOTIFY_TIMOUT, the timer will always be removed before actual call the refresh function. Is it by design? why?
	 delayedRefresh_GSourceID=g_timeout_add(RFM_INOTIFY_TIMEOUT, delayed_refreshAll, NULL);
   };
   //if both refreshdelayed and refreshimediately, i guess refreshdelayed will be removed imediately in rfm_stop_all by the refreshimediately below.
   if (refreshImediately){ /* Refresh imediately: refresh_store() will remove delayedRefresh_GSourceID if required */
	 g_debug("refresh_store imediately from inotify_handler");
         refresh_store();
   }
   return TRUE;
}

static gboolean init_inotify()
{
   rfm_inotify_fd = inotify_init();
   if ( rfm_inotify_fd < 0 ) return FALSE;
   else g_unix_fd_add(rfm_inotify_fd, G_IO_IN, inotify_handler, NULL);
   return TRUE;
}

static gboolean mounts_handler(GUnixMountMonitor *monitor)
{
   GList *listElement;
   RFM_FileAttributes *fileAttributes;
   GHashTable *mount_hash=get_mount_points();

   if (mount_hash==NULL) return TRUE;
   listElement=g_list_first(rfm_fileAttributeList);
   while (listElement != NULL) { /* Check if there are mounts in the current view */
      fileAttributes=(RFM_FileAttributes*)listElement->data;
      if (fileAttributes->is_mountPoint==TRUE)
         break;
      if (fileAttributes->is_dir==TRUE && g_hash_table_lookup(mount_hash, fileAttributes->path)!=NULL)
         break;
      listElement=g_list_next(listElement);
   }
   if (listElement!=NULL)
      refresh_store();

   g_hash_table_destroy(mount_hash);
   return TRUE;
}

static GHashTable *get_mount_points(void)
{
   FILE *fstab_fp=NULL;
   FILE *mtab_fp=NULL;
   struct mntent *fstab_entry=NULL;
   struct mntent *mtab_entry=NULL;
   GHashTable *mount_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

   if (mount_hash==NULL) return NULL;

   fstab_fp=setmntent("/etc/fstab", "r");
   if (fstab_fp!=NULL) {
      fstab_entry=getmntent(fstab_fp);
      while (fstab_entry!=NULL) {
         g_hash_table_insert(mount_hash, g_strdup(fstab_entry->mnt_dir), g_strdup("0\0"));
         fstab_entry=getmntent(fstab_fp);
      }
      endmntent(fstab_fp);
   }

   mtab_fp=setmntent("/proc/mounts", "r");
   if (mtab_fp!=NULL) {
      mtab_entry=getmntent(mtab_fp);
      while (mtab_entry!=NULL) {
         g_hash_table_insert(mount_hash, g_strdup(mtab_entry->mnt_dir), g_strdup("1\0"));
         mtab_entry=getmntent(mtab_fp);
      }
      endmntent(mtab_fp);
   }
   
   return mount_hash;   /* Must be freed after use: g_hash_table_destroy() */
}

static void free_default_pixbufs(RFM_defaultPixbufs *defaultPixbufs)
{
   g_object_unref(defaultPixbufs->file);
   g_object_unref(defaultPixbufs->dir);
   g_object_unref(defaultPixbufs->symlinkDir);
   g_object_unref(defaultPixbufs->symlinkFile);
   g_object_unref(defaultPixbufs->unmounted);
   g_object_unref(defaultPixbufs->mounted);
   g_object_unref(defaultPixbufs->symlink);
   g_object_unref(defaultPixbufs->dimensionDir);
   g_object_unref(defaultPixbufs->symlinkDimensionDir);
   g_free(defaultPixbufs);
}

gchar** replace_shell_cmd_buffer(gchar **strv, gchar *buffer) {
   int len = g_strv_length(strv);
   for (int i = 0; i<len; i++) {
        if (strcmp(strv[i], "shell_cmd_buffer") == 0) {
	  g_free(strv[i]); strv[i]=buffer;
        }
   }
   return strv;
}

static NO_FREE gchar** stdin_command_bash(CALLER_OWN gchar* user_input_cmd, gboolean inNewVT) {
  gchar** ret;
  gchar* shell_cmd_buffer = calloc(ARG_MAX, sizeof(gchar));
  gchar* rfm_alias = getenv("RFM_ALIAS");
  char tempfilename[256]="/tmp/RFM_COMMAND_XXXXXX";
  if (rfm_alias!=NULL && strcmp(rfm_alias,"")!=0){
    // 使用 mkstemp 创建临时文件
    int fd = mkstemp(tempfilename);
    if (fd == -1) {
      g_warning("mkstemp failed");
      return NULL;
    }
    // 将 user_input_cmd 写入临时文件
    ssize_t to_write = strlen(user_input_cmd);
    ssize_t written = write(fd, user_input_cmd, to_write);
    if (written != to_write) {
        g_warning("write to %s failed", tempfilename);
        close(fd);
        return NULL;
    }
    close(fd);
  }

  if (inNewVT){
    if (rfm_alias!=NULL && strcmp(rfm_alias,"")!=0) snprintf(shell_cmd_buffer,PATH_MAX,"source %s; set -o history; source %s;read -p '\033[33m%s\033[0m'; exit \$? 2>/dev/null", rfm_alias, tempfilename, PRESS_ENTER_TO_CLOSE_WINDOW);
    else snprintf(shell_cmd_buffer,PATH_MAX,"set -o history; %s;read -p '\033[33m%s\033[0m'; exit \$? 2>/dev/null", user_input_cmd, PRESS_ENTER_TO_CLOSE_WINDOW);
    //without set -o history and "bash -i", bash builtin history command will not show results.
    //Seems to be by design, but why? for the nu shell, nu -c "history" just works
    //without exit, rfm will quit after commands such as ls, nano, but commands such as echo 1 will work. if you change exit to sleep 20, even echo 1 will make rfm terminate. Are there any race condition here?
    gchar * term_e = getenv("RFM_TERM_E");
    if (term_e!=NULL && strcmp(term_e,"")!=0) ret = g_strdupv(stdin_cmd_template_bash_newVT_nonHold_with_e);
    else ret = g_strdupv(stdin_cmd_template_bash_newVT_nonHold);
  }else {
    if (rfm_alias!=NULL && strcmp(rfm_alias,"")!=0) snprintf(shell_cmd_buffer,PATH_MAX,"source %s; set -o history; source %s; exit \$? 2>/dev/null",rfm_alias, tempfilename);
    else snprintf(shell_cmd_buffer,PATH_MAX,"set -o history; %s; exit \$? 2>/dev/null",user_input_cmd);
    ret = g_strdupv(stdin_cmd_template_bash);
  }
  return replace_shell_cmd_buffer(ret, shell_cmd_buffer);
}
static NO_FREE gchar** stdin_command_nu(CALLER_OWN gchar* user_input_cmd, gboolean inNewVT) {
//TODO: add read -p into shell_cmd_buffer when inNewVT like stdin_command_bash
  gchar** ret;
  gchar* shell_cmd_buffer = calloc(ARG_MAX, sizeof(gchar));
  snprintf(shell_cmd_buffer,PATH_MAX,"%s",user_input_cmd);
  gchar * term_e = getenv("RFM_TERM_E");
  ret = inNewVT? ((term_e!=NULL && strcmp(term_e,"")!=0) ? stdin_cmd_template_nu_newVT_nonHold_with_e : stdin_cmd_template_nu_newVT_nonHold ) : stdin_cmd_template_nu;
  return replace_shell_cmd_buffer(ret, shell_cmd_buffer);
}


#ifdef GNU_readline
static void add_history_timestamp(){
    time_t now;
    struct tm *local;
    char iso_date[30];
 
    // 获取当前时间
    time(&now);
    // 转换为本地时间
    local = localtime(&now);
    // 格式化为ISO日期时间
    if (strftime(iso_date, sizeof(iso_date), "# %Y-%m-%dT%H:%M:%S%z", local) == 0) {
        g_warning("strftime failed\n");
    }else add_history_time(iso_date);
}
#endif

void save_current_history() {
    if (history_entry_added > 0) {
#ifdef GNU_readline
        int e = append_history(history_entry_added, rfm_historyFileLocation);
        if (e!= 0) {
	  g_warning("failed to append_history, error code:%d",e);
	  if (e==2){ // 在我的环境里，文件不存在时返回2,所以下面调用 write_history 函数
	    e = write_history(rfm_historyFileLocation);
	    if (e!=0) g_warning("failed to write_history, error code:%d",e);
	  }
	}
#endif
        g_debug("save command history to: %s",rfm_historyFileLocation);
    }
#ifdef reedline
    reedline_save_history(reedline_handle);
#endif
}

void load_history_for_prompt() {
#ifdef GNU_readline
    clear_history();
    rfm_historyFileLocation = g_build_filename(rfm_historyFile_ParentDirectory, current_stdin_cmd_interpretor_pointer->HistoryFilename ? current_stdin_cmd_interpretor_pointer->HistoryFilename : ".rfm_history", NULL);
    int e = read_history(rfm_historyFileLocation);
    if (e!=0) g_warning("failed to read_history(%s) error code:%d.",rfm_historyFileLocation,e);
#endif
    g_debug("read command history from: %s",rfm_historyFileLocation);
    history_entry_added = 0;
}

static void add_history_after_stdin_command_execution(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution){
#ifdef GNU_readline           
          add_history(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str);
	  add_history_timestamp();
#endif
          history_entry_added++;
	  if (OriginalReadlineResult!=NULL && g_strcmp0(OriginalReadlineResult, readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str)!=0){ //with rfm -x , Originalreadlineresult can be null here
#ifdef GNU_readline
	    add_history(OriginalReadlineResult);
	    add_history_timestamp();
#endif
            history_entry_added++;
	  }
          g_string_free(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution,TRUE);
}

static void exec_stdin_command_in_new_VT(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *child_attribs){
	      // this child_attribs will be freed by g_spawn_wrapper call tree
	      child_attribs->RunCmd = cmd_interpretor_pointer->cmdTransformer(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str, TRUE);
	      child_attribs->runOpts = G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL|G_SPAWN_STDIN_FROM_DEV_NULL;
	      child_attribs->spawn_async = TRUE;
	      g_spawn_wrapper_(NULL, NULL, child_attribs);
              g_debug("exec_stdin_command_in_new_VT(%s,cmd_interpretor name:%s)",readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str,cmd_interpretor_pointer->name);
	      add_history_after_stdin_command_execution(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
}

static gboolean exec_stdin_command(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *childAttribs){
  gboolean ret = FALSE;
  
  if (readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution!=NULL){ //this means it's not initial run after rfm start, so i should first run cmd from previous readline here
#ifdef PythonEmbedded
	  if (pyProgramName!=NULL && g_strcmp0(cmd_interpretor_pointer->name,"PythonEmbedded")==0){
	        ret = (PyRun_SimpleString(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str)==0);
		add_history_after_stdin_command_execution(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
		return ret;
	  }
#endif
          childAttribs->RunCmd = cmd_interpretor_pointer->cmdTransformer(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str,FALSE);
          if (SearchResultTypePointer) {
	    if (childAttribs->exec_stdin_command_in_gtk_thread){
	        childAttribs->runOpts |= G_SPAWN_SEARCH_PATH|G_SPAWN_CHILD_INHERITS_STDIN|G_SPAWN_CHILD_INHERITS_STDERR;
	        childAttribs->output_read_by_program = TRUE;
	        childAttribs->g_idle_add_once_to_run_CallBackFunc = TRUE;
	        childAttribs->customCallBackFunc = update_SearchResultFileNameList_and_refresh_store;
	        childAttribs->no_free = TRUE;
	        ret = g_spawn_wrapper_(NULL, NULL, childAttribs);
	    }else{
	    childAttribs->g_idle_add_once_to_run_CallBackFunc = TRUE;
	    childAttribs->output_read_by_program = TRUE;
	    childAttribs->customCallBackFunc = update_SearchResultFileNameList_and_refresh_store;
	    childAttribs->runOpts = G_SPAWN_SEARCH_PATH|G_SPAWN_STDIN_FROM_DEV_NULL;
	    childAttribs->setsid = TRUE;//search 后台进程有可能运行时间比较长，为了防止和终端输入争抢，设置这里，所以同时就不设置 G_SPAWN_CHILD_INHERITS_STDIN|G_SPAWN_CHILD_INHERITS_STDOUT,但这也意味着执行search的过程就无法和用户通过命令行交互
	    childAttribs->no_free = TRUE; //update_SearchResultFileNameList_and_refresh_store 要多次调用SearchResultTypeProcessingFunc, 可能会重复使用这个childAttribs,多次穿过 g_spawn_wrapper调用, 在update_SearchResultFileNameList_and_refresh_store 调用的最后置为 FALSE.
	    ret = g_spawn_wrapper_(NULL, NULL, childAttribs);
	    childAttribs->name = strdup(SearchResultTypePointer->name);
	    }
	  }else{
	    if (cmd_interpretor_pointer->customCallBackFunc){
	      childAttribs->spawn_async = TRUE; childAttribs->keep_total_buffer_in_show_child_output = TRUE; childAttribs->customCallBackFunc = cmd_interpretor_pointer->customCallBackFunc; //这一行设置被用作在ExecCallback_freeChildattribs 里面 g_thread_new readline thread 的一个条件，这组条件也可以看作有parse_and_exec_command_in_gtk_thread 中的条件 (effective_cmd_interpretor_pointer->customCallBackFunc && !execCmdInNewVT && !execStdinCmdInNewVT) 得来
	      childAttribs->g_idle_add_once_to_run_CallBackFunc = TRUE;
	      childAttribs->runOpts = G_SPAWN_SEARCH_PATH|G_SPAWN_CHILD_INHERITS_STDIN;
	    }else if (childAttribs->output_read_by_program){ childAttribs->runOpts |= G_SPAWN_SEARCH_PATH|G_SPAWN_CHILD_INHERITS_STDIN;
	    }else childAttribs->runOpts |= G_SPAWN_SEARCH_PATH|G_SPAWN_CHILD_INHERITS_STDIN|G_SPAWN_CHILD_INHERITS_STDOUT|G_SPAWN_CHILD_INHERITS_STDERR;
	    ret = g_spawn_wrapper_(NULL, NULL, childAttribs);
	  }
	  add_history_after_stdin_command_execution(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
  }else {
    g_debug("exec_stdin_command(NULL,%s)",cmd_interpretor_pointer->name);
    if (!(childAttribs->rfmEditFilePath)) free_child_attribs(childAttribs); //rfmEditfilepath 有值得话，需要传递到readlineInSeperateThread里
  }
  return ret;
}

//this runs in gtk main thread
static void exec_stdin_command_with_SearchResultAsInput(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution, gboolean from_stdin_readline, NO_FREE stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs *childAttribs){
            if (readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution==NULL){
              free_child_attribs(childAttribs);
	      readlineThread = g_thread_new("readline", readlineInSeperateThread, NULL);
	      return;
	    }

            if (!SearchResultViewInsteadOfDirectoryView) Switch_SearchResultView_DirectoryView();

	    childAttribs->RunCmd = cmd_interpretor_pointer->cmdTransformer(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str,FALSE);
	    childAttribs->stdIn_fd = -1; //set to non-zero, and will be set to a valid fd by g_spawn_async_with_pipes

            gboolean export_filepath_only = (childAttribs->customCallBackFunc == InIconView);
            //InIconview 本来是作为showCondition函数，用来控制文件集合操作上下文菜单项只在IconView显示的，以便在导出当前查询结果的时候只导出文件路径列。但用下来发觉总是要切换到Iconview操作麻烦，又不想在config.h定义里面为文件菜单定义扩展出额外的属性列，就把InIconView函数放在func列作为标志使用，func列在file_menu_exec 函数中会被赋值给childAttribs->customCallBackFunc. 下面一句覆盖掉前先把标志转移到 export_filepath_only. 也许把这个InIconView函数名改为export_filepath_only函数更好，保留InIconview名字是考虑到也许将来别的地方会用到这个函数 

            childAttribs->customCallBackFunc =
#ifdef reedline
                                               g_spawn_async_callback_to_new_readline_thread; //reedline 下，!from_stdin_readline的时候 readlineThread 也会被interrupt_readline掉
#endif
#ifdef GNU_readline
	                                       from_stdin_readline ? g_spawn_async_callback_to_new_readline_thread : ( SearchResultTypePointer? update_SearchResultFileNameList_and_refresh_store : NULL); //如果不是从readline获取的命令，callback里面就不能包含new readline，否则会超过一个readline线程在运行.
#endif
	    if (SearchResultTypePointer) {
              childAttribs->g_idle_add_once_to_run_CallBackFunc = TRUE; //TODO：目前函数是在 gtk main thread 运行的，所以这一行应该可以不要，加一下避免以后改动当前函数运行环境导致出错
	      childAttribs->output_read_by_program = TRUE;
	      childAttribs->no_free = TRUE;
	      childAttribs->setsid = TRUE;
	      //}else childAttribs->runOpts = G_SPAWN_CHILD_INHERITS_STDOUT | G_SPAWN_CHILD_INHERITS_STDERR;
	      //不能保证writeSearchResultIntoFD执行的时候不向命令窗口输出内容,不知道子进程输出和writeSearchResultIntoFD同时争抢命令窗口会发生什么. 所以不如让 child_supervisor 读取子进程输出 fd 后再显示,至少都是在 gtk main loop 里面输出
	    }
	    if (!g_spawn_async_with_pipes_wrapper(childAttribs->RunCmd, childAttribs)) {
		g_warning("Failed to spawn search command");
		if (readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution) {
		    add_history_after_stdin_command_execution(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
		}
	        childAttribs->no_free = FALSE;
	        free_child_attribs(childAttribs);
		readlineThread = g_thread_new("readline", readlineInSeperateThread, NULL);//SearchResultAsInput情况下，g_spawn 失败,（命令根本就没执行，比如命令没找到），从这里新建readline
	    } else {
	        writeSearchResultIntoFD_in_gtk_thread(childAttribs->stdIn_fd,export_filepath_only);
		add_history_after_stdin_command_execution(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution);
	    }
}

gpointer transferPointerOwnership(gpointer *newOwner, gpointer *oldOwner)
{
  *newOwner=*oldOwner;
  *oldOwner=NULL;
  return *newOwner;
}

static void g_spawn_async_callback_to_new_readline_thread(RFM_ChildAttribs *child_attribs){
  if (SearchResultTypePointer) update_SearchResultFileNameList_and_refresh_store(child_attribs); //这里假定当前方法是在gtk main thread 上运行的,如不是,就得用g_idle_add_once
  readlineThread=g_thread_new("readline", readlineInSeperateThread, NULL);// SearchResultAsInput情况下，命令执行成功，从这里新建 readline;  这种情况下,新的readlineThread下不会再运行exec_stdin_command, 所以命令参数直接传NULL过去就可以了
}

static void writeSearchResultIntoFD_in_gtk_thread(gint fd,gboolean exportFilePathOnly){
  g_debug("writeSearchResultIntoFD(%d)",fd);
  if (SearchResultViewInsteadOfDirectoryView){
    GList* columns=NULL;
    GtkTreeIter iter;
    GList *selectedListElement=NULL;
    GList *stdin_cmd_selection_list = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
    if (stdin_cmd_selection_list) selectedListElement=g_list_first(stdin_cmd_selection_list);

    if (treeview && !exportFilePathOnly){
    gboolean firstColumn=TRUE;
    for (size_t i=0; i<TREEVIEW_COLUMNS_array_size;i++){
      if (TREEVIEW_COLUMNS[i].Show && TREEVIEW_COLUMNS[i].enumCol!=NUM_COLS && (TREEVIEW_COLUMNS[i].gtkCol||no_window)){
	if (firstColumn){
	  dprintf(fd, "%s", TREEVIEW_COLUMNS[i].title); //首列输出列标题
	  firstColumn=FALSE;
	}else dprintf(fd, "%s%s",SearchResultColumnSeperator,TREEVIEW_COLUMNS[i].title); //非首列输出 分隔符 加列标题
      columns=g_list_prepend(columns, &(TREEVIEW_COLUMNS[i].enumCol));
      }
    }
    columns=g_list_reverse(columns);
    }

    if (stdin_cmd_selection_list?
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, selectedListElement->data)
	:gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter)){
    do{

    if (treeview && !exportFilePathOnly){
        if (columns){
	GList* c=columns;
	do{
	  GValue val = G_VALUE_INIT;
	  //g_value_init(&val, G_TYPE_GTYPE);
	  gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, *((enum RFM_treeviewCol *)(c->data)), &val);
	  if (gtk_tree_model_get_column_type((GTK_TREE_MODEL(store)), *((enum RFM_treeviewCol *)(c->data)))==G_TYPE_STRING) { //到目前为止，只有 STRING 和 INT64 两种类型的列被显示，参加config.h里面 treeview 列定义以及对应列在 treeview store new 时候的类型
	      gchar * str_value = g_value_get_string(&val);
	      if (c==columns)//this means the first column
		dprintf(fd, "\n%s", str_value? str_value:""); //首列输出回车 加 值
	      else{
		if (str_value && strstr(str_value, SearchResultColumnSeperator)!=NULL) g_warning(SearchResult_contains_ColumnSeperator, SearchResultColumnSeperator, str_value);
		dprintf(fd, "%s%s",SearchResultColumnSeperator, str_value?str_value:""); //非首列输出 分隔符 加值
	      }
	      if (str_value) g_free(str_value);
	  }else{ //GTYPE_UINT64
	      if (c==columns)//this means the first column
		dprintf(fd, "\n%ld", g_value_get_uint64(&val)); //首列输出回车 加 值
	      else dprintf(fd, "%s%ld",SearchResultColumnSeperator, g_value_get_uint64(&val)); //非首列输出 分隔符 加值
	  }
	  //g_value_unset(&val);
	}while((c=g_list_next(c)));
	}else g_warning("no column shown in view!");

    }else{// !treeview || exportFilePathOnly
      GValue val = G_VALUE_INIT;
      if (treeview || exportFilePathOnly) gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, COL_FULL_PATH, &val);
      else gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, COL_ICONVIEW_MARKUP, &val);

      dprintf(fd, "%s\n", g_value_get_string(&val));
      g_value_unset(&val);
    }

    }while (stdin_cmd_selection_list?
	      ((selectedListElement=g_list_next(selectedListElement)) && gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, selectedListElement->data))
	      :gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
    if (treeview) dprintf(fd, "\n");

    if (stdin_cmd_selection_list) g_list_free_full(stdin_cmd_selection_list, (GDestroyNotify)gtk_tree_path_free);
    if (treeview && columns) g_list_free(columns);
    }else g_warning("empty search result view!");
  }else g_warning("currently not in search result view!");
  close(fd);
}

static void readlineInSeperateThread(RFM_ChildAttribs *childAttribs) {
  g_debug("readlineInSeperateThread in thread %p", readlineThread);
  if (childAttribs && !childAttribs->exec_stdin_command_in_gtk_thread && !execStdinCmdInNewVT) exec_stdin_command(childAttribs->readlineResultString, effective_cmd_interpretor_pointer, childAttribs);
  //这里不用 free childAttribs, 因为如果上一步if条件没满足，没有执行exec_stdin_command ，一定是在gtk_thread里面执行了 exec 的某个版本，但因为是异步的，这里我又不方便加assert判断 childAttribs
  execStdinCmdInNewVT = FALSE;
  
  if(startWithTerm() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)){
#ifdef reedline
    // rfmEdit 其实也可以只在readlineInSeperateThread 里完成 parse 和执行，而不是像现在那样和其他命令一样在gtk线程里面parse,然后绕一个圈子又回到readlineInSeperateThread里执行。现在这样，好处是可以利用 leading_space 消除，当前选中文件补全等 gtk 线程里面的功能，并且将来还有可能扩展
    if (childAttribs && childAttribs->rfmEditFilePath) { rfmEdit(childAttribs->rfmEditFilePath); free_child_attribs(childAttribs); }
#endif
    // Allocate more space for potentially longer AI prompts
    gchar *prompt = calloc(strlen(InheritedPrompt)+256, sizeof(gchar));
    sprintf(prompt,"%s",InheritedPrompt);
#ifdef RFM_AddOn
    if (Addon_Prompt) {
        strcat(prompt, Addon_Prompt);
    } else {
        strcat(prompt,current_stdin_cmd_interpretor_pointer->prompt);
    }
#else
    strcat(prompt, current_stdin_cmd_interpretor_pointer->prompt);
#endif
    //SearchResultTypeIndex=-1; //这个branch之前,一直在这里设成FALSE,没注意到有啥问题,但它改成数组索引后测试发现有race condition
    if (keep_selection_on_view_across_refresh && In_refresh_store) strcat(prompt,exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread ? "?]":"?>");
    else if (ItemSelected==0) strcat(prompt,exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread ? "]":">");
    else strcat(prompt,exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread ? "*]":"*>");

    if (OriginalReadlineResult)
#ifdef reedline
        string_free(OriginalReadlineResult);
#else
        g_free(OriginalReadlineResult);
#endif
    OriginalReadlineResult = NULL;
    int i=0;
    while (
#ifdef GNU_readline
	   rl_done!=77 &&
#endif
	   (OriginalReadlineResult = 
#ifdef GNU_readline
           readline(prompt)
#endif
#ifdef reedline
           reedline_readline(reedline_handle, prompt, &functionPointersForReedlineWrapper)
#endif
	   )==NULL){ g_debug("readline return null, %d",i++);}

#ifdef reedline
      if (strcmp(OriginalReadlineResult, REEDLINE_INTERRUPTED)!=0)
#endif
#ifdef GNU_readline
      if (rl_done == 77) { rl_done=0; g_debug("reset rl_done to 0 in readlineThread %p", readlineThread);}
      else
#endif
	stdin_command_Scheduler = g_idle_add_once(parse_and_exec_stdin_command_in_gtk_thread, OriginalReadlineResult?strdup(OriginalReadlineResult):strdup(""));
    free(prompt);
  }
}


static void ensure_foreground_process(void) {
    int fd = open("/dev/tty", O_RDWR);
    if (fd >= 0) {
        pid_t current_pgid = getpgrp();
        pid_t foreground_pgid = tcgetpgrp(fd);
        // 判断是否在后台运行
        if (current_pgid != foreground_pgid) {
            g_warning("rfm is running in background (current PGID: %d, foreground PGID: %d), trying to bring to foreground", current_pgid, foreground_pgid);
            // 尝试设置为前台进程
            int result = tcsetpgrp(fd, current_pgid);
            if (result == 0) {
                g_debug("Successfully brought rfm to foreground");
            } else {
                g_warning("Failed to bring rfm to foreground: %s", strerror(errno));
            }
        } else {
            g_debug("rfm is already in foreground (PGID: %d)", current_pgid);
        }
        close(fd);
    } else {
        g_warning("Could not open /dev/tty: %s", strerror(errno));
    }
}

/**
 * @brief 显示 rfm 内置命令、命令解释器和搜索结果类型的帮助信息。
 *
 * 此函数处理用户在 rfm 命令行输入的 'help' 命令。
 * 它可以显示通用帮助或特定主题的帮助。
 *
 * @param parsed_msg 指向 wordexp 解析后的命令行参数结构体。
 *                   parsed_msg->we_wordc 是参数数量。
 *                   parsed_msg->we_wordv 是参数字符串数组。
 *                   如果 we_wordc == 1，表示通用帮助请求 ("help")。
 *                   如果 we_wordc > 1，表示特定帮助请求 ("help topic")，
 *                   topic 在 parsed_msg->we_wordv[1] 中。
 * @param readline_result_string_after_file_name_substitution 指向经过文件名替换后的原始命令行 GString。
 *                                                            在此函数中未使用，但可能是内置命令处理函数的标准签名。
 */
static void stdin_command_help(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){
      // 判断是请求通用帮助还是特定命令/主题的帮助
      // 如果 parsed_msg 不为 NULL 且参数数量不等于 1，则认为是特定帮助
      gboolean for_specific_command = (parsed_msg!=NULL && (parsed_msg->we_wordc!=1));

      // 如果是通用帮助请求，打印预定义的通用帮助头信息 (通常定义在 config.h)
      if (!for_specific_command) printf("%s",builtinCMD_Help);

      // --- 打印内置命令 (builtinCMD) 的帮助信息 ---
      // 遍历在 config.h 中定义的 builtinCMD 数组
      for(guint i=0;i<G_N_ELEMENTS(builtinCMD);i++){
        // 如果是通用帮助请求，或者 是特定帮助请求且用户请求的主题与当前命令匹配
        if (!for_specific_command || (for_specific_command && strcmp(parsed_msg->we_wordv[1], builtinCMD[i].cmd)==0)) {
          // 打印命令名称（黄色高亮）和其帮助信息
          printf("    \033[33m%s\033[0m   %s\n", builtinCMD[i].cmd, builtinCMD[i].help_msg); // \033[33m 是黄色, \033[0m 重置颜色
        }
      }
      // --- 打印标准输入命令解释器 (stdin_cmd_interpretors) 的信息 ---
      // 遍历在 config.h 中定义的 stdin_cmd_interpretors 数组
      for(guint i=0;i<G_N_ELEMENTS(stdin_cmd_interpretors);i++){
        // 仅在通用帮助请求时打印此部分信息
        if (!for_specific_command){
          // 打印解释器的激活键（黄色高亮）和其名称
          printf("    \033[33m%s\033[0m   %s\n", stdin_cmd_interpretors[i].activationKey, stdin_cmd_interpretors[i].name);
        }
      }


      // --- 打印搜索结果类型 (searchresultTypes) 的帮助信息 ---
      // 如果是通用帮助请求
      if (!for_specific_command){
        // 打印搜索结果类型的段落标题（绿色高亮）和通用说明文本 (通常定义在 config.h)
        printf("\n\033[32m %s\033[0m\n",RFM_SEARCH_RESULT_TYPES); // \033[32m 是绿色
        printf("%s\n", RFM_SEARCH_RESULT_TYPES_HELP);
      }
      // 遍历在 config.h 中定义的 searchresultTypes 数组
      for(guint i=0;i<G_N_ELEMENTS(searchresultTypes);i++){
        // 如果是通用帮助请求，或者 是特定帮助请求且用户请求的主题与当前类型名称匹配
        if (!for_specific_command || (for_specific_command && strcmp(parsed_msg->we_wordv[1], searchresultTypes[i].name)==0)) {
          // 打印类型名称（蓝色高亮）和其描述
          printf("    \033[34m%s\033[0m   %s\n",searchresultTypes[i].name,searchresultTypes[i].Description); // \033[34m 是蓝色
        }
      }

      // --- 打印去除颜色代码的提示 ---
      // 提供一个示例命令，用户可以使用它来查看不带 ANSI 颜色代码的帮助输出
      printf("\n\n \033[33muse command follows to output current help without color:");
      printf("\n rfm.sh --help|sed 's/\\x1B\\[[0-9;]*m//g'\033[0m\n");
}


static int get_treeviewColumnsIndexByEnum(enum RFM_treeviewCol col){
  for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
    if (TREEVIEW_COLUMNS[i].enumCol==col) return i;
  }
  return -1;
}

static RFM_treeviewColumn* get_treeviewcolumnByGtkTreeviewcolumn(GtkTreeViewColumn *gtkCol){
  for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
    if (TREEVIEW_COLUMNS[i].gtkCol==gtkCol) return &TREEVIEW_COLUMNS[i];
  }
  return NULL;
}

static enum RFM_treeviewCol get_available_ExtColumn(enum RFM_treeviewCol col){
  g_assert(col>=COL_Ext1 && col<NUM_COLS);
  enum RFM_treeviewCol start = col;
  while (col < NUM_COLS && get_treeviewColumnsIndexByEnum(col)>=0) col++;
  if(col == NUM_COLS) g_warning("no available extended column since column enum %d", start);
  return col;
}

//ProcessOnelineForSearchResult_internel 里假设查询处理开始时C1是可用的，但现在要把Dimension对应到扩展列，就从后往前依次获取，通常col参数设成 NUM_COLS-1
static enum RFM_treeviewCol get_available_ExtColumn_from_end(enum RFM_treeviewCol col){
  g_assert(col>=COL_Ext1 && col<NUM_COLS);
  enum RFM_treeviewCol start = col;
  while (col >= COL_Ext1 && get_treeviewColumnsIndexByEnum(col)>=0) col--;
  if(col < COL_Ext1){
    g_warning("no available extended column searching from column enum %d backward", start);
    return NUM_COLS;
  }else return col;
}

static RFM_treeviewColumn* get_treeviewColumnByEnum(enum RFM_treeviewCol col){
  int i = get_treeviewColumnsIndexByEnum(col);
  return i<0 ? NULL : &(TREEVIEW_COLUMNS[i]);
}

static RFM_treeviewColumn* get_treeviewColumnByTitle(char* title){
  for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
    if (g_strcmp0(TREEVIEW_COLUMNS[i].title, title)==0) return &(TREEVIEW_COLUMNS[i]);
  }
  return NULL;
}

void move_array_item_a_after_b(void * array, int index_b, int index_a, uint32_t array_item_size, uint32_t array_length){
                void* temp_array = malloc(array_item_size * array_length);
  
                if ((index_b+1)>index_a){ //第一,第二种情况  比如 2,1 满足 2+1>1
			for(int k=0;k<index_a;k++){   // 0 <- 0    
			  memcpy(temp_array+k*array_item_size, array + k*array_item_size, array_item_size);
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k,k);
			}
			//k=1=col_index
			for(int k=index_a+1;k<index_b+1;k++){  // 1 <- 2
			  memcpy(temp_array+(k-1)*array_item_size, array+k*array_item_size, array_item_size);
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k-1,k);
			}
			//上面最后一次memcpy,treeviewColumn[baseColumnIndex]也已经复制到了 temptreeviewcolumns[basecolumnindex-1]
			memcpy(temp_array+index_b*array_item_size, array+index_a*array_item_size, array_item_size); // 2 <- 1
			g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",index_b,index_a);
			for(int k=index_b+1; k<array_length;k++){
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size);
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k,k);
			}
 		}else{//第四种情况, 比如 1,3 满足 1+1 < 3
			for(int k=0;k<index_b+1;k++){ // 0 <- 0   1 <- 1
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size);
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k,k);
			}
			memcpy(temp_array+(index_b+1)*array_item_size, array+index_a*array_item_size, array_item_size); // 2 <- 3
			g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",index_b+1,index_a);
			for(int k=index_b+1;k<index_a;k++){
			  memcpy(temp_array+(k+1)*array_item_size, array+k*array_item_size, array_item_size); // 3 <- 2
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k+1,k);
			}
			for(int k=index_a+1;k<array_length;k++){
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size); // 4 <- 4
			  g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"[%d] <- [%d]",k,k);
			}
		}
		for(int k=0;k<array_length;k++) //I think i cannot free treeviewcolumns because of the way it's defined in config.h, so i have to copy temptreeviewcolumns back. we may define treeviewcolumns[2,] and use treeviewcolumns[0,] treeviewcolumns[1,] in turn to eliminate this copy. But anyway, not big deal, current way is more readable.
			memcpy(array+k*array_item_size, temp_array+k*array_item_size, array_item_size);

		free(temp_array);

}

static void show_hide_treeview_columns_in_order(CALLER_OWN gchar* order_sequence) {
  	      g_log(RFM_LOG_COLUMN,G_LOG_LEVEL_DEBUG,"        order_seq  %s",order_sequence);
              g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"in log subdomain %s, number in [] is the index for treeviewColumns array, otherwise, is column enum value.",RFM_LOG_COLUMN_VERBOSE);

              gboolean pending_refresh_view = FALSE;
              gboolean pending_refresh_store = FALSE;
              gchar** order_seq_array = g_strsplit_set(order_sequence, ",;", TREEVIEW_COLUMNS_array_size + 2);
	      guint j=0; //用来索引 order_seq_array,下面的do while循环,j从零开始循环一遍到order_seq_array最后一项
	      int target_treeviewColumn_index_for_order_sequence_item_j_to_move_after=-1; //用来索引 treeviewColumns,在下面的do while循环,第一轮,也就是j=0时,会赋值为treeviewColumn_index_for_order_sequence_item_j, 其他情况都通过 ++ 递增
	      do {
		gchar * cmd=get_showcolumn_cmd_from_currently_displaying_columns(); //TODO: cmd is only used in debug level log entry, is it possible to run this only when debug? 这里的意思时说能不能只有当前满足G_LOG_LEVEL_DEBUG 的时候才运行这个函数,省得浪费性能
		g_log(RFM_LOG_COLUMN,G_LOG_LEVEL_DEBUG,"        %s",cmd);
		g_free(cmd);
		g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"order_seq  %s",order_sequence);//把目标行紧接当前状态行显示方便在log里观察		

		//如果当前 order_seq_array 项不为空, 为空且order_seq_array[j+1]!=NULL(表示不是最后一个项,也就是说只能是第一项),则只会执行j++,
		if (g_strcmp0(order_seq_array[j], "")!=0){ //to deal with situation such as ,2 (这时,split后逗号前的项为"") or 2, (这时,splite后逗号后的项为"")
		    int col_enum_with_sign = atoi(order_seq_array[j]);
		    //atoi 失败时返回 0,也就是说这时 order_seq_array[j] 不是数字, 是扩展列显示名比如ImageSize,MailTo等
		    if (col_enum_with_sign==0){
		      col_enum_with_sign = (order_seq_array[j][0]=='-')? -1 : 1;
		      RFM_treeviewColumn* col = get_treeviewColumnByTitle( col_enum_with_sign>0 ? order_seq_array[j] : order_seq_array[j] + 1);//第一位是'-'要从title里去掉
		      if (col==NULL) {
			g_warning("failed to find column %s", col_enum_with_sign>0 ? order_seq_array[j] : order_seq_array[j] + 1);
			g_strfreev(order_seq_array);
			return;
		      }
		      if (col_enum_with_sign>0){ //只有显示列才有必要为其设置 enumCol,若是隐藏操作,没必要
			if (col->enumCol==NUM_COLS){
			  col_enum_with_sign = get_available_ExtColumn(COL_Ext1);
			  if (col_enum_with_sign==NUM_COLS) {
			    g_warning("no available extended columns for %s", order_seq_array[j]);
			    g_strfreev(order_seq_array);
			    return;
			  }
			  col->enumCol=col_enum_with_sign;
			  pending_refresh_store = TRUE;
			}else if (g_hash_table_lookup(DimensionPropertiesHashTable_name_asKey,col->title)){//参见SetViewColumnForDimension. TODO:这个当时就拍脑袋决定 Dimension 设置 enumCol 使用独特的方式，现在看似是work的。 这里上面几行有 get_available_ExtColumn(COL_Ext1)，貌似是在显示列的时候才设置enumCol,维度列能不能也走那条路？
			  col_enum_with_sign = col->enumCol;
			  pending_refresh_store = TRUE;
			}else col_enum_with_sign = col->enumCol;
		      }else if (col->enumCol==NUM_COLS) {
			g_warning("no need to hide column %s, because its enumCol==NUM_COLS", col->title);
			j++;
			continue;
		      }
		      else col_enum_with_sign = col_enum_with_sign * col->enumCol;
		    }
		    
		    guint col_enum_at_order_sequence_item_j = abs(col_enum_with_sign);//col_enum 表示列常数,比如 COL_FILENAME
		    int treeviewColumn_index_for_order_sequence_item_j = get_treeviewColumnsIndexByEnum(col_enum_at_order_sequence_item_j);//col_index 表示col_enum 在treeviewColumns数组里当前的下标
		    g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"j:%d; col_enum_at_order_sequence_item_j:%d; treeviewColumn_index_for_order_sequence_item_j:[%d]; target_index_for_order_sequence_item_j_to_move_after:[%d] ",j,col_enum_at_order_sequence_item_j,treeviewColumn_index_for_order_sequence_item_j,target_treeviewColumn_index_for_order_sequence_item_j_to_move_after);
                    
		    if (treeviewColumn_index_for_order_sequence_item_j<0) {
		      g_warning("cannot find column %d.",col_enum_at_order_sequence_item_j);
		      guint c = g_strv_length(order_seq_array);
		      for(guint f=j+1; f<c;f++) g_free(order_seq_array[f]);
		      g_strfreev(order_seq_array);
		      return;
		    }

		    TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].Show = (col_enum_with_sign>=0);
		    if (TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].gtkCol!=NULL){
		      if (icon_or_tree_view!=NULL && treeview) {
			gtk_tree_view_column_set_visible(TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].gtkCol,TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].Show);
			if (j>=1) {
			  //考虑下这个命令： showcolumn -ATime,Git， rfm启动后 ATime 对应的gtkCol是空的，所以下面要找到ATime前一个有效的gtkCol, 注意target_treeviewColumn_index_for_order_sequence_item_j_to_move_after 的值并不改变，也就是TREEVIEW—OLUMNS layout不受影响
			  int target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol = target_treeviewColumn_index_for_order_sequence_item_j_to_move_after;
			  while (target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol>=0 && !TREEVIEW_COLUMNS[target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol].gtkCol) target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol--;
			  gtk_tree_view_move_column_after(GTK_TREE_VIEW(icon_or_tree_view) , TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].gtkCol, target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol<0 ? NULL : TREEVIEW_COLUMNS[target_treeviewColumn_index_for_order_sequence_item_j_to_move_after_with_valid_gtkCol].gtkCol);
			}
		      }
		    }else if (TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].Show && order_sequence!=treeviewcolumn_init_order_sequence && !do_not_show_VALUE_MAY_NOT_LOADED_message_because_we_will_add_GtkTreeViewColumn_later){
		      //printf(VALUE_MAY_NOT_LOADED,col_enum_at_order_sequence_item_j,TREEVIEW_COLUMNS[treeviewColumn_index_for_order_sequence_item_j].title);
		      pending_refresh_view = TRUE;
		    }
		    
		/* 几种情况: */
		/* 第一:   2,1 */
		/* 第二:   3,1 */
		/* 第三:   1,2 */
		/* 第四:   1,3 */
		/* 第五:   ,2 */
		/* 第六:   2,  结尾的逗号表示2后面都不显示 */
		/* 从k=0开始复制,首先j==0这个do while循环,只给baseColumnIndex赋值,除了第五种情况下,baseColumnindex保持-1, 注意不是因为前面g_warning里面没找到输入错误的enum, 而是因为 do 后面第一个 customReorderRelation[j]==""的判断,导致这一轮do循环直接空转*/
		/* 当j>=1时,我们需要完成 basecolumnindex+1 <- col_index 的复制, 因此,在下标小于 min(basecolumnindex+1, col_index)时,直接 k<-k 复制就可以了  */
		/* 另外baseColumnIndex==col_index时,也就是类似第三种情况,我们无需调整位置,这轮do while 循环只需更新下baseColumnInex就可以了 */
		    if (j>=1 && (target_treeviewColumn_index_for_order_sequence_item_j_to_move_after+1!=treeviewColumn_index_for_order_sequence_item_j)){
		      g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"target_index_for_order_sequence_item_j_to_move_after+1 <- treeviewColumn_index_for_order_sequence_item_j: [%d] <- [%d]",target_treeviewColumn_index_for_order_sequence_item_j_to_move_after+1,treeviewColumn_index_for_order_sequence_item_j);
		      //把目前处于treeviewColumn_index_for_order_sequence_item_j位置的列移动到target_treeviewColumn_index_for_order_sequence_item_j_to_move_after后面,也就是target_treeviewColumn_index_for_order_sequence_item_j_to_move_after+1的位置
		      move_array_item_a_after_b(TREEVIEW_COLUMNS, target_treeviewColumn_index_for_order_sequence_item_j_to_move_after, treeviewColumn_index_for_order_sequence_item_j, sizeof(RFM_treeviewColumn), TREEVIEW_COLUMNS_array_size);
		    }
		  
		    if (j>=1) target_treeviewColumn_index_for_order_sequence_item_j_to_move_after++;
		    else target_treeviewColumn_index_for_order_sequence_item_j_to_move_after = treeviewColumn_index_for_order_sequence_item_j;

		    
		// emacs 里用退格键删除下一行首的右括号,再加回去,就会在minibuffer里显示右括号匹配的左括号行		    
                } else if (order_seq_array[j+1]==NULL) { // (g_strcmp0(order_seq_array[j],"")==0) and the last elements in case 2,
		//根据匹配if条件推断当前 order_seq_array 项为"",且后一项为NULL,说明是数组最后一项,跟在分隔副逗号后面且没内容
		    g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"j:%d; order_seq_array[j+1]==NULL",j);
                    for (size_t i = target_treeviewColumn_index_for_order_sequence_item_j_to_move_after + 1;
                         i < TREEVIEW_COLUMNS_array_size; i++) {
		      TREEVIEW_COLUMNS[i].Show = FALSE;
		      if (TREEVIEW_COLUMNS[i].gtkCol!=NULL && icon_or_tree_view!=NULL && treeview) gtk_tree_view_column_set_visible(TREEVIEW_COLUMNS[i].gtkCol, FALSE);
		      g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"%d invisible after ending ','",TREEVIEW_COLUMNS[i].enumCol);
                    }

                } else g_log(RFM_LOG_COLUMN_VERBOSE,G_LOG_LEVEL_DEBUG,"j:%d; order_seq_array[j]:%s",j,order_seq_array[j]);
		
                j++;
	
              } while (order_seq_array[j]!=NULL);
	      g_strfreev(order_seq_array);
              if (pending_refresh_store) g_idle_add_once(refresh_store, NULL);
	      else if (pending_refresh_view) refresh_view(FALSE);
}

static MEM_ALLOC gchar* get_current_column_layout(gboolean title){
            gchar *column_layout = calloc(TREEVIEW_COLUMNS_array_size * (title?(ColumnTitleMaxLen+2):5)+1, sizeof(char));// we suppose no more than 999 treeview_columns, and ",-999" takes 5 chars
            for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++){
	      if (TREEVIEW_COLUMNS[i].enumCol==NUM_COLS) continue;
	      if (title){
		char onecolumn[ColumnTitleMaxLen+2];
		snprintf(onecolumn,ColumnTitleMaxLen+2,",%s%s",(TREEVIEW_COLUMNS[i].Show?"":"-"),TREEVIEW_COLUMNS[i].title);
		strcat(column_layout, onecolumn);
	      }else{
		char onecolumn[6];//5 chars for onecolumn + ending null
		snprintf(onecolumn,6,",%4d", TREEVIEW_COLUMNS[i].enumCol * (TREEVIEW_COLUMNS[i].Show?1:-1));
		strcat(column_layout, onecolumn);
	      }
	    }
	    if (!g_str_has_suffix(column_layout, ",")) strcat(column_layout,",");
            return column_layout;
}

static MEM_ALLOC gchar* get_showcolumn_cmd_from_currently_displaying_columns(){
            gchar* column_layout = get_current_column_layout(TRUE);
	    gchar* showColumnHistory = g_strconcat("showcolumn ",column_layout,NULL);
            g_free(column_layout);
	    return showColumnHistory;
}

static void sort_on_column(wordexp_t * parsed_msg){
  RFM_treeviewColumn* col;
  int col_enum = atoi(parsed_msg->we_wordv[1]);

  if (col_enum==0){ //atoi failed, so column name instead of column enum used in command
    col = get_treeviewColumnByTitle(parsed_msg->we_wordv[1]);
  }else {
    col = get_treeviewColumnByEnum(col_enum);
  }

  if (col==NULL)
    g_warning("invalid column:%s",parsed_msg->we_wordv[1]);
  else if (col->enumCol==NUM_COLS)
    g_warning("column %s don't have associated storage column", col->title);
  else sort_on_column_header(NULL, col);
}

static void print_columns_status(wordexp_t * parsed_msg){
            printf(CURRENT_COLUMN_STATUS,current_sort_column_id,current_sorttype);
	    for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++)
	      if (TREEVIEW_COLUMNS[i].enumCol<NUM_COLS)
		printf("%4d: %s%s\n", TREEVIEW_COLUMNS[i].Show? TREEVIEW_COLUMNS[i].enumCol : (-1)*TREEVIEW_COLUMNS[i].enumCol,TREEVIEW_COLUMNS[i].title, (current_sort_column_id==TREEVIEW_COLUMNS[i].enumCol?(current_sorttype==GTK_SORT_ASCENDING?"{v}":"{^}"):""));
	      else if (TREEVIEW_COLUMNS[i].ValueFunc!=getExtColumnValueFromHashTable){//如果是C1,C2,...C60这样的列且还没映射到store, 就不要显示了，否则显示太长，用户需要往上翻看实际关心的列标题。除了用这种方式判断，有没有更好的方法判断这种扩展列？ 如果是getDimensionValueFromHashTable,則意味這 enumCol<NUM_COLS, 因為已經分配了
	        g_assert(!TREEVIEW_COLUMNS[i].Show);
		printf("    : %s\n",TREEVIEW_COLUMNS[i].title);
	      }

	    printf(SHOWCOLUMN_USAGE);
	    gchar* cmd=get_showcolumn_cmd_from_currently_displaying_columns();
#ifdef GNU_readline
	    add_history(cmd);
	    add_history_timestamp();
#endif
            history_entry_added++;
	    g_free(cmd);  
}


// Use enum int instead of char* if we need to hardcode some column displaying
// sequence in code, so that it will not cause bug when we adjust column enum
// definition
// Use leading and ending INT_MAX to represent leading and ending ',' used in
// char* order_sequence
//
// 看上去似乎内部用本函数, 外部用show_hide_treeview_columns_in_order
// 来调用本函数比较合理,但是,show_hide_treeview_columns_in_order能工作蛮久了,又比较复杂,不想动;
// 更重要的是,它可以不依赖stdarg.h,而本函数需要用到变长参数列表,所以把本函数放到外层也合理
static void show_hide_treeview_columns_enum(int count, ...){
    gchar* restrict order_seq=calloc(TREEVIEW_COLUMNS_array_size*5+13, sizeof(char));
    va_list valist;
    va_start(valist, count);
    for (int i = 0; i < count; i++) {
        int arg_i = va_arg(valist, int);
	char onecolumn[20];
	if (i==0 && arg_i==INT_MAX){ //for first argument, it means leading ',' if it is INT_MAX
	  order_seq=strcat(order_seq, strdup(","));
	}else if (i==(count-1) && arg_i!=INT_MAX){ //for last argument, if it is not INT_MAX, remove the ending ','
	  int len=strlen(order_seq);
	  order_seq[len-1]=0;
	}else if (arg_i!=INT_MAX){
	  snprintf(onecolumn,20, "%4d,", arg_i); //the leading space just mean to make the log entry align and more readable
	  order_seq=strcat(order_seq, onecolumn);
	}
    }
    va_end(valist);
    show_hide_treeview_columns_in_order(order_seq);
    g_free(order_seq);
}

static void cmdSearchResultColumnSeperator(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){
	if (parsed_msg->we_wordc==1) printf("%s\n",SearchResultColumnSeperator);
	else snprintf(SearchResultColumnSeperator,32,"%s",parsed_msg->we_wordv[1]);
}

static void cmdThumbnailsize(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){
	if (parsed_msg->we_wordc==2){
	    guint ts = atoi(parsed_msg->we_wordv[1]);
	    if (ts>0) {
	      RFM_THUMBNAIL_SIZE=ts;
	      snprintf(thumbnailsize_str,8, "%dx%d",RFM_THUMBNAIL_SIZE,RFM_THUMBNAIL_SIZE);
	    }else g_warning("invalid thumbnailsize");
	}else printf("%d\n",RFM_THUMBNAIL_SIZE);
}

static void cmdToggleInotifyHandler(wordexp_t *parsed_msg,GString *readline_result_string_after_file_name_substitution) {
	  pauseInotifyHandler = !pauseInotifyHandler;
	  if (pauseInotifyHandler) printf("InotifyHandler Off\n"); else printf("InotifyHandler On\n");        
}

static void cmdPagesize(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){
        if (parsed_msg->we_wordc==2){
	  set_pagesize_for_searchresultview(parsed_msg->we_wordv[1]);
	}else printf("%d\n",PageSize_SearchResultView);
}

static void cmdSort(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){
	  if (parsed_msg->we_wordc==1) print_columns_status(parsed_msg);
	  else sort_on_column(parsed_msg);
}

static void cmd_glog(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution) {
	  if (parsed_msg->we_wordc>1){
	    if (g_strcmp0(parsed_msg->we_wordv[1],"off")==0)
	      g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	    else if (g_strcmp0(parsed_msg->we_wordv[1],"on")==0)
	      g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, g_log_default_handler, NULL);
	    else printf("Usage: glog off|on\n");
	  }else
	    printf("Usage: glog off|on\n");
}

static void cmd_showcolumn(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution) {
	  if (parsed_msg->we_wordc==1){
	    print_columns_status(parsed_msg);
	  }else{
            for(guint i=1;i<parsed_msg->we_wordc;i++){
	      show_hide_treeview_columns_in_order(parsed_msg->we_wordv[i]);
	    }
	  }
}

static void cmd_setenv(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution) {
  if (parsed_msg->we_wordc==3){
#ifdef GLIB280PLUS
    if (strcmp(parsed_msg->we_wordv[1], "G_MESSAGES_DEBUG")==0){
      printf("\n%s\n","G_MESSAGES_DEBUG env variable will not be changed, g_log_writer_default_set_debug_domains will be called instead, see https://gitlab.gnome.org/GNOME/glib/-/commit/e862f34dd8292f6ffcd30e20c22cd04d47531c3a");
      gchar** domains = g_strsplit(parsed_msg->we_wordv[2], ", ", -1);
      g_log_writer_default_set_debug_domains(domains);
      g_strfreev(domains);
    }
#endif
    if (strcmp(parsed_msg->we_wordv[1], "RFM_PAGESIZE")==0){
      set_pagesize_for_searchresultview(parsed_msg->we_wordv[2]);
    }else setenv(parsed_msg->we_wordv[1],parsed_msg->we_wordv[2],1);
  }else printf(SETENV_USAGE);
}

void addToSearchResult(FREE char *fileFullpath) {
        RFM_ChildAttribs *child_attribs=calloc(1, sizeof(RFM_ChildAttribs));
        child_attribs->RunCmd = g_strdupv(Union_SearchResult);
        gchar* cmd = strdup(child_attribs->RunCmd[1]);
        child_attribs->fullpath_list_as_RunCmd_parameters = g_list_prepend(child_attribs->fullpath_list_as_RunCmd_parameters,fileFullpath);
        child_attribs->customCallBackFunc = (void (*)(RFM_ChildAttribs *))InIconView; //这里参见rfm.c exec_stdin_command_with_SearchResultAsInput 中注释
        parse_and_exec_command_in_gtk_thread(cmd, FALSE, &stdin_cmd_interpretors[0], child_attribs);
}

static void cmd_addToSearchResult(CALLER_OWN wordexp_t *parsed_msg,CALLER_OWN  GString *readline_result_string_after_file_name_substitution) {
  if (parsed_msg->we_wordc==2){
    addToSearchResult(g_strdup(parsed_msg->we_wordv[1]));
  }
}

static void call_g_dir_read_name_recursivly_to_get_dir_names_as_dimension_members(GSList **list, const char *base, const char *path) {
    gchar *full = g_build_filename(base, path, NULL);
    GDir *dir = g_dir_open(full, 0, NULL);
    
    if (dir) {
        g_log(RFM_LOG_DIMENSION, G_LOG_LEVEL_DEBUG, "g_dir_open %s return not null",full);
        const gchar *name;
        while ((name = g_dir_read_name(dir))) {
            if (name[0] != '.') {
                gchar *new_path = path[0] ? g_build_filename(path, name, NULL) : g_strdup(name);
                gchar *test = g_build_filename(base, new_path, NULL);
                if (g_file_test(test, G_FILE_TEST_IS_DIR) && !g_file_test(test, G_FILE_TEST_IS_SYMLINK)) {
		    g_log(RFM_LOG_DIMENSION, G_LOG_LEVEL_DEBUG, "g_file_test %s is dir",test);
                    *list = g_slist_append(*list, new_path);
                    call_g_dir_read_name_recursivly_to_get_dir_names_as_dimension_members(list, base, new_path);
                } else {
                    g_free(new_path);
                }
                g_free(test);
            }
        }
        g_dir_close(dir);
    }
    g_free(full);
}

static char **get_rfm_dimension_members_for_one_dimension(const char *dimension_localAddress, int start, int end) {
    //if (start && rl_line_buffer[start-1] != ' ') return NULL;
    GSList *list = NULL;
    call_g_dir_read_name_recursivly_to_get_dir_names_as_dimension_members(&list, dimension_localAddress, "");
    
    int count = g_slist_length(list);
    if (count > 0) {
        char **matches = malloc(sizeof(char *) * (count + 1));
        GSList *iter = list;
        for (int i = 0; iter; iter = iter->next, i++) {
          matches[i] = g_strdup_printf("=%s",(char *)iter->data); //维度值加前缀=
        }
        matches[count] = NULL;
        g_slist_free_full(list,g_free);
        return matches;
    }else return NULL;
}

#ifdef reedline
static GPtrArray* rfm_editor_completion_based_on_search_result(char *text) {
  GPtrArray* ret = g_ptr_array_new();
  int ExtColumnHashTable_column_index_as_ret_row_index = 0;
  int ret_row_index=0;
  if (SearchResultViewInsteadOfDirectoryView){
    	    for(size_t i=0;i<TREEVIEW_COLUMNS_array_size;i++)
	      if (TREEVIEW_COLUMNS[i].enumCol<NUM_COLS && TREEVIEW_COLUMNS[i].Show && TREEVIEW_COLUMNS[i].gtkCol && TREEVIEW_COLUMNS[i].ValueFunc==getExtColumnValueFromHashTable){//TODO:getExtColumnValueFromHashTable情況下，hashtable中包含是已經在當前searchresult中用到的維度值。如果是getDimensionValueFromHashTable, 則hashtable中是所有已經用到的維度值和所有被維度值標記的值,會遠大於所有維度值，有大量重複。這裡，在getDimensionValueFromHashTable情況下，如果能獲得所有維度值比較合適，distinct的
		if (text==NULL || g_strcmp0(text, "")==0){//输入上下文为空时，把查询结果列名作为补全内容
		  g_ptr_array_add(ret, g_strdup_printf("|%s",TREEVIEW_COLUMNS[i].title));//因为linux文件名路径中不可以出现|字符，所以返回项中加|前缀以便和路径补全的结果区分，下同
                }else if (strstr(text, TREEVIEW_COLUMNS[i].title) && ExtColumnHashTable[ExtColumnHashTable_column_index_as_ret_row_index]){ //否则，从列名包含在输入上下文内容中的列对应的ExtColumnHashTable中取值
		  //if (ItemSelected==1){ //TODO: 如果是选中某行，从此行结果中选ret,而不是从整个column中选.
		  //}else{
		    	    GHashTableIter ht_iter;
			    gchar *key;
	                    gchar *value;
	                    g_hash_table_iter_init(&ht_iter, ExtColumnHashTable[ExtColumnHashTable_column_index_as_ret_row_index]);
	                    while (g_hash_table_iter_next(&ht_iter, &key, &value)){
			      if (FileNameIndexInColumnTitles && strcmp(key,"1")==0) continue;
			      gboolean already_in_ret = FALSE;
                              for(int i=ret->len-1;i>=0;i--){
				if (strcmp(g_ptr_array_index(ret, i), value)==0) { already_in_ret=TRUE;break; }
			      }
			      if (!already_in_ret) g_ptr_array_add(ret,g_strdup_printf("|%s",value));
			    }
		  //}
		}
		ExtColumnHashTable_column_index_as_ret_row_index++;
	      }
  }
  return ret;
}

static GPtrArray* rfm_completion_based_on_dimensions(char* text) {
  GPtrArray * ret = g_ptr_array_new();
  GHashTableIter ht_iter;
  gchar *key;
  DimensionProperties* value;
  g_hash_table_iter_init(&ht_iter, DimensionPropertiesHashTable_name_asKey);
  while (g_hash_table_iter_next(&ht_iter, &key, &value)) {
    if (text==NULL || g_strcmp0(text, "")==0){//输入上下文为空
      g_ptr_array_add(ret, g_strdup_printf("}%s",key));  //维度名加前缀}
    }else if (g_strcmp0(text,key)==0){//输入上下文非空,且等于维度名
      g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"rfm_completion_based_on_dimensions(%s) dimension localAddress:%s",text,value->localAddress);
      int len = strlen(value->localAddress);
      g_ptr_array_free(ret,FALSE);
      char** ret_= get_rfm_dimension_members_for_one_dimension(value->localAddress,len,len+1);
      ret = g_ptr_array_new_from_array(ret_, g_strv_length(ret_),NULL,NULL,(GDestroyNotify)g_free);
      g_free(ret_);
    }
  }
  return ret;
}

static gboolean parse_rfmEdit(GString *readline_result_string_after_file_name_substitution ,RFM_ChildAttribs *childAttribs) {
  wordexp_t parsed_msg;
  int wordexp_retval = wordexp(readline_result_string_after_file_name_substitution->str,&parsed_msg,0);
  if (wordexp_retval==0 && g_strcmp0(parsed_msg.we_wordv[0], "rfmEdit")==0 && parsed_msg.we_wordc==2){
    childAttribs->rfmEditFilePath = strdup(parsed_msg.we_wordv[1]);
    wordfree(&parsed_msg);
    return TRUE;
  }else return FALSE;
}

static void rfmEdit(gchar* filepath) {
    // 调用编辑器
    char *error_msg = NULL;
    In_rfm_editor = TRUE;
    int result = rfm_editor_edit_file(&functionPointersForReedlineWrapper, filepath, &error_msg);
    In_rfm_editor = FALSE;

    if (result==2) {
        g_message("文件保存成功: %s", filepath);
    } else if (result==0 && error_msg) {
        g_warning("编辑失败: %s", error_msg);
        g_free(error_msg);
    }
}
#endif


//返回TRUE表示成功匹配为BuiltInCmd并被执行,无论执行成功与否,都不需要在调用Shell来执行了;返回false表示未能识别为builtInCmd,需要进一步处理. 返回true后, GString*参数内存被释放,但调用者仍需将其设为NULL,因为后续处理根据其是否为NULL来判断是否需要继续处理(比如是否需要发送到Shell执行)
static gboolean parse_and_exec_stdin_builtin_command_in_gtk_thread(CALLER_OWN wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution, RFM_ChildAttribs* childAttribs){

        for(guint i=0;i<G_N_ELEMENTS(builtinCMD);i++){
	  if (g_strcmp0(parsed_msg->we_wordv[0], builtinCMD[i].cmd)==0) {
	        builtinCMD[i].action(parsed_msg,readline_result_string_after_file_name_substitution, childAttribs);
	        g_debug("builtin command %s run", builtinCMD[i].cmd);
		add_history_after_stdin_command_execution(readline_result_string_after_file_name_substitution);
		return TRUE;
	  }
	}
	
	//i don't want to call add_history for the following short builtin commands, so i do not move them into the builtinCMD array
        if (g_strcmp0(parsed_msg->we_wordv[0],"setpwd")==0) {
	  setenv("PWD",rfm_curPath,1);
	  printf("%s\n",getenv("PWD"));
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"pwd")==0) {
	  printf("%s\n",getenv("PWD"));
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"quit")==0) {
	  cleanup();
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"/")==0) {
	  switch_iconview_treeview();
	}else if (g_strcmp0(parsed_msg->we_wordv[0],"//")==0) {
	  Switch_SearchResultView_DirectoryView();

        }else{
	  gchar** dimNameAndValue = g_strsplit(parsed_msg->we_wordv[0],"=",2);
	  if (g_strv_length(dimNameAndValue)==2){
	    GHashTableIter iter_for_DimensionPropertiesHashTable;
	    gchar *key;
	    DimensionProperties *value;
	    g_hash_table_iter_init(&iter_for_DimensionPropertiesHashTable,DimensionPropertiesHashTable_name_asKey);
	    while (g_hash_table_iter_next(&iter_for_DimensionPropertiesHashTable, &key, &value)) {
	      if (g_strcmp0(dimNameAndValue[0], key)==0) {
		set_dimension_value_by_creating_soft_links_to_selected_files(dimNameAndValue);
		g_strfreev(dimNameAndValue);
		g_string_free(readline_result_string_after_file_name_substitution,TRUE);
		return TRUE;
	      }
	    }
	  }
	  return FALSE; // parsed_msg->we_wordv[0] does not match any build command
	}
	g_string_free(readline_result_string_after_file_name_substitution,TRUE);
	return TRUE; //execution reaches here if parsed_msg->we_wordv[0] matchs any keyword, and have finished the corresponding logic
}

static void cmd_cd(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution, RFM_ChildAttribs* childAttribs) {
	  if (parsed_msg->we_wordc==2){
	    gchar * addr=parsed_msg->we_wordv[1];
	    g_debug("cd %s", addr);
	    if (g_strcmp0(addr, "..")==0) {
	      up_clicked();
	    }else if (g_strcmp0(addr, ".")==0){

	    }else if (g_strcmp0(addr, "-")==0){
	      set_rfm_curPath(getenv("OLDPWD"));
	    }else{
	      gchar* full_addr = (addr[0]=='/') ? strdup(addr) : g_build_filename(rfm_curPath, strdup(addr), NULL);
	      struct stat statbuf;
              if (stat(full_addr,&statbuf)!=0){
		g_warning("Can't stat %s\n", full_addr);
	      }else{
		if (S_ISDIR(statbuf.st_mode))
		  addr = strdup(full_addr);
		else{
		  addr = g_path_get_dirname(full_addr);
		  g_list_free_full(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],(GDestroyNotify)g_free);
		  filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = NULL;
		  filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView], strdup(full_addr));
		  //note that if user type "cd " with no parameter in searchresultview, we will switch to dir view as code serveral lines bellow. But we won't switch to dir view here.
		}
		set_rfm_curPath(addr);
		g_free(addr);
	      }
	      g_free(full_addr);
	    }
	  }else if (parsed_msg->we_wordc==1){ // cd without parameter
	      if (childAttribs->stdin_cmd_ending_space){
		if (ItemSelected==1){
		  if (selected_file_mode_str[0]=='d'){
		    set_rfm_curPath(rfm_selection_completion);//因为当前函数和selectionChanged函数运行在同一线程里，所以读取rfm_selection_completion不用加锁
		    if (SearchResultViewInsteadOfDirectoryView) Switch_SearchResultView_DirectoryView();
		  }else if (SearchResultViewInsteadOfDirectoryView){
		    SearchResultViewInsteadOfDirectoryView = SearchResultViewInsteadOfDirectoryView^1;
		    g_list_free_full(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],(GDestroyNotify)g_free);
		    filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]=NULL;
		    filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]=g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],strdup(rfm_selection_completion));
		    char * parentdir = g_path_get_dirname(rfm_selection_completion);
		    set_rfm_curPath(parentdir);
		    g_free(parentdir);
		  }
		}else
		  g_warning("file or directory not selected, or invalid");
	      }else { //!stdin_cmd_ending_space
		printf("%s\n",rfm_curPath);
	      }
	  //when we set_rfm_curPath, we don't change rfm environment variable PWD
	  //so, shall we consider update env PWD value for rfm in set_rfm_curPath?
	  //The answer is we should not, since we sometime, with need PWD, which child process will inherit, to be different from the directory of files selected that will be used by child process.
	  //Instead, we use the setpwd command.
	  }//end cd without parameter
}


static void toggle_exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread(){
  exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread = !exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread;
}

// searchresulttype can be matched with index number or name
// you can use >0  or >default or >your_custom_searchresulttype_name_or_index as suffix of your command
// i use the ">" as RFM_SearchResultTypeNamePrefix since the beginning of development, but it can be confused with redirect to file in Bash. So, you mean define it to "}" or anything else you like in config.h
static RFM_SearchResultType* MatchSearchResultType(gchar* readlineResult, RFM_ChildAttribs *childattribs){
            gint len = strlen(readlineResult);
	    if (len<=2) return NULL; // the shortest suffix is >0 or >1 ...

	    for(guint i=0; i<G_N_ELEMENTS(searchresultTypes); i++){

	      char* searchTypeNumberSuffix=calloc(256,sizeof(char));//RFM_SearchResultTypeNamePrefix can be long, but less then 256- 
	      snprintf(searchTypeNumberSuffix,256,"%s%d",RFM_SearchResultTypeNamePrefix,i);
	      //先用searchresultTypes数组下标数字匹配,类似0,1,2代表stdin,stdout,stderr, 数字提供了常用searchresultTypes快速输入简写方式
	      if (g_str_has_suffix(readlineResult, searchTypeNumberSuffix)){
		g_debug("SearchResultTypeIndex:%d; searchResultTypeName:%s",i,searchresultTypes[i].name);
		readlineResult[len-strlen(searchTypeNumberSuffix)]='\0';//remove suffix from readlineResult
		g_free(searchTypeNumberSuffix);
                if (strcmp(SrchRsltType_current, searchresultTypes[i].name)==0){
		  return SearchResultTypePointerForCurrentExistingSearchResult ? SearchResultTypePointerForCurrentExistingSearchResult : &(searchresultTypes[0]);
		}else return &(searchresultTypes[i]);
	      }
	      g_free(searchTypeNumberSuffix);
	      
      	      char* searchTypeNameSuffix=g_strconcat(RFM_SearchResultTypeNamePrefix,searchresultTypes[i].name,NULL);
	      if (g_str_has_suffix(readlineResult, searchTypeNameSuffix)){
		g_debug("SearchResultTypeIndex:%d; searchResultTypeName:%s",i,searchresultTypes[i].name);
		readlineResult[len-strlen(searchTypeNameSuffix)]='\0';//remove suffix from readlineResult
		g_free(searchTypeNameSuffix);
                if (strcmp(SrchRsltType_current, searchresultTypes[i].name)==0){
		  return SearchResultTypePointerForCurrentExistingSearchResult ? SearchResultTypePointerForCurrentExistingSearchResult : &(searchresultTypes[0]);
		}else return &(searchresultTypes[i]);
	      }
	      g_free(searchTypeNameSuffix);
	    }
#ifdef RFM_AddOn
	    GHashTableIter ht_iter;
	    gchar *key;
	    RFM_ADDON *value;
	    g_hash_table_iter_init(&ht_iter, addons);
	    while (g_hash_table_iter_next(&ht_iter, &key, &value)){
	           RFM_ADDON_RESOURCE_TYPE* addon_resource = value->resource_managed_by_addon;
		   for (guint i = 0; i < addon_resource->searchresulttypes_array_size; i++) {
		         char* searchTypeNameSuffix=g_strconcat(RFM_SearchResultTypeNamePrefix,addon_resource->searchresulttypes[i].name,NULL);
			 if (g_str_has_suffix(readlineResult, searchTypeNameSuffix)){
			   g_debug("Addon:%s; searchResultTypeName:%s",value->addon_c_fullpath,addon_resource->searchresulttypes[i].name);
			   readlineResult[len-strlen(searchTypeNameSuffix)]='\0';//remove suffix from readlineResult
			   g_free(searchTypeNameSuffix);

			   childattribs->searchresulttype_addon_dir = g_dirname(value->addon_c_fullpath);
			   return &(addon_resource->searchresulttypes[i]);
			 }
			 g_free(searchTypeNameSuffix);
		   }
	    }
#endif
	    if (strstr(readlineResult, RFM_SearchResultTypeNamePrefix)!=NULL)
	      printf("SearchResultTypePrefix %s in input, and no SearchResultType matched, but this can be just fine. TODO: add a builtin command to list available SearchResultTypes\n",RFM_SearchResultTypeNamePrefix);
	    return NULL;
}

static void set_stdin_cmd_leading_space_true(){
  stdin_cmd_leading_space=TRUE;
}

static void combine_command_with_selected_files_and_setenv(CALLER_OWN GString* readlineResultString, gboolean cmd_end_with_space, gchar** *env, GList* fullpath_list_as_parameter) {
	      // combine runCmd with selected files to get gchar** v
	      // TODO: the following code share the same pattern as g_spawn_wrapper_for_selected_fileList_ , anyway to remove the duplicate code?
	    GList *listElement;
            if (fullpath_list_as_parameter==NULL) fullpath_list_as_parameter = filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView];
	    if (fullpath_list_as_parameter!=NULL) {
	        if (strstr(readlineResultString->str, RFM_selected_filename_list_placeholder)!=NULL){ //把 %% 替换成n个%s，即 %s %s %s
		  int filepath_list_length = g_list_length(fullpath_list_as_parameter);
		  gchar* filename_placehoders = calloc(filepath_list_length * (strlen(RFM_selected_filename_placeholder)+1) +1, sizeof(gchar));
		  for(int i=0;i<filepath_list_length;i++) {
		    strcat(filename_placehoders, RFM_selected_filename_placeholder);
		    strcat(filename_placehoders," ");
		  }
		  g_string_replace(readlineResultString, RFM_selected_filename_list_placeholder, filename_placehoders, 1);
		  g_free(filename_placehoders);
	        }
		listElement=g_list_first(fullpath_list_as_parameter);
	        gchar *env_rfm_selected_files = NULL;
		while(listElement!=NULL) {
		  //if there is %s in msg, replace it with selected filename one by one, otherwise, append filenames to the end.
		  if (cmd_end_with_space){
		    if (strstr(readlineResultString->str, RFM_selected_filename_placeholder) == NULL) g_string_append(readlineResultString, selected_filename_placeholder_in_space);
		    //if file path contains space, wrap path inside ''
		    if (strstr(listElement->data," ") != NULL) g_string_replace(readlineResultString, RFM_selected_filename_placeholder, selected_filename_placeholder_in_quotation, 1);
		    g_string_replace(readlineResultString, RFM_selected_filename_placeholder,listElement->data, 1);
		    printf("%s\r\n",listElement->data);
		  }

		  if (env_rfm_selected_files != NULL) {
		    gchar *new_str = g_strconcat(env_rfm_selected_files, "\n", listElement->data, NULL);
		    g_free(env_rfm_selected_files);
		    env_rfm_selected_files = new_str;
		  } else {
		    env_rfm_selected_files = g_strdup(listElement->data);
		  }

		  listElement=g_list_next(listElement);
		}

		setenv("RFM_SELECTED_FILES",env_rfm_selected_files,1);//selectionChanged 函数维护全局变量stdin_cmd_selection_list，环境变量只有在创建子进程时才用
	        g_free(env_rfm_selected_files);
		if (ItemSelected==1){ //通过环境变量把当前选中的文件的数据(比如grepMatch列)传递给子进程,目前人为限制只有单选文件时起作用,因为多选文件时不同文件的同一列值可能不同
		    *env = g_get_environ();
		    set_env_to_pass_into_child_process_for_view_columns_when_ItemSelected_equals_one(&iter_selectionChanged, env);
		}
            } else {
	      unsetenv("RFM_SELECTED_FILES");
	    }
}  


// this runs in gtk thread
// from_stdin_readline 为 TRUE 时，表示用户从命令窗口输入的字符串，这时忽略 cmd_interpretor_index 参数为调用时的 current_stdin_cmd_interpretor
// from_stdin_readline 为假时，readlineResults 通常是 config.h 里面配置好的命令字符串。此时命令字符串数组头部会配置 cmd_interpretor_index 对应的name
// 目前布尔返回值只在!from_stdin_readline情况下返回 exec_stdin_command 的返回值，用于编译 rfm_local.c 后返回成败
gboolean parse_and_exec_command_in_gtk_thread (FREE gchar * readlineResult, gboolean const from_stdin_readline, NO_FREE const stdin_cmd_interpretor *cmd_interpretor_pointer, RFM_ChildAttribs* childAttribs)
{
        gboolean ret = FALSE;
	stdin_cmd_leading_space=FALSE;
        gboolean execCmdInNewVT=FALSE;//used by file_menu_exec, i.e., not from readline
        effective_cmd_interpretor_pointer = cmd_interpretor_pointer; // Use this for actual execution
        gint len = strlen(readlineResult);
	g_debug ("parse_and_exec_command_in_gtk_thread, RFM_ChildAttribs %p, readline length %u: %s", childAttribs, len, readlineResult);

#ifdef reedline
        if (!from_stdin_readline) reedline_interrupt(reedline_handle);//下面g_thread_join 从效果上相当于 reedline_interrupt 是同步的
#endif
#ifdef GNU_readline
        if (!from_stdin_readline)
            //rl_done = 77; //77这个数没啥特别意义，AI说只要非零就可以，例子给的是1, 我怕有冲突，就设了个特别的数，以便可以比较，知道是这里设的
            childAttribs->setsid = TRUE;//TODO:用户选择文本文件按回车，item_activated 函数调用 bat 命令在终端模拟器内显示文本。 之前reedline 方式下会出现显示只换行不回车的情况，AI的解释是reedline下terminal被设置成raw模式，所以上面调用reedline_interrupt停止reedline。类似，设置rl_done停止gnu readline，但这gnu readline 下会出错，所以注释掉了。另外，在gnu readline 下测试似乎没有只换行不回车的现象，倒是rfm会被自动设成后台进程，需fg命令切回前台，于是这里尝试设置setsid, 简单测试下来rfm不被自动放到后台了，但readline 和 exec_stdin_command 执行的终端输出干扰应该还是存在的，表现为exec_stdin_command执行输出完，回车命令提示符后面有时会多出一些不该出现的内容,以及：bash: 无法设定终端进程组 (-1): 对设备不适当的 ioctl 操作; bash: 此 shell 中无任务控制
#endif
#ifdef reedline
	if (!from_stdin_readline && readlineThread) { g_thread_join(readlineThread); g_debug("g_thread_join finished for readlineThread %p when !from_stdin_readline",readlineThread);readlineThread=NULL; }//we won't have more than one readlineThread running at the same time since we join before new thread here
#endif
	if (len == 0){
	    time_t now_time=time(NULL);
            if ((now_time - lastEnter)<=1){
                lastEnter=now_time;
               	refresh_store();
            }else
                lastEnter=now_time;
	}else{
            //在AI提示符状态下，期望builtin命令能够执行，而一条重要的buildin命令cd(带空格)执行cmd_cd,跳转到查询结果窗口当前选中文件的parent目录，就依赖stdin_cmd_ending_space, 所以把第一次的stdin_cmd_ending_space 判断提前否则如果cmd_interpretor_index ！= stdin_cmd_interpretor_index_for_AI，就直接被跳过了
            childAttribs->stdin_cmd_ending_space = (len>=1 && readlineResult[len-1]==' ');
            while (len >= 1 && readlineResult[len - 1] == ' ') {
              readlineResult[len - 1] = '\0';
              len--;
            } // remove ending space
              //  
            // Check if the command starts with an interpreter prefix to override execution
            gboolean prefix_found = FALSE;
            int prefix_len;
            for (guint i = 0; i < G_N_ELEMENTS(stdin_cmd_interpretors); i++) {

                    prefix_len = strlen(stdin_cmd_interpretors[i].activationKey);
	            if (g_strcmp0(readlineResult, stdin_cmd_interpretors[i].activationKey)==0){
		       save_current_history();
		       current_stdin_cmd_interpretor_pointer = &(stdin_cmd_interpretors[i]);
#ifdef RFM_AddOn
		       g_free(Addon_Prompt);Addon_Prompt = NULL;
		       sprintf(rfm_historyFile_ParentDirectory, "%s", getenv("HOME"));//对于在config.h里面定义的cmd_interpretor,history都保留在home下; 下面切换到Addon里面的cmd_interpretor, 由addon代码处理history切换，这里有可能是从Addon切换回来的，所以要重置到home下
#endif
		       load_history_for_prompt();
		       goto switchToReadlineThread;//如果当前输入命令是选择命令解释器,选择完后就直接进入一轮命令读取,无需再对当前输入命令进一步处理
		    
                    // Check if it has the prefix AND is longer than just the prefix itself
                    }else if (g_str_has_prefix(readlineResult, stdin_cmd_interpretors[i].activationKey) && len > prefix_len) {
                        memmove(readlineResult, readlineResult + prefix_len, len - prefix_len + 1); // Shift string left
                        len = strlen(readlineResult); // Update length
                        effective_cmd_interpretor_pointer = &(stdin_cmd_interpretors[i]); // Set effective interpreter for this command
                        prefix_found = TRUE;
                        g_debug("Detected '%s' prefix. Will run as %s command: %s",stdin_cmd_interpretors[i].activationKey, stdin_cmd_interpretors[i].name, readlineResult);
                        break; // Found the prefix, stop checking
                    }
            }

#ifdef RFM_AddOn
            if (!(prefix_found)){
	      GHashTableIter ht_iter;
	      gchar *key;
	      RFM_ADDON *value;
	      g_hash_table_iter_init(&ht_iter, addons);
	      while (g_hash_table_iter_next(&ht_iter, &key, &value)){
	           RFM_ADDON_RESOURCE_TYPE* addon_resource = value->resource_managed_by_addon;
	           if (addon_resource->check_cmd_interpretor_switch_or_effective_cmd_interpretor){
		       stdin_cmd_interpretor * new_cmd_interpretor = addon_resource->check_cmd_interpretor_switch_or_effective_cmd_interpretor(&readlineResult, &effective_cmd_interpretor_pointer, from_stdin_readline? &execStdinCmdInNewVT:&execCmdInNewVT, &Addon_Prompt, &(childAttribs->cmd_interpretor_addon_dir));//在目前AI_Model addon 的实现中，如果 effective_cmd_interpretor是AI_Model, 就会设置addon_dir
		       if (new_cmd_interpretor){ //切换到AI_model
			   current_stdin_cmd_interpretor_pointer = new_cmd_interpretor;
			   goto switchToReadlineThread;
		       }else if(childAttribs->cmd_interpretor_addon_dir){ //如果不是 new_cmd_interpretor ,但 addon_dir 又不为空，可以推断出一定是找到命令中包含了AI prefix
			   len = strlen(readlineResult); // Update length
			   break;
		       }
		   }
	      }
            }
#endif

	    //正则表达式替换发生在命令解释器识别之后
	    for (guint i=0;i<G_N_ELEMENTS(regex_rules);i++){
	      if (regex_rules[i].pattern_compiled && str_regex_replace(&readlineResult, regex_rules[i].pattern_compiled, regex_rules[i].replacement) && regex_rules[i].action) (regex_rules[i].action)();
	    }
	    len = strlen(readlineResult);

	    //stdin_cmd_ending_space, execStdinCmdInNewVT, SearchResultAsInput 应该也可以通过正则表达式匹配实现.
	    //一方面之前写这些代码的时候还没添加正则表达式匹配的功能
	    //再者有些人可能不喜欢正则表达式,而这些又是对rfm来说比较核心的功能,所以就暂不改用正则表达式识别
	    //目前正则表达式匹配主要是解决一些字母大小写,消除空格等,把用户输入命令规范化成统一定义的格式

            if (g_str_has_suffix(readlineResult, RFM_NEW_TERM_SUFFIX) && len>strlen(RFM_NEW_TERM_SUFFIX)){
	      if (readlineResult[len-strlen(RFM_NEW_TERM_SUFFIX)-1] == '\\'){
		memcpy(&readlineResult[len-strlen(RFM_NEW_TERM_SUFFIX)-1], &readlineResult[len-strlen(RFM_NEW_TERM_SUFFIX)], strlen(RFM_NEW_TERM_SUFFIX)+1);// 如果结尾是'\&', 即用户的确需要&字符在行尾，用\转义,则去掉\, 把后面字符（包括结尾的'\0'）向前移一位，改成'&'
	      }else {
		if (from_stdin_readline) execStdinCmdInNewVT = TRUE;
		else execCmdInNewVT = TRUE;
		readlineResult[len-strlen(RFM_NEW_TERM_SUFFIX)] = '\0'; //remove ending '&'
	      }
      	      len = strlen(readlineResult);
	    }

#ifdef RFM_AddOn
            if (childAttribs->cmd_interpretor_addon_dir){ //如果使用的是Addon cmd interpretor,就无需执行后续计算execStdinCmdInNewVT的代码了，因为rfm和cmd interpretor addon 的沟通都通过 check_cmd_interpretor_switch_or_effective_cmd_interpretor 函数及参数完成。
	    }else{
#endif

	    //if we have searchresulttypesuffix in command, the ending space rule can be confusing: shall we have space after searchresulttype or before searchresulttype? Both will be considered as ending space!
	    childAttribs->stdin_cmd_ending_space = (childAttribs->stdin_cmd_ending_space || (len>=1 && readlineResult[len-1]==' '));
	    while (len>=1 && readlineResult[len-1]==' ') { readlineResult[len-1]='\0'; len--; } //remove ending space

	    if (!execStdinCmdInNewVT && !(!from_stdin_readline && execCmdInNewVT)){
	      SearchResultTypePointer = MatchSearchResultType(readlineResult,childAttribs);
	      len = strlen(readlineResult);
	    }

	    childAttribs->stdin_cmd_ending_space = (childAttribs->stdin_cmd_ending_space || (len>=1 && readlineResult[len-1]==' '));
	    while (len>=1 && readlineResult[len-1]==' ') { readlineResult[len-1]='\0'; len--; } //remove ending space

	    if (g_str_has_prefix(readlineResult, RFM_SEARCH_RESULT_AS_INPUT_PREFIX)) {
	        if ((from_stdin_readline && execStdinCmdInNewVT) || execCmdInNewVT){
		  g_warning("%s does not work with %s for now. TODO: we might save search result into temp file and use it in new terminal emulator in future.", RFM_SEARCH_RESULT_AS_INPUT_PREFIX, RFM_NEW_TERM_SUFFIX);
		  readlineResult[0]='\0';//设置读取字符串为空，执行一个空命令
		}else{
	          childAttribs->SearchResultAsInput=TRUE;
		  for (int i=0;i<len;i++) readlineResult[i]=readlineResult[i + strlen(RFM_SEARCH_RESULT_AS_INPUT_PREFIX)]; //remove the prefix SearchResult|, which has 13 char
		}
	    }
#ifdef RFM_AddOn
	    }//end else (cmd_interpretor_index == stdin_cmd_interpretor_index_for_AI){ 对于输入的 AI 提示词，就不要尝试解析了，
	    //但是下面处理当前选中文件的部分还是需要的，当前选中文件可作为上下文传给AI
#endif
            childAttribs->readlineResultString=g_string_new(strdup(readlineResult));

            combine_command_with_selected_files_and_setenv(childAttribs->readlineResultString, childAttribs->stdin_cmd_ending_space,&(childAttribs->env), childAttribs->fullpath_list_as_RunCmd_parameters);
            
#ifdef RFM_AddOn
	    if (!execStdinCmdInNewVT && !(!from_stdin_readline && execCmdInNewVT) || Addon_Prompt){ //Addon_Prompt不为空实际上就相当于 current_stdin_cmd_interpretor_pointer == AI_Model
	      //TODO：在NewTerm里执行命令就意味着无法运行下面的builtin_commnd,但是AI模式除外，AI模式下可能会在NewTerm里执行命令，但仍然可以执行builtin_command. 用Addon_Prompt 区分状态可能不是很好，现在只有AI模式一个Addon包含 stdin_cmd_interpretor, 所以能用，以后增加类似 Addon 还要重新定义
#else
	    if (!execStdinCmdInNewVT && !(!from_stdin_readline && execCmdInNewVT)){
#endif
              wordexp_t parsed_msg;
	      int wordexp_retval = wordexp(readlineResult,&parsed_msg,0);
	      if (wordexp_retval==0 && parsed_msg.we_wordc>0 && (
#ifdef reedline
					parse_rfmEdit(childAttribs->readlineResultString, childAttribs) ||
#endif
					parse_and_exec_stdin_builtin_command_in_gtk_thread(&parsed_msg, childAttribs->readlineResultString, childAttribs))){ //wordexp_retval 0 means success
		childAttribs->readlineResultString=NULL; //since readlineInseperatethread function will check this, we must clear it here after cmd already been executed.
	      }
	      if (wordexp_retval == 0) wordfree(&parsed_msg);
	    }
	} //end if (len == 0)
 switchToReadlineThread:
        g_free (readlineResult); // Free the original buffer

	assert(startWithTerm()|| !from_stdin_readline);
          // Use effective_cmd_interpretor_index for execution calls
	  if (childAttribs->SearchResultAsInput) exec_stdin_command_with_SearchResultAsInput(childAttribs->readlineResultString, from_stdin_readline, effective_cmd_interpretor_pointer, childAttribs);//c语言推荐的做法是变量free了以后置NULL,但这里exec_stdin_command_with_SearchResultAsInput中异步的g_spawn_返回false后才free childAttribs, 成功得话要等异步callback得话才free childAttribs, 所以这里没法判断是否置NULL。TODO：好的做法似乎应该是传入 &childAttribs, 甚至 free_child_attribs 函数就应该改成接收 &childAttribs 参数， free完后就立刻就地置NULL
	  else if (((exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread || !from_stdin_readline) && !(execStdinCmdInNewVT || (!from_stdin_readline && execCmdInNewVT)))
		   || (current_stdin_cmd_interpretor_pointer==cmd_interpretor_pointer //这一行排除了在命令提示符下单纯输入 d>  b> 等切换cmd_interpretor的情况，这种情况下，不在这里执行 exec_stdin_command, 也就不会free childattribs, 而是需要g_thread_new readlineThread, 参见下面g_thread_new前条件判断
		       && effective_cmd_interpretor_pointer->customCallBackFunc && !execCmdInNewVT && !execStdinCmdInNewVT) ){ //这一行条件用来判断需要使用customCallBackFunction来处理 exec_stdin_command 的输出的情况，比如AI LLM返回的内容，这种情况下，g_spawn_wrapper 用异步，exec_stdin_command 会很早返回，但g_spawn_wrapper 还没完成，为了避免争夺终端模拟器，这时不能开始readline,所以就暂时不要 g_thread_new readline,在 g_spawn_wrapper 的异步callback里面再考虑 g_thread_new readline。另外这一行判断也是保证此时exec_stdin_command在gtk thread里执行，否则在readline thread里执行得话，就不方便控制exec_stdin_command 中g_spawn_wrapper 实际callback完成后才调用 readline函数了。注意之一行判断条件在下面 g_thread_new readline 前也有使用
	    if (childAttribs->readlineResultString){
	           childAttribs->exec_stdin_command_in_gtk_thread = TRUE;//因为上面的组合判断条件越来越复杂了，不方便把等价条件放入readlineInSeperateThread; 并且exec_stdin_command中调用的g_spawn_wrapper可能是异步的，也不能直接在这里释放childAttribs并置NULL,所以这里添加了一个标志属性简化代码
	           ret = exec_stdin_command(childAttribs->readlineResultString, effective_cmd_interpretor_pointer, childAttribs);
	    }
          }
          if ((execStdinCmdInNewVT || (!from_stdin_readline && execCmdInNewVT)) && childAttribs->readlineResultString) exec_stdin_command_in_new_VT(childAttribs->readlineResultString, effective_cmd_interpretor_pointer, childAttribs); // 在AI Mode下输入builtin cmd,如 help, 会因为是AI mode 一开始把 execStdinCmdInNewVT 设成true,所以这里加上 readlineResultString 非NULL判断

          // Pass the original readlineResultString (potentially NULL if handled by builtin) to the next thread
          // The next thread will use the global current_stdin_cmd_interpretor for its prompt
#ifdef reedline
          if (!childAttribs->SearchResultAsInput && (
				       current_stdin_cmd_interpretor_pointer!=cmd_interpretor_pointer // 在本函数开头运行current_stdin_cmd_interpretor_pointer = &(stdin_cmd_interpretors[i]) 或 current_stdin_cmd_interpretor_pointer = new_cmd_interpretor, 接着 goto switchToReadlineThread 时，满足此行条件。
				       || !ret
				       || !(effective_cmd_interpretor_pointer->customCallBackFunc && !execCmdInNewVT && !execStdinCmdInNewVT))) {
#endif
#ifdef GNU_readline
          if (childAttribs->readlineResultString==NULL || (!childAttribs->SearchResultAsInput && from_stdin_readline && (
							      current_stdin_cmd_interpretor_pointer!=cmd_interpretor_pointer
							      || !ret
							      || !(effective_cmd_interpretor_pointer->customCallBackFunc && !execCmdInNewVT && !execStdinCmdInNewVT)))) {
#endif
              assert(readlineThread==NULL);
              if (window) readlineThread=g_thread_new("readline", readlineInSeperateThread, from_stdin_readline?childAttribs:NULL);
          }

        return ret;
}

static void parse_and_exec_stdin_command_in_gtk_thread (gchar * readlineResult){
  if (readlineThread) { g_thread_join(readlineThread); g_debug("g_thread_join finished for readlineThread %p when from_stdin_readline",readlineThread); readlineThread=NULL; }
  RFM_ChildAttribs *childAttribs = calloc(1, sizeof(RFM_ChildAttribs));
  parse_and_exec_command_in_gtk_thread(readlineResult, TRUE, current_stdin_cmd_interpretor_pointer, childAttribs);
}

static void parse_and_exec_cmd_in_gtk_thread (gchar * cmd){
  RFM_ChildAttribs *childAttribs = calloc(1, sizeof(RFM_ChildAttribs));
  // 在创建子进程前暂停readline
  //rl_callback_handler_remove();
  parse_and_exec_command_in_gtk_thread(cmd, FALSE, current_stdin_cmd_interpretor_pointer, childAttribs);
  // 子进程结束后恢复
  //rl_callback_handler_install(prompt, readline_callback);
}

#ifdef GNU_readline
static char** rfm_filename_completion(const char *text, int *count){
#endif
#ifdef reedline
static GPtrArray* rfm_filename_completion(const char *text, int *count){
  if (g_strcmp0(text, BuiltInCmd_dimension)==0){
    return rfm_completion_based_on_dimensions("");
  }
#endif
  if (ItemSelected && (text==NULL || g_strcmp0(text, "")==0)) { //有文件被选中 且 当前光标上下文为空
    char** ret = calloc(2,sizeof(char*));
    g_mutex_lock(&rfm_selection_completion_lock);
    ret[0] = strdup(rfm_selection_completion);
    //see the update of rfm_selection_completion and usage of lock, it possible that when ItemSelected=0 here, we read the old rfm_selection_completion value, but i just prevent read just after rfm_selection_completion been freed here.
    g_mutex_unlock(&rfm_selection_completion_lock);
    g_log(RFM_LOG_READLINE,G_LOG_LEVEL_DEBUG,"rfm_filename_completion return %s",ret[0]);
    *count = 1;//TODO:实现多个补全
#ifdef GNU_readline
    return ret;
#endif
#ifdef reedline
    return g_ptr_array_new_from_array(ret, g_strv_length(ret),NULL,NULL,(GDestroyNotify)g_free); // -1 means ret end with null
  }else { //没有文件被选中，或光标前有非空字符
    GPtrArray* ret = rfm_editor_completion_based_on_search_result(text);
    g_ptr_array_extend(ret, rfm_completion_based_on_dimensions(text),NULL,NULL);
    char** tmp=rl_completion_matches(text, rl_filename_completion_function);
    g_ptr_array_extend(ret, g_ptr_array_new_from_array(tmp,g_strv_length(tmp),NULL,NULL,(GDestroyNotify)g_free), NULL,NULL);
    return ret;
  }
#else
  }else return NULL;
#endif
}

#ifdef GNU_readline
static char** gnu_readline_completion(const char *text, int start, int end){
  g_log(RFM_LOG_READLINE, G_LOG_LEVEL_DEBUG,"call gnu_readline_completion with %s, %d, %d",text, start, end);
  int count=0;
  char ** ret = rfm_filename_completion(text, &count);
  if (ret) {
    //rl_attempted_completion_over = 1;
    rl_completion_suppress_append = 1;    
    return ret;
  } else if (OLD_rl_attempted_completion_function) {
    g_log(RFM_LOG_READLINE, G_LOG_LEVEL_DEBUG,"call OLD_rl_attempted_completion_function with %s, %d, %d",text, start, end);
    return OLD_rl_attempted_completion_function(text,start,end);
  }else return NULL;
}
#endif


static gboolean window_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
    ensure_foreground_process();
    return FALSE;
}

static int setup()
{
   gtk_init(NULL, NULL);
   if (!no_window && showHelpOnStart && startWithTerm() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)) stdin_command_help(NULL,NULL);
   if (!no_window){
   window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(window), 640, 400);
   g_signal_connect(window, "focus-in-event", G_CALLBACK(window_focus_in_event), NULL);

   if (getenv("rfmFontCSS")){//可以在rfm.sh里面设置字体环境变量
     GtkCssProvider *provider = gtk_css_provider_new();
     gtk_css_provider_load_from_data(provider, getenv("rfmFontCSS"), -1, NULL);
     GdkScreen *screen = gdk_screen_get_default();
     gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   GdkDisplay *display = gtk_widget_get_display(window);
   gchar* theme_name = getenv("XCURSOR_THEME");
   if (theme_name!=NULL){
     if (GDK_IS_WAYLAND_DISPLAY (display))
       gdk_wayland_display_set_cursor_theme(display, theme_name, 24);
     else if (GDK_IS_X11_DISPLAY (display))
       gdk_x11_display_set_cursor_theme (display, theme_name, 24);
   }

   rfm_main_box=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add(GTK_CONTAINER(window), rfm_main_box);
   }
  
   rfm_homePath=g_strdup(g_get_home_dir());
  
   rfm_thumbDir=g_build_filename(g_get_user_cache_dir(), "thumbnails", "normal", NULL);

   thumb_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)gtk_tree_row_reference_free);
#ifdef RFM_CACHE_THUMBNAIL_IN_MEM
   pixbuf_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
#endif
#ifdef GitIntegration
   gitTrackedFiles=g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
   gitDeletedDirs=g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
  //gitCommitMsg=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
#endif 

   if (rfm_do_thumbs==1 && !g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR)) {
      if (g_mkdir_with_parents(rfm_thumbDir, S_IRWXU)!=0) {
         g_warning("Setup: Can't create thumbnail directory.");
         rfm_do_thumbs=0;
      }
   }
   
   g_signal_connect (rfm_mountMonitor, "mounts-changed", G_CALLBACK (mounts_handler), NULL);
   g_signal_connect (rfm_mountMonitor, "mountpoints-changed", G_CALLBACK (mounts_handler), NULL); /* fstab changed */

   init_inotify();

   #ifdef RFM_ICON_THEME
      icon_theme=gtk_icon_theme_new();
      gtk_icon_theme_set_custom_theme(icon_theme,RFM_ICON_THEME);
   #else
      icon_theme=gtk_icon_theme_get_default();
   #endif

   defaultPixbufs=load_default_pixbufs(); /* TODO: WARNING: This can return NULL! */
   //g_object_set_data_full(G_OBJECT(window),"rfm_default_pixbufs",defaultPixbufs,(GDestroyNotify)free_default_pixbufs);
   //Note that it is very important that the following G_TYPE_*s are in the same order of enum definition (RFM_treeviewCOL such as COL_FILENAME)
   store=gtk_list_store_new(NUM_COLS,
			      G_TYPE_STRING,    //iconview_markup
			      G_TYPE_STRING,    //iconview_tooltip
                              GDK_TYPE_PIXBUF,  /* Displayed icon */
			      G_TYPE_STRING,     //MODE_STR
			    //G_TYPE_STRING,    /* Displayed name */
			      G_TYPE_STRING,    //filename
			      G_TYPE_STRING,  //link_target
			      G_TYPE_STRING,    //fullpath 
                              G_TYPE_UINT64,    /* File mtime: time_t is currently 32 bit signed */
			      G_TYPE_STRING, //MTIME_STR
			      G_TYPE_UINT64, //size
			      G_TYPE_STRING, //size_display
			      G_TYPE_POINTER,  /* RFM_FileAttributes */
			      G_TYPE_STRING,    //OWNER
			      G_TYPE_STRING,   //GROUP
			      G_TYPE_STRING,  //mime_root
			      G_TYPE_STRING,  //mime_sub
			      G_TYPE_STRING, //ATIME_STR
			      G_TYPE_STRING, //CTIME_STR
#ifdef GitIntegration
			      G_TYPE_STRING, //GIT_STATUS_STR
			      G_TYPE_STRING,     //git commit message
			      G_TYPE_STRING, //git author
			      G_TYPE_STRING, //git commit date
			      G_TYPE_STRING, //git commit id
#endif
			      G_TYPE_STRING, //mime_sort
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext1..5
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext6..10
   			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext11..15
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext16..20
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext21..25
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext26..30
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext31..35
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext36..40
   			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext41..45
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext46..50
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext51..55
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING); //COL_Ext56..60
   treemodel=GTK_TREE_MODEL(store);
   
   if (!no_window) g_signal_connect(window,"destroy", G_CALLBACK(cleanup), NULL);

   g_debug("initDir: %s",initDir);
   g_debug("rfm_curPath: %s",rfm_curPath);
   g_debug("rfm_homePath: %s",rfm_homePath);
   g_debug("rfm_thumbDir: %s",rfm_thumbDir);

#ifdef PythonEmbedded
   startPythonEmbedding();
#endif
   if (getcwd(cwd, sizeof(cwd)) == NULL) die("ERROR: %s: getcwd() failed.\n", PROG_NAME); /* getcwd returns NULL if cwd[] not big enough! */

   sprintf(rfm_historyFile_ParentDirectory, "%s", getenv("HOME"));
   rfm_historyFileLocation = g_build_filename(rfm_historyFile_ParentDirectory, current_stdin_cmd_interpretor_pointer->HistoryFilename ? current_stdin_cmd_interpretor_pointer->HistoryFilename : ".rfm_history", NULL);
   rfm_historyDirectory_FileLocation = g_build_filename(getenv("HOME"),".rfm_history_directory", NULL);
#ifdef GNU_readline  
   int e;
   using_history();
   stifle_history(RFM_HISTORY_SIZE);
   history_write_timestamps=TRUE;
   if ((e=read_history(rfm_historyFileLocation))) g_warning("failed to read_history(%s) error code:%d.",rfm_historyFileLocation,e);
   OLD_rl_attempted_completion_function = rl_attempted_completion_function;
   rl_attempted_completion_function = gnu_readline_completion;
   rl_variable_bind("menu-complete-display-prefix", "on");
   rl_bind_key('\t', rl_menu_complete);
   rl_variable_bind("show-all-if-ambiguous", "on");
   rl_variable_bind("completion-ignore-case", "on");
   rl_variable_bind("mark-directories", "on");
   rl_bind_keyseq("\e[A", rl_history_search_backward);
   rl_bind_keyseq("\e[B", rl_history_search_forward);
   //rl_event_hook = rl_event_hook_to_help_terminate_readline; //TODO:暂不使用设rl_done为非零的功能，这一句也暂时没用，如果要设rl_done非零，取消这一句注释
#endif
#ifdef reedline
  functionPointersForReedlineWrapper.get_completion_fn = rfm_filename_completion;
  functionPointersForReedlineWrapper.g_idle_add_once_handle_escape = g_idle_add_once_handle_escape_key;
  reedline_handle = reedline_create(&functionPointersForReedlineWrapper);
  if (!reedline_handle) g_error("reedline_create return NULL handle");
#endif

   rfm_prePath= getenv("OLDPWD");
   if (rfm_prePath!=NULL) rfm_prePath=strdup(rfm_prePath); //i don't remember exactly, but i think i do this because i read somewhere that getenv result is special and i should not free it.
   if (!no_window){
   add_toolbar(rfm_main_box, defaultPixbufs);
   menu=gtk_menu_new();
   add_file_menuitems(&menuItem,run_actions,G_N_ELEMENTS(run_actions));

   columnHeaderMenu = gtk_menu_new();
   InsertBeforeMenuItem = gtk_menu_item_new_with_label(MENU_InsertBeforeThisColumn);
   gtk_menu_shell_append(GTK_MENU_SHELL(columnHeaderMenu), InsertBeforeMenuItem);
   InsertAfterMenuItem = gtk_menu_item_new_with_label(MENU_InsertAfterThisColumn);
   gtk_menu_shell_append(GTK_MENU_SHELL(columnHeaderMenu), InsertAfterMenuItem);
   }
#ifdef RFM_AddOn
   g_debug("%s md5 hash:%s",RFM_AddOn_H_filepath,RFM_AddOn);
  //TODO: 考虑partse_and_exec_cmd_in_gtk_thread里支持 类似callback, readoutput by program 这种机制, 也许可以和SearchResult那一块有交集.这会儿作为workaround,就为命令做了一个单独的bash脚本,一边可以在命令里有管道
   if (!disable_addon) {
     all_addon_definitions_by_dir = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_hash_table_destroy);
     RFM_ChildAttribs *child_attribs = calloc(1, sizeof(RFM_ChildAttribs));
     child_attribs->customCallBackFunc = locate_all_addons_callback;
     child_attribs->customCallbackUserData = child_attribs;
     child_attribs->output_read_by_program = TRUE;
     child_attribs->setsid = TRUE;
     //child_attribs->spawn_async = TRUE; //TODO: 倒不是说这里要用async,只是开发时发现，这个会被 exec_stdin_command 里的设置覆盖掉
     parse_and_exec_command_in_gtk_thread(g_strdup(locate_all_addons[1]), FALSE, match_cmd_str_array_with_interpretors(locate_all_addons), child_attribs);
   }
#endif
   if (initDir == NULL) initDir = cwd;
   set_rfm_curPath(initDir);

   ReadFromPipeStdinIfAny(pipefd);
   if (no_window) writeSearchResultIntoFD_in_gtk_thread(STDOUT_FILENO, FALSE);
  
   refresh_store();
   if(!no_window && startWithTerm() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)){
       readlineThread = g_thread_new("readline", readlineInSeperateThread, NULL);
   }
   
   //block Ctrl+C. Without this, Ctrl+C in readline will terminate rfm. Now, if you run htop with readline, Ctrl+C only terminate htop. BTW, it's strange that i had tried sigprocmask, pthread_sigmask, and rl_clear_signals, and they didn't work.
   ignore_SIGINT.sa_handler = SIG_IGN;
   ignore_SIGINT.sa_flags = 0;
   sigaction(SIGINT, &ignore_SIGINT,NULL);

   return 0;
}

//RFM_builtinCMD 里传过来的和 g_signal_connect 里传过来的不一样，这里声明的是最初g_signal_connect里用的，但后来RFM_builtinCMD里也用到这个函数了,所以这里现在的signature干脆不含参数，避免错误的参数值被使用
static void cleanup()
{
   if (StartedAs_rfmFileChooser){
      GtkTreeIter iter;
      int returnToFile_fd=-1;
      
      if (rfmFileChooserReturnSelectionIntoFilename!=NULL && (returnToFile_fd=open(rfmFileChooserReturnSelectionIntoFilename,O_WRONLY|O_NONBLOCK))<0)
	g_warning("failed to open rfmFileChooserReturnSelectionIntoFilename:%s",rfmFileChooserReturnSelectionIntoFilename);
      else if(returnToFile_fd>=0 || rfmFileChooserReturnSelectionIntoFilename==NULL ){
	  g_debug("fd for rfmFileChooserReturnSelectionIntoFilename in child process:%d",returnToFile_fd);
	  GList *selectionList=filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] ? g_list_first(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]):NULL;
	  while(selectionList!=NULL){
	    if (returnToFile_fd>=0){
	      write(returnToFile_fd, selectionList->data, strlen(selectionList->data));
	      write(returnToFile_fd,"\n",1);
	    }else printf("%s\n",selectionList->data);
	    selectionList=g_list_next(selectionList);
	  };
	  if (returnToFile_fd>0) close(returnToFile_fd);
      }
   }

   //https://unix.stackexchange.com/questions/534657/do-inotify-watches-automatically-stop-when-a-program-ends
   inotify_rm_watch(rfm_inotify_fd, rfm_curPath_wd);
   close(rfm_inotify_fd);
   save_current_history();
#ifdef GNU_readline
   rl_attempted_completion_function = OLD_rl_attempted_completion_function;
#endif
#ifdef PythonEmbedded
   cmdStopShare(NULL, NULL);
   //endPythonEmbedding(); py_finializeex() cause sigsegv on my laptop
#endif
   gtk_main_quit();
   window = NULL; //这里window 作为标志位，防止 quit 命令执行完cleanup后，继续新建 readlineThread
}

/* From http://dwm.suckless.org/ */
static void die(const char *errstr, ...) {
   va_list ap;

   va_start(ap, errstr);
   vfprintf(stderr, errstr, ap);
   va_end(ap);
   exit(EXIT_FAILURE);
}


static void regcomp_for_regex_rules(){
   for(guint i=0;i<G_N_ELEMENTS(regex_rules);i++){
       regex_t * restrict regex = calloc(1, sizeof(regex_t));
       int c;
       //https://www.gnu.org/software/sed/manual/html_node/Character-Classes-and-Bracket-Expressions.html#Character-Classes-and-Bracket-Expressions
       //https://www.gnu.org/software/sed/manual/html_node/BRE-syntax.html#BRE-syntax
       if ((c = regcomp(regex, regex_rules[i].pattern, 0))){
		/************************************************************************/
		/*  正则表达式编译出错输出错误信息                                      */
		/*  调用 regerror 将错误信息输出到 regerrbuf 中                         */
		/*  regerrbuf 末尾置0,确保上面调用regerror 导致 regerrbuf 溢出的情况下, */
		/*  字符串仍有有结尾0                                                   */
		/************************************************************************/
                char regerrbuf[256];
                regerror(c, regex, regerrbuf, sizeof(regerrbuf));
		regerrbuf[sizeof(regerrbuf) - 1] = '\0';
		g_warning("%s", regerrbuf);
		regfree(regex);
       }else regex_rules[i].pattern_compiled = regex;
   }
}

//从treeviewColumns_init 复制到 treeviewColumns, 并用C1作为模板，补齐C2,C3,...,C60列
static void init_ext_columns_in_treeviewColumns() {
    // 复制原始列
    memcpy(treeviewColumns, treeviewColumns_init, sizeof(treeviewColumns_init));
    // 追加C2-C60列
    for(int i = 2; i <= NUM_Ext_Columns; i++) {
      // 复制C1的所有属性，假设C1是最后一个元素
      memcpy(&treeviewColumns[G_N_ELEMENTS(treeviewColumns_init) + i - 2], &treeviewColumns_init[G_N_ELEMENTS(treeviewColumns_init) - 1], sizeof(RFM_treeviewColumn));
      snprintf(treeviewColumns[G_N_ELEMENTS(treeviewColumns_init) + i - 2].title, 10, "C%d", i);
    }
}

int main(int argc, char *argv[])
{
   struct stat statbuf;
   gchar *thumbnailsize;
   gchar *auto_execution_command_after_rfm_start = NULL;
   g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, g_log_default_handler, NULL);

   setenv("RFM_GIT_ROOT","/",1);
   addons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
   addons_hash = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
   addons_filename = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

   rfm_mountMonitor=g_unix_mount_monitor_get();
   delayedRefresh_GSourceID=0;

   if (thumbnailers[0].thumbRoot==NULL)
      rfm_do_thumbs=0;
   else
      rfm_do_thumbs=1;

   for(int i=0;i<=NUM_Ext_Columns;i++) {
     ExtColumnHashTable[i]=NULL;
   }

   init_ext_columns_in_treeviewColumns();
   //上面 根据 treeviewColumns_init 初始化 treeviewColumns 也可以和下面两个memcpy 合并，也就是 根据treeviewColumns_init 直接初始化 SearchResultViewColumnsLayout 和 DirectoryViewColumnsLayout. 可以省掉treeviewColumns 数组。
   //但 treeviewColumns_init 到 treeviewColumns 的复制，主要是为了config.def.h 看起来更清爽，也减少了 AI 编程时传给大模型prompt量，减少珍贵的context window 的占用。主要是我不知道C语言里还能有啥好办法初始化时扩展静态数组，才这么做的，也许将来会改。而下面的memcpy是除了初始化外，程序运行时还多次要用到的。这两步数组复制的目的和功能不同，treeviewColumns 这点本身运行内存的占用在现代计算机上算不得啥，所以就不合并了。
   SearchResultViewColumnsLayout = malloc(sizeof(RFM_treeviewColumn)*G_N_ELEMENTS(treeviewColumns));
   memcpy(SearchResultViewColumnsLayout, treeviewColumns, sizeof(RFM_treeviewColumn)*G_N_ELEMENTS(treeviewColumns));
   DirectoryViewColumnsLayout = malloc(sizeof(RFM_treeviewColumn)*G_N_ELEMENTS(treeviewColumns));
   memcpy(DirectoryViewColumnsLayout, treeviewColumns, sizeof(RFM_treeviewColumn)*G_N_ELEMENTS(treeviewColumns));
   sprintf(selected_filename_placeholder_in_space," %s ",RFM_selected_filename_placeholder);
   sprintf(selected_filename_placeholder_in_quotation, "'%s'",RFM_selected_filename_placeholder);
   snprintf(thumbnailsize_str,8, "%dx%d",RFM_THUMBNAIL_SIZE,RFM_THUMBNAIL_SIZE);

   set_pagesize_for_searchresultview(getenv("RFM_PAGESIZE"));

   regcomp_for_regex_rules();
   
   PROG_NAME = strdup(argv[0]);
   int c=1;
   while (c<argc) {
     if (argv[c][0]=='-'){
      switch (argv[c][1]) {
      case 'd':
	 if (argc<=(c+1)) {
	   g_warning("A directory path is required for the -d option.");
	   break;
	 }
         //int i=strlen(argv[c+1])-1;
         //if (i!=0 && argv[c+1][i]=='/') argv[c+1][i]='\0';
         //if (strstr(argv[c+1],"/.")!=NULL) die("ERROR: %s: Hidden files are not supported.\n", PROG_NAME);
         if (stat(argv[c+1],&statbuf)!=0) die("ERROR: %s: Can't stat %s\n", PROG_NAME, argv[c+1]);
	 if (S_ISDIR(statbuf.st_mode)){
	    if (access(argv[c+1], R_OK)!=0) die("ERROR: %s: Can't enter %s\n", PROG_NAME, argv[c+1]);
	    initDir=canonicalize_file_name(argv[c+1]);
	 }else{
	    initDir=g_path_get_dirname(argv[c+1]);
	    filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView], g_strdup(argv[c+1]));
         }           
	 c++;
	 while(argc>c+1 && argv[c+1][0]!='-') filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView], g_strdup(argv[++c]));
         break;
      case 'v':
         printf("%s-%s, Copyright (C) Rodney Padgett, guyuming, see LICENSE for details\n", PROG_NAME, VERSION);
         return 0;
      case 'p':
	   if (initDir!=NULL) die("if you have -d specified, and read file name list from pipeline, -p parameter must goes BEFORE -d\n");
	   set_pagesize_for_searchresultview(argv[c] + 2 * sizeof(gchar));
	   SearchResultViewInsteadOfDirectoryView = 1;
         break;
      case 'l':
	 treeview=TRUE;
	 break;
      case 's':
	 treeviewcolumn_init_order_sequence = &(argv[c][2]);
	 show_hide_treeview_columns_in_order(treeviewcolumn_init_order_sequence);
	 break;
      case 'S':
	 exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread = TRUE;
	 break;
      case 'h':
	printf(rfmLaunchHelp, PROG_NAME);
	return 0;
      case 'n':
	showHelpOnStart = FALSE;
	break;
      case 'H': pauseInotifyHandler=TRUE; break;
      case 'r':
	StartedAs_rfmFileChooser=TRUE;
	if (argc>c+1 && g_str_has_prefix(argv[c+1], RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX)){
	  rfmFileChooserReturnSelectionIntoFilename = argv[c+1];
	  c++;
	}
	if (rfmFileChooserReturnSelectionIntoFilename==NULL) rfmFileChooserReturnSelectionIntoFilename = getenv("rfmFileChooserReturnSelectionIntoFilename");
	g_debug("rfmFileChooserReturnSelectionIntoFilename:%s",rfmFileChooserReturnSelectionIntoFilename);
	break;
      case 'x': //auto execute after start. for example, start with locate rfm.c >0, to avoid locate rfm.c|rfm. We can use rfm -x "locate rfm.c>0"
	if (argc<=(c+1)) die("ERROR: %s: A command string which can be executed by rfm is required for the option. for example: rfm -x \"locate rfm.c>0\"\n", PROG_NAME);
	auto_execution_command_after_rfm_start = g_strdup(argv[c+1]);
	c++;
	break;
      case 't':
	rfmStartWithVirtualTerminal=FALSE;
	break;
      case 'T':
	thumbnailsize =argv[c] + 2 * sizeof(gchar); // remove the '-T' prefix in argv[c]
	int ts=atoi(thumbnailsize);
	if (ts!=0) {
	  RFM_THUMBNAIL_SIZE=ts;
	  snprintf(thumbnailsize_str,8, "%dx%d",RFM_THUMBNAIL_SIZE,RFM_THUMBNAIL_SIZE);
	}else die("-T should be followd with custom RFM_THUMBNAIL_SIZE, for example: -T256");
	break;
      case '-': //-- for long argument
	char* longArgument=argv[c] + 2 * sizeof(gchar);
	if (g_strcmp0(longArgument, BuiltInCmd_SearchResultColumnSeperator)==0){
	  printf("%s\n",SearchResultColumnSeperator);
	  exit(Success);
	}else if (g_strcmp0(longArgument, "help")==0){
	  printf(rfmLaunchHelp, PROG_NAME);
	  stdin_command_help(NULL, NULL);
	  return 0;
	}else if (g_strcmp0(longArgument, "InheritedPrompt")==0){
	  if (argc<=(c+1)) {
	   g_warning("Missing prompt string!");
	   break;
	  }
	  InheritedPrompt = argv[c+1];
	  c++;
#ifdef RFM_AddOn
        }else if (g_strcmp0(longArgument, "disable-addon")==0){
	  disable_addon = TRUE;
#endif
        }else if (g_strcmp0(longArgument, "no-gui")==0){
	  no_window = TRUE;
	}
	break;
      default:
	 die("invalid parameter, %s -h for help\n",PROG_NAME);
      }
    }
    else handle_commandline_argument(argv[c]);
    c++;
   }

   if (setup()==0){
      if (auto_execution_command_after_rfm_start) g_idle_add_once(parse_and_exec_cmd_in_gtk_thread, auto_execution_command_after_rfm_start);
      if (!no_window) gtk_main();
   }else
      die("ERROR: %s: setup() failed\n", PROG_NAME);

   if (startWithTerm()) system("reset -I");
   return 0;
}

static int ProcessOnelineForSearchResult(CALLER_OWN char* oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
  FileNameIndexInColumnTitles = FALSE;
  if (search_result_type_withtitle_column_title_array) { g_strfreev(search_result_type_withtitle_column_title_array);search_result_type_withtitle_column_title_array=NULL; }
  return ProcessOnelineForSearchResult_internel(oneline, new_search, FileNameIndexInColumnTitles);
}

static int ProcessOnelineForSearchResult_with_title(CALLER_OWN char* oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
  g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"ProcessOnelineForSearchResult_with_title(%s, new_search:%d) fileAttributeID:%d", oneline, new_search,fileAttributeID);
  if (childAttribs->SearchResultAsInput && !treeview){//iconview视图下面查询结果第一行不会导出文件名标题列(参见writeSearchResultIntoFD_in_gtk_thread)，所以读入的时候也如同没有 _with_title 一样。
    FileNameIndexInColumnTitles = FALSE;
    return ProcessOnelineForSearchResult_internel(oneline, new_search, FileNameIndexInColumnTitles);
  }

  if (fileAttributeID==1){
    if (search_result_type_withtitle_column_title_array) { g_strfreev(search_result_type_withtitle_column_title_array);search_result_type_withtitle_column_title_array=NULL; }
    search_result_type_withtitle_column_title_array = g_strsplit(oneline, SearchResultColumnSeperator, TREEVIEW_COLUMNS_array_size);
    uint i=0;
    while(search_result_type_withtitle_column_title_array[i]){
      if (strcmp(search_result_type_withtitle_column_title_array[i], fld_FileName)==0){
	FileNameIndexInColumnTitles = i + 1; //filename_index_in_column_titles start with 1
	return ProcessOnelineForSearchResult_internel(oneline, new_search, FileNameIndexInColumnTitles);
      }
      i++;
    }
    g_warning("no %s field in search result!",fld_FileName);
    g_strfreev(search_result_type_withtitle_column_title_array);
    return 0;
  }else return ProcessOnelineForSearchResult_internel(oneline, new_search, FileNameIndexInColumnTitles);
}

// default search result processing function
// 处理通过pipeline 或者内置 >0 方式获得的用分隔符分开的字符串行
// 同一searchresult包含的多行结果分隔符数相同,类似 csv 格式
// first_line_column_title 为FALSE时,搜索结果第一行不包含表头,显示C1,C2,C3,...的顺序列名
// first_line_column_title 为TRUE的时候,第一行为表头,且只能用于扩展列,不会去查找现有同名列
// 当 first_line_column_title 为false时，假定文件名列排第一， filename_index_in_column_title 为0 即false；当first_line_column_title 为true时，filename_index_in_column_titles 大于等于1,即从1开始。在本commit前，假设文件名始终位于查询结果首列，不论是否first_line_column_title(即查询结果类型withTitle)。
static int ProcessOnelineForSearchResult_internel(CALLER_OWN char* oneline, gboolean new_search, int filename_index_in_column_titles){
           g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"ProcessOnelineForSearchResult_internel(%s, new_search:%d, filename_index_in_column_titles:%d)", oneline, new_search, filename_index_in_column_titles);
           if (oneline == NULL || !new_search) return 0;
	   char* key=calloc(10, sizeof(char));
           char* filename = oneline;
           char* currentExtColumnValue = NULL;
	   char* seperatorPointerAfterCurrentExtColumnValue = strstr(oneline, SearchResultColumnSeperator);
	   uint currentColumnTitleIndex=1;
	   if (seperatorPointerAfterCurrentExtColumnValue) { //found SearchResulutColumnSeperator such as ":" in oneline; emacs里 ctrl+alt+f  ctrl+alt+b 跳转到匹配的括号
	       snprintf(key,10, "%d", fileAttributeID);
	       *seperatorPointerAfterCurrentExtColumnValue = 0; //ending NULL for filename

               if (filename_index_in_column_titles<=1){ //如果第一列是文件名
		 currentExtColumnValue = seperatorPointerAfterCurrentExtColumnValue + strlen(SearchResultColumnSeperator); //如果第一列是文件名，则移动currentExtColumnValue指针到下一列。
		 filename = oneline;
		 g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"first column is filename:%s", filename);
	       }else{
		 currentExtColumnValue = oneline;
		 g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"first column is not filename:%s", oneline);
	       }
	     
	       enum RFM_treeviewCol current_Ext_Column=COL_Ext1;
	       char currentColumnTitleIndexName[10]; //这个总是保存 C1,C2,C3. 对于searchresulttype withTitle，即 filename_index_in_column_title>0 的情况，真正的column title 保存在 key(即fileAttributeID)值为1 的 ExtColumnHashTable[currentExtColumnHashTableIndex] 中
	       snprintf(currentColumnTitleIndexName,10,"C%d", currentColumnTitleIndex);
	       RFM_treeviewColumn* col = get_treeviewColumnByTitle(currentColumnTitleIndexName);
	       // Column Titles will always be C1, C2, C3, ..., that is, continuous with no gap, and start from C1
	       // but current_Ext_Column and currentExtColumnHashTable Ext can have gap: it can be COL_Ext2, COL_Ext4,...,
	       // 上面一行的假设 current_Ext_Column 有空缺,我想象中可能会是用户通过 showcolumn 命令占用了一些扩展列.但这只是我在写这段代码时想象中可能出现的情况,并未在测试中实际遇到过
	       // 到目前为止,实际测试中 current_Ext_column 也是连续的
	       // 这个假设对目前代码的影响就是 用currentColumnTitleIndex 这样一个连续编号,得到 C1, C2, C3, ... 这样的连续编号列名, 然后再通过 get_treeviewColumnByTitle 找到列
	       // current_Ext_Column 对应的COL_ExtX,中X下标总是等于 currentExtColumnHashTableIndex + 1

	       if (fileAttributeID==1){ //bind to available extended column for the first line.
		    g_assert(col->enumCol==NUM_COLS);//这里假设C1是空的
		    current_Ext_Column = get_available_ExtColumn(current_Ext_Column); //This function is called for new search result, COL_Ext1 will be available, so i don't check current_Ext_column < NUM_COLS here
	       } else {
		    current_Ext_Column = col->enumCol;
	       }
		 
	       do {
		    uint currentExtColumnHashTableIndex = current_Ext_Column - COL_Ext1;
                    if (ExtColumnHashTable[currentExtColumnHashTableIndex]==NULL) {
                        ExtColumnHashTable[currentExtColumnHashTableIndex] = g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
                        ExtColumnHashTable_keep_during_refresh[currentExtColumnHashTableIndex]=TRUE;
		        SearchResultContainNonFilenameColumn = TRUE;
                    }
		    
		    if (currentExtColumnValue!=oneline) {//当currentExtColumnValue==oneline 时，说明第一列不是文件名，do 循环前已经把第一列后面的SearchResultColumnSeperator首字符置0了。
		      //do 循环的开始，找下一个SearchResultColumnSeperator, 并将其首字符置0,即为currentExtColumnValue尾部设NULL
		      seperatorPointerAfterCurrentExtColumnValue = strstr(currentExtColumnValue, SearchResultColumnSeperator);
		      if (seperatorPointerAfterCurrentExtColumnValue) *seperatorPointerAfterCurrentExtColumnValue = 0; //ending NULL for currentExtColumnValue
		    }
		    g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"Insert column %s into ExtColumnHashTable[%d]: fileAttributeID as key %s,value %s", currentColumnTitleIndexName, currentExtColumnHashTableIndex,key,currentExtColumnValue);
		    g_hash_table_insert(ExtColumnHashTable[currentExtColumnHashTableIndex], strdup(key), strdup(currentExtColumnValue));

		    if (seperatorPointerAfterCurrentExtColumnValue) currentExtColumnValue = seperatorPointerAfterCurrentExtColumnValue + strlen(SearchResultColumnSeperator); //moving char pointer

      		    currentColumnTitleIndex++;
		    snprintf(currentColumnTitleIndexName,10,"C%d", currentColumnTitleIndex);

		    if (currentColumnTitleIndex == filename_index_in_column_titles) {//如果循环下一轮要处理的列是文件名列，则设置filename并移动指针到下一列
		      filename = currentExtColumnValue;
		      
		      //TODO：这里在一个do循环中，使用 strstr 查找 SearchResultColumnSeperator两次，即处理了两列，不够优美。重构这个函数，在do循环体内处理filename列，而不是像现在这样，在循环体内跳过filename列，到循环结束后再prepend到 SearchResultFileNameList
		      g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"skip filename column");
		      *seperatorPointerAfterCurrentExtColumnValue = 0; //为filename前一个字段设置ending NULL
		      seperatorPointerAfterCurrentExtColumnValue = strstr(currentExtColumnValue, SearchResultColumnSeperator);//查找filename后面的seperator
		      if (seperatorPointerAfterCurrentExtColumnValue){
			currentExtColumnValue = seperatorPointerAfterCurrentExtColumnValue + strlen(SearchResultColumnSeperator); //moving char pointer
			*seperatorPointerAfterCurrentExtColumnValue = 0; //为filename设置ending NULL
		      }
		    }

		    if (fileAttributeID==1){//搜索结果第一行
		         g_assert(col->enumCol==NUM_COLS);
			 g_assert(col->ValueFunc==getExtColumnValueFromHashTable);
		         col->enumCol = current_Ext_Column;

                         col = get_treeviewColumnByTitle(currentColumnTitleIndexName);
			 
			 current_Ext_Column++;
			 current_Ext_Column=get_available_ExtColumn(current_Ext_Column);
		    }else {
		         //如果搜索结果为 filename:xxx 的形式,column title 会是 filename C1
		         //我们假定搜索结果每行列数都是相同的,第二行数据用 get_treeviewColumnByTitle(C1) 一定可以找到第一行为C1分配的位置
		         //但这里currentColumnTitle do while 循环最后会调用get_treeviewColumnByTitle(C2), 对于本方法并没有问题,因为while 判断会结束循环
		         //但如果ProcessKeyValuePairInFilesFromSearchResult 调用本方法,get_treeviewColumnByTitle(C2) 会返回NULL,所以下面语句加了if NULL 判断
		         if ((col = get_treeviewColumnByTitle(currentColumnTitleIndexName)))
			     current_Ext_Column = col->enumCol;
			 else current_Ext_Column = NUM_COLS;
		    }

	       } while(current_Ext_Column<NUM_COLS && seperatorPointerAfterCurrentExtColumnValue);
	   }
	   free(key);
	   if (!(fileAttributeID==1 && filename_index_in_column_titles)){
	     SearchResultFileNameList=g_list_prepend(SearchResultFileNameList, strdup(filename));
	     SearchResultFileNameListLength++;
	     g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"appended into SearchResultFileNameList:%s", filename);
	   }
	   return currentColumnTitleIndex;
}

static void CallDefaultSearchResultFunctionForNewSearch(CALLER_OWN char *one_line_in_search_result, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
     if (fileAttributeID==1) {
       currentSearchResultTypeStartingColumnTitleIndex = ProcessOnelineForSearchResult(one_line_in_search_result,TRUE, cmdTemplate, fileAttributes, childAttribs);
       currentPageStartingColumnTitleIndex = currentSearchResultTypeStartingColumnTitleIndex;
     //首行记录在  ProcessOnelineForSearchResult 里已经被分配掉的列序号
     }else ProcessOnelineForSearchResult(one_line_in_search_result, TRUE, cmdTemplate, fileAttributes, childAttribs);
}

// 处理存储键值对文件(类似INI)数据,每行一个文件,每个文件可包含不同的键
// 搜索结果除了包含文件名,还可以包含其他类似csv行数据,也就是先用 ProcessOnelineForSearchResult 处理
// if new_search, oneline contains leading filename plus optional columns, otherwise, oneline contains only filename
int ProcessKeyValuePairInFilesFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
   if (new_search){
     CallDefaultSearchResultFunctionForNewSearch(oneline, TRUE, cmdTemplate, fileAttributes, childAttribs);
     return 0;// 对于new_search, 调用本方法的目的仅仅是调用default的ProcessOnelineForSearchResult
   }

   GKeyFile *keyfile=g_key_file_new();
   GError *error=NULL;
   gchar * absolute_address = g_canonicalize_filename(oneline, rfm_SearchResultPath);
   if (!g_key_file_load_from_file(keyfile, absolute_address, G_KEY_FILE_NONE, &error)){
     g_warning("ProcessKeyValuePairInFilesFromSearchResult. error loading key value pair from file %s, error code: %d, message:%s",oneline, error==NULL?0:error->code, error==NULL?"":error->message);
     g_error_free(error);
   }else{
     ProcessKeyValuePairInData(keyfile, dumb_keyfile_groupname);
     g_key_file_free(keyfile);
   }
   g_free(absolute_address);
}

//根据key,找到合适的列，合适的currentExtColumnHashTableIndex, 执行g_hash_table_insert(ExtColumnHashTable[currentExtColumnHashTableIndex], strFileAttributeID, currentExtColumnValue);
static void ProcessKeyValuePair(CALLER_OWN gchar *key, OWNER_CHANGE gchar* currentExtColumnValue, CALLER_OWN char* currentColumnOriginalTitle){
	  RFM_treeviewColumn * col;
	  enum RFM_treeviewCol current_Ext_Column;
	  if ((col=get_treeviewColumnByTitle(key))){ //已经有同名列了
	    current_Ext_Column=col->enumCol;
	  }else{//新列名
	    col = get_treeviewColumnByTitle(currentColumnOriginalTitle); //找到当前(空余,未被使用的)列名,如C1, C2, C3 ... 等等
            current_Ext_Column=get_available_ExtColumn(COL_Ext1);//既然是空余列名,一定未绑定 emuncol, 找一个空的enum,就是COL_ExtX
	    //简单地看,列名C1,C2和COL_Ext1, COL_Ext2 应该是一一对应的,似乎不存在找到空余的列名,却没有空余的COL_ExtX 的情况
	    //但是万一有比如预定名称的列如MailFrom占据了某个COL_ExtX,会早成列名C1,C2虽够,但COL_ExtX不够了.
	    //也就是说COL_ExtX总是比C1,C2..CX先用完
	    if (current_Ext_Column==NUM_COLS) {
	      g_warning("Not enough COL_Exts for %s",key);
	      return;
	    }
	    col->enumCol=current_Ext_Column;
	    snprintf(col->title,ColumnTitleMaxLen,"%s",key);
	    col->ValueFunc=getExtColumnValueFromHashTable;

            currentPageStartingColumnTitleIndex++;
	    snprintf(currentColumnOriginalTitle,10,"C%d", currentPageStartingColumnTitleIndex);
	  }

	  int currentExtColumnHashTableIndex = current_Ext_Column - COL_Ext1;
          if (currentExtColumnHashTableIndex<0) { g_warning("ExtColumn name conflict with builtin column name:%s",col->title); return; };
	  if (ExtColumnHashTable[currentExtColumnHashTableIndex]==NULL) ExtColumnHashTable[currentExtColumnHashTableIndex] = g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);

	  char* strFileAttributeID=calloc(10, sizeof(char));
	  snprintf(strFileAttributeID,10, "%d", fileAttributeID);
	  g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"Insert column %s into ExtColumnHashTable[%d]: fileAttributeID as key %s,value %s", col->title, currentExtColumnHashTableIndex,strFileAttributeID,currentExtColumnValue);
	  g_hash_table_insert(ExtColumnHashTable[currentExtColumnHashTableIndex], strFileAttributeID, currentExtColumnValue);
}


static char* get_json_str_value(JsonObject *obj, const gchar *key) {
    JsonNode *node = json_object_get_member(obj, key);
    if (!node) {
        g_warning("Key '%s' not found\n", key);
        return NULL;
    }

    switch (json_node_get_value_type(node)) {
        case G_TYPE_STRING:
            return g_strdup_printf("%s", json_node_get_string(node));
        case G_TYPE_INT64:
            return g_strdup_printf("%ld", json_node_get_int(node));
        case G_TYPE_DOUBLE:
            return g_strdup_printf("%f", json_node_get_double(node));
        case G_TYPE_BOOLEAN:
            return g_strdup_printf("%s", json_node_get_boolean(node) ? "true" : "false");
        case G_TYPE_INVALID:
        case G_TYPE_NONE:
            return NULL;
        default:
            return g_strdup_printf("Unhandled type");
    }
}

static int ProcessJsonStrWrapper(RFM_ChildAttribs *child_attribs) {
  ProcessJsonStr(child_attribs->stdOut);
}

static int ProcessJsonStr(CALLER_OWN gchar* json_data){
    // 解析JSON字符串
    GError *error = NULL;
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, json_data, -1, &error)) {
        g_warning("Error parsing JSON: %s", error->message);
        g_error_free(error);
    }else{
         // 获取根节点（假设根为JsonObject）
        JsonNode *root = json_parser_get_root(parser);
        JsonObject *root_obj = json_node_get_object(root);
        GList * keys = json_object_get_members(root_obj);
        g_log(RFM_LOG_DATA, G_LOG_LEVEL_DEBUG, "number of keys:%d", g_list_length(keys));

        GList *key = g_list_first(keys);
	char currentColumnOriginalTitle[10];
	snprintf(currentColumnOriginalTitle,10,"C%d", currentPageStartingColumnTitleIndex);

	while (key){
	  gchar* value = get_json_str_value(root_obj,key->data);
	  if (value) ProcessKeyValuePair(key->data, value, currentColumnOriginalTitle);
	  //else g_warning("ProcessJsonStr. value for key %s not found:", key->data);
	  key = g_list_next(key);
	}
	g_list_free(keys);
    }
    g_object_unref(parser);
}

static int ProcessJsonFileFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, const char** cmdTemplate, const RFM_FileAttributes* fileAttributes, RFM_ChildAttribs *childAttribs){
   if (new_search){
     CallDefaultSearchResultFunctionForNewSearch(oneline, TRUE, cmdTemplate, fileAttributes, childAttribs);
     return 0;// 对于new_search, 调用本方法的目的仅仅是调用default的ProcessOnelineForSearchResult
   }
   gchar * absolute_address = g_canonicalize_filename(oneline, rfm_SearchResultPath);

    // 读取JSON文件内容
    gchar *json_data = NULL;
    GError *error = NULL;
    if (!g_file_get_contents(absolute_address, &json_data, NULL, &error)) {
        g_warning("Error reading file: %s", error->message);
        g_error_free(error);
    }else{
        ProcessJsonStr(json_data);
        g_free(json_data);
    }

    g_free(absolute_address);
}


// 查看 SearchResultLineProcessingFunc 函数指针了解此函数调用
// 这个函数的 cmdtemplate 有点类似 parse_and_exec_cmd_in_gtk_thread 的readlineResult, （还是等同？）
// 但这个函数和 parse_and_exec_cmd_in_gtk_thread  的不同在于本函数是处理查询结果的一条记录的，即一个查询到的文件，cmdtemplate 是用来解析这个文件内容的，处理的上下文就是这个文件。
// 而 parse_and_exec_cmd_in_gtk_thread 处理用户输入或config.h写死的命令时会考虑当前选中文件上下文
// cmdtemplate的解析结果会通过 cmdOutput 局部变量传递到下一环节。这个cmdOutput会对应到查询结果的一行
// 而parse_and_exec_cmd_in_gtk_thread 里的readlineResult是返回这个查询结果多行记录的，每行一个文件。
// 所以这两个函数看上去都是执行命令模板的，有类似之处，却不好合并
int ProcessKeyValuePairInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, char** cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* childAttribs){
  ProcessCmdOutputFromSearchResultWithFunction(oneline,new_search,cmdtemplate,fileAttributes,childAttribs,ProcessGKeyfileStrWrapper);
}

int ProcessJsonInCmdOutputFromSearchResult(CALLER_OWN char *oneline, gboolean new_search, char** cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* child_attribs){
  ProcessCmdOutputFromSearchResultWithFunction(oneline,new_search,cmdtemplate,fileAttributes,child_attribs,ProcessJsonStrWrapper);
}

//SearchResultType 里面定义的cmdTemplate格式是独特的：一方面，可以指明cmd_interpretor, 使用match_cmd_str_array_with_interpretors, 另一方面，却又不是通过parse_and_exec_command_in_gtk_thread来执行，通过调用cmd_interpretor的cmdTransformer来转换成 g_spawn_wrapper接受的形式。同时做了一个简单的把cmdTemplate中%s替换成查询结果文件名oneline的操作，这个替换既没使用build_cmd_vector中的文件名替换，又没使用parse_and_exec_command_in_gtk_thread中的选中文件名替换
static int ProcessCmdOutputFromSearchResultWithFunction(CALLER_OWN char *oneline, gboolean new_search, char** cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* child_attribs, int (*func)(RFM_ChildAttribs*)){
   if (new_search){
     CallDefaultSearchResultFunctionForNewSearch(oneline, TRUE, cmdtemplate, fileAttributes, child_attribs);
     return 0;// 对于new_search, 调用本方法的目的仅仅是调用default的ProcessOnelineForSearchResult
   }

   char SearchResultOnelineDocProcessingCmd[PATH_MAX];

   stdin_cmd_interpretor *cmd_interpretor_pointer = match_cmd_str_array_with_interpretors(cmdtemplate);//TODO:这个函数扩充到AI,Addon interpretor
   char** cmd;
   gchar * absolute_address = g_canonicalize_filename(oneline, rfm_SearchResultPath);
   if (cmd_interpretor_pointer){//用户在config.h里定义的命令模板字符串数组，如果第0个字符串指定命令解释器，则第1个字符串指定命令模板; 否则，第0个字符串即是命令模板
     snprintf(SearchResultOnelineDocProcessingCmd,PATH_MAX,cmdtemplate[1],quote_path(&absolute_address));
     cmd = cmd_interpretor_pointer->cmdTransformer(SearchResultOnelineDocProcessingCmd, FALSE);
   }else{
     snprintf(SearchResultOnelineDocProcessingCmd,PATH_MAX,cmdtemplate[0],quote_path(&absolute_address));
     cmd = current_stdin_cmd_interpretor_pointer->cmdTransformer(SearchResultOnelineDocProcessingCmd, FALSE);
   }

   g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"ProcessCmdOutputFromSearchResultWithFunction file:%s, cmd:%s, MatchedCmdInterpretor:%s, current_stdin_cmd_interpretor %s", absolute_address, SearchResultOnelineDocProcessingCmd, cmd_interpretor_pointer?cmd_interpretor_pointer->name:"null", current_stdin_cmd_interpretor_pointer?current_stdin_cmd_interpretor_pointer->name:"null");
   g_free(absolute_address);

   RFM_ChildAttribs *childAttribs = calloc(1, sizeof(RFM_ChildAttribs));
   childAttribs->customCallBackFunc = func;
   childAttribs->RunCmd = cmd;
   childAttribs->spawn_async = FALSE;
   childAttribs->output_read_by_program=TRUE;
   childAttribs->setsid = child_attribs->setsid;
   if (!no_window) childAttribs->runOpts = G_SPAWN_STDIN_FROM_DEV_NULL; //如果在no_window模式下设置STDIN_FROM_DEV_NULL, 就会报错：无法设定终端进程组 (-1): 对设备不适当的 ioctl 操作。bash: 此 shell 中无任务控制
   if (child_attribs->toolbar_button_or_filemenu_addon_dir) childAttribs->toolbar_button_or_filemenu_addon_dir = strdup(child_attribs->toolbar_button_or_filemenu_addon_dir);
   if (child_attribs->searchresulttype_addon_dir) childAttribs->searchresulttype_addon_dir = strdup(child_attribs->searchresulttype_addon_dir);
   g_spawn_wrapper_(NULL,NULL,childAttribs);
}

static int ProcessGKeyfileStrWrapper(RFM_ChildAttribs *child_attribs) {
  ProcessGKeyfileStr(child_attribs->stdOut);
}

static int ProcessGKeyfileStr(CALLER_OWN gchar *cmdOutput){
     GError *error=NULL;
     GKeyFile *keyfile=g_key_file_new();
     if (!g_key_file_load_from_data(keyfile, cmdOutput, strlen(cmdOutput), G_KEY_FILE_NONE, &error)){
       g_warning("ProcessGKeyfileStr. error loading key value pair in cmdOutput, error code: %d, message:%s", error==NULL?0:error->code, error==NULL?"":error->message);
       if (error) g_error_free(error);
     }else{
       ProcessKeyValuePairInData(keyfile, dumb_keyfile_groupname);
       g_key_file_free(keyfile);
     }
}

static int ProcessKeyValuePairInData(GKeyFile *keyfile, char* groupname){
   gsize size=0;
   gchar** keys=NULL;
   GError *error=NULL;
   keys = g_key_file_get_keys(keyfile,groupname,&size, &error);
   g_log(RFM_LOG_DATA, G_LOG_LEVEL_DEBUG, "number of keys:%ld",size);

   char currentColumnOriginalTitle[10];
   snprintf(currentColumnOriginalTitle,10,"C%d", currentPageStartingColumnTitleIndex);

   for(gsize i=0; i<size; i++){
       gchar* currentExtColumnValue = g_key_file_get_string(keyfile, groupname, keys[i], &error);
       if (currentExtColumnValue==NULL){
	 g_warning("ProcessKeyValuePairInData. groupname %s, keyvalue %s, not found, error:%s",groupname,keys[i],error? error->message:"");
	 g_error_free(error); error=NULL;continue;
       }else ProcessKeyValuePair(keys[i], currentExtColumnValue, currentColumnOriginalTitle);
   }
   g_strfreev(keys);
}


// 根据文件属性,例如文件名后缀,调用适当的处理函数
// new_search为FALSE时,oneline仅包含文件路径
// 用SearchResultType[i].name 去匹配文件名后缀,以匹配到的最长后缀为准
// 比如 searchresulttype name 有 ".md", 还有 "TODO.md"
// 一个没匹配到,默认用 searchresultype[0]
 static int CallMatchingProcessorForSearchResultLine(char *oneline, gboolean new_search, char *cmdtemplate, RFM_FileAttributes* fileAttributes, RFM_ChildAttribs* childAttribs){
   assert(g_strcmp0(SearchResultTypePointerForCurrentExistingSearchResult->name,SrchRsltType_multitype)==0);
   if (new_search){
     CallDefaultSearchResultFunctionForNewSearch(oneline, TRUE, cmdtemplate, fileAttributes , childAttribs);
     return 0;// 对于new_search, 调用本方法的目的仅仅是调用default的ProcessOnelineForSearchResult
   }

   char* longest_matched_searchresulttype_name = "";
   RFM_SearchResultType * matched_searchresulttype_pointer = &(searchresultTypes[0]);//没有匹配到得话就直接调用 default, 0号
   for(guint i=0; i<G_N_ELEMENTS(searchresultTypes); i++){
     if (g_str_has_suffix(oneline, searchresultTypes[i].name)){
       if (strlen(searchresultTypes[i].name)>strlen(longest_matched_searchresulttype_name)){
	 longest_matched_searchresulttype_name = searchresultTypes[i].name;
	 matched_searchresulttype_pointer = &(searchresultTypes[i]);
       }
     }
   }

#ifdef RFM_AddOn
	    GHashTableIter ht_iter;
	    gchar *key;
	    RFM_ADDON *value;
	    g_hash_table_iter_init(&ht_iter, addons);
	    while (g_hash_table_iter_next(&ht_iter, &key, &value)){
	           RFM_ADDON_RESOURCE_TYPE* addon_resource = value->resource_managed_by_addon;
		   for (guint i = 0; i < addon_resource->searchresulttypes_array_size; i++) {
		         if (g_str_has_suffix(oneline, addon_resource->searchresulttypes[i].name)){//注意这里和MatchSearchResultType函数的区别，那里匹配的是加上 RFM_SearchResultTypeNamePrefix 的searchresulttype。TODO:不能扩展下MatchSearchResultType函数，在这里也调用吗？代码会短些
			   if (strlen(addon_resource->searchresulttypes[i].name)>strlen(longest_matched_searchresulttype_name)){
			     longest_matched_searchresulttype_name = addon_resource->searchresulttypes[i].name;
			     matched_searchresulttype_pointer = &(addon_resource->searchresulttypes[i]);
			     childAttribs->searchresulttype_addon_dir = g_path_get_dirname(value->addon_c_fullpath);
			   }
			 }
		   }
	    }
#endif
	    g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"%s: SearchResultType name %s matched for file:%s", SrchRsltType_multitype,longest_matched_searchresulttype_name,oneline);
	    int ret = matched_searchresulttype_pointer->SearchResultLineProcessingFunc(oneline,new_search,matched_searchresulttype_pointer->cmdTemplate, fileAttributes, childAttribs);
	    g_free(childAttribs->searchresulttype_addon_dir); childAttribs->searchresulttype_addon_dir = NULL;//当前的searchresulttype是multitype,对应的addon_dir是null
   //对于multitype来说，每一行的showcolumn应该是匹配到的具体searchresulttype的showcolumn, 所以应该给每种匹配到的showcolumn做个并集
   if (SearchResultTypePointerForCurrentExistingSearchResult->showcolumn==NULL){
     SearchResultTypePointerForCurrentExistingSearchResult->showcolumn = g_ptr_array_new();
     g_debug("SearchResultTypePointerForCurrentExistingSearchResult->showcolumn set to GPtrArray* %p for SearchResultType %s %p",SearchResultTypePointerForCurrentExistingSearchResult->showcolumn, SearchResultTypePointerForCurrentExistingSearchResult->name, SearchResultTypePointerForCurrentExistingSearchResult);
   }
   if (matched_searchresulttype_pointer->showcolumn){
       for(int i=0; matched_searchresulttype_pointer->showcolumn[i];i++){
	 gboolean already_exists_in_multitype_showcolumn = FALSE; //用于避免重复加入字符串数组元素
	 for(guint j=0; j<((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn))->len;j++)
	   if (matched_searchresulttype_pointer->showcolumn[i]==((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn))->pdata[j]) { already_exists_in_multitype_showcolumn = TRUE; break; }
	 if (!already_exists_in_multitype_showcolumn) {
	   g_ptr_array_add((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn), matched_searchresulttype_pointer->showcolumn[i]);
	   g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"%s: SearchResultType name %s matched for file:%s; showcolumn element '%s' added",SrchRsltType_multitype,longest_matched_searchresulttype_name,oneline,matched_searchresulttype_pointer->showcolumn[i]);
	 }
       }
   }
   return ret;
}

static gchar* getExtColumnValueFromHashTable(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex){
   if (ExtColumnHashTable[ExtColumnHashTableIndex]==NULL) return NULL;
   gchar key[10];
   snprintf(key,10, "%d", fileAttribs->id);
   gchar *ret=g_hash_table_lookup(ExtColumnHashTable[ExtColumnHashTableIndex], key);
   g_log(RFM_LOG_DATA_EXT, G_LOG_LEVEL_DEBUG, "getExtColumnValueFromHashTable(fileAttributeId %d, ExtColumnHashTableIndex %d) return %s",fileAttribs->id,ExtColumnHashTableIndex,ret);
   return ret;
}

static gchar* getDimensionValueFromHashTable(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex){
   if (ExtColumnHashTable[ExtColumnHashTableIndex]==NULL) return NULL;
   gchar *ret=g_hash_table_lookup(ExtColumnHashTable[ExtColumnHashTableIndex], fileAttribs->path);
   g_log(RFM_LOG_DATA_EXT, G_LOG_LEVEL_DEBUG, "getDimensionValueFromHashTable(fullpath %s, ExtColumnHashTableIndex %d) return %s",fileAttribs->path,ExtColumnHashTableIndex,ret);
   if (!ret && fileAttribs->is_symlink) {
     // 构建符号链接所在目录的GFile
     GFile *symlink_file = g_file_new_for_path(fileAttribs->path);
     GFile *parent_dir = g_file_get_parent(symlink_file);
     // 解析相对路径
     GFile *target_file = g_file_resolve_relative_path(parent_dir, fileAttribs->link_target_filename);
     gchar *abs_path = g_file_get_path(target_file);
     ret=g_hash_table_lookup(ExtColumnHashTable[ExtColumnHashTableIndex], abs_path);
     g_log(RFM_LOG_DATA_EXT, G_LOG_LEVEL_DEBUG, "getDimensionValueFromHashTable(linktarget fullpath %s, ExtColumnHashTableIndex %d) return %s",abs_path,ExtColumnHashTableIndex,ret);
     g_free(abs_path);g_object_unref(target_file);g_object_unref(parent_dir);g_object_unref(symlink_file);
   }
   return ret;
}


static NO_FREE RFM_FileAttributes *get_fileAttributes_byId(guint fileAttributeId) {
   GList *listElement;
   RFM_FileAttributes *fileAttributes;
   if (rfm_fileAttributeList==NULL) return NULL;
   listElement=g_list_first(rfm_fileAttributeList);
   while (listElement != NULL) {
     fileAttributes=(RFM_FileAttributes*)listElement->data;
     if (fileAttributes->id == fileAttributeId) return fileAttributes;
     listElement=g_list_next(listElement);
   }
}

static void ReadFromPipeStdinIfAny(char * fd)
{
   if (readlink_proc_self_fd_pipefd_and_confirm_pipefd_is_used_by_pipeline()){
         //if (initDir!=NULL && (SearchResultViewInsteadOfDirectoryView^1)) die("if you have -d specified, and read file name list from pipeline, -p parameter must goes BEFORE -d\n");
         //TODO：本commit 移动了ReadFromPipeStdinIfAny调用在setup里的位置, 所以上面一行不能要了，否则rfm直接退出，但我这会儿不太记得上面这句在这里有啥作用
	 RFM_ChildAttribs *childAttribs = calloc(1, sizeof(RFM_ChildAttribs));
         childAttribs->no_free = TRUE;
	 if (read_char_pipe(atoi(fd), PIPE_SZ, &childAttribs->stdOut)<0) g_warning("read_char_pipe failed");
         SearchResultTypePointer = &(searchresultTypes[1]);//multitype
         update_SearchResultFileNameList_and_refresh_store(childAttribs);
	 ensure_fd0_is_opened_for_stdin_inherited_from_parent_process_instead_of_used_by_pipeline_any_more();
   }
}

void handle_commandline_argument(char* arg){
   if (g_str_has_prefix(arg, "/dev/fd/")) {
      //try  `gdb --args rfm <(locate rfm.c)` and find in htop what this /dev/fd means
      pipefd = strdup(arg + 8);
   }
}

int readlink_proc_self_fd_pipefd_and_confirm_pipefd_is_used_by_pipeline(){
   static char buf[PATH_MAX];
   char name[50];
   snprintf(name,50,"/proc/self/fd/%s", pipefd);
   int rslt = readlink(name, buf, PATH_MAX);
   if (rslt>4) {
     buf[4]='\0';
     if (strcmp(buf,"pipe")==0){
       if (atoi(pipefd)==STDIN_FILENO) pipeStream=stdin;
       else pipeStream=fdopen(atoi(pipefd),"r");
       return TRUE;
     }
   }
   return FALSE;
}

void ensure_fd0_is_opened_for_stdin_inherited_from_parent_process_instead_of_used_by_pipeline_any_more(){
         if (atoi(pipefd) == STDIN_FILENO) { // open parent stdin to replace pipe
           char *tty = ttyname(STDOUT_FILENO);
	   int pts=open(tty,O_RDWR);
	   dup2(pts,STDIN_FILENO); //i guess dup2 call fclose for pipeStream(STDIN_FILENO) here
	   close(pts);
         }else fclose(pipeStream);
}


//对于searchresulttype withTitle 的，显示输入的title列
//for searchresults that contains only filepath, like those from locate, we use the same column layout as directory view
//for those contain more than filepath(SearchResultContainNonFilenameColumn), the additionl column usually will be filled in the ExtColumnHashTable, we use following layout.
//参见 RFM_SearchResultType.showcolumn 属性注释。
static void showSearchResultExtColumnsBasedOnHashTableValues(){

  gboolean old_value=do_not_show_VALUE_MAY_NOT_LOADED_message_because_we_will_add_GtkTreeViewColumn_later;
  do_not_show_VALUE_MAY_NOT_LOADED_message_because_we_will_add_GtkTreeViewColumn_later=TRUE;
  if (search_result_type_withtitle_column_title_array){//只有在searchresulttype(即(SearchResultTypePointerForCurrentExistingSearchResult)是 withTitle 的情况下才不为空

         for(int i=0;i<NUM_Ext_Columns;i++) if (ExtColumnHashTable[i]){//ProcessOnelineForSearchResult_with_title 的时候，对于没列输入的数据，都使用ExtColumnHashTable保存，并把title保存在key 1 里，对于现有的同名列，如文件属性，DimensionValue,也使用单独的ExtColumnHashTable。
	   char* column_title_in_first_line = (char*)g_hash_table_lookup(ExtColumnHashTable[i], "1");
	   if (column_title_in_first_line){
	       RFM_treeviewColumn *existingColWithTheSameTitle = get_treeviewColumnByTitle(column_title_in_first_line);
	       if (existingColWithTheSameTitle){ //如有同名列，列名会保持C1,C2,这种形式，而不会改成冲突的名字。FileNameIndexInColumnTitles 不为0可以等同于下面search_result_type_withtitle_column_title_array不为空的判断，因为下面search_result_type_withtitle_column_title_array时使用的是column title字符串显示列，这样，ProcessOnelineForSearchResult_with_title 里保存在ExtColumnHashTable里的值实际不会被显示，实际显示的是从文件属性读取的。TODO:这种机制也许将来可以被用来导入并批量更新文件属性
		 g_debug("showSearchResultExtColumnsBasedOnHashTableValues:column %s in first line already exists", column_title_in_first_line);
		 if ((existingColWithTheSameTitle->enumCol < NUM_COLS) && (existingColWithTheSameTitle->enumCol >= COL_Ext1)) ExtColumnHashTable_keep_during_refresh[existingColWithTheSameTitle->enumCol - COL_Ext1] = TRUE;
	       }else{
		 RFM_treeviewColumn *col = get_treeviewColumnByEnum(i + COL_Ext1);
		 snprintf(col->title,ColumnTitleMaxLen,"%s",(char*)g_hash_table_lookup(ExtColumnHashTable[i], "1"));
	       }
	       //g_free(column_title_in_first_line); 文档上说g_hash_table_lookup的返回值是caller own的，但这里free会造成代码在别的地方莫名crash
	   }else g_debug("showSearchResultExtColumnsBasedOnHashTableValues:no column title for key 1 in ExtColumnHashTable[%d]",i);
         }

         int num_of_columns = g_strv_length(search_result_type_withtitle_column_title_array);
         gchar **tmp_strings = calloc(num_of_columns + 3, sizeof(gchar*));// 多两个位置用于首尾的逗号,加上endingNULL
         tmp_strings[0] = ""; tmp_strings[num_of_columns + 1] = "";
         for (int i = 0; i < num_of_columns; i++) {
           tmp_strings[i + 1] = search_result_type_withtitle_column_title_array[i]; // 原始字符串
         }
         gchar* column_titles_begin_and_end_with_comma = g_strjoinv(",", tmp_strings);
         show_hide_treeview_columns_in_order(column_titles_begin_and_end_with_comma);
         g_free(tmp_strings); g_free(column_titles_begin_and_end_with_comma);

  }else if(SearchResultContainNonFilenameColumn){//只显示文件名和这些列,而不显示那些常规的如文件大小日期列,如要显示这些列,可在 showcolumn 里面指定.
         show_hide_treeview_columns_enum(3, INT_MAX,COL_FILENAME,INT_MAX);
	 enum RFM_treeviewCol previousColumn = COL_FILENAME;
	 for(int i=COL_Ext1; i<FirstDimensionCol;i++){ //DimensionColumn在dimProps->use_getExtColumnValueFromHashTable的情况下，总是会填充值的，及时.Show 属性值是false的情况下，并且，一般是不会把DimensionValue通过查询结果类型读入的，除非将来通过withTitle的方式，批次读入，用以批次更新 DimensionValue, 当前条件分支上一个分支内的TODO
	   if (ExtColumnHashTable[i-COL_Ext1]!=NULL){
	     show_hide_treeview_columns_enum(3, previousColumn,i,INT_MAX);//INT_MAX 结尾表示只显示指明的列,不显示其他默认显示常规列如文件大小日期.这样设计的出发点是:当我们在查询结果里只包含文件名时,我们希望显示文件名,大小,日期等如同目录视图一样的列. 而当查询结果包含除文件名之外的扩展列时,我们希望只显示文件名和这些扩展列,而不包括文件大小日期等常规列,以便让用户注意力能集中在这些扩展列上
	     previousColumn = i;
	   }
	 }
  }

  guint i=0;
  gchar* column_sequence;
  if (g_strcmp0(SearchResultTypePointerForCurrentExistingSearchResult->name,SrchRsltType_multitype)==0 ?((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn)) : SearchResultTypePointerForCurrentExistingSearchResult->showcolumn){
    while ((column_sequence = (g_strcmp0(SearchResultTypePointerForCurrentExistingSearchResult->name,SrchRsltType_multitype)==0 ?
			             (i<(((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn))->len) ?
				          g_ptr_array_index((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn), i)
				          :NULL)
			             : SearchResultTypePointerForCurrentExistingSearchResult->showcolumn[i]))){
      show_hide_treeview_columns_in_order(column_sequence);//如果查询结果不包含getExtColumnValueFromHashTable列,只包含文件名,默认会显示文件日期大小等常规列,如果不想显示这些列,可以指定 { ",FileName,", NULL } 这样一个包含 ",FileName," 逗号开头结尾的showcolumn字符串数组
      i++;
    }
  }
  do_not_show_VALUE_MAY_NOT_LOADED_message_because_we_will_add_GtkTreeViewColumn_later=old_value;
  showSearchResultExtColumnsBasedOnHashTableValues_has_already_been_called_for_searchresult = TRUE;
  if (g_strcmp0(SearchResultTypePointerForCurrentExistingSearchResult->name,SrchRsltType_multitype)==0 && SearchResultTypePointerForCurrentExistingSearchResult->showcolumn) {
    g_ptr_array_free((GPtrArray*)(SearchResultTypePointerForCurrentExistingSearchResult->showcolumn), FALSE); //multitype searcharesulttype 的showcolumn属性默认是NULL的，动态添加  MatchedSearchResultType 的showcolumn, 用完后释放字符串数组，字符串本身不用释放
    SearchResultTypePointerForCurrentExistingSearchResult->showcolumn = NULL;
    g_debug("free SearchResultTypePointerForCurrentExistingSearchResult->showcolumn and set to NULL for SearchResultType %s %p", SearchResultTypePointerForCurrentExistingSearchResult->name, SearchResultTypePointerForCurrentExistingSearchResult);
  }
}


// 根据 SearchResultTypeIndex 调用SearchResultLineProcessingFunc
// 进入查询结果FirstPage,这个会调用refresh_store
// 准备ExtColumnHashTable, 释放旧值
// 虽然此处初始化和释放(也是通过old指针)了SearchResultFileNameList,
// 但实际添加值是通过默认查询结果类型处理函数 ProcessOnelineForSearchResult 添加的
// 所以特定的SearchResultLineProcessingFunc都会先调用此默认处理函数
//
// SearchResultLineProcessingFunc 实际会被调用两次,
// 一次是本方法,另一次通过refresh_store调用
// fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store();
// 两次调用通过 newsearch 参数区别
// 本函数是newsearch 为True, 对应新的查询结果集
// fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store 里的调用 newsearch 为false,对应页面刷新以及翻页
static void update_SearchResultFileNameList_and_refresh_store(RFM_ChildAttribs *childAttribs){
  g_assert(SearchResultTypePointer);
  gchar *filenamelist = childAttribs->stdOut;
  
  GList * old_filenamelist = SearchResultFileNameList;
  static GHashTable* old_ExtColumnHashTable[NUM_Ext_Columns + 1]; //静态存储期（static storage duration）的变量（包括全局变量和用static 关键字声明的局部变量）如果没有显式初始化，会被自动初始化为零。
  SearchResultFileNameList = NULL;
  for(int i=0;i<=NUM_Ext_Columns;i++) {
    if (old_ExtColumnHashTable[i] != NULL) g_warning("update_SearchResultFileNameList_and_refresh_store 函数重入了，造成HashTable内存泄漏");
    old_ExtColumnHashTable[i]=ExtColumnHashTable[i];
    ExtColumnHashTable[i]=NULL;
    ExtColumnHashTable_keep_during_refresh[i]=FALSE;
  }
  FirstDimensionCol = NUM_COLS;
  SearchResultContainNonFilenameColumn = FALSE;
  SearchResultViewInsteadOfDirectoryView = 1;
  memcpy(SearchResultViewColumnsLayout, treeviewColumns, sizeof(RFM_treeviewColumn)*G_N_ELEMENTS(treeviewColumns));
#ifdef RFM_AddOn
  TREEVIEW_COLUMNS_array_size = G_N_ELEMENTS(treeviewColumns);
  GHashTableIter ht_iter;
  gchar *key;
  RFM_ADDON *value;
  g_hash_table_iter_init(&ht_iter, addons);
  while (g_hash_table_iter_next(&ht_iter, &key, &value)){
    append_addon_treeviewColumns(value,TRUE,FALSE);
  }
#endif
  showSearchResultExtColumnsBasedOnHashTableValues_has_already_been_called_for_searchresult = FALSE;
  char old_searchresultcolumnSeperator[32];
  //apply searchresulttype specific column seperator
  if (SearchResultTypePointer->SearchResultColumnSeperator) {
    strcpy(old_searchresultcolumnSeperator, SearchResultColumnSeperator);
    strcpy(SearchResultColumnSeperator, SearchResultTypePointer->SearchResultColumnSeperator);
  }
  SearchResultFileNameListLength=0;
  fileAttributeID=1;

  g_free(rfm_SearchResultPath);
  rfm_SearchResultPath=strdup(rfm_curPath);//按理说在这个应该在执行查询命令获得filenamelist的时候获取,但Search可能是在exec_stdin_command里执行，也可能在exec_stdin_command_with_searchresultasinput 里执行，要重复写了，所以就放这里了
  g_debug("update_SearchResultFileNameList_and_refresh_store, rfm_SearchResultPath %s", rfm_SearchResultPath);
  g_log(RFM_LOG_DATA_SEARCH,G_LOG_LEVEL_DEBUG,"update_SearchResultFileNameList, length: %ld charactor(s)",filenamelist?strlen((gchar*)filenamelist):0);

  //释放上次查询的childAttribs,换上这次新的
  if (childAttribsForCurrentDisplayingSearchResult){
    childAttribsForCurrentDisplayingSearchResult->no_free = FALSE;
    free_child_attribs(childAttribsForCurrentDisplayingSearchResult);
  }
  childAttribsForCurrentDisplayingSearchResult = childAttribs;

  if (filenamelist){
  gchar * oneline=strtok((gchar*)filenamelist,"\n");
  while (oneline!=NULL){
    SearchResultTypePointer->SearchResultLineProcessingFunc(oneline, TRUE, SearchResultTypePointer->cmdTemplate,NULL, childAttribs);
    fileAttributeID++;
    oneline=strtok(NULL, "\n");
  }
  }

  if (SearchResultFileNameList != NULL) SearchResultFileNameList=g_list_first(g_list_reverse(SearchResultFileNameList));
  current_sort_column_id = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  SearchResultTypePointerForCurrentExistingSearchResult=SearchResultTypePointer;//执行到这里一次查询才算是成功了，保留指针以便以后 refresh_store 操作
  
  if (SearchResultTypePointer->SearchResultColumnSeperator) {
    strcpy(SearchResultColumnSeperator, old_searchresultcolumnSeperator);
  }
  SearchResultTypePointer = NULL; //完成向SearchResultTypePointerForCurrentExistingSearchResult转移后清零,TODO:如果Search命令执行失败如何清零?把他们从全局变量改为childAttribs属性应该就好了

  FirstPage();
  if (old_filenamelist!=NULL) g_list_free(old_filenamelist);
  for(int i=0;i<=NUM_Ext_Columns;i++) if (old_ExtColumnHashTable[i]!=NULL) { g_hash_table_destroy(old_ExtColumnHashTable[i]); old_ExtColumnHashTable[i] = NULL; }
}

static gboolean startWithTerm(){
  return rfmStartWithVirtualTerminal && isatty(0);
}

#ifdef RFM_FILE_CHOOSER
GList* str_array_ToGList(char* a[]){
  GList * ret = NULL;
  if (a!=NULL){
    guint c=g_strv_length(a);
    for(guint i=0;i<c;i++) ret=g_list_prepend(ret, a[i]);
  }
  return ret;
}

char** GList_to_str_array(GList *l, int count) {
  if (l==NULL) return NULL;
  char** ret = calloc(count+1, sizeof(char*));
  l=g_list_first(l);
  for(int i=0;i<count;i++) {
    ret[i]=l->data;
    l=g_list_next(l);
  }
  g_list_free(l);
  return ret;
}

static void rfmFileChooserResultReader(RFM_ChildAttribs* child_attribs){
  gchar *child_StdOut=NULL;
  if (child_attribs->spawn_async){
    child_StdOut = child_attribs->stdOut;
  }else{
    //g_spawn_sync has the parameter stdOut, but no stdOut_fd as g_spawn_async, so i readout from the namedpipefd here.
    read_char_pipe(child_attribs->stdOut_fd, PIPE_SZ, &child_StdOut); //TODO: g_warning("read_char_pipe failed") if <0 ?
    close(child_attribs->stdOut_fd);
  }

  int returnedCount = 0;
  //TODO: g_spawn_wrapper seem not dealing with exitcode
  GList** fileSelectionList = child_attribs->customCallbackUserData; //This is actually the global fileChooserSelectionList
  g_list_free_full(*fileSelectionList, (GDestroyNotify)g_free); // The file path str have already been freed with command returned by build_cmd_vector called by g_spawn_wrapper_ ? Maybe not, the build_cmd_vector take file names owned by rfm_FileAttributelist before and never free filefullpath. but here filefullpath comes from test_rfmFilechooser
  *fileSelectionList=NULL;
  if(child_StdOut!=NULL) {
    gchar * oneline=strtok(child_StdOut,"\n");
    while (oneline!=NULL){
      *fileSelectionList = g_list_prepend(*fileSelectionList, strdup(oneline));
      g_debug("rfmFileChooser return:%s",oneline);
      oneline=strtok(NULL, "\n");
      returnedCount++;
    }
    if (!child_attribs->spawn_async) g_free(child_StdOut);
  }else{
    g_debug("rfmFileChooser return nothing");
  }

  char named_pipe_name[50];
  snprintf(named_pipe_name,50, "%s%d", RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX,getpid());
  remove(named_pipe_name);

  if (child_attribs->spawn_async && FileChooserClientCallback!=NULL) (*FileChooserClientCallback)(GList_to_str_array(*fileSelectionList,returnedCount));
}


   /* default selection files can be passed in with fileSelectionList. After user */
   /* interaction, this list is returned with user selection. And the default */
   /* selection is freed in rfmFileChooserResultReader */
GList* rfmFileChooser_glist(enum rfmTerminal startWithVirtualTerminal, char* search_cmd, gboolean async, GList** fileChooserSelectionListAddress, void (*fileChooserClientCallback)(char **)) {
  char named_pipe_name[50];
  snprintf(named_pipe_name,50, "%s%d", RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX,getpid());
  if (async && fileChooserClientCallback==NULL) { g_warning("to call rfmFileChooser async, fileChooserClientCallback function pointer must be provided."); return NULL; }
  FileChooserClientCallback = fileChooserClientCallback;
  // 同步spawn的时候,我们依然使用namedpipe,而不是stdout来传递返回的选中文件,主要是因为stdout里面有可能会混入rfm的其他输出内容,专门的namedpipe就没这个问题
  if (mkfifo(named_pipe_name, 0700)==0){ //0700 is  rwx------ https://jameshfisher.com/2017/02/24/what-is-mode_t/
    int named_pipe_fd = open(named_pipe_name, O_RDONLY|O_NONBLOCK);
    if (named_pipe_fd>0){
      g_debug("named_pipe_fd in calling process:%d",named_pipe_fd);
      RFM_ChildAttribs *child_attribs=calloc(1,sizeof(RFM_ChildAttribs));
      child_attribs->customCallBackFunc = rfmFileChooserResultReader;
      child_attribs->customCallbackUserData = fileChooserSelectionListAddress;
      child_attribs->stdOut_fd = named_pipe_fd;
      child_attribs->stdOut = NULL;
      child_attribs->stdErr = NULL;
      child_attribs->spawn_async = async;
      child_attribs->output_read_by_program=TRUE;
      child_attribs->name=g_strdup("rfmFileChooser");
      child_attribs->RunCmd = rfmFileChooser_CMD(startWithVirtualTerminal, search_cmd, GList_to_str_array(*fileChooserSelectionListAddress, g_list_length(*fileChooserSelectionListAddress)), named_pipe_name);

      if (g_spawn_wrapper_(NULL, NULL, child_attribs)) return *fileChooserSelectionListAddress;
    }else g_warning("failed to open %s",named_pipe_name);
  }else g_warning("mkfifo mode 0700 (rwx------) failed for:%s",named_pipe_name);
  return NULL;
}

char** rfmFileChooser(enum rfmTerminal startWithVirtualTerminal, char* search_cmd, gboolean async, char *fileSelectionStringArray[], void (*fileChooserClientCallback)(char**)) {
  fileChooserSelectionList = str_array_ToGList(fileSelectionStringArray);
  rfmFileChooser_glist(startWithVirtualTerminal, search_cmd, async, &fileChooserSelectionList, fileChooserClientCallback);
  if (async) return NULL;
  else return GList_to_str_array(fileChooserSelectionList, g_list_length(fileChooserSelectionList));
}

static gchar** rfmFileChooser_CMD(enum rfmTerminal startWithVT, gchar* search_cmd, gchar** defaultFileSelection, gchar* rfmFileChooserReturnSelectionIntoFilename){
    gchar* shell_cmd_buffer = calloc(ARG_MAX, sizeof(gchar));
    if (search_cmd == NULL || g_strcmp0(search_cmd,"")==0)
        snprintf(shell_cmd_buffer,PATH_MAX, "rfm.sh -r %s", rfmFileChooserReturnSelectionIntoFilename);
    else
        snprintf(shell_cmd_buffer,PATH_MAX, "exec %s | rfm.sh -r %s -p", search_cmd, rfmFileChooserReturnSelectionIntoFilename);

    if (defaultFileSelection != NULL){
      strcat(shell_cmd_buffer, " -d");
      guint c = g_strv_length(defaultFileSelection);
      for(guint i=0; i<c; i++)
	if (defaultFileSelection[i]!=NULL && strlen(defaultFileSelection[i])>0){
	  strcat(shell_cmd_buffer, " ");
	  strcat(shell_cmd_buffer, defaultFileSelection[i]);
	};
    };

    if (startWithVT == NEW_TERMINAL){
      return replace_shell_cmd_buffer(g_strdup(stdin_cmd_template_bash_newVT_nonHold), shell_cmd_buffer);
    }else if (startWithVT == NO_TERMINAL){
	strcat(shell_cmd_buffer, " -t");
        return replace_shell_cmd_buffer(g_strdup(stdin_cmd_template_bash), shell_cmd_buffer);
    }else return replace_shell_cmd_buffer(g_strdup(stdin_cmd_template_bash), shell_cmd_buffer);
}
#endif

//用replacement 替换 str 中匹配 pattern 的部分,匹配成功返回TRUE
//匹配成功后，*str原先的内存会被释放，*str会指向新分配的替换后的字符串
gboolean str_regex_replace(char **str, const regex_t * restrict pattern_compiled, const char *replacement) {
      gboolean ret = FALSE;
      regmatch_t pm[1];
      size_t str_len = strlen(*str);
      size_t nmatch = 1;
      size_t rc = regexec(pattern_compiled, *str, nmatch, pm, 0);
 
      while (rc == 0) {
	ret = TRUE;
        regoff_t soff = pm[0].rm_so;
        regoff_t eoff = pm[0].rm_eo;
 
        size_t buffer_len = str_len + strlen(replacement) - (eoff - soff);
        char* buffer = calloc(buffer_len + 1, sizeof(char));

	g_debug("before regex replace:%s",*str);
	memcpy(buffer, *str, soff);
	memcpy(buffer + soff, replacement, strlen(replacement));
	memcpy(buffer + soff + strlen(replacement), *str + eoff, str_len - eoff + 1); //位移是否要+1这个问题我总是拿不准, 为了防止复制越界, calloc 的时候我多申请1个, 这样总不会越界了吧, 如果少复制了字符,下面的g_debug 输出里容易看出来
	g_debug("after regex replace:%s",buffer);

	free(*str);
	*str = buffer;
	str_len = strlen(*str);
	
        rc = regexec(pattern_compiled, buffer + eoff, nmatch, pm, 0);
      }

      return ret;
}

//这里回顾一下，到目前为止，treeviewcolumns静态数组里有4种列：
//1.文件系统自带的，比如文件名，大小，这种的Title和enumCol在静态数组初始模板里写死的
//2.如图片尺寸这种预定义的扩展列，Title是写死的，所以总是会出现在showcolumn命令输出里，enumCol 需要在显示的时候使用get_available_ExtColumn 分配
//3.从查询结果里读出的列，列名事先不知道，没法写死在静态数组初始模板里，处理查询结果时按顺序在列名C1,C2,C3中使用get_available_ExtColumn 分配enumCol，如果查询结果首行是列名，则用首行列名替换Cx
//4.本函数处理的维度对应列，列名同维度名，而维度名也可以认为是动态的
static gboolean SetViewColumnForDimension(DimensionProperties* dimProps){
      RFM_treeviewColumn * col;
      gchar *dimName = dimProps->name;
      enum RFM_treeviewCol current_Ext_Column;
      if ((col=get_treeviewColumnByTitle(dimName))){ //已经有同名列了
	    current_Ext_Column=col->enumCol;
	    g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"SetViewColumnForDimension(%s), RFM_treeviewCol:%d already exist",dimName, current_Ext_Column);
      }else{//新列名
	    current_Ext_Column=get_available_ExtColumn_from_end(NUM_COLS-1);
	    if (current_Ext_Column==NUM_COLS) {
	      g_warning("Not enough COL_Exts for dimension %s",dimName);
	      return FALSE;
	    }else{
	      if (current_Ext_Column<FirstDimensionCol) FirstDimensionCol = current_Ext_Column;
	      char current_Ext_Column_Original_Title[10];
	      snprintf(current_Ext_Column_Original_Title,10,"C%d", current_Ext_Column-COL_Ext1+1);
	      col = get_treeviewColumnByTitle(current_Ext_Column_Original_Title);
	      col->enumCol=current_Ext_Column;
	      snprintf(col->title,ColumnTitleMaxLen,"%s",dimName);
	      col->ValueFunc = dimProps->use_getExtColumnValueFromHashTable? getExtColumnValueFromHashTable : getDimensionValueFromHashTable; //默認為false,所以是getDimensionValueFromHashTable. 使用getExtColumnValueFromHashTable, 需要為每一行文件spawn子進程，調用腳本獲取維度值，性能非常差，所以增加了 getDimensionvaluefromhashtable, 一次性把所有文件對於某一維度的值都裝入hashtable,然後再以fullpath,而不是fileAttributeID為key到hashtable中取維度值
	    }
      }
      int currentExtColumnHashTableIndex = current_Ext_Column - COL_Ext1;
      if (currentExtColumnHashTableIndex<0) { g_warning("dimension name conflict with builtin column name:%s",col->title); return FALSE; };
      if (ExtColumnHashTable[currentExtColumnHashTableIndex]==NULL) {
	      ExtColumnHashTable[currentExtColumnHashTableIndex] = g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
	      //if (SearchResultViewInsteadOfDirectoryView && col->Show) ExtColumnHashTable_keep_during_refresh[currentExtColumnHashTableIndex]=TRUE;
	      if (SearchResultViewInsteadOfDirectoryView) ExtColumnHashTable_keep_during_refresh[currentExtColumnHashTableIndex]=TRUE;
              g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"SetViewColumnForDimension(%s), g_hash_table_new_full called for ExtColumnHashTable[%d], RFM_treeviewCol:%d", dimName, currentExtColumnHashTableIndex, current_Ext_Column);
      }
      return TRUE;
}

static void load_ExtColumnHashTable_for_one_Dimension_for_GtkTreeIter(RFM_ChildAttribs* child_attribs) {
  RFM_store_cell *cell = child_attribs->customCallbackUserData;
  RFM_treeviewColumn *col = get_treeviewColumnByTitle(cell->dimProperties->name);
  char *child_stdout=(char*)(child_attribs->stdOut);
  RFM_FileAttributes *fileAttributes=NULL;
  gtk_tree_model_get (GTK_TREE_MODEL(store), cell->iter, COL_ATTR, &fileAttributes, -1);
  if (g_hash_table_insert(ExtColumnHashTable[col->enumCol - COL_Ext1], g_strdup_printf("%d",fileAttributes->id), strdup(child_stdout)))
    g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"load_ExtColumnHashTable_for_Dimension_for_GtkTreeIter, g_hash_table_insert called for ExtColumnHashTable[%d], RFM_treeviewCol:%d,%s, fileAttributes->id:%d, fileAttributes->path:%s", col->enumCol - COL_Ext1, col->enumCol,col->title, fileAttributes->id, fileAttributes->path);
  else
    g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"load_ExtColumnHashTable_for_Dimension_for_GtkTreeIter, g_hash_table_insert overwrite existing value for ExtColumnHashTable[%d], RFM_treeviewCol:%d,%s, fileAttributes->id:%d, fileAttributes->path:%s", col->enumCol - COL_Ext1, col->enumCol,col->title, fileAttributes->id, fileAttributes->path);
  g_free(cell);
}

static void load_ExtColumnHashTables_for_all_Dimensions_for_GtkTreeIter(GtkTreeIter* iter_for_file_in_store){
	GValue full_path=G_VALUE_INIT;
	gtk_tree_model_get_value(treemodel, iter_for_file_in_store, COL_FULL_PATH, &full_path);
	GList *dimension_localAddress = NULL;

        GHashTableIter iter_for_DimensionPropertiesHashTable;
        gchar *key;
        DimensionProperties *value;
        g_hash_table_iter_init(&iter_for_DimensionPropertiesHashTable,DimensionPropertiesHashTable_name_asKey);
        while (g_hash_table_iter_next(&iter_for_DimensionPropertiesHashTable, &key, &value)) {
	        RFM_treeviewColumn *col = get_treeviewColumnByTitle(value->name);
                if (col==NULL || !col->Show || col->ValueFunc!=getExtColumnValueFromHashTable) continue; //如果ValueFunc使用getDimensionValueFromHashTable,則不在這裡處理。 這裡col==NULL的判斷是這個commit添加的，其實不需要，因為在 SetViewColumnForDimension 函數中會設置col
	        dimension_localAddress = g_list_prepend(dimension_localAddress,value->localAddress);
		RFM_store_cell* cell=malloc(sizeof(RFM_store_cell));
	        cell->iter = iter_for_file_in_store;
	        cell->dimProperties = value;
		g_spawn_wrapper(findDimensionValueForFile, dimension_localAddress, G_SPAWN_DEFAULT, NULL, g_value_get_string(&full_path), FALSE, load_ExtColumnHashTable_for_one_Dimension_for_GtkTreeIter,cell,TRUE);
                g_list_free(dimension_localAddress);dimension_localAddress = NULL;
        }
        g_value_unset(&full_path);
}

static void load_AllDimensionValues_for_one_DimensionColumn(RFM_ChildAttribs* childAttribs) {
  DimensionProperties *dimProps = (DimensionProperties*)childAttribs->customCallbackUserData;
  RFM_treeviewColumn *col = get_treeviewColumnByTitle(dimProps->name);
  assert(col);//在load_AllDimensionValues_for_DimensionColumns中已經驗證過col非NULL了
  assert(ExtColumnHashTable[col->enumCol - COL_Ext1]);//hasttable的創建在SetViewColumnForDimension 函數中已經完成
  char *child_stdout=(char*)(childAttribs->stdOut);
  if (child_stdout==NULL) return;
  gchar * savedptr=NULL;
  gchar * oneline=strtok_r(child_stdout,"\n",&savedptr);
  while (oneline!=NULL){
    gchar** fullpath_dimValue = g_strsplit(oneline, ":", 2);
    gchar* existingDimValue = g_hash_table_lookup(ExtColumnHashTable[col->enumCol - COL_Ext1],strdup(fullpath_dimValue[0]));
    if (existingDimValue){
      gchar* newDimValue = g_strconcat(existingDimValue, ";", fullpath_dimValue[1], NULL);
      g_hash_table_insert(ExtColumnHashTable[col->enumCol - COL_Ext1],strdup(fullpath_dimValue[0]), newDimValue);
      g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"load_AllDimensionValues_for_one_DimensionColumn, g_hash_table_insert overwrite existingDimValue %s for ExtColumnHashTable[%d], RFM_treeviewCol:%d,%s, fullpath:%s with newDimValue:%s", existingDimValue, col->enumCol - COL_Ext1, col->enumCol,col->title, fullpath_dimValue[0], fullpath_dimValue[1]);
    }else{
      g_hash_table_insert(ExtColumnHashTable[col->enumCol - COL_Ext1],strdup(fullpath_dimValue[0]), strdup(fullpath_dimValue[1]));
      g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"load_AllDimensionValues_for_one_DimensionColumn, g_hash_table_insert called for ExtColumnHashTable[%d], RFM_treeviewCol:%d,%s, fullpath:%s, dimValue:%s", col->enumCol - COL_Ext1, col->enumCol,col->title, fullpath_dimValue[0], fullpath_dimValue[1]);
    }
    g_strfreev(fullpath_dimValue);
    oneline=strtok_r(NULL, "\n",&savedptr);
  }
}

static void load_AllDimensionValues_for_DimensionColumns() {
        GHashTableIter iter_for_DimensionPropertiesHashTable;
        gchar *key;
        DimensionProperties *value;
        g_hash_table_iter_init(&iter_for_DimensionPropertiesHashTable,DimensionPropertiesHashTable_name_asKey);
        while (g_hash_table_iter_next(&iter_for_DimensionPropertiesHashTable, &key, &value)) {
	        RFM_treeviewColumn *col = get_treeviewColumnByTitle(value->name);
                if (col==NULL || !col->Show || col->ValueFunc!=getDimensionValueFromHashTable) continue;
		g_spawn_wrapper(ShowAllValuesForOneDimension, NULL, G_SPAWN_DEFAULT, NULL, value->localAddress, FALSE, load_AllDimensionValues_for_one_DimensionColumn,value,TRUE);
        }
}

static void load_DimensionPropertiesHashTable_Callback(RFM_ChildAttribs* child_attribs) {
  char *child_stdout=(char*)(child_attribs->stdOut);
  gchar * savedptr=NULL;
  gchar * oneline=strtok_r(child_stdout,"\n",&savedptr);
  while (oneline!=NULL){
    GKeyFile *gkeyfile = g_key_file_new();
    GError *error=NULL;
    if (g_key_file_load_from_file(gkeyfile,oneline, G_KEY_FILE_NONE, &error)){
      DimensionProperties * dimProps = calloc(1, sizeof(DimensionProperties));
      dimProps->ID = g_strstrip(g_key_file_get_string(gkeyfile, dumb_keyfile_groupname, RFM_DIMENSION_ID, &error));
      if (error) { g_debug("load_DimensionPropertiesHashTable_Callback. error loading key value pair from file %s, error code: %d, message:%s",oneline, error->code, error->message); g_error_free(error); error=NULL;}
      dimProps->name = g_strstrip(g_key_file_get_string(gkeyfile, dumb_keyfile_groupname, RFM_DIMENSION_NAME, &error));
      if (error) { g_debug("load_DimensionPropertiesHashTable_Callback. error loading key value pair from file %s, error code: %d, message:%s",oneline, error->code, error->message); g_error_free(error); error=NULL;}
      dimProps->localAddress = g_path_get_dirname(oneline);
      if (dimProps->name==NULL || dimProps->name[0]=='\0') dimProps->name = g_path_get_basename(dimProps->localAddress);
      if (strlen(dimProps->name)>ColumnTitleMaxLen){
	g_warning("Dimension name shall be no more than %d chars",ColumnTitleMaxLen);
	dimProps->name[ColumnTitleMaxLen]='\0';
      }
      if (SetViewColumnForDimension(dimProps)){
	g_log(RFM_LOG_DIMENSION,G_LOG_LEVEL_DEBUG,"add dimension name:%s, localAddress:%s, ID:%s, into hashtable", dimProps->name,dimProps->localAddress, dimProps->ID);
	g_hash_table_insert(DimensionPropertiesHashTable_name_asKey, g_strdup(dimProps->name), dimProps);
	g_hash_table_insert(DimensionPropertiesHashTable_localAddress_asKey, g_strdup(dimProps->localAddress), dimProps);
      }
      g_key_file_free(gkeyfile);
    }else{
      g_warning("load_DimensionPropertiesHashTable_Callback. error loading key value pair from file %s, error code: %d, message:%s",oneline, error==NULL?0:error->code, error==NULL?"":error->message);
      g_error_free(error);
    }

    //if (git_root && g_str_has_prefix(fullpath, git_root)){ //维度位于当前git仓库
    // }
    oneline=strtok_r(NULL, "\n",&savedptr);
  }
}

static void cmd_set_dimension_value_for_selected_files(wordexp_t *parsed_msg, GString *readline_result_string_after_file_name_substitution){
  if (parsed_msg->we_wordc == 2){
    gchar** dimNameAndValue = g_strsplit(parsed_msg->we_wordv[1],"=",2);
    set_dimension_value_by_creating_soft_links_to_selected_files(dimNameAndValue);
    g_strfreev(dimNameAndValue);
  }
}

static void set_dimension_value_by_creating_soft_links_to_selected_files(gchar* dimNameAndValue[2]){
    DimensionProperties* dimProps = g_hash_table_lookup(DimensionPropertiesHashTable_name_asKey,dimNameAndValue[0]);
    if (dimProps){
      g_log(RFM_LOG_DIMENSION, G_LOG_LEVEL_DEBUG, "cmd_set_dimension_value_for_selected_files: dim %s, value:%s", dimNameAndValue[0],dimNameAndValue[1]);
      gchar* softlink_dir = g_build_filename(dimProps->localAddress, dimNameAndValue[1],NULL);
      if (filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]!=NULL) {
	GList *filepath_list_for_selection_on_view_clone = g_list_first(g_list_copy_deep(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_strdup,NULL));
	g_spawn_wrapper(create_softlink_for_dimension,filepath_list_for_selection_on_view_clone , G_SPAWN_DEFAULT | G_SPAWN_STDIN_FROM_DEV_NULL, NULL,softlink_dir, FALSE, NULL, NULL, FALSE);
	g_list_free_full(filepath_list_for_selection_on_view_clone,g_free);
      }else g_warning("no files selected");
    }
}

static void refresh_DimensionPropertiesHashTable() {
  if (DimensionPropertiesHashTable_localAddress_asKey) { g_hash_table_destroy(DimensionPropertiesHashTable_localAddress_asKey);}
  if (DimensionPropertiesHashTable_name_asKey) { g_hash_table_destroy(DimensionPropertiesHashTable_name_asKey);}
  DimensionPropertiesHashTable_name_asKey = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)free_DimensionProperties);
  DimensionPropertiesHashTable_localAddress_asKey = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);//正反两个hashtable共享一个DimensionProperties结构，只要free一次就可以了
  g_spawn_wrapper(global_dimension_properties_files_locating_cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, FALSE, load_DimensionPropertiesHashTable_Callback, NULL, TRUE);
}

static void free_DimensionProperties(DimensionProperties *dimProp) {
  free(dimProp->ID); free(dimProp->RemoteAddress); free(dimProp->name); free(dimProp->localAddress);
}
#ifdef RFM_AddOn

// Helper function to calculate MD5 hash of a file
static gchar* calculate_file_hash(const gchar *filepath) {
    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;
    if (!g_file_get_contents(filepath, &contents, &length, &error)) {
        g_warning("Failed to read file %s: %s", filepath, error->message);
        g_error_free(error);
        return NULL;
    }
    gchar *hash_str = g_compute_checksum_for_string(G_CHECKSUM_MD5, contents, length);
    g_free(contents);
    return hash_str;
}

// Compile rfm_local.c into rfm_local.so in the current directory
static gboolean compile_local_extension(RFM_ADDON* addon) {
    gchar *c_file_hash = NULL; // To store the hash of the .c file

    // Calculate hash of the .c file
    c_file_hash = calculate_file_hash(addon->addon_c_fullpath);
    if (!c_file_hash) {
        g_warning("Failed to calculate hash for %s", addon->addon_c_fullpath);
        // No goto cleanup here, just return FALSE after freeing potential resources
        // Note: local_c_path and local_so_path are freed in the caller (load_local_extension)
        return FALSE;
    }
    g_debug("Hash of %s: %s", addon->addon_c_fullpath, c_file_hash);

    // Prepare the hash definition for the compiler command
    gchar *rfm_addon_C_hash_via_gcc = g_strdup_printf("-DRFM_ADDON_C_HASH=\\\"%s\\\"", c_file_hash);
    gchar *rfm_addon_H_hash_via_gcc = g_strdup_printf("-DRFM_ADDON_H_HASH=\\\"%s\\\"", RFM_AddOn);
    g_free(c_file_hash); // Free the calculated hash string

    gchar* DGitIntegration = "";
#ifdef GitIntegration
    DGitIntegration = "-DGitIntegration";
#endif
    gchar* DCmarkTools = "";
#ifdef CmarkTools
    DCmarkTools = "-DCmarkTools";
#endif

    gchar* rfm_local_c_compile_cmd = g_strdup_printf(rfm_local_c_compile_cmd_template, RFM_AddOn_H_filepath, DCmarkTools, DGitIntegration, rfmLANGUAGE, RFM_SearchResultTypeNamePrefix, RFM_NEW_TERM_SUFFIX, RFM_selected_filename_placeholder, RFM_SEARCH_RESULT_AS_INPUT_PREFIX, addon->addon_so_fullpath, rfm_addon_C_hash_via_gcc, addon->addon_c_fullpath, rfm_addon_H_hash_via_gcc);
    gboolean success = g_spawn_wrapper_wordexp(rfm_local_c_compile_cmd, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, FALSE, NULL, NULL, FALSE);
    g_free(rfm_local_c_compile_cmd); g_free(rfm_addon_C_hash_via_gcc); g_free(rfm_addon_H_hash_via_gcc);
    return success;
}

// treeviewColumns相关初始化流程：
// 1. treeviewColumns_init 到 treeviewColumns 主要是展开 C1,C2,...列，节省config.h空间，config.h里面只包含C1列作为模板，不包括C2,C3,... 。这一步只在应用初始化时做,后续操作就不再访问 treeviewColumns_init, 只用treeviewColumns作为模板
// 2. treeviewColumns 复制到 SearchResultViewColumnsLayout，每个新查询开始运行时做
// 3. treeviewColumns 复制到 DirectoryViewColumnsLayout，和Search不同，进入新目录时无需重新复制模板
// 4. 每次AddOn激活时，使用 append_treeviewColumns 把addon上定义的列复制到 Search和DirectoryViewColumnsLayout里面，需要重新计算array_size
// 5. unload AddOn 的时候就要通过本方法去除上一步添加的Column,实际是通过重做2,3,4步实现
static void remove_addon_treeviewColumns(RFM_ADDON *addon){
  if (addon->resource_managed_by_addon->treeviewColumns_array_size>0){
    for(size_t i=0; i<addon->resource_managed_by_addon->treeviewColumns_array_size; i++){
      RFM_treeviewColumn *col = get_treeviewColumnByTitle(addon->resource_managed_by_addon->treeviewColumns[i].title);
      //TODO:在load_addon的时候检查title重复冲突
      if (col) col->enumCol = NUM_COLS;//先置空所有 enumCol TODO: 如果控件不为空,要释放控件吗?还是refresh_store的时候会自动处理?
      else g_warning("treeview column title %s in addon %s not found",addon->resource_managed_by_addon->treeviewColumns[i].title, addon->addon_c_fullpath);
    }
    gchar *column_layout_without_addon = get_current_column_layout(TRUE);//获取除了此addon外的列layout

    free(DirectoryViewColumnsLayout); free(SearchResultViewColumnsLayout);
    //重新初始化 search 和dir viewcolumnslayouts,TODO:gtk控件要处理吗?
    TREEVIEW_COLUMNS_array_size = G_N_ELEMENTS(treeviewColumns);
    SearchResultViewColumnsLayout = malloc(sizeof(RFM_treeviewColumn)*TREEVIEW_COLUMNS_array_size);
    memcpy(SearchResultViewColumnsLayout, treeviewColumns, sizeof(RFM_treeviewColumn)*TREEVIEW_COLUMNS_array_size);
    DirectoryViewColumnsLayout = malloc(sizeof(RFM_treeviewColumn)*TREEVIEW_COLUMNS_array_size);
    memcpy(DirectoryViewColumnsLayout, treeviewColumns, sizeof(RFM_treeviewColumn)*TREEVIEW_COLUMNS_array_size);
    
    GHashTableIter ht_iter;
    gchar *key;
    RFM_ADDON *value;
    g_hash_table_iter_init(&ht_iter, addons);
    while (g_hash_table_iter_next(&ht_iter, &key, &value)){
      if (value!=addon) append_addon_treeviewColumns(value,TRUE,TRUE);
    }

    show_hide_treeview_columns_in_order(column_layout_without_addon);//重新应用前面保存的layout
    g_free(column_layout_without_addon);
  }
}

//不同于addon中filemen,toolbarbutton等,addon中的treeviewColumns定义在使用前需要append到全局的treeviewColumns数组里,这主要是因为显示隐藏列,调整顺序等操作都是基于一个数组定义的
static void append_addon_treeviewColumns(RFM_ADDON *addon, gboolean searchresultview, gboolean dirview) {
  if (addon->resource_managed_by_addon->treeviewColumns_array_size>0){
    RFM_treeviewColumn *old_DirectoryViewColumnLayout = DirectoryViewColumnsLayout;
    RFM_treeviewColumn *old_SearchResultViewColumnsLayout = SearchResultViewColumnsLayout;
    size_t old_TREEVIEW_COLUMNS_array_size = TREEVIEW_COLUMNS_array_size;
    TREEVIEW_COLUMNS_array_size += addon->resource_managed_by_addon->treeviewColumns_array_size;
    if (dirview){
      DirectoryViewColumnsLayout = malloc(TREEVIEW_COLUMNS_array_size*sizeof(RFM_treeviewColumn));
      memcpy((void*)DirectoryViewColumnsLayout, old_DirectoryViewColumnLayout, sizeof(RFM_treeviewColumn)*old_TREEVIEW_COLUMNS_array_size);
      memcpy((void*)DirectoryViewColumnsLayout + sizeof(RFM_treeviewColumn)*old_TREEVIEW_COLUMNS_array_size, addon->resource_managed_by_addon->treeviewColumns, sizeof(RFM_treeviewColumn)*addon->resource_managed_by_addon->treeviewColumns_array_size);
      free(old_DirectoryViewColumnLayout);
    }
    if (searchresultview){
      SearchResultViewColumnsLayout = malloc(TREEVIEW_COLUMNS_array_size*sizeof(RFM_treeviewColumn));
      memcpy((void*)SearchResultViewColumnsLayout, old_SearchResultViewColumnsLayout, sizeof(RFM_treeviewColumn)*old_TREEVIEW_COLUMNS_array_size);
      memcpy((void*)SearchResultViewColumnsLayout + sizeof(RFM_treeviewColumn)*old_TREEVIEW_COLUMNS_array_size, addon->resource_managed_by_addon->treeviewColumns, sizeof(RFM_treeviewColumn)*addon->resource_managed_by_addon->treeviewColumns_array_size);
      free(old_SearchResultViewColumnsLayout);
    }
  }
}

static void load_addon_dir(char *addon_dir) {
  GHashTable *addons_in_dir = g_hash_table_lookup(all_addon_definitions_by_dir, addon_dir);
  if (addons_in_dir){
     GHashTableIter ht_iter;
     gchar *key;
     gchar *value;
     g_hash_table_iter_init(&ht_iter, addons_in_dir);
     while (g_hash_table_iter_next(&ht_iter, &key, &value)){
       if (!g_hash_table_lookup(addons, key)) load_addon(key);
     }
  }
}

static void load_addon(CALLER_OWN char* addon_fullpath){
    assert(addon_fullpath[0]=='/');//检查是绝对路径
    char *dlsym_error = NULL; //The message returned by dlerror() may reside in a statically allocated buffer that is overwritten by subsequent dlerror()calls. https://www.man7.org/linux/man-pages/man3/dlerror.3.html
    g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "load_addon(%s)",addon_fullpath);
    RFM_ADDON * addon = calloc(1, sizeof(RFM_ADDON));
    addon->addon_c_fullpath = g_strdup(addon_fullpath);
    addon->addon_hash = calculate_file_hash(addon->addon_c_fullpath);
    gchar* addon_dir = g_path_get_dirname(addon_fullpath);
    // 生成.so文件名：将.c替换为.so
    addon->addon_base_filename = g_path_get_basename(addon_fullpath);
    addon->addon_so_fullpath = g_strdup_printf("%s/%.*s.so",addon_dir, (int)(strlen(addon->addon_base_filename)-2), addon->addon_base_filename); // 去掉.c，加上.so
    gboolean compile_needed = FALSE;
    if (g_file_test(addon->addon_c_fullpath, G_FILE_TEST_EXISTS)) {
       if (g_hash_table_lookup(addons, addon->addon_c_fullpath)){
	   g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Addon %s already loaded, no need to load again", addon->addon_c_fullpath);
	   goto cleanup;
       }
       //上面的判断是防止同一个addon被重复加载，所以loglevel info足够了; 而下面的判断是防止同名文件（同一个addon的不同版本出现在不同目录），和不同名但同hash（同一个addon不同名出现在不同目录）,这是需要用户手工干预的，比如改文件名，所以用warning。注意这里考虑的主要是global类型的addon。 Directory类型的同一个addon自然可以出现在不同目录中，但不会同时加载的
       //TODO: 如果是DirectoryTree类型的Addon,命名冲突如何？这等今后DirectoryTree类型用起来再说。目前只有global类型的AI addon我常用，AI 出现以后，rfm TODO addon都不怎么用了。
       if (g_strcmp0(addon->addon_base_filename, RFM_ADDON_FILENAME_SUFFIX)!=0){ //之前addon统一叫rfm_local.c, 所以这种情况要排除
         RFM_ADDON *addon_with_the_same_name = g_hash_table_lookup(addons_filename, addon->addon_base_filename);
         if (addon_with_the_same_name) {
           g_warning("Addon %s with the same name already loaded, no need to load %s", addon_with_the_same_name->addon_c_fullpath, addon->addon_c_fullpath);
           goto cleanup;
         }
       }
       if (!g_file_test(addon->addon_so_fullpath, G_FILE_TEST_EXISTS)) {
           g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "%s not found, compilation needed.", addon->addon_so_fullpath);
           compile_needed = TRUE;
       } else {
           // Compare hashes instead of timestamps
	   RFM_ADDON* addon_with_the_same_hash = g_hash_table_lookup(addons_hash, addon->addon_hash);
	   if (addon_with_the_same_hash){
	     g_warning("Addon %s with the same hash already loaded , no need to load %s", addon_with_the_same_hash->addon_c_fullpath, addon->addon_c_fullpath);
	     goto cleanup;
	   }
           gchar *addon_c_hash_in_SO = NULL;
	   gchar *addon_h_hash_in_SO = NULL;
           void *temp_handle = dlopen(addon->addon_so_fullpath, RTLD_LAZY | RTLD_LOCAL);

           if (temp_handle) {
               // Clear any previous error before dlsym
               dlerror();
               // Use void* for dlsym, then cast if needed
               void *symbol_ptr = dlsym(temp_handle, "rfm_addon_c_file_hash");
	       void *symbol_ptr1 = dlsym(temp_handle, "rfm_addon_h_file_hash");
               dlsym_error = dlerror(); // Check error *after* calling dlsym

               if (!dlsym_error && symbol_ptr && symbol_ptr1) {
                   // Cast to the correct type ONLY if symbol was found
                   const char * const * hash_ptr = (const char * const *)symbol_ptr;
		   const char * const * hash_ptr1 = (const char * const *)symbol_ptr1;
                   if (hash_ptr && *hash_ptr && hash_ptr1 && *hash_ptr1) { // Check if the pointer itself and the string it points to are valid
                        addon_c_hash_in_SO = g_strdup(*hash_ptr);
		        addon_h_hash_in_SO = g_strdup(*hash_ptr1);
                        g_log(RFM_LOG_ADDON, G_LOG_LEVEL_DEBUG, "Found hash in SO, rfm_addon_c_file_hash:%s, rfm_addon_h_file_hash:%s", addon_c_hash_in_SO, addon_h_hash_in_SO);
                   } else {
                        g_warning("Symbol 'rfm_addon_c_file_hash' or 'rfm_addon_h_file_hash' found in %s but points to NULL. Recompiling.", addon->addon_so_fullpath);
                        compile_needed = TRUE; // Force recompile if symbol is NULL
                   }
               } else {
                   g_warning("Could not find symbol 'rfm_addon_c_file_hash' or 'rfm_addon_h_file_hash' in %s: %s. Recompiling.", addon->addon_so_fullpath, dlsym_error ? dlsym_error : "(symbol not found)");
                    compile_needed = TRUE; // Force recompile if symbol not found
               }
               dlclose(temp_handle);
           } else {
               g_warning("Could not dlopen %s to check hash: %s. Recompiling.", addon->addon_so_fullpath, dlerror());
               compile_needed = TRUE; // Force recompile if .so can't be opened
           }

           if (!compile_needed && addon->addon_hash && addon_c_hash_in_SO && addon_h_hash_in_SO) {
               if (g_strcmp0(addon->addon_hash, addon_c_hash_in_SO) != 0) {
                   g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Hash mismatch for %s. Recompilation needed. (C: %s, SO: %s)", addon->addon_c_fullpath, addon->addon_hash, addon_c_hash_in_SO);
                    compile_needed = TRUE;
               }else if (g_strcmp0(RFM_AddOn, addon_h_hash_in_SO) != 0) {
		    g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "rfm_addon.h hash mismatch for %s. Recompilation needed. (H: %s, SO: %s)", addon->addon_c_fullpath, RFM_AddOn, addon_h_hash_in_SO);
                    compile_needed = TRUE;
	       }else {
                    g_log(RFM_LOG_ADDON, G_LOG_LEVEL_DEBUG, "Hashes match for %s. No recompilation needed.", addon->addon_c_fullpath);
               }
           } else if (!compile_needed && !addon->addon_hash) {
                g_warning("Failed to calculate hash for current %s. Recompiling.", addon->addon_c_fullpath);
                compile_needed = TRUE; // Force recompile if current hash fails
           }
           // compile_needed might have been set true inside the dlopen/dlsym block

           g_free(addon_c_hash_in_SO);
       }

        if (compile_needed) {
            if (!compile_local_extension(addon)) {
                g_warning("Compilation failed, cannot load local extension.");
                goto cleanup;
            }
        }

        // Load the .so file
        addon->addon_so_handle = dlopen(addon->addon_so_fullpath, RTLD_LAZY | RTLD_LOCAL);
        if (!addon->addon_so_handle) {
            g_warning("dlopen failed for %s: %s", addon->addon_so_fullpath, dlerror());
            goto cleanup;
        }

        //查找 addon 资源
        addon->resource_managed_by_addon = dlsym(addon->addon_so_handle, RFM_ADDON_RESOURCE_STRUCT);
        dlsym_error = dlerror();
        if (dlsym_error) {
            g_warning("dlsym failed for %s in %s: %s", RFM_ADDON_RESOURCE_STRUCT, addon->addon_so_fullpath, dlsym_error);
            goto cleanup;
        }

        //查找 addon 类型
        enum RFM_ADDON_TYPE *addon_type_variable = dlsym(addon->addon_so_handle, RFM_ADDON_TYPE_VAR_NAME);
        dlsym_error = dlerror();
        if (dlsym_error) {
            g_warning("dlsym failed for %s in %s: %s", RFM_ADDON_TYPE_VAR_NAME, addon->addon_so_fullpath, dlsym_error);
            goto cleanup;
        }else addon->addon_type = *addon_type_variable;

	if (addon->addon_type == Directory && strcmp(rfm_curPath, addon_dir)!=0) goto cleanup;

	addon->addon_leave_func = dlsym(addon->addon_so_handle, RFM_ADDON_LEAVE_FUNC);
        dlsym_error = dlerror();
        if (dlsym_error) {
	    g_warning("dlsym failed for %s in %s: %s", RFM_ADDON_LEAVE_FUNC, addon->addon_so_fullpath, dlsym_error);
	    goto cleanup;
	}

        // Find and call enter function
        addon->addon_enter_func = dlsym(addon->addon_so_handle, RFM_ADDON_ENTER_FUNC);
        dlsym_error = dlerror();
        if (dlsym_error) {
            g_warning("dlsym failed for %s in %s: %s", RFM_ADDON_ENTER_FUNC, addon->addon_so_fullpath, dlsym_error);
            goto cleanup;
        } else if (addon->addon_enter_func){
            g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Calling %s from %s", RFM_ADDON_ENTER_FUNC, addon->addon_so_fullpath);
            (*(addon->addon_enter_func))(addon_dir); //call enter function
	    g_hash_table_insert(addons, strdup(addon->addon_c_fullpath), addon);
	    g_hash_table_insert(addons_hash, addon->addon_hash, addon);
	    g_hash_table_insert(addons_filename, addon->addon_base_filename, addon);
	    append_addon_treeviewColumns(addon,TRUE,TRUE);
	    g_debug("addons size after loading:%d",g_hash_table_size(addons));
	    g_free(addon_dir);
	    return; // 成功call enter funtion 就不要cleanup, unload 完再cleanup
        } else {
             g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Function %s not found in %s (optional).", RFM_ADDON_ENTER_FUNC, addon->addon_so_fullpath);
        }
    } else {
         g_log(RFM_LOG_ADDON, G_LOG_LEVEL_DEBUG, "No local extension file %s found.", addon->addon_c_fullpath);
    }
 cleanup:
    if (addon->addon_so_handle) {
        if (dlclose(addon->addon_so_handle) != 0) g_warning("dlclose failed: %s", dlerror());
        addon->addon_so_handle = NULL;
    }
    free_addon(addon);
    g_free(addon_dir);
}

static void locate_all_addons_callback(RFM_ChildAttribs* child_attribs) {
  char *child_stdout=(char*)(child_attribs->stdOut);
  g_debug("locate_all_addons_callback with child_attribs:%p",child_attribs);
  if (!child_stdout) return;
  g_hash_table_remove_all(all_addon_definitions_by_dir);
  gchar * savedptr=NULL;
  gchar * oneline=strtok_r(child_stdout,"\n",&savedptr);
  while (oneline!=NULL){
      g_debug("locate_all_addons_callback processing oneline:%s", oneline);
      gchar** addon_fullpath_and_type = g_strsplit(oneline, ":", 2); // locate_all_addons 返回冒号分割的行，第一个冒号前为fullpath, 冒号后为const enum RFM_ADDON_TYPE addon_type = DirectoryTree;这样的字符串
      if (g_strv_length(addon_fullpath_and_type)==2){
	gchar * fullpath_dir = g_path_get_dirname(addon_fullpath_and_type[0]);
	GHashTable* addons_in_dir = g_hash_table_lookup(all_addon_definitions_by_dir, fullpath_dir);
	if (!addons_in_dir) {
	  addons_in_dir = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	  g_hash_table_insert(all_addon_definitions_by_dir, g_strdup(fullpath_dir), addons_in_dir);
	}
	g_hash_table_insert(addons_in_dir, g_strdup(addon_fullpath_and_type[0]), g_strdup(addon_fullpath_and_type[1]));
	//加载Global addon
	if (strstr(addon_fullpath_and_type[1], "Global") && !g_hash_table_lookup(addons, addon_fullpath_and_type[0])) load_addon(addon_fullpath_and_type[0]);
	g_strfreev(addon_fullpath_and_type);
	g_free(fullpath_dir);
      }else g_warning("locate_all_addons return line invalid:%s",oneline);
      oneline=strtok_r(NULL, "\n",&savedptr);
  }
}

static void find_and_unload_addons_before_leaving_current_directory(CALLER_OWN const gchar* path) {
    GHashTableIter ht_iter;
    gchar *key;
    RFM_ADDON *value;
    g_hash_table_iter_init(&ht_iter, addons);
    while (g_hash_table_iter_next(&ht_iter, &key, &value)){
      gchar * addon_dir = g_path_get_dirname(value->addon_c_fullpath);
      if (value->addon_type==Directory || (value->addon_type==DirectoryTree && !g_str_has_prefix(path, addon_dir))) {
	g_hash_table_iter_remove(&ht_iter);
	unload_local_extension(value);
	free_addon(value);
        g_debug("addons size after unloading:%d",g_hash_table_size(addons));
      }
      g_free(addon_dir);
    }
}

static void unload_local_extension(RFM_ADDON *addon) {
  if (addon->addon_so_handle) {
        g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Unloading local extension. %s", addon->addon_so_fullpath);
        if (addon->addon_leave_func) {
            g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Calling %s from %s", RFM_ADDON_LEAVE_FUNC, addon->addon_so_fullpath);
            (*(addon->addon_leave_func))(); //call leave function
        } else {
             g_log(RFM_LOG_ADDON, G_LOG_LEVEL_INFO, "Function %s not found in %s (optional).", RFM_ADDON_LEAVE_FUNC, addon->addon_so_fullpath);
        }
        remove_addon_treeviewColumns(addon);
        if (dlclose(addon->addon_so_handle) != 0) {
             g_warning("dlclose failed: %s", dlerror());
        }
    }
}

static void free_addon(RFM_ADDON *addon){
	g_free(addon->addon_c_fullpath);
	g_free(addon->addon_so_fullpath);
        g_free(addon->addon_hash);
        g_free(addon->addon_base_filename);
        free(addon);
}

static NO_FREE char *AddOnStatus(RFM_FileAttributes* fileAttribs, guint ExtColumnHashTableIndex) {
  if (g_hash_table_lookup(addons, fileAttribs->path)) return "loaded";
  else return "not loaded";
}

static void unload_addon_filemenu_func() {
  GList* selection_GtkTreePath_list = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  GtkTreeIter iter;
  GList *listElement = g_list_first(selection_GtkTreePath_list);
  while(listElement!=NULL) {
	 gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data); //GtkTreePath to GtkTreeIter
         RFM_FileAttributes *fileAttributes=NULL;
	 gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
	 RFM_ADDON *addon = g_hash_table_lookup(addons, fileAttributes->path);
         if (addon) {
	   g_hash_table_remove(addons, fileAttributes->path);
	   g_hash_table_remove(addons_hash, addon->addon_hash);
	   g_hash_table_remove(addons_filename, addon->addon_base_filename);
	   unload_local_extension(addon);
	   free_addon(addon);
	 }
         listElement=g_list_next(listElement);
  }
  g_list_free_full(selection_GtkTreePath_list, (GDestroyNotify)gtk_tree_path_free);
};
#endif
