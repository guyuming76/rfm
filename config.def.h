/* Config file for rfm */

//#define DebugPrintf

/*#define RFM_ICON_THEME "elementary"*/
/*#define RFM_SINGLE_CLICK "True"*/
#define RFM_TOOL_SIZE 22
#define RFM_ICON_SIZE 48
#define RFM_THUMBNAIL_SIZE 128 /* Maximum size for thumb dir normal is 128 */
#define RFM_THUMBNAIL_LARGE_SIZE  RFM_THUMBNAIL_SIZE * 2
#define RFM_MX_MSGBOX_CHARS 1500 /* Maximum chars for RFM_EXEC_SYNC_MARKUP output; messages exceeding this will be displayed using RFM_EXEC_SYNC_TEXT_BOX mode */
#define RFM_MX_ARGS 128 /* Maximum allowed number of command line arguments in action commands below */
#define RFM_MOUNT_MEDIA_PATH "/run/media" /* Where specified mount handler mounts filesystems (e.g. udisksctl mount) */
#define RFM_MTIME_OFFSET 60      /* Display modified files as bold text (age in seconds) */
#define RFM_INOTIFY_TIMEOUT 500  /* ms between inotify events after which a full refresh is done */

/* Built in commands - MUST be present */
/* rfmBinPath is passed in by compiler via Makefile*/
static const char *f_rm[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmRemove.sh",NULL };
static const char *f_cp[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmCopy.sh", NULL };
static const char *f_mv[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmMove.sh", NULL };
#ifdef DragAndDropSupport
static const char *f_cp_DnD[]   = { "/bin/cp", "-p", "-R", "-f", NULL };
static const char *f_mv_DnD[]   = { "/bin/mv", "-f", NULL };
#endif

/* Run action commands: called as run_action <list of paths to selected files> */
static const char *play_video[] = { "/usr/bin/mpv", NULL };
static const char *play_audio[] = { "/usr/bin/mpv", "--player-operation-mode=pseudo-gui", "--", NULL };
static const char *av_info[]    = { "/usr/bin/mediainfo", "-f", NULL };
static const char *textEdit[]   = { "/usr/bin/emacs", "--no-splash", NULL };
static const char *mupdf[]      = { "/usr/bin/evince", NULL };
static const char *show_image[] = { rfmBinPath "/rfmRefreshImage.sh", NULL };
static const char *soffice[]    = { "/usr/bin/soffice", NULL };
static const char *extract_archive[] = { rfmBinPath "/extractArchive.sh", NULL };
static const char *create_archive[]  = { rfmBinPath "/createArchive.sh", NULL };
static const char *metaflac[] = { "/usr/bin/metaflac", "--list", "--block-type=VORBIS_COMMENT", NULL };
static const char *du[]       = { "/usr/bin/du", "-s", "-h", NULL };
static const char *mount[]    = { rfmBinPath "/suMount.sh", NULL };
static const char *umount[]   = { rfmBinPath "/suMount.sh", "-u", NULL };
/* static const char *udisks_mount[]   = { "/usr/bin/udisksctl", "mount", "--no-user-interaction", "-b", NULL }; */
/* static const char *udisks_unmount[] = { "/usr/bin/udisksctl", "unmount", "--no-user-interaction", "-b", NULL }; */
static const char *properties[]     = { rfmBinPath "/rfmVTforCMD_hold.sh", rfmBinPath "/rfmProperties.sh", NULL };
static const char *www[]        = { "/usr/local/bin/www", NULL };
static const char *man[]        = { "/usr/bin/groff", "-man", "-Tutf8", NULL };
static const char *java[]       = { "/usr/bin/java", "-jar", NULL };
static const char *audioSpect[] = { "/usr/local/bin/spectrogram.sh", NULL };
static const char *open_with[]  = { rfmBinPath "/open_with_dmenu.sh", NULL };
static const char *exiftran[]   = { "/usr/bin/exiftran", "-a", "-i", NULL };  /* pacman -S fbida */
static const char *gnumeric[]   = { "/usr/bin/gnumeric", NULL };
static const char *ftview[] = { "/usr/bin/ftview", "14", NULL }; /* pacman -S freetype2-demos */
static const char *ffmpegThumb[] =  { "/usr/bin/ffmpeg", "-i","", "-frames", "1", "-s", "256x256",NULL  };

    /* Tool button commands */
static const char *term_cmd[]  = { "/usr/bin/alacritty", NULL };
static const char *new_rfm[]  = { rfmBinPath "/rfm", NULL };

#ifdef GitIntegration
static const char *git_inside_work_tree_cmd[] = {"/usr/bin/git", "rev-parse","--is-inside-work-tree", NULL};
static const char *git_ls_files_cmd[] = {"/usr/bin/git", "ls-files", "--full-name",NULL};
static const char *git_modified_staged_info_cmd[] = {"/usr/bin/git","status","--porcelain",NULL};
static const char *git_stage_cmd[] = {"/usr/bin/git","stage",NULL};
static const char *git_root_cmd[] = {"/usr/bin/git","rev-parse", "--show-toplevel",NULL};
static const char *git_commit_message_cmd[] = {"/usr/bin/git","log","--oneline",NULL};
static const char *git_log_cmd[] = { rfmBinPath "/rfmVTforCMD_hold.sh","/usr/bin/git","log",NULL};
#endif


/* Run actions
 * NOTES: The first three MUST be the built in commands for cp, mv and rm, respectively.
 *        The first matched mime type will become the default action for that kind of file
 *        (i.e. double click opens)
 *        If the mime root is "*", the action will be shown for all files, but will
 *        NEVER be the default action for any file type.
 *        If the mime sub type is "*", the action will be shown for all files of type
 *        mime root. It may be the default action if no prior action is defined for the file.
 *        All stderr is sent to the filer's stderr.
 *        Any stdout may be displayed by defining the run option.
 * Run options are:
 *   RFM_EXEC_NONE   - run and forget; program is expected to show its own output
 *   RFM_EXEC_TEXT   - run and show output in a fixed font, scrolled text window
 *   RFM_EXEC_PANGO  - run and show output in a dialog window supporting pango markup
 *   RFM_EXEC_PLAIN  - run and show output in a plain text dialog window
 *   RFM_EXEC_INTERNAL - same as RFM_EXEC_PLAIN; intended for required commands cp, mv, rm
 *   RFM_EXEC_MOUNT  - same as RFM_EXEC_PLAIN; this should be specified for the mount command.
 *                     causes the filer to auto switch on success to the directory defined by
 *                     #define RFM_MOUNT_MEDIA_PATH.
 *   RFM_EXEC_STDOUT - run and show output in stdout of parent process.
 */
static RFM_RunActions run_actions[] = {
   /* name           mime root        mime sub type             argument          run options */
   { RunActionCopy,         "*",              "*",                    f_cp,             RFM_EXEC_INTERNAL },
   { RunActionMove,         "*",              "*",                    f_mv,             RFM_EXEC_INTERNAL },
   { RunActionDelete,       "*",              "*",                    f_rm,             RFM_EXEC_INTERNAL },
#ifdef GitIntegration
   { RunActionGitStage,    "*",              "*",                    git_stage_cmd,    RFM_EXEC_INTERNAL },
   { "git log",      "*",              "*",                    git_log_cmd,      RFM_EXEC_NONE },
#endif
   { "Properties",   "*",              "*",                    properties,       RFM_EXEC_PANGO },
   { "Open with...", "*",              "*",                    open_with,        RFM_EXEC_NONE },
   { "Open",         "image",          "*",                    show_image,       RFM_EXEC_NONE },
   { "Rotate",       "image",          "jpeg",                 exiftran,         RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.oasis.opendocument.text",          soffice,  RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.oasis.opendocument.spreadsheet",   soffice,  RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.wordprocessingml.document", soffice, RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.spreadsheetml.sheet",       soffice, RFM_EXEC_NONE },
   { "Open",         "application",    "msword",               soffice,          RFM_EXEC_NONE },
   { "Open",         "application",    "x-gnumeric",           gnumeric,         RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.ms-excel",         gnumeric,         RFM_EXEC_NONE },
   { "Open",         "application",    "vnd.ms-excel",         soffice,          RFM_EXEC_NONE },
   { "extract",      "application",    "x-compressed-tar",     extract_archive,  RFM_EXEC_NONE },
   { "extract",      "application",    "x-bzip-compressed-tar",extract_archive,  RFM_EXEC_NONE },
   { "extract",      "application",    "x-xz-compressed-tar",  extract_archive,  RFM_EXEC_NONE },
   { "extract",      "application",    "x-zstd-compressed-tar", extract_archive,  RFM_EXEC_NONE },
   { "extract",      "application",    "zip",                  extract_archive,  RFM_EXEC_NONE },
   { "extract",      "application",    "x-rpm",                extract_archive,  RFM_EXEC_NONE },
   { "View",         "application",    "pdf",                  mupdf,            RFM_EXEC_NONE },
   { "View",         "application",    "epub+zip",             mupdf,            RFM_EXEC_NONE },
   { "View",         "application",    "x-troff-man",          man,              RFM_EXEC_NONE },
   { "Run",          "application",    "x-java-archive",       java,             RFM_EXEC_NONE },
   { "Open as text", "application",    "*",                    textEdit,         RFM_EXEC_NONE },
   { "Play",         "audio",          "*",                    play_audio,       RFM_EXEC_NONE },
   { "flac info",    "audio",          "flac",                 metaflac,         RFM_EXEC_PLAIN },
   { "info",         "audio",          "*",                    av_info,          RFM_EXEC_PLAIN },
   { "stats",        "audio",          "*",                    audioSpect,       RFM_EXEC_PANGO },
   { "count",        "inode",          "directory",            du,               RFM_EXEC_PLAIN },
   { "archive",      "inode",          "directory",            create_archive,   RFM_EXEC_NONE },
   { "mount",        "inode",          "mount-point",          mount,            RFM_EXEC_PLAIN },
   { "unmount",      "inode",          "mount-point",          umount,           RFM_EXEC_PLAIN },
   { "mount",        "inode",          "blockdevice",          mount,            RFM_EXEC_MOUNT },
   { "unmount",      "inode",          "blockdevice",          umount,           RFM_EXEC_PLAIN },
   { "edit",         "text",           "*",                    textEdit,         RFM_EXEC_NONE },
   { "Open",         "text",           "html",                 www,              RFM_EXEC_NONE },
   { "Play",         "video",          "*",                    play_video,       RFM_EXEC_NONE },
   { "info",         "video",          "*",                    av_info,          RFM_EXEC_TEXT },
   { "View",         "font",           "*",                    ftview,           RFM_EXEC_NONE },
};

/* Toolbar button definitions
 * icon is the name (not including .png) of an icon in the current theme.
 * function is the name of a function to be defined here. If function is NULL,
 * the argument is assumed to be a shell command. All commands are run
 * async - no stdout / stderr is shown. The current working directory for
 * the command environment is changed to the currently displayed directory.
 * If function is not NULL, arguments may be passed as a const char *arg[].
 */

//static const char *dev_disk_path[]= { "/dev/disk" };
//static void show_disk_devices(const char **path) {
//   set_rfm_curPath((char*)path[0]);
//}

static RFM_ToolButtons tool_buttons[] = {
   /* name           icon                       function    		argument*/
   { "Terminal",     "utilities-terminal",      NULL,                   term_cmd },
   { "rfm",          "system-file-manager",     NULL,                   new_rfm },
// { "mounts",       "drive-harddisk",          show_disk_devices,      dev_disk_path },
};

/* Thumbnailers
 * There must be at least one entry in the thumbnailers array.
 * To disable thumbnailing use:
 * static const RFM_Thumbnailers thumbnailers[]={{ NULL, NULL, NULL }};
 * Use thumbnail function set to NULL for built-in thumbnailer (gdk_pixbuf):
 * static const RFM_Thumbnailers thumbnailers[]={{ "image", "*", NULL }};
 * For other thumbnails, a function may be defined to handle that kind of thumbnail.
 * The function prototype is
 * GdkPixbuf *(func)(gchar *path, gint size);
 * where path is the path and filename of the file to be thumbnailed, and size is
 * the size of the thumbnail (RFM_THUMBNAIL_SIZE will be passed).
 * The function should return the thumbnail as a pixbuf.
 */


//#include "libdcmthumb/dcmThumb.h"
static gpointer ffmpeg4Thumb(RFM_ThumbQueueData * thumbData)
{
  gchar *thumb_path=g_build_filename(rfm_thumbDir, thumbData->thumb_name, NULL);
  //gchar *input_file=strcat("-i ", thumbData->path);
  GList * input_files=NULL;
  input_files=g_list_prepend(input_files, g_strdup(thumbData->path));
  g_spawn_wrapper(ffmpegThumb, input_files, 1, RFM_EXEC_NONE, thumb_path, FALSE, NULL, NULL);
  g_list_free(input_files);
  return NULL;
}

static const RFM_Thumbnailers thumbnailers[] = {
    /* mime root      mime sub type        thumbnail function */
    {"image", "*", NULL},
    {"video","mp4",ffmpeg4Thumb},
    //   { "application",  "dicom",             dcmThumb},
};

#define Allow_Thumbnail_Without_tExtThumbMTime
#define MOD_KEY GDK_SUPER_MASK // the windows logo key
//#define MOD_KEY GDK_META_MASK // the Alt key

static gboolean ignored_filename(gchar *name){
  if (name[0]=='.') return TRUE; /* Don't show hidden files */
  if (strcmp(name, "gmon.out")==0) return TRUE;
  return FALSE;
}
