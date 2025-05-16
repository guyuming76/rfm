//rfm项目里目前有两个.h文件： rfm_local.h 和 config.h.
//最初只有config.h,用于suckless风格的定制，自定义文件菜单之类
//rfm_local.h的加入是为了满足 rfm_local.c 的动态编译。rfm_local.h 要在 rfm.c 和 rfm_local.c 之间共享
//事实上，config.h里面的定义也是依赖rfm_local.h内数据类型定义的，但config.h内无需#include rfm_local.h,这时因为在rfm.c里面对于config.h的引用出现在对rfm_local.h的应用的后面，c语言编译器似乎可以处理这种情况
//TODO：虽说rfm_local.h和config.h的需求不同，有共存的合理性，但若能合并成一个config.h似乎更好
//比如，config.h里 RFM_SearchResultTypeNamePrefix 定义是为了方便用户定制的，但在目前架构下，rfm_local.c 里就看不到这个定义。但若把这个定义移入rfm_local.h, 就破坏了rfm_local.h的原本设计目的：这个不是用来让用户定制的
//若是目前在rfm_local.c里直接引用config.h,编译是通不过的，config.h依赖在rfm.c里其出现位置前的许多类型定义和方法声明，需要调整以便能够被rfm_local.h引用
//另外需要解决的是rfm_local.c对config.h的定位的问题，之前config.h只要放在rfm源码目录里就可以了，将来可能需要放在某个全局或本地配置目录里,gentoo suckless选项下倒是有专门的存放路径


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
