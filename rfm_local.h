
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
