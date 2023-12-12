/* RFM - Rod's File Manager
 *
 * See LICENSE file for copyright and license details.
 *
 * Edit config.h to define run actions and set options
 *
 * Compile with: gcc -Wall -g rfm.c `pkg-config --libs --cflags gtk+-3.0` -o rfm
 *            OR use the supplied Makefile.
 */

#include "gdk/gdkkeysyms.h"
#include <linux/limits.h>
#define _GNU_SOURCE

#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <gio/gunixmounts.h>
#include <glib-unix.h>
#include <mntent.h>
#include <icons.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h>

#ifdef PythonEmbedded
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#endif

#define INOTIFY_MASK IN_MOVE|IN_CREATE|IN_CLOSE_WRITE|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF
#define PIPE_SZ 65535      /* Kernel pipe size */

#define   RFM_EXEC_NONE     G_SPAWN_STDOUT_TO_DEV_NULL
#define   RFM_EXEC_STDOUT   G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_CHILD_INHERITS_STDOUT | G_SPAWN_CHILD_INHERITS_STDERR
#define   RFM_EXEC_OUPUT_READ_BY_PROGRAM G_SPAWN_DEFAULT
#define   RFM_EXEC_FILE_CHOOSER G_SPAWN_STDIN_FROM_DEV_NULL
#define   RFM_EXEC_MOUNT   G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_CHILD_INHERITS_STDOUT | G_SPAWN_CHILD_INHERITS_STDERR  //TODO: i don't know what this mean yet.

typedef struct {
   gchar *path;
   gchar *thumb_name;
   gchar *md5;
   gchar *uri;
   gint64 mtime_file;
   gint t_idx;
   pid_t rfm_pid;
   gint thumb_size; 
} RFM_ThumbQueueData;

typedef struct {
   gchar *thumbRoot;
   gchar *thumbSub;
  //GdkPixbuf *(*func)(RFM_ThumbQueueData * thumbData);
   const gchar **thumbCmd;
} RFM_Thumbnailer;


typedef struct {
   gchar *runName;
   gchar *runRoot;
   gchar *runSub;
   const gchar **runCmd;
  //gint  runOpts;
   gboolean (*showCondition)();
} RFM_MenuItem;

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
   gchar *name;
   const gchar **RunCmd;
   GSpawnFlags  runOpts;
   GPid  pid;
   gint  stdOut_fd;
   gint  stdErr_fd;
   char *stdOut;
   char *stdErr;
   int   status;
   void (*customCallBackFunc)(gpointer); //In Searchresultviewinsteadofdirectoryview situation, after runAction such as Move, i need to fill_store to reflect remove of files, so, i need a callback function. Rodney's original code only deals with working directory, and use INotify to reflect the change.
   gpointer customCallbackUserData; //this is not freed in free_child_attribs, user should free it.
   gboolean spawn_async;
   gint exitcode;
} RFM_ChildAttribs;

typedef struct {
   GtkWidget *menu;
   GtkWidget **action;
} RFM_fileMenu;

typedef struct {
  GtkWidget *toolbar;
  GtkWidget **buttons;
} RFM_toolbar;

typedef struct {
  gchar *cmd;
  void (*action)();
  gchar *help_msg;
} RFM_builtinCMD;

//I don't understand why Rodney need this ctx type. it's only instantiated in main, so, all members can be changed into global variable, and many function parameter can be removed. However, if there would be any important usage, adding the removed function parameters will be time taking. So, just keep as is, although it makes current code confusing.
typedef struct {
   gint        rfm_sortColumn;   /* The column in the tree model to sort on */
   GUnixMountMonitor *rfm_mountMonitor;   /* Reference for monitor mount events */
   gint        showMimeType;              /* Display detected mime type on stdout when a file is right-clicked: toggled via -i option */
   guint       delayedRefresh_GSourceID;  /* Main loop source ID for refresh_store() delayed refresh timer */
} RFM_ctx;

typedef struct {  /* Update free_fileAttributes() and malloc_fileAttributes() if new items are added */
   gchar *path;
   gchar *file_name;
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

   gchar * mtime;
   gchar * atime;
   gchar * ctime;

} RFM_FileAttributes;

typedef struct {
   GdkPixbuf *file, *dir;
   GdkPixbuf *symlinkDir;
   GdkPixbuf *symlinkFile;
   GdkPixbuf *unmounted;
   GdkPixbuf *mounted;
   GdkPixbuf *symlink;
   GdkPixbuf *broken;
} RFM_defaultPixbufs;

enum RFM_treeviewCol{
   COL_ICONVIEW_MARKUP,
   COL_ICONVIEW_TOOLTIP,
   COL_PIXBUF,
   COL_MODE_STR,
   //COL_DISPLAY_NAME,
   COL_FILENAME,
   COL_FULL_PATH,
   COL_MTIME,
   COL_MTIME_STR,
   COL_SIZE,
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
#endif
   COL_Ext1,
   COL_Ext2,
   COL_Ext3,
   COL_Ext4,
   COL_Ext5,
   COL_Ext6,
   COL_Ext7,
   COL_Ext8,
   COL_Ext9,
   COL_Ext10,
   NUM_COLS
};

typedef struct {
  gchar* title;
  enum RFM_treeviewCol enumCol;
  gboolean Show;
  GtkTreeViewColumn* gtkCol;
  gboolean (*showCondition)(RFM_FileAttributes * fileAttributes);
  enum RFM_treeviewCol enumSortCol;
  gchar* ValueCmd;
  gchar* (*ValueFunc)(gchar*);
  gchar* MIME_root;
  gchar* MIME_sub;
  gboolean iconview_markup;
  gboolean iconview_tooltip;
} RFM_treeviewColumn;


//TODO: keep GtkTreeIter somewhere such as in fileAttribute, so that we can try load gitmsg and extcolumns with spawn async instead of sync. However, that can be complicated, what if we have spawned so many processes and user clicked refresh? we have to build Stop mechanism into it. Simple strategy is not to load this slow columns unless user configures to show them.
typedef struct {
  GtkTreeIter * iter;
  gint store_column;
  gboolean iconview_markup;
  gboolean iconview_tooltip;
} RFM_store_cell;

typedef struct {
  gchar* name;
  gchar* activationKey;
  gchar* prompt;
  gchar** (*cmdTransformer)(gchar *);
} stdin_cmd_interpretor;

enum rfmTerminal{
  NO_TERMINAL,
  NEW_TERMINAL,
  INHERIT_TERMINAL,
};


static gchar*  PROG_NAME = NULL;

static gboolean StartedAs_rfmFileChooser = FALSE;
static int rfmFileChooserResultNumber = 0;
static gchar *rfmFileChooserReturnSelectionIntoFilename = NULL;

// I need a method to show in stdin prompt whether there are selected files in
// gtk view. if there are, *> is prompted, otherwise, just prompt >
static gint ItemSelected = 0;
// since it's may be complicated if possible to update stdin prompt whenever the terminal window get focus, i just show ItemSelected prompt in new prompt, and user press two times to refresh gtk view.So, I need a way to recognize consecutive enter press.
static time_t lastEnter;
static gboolean In_refresh_store=FALSE;

static GtkWidget *window=NULL;      /* Main window */
static GtkWidget *rfm_main_box;
static GtkWidget *scroll_window = NULL;
static GtkWidget *icon_or_tree_view = NULL;
static GtkWidget * PathAndRepositoryNameDisplay;
static RFM_ctx *rfmCtx=NULL;
static gchar *rfm_homePath;         /* Users home dir */
static gchar *rfm_thumbDir;         /* Users thumbnail directory */
static gint rfm_do_thumbs;          /* Show thumbnail images of files: 0: disabled; 1: enabled; 2: disabled for current dir */
static GList *rfm_fileAttributeList=NULL; /* Store holds references to this list: use clear_store() to free */
static GList *rfm_thumbQueue=NULL;
static GList *rfm_childList=NULL;

static guint rfm_readDirSheduler=0;
static guint rfm_thumbScheduler = 0;
//TODO: i added the following two schedulers so that i can put off the loading of these slow columns after file list appears in the view first. But have not implemented them yet. Anyway, if user choose to show this slow columns, they have to wait to the end, show files slowly one by one may be better.
static guint rfm_extColumnScheduler = 0;
#ifdef GitIntegration
static guint rfm_gitCommitMsgScheduler = 0;
#endif

static guint stdin_command_Scheduler=0;
static GThread * readlineThread=NULL;

static int rfm_inotify_fd;
static int rfm_curPath_wd;    /* Current path (rfm_curPath) watch */
static int rfm_thumbnail_wd;  /* Thumbnail watch */

static char *initDir=NULL;
static gchar *rfm_curPath=NULL;  /* The current directory */
static gchar *rfm_SearchResultPath=NULL; /*keep the rfm_curPath value when SearchResult was created */
static gchar *rfm_prePath=NULL;  /* Previous directory: only set when up button is pressed, otherwise should be NULL */
static char cwd[PATH_MAX];

static GtkAccelGroup *agMain = NULL;
static RFM_toolbar *tool_bar = NULL;
static RFM_defaultPixbufs *defaultPixbufs=NULL;

static GtkIconTheme *icon_theme;

static GHashTable *thumb_hash=NULL; /* Thumbnails in the current view */

static GtkListStore *store=NULL;
static GtkTreeModel *treemodel=NULL;

static gboolean treeview=FALSE;
static gchar* treeviewcolumn_init_order_sequence = NULL;
static gchar* auto_execution_command_after_rfm_start = NULL;
// keep previous selection when go back from cd directory to search result.
// two elements, one for search result view, the other for directory view
// filepath string in this list is created with strdup.
static GList * filepath_lists_for_selection_on_view[2] = {NULL,NULL};
static GList * filepath_lists_for_selection_on_view_clone;
// if true, means that rfm read file names in following way:
//      ls|xargs realpath|rfm
// or
//      locate blablablaa |rfm
// , instead of from a directory
static unsigned int SearchResultViewInsteadOfDirectoryView=0;
static GList *SearchResultFileNameList = NULL;
static gint fileNum=0;
static gint currentFileNum=0;
static char* pipefd="0";
static GList *CurrentPage_SearchResultView=NULL;
static gint PageSize_SearchResultView=20;
/*keep the original user inputs for add_history*/
static gchar *OriginalReadlineResult=NULL;
static guint history_entry_added=0;
static char *rfm_historyFileLocation;
static char** (*OLD_rl_attempted_completion_function)(const char *text, int start, int end);
static char **rfm_filename_completion(const char *text, int start, int end);
static char *rfm_selection_completion = NULL;
static GMutex rfm_selection_completion_lock;
//used by exec_stdin_command and exec_stdin_command_builtin to share status
static gboolean stdin_cmd_ending_space=FALSE;
static GList * stdin_cmd_selection_list=NULL; //selected files used in stdin cmd expansion(or we call it substitution) which replace ending space and %s with selected file names
static RFM_FileAttributes *stdin_cmd_selection_fileAttributes;
static uint current_stdin_cmd_interpretor = 0;
static enum rfmTerminal rfmStartWithVirtualTerminal = INHERIT_TERMINAL;

#ifdef GitIntegration
// value " M " for modified
// value "M " for staged
// value "MM" for both modified and staged
// value "??" for untracked
// the same as git status --porcelain
static GHashTable *gitTrackedFiles;
static gboolean curPath_is_git_repo = FALSE;
static gboolean cur_path_is_git_repo(RFM_FileAttributes * fileAttributes) { return curPath_is_git_repo; }
static void set_window_title_with_git_branch_and_sort_view_with_git_status(gpointer *child_attribs);
static void set_terminal_window_title(char * title);
#endif

void move_array_item_a_after_b(void * array, int index_b, int index_a, uint32_t array_item_size, uint32_t array_length);
static gboolean startWithVT();
static void show_msgbox(gchar *msg, gchar *title, gint type);
static void die(const char *errstr, ...);
static RFM_defaultPixbufs *load_default_pixbufs(void);
static void set_rfm_curPath(gchar *path);
static int setup(RFM_ctx *rfmCtx);
static void ReadFromPipeStdinIfAny(char *fd);
static void update_SearchResultFileNameList_and_refresh_store(gpointer filenamelist);
// read input from parent process stdin , and handle input such as
// cd .
// cd /tmp
static RFM_treeviewColumn* GetColumnByEnun(enum RFM_treeviewCol col);
static void exec_stdin_command(gchar *msg);
static gboolean exec_stdin_command_builtin(wordexp_t * parsed_msg, GString* readline_result_string);
static void stdin_command_help();
static void readlineInSeperateThread();
static gboolean inotify_handler(gint fd, GIOCondition condition, gpointer rfmCtx);
static void inotify_insert_item(gchar *name, gboolean is_dir);
static gboolean delayed_refreshAll(gpointer user_data);
static void refresh_store(RFM_ctx *rfmCtx);
static void clear_store(void);
static void rfm_stop_all(RFM_ctx *rfmCtx);
static gboolean fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store();
static gboolean read_one_DirItem_into_fileAttributeList_and_insert_into_store_in_each_call(GDir *dir);
static void Iterate_through_fileAttribute_list_to_insert_into_store();
static void Insert_fileAttributes_into_store(RFM_FileAttributes *fileAttributes,GtkTreeIter *iter);
static void Insert_fileAttributes_into_store_with_thumbnail_and_more(RFM_FileAttributes* fileAttributes);
static RFM_FileAttributes *malloc_fileAttributes(void);
static RFM_FileAttributes *get_fileAttributes_for_a_file(const gchar *name, guint64 mtimeThreshold, GHashTable *mount_hash);
static GHashTable *get_mount_points(void);
static gboolean mounts_handler(GUnixMountMonitor *monitor, gpointer rfmCtx);
#ifdef GitIntegration
static void readGitCommitMsgFromGitLogCmdAndUpdateStore(RFM_ChildAttribs * childAttribs);
static void load_GitTrackedFiles_into_HashTable();
static void load_gitCommitMsg_for_store_row(GtkTreeIter *iter);
#endif
static void iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration(void);
static void load_thumbnail_or_enqueue_thumbQueue_for_store_row(GtkTreeIter *iter);
static RFM_ThumbQueueData *get_thumbData(GtkTreeIter *iter);
static gint find_thumbnailer(gchar *mime_root, gchar *mime_sub_type);
static int load_thumbnail(gchar *key);
static void rfm_saveThumbnail(GdkPixbuf *thumb, RFM_ThumbQueueData *thumbData);
static gboolean mkThumb();

static GtkWidget *add_view(RFM_ctx *rfmCtx);
static void add_toolbar(GtkWidget *rfm_main_box, RFM_defaultPixbufs *defaultPixbufs, RFM_ctx *rfmCtx);
static void refresh_toolbar();
static gboolean view_key_press(GtkWidget *widget, GdkEvent *event,RFM_ctx *rfmCtx);
static gboolean view_button_press(GtkWidget *widget, GdkEvent *event,RFM_ctx *rfmCtx);
static void item_activated(GtkWidget *icon_view, GtkTreePath *tree_path, gpointer user_data);
static void row_activated(GtkTreeView *tree_view, GtkTreePath *tree_path,GtkTreeViewColumn *col, gpointer user_data);
static GList* get_view_selection_list(GtkWidget * view, gboolean treeview, GtkTreeModel ** model);
static void set_view_selection(GtkWidget* view, gboolean treeview, GtkTreePath* treePath);
static void set_view_selection_list(GtkWidget *view, gboolean treeview,GList *selectionList);
static gboolean path_is_selected(GtkWidget *widget, gboolean treeview, GtkTreePath *path);

/* callback function for tool_buttons. Its possible to make function such as home_clicked as callback directly, instead of use exec_user_tool as a wrapper. However,i would add GtkToolItems as first parameter for so many different callback functions then. */
/* since g_spawn_wrapper will free child_attribs, and we don't want the childAttribs object associated with UI interface item to be freed, we duplicate childAttribs here. */
static void exec_user_tool(GtkToolItem *item, RFM_ChildAttribs *childAttribs);
static void up_clicked(gpointer user_data);
static void home_clicked(gpointer user_data);
static void PreviousPage(RFM_ctx *rfmCtx);
static void NextPage(RFM_ctx *rfmCtx);
static void info_clicked(gpointer user_data);
static void switch_iconview_treeview(RFM_ctx *rfmCtx);
static void Switch_SearchResultView_DirectoryView(GtkToolItem *item,RFM_ctx *rfmCtx);
/* callback function for file menu */
/* since g_spawn_wrapper will free child_attribs, and we don't want the childAttribs object associated with UI interface item to be freed, we duplicate childAttribs here. */
static void file_menu_exec(GtkMenuItem *menuitem, RFM_ChildAttribs *childAttribs);
static RFM_fileMenu *setup_file_menu(RFM_ctx * rfmCtx);
static gboolean popup_file_menu(GdkEvent *event, RFM_ctx *rfmCtx);

static void copy_curPath_to_clipboard(GtkWidget *menuitem, gpointer user_data);

static void g_spawn_wrapper_for_selected_fileList_(RFM_ChildAttribs *childAttribs);
/* instantiate childAttribs and call g_spawn_wrapper_ */
/* caller should g_list_free(file_list), but usually not g_list_free_full, since the file char* is usually owned by rfm_fileattributes */
static gboolean g_spawn_wrapper(const char **action, GList *file_list, int run_opts, char *dest_path, gboolean async,void(*callbackfunc)(gpointer),gpointer callbackfuncUserData);
/* call build_cmd_vector to create the argv parameter for g_spawn_* */
/* call different g_spawn_* functions based on child_attribs->spawn_async and child_attribs->runOpts */
/* free child_attribs */
static gboolean g_spawn_wrapper_(GList *file_list, char *dest_path, RFM_ChildAttribs * childAttribs);
/* create argv parameter for g_spawn functions  */
static gchar **build_cmd_vector(const char **cmd, GList *file_list, char *dest_path);
static gboolean g_spawn_async_with_pipes_wrapper(gchar **v, RFM_ChildAttribs *child_attribs);
static gboolean g_spawn_async_with_pipes_wrapper_child_supervisor(gpointer user_data);
static void child_handler_to_set_finished_status_for_child_supervisor(GPid pid, gint status, RFM_ChildAttribs *child_attribs);
static gboolean ExecCallback_freeChildAttribs(RFM_ChildAttribs * child_attribs);
static void show_child_output(RFM_ChildAttribs *child_attribs);
static int read_char_pipe(gint fd, ssize_t block_size, char **buffer);
static void GSpawnChildSetupFunc_setenv(gpointer user_data);

/* Free functions*/
static void free_thumbQueueData(RFM_ThumbQueueData *thumbData);
static void free_child_attribs(RFM_ChildAttribs *child_attribs);
static void free_fileAttributes(RFM_FileAttributes *fileAttributes);
static void free_default_pixbufs(RFM_defaultPixbufs *defaultPixbufs);
static void cleanup(GtkWidget *window, RFM_ctx *rfmCtx);


#ifdef PythonEmbedded
/*https://docs.python.org/3.10/extending/embedding.html*/
static  wchar_t *pyProgramName=NULL;
static void startPythonEmbedding(){
  pyProgramName = Py_DecodeLocale(PROG_NAME, NULL);
  if (pyProgramName == NULL) {
        g_warning("Py_DecodeLocale: cannot decode argv[0]");
	return;
  }
  Py_SetProgramName(pyProgramName);  /* optional but recommended */
  Py_Initialize();
}

static void endPythonEmbedding(){
    if (Py_FinalizeEx() < 0) {
        g_warning("Py_FinalizeEx()<0");
	return;
    }
    PyMem_RawFree(pyProgramName);
    pyProgramName=NULL;
}
#endif

#include "rfmFileChooser.h"
#include "config.h"

static char * st_mode_str(guint32 st_mode){
    char * ret=calloc(11,sizeof(char));
    //文件类型
    if(S_ISDIR(st_mode))//目录文件
      ret[0]='d';
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
    //else if(S_ISSOCK(st_mode))//套接字文件
    //  ret[0]='s';
    // TODO: code copied from https://blog.csdn.net/xieeryihe/article/details/121715202 ,why S_ISSOCK not defined on my system?
    else ret[0]='-';

    //文件所有者权限
    if(st_mode&S_IRUSR) ret[1]='r';
    else ret[1]='-';
    if(st_mode&S_IWUSR) ret[2]='w';
    else ret[2]='-';
    if(st_mode&S_IXUSR) ret[3]='x';
    else ret[3]='-';
    
    //用户组权限
    if(st_mode&S_IRGRP) ret[4]='r';
    else ret[4]='-';
    if(st_mode&S_IWGRP) ret[5]='w';
    else ret[5]='-';
    if(st_mode&S_IXGRP) ret[6]='x';
    else ret[6]='-';

    //其他用户权限
    if(st_mode&S_IROTH) ret[7]='r';
    else ret[7]='-';
    if(st_mode&S_IWOTH) ret[8]='w';
    else ret[8]='-';
    if(st_mode&S_IXOTH) ret[9]='x';
    else ret[9]='-';

    ret[10]=0;
    return ret;
}

static void free_thumbQueueData(RFM_ThumbQueueData *thumbData)
{
   g_free(thumbData->uri);
   g_free(thumbData->path);
   g_free(thumbData->md5);
   g_free(thumbData->thumb_name);
   free(thumbData);
}

static void free_child_attribs(RFM_ChildAttribs *child_attribs)
{
   g_free(child_attribs->stdOut);
   g_free(child_attribs->stdErr);
   g_free(child_attribs->name);
   //how can we memcpy the content of customCallbackUserData and free it here?
   //g_free(child_attribs->customCallbackUserData);
   g_free(child_attribs);
}

static void rfm_stop_all(RFM_ctx *rfmCtx) {
   if (rfmCtx->delayedRefresh_GSourceID > 0)
      g_source_remove(rfmCtx->delayedRefresh_GSourceID);

   if (rfm_readDirSheduler>0)
      g_source_remove(rfm_readDirSheduler);

   if (rfm_thumbScheduler>0)
      g_source_remove(rfm_thumbScheduler);

   if (rfm_extColumnScheduler>0) g_source_remove(rfm_extColumnScheduler);
#ifdef GitIntegration
   if (rfm_gitCommitMsgScheduler>0) g_source_remove(rfm_gitCommitMsgScheduler);
#endif

   rfmCtx->delayedRefresh_GSourceID=0;
   rfm_readDirSheduler=0;
   rfm_thumbScheduler=0;
   rfm_extColumnScheduler=0;
#ifdef GitIntegration
   rfm_gitCommitMsgScheduler=0;
#endif
   g_list_free_full(rfm_thumbQueue, (GDestroyNotify)free_thumbQueueData);
   rfm_thumbQueue=NULL;
   gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
}


static gboolean ExecCallback_freeChildAttribs(RFM_ChildAttribs * child_attribs){
   if(child_attribs->exitcode==0 && (child_attribs->customCallBackFunc)!=NULL){
     if (child_attribs->runOpts!=RFM_EXEC_OUPUT_READ_BY_PROGRAM && child_attribs->runOpts!=RFM_EXEC_FILE_CHOOSER){
       //for old callback such as refresh_store, there is no need for child_attribs->stdout, so pass in customcallbackuserdata as parameter to remain compatible.

       (child_attribs->customCallBackFunc)(child_attribs->customCallbackUserData);
     }else{
       (child_attribs->customCallBackFunc)(child_attribs);
     }
   }

   free_child_attribs(child_attribs);
   return TRUE;
}


/* Supervise the children to prevent blocked pipes */
static gboolean g_spawn_async_with_pipes_wrapper_child_supervisor(gpointer user_data)
{
   RFM_ChildAttribs *child_attribs=(RFM_ChildAttribs*)user_data;

   if (child_attribs->runOpts!=RFM_EXEC_NONE) read_char_pipe(child_attribs->stdOut_fd, PIPE_SZ, &child_attribs->stdOut);
   read_char_pipe(child_attribs->stdErr_fd, PIPE_SZ, &child_attribs->stdErr);
   
   if (child_attribs->runOpts!=RFM_EXEC_OUPUT_READ_BY_PROGRAM && child_attribs->runOpts!=RFM_EXEC_NONE) show_child_output(child_attribs);
   if (child_attribs->status==-1)
       return TRUE;
   
   close(child_attribs->stdOut_fd);
   close(child_attribs->stdErr_fd);
   g_spawn_close_pid(child_attribs->pid);

   if (child_attribs->stdErr!=NULL && strlen(child_attribs->stdErr)>0) g_warning("g_spawn_async_with_wrapper_child_supervisor: %s",child_attribs->stdErr);
   rfm_childList=g_list_remove(rfm_childList, child_attribs);
   /* if (rfm_childList==NULL) */
   /*    gtk_widget_set_sensitive(GTK_WIDGET(info_button), FALSE); */

   GError *err=NULL;
   char * msg=NULL;

   //for g_spawn_sync, the lass parameter is GError *, so why pass in NULL there and get it here instead?
   /* if (g_spawn_check_wait_status(child_attribs->status, &err) && child_attribs->runOpts==RFM_EXEC_MOUNT) */
   /*    set_rfm_curPath(RFM_MOUNT_MEDIA_PATH); */
   g_spawn_check_wait_status(child_attribs->status, &err);
   //TODO: after i change runOpts to GSpawnFlags, RFM_EXEC_MOUNT won't work, i don't understand what it for, maybe later, we can use fields such as runCmd or some naming conventions in runaction or childAttribs to indicate this type.

   if (err!=NULL) {
      child_attribs->exitcode = err->code;
      if (g_error_matches(err, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED)) {
	 msg=g_strdup_printf("%s (pid: %i): stopped: %s", child_attribs->name, child_attribs->pid, err->message);
         show_msgbox(msg, child_attribs->name, GTK_MESSAGE_ERROR);
         g_free(msg);
      }
      g_error_free(err);
   }

   ExecCallback_freeChildAttribs(child_attribs);
   return FALSE;
}

static void child_handler_to_set_finished_status_for_child_supervisor(GPid pid, gint status, RFM_ChildAttribs *child_attribs)
{
   child_attribs->status=status; /* show_child_output() is called from child_supervisor() in case there is any data left in the pipes */
}

static void show_child_output(RFM_ChildAttribs *child_attribs)
{
   gchar *msg=NULL;
   
   /* Show any output we have regardless of error status */
   if (child_attribs->stdOut!=NULL && child_attribs->stdOut[0]!=0) {
     if (child_attribs->runOpts==RFM_EXEC_STDOUT || strlen(child_attribs->stdOut) > RFM_MX_MSGBOX_CHARS)
       printf("PID %s:%s",child_attribs->pid,child_attribs->stdOut);
     else
         show_msgbox(child_attribs->stdOut, child_attribs->name, GTK_MESSAGE_INFO);
     child_attribs->stdOut[0]=0;
   }

   if (child_attribs->stdErr!=NULL && child_attribs->stdErr[0]!=0 && strlen(child_attribs->stdErr)>0) {
      if (child_attribs->runOpts==RFM_EXEC_STDOUT || strlen(child_attribs->stdErr) > RFM_MX_MSGBOX_CHARS){
         msg=g_strdup_printf("%s (%i): Finished with exit code %i.\n%s", child_attribs->name, child_attribs->pid, child_attribs->exitcode, child_attribs->stdErr);
	 g_warning("%s",msg);
      } else {
         msg=g_strdup_printf("%s (%i): Finished with exit code %i.\n\n%s", child_attribs->name, child_attribs->pid, child_attribs->exitcode, child_attribs->stdErr);
         show_msgbox(msg, child_attribs->name, GTK_MESSAGE_ERROR);
      }
      g_free(msg);
      child_attribs->stdErr[0]=0;
   }
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
      return -1;
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
   defaultPixbufs->symlinkDir=gdk_pixbuf_copy(defaultPixbufs->dir);
   gdk_pixbuf_composite(defaultPixbufs->symlink,defaultPixbufs->symlinkDir,0,0,gdk_pixbuf_get_width(defaultPixbufs->symlink),gdk_pixbuf_get_height(defaultPixbufs->symlink),0,0,1,1,GDK_INTERP_NEAREST,200);
   defaultPixbufs->symlinkFile=gdk_pixbuf_copy(defaultPixbufs->file);
   gdk_pixbuf_composite(defaultPixbufs->symlink,defaultPixbufs->symlinkFile,0,0,gdk_pixbuf_get_width(defaultPixbufs->symlink),gdk_pixbuf_get_height(defaultPixbufs->symlink),0,0,1,1,GDK_INTERP_NEAREST,200);
   defaultPixbufs->unmounted=gdk_pixbuf_copy(defaultPixbufs->dir);
   gdk_pixbuf_composite(umount_pixbuf,defaultPixbufs->unmounted,1,1,gdk_pixbuf_get_width(umount_pixbuf),gdk_pixbuf_get_height(umount_pixbuf),0,0,1,1,GDK_INTERP_NEAREST,100);
   defaultPixbufs->mounted=gdk_pixbuf_copy(defaultPixbufs->dir);
   gdk_pixbuf_composite(mount_pixbuf,defaultPixbufs->mounted,1,1,gdk_pixbuf_get_width(mount_pixbuf),gdk_pixbuf_get_height(mount_pixbuf),0,0,1,1,GDK_INTERP_NEAREST,100);

   g_object_unref(umount_pixbuf);
   g_object_unref(mount_pixbuf);
  
   return defaultPixbufs;
}


static void show_msgbox(gchar *msg, gchar *title, gint type)
{
   GtkWidget *dialog;
   gchar *utf8_string=g_locale_to_utf8(msg, -1, NULL, NULL, NULL);

   if (utf8_string==NULL)
      dialog=gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", "Can't convert message text to UTF8!");
   else {
      dialog=gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, NULL);
      gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), utf8_string);
   }

   gtk_window_set_title(GTK_WINDOW(dialog), title);
   g_signal_connect_swapped (dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
   gtk_widget_show_all(dialog);
   if (type==GTK_MESSAGE_ERROR)
      gtk_dialog_run(GTK_DIALOG(dialog)); /* Show a modal dialog for errors/warnings */
   g_free(utf8_string);
}


static gboolean g_spawn_async_with_pipes_wrapper(gchar **v, RFM_ChildAttribs *child_attribs)
{
   gboolean rv=FALSE;
   if (child_attribs!=NULL) {
      child_attribs->pid=-1;
      GError* err = NULL;
      rv=g_spawn_async_with_pipes(rfm_curPath, v, NULL, G_SPAWN_DO_NOT_REAP_CHILD | child_attribs->runOpts,
				  GSpawnChildSetupFunc_setenv,child_attribs,
                                  &child_attribs->pid, NULL, ((child_attribs->runOpts==RFM_EXEC_FILE_CHOOSER)? NULL: &child_attribs->stdOut_fd),
                                  &child_attribs->stdErr_fd, &err);
      
      g_debug("g_spawn_async_with_pipes_wrapper:  workingdir:%s, argv:%s, G_SPAWN_DO_NOT_REAP_CHILD",rfm_curPath,v[0]);
      if (rv==TRUE) {
         /* Don't block on read if nothing in pipe */
         if (! g_unix_set_fd_nonblocking(child_attribs->stdOut_fd, TRUE, NULL))
	    g_warning("Can't set child stdout to non-blocking mode, PID:%d,FD:%d",child_attribs->pid,child_attribs->stdOut_fd);
         if (! g_unix_set_fd_nonblocking(child_attribs->stdErr_fd, TRUE, NULL))
	    g_warning("Can't set child stderr to non-blocking mode, PID:%d,FD:%d",child_attribs->pid,child_attribs->stdErr_fd);

         if(child_attribs->name==NULL) child_attribs->name=g_strdup(v[0]);
         child_attribs->status=-1;  /* -1 indicates child is running; set to wait wstatus on exit */
	 child_attribs->exitcode=0;

         g_timeout_add(100, (GSourceFunc)g_spawn_async_with_pipes_wrapper_child_supervisor, (void*)child_attribs);
         g_child_watch_add(child_attribs->pid, (GChildWatchFunc)child_handler_to_set_finished_status_for_child_supervisor, child_attribs);
         rfm_childList=g_list_prepend(rfm_childList, child_attribs);
         //gtk_widget_set_sensitive(GTK_WIDGET(info_button), TRUE);
      }else{
	g_warning("g_spawn_async_with_pipe error:%s",err->message);
	g_free(err);
      }
   }
   return rv;
}

// The char* in file_list will be used (WITHOUT strdup) in returned gchar** v and owned by rfm_FileAttributelist before
static gchar **build_cmd_vector(const char **cmd, GList *file_list, char *dest_path)
{
   long j=0;
   gchar **v=NULL;
   GList *listElement=NULL;

   listElement = (file_list==NULL)? NULL : g_list_first(file_list);
  
   if((v=calloc((RFM_MX_ARGS), sizeof(gchar*)))==NULL)
      return NULL;

   while (cmd[j]!=NULL && j<RFM_MX_ARGS) {
     if (strcmp(cmd[j],"")!=0)
       v[j]=(gchar*)cmd[j]; /* FIXME: gtk spawn functions require gchar*, but we have const gchar*; should probably g_strdup() and create a free_cmd_vector() function */
     else if (listElement != NULL) {
       // before this commit, file_list and dest_path are all appended after cmd, but commands like ffmpeg to create thumbnail need to have file name in the middle of the argv, appended at the end won't work. So i modify the rule here so that if we have empty string in cmd, we replace it with item in file_list. So, file_list can work as generic argument list later, not necessarily the filename. And replacing empty string place holders in cmd with items in file_list can be something like printf.
       v[j]=listElement->data; //this data is owned by rfm_FileAttributes and was not freed before, but now, v and file_list share the reference.  g_list_free_full never called on file_list, and filename char* is not freed when free(v),  but freed with rfm_FileAttributes.
       listElement=g_list_next(listElement);
     }

      j++;
   }
   if (j==RFM_MX_ARGS) {
      g_warning("build_cmd_vector: %s: RFM_MX_ARGS exceeded. Check config.h!",v[0]);
      free(v);
      return NULL;
   }
   while (listElement!=NULL) {
      v[j]=listElement->data;
      j++;
      listElement=g_list_next(listElement);
   }

   v[j]=dest_path; /* This may be NULL anyway */
   v[++j]=NULL;

   g_debug("%s",g_strjoinv(" ", v));
   return v;
}

static void GSpawnChildSetupFunc_setenv(gpointer user_data) {
      RFM_ChildAttribs* child_attribs=(RFM_ChildAttribs*) user_data;
      //if (rfm_curPath!=NULL) setenv("PWD",rfm_curPath,TRUE);
      //working_directory won't update child process PWD env, which inherits parents PWD env,why?
}

//caller should g_list_free(file_list), but usually not g_list_free_full, since the file char* is usually owned by rfm_fileattributes
static gboolean g_spawn_wrapper_(GList *file_list, char *dest_path, RFM_ChildAttribs * child_attribs)
{
   gchar **v=NULL;
   gboolean ret=TRUE;

   v=build_cmd_vector(child_attribs->RunCmd, file_list, dest_path);
   if (v != NULL) {
      if (child_attribs->spawn_async){
	       if (!g_spawn_async_with_pipes_wrapper(v, child_attribs)) {
                   g_warning("g_spawn_wrapper_->g_spawn_async_with_pipes_wrapper: %s failed to execute. Check command in config.h!",v[0]);
                   free_child_attribs(child_attribs); //这里是失败的异步，成功的异步会在  child_supervisor_to_ReadStdout_ShowOutput_ExecCallback 里面 free
	           ret = FALSE;
               };
      } else {
	       child_attribs->exitcode=0;
	       child_attribs->status=-1;
/* (rfm:11714): GLib-CRITICAL **: 14:05:51.441: g_spawn_sync: assertion 'standard_output == NULL || !(flags & G_SPAWN_STDOUT_TO_DEV_NULL)' failed */

/* (rfm:11714): rfm-WARNING **: 14:05:51.441: g_spawn_wrapper_->g_spawn_sync /usr/bin/ffmpeg failed to execute. Check command in config.h! */	       
	       g_debug("g_spawn_wrapper_->g_spawn_sync, workingdir:%s, argv:%s ",rfm_curPath,v[0]);
	       if (!g_spawn_sync(rfm_curPath, v, NULL,child_attribs->runOpts, GSpawnChildSetupFunc_setenv,child_attribs,(child_attribs->runOpts & G_SPAWN_STDOUT_TO_DEV_NULL)? NULL : &child_attribs->stdOut, &child_attribs->stdErr,&child_attribs->status,NULL)){
	            g_warning("g_spawn_wrapper_->g_spawn_sync %s failed to execute. Check command in config.h!", v[0]);
	            free_child_attribs(child_attribs);
	            ret = FALSE;
	       }else
	            ExecCallback_freeChildAttribs(child_attribs);
      }
      
      free(v); //only free v, but char* data such as filename in command not freed here.
   }
   else{
      g_warning("g_spawn_wrapper_: %s failed to execute: build_cmd_vector() returned NULL.",child_attribs->RunCmd[0]);
      free_child_attribs(child_attribs);
      ret = FALSE;
   }
   return ret;
}

static gboolean g_spawn_wrapper(const char **action, GList *file_list, int run_opts, char *dest_path, gboolean async,void(*callbackfunc)(gpointer),gpointer callbackfuncUserData){
  RFM_ChildAttribs *child_attribs=calloc(1,sizeof(RFM_ChildAttribs));
  child_attribs->customCallBackFunc=callbackfunc;
  child_attribs->customCallbackUserData=callbackfuncUserData;
  child_attribs->runOpts=run_opts;
  child_attribs->RunCmd = action;
  child_attribs->stdOut = NULL;
  child_attribs->stdErr = NULL;
  child_attribs->spawn_async = async;
  child_attribs->name=g_strdup(action[0]);
	
  return g_spawn_wrapper_(file_list,dest_path,child_attribs);
}

/* Load and update a thumbnail from disk cache: key is the md5 hash of the required thumbnail */
static int load_thumbnail(gchar *key)
{
   GtkTreeIter iter;
   GdkPixbuf *pixbuf=NULL;
   GtkTreePath *treePath;
   gchar *thumb_path;
   const gchar *tmp=NULL;
   GtkTreeRowReference *reference;
   gint64 mtime_file=0;
   gint64 mtime_thumb=1;

   reference=g_hash_table_lookup(thumb_hash, key);
   if (reference==NULL) return 1;  /* Key not found */

   treePath=gtk_tree_row_reference_get_path(reference);
   if (treePath == NULL)
      return 1;   /* Tree path not found */
      
   gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treePath);
   gtk_tree_path_free(treePath);
   thumb_path=g_build_filename(rfm_thumbDir, key, NULL);
   pixbuf=gdk_pixbuf_new_from_file(thumb_path, NULL);
   g_free(thumb_path);
   if (pixbuf==NULL)
      return 2;   /* Can't load thumbnail */

   gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_MTIME, &mtime_file, -1);
   tmp=gdk_pixbuf_get_option(pixbuf, "tEXt::Thumb::MTime");
   if (tmp!=NULL) mtime_thumb=g_ascii_strtoll(tmp, NULL, 10); /* Convert to gint64 */
   if (mtime_file!=mtime_thumb) {
#ifdef Allow_Thumbnail_Without_tExtThumbMTime
     if (tmp != NULL) {
#endif
       g_object_unref(pixbuf);
       return 3; /* Thumbnail out of date */
#ifdef Allow_Thumbnail_Without_tExtThumbMTime
     }
#endif
   }
   
   gtk_list_store_set (store, &iter, COL_PIXBUF, pixbuf, -1);
   g_object_unref(pixbuf);
   return 0;
}

/* Return the index of a defined thumbnailer which will handle the mime type */
static gint find_thumbnailer(gchar *mime_root, gchar *mime_sub_type)
{
   gint t_idx=-1;
   gint a_idx=-1;
   gint i;
   
   for(i=0;i<G_N_ELEMENTS(thumbnailers);i++) {   /* Check for user defined thumbnailer */
      if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strcmp(mime_sub_type, thumbnailers[i].thumbSub)==0) {
         t_idx=i;
         break;
      }
      if (strcmp(mime_root, thumbnailers[i].thumbRoot)==0 && strncmp("*", thumbnailers[i].thumbSub, 1)==0)
         a_idx=i;
   }
   if (t_idx==-1) t_idx=a_idx; /* Maybe we have a generator for all sub types of mimeRoot */

   return t_idx;
}

static void rfm_saveThumbnail(GdkPixbuf *thumb, RFM_ThumbQueueData *thumbData)
{
   GdkPixbuf *thumbAlpha=NULL;
   gchar *mtime_tmp;
   gchar *tmp_thumb_file;
   gchar *thumb_path;

   thumbAlpha=gdk_pixbuf_add_alpha(thumb, FALSE, 0, 0, 0);  /* Add a transparency channel: some software expects this */

   if (thumbAlpha!=NULL) {
      thumb_path=g_build_filename(rfm_thumbDir, thumbData->thumb_name, NULL);
      tmp_thumb_file=g_strdup_printf("%s-%s-%ld", thumb_path, PROG_NAME, (long)thumbData->rfm_pid); /* check pid_t type: echo | gcc -E -xc -include 'unistd.h' - | grep 'typedef.*pid_t' */
      mtime_tmp=g_strdup_printf("%"G_GINT64_FORMAT, thumbData->mtime_file);
      if (tmp_thumb_file!=NULL && mtime_tmp!=NULL) {
         gdk_pixbuf_save(thumbAlpha, tmp_thumb_file, "png", NULL,
            "tEXt::Thumb::MTime", mtime_tmp,
            "tEXt::Thumb::URI", thumbData->uri,
            "tEXt::Software", PROG_NAME,
            NULL);
         if (chmod(tmp_thumb_file, S_IRUSR | S_IWUSR)==-1)
            g_warning("rfm_saveThumbnail: Failed to chmod %s\n", tmp_thumb_file);
         if (rename(tmp_thumb_file, thumb_path)!=0)
            g_warning("rfm_saveThumbnail: Failed to rename %s\n", tmp_thumb_file);
      }
      g_free(thumb_path);
      g_free(mtime_tmp);
      g_free(tmp_thumb_file);
      g_object_unref(thumbAlpha);
   }
}

//called by gtk idle time scheduler and create one thumbnail a time for the enqueued in rfm_thumbQueue;
static gboolean mkThumb()
{
   RFM_ThumbQueueData *thumbData;
   GdkPixbuf *thumb=NULL;
   GError *pixbufErr=NULL;

   thumbData=(RFM_ThumbQueueData*)rfm_thumbQueue->data;
   if (thumbnailers[thumbData->t_idx].thumbCmd==NULL){
      thumb=gdk_pixbuf_new_from_file_at_scale(thumbData->path, thumbData->thumb_size, thumbData->thumb_size, TRUE, &pixbufErr);
      if (thumb!=NULL) {
	rfm_saveThumbnail(thumb, thumbData);
	g_debug("thumbnail saved for %s",thumbData->thumb_name);
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
      g_spawn_wrapper(thumbnailers[thumbData->t_idx].thumbCmd, input_files, RFM_EXEC_NONE, thumb_path, FALSE, NULL, NULL);
      g_list_free(input_files);
   }
   
   if (rfm_thumbQueue->next!=NULL) {   /* More items in queue */
      rfm_thumbQueue=g_list_next(rfm_thumbQueue);
      g_debug("mkThumb return TRUE after:%s",thumbData->thumb_name);
      return TRUE;
   }
   
   g_list_free_full(rfm_thumbQueue, (GDestroyNotify)free_thumbQueueData);
   rfm_thumbQueue=NULL;
   rfm_thumbScheduler=0;
   g_debug("mkThumb return FALSE,which means mkThumb finished");
   return FALSE;  /* Finished thumb queue */
}

static RFM_ThumbQueueData *get_thumbData(GtkTreeIter *iter)
{
   GtkTreePath *treePath=NULL;
   RFM_ThumbQueueData *thumbData;
   RFM_FileAttributes *fileAttributes;

   gtk_tree_model_get(GTK_TREE_MODEL(store), iter, COL_ATTR, &fileAttributes, -1);

   if (fileAttributes->is_dir || fileAttributes->is_symlink) return NULL;
   
   thumbData=calloc(1, sizeof(RFM_ThumbQueueData));
   if (thumbData==NULL) return NULL;

   thumbData->t_idx=find_thumbnailer(fileAttributes->mime_root, fileAttributes->mime_sub_type);
   if (thumbData->t_idx==-1) {
      free(thumbData);
      return NULL;  /* Don't show thumbnails for files types with no thumbnailer */
   }

   thumbData->thumb_size = RFM_THUMBNAIL_SIZE;
   thumbData->path=g_strdup(fileAttributes->path);
   thumbData->mtime_file = fileAttributes->file_mtime==NULL? 0 : g_date_time_to_unix(fileAttributes->file_mtime);
   thumbData->uri=g_filename_to_uri(thumbData->path, NULL, NULL);
   thumbData->md5=g_compute_checksum_for_string(G_CHECKSUM_MD5, thumbData->uri, -1);
   thumbData->thumb_name=g_strdup_printf("%s.png", thumbData->md5);
   thumbData->rfm_pid=getpid();  /* pid is used to generate a unique temporary thumbnail name */

   

   /* Map thumb path to model reference for inotify */
   treePath=gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
   g_hash_table_insert(thumb_hash, g_strdup(thumbData->thumb_name), gtk_tree_row_reference_new(GTK_TREE_MODEL(store), treePath));
   gtk_tree_path_free(treePath);

   return thumbData;
}

static void free_fileAttributes(RFM_FileAttributes *fileAttributes) {
   g_free(fileAttributes->path);
   g_free(fileAttributes->file_name);
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
   if (fileAttributes==NULL)
     g_error("calloc failed!");

   /* fileAttributes->path=NULL; */
   /* fileAttributes->file_name=NULL; */
   /* fileAttributes->display_name=NULL; */
   /* fileAttributes->pixbuf=NULL; */
   /* fileAttributes->mime_root=NULL; */
   /* fileAttributes->mime_sub_type=NULL; */
   /* fileAttributes->file_mtime=0; */
   /* fileAttributes->is_dir=FALSE; */
   /* fileAttributes->is_mountPoint=FALSE; */
   /* fileAttributes->is_symlink=FALSE; */
   /* fileAttributes->icon_name=NULL; */
   /* fileAttributes->group=NULL; */
   /* fileAttributes->owner=NULL; */
   /* fileAttributes->file_mode_str=NULL; */
   /* fileAttributes->mime_sort=NULL; */
   return fileAttributes;
}

static RFM_FileAttributes *get_fileAttributes_for_a_file(const gchar *name, guint64 mtimeThreshold, GHashTable *mount_hash)
{
   GFile *file=NULL;
   GFileInfo *info=NULL;
   gchar *attibuteList="*";
   GFileType fileType;
   RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");
   gchar *mime_type=NULL;
   //gchar *utf8_display_name=NULL;
   gchar *is_mounted=NULL;
   gint i;
   RFM_FileAttributes *fileAttributes=malloc_fileAttributes();

   gchar *absoluteaddr;
   if (name[0]=='/')
     absoluteaddr = g_build_filename(name, NULL); /*if mlocate index not updated with updatedb, address returned by locate will return NULL after canonicalize */
   else if (SearchResultViewInsteadOfDirectoryView)
     absoluteaddr = g_build_filename(rfm_SearchResultPath, name, NULL);
   else
     absoluteaddr = g_build_filename(rfm_curPath, name, NULL);
     //address returned by find can be ./blabla, and g_build_filename return something like /rfm/./blabla, so, need to be canonicalized here
   
   if (canonicalize_file_name(absoluteaddr)==NULL){
     g_warning("invalid address:%s",name);
     fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
     fileAttributes->file_name=g_strdup(name);
     fileAttributes->path=absoluteaddr;
     fileAttributes->mime_root=g_strdup("na");
     fileAttributes->mime_sub_type=g_strdup("na");
     return fileAttributes;
   }else{
     fileAttributes->path=absoluteaddr;
     g_debug("fileAttributes->path:%s",fileAttributes->path);
   }
   //attibuteList=g_strdup_printf("%s,%s,%s,%s",G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK);
   file=g_file_new_for_path(fileAttributes->path);
   info=g_file_query_info(file, attibuteList, G_FILE_QUERY_INFO_NONE, NULL, NULL);
   //g_free(attibuteList);
   g_object_unref(file); file=NULL;

   if (info == NULL ) {
      g_warning("g_file_query_info return NULL for %s",fileAttributes->path);
      g_free(fileAttributes->path);
      g_free(fileAttributes);
      return NULL;
   }

   fileAttributes->file_mtime=g_file_info_get_modification_date_time(info);
   fileAttributes->file_atime=g_file_info_get_access_date_time(info);
   fileAttributes->file_ctime=g_file_info_get_creation_date_time(info);
   fileAttributes->file_size=g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
   fileAttributes->file_mode=g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_MODE);
   fileAttributes->file_mode_str=st_mode_str(fileAttributes->file_mode);
   fileAttributes->file_name=g_strdup(name);

   fileAttributes->is_symlink=g_file_info_get_is_symlink(info);
   fileType=g_file_info_get_file_type(info);

   switch (fileType) {
      case G_FILE_TYPE_DIRECTORY:
         fileAttributes->is_dir=TRUE;
         fileAttributes->mime_root=g_strdup("inode");
         if (fileAttributes->is_symlink) {
            fileAttributes->mime_sub_type=g_strdup("symlink");
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->symlinkDir);
         }
         else if (g_hash_table_lookup_extended(mount_hash, fileAttributes->path, NULL, (gpointer)&is_mounted)) {
            fileAttributes->mime_sub_type=g_strdup("mount-point");
            fileAttributes->is_mountPoint=TRUE;
            if (is_mounted[0]=='0')
               fileAttributes->pixbuf=g_object_ref(defaultPixbufs->unmounted);
            else
               fileAttributes->pixbuf=g_object_ref(defaultPixbufs->mounted);
         }
         else {
            fileAttributes->mime_sub_type=g_strdup("directory");
            fileAttributes->pixbuf=g_object_ref(defaultPixbufs->dir);
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

   fileAttributes->owner = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_OWNER_USER);
   fileAttributes->group = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_OWNER_GROUP);

   g_object_unref(info); info=NULL;
   return fileAttributes;
}

static void iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration(void)
{
   GtkTreeIter iter;
   gboolean valid;

   valid=gtk_tree_model_get_iter_first(treemodel, &iter);
   while (valid) {
     load_thumbnail_or_enqueue_thumbQueue_for_store_row(&iter);
#ifdef GitIntegration
     if (GetColumnByEnun(COL_GIT_COMMIT_MSG)->Show) load_gitCommitMsg_for_store_row(&iter);
#endif
     valid=gtk_tree_model_iter_next(treemodel, &iter);
   }
   if (rfm_thumbQueue!=NULL)
      rfm_thumbScheduler=g_idle_add((GSourceFunc)mkThumb, NULL);
}


static void load_thumbnail_or_enqueue_thumbQueue_for_store_row(GtkTreeIter *iter){
   RFM_ThumbQueueData *thumbData=NULL;
      thumbData=get_thumbData(iter); /* Returns NULL if thumbnail not handled */
      if (thumbData!=NULL) {
         /* Try to load any existing thumbnail */
	int ld=load_thumbnail(thumbData->thumb_name);
	 if ( ld == 0) { /* Success: thumbnail exists in cache and is valid */
	   g_debug("thumbnail %s exists for %s",thumbData->thumb_name, thumbData->path);
           free_thumbQueueData(thumbData);
         } else { /* Thumbnail doesn't exist or is out of date */
           rfm_thumbQueue = g_list_append(rfm_thumbQueue, thumbData);
	   g_debug("thumbnail %s creation enqueued for %s; load_thumbnail failure code:%d.",thumbData->thumb_name,thumbData->path,ld);
         }
      }
}

#ifdef GitIntegration
static void load_gitCommitMsg_for_store_row(GtkTreeIter *iter){
      if (curPath_is_git_repo){
	//gchar* fullpath=calloc(PATH_MAX, sizeof(gchar));
	GValue full_path=G_VALUE_INIT;
	gtk_tree_model_get_value(treemodel, iter, COL_FULL_PATH, &full_path);
	GList *file_list = NULL;
	file_list = g_list_append(file_list,g_value_get_string(&full_path));
	GtkTreeIter **iterPointerPointer=calloc(1, sizeof(GtkTreeIter**));//This will be passed into childAttribs, which will be freed in g_spawn_wrapper. but we shall not free iter, so i use pointer to pointer here.
	*iterPointerPointer=iter;
	if(!g_spawn_wrapper(git_commit_message_cmd, file_list, RFM_EXEC_OUPUT_READ_BY_PROGRAM ,NULL, FALSE, readGitCommitMsgFromGitLogCmdAndUpdateStore, iterPointerPointer)){
	}
	g_list_free(file_list);
      }
}
#endif


static void Update_Store_ExtColumns(RFM_ChildAttribs *childAttribs) {
   gchar * value=childAttribs->stdOut;
   value[strcspn(value, "\n")] = 0;
   g_debug("ExtColumn Value:%s",value);
   RFM_store_cell* cell= *(RFM_store_cell**)(childAttribs->customCallbackUserData);
   if (cell->store_column>=0 && cell->store_column<NUM_COLS)  gtk_list_store_set(store,cell->iter, cell->store_column, value, -1);
   if (g_strcmp0(value,"")!=0){
     if (cell->iconview_markup) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_MARKUP, value, -1);
     if (cell->iconview_tooltip) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_TOOLTIP, value, -1);
   }
   g_free(cell);
}

static void load_ExtColumns_and_iconview_markup_tooltip(RFM_FileAttributes* fileAttributes, GtkTreeIter *iter){
      g_debug("load_ExtColumns for %s",fileAttributes->path);
      for(guint i=0;i<G_N_ELEMENTS(treeviewColumns);i++){
	if ((treeviewColumns[i].Show || treeviewColumns[i].iconview_markup || treeviewColumns[i].iconview_tooltip)
	    && (g_strcmp0(treeviewColumns[i].MIME_root, "*")==0 || g_strcmp0(fileAttributes->mime_root, treeviewColumns[i].MIME_root)==0)
	    && (treeviewColumns[i].showCondition==NULL || treeviewColumns[i].showCondition(fileAttributes) )){
	  if (g_strcmp0(treeviewColumns[i].MIME_sub, "*") || g_strcmp0(treeviewColumns[i].MIME_sub, fileAttributes->mime_sub_type)){
	    RFM_store_cell* cell=malloc(sizeof(RFM_store_cell));
	    cell->iter = iter;
	    cell->store_column = treeviewColumns[i].enumCol;
	    cell->iconview_markup = treeviewColumns[i].iconview_markup;
	    cell->iconview_tooltip = treeviewColumns[i].iconview_tooltip;
	    if (treeviewColumns[i].ValueCmd!=NULL){
              gchar* ExtColumn_cmd = g_strdup_printf(treeviewColumns[i].ValueCmd, fileAttributes->path);
	      gchar* ExtColumn_cmd_template[] = {"/bin/bash", "-c", ExtColumn_cmd, NULL};
	      g_spawn_wrapper(ExtColumn_cmd_template, NULL, RFM_EXEC_OUPUT_READ_BY_PROGRAM, NULL, FALSE, Update_Store_ExtColumns, &cell);
	    }else if (treeviewColumns[i].ValueFunc!=NULL){
	    //TODO:ValueCmd==NULL but with function!=NULL
	    }else if ((cell->iconview_markup) || (cell->iconview_tooltip)){
	      GValue enumColValue = G_VALUE_INIT;
	      gtk_tree_model_get_value(treemodel, iter, treeviewColumns[i].enumCol, &enumColValue);;
	      if (cell->iconview_markup) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_MARKUP, &enumColValue, -1);
	      if (cell->iconview_tooltip) gtk_list_store_set(store, cell->iter, COL_ICONVIEW_TOOLTIP, &enumColValue, -1);
	    }
	  }
	}
      }  
}


static void Iterate_through_fileAttribute_list_to_insert_into_store()
{
   GList *listElement;
   RFM_FileAttributes *fileAttributes;
   GtkTreeIter iter;
   if (rfm_fileAttributeList==NULL) return;
   listElement=g_list_first(rfm_fileAttributeList);
   while (listElement != NULL) {
      fileAttributes=(RFM_FileAttributes*)listElement->data;
      Insert_fileAttributes_into_store(fileAttributes, &iter);
      load_ExtColumns_and_iconview_markup_tooltip(fileAttributes, &iter);
   listElement=g_list_next(listElement);
   }
}

static void Insert_fileAttributes_into_store(RFM_FileAttributes *fileAttributes, GtkTreeIter *iter)
{
      GtkTreePath *treePath=NULL;
      GdkPixbuf *theme_pixbuf=NULL;
      RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");

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
      gtk_list_store_insert_with_values(store, iter, -1,
                          COL_MODE_STR, fileAttributes->file_mode_str,
					//COL_DISPLAY_NAME, fileAttributes->display_name,
			  COL_FILENAME,fileAttributes->file_name,
			  COL_FULL_PATH,fileAttributes->path,
                          COL_PIXBUF, fileAttributes->pixbuf,
			  COL_MTIME, fileAttributes->file_mtime==NULL? 0 : g_date_time_to_unix(fileAttributes->file_mtime),
			  COL_MTIME_STR,fileAttributes->mtime,
             		  COL_SIZE,fileAttributes->file_size,
                          COL_ATTR, fileAttributes,
                          COL_OWNER,fileAttributes->owner,
			  COL_GROUP,fileAttributes->group,
                          COL_MIME_ROOT,fileAttributes->mime_root,
			  COL_MIME_SUB,fileAttributes->mime_sub_type,
			  COL_ATIME_STR,fileAttributes->atime,
                          COL_CTIME_STR,fileAttributes->ctime,
#ifdef GitIntegration
                          COL_GIT_STATUS_STR,gitStatus,
					//COL_GIT_COMMIT_MSG,git_commit_msg,
#endif
			  COL_MIME_SORT,fileAttributes->mime_sort,
			  COL_ICONVIEW_MARKUP, fileAttributes->file_name,
			  COL_ICONVIEW_TOOLTIP,NULL,
                          -1);

      g_debug("Inserted into store:%s",fileAttributes->file_name);
      
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
	    break;
	  }
	  selection_filepath_list = g_list_next(selection_filepath_list);
	}
     }
}


#ifdef GitIntegration
static void readGitCommitMsgFromGitLogCmdAndUpdateStore(RFM_ChildAttribs * childAttribs){
   gchar * commitMsg=childAttribs->stdOut;
   commitMsg[strcspn(commitMsg, "\n")] = 0;
   g_debug("gitCommitMsg:%s",commitMsg);
   GtkTreeIter *iter= *(GtkTreeIter**)(childAttribs->customCallbackUserData);
   gtk_list_store_set(store,iter,COL_GIT_COMMIT_MSG, commitMsg, -1);
}

static void load_GitTrackedFiles_into_HashTable()
{
  gchar * child_stdout;
  gchar * child_stderr;
  gchar * git_root;

  if (g_spawn_sync(rfm_curPath, git_root_cmd, NULL, 0, NULL, NULL,&child_stdout, &child_stderr, 0, NULL)) {
    git_root = g_strdup(strtok(child_stdout, "\n"));

    g_debug("git root:%s",git_root);

    g_free(child_stdout);
    g_free(child_stderr);
  } else {
    g_warning("%s", child_stderr);
    g_free(child_stdout);
    g_free(child_stderr);   
    return;
  }    
  
  if (g_spawn_sync(rfm_curPath, git_ls_files_cmd, NULL, 0, NULL, NULL, &child_stdout, &child_stderr, 0, NULL)){

    gchar * oneline=strtok(child_stdout,"\n");
    while (oneline!=NULL){
      gchar * fullpath=g_build_filename(git_root,oneline,NULL);         
      g_hash_table_insert(gitTrackedFiles,fullpath,g_strdup(""));
      g_debug("gitTrackedFile:%s",fullpath);
      oneline=strtok(NULL, "\n");
    }
  }
  else{
    g_warning("%s",child_stderr);
  }
  g_free(child_stdout);
  g_free(child_stderr);


  if (g_spawn_sync(rfm_curPath, git_modified_staged_info_cmd, NULL, 0, NULL, NULL, &child_stdout, &child_stderr, 0, NULL)){

    gchar *oneline=strtok(child_stdout,"\n");
    while (oneline!=NULL && strlen(oneline)>3){
      //first 2 char of oneline is git status
      gchar *status=g_strndup(oneline, 2);
      //copy rest of oneline into filename
      gchar *beginfilename = oneline + 3;
      gchar *filename=g_strndup(beginfilename,strlen(oneline)-3);
      //TODO: remove ending "/"
      //when i mkdir test in gitreporoot
      //cd test
      //touch testfile.md
      //i can see ?? test/ in git status --porcelain result
      //but the same file path in fileattributes does not have ending /, so , they won't match
      //BTW, git status --porcelain don't have something like ?? test/testfile.md, is it a git issue?
      gchar *fullpath=g_build_filename(git_root,filename,NULL);         
      g_debug("gitTrackedFile Status:%s,%s",status,fullpath);

      g_hash_table_insert(gitTrackedFiles,g_strdup(fullpath),status);
      
      if ((SearchResultViewInsteadOfDirectoryView^1) && (g_strcmp0(" D", status)==0 || g_strcmp0("D ", status)==0) && g_strcmp0(g_path_get_dirname(fullpath), rfm_curPath)==0){
	//add item into fileattributelist so that user can git stage on it
	RFM_FileAttributes *fileAttributes=malloc_fileAttributes();
	if (fileAttributes==NULL)
	  g_warning("malloc_fileAttributes failed");
	else{//if the file is rfm ignord file, we still add it into display here,but this may be changed.
	  fileAttributes->pixbuf=g_object_ref(defaultPixbufs->broken);
	  fileAttributes->file_name=g_strdup(filename);
	  //fileAttributes->display_name=g_strdup(filename);
	  fileAttributes->path=fullpath;
	  fileAttributes->mime_root=g_strdup("na");
	  fileAttributes->mime_sub_type=g_strdup("na");
	  //rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	  Insert_fileAttributes_into_store_with_thumbnail_and_more(fileAttributes);
	  //TODO: after i run `git rm -r --cached video`, rfm will show video subdir as shown on devPicAndVideo/20231210_15h14m08s_grim.png
	  //so we should not insert fileattributes if the file is still in directory, and it is just removed from git track, not deleted
	  //or we can insert fileattributes here, but update it when read fileattribute later from directory, instead of insert it again.
	  //Furthur, think about we need to load extended attribs for files, such as those in email, shall we consider general multiple pass fileattributes loading framework?
	}
      }
      
      g_free(filename);
      oneline=strtok(NULL, "\n");
    }
  }
  else{
    g_warning("%s",child_stderr);
  }
  g_free(child_stdout);
  g_free(child_stderr);
  g_free(git_root);
}
#endif

static void Insert_fileAttributes_into_store_with_thumbnail_and_more(RFM_FileAttributes* fileAttributes){
	    GtkTreeIter iter;
            rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
	    Insert_fileAttributes_into_store(fileAttributes,&iter);
	    load_ExtColumns_and_iconview_markup_tooltip(fileAttributes, &iter);
	    if (rfm_do_thumbs==1 && g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR)){
	      load_thumbnail_or_enqueue_thumbQueue_for_store_row(&iter);
#ifdef GitIntegration
              if (GetColumnByEnun(COL_GIT_COMMIT_MSG)->Show) load_gitCommitMsg_for_store_row(&iter);
#endif
	    }  
}

static gboolean read_one_DirItem_into_fileAttributeList_and_insert_into_store_in_each_call(GDir *dir) {
   const gchar *name=NULL;
   time_t mtimeThreshold=time(NULL)-RFM_MTIME_OFFSET;
   RFM_FileAttributes *fileAttributes;
   GHashTable *mount_hash=get_mount_points();

   name=g_dir_read_name(dir);
   if (name!=NULL) {
     if (!ignored_filename(name)) {
         fileAttributes=get_fileAttributes_for_a_file(name, mtimeThreshold, mount_hash);
         if (fileAttributes!=NULL){
	   Insert_fileAttributes_into_store_with_thumbnail_and_more(fileAttributes);
	 }
      }
      g_hash_table_destroy(mount_hash);
      return TRUE;   /* Return TRUE if more items */
   }
   else if (rfm_thumbQueue!=NULL)
      rfm_thumbScheduler=g_idle_add((GSourceFunc)mkThumb, NULL);

   rfm_readDirSheduler=0;
   g_hash_table_destroy(mount_hash);
   In_refresh_store = FALSE;
   gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
   return FALSE;
}

static void TurnPage(RFM_ctx *rfmCtx, gboolean next) {
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
    refresh_store(rfmCtx);
  }
}

static void NextPage(RFM_ctx *rfmCtx) { TurnPage(rfmCtx, TRUE); }

static void PreviousPage(RFM_ctx *rfmCtx) { TurnPage(rfmCtx, FALSE); }

static void FirstPage(RFM_ctx *rfmCtx){
  CurrentPage_SearchResultView = SearchResultFileNameList;
  currentFileNum = 1;
  refresh_store(rfmCtx);
}

static void set_DisplayingPageSize_ForFileNameListFromPipesStdIn(uint pagesize){
  if (SearchResultViewInsteadOfDirectoryView && SearchResultFileNameList!=NULL) {
    PageSize_SearchResultView = pagesize;
    FirstPage(rfmCtx);
  }
}


static gboolean fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store() {
  time_t mtimeThreshold=time(NULL)-RFM_MTIME_OFFSET;
  RFM_FileAttributes *fileAttributes;
  GHashTable *mount_hash=get_mount_points();

  gint i=0;
  GList *name=CurrentPage_SearchResultView;
  while (name!=NULL && i<PageSize_SearchResultView){
    fileAttributes = get_fileAttributes_for_a_file(name->data, mtimeThreshold, mount_hash);
    if (fileAttributes != NULL) {
      rfm_fileAttributeList=g_list_prepend(rfm_fileAttributeList, fileAttributes);
      g_debug("appended into rfm_fileAttributeList:%s", (char*)name->data);
    }
    name=g_list_next(name);
    i++;
  }
  rfm_fileAttributeList=g_list_reverse(rfm_fileAttributeList);

  //We can load thumbnail and gitCommitMsg one by one right after one row of store inserted, as we do in read_one_DirItem_into_fileAttributeList_and_insert_into_store_in_each_cal. But since for pipeline, we have the paging mechanism, we won't have too much rows loaded in a batch, so, we keep the old way of loading thumbnails and gitCommitMsg in batch here. 
  Iterate_through_fileAttribute_list_to_insert_into_store();
  if (rfm_do_thumbs == 1 && g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR))
    iterate_through_store_to_load_thumbnails_or_enqueue_thumbQueue_and_load_gitCommitMsg_ifdef_GitIntegration();

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

   return rv;
}

static void set_Titles(gchar * title){
   gtk_window_set_title(GTK_WINDOW(window), title);
   gtk_tool_button_set_label(PathAndRepositoryNameDisplay, title);
   if (startWithVT() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)) set_terminal_window_title(title);
   g_free(title);
}

static void refresh_store(RFM_ctx *rfmCtx)
{
   In_refresh_store = TRUE;
   //clone filepath_lists_for_selection_on_view
   g_list_free(filepath_lists_for_selection_on_view_clone);
   filepath_lists_for_selection_on_view_clone=NULL;
   GList * selection_filepath_list = g_list_first(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]);
   while (selection_filepath_list!=NULL) {
     filepath_lists_for_selection_on_view_clone = g_list_prepend(filepath_lists_for_selection_on_view_clone, selection_filepath_list->data);
     selection_filepath_list=g_list_next(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]);
   }
   if (RFM_AUTOSELECT_OLDPWD_IN_VIEW && rfm_prePath!=NULL)
     filepath_lists_for_selection_on_view_clone = g_list_prepend(filepath_lists_for_selection_on_view_clone, rfm_prePath);
   
   gtk_widget_hide(rfm_main_box);
   if (scroll_window) gtk_widget_destroy(scroll_window);
  
   rfm_stop_all(rfmCtx);
   gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, FALSE);
   clear_store();

#ifdef GitIntegration
   if (curPath_is_git_repo) load_GitTrackedFiles_into_HashTable();
#endif

   icon_or_tree_view = add_view(rfmCtx);
   
   gchar * title;
   if (SearchResultViewInsteadOfDirectoryView) {
     title=g_strdup_printf(PipeTitle, currentFileNum,fileNum,PageSize_SearchResultView);
     fill_fileAttributeList_with_filenames_from_search_result_and_then_insert_into_store();
     In_refresh_store=FALSE;
     gtk_widget_set_sensitive(PathAndRepositoryNameDisplay, TRUE);
   } else {
     gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), sort_func, NULL, NULL);
     gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), rfmCtx->rfm_sortColumn, GTK_SORT_ASCENDING);
     GDir *dir=NULL;
     dir=g_dir_open(rfm_curPath, 0, NULL);
     if (!dir) return;
     title=g_strdup(rfm_curPath);
     rfm_readDirSheduler=g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)read_one_DirItem_into_fileAttributeList_and_insert_into_store_in_each_call, dir, (GDestroyNotify)g_dir_close);
  }
#ifdef GitIntegration
   if ((SearchResultViewInsteadOfDirectoryView^1) && curPath_is_git_repo)
      g_spawn_wrapper(git_current_branch_cmd, NULL, RFM_EXEC_OUPUT_READ_BY_PROGRAM, NULL, TRUE, set_window_title_with_git_branch_and_sort_view_with_git_status, NULL);
   else set_Titles(title);
#else
   set_Titles(title);
#endif

   gtk_widget_show_all(window);
   refresh_toolbar();
}


#ifdef GitIntegration
static void set_curPath_is_git_repo(gpointer *child_attribs)
{
  char *child_StdOut=((RFM_ChildAttribs *)child_attribs)->stdOut;
  if(child_StdOut!=NULL && strcmp(child_StdOut, "true\n")==0){
    curPath_is_git_repo=TRUE;
  }else{
    curPath_is_git_repo=FALSE;
  }
  
  g_debug("curPath_is_git_repo:%d",curPath_is_git_repo);
}

static char cmd_to_set_terminal_title[PATH_MAX];
//echo -en "\033]0;title\a"
static void set_terminal_window_title(char * title)
{
  sprintf(cmd_to_set_terminal_title, "echo -en \"\\033]0;%s\\a\"", title);
  char* cmd[]={ "bash", "-c", cmd_to_set_terminal_title,NULL};
  g_spawn_sync(rfm_curPath, cmd, NULL,
                           G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN |
                               G_SPAWN_CHILD_INHERITS_STDOUT |
                               G_SPAWN_CHILD_INHERITS_STDERR,
	       NULL, NULL, NULL, NULL, NULL, NULL);
}

static void set_window_title_with_git_branch_and_sort_view_with_git_status(gpointer *child_attribs) {
  char *child_StdOut=((RFM_ChildAttribs *)child_attribs)->stdOut;
  if(child_StdOut!=NULL) {
    child_StdOut[strcspn(child_StdOut, "\n")] = 0;
    g_debug("git current branch:%d",child_StdOut);
    gchar * title=g_strdup_printf("%s [%s]",rfm_curPath,child_StdOut);
    set_Titles(title);
  }else{
    g_warning("failed to get git current branch!");
  }

  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), COL_GIT_STATUS_STR, GTK_SORT_DESCENDING);
}
#endif


static void set_rfm_curPath_internal(gchar* path){
  //https://blog.csdn.net/CaspianSea/article/details/78817778
       if (rfm_curPath != path) {
	 if (rfm_curPath!=NULL){
	   g_free(rfm_prePath);
	   rfm_prePath=rfm_curPath;
	   setenv("OLDPWD", rfm_prePath, 1);
	 }
         rfm_curPath = g_strdup(path);
	 chdir(rfm_curPath);//TODO: i don't know whether we really need this

         //g_setenv("PWD", rfm_curPath, 1);
	 //TODO: i am not sure wether i shold set PWD env value here too, and remove the builtin command setpwd
   //when we run commands such as echo $PWD, we spawn child process with working directory rfm_curPath, so, even we do not set $PWD, we get result as expected with this kind of commands.
   //so, we should first know the relationship between working directory parameter in g_spawn and PWD env value inheritence before we do anything furthur here.
   //anyway, setting OLDPWD is necessary for us here.
      }  
}

/* caller owns path, free(rfm_prePath), rfm_curPath assigned to rfm_prePath; strdup(path) will be assigned to rfm_curPath,*/
static void set_rfm_curPath(gchar* path)
{
   char *msg;
   int rfm_new_wd;
   //int e;

   if (path==NULL) return;
   add_history(g_strconcat("cd ", path, NULL));
   history_entry_added++;
   //TODO: in future if we need some directory specific history file, we may check that if the directory contains .rfm_history file, it will use this local history, otherwise, use the default global history file.
   /* if (rfm_curPath!=NULL && (e=append_history(history_entry_added, g_build_filename(rfm_curPath,".rfm_history", NULL)))) */
   /*     g_warning("failed to append_history(%d,%s) error code:%d",history_entry_added,g_build_filename(rfm_curPath,".rfm_history", NULL),e); */
   if (SearchResultViewInsteadOfDirectoryView) {
     set_rfm_curPath_internal(path);
       //inotify_rm_watch(rfm_inotify_fd, rfm_curPath_wd);
   }else{

     rfm_new_wd=inotify_add_watch(rfm_inotify_fd, path, INOTIFY_MASK);
     if (rfm_new_wd < 0) {
       g_warning("set_rfm_curPath(): inotify_add_watch() failed for %s",path);
       msg=g_strdup_printf("%s:\n\nCan't enter directory!", path);
       show_msgbox(msg, "Warning", GTK_MESSAGE_WARNING);
       g_free(msg);
     } else {
       set_rfm_curPath_internal(path);
       
       // inotify_rm_watch will trigger refresh_store in inotify_handler
       // and it will destory and recreate view base on conditions such as whether curPath_is_git_repo
       inotify_rm_watch(rfm_inotify_fd, rfm_curPath_wd);
       rfm_curPath_wd = rfm_new_wd;
       refresh_store(rfmCtx);
     }
   }

   /* clear_history(); */
   /* if (e=read_history(g_build_filename(rfm_curPath,".rfm_history", NULL))) */
   /*     g_warning("failed to read_history(%s) error code:%d. it's normal if you enter this directory for the first time with rfm.",g_build_filename(rfm_curPath,".rfm_history", NULL),e); */
   /* history_entry_added=0; */
#ifdef GitIntegration
   g_spawn_wrapper(git_inside_work_tree_cmd, NULL, RFM_EXEC_OUPUT_READ_BY_PROGRAM, NULL, FALSE, set_curPath_is_git_repo, NULL);
   /* if (!rfmReadFileNamesFromPipeStdIn && curPath_is_git_repo) */
   /*    g_spawn_wrapper(git_current_branch_cmd, NULL, 0, RFM_EXEC_OUPUT_READ_BY_PROGRAM, NULL, TRUE, set_window_title_with_git_branch, NULL); */
#endif
}


/*why i maintain this ItemSelected value here instead of getting it on need?*/
/*because it is used in the readline thread, and i don't think the get_view_selection_list is thread safe*/
static void selectionChanged(GtkWidget *view, gpointer user_data)
{
  GList *newSelectionFilePathList=NULL;
  GList *selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  if (selectionList==NULL){
    ItemSelected=0;
  }else{
    GtkTreeIter iter;
    selectionList = g_list_first(selectionList);
    while(selectionList!=NULL){
	GValue fullpath = G_VALUE_INIT;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, selectionList->data);
	gtk_tree_model_get_value(treemodel, &iter, COL_FULL_PATH, &fullpath);
	newSelectionFilePathList = g_list_prepend(newSelectionFilePathList, g_strdup(g_value_get_string(&fullpath)));
	g_debug("selected file before refresh:%s",(char *)newSelectionFilePathList->data);
	selectionList=g_list_next(selectionList);
    }
    g_mutex_lock(&rfm_selection_completion_lock);
    free(rfm_selection_completion);
    rfm_selection_completion = g_strdup(newSelectionFilePathList->data);
    ItemSelected=1;
    g_mutex_unlock(&rfm_selection_completion_lock);
  }

  g_list_free_full(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_free);
  filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = newSelectionFilePathList;

  g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
}

static void item_activated(GtkWidget *icon_view, GtkTreePath *tree_path, gpointer user_data)
{
   GtkTreeIter iter;
   long int i;
   long int r_idx=-1;
   gchar *msg;
   GList *file_list=NULL;
   RFM_FileAttributes *fileAttributes;

   gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, tree_path);
   gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);

   file_list=g_list_append(file_list, g_strdup(fileAttributes->path));

   if (!fileAttributes->is_dir) {

      for(i=0; i<G_N_ELEMENTS(run_actions); i++) {
         if (strcmp(fileAttributes->mime_root, run_actions[i].runRoot)==0) {
            if (strcmp(fileAttributes->mime_sub_type, run_actions[i].runSub)==0 || strncmp("*", run_actions[i].runSub, 1)==0) { /* Exact match */
               r_idx=i;
               break;
            }
         }
      }

      if (r_idx != -1)
         g_spawn_wrapper(run_actions[r_idx].runCmd, file_list, G_SPAWN_DEFAULT, NULL,TRUE,NULL,NULL);
      else {
         msg=g_strdup_printf("No run action defined for mime type:\n %s/%s\n", fileAttributes->mime_root, fileAttributes->mime_sub_type);
         show_msgbox(msg, "Run Action", GTK_MESSAGE_INFO);
         g_free(msg);
      }
   }
   else /* If dir, reset rfm_curPath and fill the model */
      set_rfm_curPath(fileAttributes->path);

   g_list_foreach(file_list, (GFunc)g_free, NULL);
   g_list_free(file_list);
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

static void up_clicked(gpointer user_data)
{
   gchar *dir_name;
   dir_name=g_path_get_dirname(rfm_curPath);
   set_rfm_curPath(dir_name);
   g_free(dir_name);
}

static void home_clicked(gpointer user_data)
{
   set_rfm_curPath(rfm_homePath);
}

static void info_clicked(gpointer user_data)
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


static void exec_user_tool(GtkToolItem *item, RFM_ChildAttribs *childAttribs)
{
   if (childAttribs->RunCmd==NULL) /* Argument is an internal function */
     childAttribs->customCallBackFunc(childAttribs->customCallbackUserData);
   else { /* Assume argument is a shell exec */
     RFM_ChildAttribs *child_attribs=calloc(1, sizeof(RFM_ChildAttribs));
     child_attribs->customCallBackFunc=childAttribs->customCallBackFunc;
     child_attribs->customCallbackUserData=childAttribs->customCallbackUserData;
     child_attribs->runOpts=childAttribs->runOpts;
     child_attribs->RunCmd = childAttribs->RunCmd;
     child_attribs->stdOut = NULL;
     child_attribs->stdErr = NULL;
     child_attribs->spawn_async = childAttribs->spawn_async;
     child_attribs->name=g_strdup(childAttribs->name);

     g_spawn_wrapper_(NULL, NULL, child_attribs);
   }
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


static void g_spawn_wrapper_for_selected_fileList_(RFM_ChildAttribs *childAttribs)
{
   GtkTreeIter iter;
   GList *listElement;
   GList *actionFileList=NULL;
   GList *selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
   RFM_FileAttributes *fileAttributes;

   if (selectionList!=NULL) {
      listElement=g_list_first(selectionList);

      while(listElement!=NULL) {
         gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
         gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
         actionFileList=g_list_append(actionFileList, fileAttributes->path);
         listElement=g_list_next(listElement);
      }
      g_spawn_wrapper_(actionFileList,NULL,childAttribs);
      g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
      g_list_free(actionFileList); /* Do not free list elements: owned by GList rfm_fileAttributeList */
   }
}

static void file_menu_exec(GtkMenuItem *menuitem, RFM_ChildAttribs *childAttribs)
{ 
  RFM_ChildAttribs *child_attribs=calloc(1, sizeof(RFM_ChildAttribs));
  child_attribs->customCallBackFunc=childAttribs->customCallBackFunc;
  child_attribs->customCallbackUserData=childAttribs->customCallbackUserData;
  child_attribs->runOpts=childAttribs->runOpts;
  child_attribs->RunCmd = childAttribs->RunCmd;
  child_attribs->stdOut = NULL;
  child_attribs->stdErr = NULL;
  child_attribs->spawn_async = childAttribs->spawn_async;
  child_attribs->name=g_strdup(childAttribs->name);
	
  g_spawn_wrapper_for_selected_fileList_(child_attribs);
}


/* Every action defined in config.h is added to the menu here; each menu item is associated with
 * the corresponding index in the run_actions_array.
 * Items are shown or hidden in popup_file_menu() depending on the selected files mime types.
 */
static RFM_fileMenu *setup_file_menu(RFM_ctx * rfmCtx){
   gint i;
   RFM_fileMenu *fileMenu=NULL;

   if(!(fileMenu = calloc(1, sizeof(RFM_fileMenu))))
      return NULL;
   if(!(fileMenu->action = calloc(G_N_ELEMENTS(run_actions), sizeof(fileMenu->action)))) {
      free(fileMenu);
      return NULL;
   }
   fileMenu->menu=gtk_menu_new();

   for(i=0; i<G_N_ELEMENTS(run_actions); i++) {
      fileMenu->action[i]=gtk_menu_item_new_with_label(run_actions[i].runName);
      //gtk_widget_show(fileMenu->action[i]);
      gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu->menu), fileMenu->action[i]);

      RFM_ChildAttribs *child_attribs = calloc(1,sizeof(RFM_ChildAttribs));
      // this child_attribs will be freed by g_spawn_wrapper call tree if menuitem clicked,
      // and it will be copied to a new instance in file_menu_exec.
      // but if menuitem not clicked? currently, setup_file_menu won't be called many times, so, it will be freed after application quit, no need to free manually.
      child_attribs->RunCmd = run_actions[i].runCmd;      
      child_attribs->runOpts = G_SPAWN_DEFAULT;
      child_attribs->stdOut = NULL;
      child_attribs->stdErr = NULL;
      child_attribs->spawn_async = TRUE;
      child_attribs->name = g_strdup(run_actions[i].runName);
      if ((g_strcmp0(child_attribs->name, RunActionGitStage)==0) ||
	  (SearchResultViewInsteadOfDirectoryView &&
	       ((g_strcmp0(child_attribs->name, RunActionMove)==0
	       ||g_strcmp0(child_attribs->name, RunActionDelete)==0)))){

	 child_attribs->customCallBackFunc = refresh_store;
         child_attribs->customCallbackUserData = rfmCtx;
      } else {
         child_attribs->customCallBackFunc = NULL;
	 child_attribs->customCallbackUserData = NULL;
      }

      g_signal_connect(fileMenu->action[i], "activate", G_CALLBACK(file_menu_exec), child_attribs);
   }
   return fileMenu;
}

/* Generate a filemenu based on mime type of the files selected and show
 * If all files are the same type selection_mimeRoot and selection_mimeSub are set
 * If all files are the same mime root but different sub, then selection_mimeSub is set to NULL
 * If all the files are completely different types, both root and sub types are set to NULL
 */
static gboolean popup_file_menu(GdkEvent *event, RFM_ctx *rfmCtx)
{
   int showMenuItem[G_N_ELEMENTS(run_actions)]={0};
   GtkTreeIter iter;
   GList *selectionList;
   GList *listElement;
   int i;
   RFM_fileMenu *fileMenu=g_object_get_data(G_OBJECT(window),"rfm_file_menu");
   RFM_FileAttributes *selection_fileAttributes, *fileAttributes;
   gboolean match_mimeRoot=TRUE, match_mimeSub=TRUE;

   selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
   if (selectionList==NULL)
      return FALSE;

   listElement=g_list_first(selectionList);
   if (listElement!=NULL) {   /* Get file attributes for first item in the list */
      gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &selection_fileAttributes, -1);
      listElement=g_list_next(listElement);
   }
   while(listElement!=NULL) { /* Compare first item with any other selected items */
      gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
      if (strcmp(selection_fileAttributes->mime_root, fileAttributes->mime_root)!=0) {
         match_mimeRoot=FALSE;
         break;   /* Finish here: files are different */
      }
      if (strcmp(selection_fileAttributes->mime_sub_type, fileAttributes->mime_sub_type)!=0)
         match_mimeSub=FALSE;

      listElement=g_list_next(listElement);
   }

   if (rfmCtx->showMimeType==1)
      g_info("%s: %s-%s", selection_fileAttributes->file_name, selection_fileAttributes->mime_root, selection_fileAttributes->mime_sub_type);

   for(i=0; i<G_N_ELEMENTS(run_actions); i++) {
      if (match_mimeSub) { /* All selected files are the same type */
         if (strcmp(selection_fileAttributes->mime_root, run_actions[i].runRoot)==0 && strcmp(selection_fileAttributes->mime_sub_type, run_actions[i].runSub)==0)
            showMenuItem[i]++; /* Exact match */
      }
      if (match_mimeRoot) {
         if (strcmp(selection_fileAttributes->mime_root, run_actions[i].runRoot)==0 && strncmp("*", run_actions[i].runSub, 1)==0)
            showMenuItem[i]++; /* Selected files have the same mime root type */
      }
      if (strncmp("*", run_actions[i].runRoot, 1)==0)
         showMenuItem[i]++;   /* Run actions to show for all files */
   }

   for(i=0; i<G_N_ELEMENTS(run_actions); i++) {
      if (showMenuItem[i]>0 && (run_actions[i].showCondition == NULL || run_actions[i].showCondition()))
         gtk_widget_show(fileMenu->action[i]);
      else
         gtk_widget_hide(fileMenu->action[i]);
   }

   gtk_menu_popup_at_pointer(GTK_MENU(fileMenu->menu), event);
   g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
   return TRUE;
}


static gboolean view_key_press(GtkWidget *widget, GdkEvent *event,RFM_ctx *rfmCtx) {
  GdkEventKey *ek=(GdkEventKey *)event;
  if (ek->keyval==GDK_KEY_Menu)
    return popup_file_menu(event, rfmCtx);
  else if (ek->keyval==GDK_KEY_q){
    cleanup(NULL,rfmCtx);
    return TRUE;
  } else if (ek->keyval == GDK_KEY_Escape) {
    if(treeview)
      gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(icon_or_tree_view)));
    else
      gtk_icon_view_unselect_all(GTK_ICON_VIEW(icon_or_tree_view));
    return TRUE;
  }  else
    return FALSE;
}


static gboolean view_button_press(GtkWidget *widget, GdkEvent *event, RFM_ctx *rfmCtx)
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
            popup_file_menu(event, rfmCtx);
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
   gtk_widget_show(PathAndRepositoryNameDisplay);
   for (guint i = 0; i < G_N_ELEMENTS(tool_buttons); i++) {
     if ((SearchResultViewInsteadOfDirectoryView && tool_buttons[i].showInSearchResultView) || (!SearchResultViewInsteadOfDirectoryView && tool_buttons[i].showInDirectoryView)){
       if (tool_buttons[i].showCondition == NULL || tool_buttons[i].showCondition())
	 gtk_widget_show(tool_bar->buttons[i]);
       else
	 gtk_widget_hide(tool_bar->buttons[i]);
     }else
       gtk_widget_hide(tool_bar->buttons[i]);
   }
}

static void add_toolbar(GtkWidget *rfm_main_box, RFM_defaultPixbufs *defaultPixbufs, RFM_ctx *rfmCtx)
{
   GtkWidget *buttonImage=NULL; /* Temp store for button image; gtk_tool_button_new() appears to take the reference */

   if(!(tool_bar = calloc(1, sizeof(RFM_toolbar))))
      return;
   if(!(tool_bar->buttons = calloc(G_N_ELEMENTS(tool_buttons), sizeof(tool_bar->buttons)))) {
      free(tool_bar);
      return;
   }

   tool_bar->toolbar=gtk_toolbar_new();
   gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar->toolbar), GTK_TOOLBAR_ICONS);
   gtk_box_pack_start(GTK_BOX(rfm_main_box), tool_bar->toolbar, FALSE, FALSE, 0);

   if (!agMain) agMain = gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(window), agMain);

   //we add the following toolbutton here instead of define in config.def.h because we need its global reference.
   PathAndRepositoryNameDisplay = gtk_tool_button_new(NULL,rfm_curPath);
   gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->toolbar), PathAndRepositoryNameDisplay, -1);
   g_signal_connect(PathAndRepositoryNameDisplay, "clicked", G_CALLBACK(Switch_SearchResultView_DirectoryView),rfmCtx);
   
   for (guint i = 0; i < G_N_ELEMENTS(tool_buttons); i++) {
     //if ((rfmReadFileNamesFromPipeStdIn && tool_buttons[i].readFromPipe) || (!rfmReadFileNamesFromPipeStdIn && tool_buttons[i].curPath)){
       GdkPixbuf *buttonIcon=NULL;
       if (tool_buttons[i].buttonIcon!=NULL) buttonIcon=gtk_icon_theme_load_icon(icon_theme, tool_buttons[i].buttonIcon, RFM_TOOL_SIZE, 0, NULL);
       if (buttonIcon==NULL)
	 buttonImage=NULL;
       else{
	 buttonImage=gtk_image_new_from_pixbuf(buttonIcon);
	 g_object_unref(buttonIcon);
       }

       tool_bar->buttons[i]=gtk_tool_button_new(buttonImage, tool_buttons[i].buttonName);
       gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->toolbar), tool_bar->buttons[i], -1);

       if(tool_buttons[i].Accel) gtk_widget_add_accelerator(GTK_WIDGET(tool_bar->buttons[i]), "clicked", agMain,tool_buttons[i].Accel, MOD_KEY, GTK_ACCEL_VISIBLE);
       if(tool_buttons[i].tooltip!=NULL) gtk_tool_item_set_tooltip_text(tool_bar->buttons[i],tool_buttons[i].tooltip);

      RFM_ChildAttribs *child_attribs = calloc(1,sizeof(RFM_ChildAttribs));
      child_attribs->RunCmd = tool_buttons[i].RunCmd;
      child_attribs->runOpts = G_SPAWN_DEFAULT;
      child_attribs->stdOut = NULL;
      child_attribs->stdErr = NULL;
      child_attribs->spawn_async = TRUE;
      child_attribs->name = g_strdup(tool_buttons[i].buttonName);
      child_attribs->customCallBackFunc = tool_buttons[i].func;
      child_attribs->customCallbackUserData = rfmCtx;

      g_signal_connect(tool_bar->buttons[i], "clicked", G_CALLBACK(exec_user_tool),child_attribs);

      //}
   }

}

static GtkWidget *add_view(RFM_ctx *rfmCtx)
{
   GtkWidget *_view;

   scroll_window=gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start(GTK_BOX(rfm_main_box), scroll_window, TRUE, TRUE, 0);

   if (treeview) {
     _view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
     GtkCellRenderer  * renderer  =  gtk_cell_renderer_text_new();
     gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(_view)),GTK_SELECTION_MULTIPLE);

     for(guint i=0; i<G_N_ELEMENTS(treeviewColumns); i++){
       if (!treeviewColumns[i].Show) continue;
       treeviewColumns[i].gtkCol = gtk_tree_view_column_new_with_attributes(treeviewColumns[i].title , renderer,"text" ,  treeviewColumns[i].enumCol , NULL);
       gtk_tree_view_column_set_resizable(treeviewColumns[i].gtkCol,TRUE);
       gtk_tree_view_append_column(GTK_TREE_VIEW(_view),treeviewColumns[i].gtkCol);
       gtk_tree_view_column_set_visible(treeviewColumns[i].gtkCol, treeviewColumns[i].Show && ((treeviewColumns[i].showCondition==NULL) ? TRUE: treeviewColumns[i].showCondition(NULL)));
       gtk_tree_view_column_set_sort_column_id(treeviewColumns[i].gtkCol, treeviewColumns[i].enumSortCol==NULL? treeviewColumns[i].enumCol: treeviewColumns[i].enumSortCol);
     }

   } else {
     _view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(store));
     gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(_view),GTK_SELECTION_MULTIPLE);
     gtk_icon_view_set_markup_column(GTK_ICON_VIEW(_view),COL_ICONVIEW_MARKUP);
     gtk_icon_view_set_tooltip_column(GTK_ICON_VIEW(_view), COL_ICONVIEW_TOOLTIP);
     gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(_view), COL_PIXBUF);
   }
   #ifdef RFM_SINGLE_CLICK
   gtk_icon_view_set_activate_on_single_click(GTK_ICON_VIEW(icon_view), TRUE);
   #endif
   g_signal_connect(_view, "button-press-event", G_CALLBACK(view_button_press), rfmCtx);
   g_signal_connect(_view, "key-press-event", G_CALLBACK(view_key_press), rfmCtx);

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
     g_debug("gtk_icon_view_get_column_spacing:%d",gtk_icon_view_get_column_spacing((GtkIconView *)_view));
     g_debug("gtk_icon_view_get_item_padding:%d", gtk_icon_view_get_item_padding((GtkIconView *)_view));
     g_debug("gtk_icon_view_get_spacing:%d", gtk_icon_view_get_spacing((GtkIconView *)_view));
     g_debug("gtk_icon_view_get_item_width:%d", gtk_icon_view_get_item_width((GtkIconView *)_view));
     g_debug("gtk_icon_view_get_margin:%d", gtk_icon_view_get_margin((GtkIconView *)_view));
   }
   
   return _view;
}

static void switch_iconview_treeview(RFM_ctx *rfmCtx) {
  GList *  selectionList=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
  gtk_widget_hide(rfm_main_box);
  gtk_widget_destroy(scroll_window);
  treeview=!treeview;
  icon_or_tree_view=add_view(rfmCtx);
  gtk_widget_show_all(window);
  refresh_toolbar();
  set_view_selection_list(icon_or_tree_view, treeview, selectionList);
}

static void Switch_SearchResultView_DirectoryView(GtkToolItem *item,RFM_ctx *rfmCtx)
{
    SearchResultViewInsteadOfDirectoryView=SearchResultViewInsteadOfDirectoryView^1;
    refresh_store(rfmCtx);
}

static void inotify_insert_item(gchar *name, gboolean is_dir)
{
   RFM_defaultPixbufs *defaultPixbufs=g_object_get_data(G_OBJECT(window),"rfm_default_pixbufs");
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
		       COL_ICONVIEW_MARKUP, fileAttributes->file_name,
                       -1);
}

static gboolean delayed_refreshAll(gpointer user_data)
{
  refresh_store((RFM_ctx *)user_data);
   return FALSE;
}

static gboolean inotify_handler(gint fd, GIOCondition condition, gpointer user_data)
{
   char buffer[(sizeof(struct inotify_event)+16)*1024];
   int len=0, i=0;
   int refresh_view=0;
   RFM_ctx *rfmCtx=user_data;

   len=read(fd, buffer, sizeof(buffer));
   if (len<0) {
      g_warning("inotify_handler: inotify read error\n");
      return TRUE;
   }

   if (SearchResultViewInsteadOfDirectoryView) return TRUE;
   
   while (i<len) {
      struct inotify_event *event=(struct inotify_event *) (buffer+i);
      
      if (event->len && !ignored_filename(event->name)) {
         if (event->wd==rfm_thumbnail_wd) {
            /* Update thumbnails in the current view */
            if (event->mask & IN_MOVED_TO || event->mask & IN_CREATE) { /* Only update thumbnail move - not interested in temporary files */
              load_thumbnail(event->name);

	      g_debug("thumbnail %s loaded in inotify_handler",event->name);

            }
         }
         else {   /* Must be from rfm_curPath_wd */
            if (event->mask & IN_CREATE)
               inotify_insert_item(event->name, event->mask & IN_ISDIR);
            else
               refresh_view=1; /* Item IN_CLOSE_WRITE, deleted or moved */
         }
      }
      // if (event->mask & IN_IGNORED) /* Watch changed i.e. rfm_curPath changed */
      // refresh_view=2; //do not call refresh in handler here, call it in set_rfm_curPath.

      if (event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF) {
         show_msgbox("Parent directory deleted!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         //refresh_view=2; since we now call refresh in set_rfm_curPath, don't need to set refresh_view to refresh again.
      }
      if (event->mask & IN_UNMOUNT) {
         show_msgbox("Parent directory unmounted!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         //refresh_view=2;
      }
      if (event->mask & IN_Q_OVERFLOW) {
         show_msgbox("Inotify event queue overflowed!", "Error", GTK_MESSAGE_ERROR);
         set_rfm_curPath(rfm_homePath);
         //refresh_view=2;         
      }
      i+=sizeof(*event)+event->len;
   }
   
   switch (refresh_view) {
      case 1:  /* Delayed refresh: rate-limiter */
         if (rfmCtx->delayedRefresh_GSourceID>0)
            g_source_remove(rfmCtx->delayedRefresh_GSourceID);
         rfmCtx->delayedRefresh_GSourceID=g_timeout_add(RFM_INOTIFY_TIMEOUT, delayed_refreshAll, user_data);
      break;
      case 2: /* Refresh imediately: refresh_store() will remove delayedRefresh_GSourceID if required */
	 g_debug("refresh_store imediately from inotify_handler");
         refresh_store(rfmCtx);
      break;
      default: /* Refresh not required */
      break;
   }
   return TRUE;
}

static gboolean init_inotify(RFM_ctx *rfmCtx)
{
   rfm_inotify_fd = inotify_init();
   if ( rfm_inotify_fd < 0 )
      return FALSE;
   else {
      g_unix_fd_add(rfm_inotify_fd, G_IO_IN, inotify_handler, rfmCtx);
      if (rfm_do_thumbs==1) {
         rfm_thumbnail_wd=inotify_add_watch(rfm_inotify_fd, rfm_thumbDir, IN_MOVE|IN_CREATE);
         if (rfm_thumbnail_wd < 0) g_warning("init_inotify: Failed to add watch on dir %s\n", rfm_thumbDir);
      }
   }
   return TRUE;
}

static gboolean mounts_handler(GUnixMountMonitor *monitor, gpointer rfmCtx)
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
      refresh_store((RFM_ctx*)rfmCtx);

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
   g_free(defaultPixbufs);
}

static gboolean ToSearchResultFilenameList=FALSE; // we need to pass this status from exec_stdin_command to readlineInSeperatedThread, however, we can only pass one parameter in g_thread_new, so, i use a global variable here.

static void readlineInSeperateThread(GString * readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution) {
  if (auto_execution_command_after_rfm_start!=NULL){
    //auto_execution_command_after_rfm_start was freed in exec_stdin_command as readlineresult
    auto_execution_command_after_rfm_start = NULL;
  }
  
  if (readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution!=NULL){ //this means it's not initial run after rfm start, so i should first run cmd from previous readline here
	  GError *err = NULL;
	  gchar* cmd_stdout;
	  if (ToSearchResultFilenameList && g_spawn_sync(rfm_curPath, 
					      stdin_cmd_interpretors[current_stdin_cmd_interpretor].cmdTransformer(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str),
					      NULL,
					      G_SPAWN_SEARCH_PATH|G_SPAWN_CHILD_INHERITS_STDIN|G_SPAWN_CHILD_INHERITS_STDERR,
					      NULL,NULL,&cmd_stdout,NULL,NULL,&err)){ //remove the ending ">0" in cmd with g_string_erase
	      g_idle_add_once(update_SearchResultFileNameList_and_refresh_store, (gpointer)cmd_stdout);
	  } else if (!ToSearchResultFilenameList && g_spawn_sync(rfm_curPath,
			   stdin_cmd_interpretors[current_stdin_cmd_interpretor].cmdTransformer(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str), NULL,
                           G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN |
                               G_SPAWN_CHILD_INHERITS_STDOUT |
                               G_SPAWN_CHILD_INHERITS_STDERR,
			   NULL, NULL, NULL, NULL, NULL, &err)) {
	  } else {
              g_warning("%d;%s", err->code, err->message);
	      g_error_free(err);
	  }
	  add_history(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution->str);
	  history_entry_added++;
	  if (OriginalReadlineResult!=NULL){ //with rfm -x , Originalreadlineresult can be null here
	    add_history(OriginalReadlineResult);
	    history_entry_added++;
	  }
          g_string_free(readlineResultStringFromPreviousReadlineCall_AfterFilenameSubstitution,TRUE);
  }

  if(startWithVT() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)){
    gchar prompt[5]="";
    strcat(prompt, stdin_cmd_interpretors[current_stdin_cmd_interpretor].prompt);
    ToSearchResultFilenameList=FALSE;
    if (keep_selection_on_view_across_refresh && In_refresh_store) strcat(prompt,"?>");
    else if (ItemSelected==0) strcat(prompt,">");
    else strcat(prompt,"*>");
    g_free(OriginalReadlineResult);
    while ((OriginalReadlineResult = readline(prompt))==NULL);

    stdin_command_Scheduler = g_idle_add_once(exec_stdin_command, strdup(OriginalReadlineResult));
  }
}

static void stdin_command_help() {
	  printf("%s",builtinCMD_Help);
	  for(int i=0;i<G_N_ELEMENTS(builtinCMD);i++){
	  	printf("    %s   %s\n", builtinCMD[i].cmd, builtinCMD[i].help_msg);
	  }
	  for(int i=0;i<G_N_ELEMENTS(stdin_cmd_interpretors);i++){
	        printf("    %s   %s\n", stdin_cmd_interpretors[i].activationKey, stdin_cmd_interpretors[i].name);
	  }
}


static int GetColumnIndexByEnun(enum RFM_treeviewCol col){
  for(guint i=0;i<G_N_ELEMENTS(treeviewColumns);i++){
    if (treeviewColumns[i].enumCol==col) return i;
  }
  return -1;
}

static RFM_treeviewColumn* GetColumnByEnun(enum RFM_treeviewCol col){
  int i = GetColumnIndexByEnun(col);
  return i<0 ? NULL : &treeviewColumns[i];
}

void move_array_item_a_after_b(void * array, int index_b, int index_a, uint32_t array_item_size, uint32_t array_length){
                void* temp_array = malloc(array_item_size * array_length);
  
                if ((index_b+1)>index_a){ //第一,第二种情况  比如 2,1 满足 2+1>1
			for(guint k=0;k<index_a;k++){   // 0 <- 0    
			  memcpy(temp_array+k*array_item_size, array + k*array_item_size, array_item_size);
			  g_debug("%d <- %d",k,k);
			}
			//k=1=col_index
			for(guint k=index_a+1;k<index_b+1;k++){  // 1 <- 2
			  memcpy(temp_array+(k-1)*array_item_size, array+k*array_item_size, array_item_size);
			  g_debug("%d <- %d",k-1,k);
			}
			//上面最后一次memcpy,treeviewColumn[baseColumnIndex]也已经复制到了 temptreeviewcolumns[basecolumnindex-1]
			memcpy(temp_array+index_b*array_item_size, array+index_a*array_item_size, array_item_size); // 2 <- 1
			g_debug("%d <- %d",index_b,index_a);
			for(guint k=index_b+1; k<array_length;k++){
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size);
			  g_debug("%d <- %d",k,k);
			}
 		}else{//第四种情况, 比如 1,3 满足 1+1 < 3
			for(guint k=0;k<index_b+1;k++){ // 0 <- 0   1 <- 1
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size);
			  g_debug("%d <- %d",k,k);
			}
			memcpy(temp_array+(index_b+1)*array_item_size, array+index_a*array_item_size, array_item_size); // 2 <- 3
			g_debug("%d <- %d",index_b+1,index_a);
			for(guint k=index_b+1;k<index_a;k++){
			  memcpy(temp_array+(k+1)*array_item_size, array+k*array_item_size, array_item_size); // 3 <- 2
			  g_debug("%d <- %d",k+1,k);
			}
			for(guint k=index_a+1;k<array_length;k++){
			  memcpy(temp_array+k*array_item_size, array+k*array_item_size, array_item_size); // 4 <- 4
			  g_debug("%d <- %d",k,k);
			}
		}
		for(guint k=0;k<array_length;k++) //I think i cannot free treeviewcolumns because of the way it's defined in config.h, so i have to copy temptreeviewcolumns back. we may define treeviewcolumns[2,] and use treeviewcolumns[0,] treeviewcolumns[1,] in turn to eliminate this copy. But anyway, not big deal, current way is more readable.
			memcpy(array+k*array_item_size, temp_array+k*array_item_size, array_item_size);

		free(temp_array);

}

static void show_hide_treeview_columns_in_order(gchar* order_sequence) {
	      gchar** order_seq_array = g_strsplit_set(order_sequence, ",;", G_N_ELEMENTS(treeviewColumns));
	      guint j=0;
	      int baseColumnIndex=-1;
	      do {
		if (g_strcmp0(order_seq_array[j], "")!=0){ //to deal with situation such as ,2 (or 2,)
		    int col_enum_with_sign = atoi(order_seq_array[j]);
		    guint col_enum = abs(col_enum_with_sign);
		    int col_index = GetColumnIndexByEnun(col_enum);
		    g_debug("col_index:%d  col_enum:%d",col_index,col_enum);
                    g_free(order_seq_array[j]);
		    if (col_index<0) {
		      g_warning("cannot find column %d. Memory after columnReorderRation[j] may not released, no big deal.",col_enum);
		      g_free(order_seq_array);
		      return;
		    }

		    treeviewColumns[col_index].Show = (col_enum_with_sign>=0);
		    if (treeviewColumns[col_index].gtkCol!=NULL){
		      if (treeview) {
			gtk_tree_view_column_set_visible(treeviewColumns[col_index].gtkCol,treeviewColumns[col_index].Show);
			if (j>=1) gtk_tree_view_move_column_after(GTK_TREE_VIEW(icon_or_tree_view) , treeviewColumns[col_index].gtkCol, baseColumnIndex<0? NULL:treeviewColumns[baseColumnIndex].gtkCol);
		      }
		    }else if (treeviewColumns[col_index].Show && order_sequence!=treeviewcolumn_init_order_sequence)
		      printf(VALUE_MAY_NOT_LOADED,col_enum,treeviewColumns[col_index].title);

		//reorganize the treeviewColumns array, otherwise, display order will restore to default after refresh
		    
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
		    if (j>=1 && (baseColumnIndex+1!=col_index)){
		      g_debug("baseColumnIndex+1 <- col_index: %d <- %d",baseColumnIndex+1,col_index);
		      move_array_item_a_after_b(treeviewColumns, baseColumnIndex, col_index, sizeof(RFM_treeviewColumn), G_N_ELEMENTS(treeviewColumns));
		    }
		  
		    if (j>=1) baseColumnIndex = baseColumnIndex + 1;
		    else baseColumnIndex = col_index; 
		    g_debug("baseColumnIndex:%d",baseColumnIndex);
                } else if (order_seq_array[j+1]==NULL) { // (g_strcmp0(order_seq_array[j],"")==0) and the last elements in case 2,
                    for (guint i = baseColumnIndex + 1;
                         i < G_N_ELEMENTS(treeviewColumns); i++) {
		      treeviewColumns[i].Show = FALSE;
		      if (treeviewColumns[i].gtkCol!=NULL && treeview) gtk_tree_view_column_set_visible(treeviewColumns[i].gtkCol, FALSE);
		      g_debug("%d invisible after ending ','",treeviewColumns[i].enumCol);
                    }
                }
                j++;
	      } while (order_seq_array[j]!=NULL);
	      g_free(order_seq_array);
}

static void show_hide_treeview_columns(wordexp_t * parsed_msg){
	  if (parsed_msg->we_wordc==1){
	    printf("current column status(negative means invisible):\n");
	    for(guint i=0;i<G_N_ELEMENTS(treeviewColumns);i++)
	      printf("    %d: %s\n",treeviewColumns[i].Show? treeviewColumns[i].enumCol:(-1)*treeviewColumns[i].enumCol,treeviewColumns[i].title);
	    printf(SHOWCOLUMN_USAGE);
	  }else{
            for(guint i=1;i<parsed_msg->we_wordc;i++){
	      show_hide_treeview_columns_in_order(parsed_msg->we_wordv[i]);
	    }
	 }
         return TRUE;
}


static void null_log_handler(const gchar *log_domain,GLogLevelFlags log_level,const gchar *message,gpointer user_data){
}

static gboolean exec_stdin_command_builtin(wordexp_t * parsed_msg, GString* readline_result_string_after_file_name_substitution){

        for(int i=0;i<G_N_ELEMENTS(builtinCMD);i++){
	  if (g_strcmp0(parsed_msg->we_wordv[0], builtinCMD[i].cmd)==0) {
		builtinCMD[i].action();
		return TRUE;
	  }
	}

        if (g_strcmp0(parsed_msg->we_wordv[0],"cd")==0){
	  if (parsed_msg->we_wordc==2){
	    gchar * addr=parsed_msg->we_wordv[1];
	    g_debug("cd %s", addr);

	    if (g_strcmp0(addr, "..")==0) {
	      up_clicked(NULL);
	      return TRUE;
	    }else if (g_strcmp0(addr, ".")==0){
	      return TRUE;
	    }else if (g_strcmp0(addr, "-")==0){
	      set_rfm_curPath(getenv("OLDPWD"));
	      return TRUE;
	    }else if (addr[0]=='/'){
	      set_rfm_curPath(addr);
	      return TRUE;
	    }else{
	      set_rfm_curPath(g_build_filename(rfm_curPath, addr, NULL));
	      return TRUE;
	    }
	  }else if (parsed_msg->we_wordc==1){ // cd without parameter
	      if (stdin_cmd_ending_space){
		if (stdin_cmd_selection_list!=NULL && stdin_cmd_selection_fileAttributes!=NULL){
		  //user can accidentally select multple files, but we only use one of them here.
		  if (stdin_cmd_selection_fileAttributes->file_mode_str[0]=='d'){
		    set_rfm_curPath(stdin_cmd_selection_fileAttributes->path);
		    if (SearchResultViewInsteadOfDirectoryView) Switch_SearchResultView_DirectoryView(NULL, rfmCtx);		  
		  }else if (SearchResultViewInsteadOfDirectoryView){
		    SearchResultViewInsteadOfDirectoryView = SearchResultViewInsteadOfDirectoryView^1;
		    g_list_free_full(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],g_free);
		    filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView]=g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView],strdup(stdin_cmd_selection_fileAttributes->path));
		    char * parentdir = g_path_get_dirname(stdin_cmd_selection_fileAttributes->path);
		    set_rfm_curPath(parentdir);
		    g_free(parentdir);
		  }
		}
	      }else { //!stdin_cmd_ending_space
		printf("%s\n",rfm_curPath);
	      }
	      return TRUE;
	  //when we set_rfm_curPath, we don't change rfm environment variable PWD
	  //so, shall we consider update env PWD value for rfm in set_rfm_curPath?
	  //The answer is we should not, since we sometime, with need PWD, which child process will inherit, to be different from the directory of files selected that will be used by child process.
	  //Instead, we use the setpwd command.
	  }
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"setpwd")==0) {
	  setenv("PWD",rfm_curPath,1);
	  printf("%s\n",getenv("PWD"));
	  return TRUE;
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"pwd")==0) {
	  printf("%s\n",getenv("PWD"));
	  return TRUE;
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"quit")==0) {
	  cleanup(NULL,rfmCtx);
	  return TRUE;
        }else if (g_strcmp0(parsed_msg->we_wordv[0],"help")==0) {
	  stdin_command_help();
	  return TRUE;
	}else if (g_strcmp0(parsed_msg->we_wordv[0],"/")==0) {
	  switch_iconview_treeview(rfmCtx);
	  return TRUE;
	}else if (g_strcmp0(parsed_msg->we_wordv[0],"//")==0) {
	  Switch_SearchResultView_DirectoryView(NULL, rfmCtx);
	  return TRUE;
        }else if (g_strcmp0(parsed_msg->we_wordv[0], "pagesize")==0 && parsed_msg->we_wordc==2){
	  guint ps = atoi(parsed_msg->we_wordv[1]); 
	  if (ps > 0) {
	    set_DisplayingPageSize_ForFileNameListFromPipesStdIn(ps);
	    return TRUE;
	  }
	}else if (g_strcmp0(parsed_msg->we_wordv[0], "showcolumn")==0){
	  show_hide_treeview_columns(parsed_msg);
	  add_history(readline_result_string_after_file_name_substitution->str);
	  history_entry_added++;
	  return TRUE;
	}else if (g_strcmp0(parsed_msg->we_wordv[0], "glog")==0){
	  if (parsed_msg->we_wordc>1){
	    if (g_strcmp0(parsed_msg->we_wordv[1],"off")==0)
	      g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	    else if (g_strcmp0(parsed_msg->we_wordv[1],"on")==0)
	      g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, g_log_default_handler, NULL);
	    else printf("Usage: glog off|on\n");
	  }else
	    printf("Usage: glog off|on\n");
	  return TRUE;
	}
	return FALSE;
}

static void exec_stdin_command (gchar * readlineResult)
{
        gint len = strlen(readlineResult);
	g_debug ("readline return length %u: %s", len, readlineResult);
        GString *readlineResultString = NULL;
	if (len == 0){
	    time_t now_time=time(NULL);
            if ((now_time - lastEnter)<=1){
                lastEnter=now_time;
               	refresh_store(rfmCtx);
            }else
                lastEnter=now_time;
	}else{
            for(int i=0;i<G_N_ELEMENTS(stdin_cmd_interpretors);i++){
	      if (g_strcmp0(readlineResult, stdin_cmd_interpretors[i].activationKey)==0){
		current_stdin_cmd_interpretor = i;
		goto switchToReadlineThread;
	      }
	    }

	    stdin_cmd_ending_space = (readlineResult[len-1]==' ');
	    while (readlineResult[len-1]==' ') { readlineResult[len-1]='\0'; len--; } //remove ending space

            if (len > 2 && readlineResult[len-2]=='>' && readlineResult[len-1]=='0'){ //TODO: better way to check ending with ">0"?
		ToSearchResultFilenameList=TRUE;
		readlineResult[len-1]='\0';readlineResult[len-2]='\0';
	    }
            readlineResultString=g_string_new(strdup(readlineResult));
	    if (stdin_cmd_ending_space){
	      // combine runCmd with selected files to get gchar** v
	      // TODO: the following code share the same pattern as g_spawn_wrapper_for_selected_fileList_ , anyway to remove the duplicate code?
	      GtkTreeIter iter;
	      GList *listElement;
	      stdin_cmd_selection_list=get_view_selection_list(icon_or_tree_view,treeview,&treemodel);

	      if (stdin_cmd_selection_list!=NULL) {
		listElement=g_list_first(stdin_cmd_selection_list);
		while(listElement!=NULL) {
		  gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
		  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &stdin_cmd_selection_fileAttributes, -1);

		  //if there is %s in msg, replace it with selected filename one by one, otherwise, append filenames to the end.
		  //TODO: what if userinput need %s literally? how to escape?
		  if (strstr(readlineResultString->str, "%s") == NULL) g_string_append(readlineResultString, " %s ");
		  //if file path contains space, wrap path inside ''
                  if (strstr(stdin_cmd_selection_fileAttributes->path," ") != NULL) g_string_replace(readlineResultString, "%s", "'%s'", 1);
		  g_string_replace(readlineResultString, "%s",stdin_cmd_selection_fileAttributes->path, 1);
	      
		  listElement=g_list_next(listElement);
		}
	      }
	    } //end if (endingspace)

	    
	    wordexp_t parsed_msg;
	    int wordexp_retval = wordexp(readlineResult,&parsed_msg,0);
	    if (wordexp_retval==0 && exec_stdin_command_builtin(&parsed_msg, readlineResultString)){
	      g_string_free(readlineResultString,TRUE);
	      readlineResultString=NULL; //since readlineInseperatethread function will check this, we must clear it here after cmd already been executed.
	    }
	    if (wordexp_retval == 0) wordfree(&parsed_msg);
#ifdef PythonEmbedded
	    if (readlineResultString!=NULL && pyProgramName!=NULL && g_strcmp0(stdin_cmd_interpretors[current_stdin_cmd_interpretor].name,"PythonEmbedded")==0){
	      add_history(readlineResultString->str);
	      history_entry_added++;
	      add_history(OriginalReadlineResult);
	      history_entry_added++;
	      //TODO: investigation add_history, what if Originalreadlineresult equals readlineresultstring?
              readlineResultString = g_string_append(readlineResultString, "\n");
	      PyRun_SimpleString(readlineResultString->str);
	      g_string_free(readlineResultString, TRUE);
	      readlineResultString=NULL;
	    }
#endif
	    if (stdin_cmd_selection_list!=NULL){
	      g_list_free_full(stdin_cmd_selection_list, (GDestroyNotify)gtk_tree_path_free);
	      stdin_cmd_selection_list=NULL;
	    }

	} //end if (len == 0)
 switchToReadlineThread:
        g_free (readlineResult);

	if(startWithVT() || auto_execution_command_after_rfm_start!=NULL){
	  if (auto_execution_command_after_rfm_start==NULL) g_thread_join(readlineThread);
	  readlineThread=g_thread_new("readline", readlineInSeperateThread, readlineResultString);
	}
}

//TODO: TAB completion will insert "fullpath " into readlinem, that is, with ending space. I have to backspace to remove this ending space, otherwise, ending space will be replaced with selected filename again. We need to change the behavior into insert leading space instead of ending space. This may need deeper research into the readline library.
static char** rfm_filename_completion(const char *text, int start, int end){
  if (ItemSelected && (text==NULL || g_strcmp0(text, "")==0)) {
    //rl_attempted_completion_over = 1;
    char** ret = calloc(2,sizeof(char*));
    g_mutex_lock(&rfm_selection_completion_lock);
    ret[0] = strdup(rfm_selection_completion);
    //see the update of rfm_selection_completion and usage of lock, it possible that when ItemSelected=0 here, we read the old rfm_selection_completion value, but i just prevent read just after rfm_selection_completion been freed here.
    g_mutex_unlock(&rfm_selection_completion_lock);
    return ret;
  } else if (OLD_rl_attempted_completion_function!=NULL) return OLD_rl_attempted_completion_function(text,start,end);
}

static int setup(RFM_ctx *rfmCtx)
{
   RFM_fileMenu *fileMenu=NULL;

   gtk_init(NULL, NULL);

   window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(window), 640, 400);

   rfm_main_box=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add(GTK_CONTAINER(window), rfm_main_box);

   rfm_homePath=g_strdup(g_get_home_dir());
   
   rfm_thumbDir=g_build_filename(g_get_user_cache_dir(), "thumbnails", "normal", NULL);

   thumb_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)gtk_tree_row_reference_free);

#ifdef GitIntegration
   gitTrackedFiles=g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
   //gitCommitMsg=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
#endif 

   if (rfm_do_thumbs==1 && !g_file_test(rfm_thumbDir, G_FILE_TEST_IS_DIR)) {
      if (g_mkdir_with_parents(rfm_thumbDir, S_IRWXU)!=0) {
         g_warning("Setup: Can't create thumbnail directory.");
         rfm_do_thumbs=0;
      }
   }
   
   g_signal_connect (rfmCtx->rfm_mountMonitor, "mounts-changed", G_CALLBACK (mounts_handler), rfmCtx);
   g_signal_connect (rfmCtx->rfm_mountMonitor, "mountpoints-changed", G_CALLBACK (mounts_handler), rfmCtx); /* fstab changed */

   init_inotify(rfmCtx);

   #ifdef RFM_ICON_THEME
      icon_theme=gtk_icon_theme_new();
      gtk_icon_theme_set_custom_theme(icon_theme,RFM_ICON_THEME);
   #else
      icon_theme=gtk_icon_theme_get_default();
   #endif

   fileMenu=setup_file_menu(rfmCtx); /* TODO: WARNING: This can return NULL! */
   defaultPixbufs=load_default_pixbufs(); /* TODO: WARNING: This can return NULL! */
   g_object_set_data(G_OBJECT(window),"rfm_file_menu",fileMenu);
   g_object_set_data_full(G_OBJECT(window),"rfm_default_pixbufs",defaultPixbufs,(GDestroyNotify)free_default_pixbufs);
   //TODO: why list_store_new api cannot take a dynamic array of columns, correspoin?
   store=gtk_list_store_new(NUM_COLS,
			      G_TYPE_STRING,    //iconview_markup
			      G_TYPE_STRING,    //iconview_tooltip
                              GDK_TYPE_PIXBUF,  /* Displayed icon */
			      G_TYPE_STRING,     //MODE_STR
			    //G_TYPE_STRING,    /* Displayed name */
			      G_TYPE_STRING,    //filename
			      G_TYPE_STRING,    //fullpath 
                              G_TYPE_UINT64,    /* File mtime: time_t is currently 32 bit signed */
			      G_TYPE_STRING, //MTIME_STR
			      G_TYPE_UINT64, //size
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
#endif
			      G_TYPE_STRING, //mime_sort
			    G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, //COL_Ext1..5
                            G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING); //COL_Ext6..10
   treemodel=GTK_TREE_MODEL(store);
   
   g_signal_connect(window,"destroy", G_CALLBACK(cleanup), rfmCtx);

   g_debug("initDir: %s",initDir);
   g_debug("rfm_curPath: %s",rfm_curPath);
   g_debug("rfm_homePath: %s",rfm_homePath);
   g_debug("rfm_thumbDir: %s",rfm_thumbDir);

#ifdef PythonEmbedded
   startPythonEmbedding();
#endif
   if (getcwd(cwd, sizeof(cwd)) == NULL) die("ERROR: %s: getcwd() failed.\n", PROG_NAME); /* getcwd returns NULL if cwd[] not big enough! */
   ReadFromPipeStdinIfAny(pipefd); //rfm_SearchResultPath may be filled with cwd here

   using_history();
   stifle_history(RFM_HISTORY_SIZE);
   rfm_historyFileLocation = g_build_filename(getenv("HOME"),".rfm_history", NULL);
   int e;
   if (e=read_history(rfm_historyFileLocation))
     g_warning("failed to read_history(%s) error code:%d.",rfm_historyFileLocation,e);

   rfm_prePath= getenv("OLDPWD");
   if (rfm_prePath!=NULL) rfm_prePath=strdup(rfm_prePath);

   add_toolbar(rfm_main_box, defaultPixbufs, rfmCtx);
   
   if (initDir == NULL) initDir = cwd;
   set_rfm_curPath(initDir);

   OLD_rl_attempted_completion_function = rl_attempted_completion_function;
   rl_attempted_completion_function = rfm_filename_completion;

   if (startWithVT() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)) stdin_command_help();
   if (auto_execution_command_after_rfm_start==NULL){
     refresh_store(rfmCtx);
     if(startWithVT() && !(StartedAs_rfmFileChooser && rfmFileChooserReturnSelectionIntoFilename==NULL)){
       readlineThread = g_thread_new("readline", readlineInSeperateThread, NULL);
     }
   }else stdin_command_Scheduler = g_idle_add_once(exec_stdin_command, auto_execution_command_after_rfm_start);
   
   //block Ctrl+C. Without this, Ctrl+C in readline will terminate rfm. Now, if you run htop with readline, Ctrl+C only terminate htop. BTW, it's strange that i had tried sigprocmask, pthread_sigmask, and rl_clear_signals, and they didn't work.
   struct sigaction newaction;
   newaction.sa_handler = SIG_IGN;
   newaction.sa_flags = 0;
   sigaction(SIGINT, &newaction,NULL);

   return 0;
}


static void cleanup(GtkWidget *window, RFM_ctx *rfmCtx)
{
   int e;

   if (StartedAs_rfmFileChooser){
      GtkTreeIter iter;
      int returnToFile_fd=-1;
      
      if (rfmFileChooserReturnSelectionIntoFilename!=NULL && (returnToFile_fd=open(rfmFileChooserReturnSelectionIntoFilename,O_WRONLY|O_NONBLOCK))<0)
	g_warning("failed to open rfmFileChooserReturnSelectionIntoFilename:%s",rfmFileChooserReturnSelectionIntoFilename);
      else if(returnToFile_fd>=0 || rfmFileChooserReturnSelectionIntoFilename==NULL ){
	  GList *selectionList = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);
	  selectionList=g_list_first(selectionList);
	  while(selectionList!=NULL){
	    GValue fullpath = G_VALUE_INIT;
	    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, selectionList->data);
	    gtk_tree_model_get_value(treemodel, &iter, COL_FULL_PATH, &fullpath);
	    if (returnToFile_fd>=0){
	      write(returnToFile_fd, g_value_get_string(&fullpath), strlen(g_value_get_string(&fullpath)));
	      write(returnToFile_fd,"\n",1);
	    }else printf("%s\n",g_value_get_string(&fullpath));
	    selectionList=g_list_next(selectionList);
	    rfmFileChooserResultNumber++;
	  };
	  if (returnToFile_fd>0) close(returnToFile_fd);
	  g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);
      }
   }
   //https://unix.stackexchange.com/questions/534657/do-inotify-watches-automatically-stop-when-a-program-ends
   inotify_rm_watch(rfm_inotify_fd, rfm_curPath_wd);
   if (rfm_do_thumbs==1) {
      inotify_rm_watch(rfm_inotify_fd, rfm_thumbnail_wd);
   }
   close(rfm_inotify_fd);
   if (e=append_history(history_entry_added, rfm_historyFileLocation)){
     if (e!=2) g_warning("failed to append_history(%d,%s) error code:%d",history_entry_added,rfm_historyFileLocation,e);
     else if (e=write_history(rfm_historyFileLocation))
       g_warning("failed to write_history(%s) error code:%d", rfm_historyFileLocation,e);
   }

   rl_attempted_completion_function = OLD_rl_attempted_completion_function;

#ifdef PythonEmbedded
   endPythonEmbedding();
#endif
   
   gtk_main_quit();
}

/* From http://dwm.suckless.org/ */
static void die(const char *errstr, ...) {
   va_list ap;

   va_start(ap, errstr);
   vfprintf(stderr, errstr, ap);
   va_end(ap);
   exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
   struct stat statbuf;

   g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL| G_LOG_FLAG_RECURSION, g_log_default_handler, NULL);

   //g_debug("test g_debug");
   //g_info("test g_info");
   //g_warning("test g_warning. set env G_MESSAGES_DEBUG=rfm if you want to see g_debug output");
   
   rfmCtx=calloc(1,sizeof(RFM_ctx));
   if (rfmCtx==NULL) return 1;
   rfmCtx->rfm_sortColumn=GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
   rfmCtx->rfm_mountMonitor=g_unix_mount_monitor_get();
   rfmCtx->showMimeType=0;
   rfmCtx->delayedRefresh_GSourceID=0;

   if (thumbnailers[0].thumbRoot==NULL)
      rfm_do_thumbs=0;
   else
      rfm_do_thumbs=1;

   PROG_NAME = strdup(argv[0]);
   int c=1;
   while (c<argc) {
     if (argv[c][0]=='-'){
      switch (argv[c][1]) {
      case 'd':
	 if (argc<=(c+1)) die("ERROR: %s: A directory path is required for the -d option.\n", PROG_NAME);
         //int i=strlen(argv[c+1])-1;
         //if (i!=0 && argv[c+1][i]=='/') argv[c+1][i]='\0';
         //if (strstr(argv[c+1],"/.")!=NULL) die("ERROR: %s: Hidden files are not supported.\n", PROG_NAME);
         if (stat(argv[c+1],&statbuf)!=0) die("ERROR: %s: Can't stat %s\n", PROG_NAME, argv[c+1]);
	 if (S_ISDIR(statbuf.st_mode)){
	    if (access(argv[c+1], R_OK | X_OK)!=0) die("ERROR: %s: Can't enter %s\n", PROG_NAME, argv[c+1]);
	    initDir=canonicalize_file_name(argv[c+1]);
	 }else{
	    initDir=g_path_get_dirname(argv[c+1]);
	    filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView] = g_list_prepend(filepath_lists_for_selection_on_view[SearchResultViewInsteadOfDirectoryView], g_strdup(argv[c+1]));
         }           
	 c++;

         break;
      case 'i':
            rfmCtx->showMimeType=1;
         break;
      case 'v':
         printf("%s-%s, Copyright (C) Rodney Padgett, guyuming, see LICENSE for details\n", PROG_NAME, VERSION);
         return 0;
      case 'p':
	   if (initDir!=NULL) die("if you have -d specified, and read file name list from pipeline, -p parameter must goes BEFORE -d\n");
	   SearchResultViewInsteadOfDirectoryView = 1;
	   gchar *pagesize=argv[c] + 2 * sizeof(gchar);
	   int ps=atoi(pagesize);
	   if (ps!=0) PageSize_SearchResultView=ps;
         break;

      case 'l':
	 treeview=TRUE;
	 break;
      case 's':
	 treeviewcolumn_init_order_sequence = &(argv[c][2]);
	 show_hide_treeview_columns_in_order(treeviewcolumn_init_order_sequence);
	 break;
      case 'h':
	printf(rfmLaunchHelp, PROG_NAME);
	return 0;
      case 'r':
	StartedAs_rfmFileChooser=TRUE;
	if (argc>c+1 && g_str_has_prefix(argv[c+1], RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX)){
	  rfmFileChooserReturnSelectionIntoFilename = argv[c+1];
	  c++;
	}
	if (rfmFileChooserReturnSelectionIntoFilename==NULL) rfmFileChooserReturnSelectionIntoFilename = getenv("rfmFileChooserReturnSelectionIntoFilename");
	break;
      case 'x': //auto execute after start. for example, start with locate rfm.c >0, to avoid locate rfm.c|rfm. We can use rfm -x "locate rfm.c>0"
	if (argc<=(c+1)) die("ERROR: %s: A command string which can be executed by rfm is required for the option. for example: rfm -x \"locate rfm.c>0\"\n", PROG_NAME);
	auto_execution_command_after_rfm_start = g_strdup(argv[c+1]);
	c++;
	break;
      case 't':
	rfmStartWithVirtualTerminal=FALSE;
	break;
      default:
	 die("invalid parameter, %s -h for help\n",PROG_NAME);
      }
    }
    else if (g_strcmp0(g_utf8_substring(argv[c], 0, 8),"/dev/fd/")==0) {
      //try  `gdb --args rfm <(locate rfm.c)` and find in htop what this /dev/fd means
      pipefd=g_utf8_substring(argv[c], 8, strlen(argv[c]));
    }
    c++;
   }

   if (setup(rfmCtx)==0)
      gtk_main();
   else
      die("ERROR: %s: setup() failed\n", PROG_NAME);

   if (startWithVT()) system("reset -I");
   return 0;
   //return (StartedAs_rfmFileChooser && rfmFileChooserResultNumber>0)?rfmFileChooserResultNumber:0;
}

static void ReadFromPipeStdinIfAny(char * fd)
{
   static char buf[PATH_MAX];
   char name[50];
   sprintf(name,"/proc/self/fd/%s",fd);
   int rslt = readlink(name, buf, PATH_MAX);

   g_debug("readlink for %s: %s",name,buf);

   if (strlen(buf)>4 && g_strcmp0(g_utf8_substring(buf, 0, 4),"pipe")==0){
     if (initDir!=NULL && (SearchResultViewInsteadOfDirectoryView^1)) die("if you have -d specified, and read file name list from pipeline, -p parameter must goes BEFORE -d\n");
	 else SearchResultViewInsteadOfDirectoryView=1;
	 
	 gchar *oneline_stdin=calloc(1,PATH_MAX);
	 FILE *pipeStream = stdin;
	 if (atoi(fd) != 0) pipeStream = fdopen(atoi(fd),"r");
         while (fgets(oneline_stdin, PATH_MAX,pipeStream ) != NULL) {
   	   g_debug("%s",oneline_stdin);
           oneline_stdin[strcspn(oneline_stdin, "\n")] = 0; //manual set the last char to NULL to eliminate the trailing \n from fgets
	   if (!ignored_filename(oneline_stdin)){ //TODO: shall we call ignored_filename here? we way remove it to align with the GetGlist implementation. User can filter those files with grep before rfm
	       SearchResultFileNameList=g_list_prepend(SearchResultFileNameList, oneline_stdin);
	       fileNum++;
	       g_debug("appended into FileNameListWithAbsolutePath_FromPipeStdin:%s", oneline_stdin);
	   }
	   oneline_stdin=calloc(1,PATH_MAX);
         }
         if (SearchResultFileNameList != NULL) {
           SearchResultFileNameList = g_list_reverse(SearchResultFileNameList);
           SearchResultFileNameList = g_list_first(SearchResultFileNameList);
	   CurrentPage_SearchResultView=SearchResultFileNameList;
	   currentFileNum=1;
	   g_free(rfm_SearchResultPath);
	   rfm_SearchResultPath=strdup(cwd);
	 }

         if (atoi(fd) == 0) { // open parent stdin to replace pipe
           char *tty = ttyname(1);
	   int pts=open(tty,O_RDWR);
	   dup2(pts,0);
	   close(pts);
         }
   }
}

static void update_SearchResultFileNameList_and_refresh_store(gpointer filenamelist){
  GList * old_filenamelist = SearchResultFileNameList;
  SearchResultFileNameList = NULL;
  fileNum=0;

  g_debug("update_SearchResultFileNameList length %d",strlen((gchar*)filenamelist));
  gchar * oneline=strtok((gchar*)filenamelist,"\n");
  while (oneline!=NULL){
    SearchResultFileNameList = g_list_prepend(SearchResultFileNameList, oneline);
    fileNum++;
    oneline=strtok(NULL, "\n");
  }

  if (SearchResultFileNameList != NULL) SearchResultFileNameList=g_list_first(g_list_reverse(SearchResultFileNameList));
  SearchResultViewInsteadOfDirectoryView = 1;
  g_free(rfm_SearchResultPath);
  rfm_SearchResultPath=strdup(rfm_curPath);
  FirstPage(rfmCtx);
  if (old_filenamelist!=NULL) g_list_free(old_filenamelist);
}


static gboolean startWithVT(){
  return rfmStartWithVirtualTerminal && isatty(0);
}


static void (*FileChooserClientCallback)(char**) =NULL;

GList* str_array_ToGList(char* a[]){
  GList * ret = NULL;
  if (a!=NULL)
    for(int i=0;i<G_N_ELEMENTS(a);i++) ret=g_list_prepend(ret, a[i]);
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
    read_char_pipe(child_attribs->stdOut_fd, PIPE_SZ, &child_StdOut);
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
  sprintf(named_pipe_name, "%s%d", RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX,getpid());
  remove(named_pipe_name);

  if (child_attribs->spawn_async && FileChooserClientCallback!=NULL) (*FileChooserClientCallback)(GList_to_str_array(*fileSelectionList,returnedCount));
}


/* default selection files can be passed in with fileSelectionList. After user
   interaction, this list is returned with user selection. And the default
   selection is freed in rfmFileChooserResultReader*/

GList* rfmFileChooser_glist(enum rfmTerminal startWithVirtualTerminal, char* search_cmd, gboolean async, GList** fileChooserSelectionListAddress, void (*fileChooserClientCallback)(char **)) {
  char named_pipe_name[50];
  sprintf(named_pipe_name, "%s%d", RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX,getpid());
  if (async && fileChooserClientCallback==NULL) { g_warning("to call rfmFileChooser async, fileChooserClientCallback function pointer must be provided."); return NULL; }
  FileChooserClientCallback = fileChooserClientCallback;
  if (mkfifo(named_pipe_name, 0700)==0){ //0700 is  rwx------ https://jameshfisher.com/2017/02/24/what-is-mode_t/
    int named_pipe_fd = open(named_pipe_name, O_RDONLY|O_NONBLOCK);
    if (named_pipe_fd>0){
      RFM_ChildAttribs *child_attribs=calloc(1,sizeof(RFM_ChildAttribs));
      child_attribs->customCallBackFunc = rfmFileChooserResultReader;
      child_attribs->customCallbackUserData = fileChooserSelectionListAddress;
      child_attribs->stdOut_fd = named_pipe_fd;
      child_attribs->stdOut = NULL;
      child_attribs->stdErr = NULL;
      child_attribs->spawn_async = async;
      child_attribs->runOpts=RFM_EXEC_FILE_CHOOSER;
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
