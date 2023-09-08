/* Config file for rfm */


/*#define RFM_ICON_THEME "elementary"*/
/*#define RFM_SINGLE_CLICK "True"*/
#define RFM_TOOL_SIZE 22
#define RFM_ICON_SIZE 48
#define RFM_THUMBNAIL_SIZE 128 /* Maximum size for thumb dir normal is 128 */
#define RFM_THUMBNAIL_LARGE_SIZE  RFM_THUMBNAIL_SIZE * 2
#define RFM_MX_MSGBOX_CHARS 1500 /* Maximum chars for show_msg output; messages exceeding this will be displayed in parent stdout */
#define RFM_MX_ARGS 128 /* Maximum allowed number of command line arguments in action commands below */
#define RFM_MOUNT_MEDIA_PATH "/run/media" /* Where specified mount handler mounts filesystems (e.g. udisksctl mount) */
#define RFM_MTIME_OFFSET 60      /* Display modified files as bold text (age in seconds) */
#define RFM_INOTIFY_TIMEOUT 500  /* ms between inotify events after which a full refresh is done */

/* Built in commands - MUST be present */
/* rfmBinPath is passed in by compiler via Makefile*/
static const char *f_rm[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmRemove.sh",NULL };
static const char *f_cp[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmCopy.sh", NULL };
static const char *f_mv[]   = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmMove.sh", NULL };
static const char *cp_selection_to_clipboard[] = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmCopySelectionToClipboard.sh",NULL};
static const char *cp_clipboard_to_curPath[] = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmCopyClipboardToCurPath.sh",NULL };
static const char *mv_clipboard_to_curPath[] = { rfmBinPath "/rfmVTforCMD.sh",rfmBinPath "/rfmMoveClipboardToCurPath.sh",NULL };
static const char *change_owner[] = { rfmBinPath "/rfmVTforCMD.sh", rfmBinPath "/rfmChangeOwner.sh",NULL };

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
static const char *gnumeric[]   = { "/usr/bin/gnumeric", NULL };
static const char *ftview[] = { "/usr/bin/ftview", "14", NULL }; /* pacman -S freetype2-demos */
static const char *ffmpegThumb[] =  { "/usr/bin/ffmpeg", "-i","", "-frames", "1", "-s", "256x256",NULL  };

    /* Tool button commands */
static const char *term_cmd[]  = { "/usr/bin/foot", NULL };
static const char *new_rfm[]  = { rfmBinPath "/rfmVTforCMD.sh", rfmBinPath "/rfm", NULL };

#ifdef GitIntegration
static const char *git_inside_work_tree_cmd[] = {"/usr/bin/git", "rev-parse","--is-inside-work-tree", NULL};
static const char *git_ls_files_cmd[] = {"/usr/bin/git", "ls-files", "--full-name",NULL};
static const char *git_modified_staged_info_cmd[] = {"/usr/bin/git","status","--porcelain",NULL};
static const char *git_stage_cmd[] = {"/usr/bin/git","stage",NULL};
static const char *git_root_cmd[] = {"/usr/bin/git","rev-parse", "--show-toplevel",NULL};
static const char *git_commit_message_cmd[] = {"/usr/bin/git","log","-1","--oneline",NULL};
static const char *git_log_cmd[] = { rfmBinPath "/rfmVTforCMD_hold.sh","/usr/bin/git","log",NULL};
static const char *tig_cmd[] = { rfmBinPath "/rfmVTforCMD.sh","/usr/bin/tig",NULL};
static const char *git_show_pics_cmd[] = { rfmBinPath "/rfmVTforCMD.sh", rfmBinPath "/rfmGitShowPictures.sh",NULL};
static const char *git_commit[] = { rfmBinPath "/rfmVTforCMD.sh", rfmBinPath "/rfmGitCommit.sh",NULL};
static const char *git_current_branch_cmd[] =  { "/usr/bin/git","branch","--show-current",NULL };
#endif

static RFM_builtinCMD builtinCMD[] = {
	{"q",      gtk_main_quit,         "alias for quit"}, 
};


/* Menu Items
 * NOTES: The first matched mime type will become the default action for that kind of file
 *        (i.e. double click opens)
 *        If the mime root is "*", the action will be shown for all files, but will
 *        NEVER be the default action for any file type.
 *        If the mime sub type is "*", the action will be shown for all files of type
 *        mime root. It may be the default action if no prior action is defined for the file.
 *        All stderr is sent to the filer's stderr.
 *        Any stdout may be displayed by defining the run option.
 * Run options are:
 *   RFM_EXEC_NONE   - run and forget; program is expected to show its own output
 *   RFM_EXEC_MOUNT  - same as RFM_EXEC_PLAIN; this should be specified for the mount command.
 *                     causes the filer to auto switch on success to the directory defined by
 *                     #define RFM_MOUNT_MEDIA_PATH.
 *   RFM_EXEC_STDOUT - run and show output in stdout of parent process.
 */

#define   RFM_EXEC_NONE     G_SPAWN_STDOUT_TO_DEV_NULL
#define   RFM_EXEC_STDOUT   G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_CHILD_INHERITS_STDOUT | G_SPAWN_CHILD_INHERITS_STDERR
#define   RFM_EXEC_OUPUT_READ_BY_PROGRAM G_SPAWN_DEFAULT
#define   RFM_EXEC_MOUNT   G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_CHILD_INHERITS_STDOUT | G_SPAWN_CHILD_INHERITS_STDERR  //TODO: i don't know what this mean yet.

static RFM_MenuItem run_actions[] = {
   /* name           mime root        mime sub type            runCmd            	run options 		showCondition	*/
   { RunActionCopy,  "*",              "*",                    f_cp,             	RFM_EXEC_NONE, 		NULL },
   { RunActionMove,  "*",              "*",                    f_mv,             	RFM_EXEC_STDOUT,	NULL },
   { RunActionDelete, "*",             "*",                    f_rm,             	RFM_EXEC_STDOUT,	NULL },
   { RunActionCopySelection, "*",      "*",                    cp_selection_to_clipboard, RFM_EXEC_NONE,	NULL },
#ifdef GitIntegration
   { RunActionGitStage,  "*",          "*",                    git_stage_cmd,    	RFM_EXEC_STDOUT,	cur_path_is_git_repo },
   { "git log",      "*",              "*",                    git_log_cmd,      	RFM_EXEC_NONE,		cur_path_is_git_repo },
   { "tig",          "*",              "*",                    tig_cmd,          	RFM_EXEC_NONE,		cur_path_is_git_repo },
#endif
   { "Properties",   "*",              "*",                    properties,       	RFM_EXEC_STDOUT,	NULL },
   { "Open with...", "*",              "*",                    open_with,        	RFM_EXEC_NONE,		NULL },
   { RunActionChangeOwner,"*",         "*",                    change_owner,            RFM_EXEC_NONE,          NULL },
   { "Open",         "image",          "*",                    show_image,       	RFM_EXEC_STDOUT,	NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.text",          soffice,  RFM_EXEC_NONE,		NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.spreadsheet",   soffice,  RFM_EXEC_NONE,		NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.wordprocessingml.document", soffice, RFM_EXEC_NONE, NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.spreadsheetml.sheet",       soffice, RFM_EXEC_NONE, NULL },
   { "Open",         "application",    "msword",               soffice,          	RFM_EXEC_NONE,		NULL },
   { "Open",         "application",    "x-gnumeric",           gnumeric,         	RFM_EXEC_NONE,		NULL },
   { "Open",         "application",    "vnd.ms-excel",         gnumeric,         	RFM_EXEC_NONE,		NULL },
   { "Open",         "application",    "vnd.ms-excel",         soffice,          	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "x-compressed-tar",     extract_archive,  	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "x-bzip-compressed-tar",extract_archive,  	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "x-xz-compressed-tar",  extract_archive,  	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "x-zstd-compressed-tar", extract_archive, 	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "zip",                  extract_archive,  	RFM_EXEC_NONE,		NULL },
   { "extract",      "application",    "x-rpm",                extract_archive,  	RFM_EXEC_NONE,		NULL },
   { "View",         "application",    "pdf",                  mupdf,            	RFM_EXEC_NONE,		NULL },
   { "View",         "application",    "epub+zip",             mupdf,            	RFM_EXEC_NONE,		NULL },
   { "View",         "application",    "x-troff-man",          man,              	RFM_EXEC_NONE,		NULL },
   { "Run",          "application",    "x-java-archive",       java,             	RFM_EXEC_NONE,		NULL },
   { "Open as text", "application",    "*",                    textEdit,         	RFM_EXEC_NONE,		NULL },
   { "Play",         "audio",          "*",                    play_audio,       	RFM_EXEC_NONE,		NULL },
   { "flac info",    "audio",          "flac",                 metaflac,         	RFM_EXEC_STDOUT,	NULL },
   { "info",         "audio",          "*",                    av_info,          	RFM_EXEC_STDOUT,	NULL },
   { "stats",        "audio",          "*",                    audioSpect,       	RFM_EXEC_STDOUT,	NULL },
   { "count",        "inode",          "directory",            du,               	RFM_EXEC_STDOUT,	NULL },
   { "archive",      "inode",          "directory",            create_archive,   	RFM_EXEC_NONE,		NULL },
   { "mount",        "inode",          "mount-point",          mount,            	RFM_EXEC_STDOUT,	NULL },
   { "unmount",      "inode",          "mount-point",          umount,           	RFM_EXEC_STDOUT,	NULL },
   { "mount",        "inode",          "blockdevice",          mount,            	RFM_EXEC_MOUNT,		NULL },
   { "unmount",      "inode",          "blockdevice",          umount,           	RFM_EXEC_STDOUT,	NULL },
   { "edit",         "text",           "*",                    textEdit,         	RFM_EXEC_NONE,		NULL },
   { "Open",         "text",           "html",                 www,              	RFM_EXEC_NONE,		NULL },
   { "Play",         "video",          "*",                    play_video,       	RFM_EXEC_NONE,		NULL },
   { "info",         "video",          "*",                    av_info,          	RFM_EXEC_STDOUT,	NULL },
   { "View",         "font",           "*",                    ftview,           	RFM_EXEC_NONE,		NULL },
};

/* Toolbar button definitions
 * icon is the name (not including .png) of an icon in the current theme.
 * function is the name of a function to be defined here. If function is NULL,
 * the RunCmd is assumed to be a shell command. All commands are run
 * async. The current working directory for
 * the command environment is changed to the currently displayed directory.
 * 
 * TODO: currently for functions, parameters are all rfmCtx if any. In future, we may need another column to specify a global variable as parameter for function. And we need to make rfmCtx global for this.
 * TODO: do we need a RunOption column here as the file menu items?
 * If both RunCmd and function is not NULL, function is run as callback after RunCmd finishs
 *
 * The difference between Toolbar button and file menu RunActions is that RunActions is for selected files while Toolbar RunCmd won't deal with selected files, it is for current directory */

//static const char *dev_disk_path[]= { "/dev/disk" };
//static void show_disk_devices(const char **path) {
//   set_rfm_curPath((char*)path[0]);
//}

static RFM_ToolButton tool_buttons[] = {
   /* name           icon                       function    		RunCmd             readFromPipe    curPath    Accel                  tooltip                showCondition*/
   { SwitchView,     NULL,                      switch_view,            NULL,              TRUE,           TRUE,      GDK_KEY_slash,         "MOD+/",                 NULL},
   { SwitchPipeDir,  NULL,                      toggle_readFromPipe,    NULL,              TRUE,           TRUE,      0,                      NULL,                   readFromPipe },
   { Up,             "go-up",                   up_clicked,             NULL,              FALSE,          TRUE,      GDK_KEY_Up,            "MOD+up arrow",          NULL},
   { "Refresh",      "view-refresh",            refresh_store,          NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { Paste,          NULL,                      NULL,                   cp_clipboard_to_curPath, FALSE,    TRUE,      GDK_KEY_V,             "MOD+v",                 NULL},
   { MoveTo,         NULL,                      NULL,                   mv_clipboard_to_curPath, FALSE,    TRUE,      GDK_KEY_X,             "MOD+x",                 NULL},
   { PageUp,         NULL,                      PreviousPage,           NULL,              TRUE,           FALSE,     GDK_KEY_Page_Up,       "MOD+PgUp",              NULL},
   { PageDown,       NULL,                      NextPage,               NULL,              TRUE,           FALSE,     GDK_KEY_Page_Down,     "MOD+PgDn",              NULL},
   { "Terminal",     "utilities-terminal",      NULL,                   term_cmd,          TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "rfm",          "system-file-manager",     NULL,                   new_rfm,           TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Stop",         "process-stop",            rfm_stop_all,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Info",         "dialog-information",      info_clicked,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Home",         "go-home",                 home_clicked,           NULL,              FALSE,          TRUE,      0,                      NULL,                   NULL},
#ifdef GitIntegration
   { "gitCommit",    NULL,                      refresh_store,          git_commit,        FALSE,          TRUE,      0,                      NULL,                   cur_path_is_git_repo  },
#endif
   { "cpPath",       NULL,                      copy_curPath_to_clipboard, NULL,           TRUE,           TRUE,      0,                      "copy current path to clipboard", NULL},
// { "mounts",       "drive-harddisk",          show_disk_devices,      dev_disk_path },
};

/* Thumbnailers
 * There must be at least one entry in the thumbnailers array.
 * To disable thumbnailing use:
 * static const RFM_Thumbnailers thumbnailers[]={{ NULL, NULL, NULL }};
 * Use thumbnail function set to NULL for built-in thumbnailer (gdk_pixbuf):
 * static const RFM_Thumbnailers thumbnailers[]={{ "image", "*", NULL }};
 * For other thumbnails, const gchar *thumbCmd  may be defined to handle that kind of thumbnail with shell command.
*/


static const RFM_Thumbnailer thumbnailers[] = {
    /* mime root      mime sub type        thumbCmd */
    {"image", "*", NULL},
    {"video","mp4",ffmpegThumb},
    //   { "application",  "dicom",             dcmThumb},
};

#define Allow_Thumbnail_Without_tExtThumbMTime
#define MOD_KEY GDK_SUPER_MASK // the windows logo key
//#define MOD_KEY GDK_META_MASK // the Alt key

static gboolean ignored_filename(gchar *name){
  if (name[0]=='.' && strstr(name,"./")==NULL) return TRUE; /* Don't show hidden files */
  if (strstr(name,"/.")!=NULL) return TRUE;
  if (strcmp(name, "gmon.out")==0) return TRUE;
  return FALSE;
}
