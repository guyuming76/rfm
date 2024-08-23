/* Config file for rfm */

/*#define RFM_ICON_THEME "elementary"*/
/*#define RFM_SINGLE_CLICK "True"*/
#include <linux/limits.h>
#define RFM_TOOL_SIZE 22
#define RFM_ICON_SIZE 48
static gint RFM_THUMBNAIL_SIZE = 128; /* Maximum size for thumb dir normal is 128 */
//#define RFM_CACHE_THUMBNAIL_IN_MEM TRUE
#define RFM_MX_MSGBOX_CHARS 1500 /* Maximum chars for show_msg output; messages exceeding this will be displayed in parent stdout */
#define RFM_MX_ARGS 128 /* Maximum allowed number of command line arguments in action commands below */
#define RFM_MOUNT_MEDIA_PATH "/run/media" /* Where specified mount handler mounts filesystems (e.g. udisksctl mount) */
#define RFM_MTIME_OFFSET 60      /* Display modified files as bold text (age in seconds) */
#define RFM_INOTIFY_TIMEOUT 500  /* ms between inotify events after which a full refresh is done */
#define RFM_DATETIME_FORMAT "%Y-%m-%d,%H:%M:%S"
#define RFM_HISTORY_SIZE 10000
#define RFM_AUTOSELECT_OLDPWD_IN_VIEW TRUE
#define RFM_AUTO_SORT_ENTER_VIEW FALSE
static gboolean UseTargetAddressWhenEnterSymbloicLinkForDir=TRUE;
#define RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX "/tmp/rfmFileChooser_"
#define RFM_ENV_VAR_NAME_PREFIX ""  //change this if your extented column name conflicts with existing ENV Variable name

//add the two line follows into ~/.inputrc so that readline can search history based on prefix. read https://www.man7.org/linux/man-pages/man3/readline.3.html  for more
// arrow up
// "\e[A":history-search-backward
// arrow down
// "\e[B":history-search-forward
#define RFM_LOG_DATA "rfm-data"
#define RFM_LOG_DATA_EXT "rfm-data-ext"
#define RFM_LOG_DATA_THUMBNAIL "rfm-data-thumbnail"
#define RFM_LOG_DATA_SEARCH "rfm-data-search"
#define RFM_LOG_DATA_SORT "rfm-data-sort"
#ifdef GitIntegration
#define RFM_LOG_DATA_GIT "rfm-data-git"
#endif
#define RFM_LOG_GSPAWN "rfm-gspawn"
#define RFM_LOG_COLUMN "rfm-column"
#define RFM_LOG_COLUMN_VERBOSE "rfm-column-verbose"
#define RFM_LOG_GTK "rfm-gtk"

static gboolean keep_selection_on_view_across_refresh = TRUE; 

/* rfmBinPath is passed in by compiler via Makefile*/
static const char *f_rm[]   = { "$RFM_TERM",rfmBinPath "/rfmRemove.sh",NULL };
static const char *f_cp[]   = { "$RFM_TERM",rfmBinPath "/rfmCopyMove.sh","cp", NULL };
static const char *f_mv[]   = { "$RFM_TERM",rfmBinPath "/rfmCopyMove.sh", "mv",NULL };
static const char *cp_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "/rfmCopyMoveToCurPath.sh","cp",NULL };
static const char *mv_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "/rfmCopyMoveToCurPath.sh","mv",NULL };
static const char *ln_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "/rfmCopyMoveToCurPath.sh","sl",NULL };
static const char *create_softlink[] = { "$RFM_TERM",rfmBinPath "/rfmCreateSoftLink.sh",NULL };
static const char *find_softlink[] = { "$RFM_TERM",rfmBinPath "/rfmFindLinksToTheSameFile.sh",NULL };
static const char *change_owner[] = { "$RFM_TERM", rfmBinPath "/rfmChangeOwner.sh",NULL };
static const char *play_video[] = { "/usr/bin/xdg-open", NULL };
static const char *play_audio[] = { "/usr/bin/xdg-open", NULL };
static const char *av_info[]    = { "/usr/bin/mediainfo", "-f", NULL };
static const char *textEdit[]   = { "$RFM_TERM", rfmBinPath "/rfmTextEdit.sh", NULL };
static const char *pdf[]        = { "/usr/bin/xdg-open", NULL };
static const char *mupdf[]      = { "/usr/bin/evince", NULL };
static const char *show_image[] = { rfmBinPath "/rfmRefreshImage.sh", NULL };
static const char *soffice[]    = { "/usr/bin/xdg-open", NULL };
static const char *extract_archive[] = { rfmBinPath "/extractArchive.sh", NULL };
static const char *create_archive[]  = { rfmBinPath "/createArchive.sh", NULL };
static const char *list_archive[] = { rfmBinPath "/rfmVTforCMD.sh", "tar","--list","--file", NULL};
static const char *metaflac[] = { "/usr/bin/metaflac", "--list", "--block-type=VORBIS_COMMENT", NULL };
static const char *du[]       = { "/usr/bin/du", "-s", "-h", NULL };
static const char *mount[]    = { rfmBinPath "/suMount.sh", NULL };
static const char *umount[]   = { rfmBinPath "/suMount.sh", "-u", NULL };
/* static const char *udisks_mount[]   = { "/usr/bin/udisksctl", "mount", "--no-user-interaction", "-b", NULL }; */
/* static const char *udisks_unmount[] = { "/usr/bin/udisksctl", "unmount", "--no-user-interaction", "-b", NULL }; */
static const char *properties[]     = { "$RFM_TERM", rfmBinPath "/rfmProperties.sh", NULL };
static const char *www[]        = { "/usr/bin/xdg-open", NULL };
static const char *man[]        = { "/usr/bin/groff", "-man", "-Tutf8", NULL };
static const char *java[]       = { "/usr/bin/java", "-jar", NULL };
static const char *audioSpect[] = { "/usr/local/bin/spectrogram.sh", NULL };
static const char *open_with[]  = { rfmBinPath "/open_with_dmenu.sh", NULL };
static const char *gnumeric[]   = { "/usr/bin/gnumeric", NULL };
static const char *ftview[] = { "/usr/bin/ftview", "14", NULL }; /* pacman -S freetype2-demos */
static const char *ffmpegThumb[] =  { "/usr/bin/ffmpeg", "-i","", "-frames", "1", "-s", "256x256",NULL  };
static const char *term_cmd[]  = { "$RFM_TERM", NULL };
static const char *new_rfm[]  = { "$RFM_TERM", rfmBinPath "/rfm", NULL };
static const char *share_dir[] = { "$RFM_TERM", rfmBinPath "/rfmShareDir.sh", NULL };
#ifdef GitIntegration
static const char *git_inside_work_tree_cmd[] = {"/usr/bin/git", "rev-parse","--is-inside-work-tree", NULL};
static const char *git_ls_files_cmd[] = {"/usr/bin/git", "ls-files", "--full-name",NULL};
static const char *git_modified_staged_info_cmd[] = {"/usr/bin/git","status","--porcelain",NULL};
static const char *git_stage_cmd[] = {"/usr/bin/git","stage",NULL};
static const char *git_root_cmd[] = {"/usr/bin/git","rev-parse", "--show-toplevel",NULL};
static const char *git_commit_message_cmd[] = {"/usr/bin/git","log","-1","--oneline",NULL};
static const char *git_log_cmd[] = { rfmBinPath "/rfmVTforCMD.sh","/usr/bin/git","log",NULL};
//现在只有很少的像上面这行的情况需要rfmVTforCMD_hold.sh脚本，如果想去掉它，统一用rfmVTforCMD.sh,就得想办法把类似read -p 这种代码插到上面的数组里，如果直接把 read 命令放在git log后面，NULL的前面，目前就会牺牲可以在末尾追加任意个选中文件的功能，我记得加一个“”数组项可以表示替换一个文件
//若利用stdin_command_bash 这样的返回数组的函数，就意味着要增强文件上下文菜单的配置功能，让其能够接受函数，但这样会增加代码的复杂性，我认为上下文菜单应该保持只接受静态数组这么一种相对简单的配置方式
//TODO：所以，现在要么保持有些冗余的rfmVTforCMD_hold.sh脚本，要么增强把静态数组和当前选中文件列表合并的方式，增加一个在中间合并进全部文件的符号，这个可以和命令行中间使用%s符号替换文件名一起考虑
//static const char *tig_cmd[] = { "$RFM_TERM","/usr/bin/tig",NULL};
static const char *git_show_pics_cmd[] = { "$RFM_TERM", rfmBinPath "/rfmGitShowPictures.sh",NULL};
//static const char *git_commit[] = { "$RFM_TERM", rfmBinPath "/rfmGitCommit.sh",NULL};
static const char *git_current_branch_cmd[] =  { "/usr/bin/git","branch","--show-current",NULL };
#endif

static RFM_builtinCMD builtinCMD[] = {
  //	{"q",      gtk_main_quit,         "alias for quit"},  //quit call cleanup now, not the same as q. we will solve this after we can pass parameters in function here
        {"exit",   gtk_main_quit,         "alias for quit"},
#ifdef PythonEmbedded
        {"py",     startPythonEmbedding,    "Embedding Python" },
        {"pyq",    endPythonEmbedding,      "quit Python Embedding" },
#endif
#ifdef RFM_FILE_CHOOSER
       	{"test_rfmFileChooser",Test_rfmFileChooser,"test rfmFileChooser" },
#endif
	{"onebyone",toggle_insert_fileAttributes_into_store_one_by_one,"toggle loading file one by one"},
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
 */

static RFM_MenuItem run_actions[] = {
   /* name           mime root        mime sub type            runCmd            		showCondition	*/
   { RunActionCopy,  "*",              "*",                    f_cp,             		NULL },
   { RunActionMove,  "*",              "*",                    f_mv,             		NULL },
   { RunActionDelete, "*",             "*",                    f_rm,             		NULL },
   { RunActionSL,    "*",              "*",                    create_softlink,                 NULL },
   { RunActionFindSL,"*",              "*",                    find_softlink,                   NULL },
#ifdef GitIntegration
   { RunActionGitStage,  "*",          "*",                    git_stage_cmd,    		cur_path_is_git_repo },
   { "git log",      "*",              "*",                    git_log_cmd,      		cur_path_is_git_repo },
// { "tig",          "*",              "*",                    tig_cmd,          		cur_path_is_git_repo },
#endif
   { "Properties",   "*",              "*",                    properties,       		NULL },
   { "Open with...", "*",              "*",                    open_with,        		NULL },
   { RunActionChangeOwner,"*",         "*",                    change_owner,                    NULL },
   { "Open",         "image",          "*",                    show_image,       		NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.text",          soffice,  	NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.spreadsheet",   soffice,  	NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.wordprocessingml.document", soffice,  NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.spreadsheetml.sheet",       soffice,  NULL },
   { "Open",         "application",    "msword",               soffice,          		NULL },
   { "Open",         "application",    "x-gnumeric",           gnumeric,         		NULL },
   { "Open",         "application",    "vnd.ms-excel",         gnumeric,         		NULL },
   { "Open",         "application",    "vnd.ms-excel",         soffice,          		NULL },
   { "view archive", "application",    "x-compressed-tar",     list_archive,                    NULL },
   { "view archive", "application",    "x-bzip-compressed-tar",list_archive,                    NULL },
   { "view archive", "application",    "x-xz-compressed-tar",  list_archive,                    NULL },
   { "view archive", "application",    "x-zstd-compressed-tar", list_archive,                   NULL },
   { "view archive", "application",    "zip",                  list_archive,                    NULL },
   { "view archive", "application",    "x-rpm",                list_archive,                    NULL },
   { "extract",      "application",    "x-compressed-tar",     extract_archive,  		NULL },
   { "extract",      "application",    "x-bzip-compressed-tar",extract_archive,  		NULL },
   { "extract",      "application",    "x-xz-compressed-tar",  extract_archive,  		NULL },
   { "extract",      "application",    "x-zstd-compressed-tar", extract_archive, 		NULL },
   { "extract",      "application",    "zip",                  extract_archive,  		NULL },
   { "extract",      "application",    "x-rpm",                extract_archive,  		NULL },
   { "View",         "application",    "pdf",                  pdf,             		NULL },
   { "View",         "application",    "epub+zip",             mupdf,            		NULL },
   { "View",         "application",    "x-troff-man",          man,              		NULL },
   { "Run",          "application",    "x-java-archive",       java,             		NULL },
   { "Open as text", "application",    "*",                    textEdit,         		NULL },
   { "Play",         "audio",          "*",                    play_audio,       		NULL },
   { "flac info",    "audio",          "flac",                 metaflac,         		NULL },
   { "info",         "audio",          "*",                    av_info,          		NULL },
   { "stats",        "audio",          "*",                    audioSpect,       		NULL },
   { "count",        "inode",          "directory",            du,               		NULL },
   { "archive",      "inode",          "directory",            create_archive,   		NULL },
   { "mount",        "inode",          "mount-point",          mount,            		NULL },
   { "unmount",      "inode",          "mount-point",          umount,           		NULL },
   { "mount",        "inode",          "blockdevice",          mount,            		NULL },
   { "unmount",      "inode",          "blockdevice",          umount,           		NULL },
   { "edit",         "text",           "*",                    textEdit,         		NULL },
   { "Open",         "text",           "html",                 www,              		NULL },
   { "Play",         "video",          "*",                    play_video,       		NULL },
   { "info",         "video",          "*",                    av_info,          		NULL },
   { "View",         "font",           "*",                    ftview,           		NULL },
};

/* Toolbar button definitions
 * icon is the name (not including .png) of an icon in the current theme.
 * function is the name of a function to be defined here. If function is NULL,
 * the RunCmd is assumed to be a shell command. All commands are run
 * async. The current working directory for
 * the command environment is changed to the currently displayed directory.
 * 
 * TODO: currently for functions, parameters are all rfmCtx if any. In future, we may need another column to specify a global variable as parameter for function. And we need to make rfmCtx global for this.
 * If both RunCmd and function is not NULL, function is run as callback after RunCmd finishs
 *
 * The difference between Toolbar button and file menu RunActions is that RunActions is for selected files while Toolbar RunCmd won't deal with selected files, it is for current directory */

//static const char *dev_disk_path[]= { "/dev/disk" };
//static void show_disk_devices(const char **path) {
//   set_rfm_curPath((char*)path[0]);
//}

static RFM_ToolButton tool_buttons[] = {
   /* name           icon                       function    		RunCmd      SearchResultView   DirectoryView   Accel                 tooltip                showCondition*/
   { Paste,          NULL,                      NULL,                   cp_clipboard_to_curPath, FALSE,    TRUE,      GDK_KEY_V,             "MOD+v",                 NULL},
   { MoveTo,         NULL,                      NULL,                   mv_clipboard_to_curPath, FALSE,    TRUE,      GDK_KEY_X,             "MOD+x",                 NULL},
   { LinkTo,         NULL,                      NULL,                   ln_clipboard_to_curPath, FALSE,    TRUE,      0,                     NULL,                    NULL},
   { PageUp,         NULL,                      PreviousPage,           NULL,              TRUE,           FALSE,     GDK_KEY_Page_Up,       "MOD+PgUp",              NULL},
   { PageDown,       NULL,                      NextPage,               NULL,              TRUE,           FALSE,     GDK_KEY_Page_Down,     "MOD+PgDn",              NULL},
   { SwitchView,     NULL,                     switch_iconview_treeview, NULL,             TRUE,           TRUE,      GDK_KEY_slash,         "MOD+/",                 NULL},
   { Up,             "go-up",                   up_clicked,             NULL,              FALSE,          TRUE,      GDK_KEY_Up,            "MOD+up arrow",          NULL},
   { "Refresh",      "view-refresh",            refresh_store,          NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Share",        NULL,                      NULL,                   share_dir,         FALSE,          TRUE,      0,                      "share directory",      NULL},
// { "Terminal",     "utilities-terminal",      NULL,                   term_cmd,          TRUE,           TRUE,      0,                      NULL,                   NULL},
// { "rfm",          "system-file-manager",     NULL,                   new_rfm,           TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Stop",         "process-stop",            rfm_stop_all,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Info",         "dialog-information",      info_clicked,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
// { "Home",         "go-home",                 home_clicked,           NULL,              FALSE,          TRUE,      0,                      NULL,                   NULL},
//#ifdef GitIntegration
// { "gitCommit",    NULL,                      refresh_store,          git_commit,        FALSE,          TRUE,      0,                      NULL,                   cur_path_is_git_repo  },
//#endif
   { cpPath,         NULL,                      copy_curPath_to_clipboard, NULL,           TRUE,           TRUE,      0,                      "copy current path to clipboard", NULL},
// { "mounts",       "drive-harddisk",          show_disk_devices,      dev_disk_path },
};


//exiftool is based on perl while exiv2 c++
//exiv2 -M'set Exif.Photo.UserComment 翠铜矿' filename   //the command used to set comment
//#define getImageSize  "exiftool %s |grep \"^Image Size\"|awk -F : '{print $2}'"
#define getImageSize  "exiv2 %s |grep \"^Image size\"|awk -F : '{print $2}'"
//#define getComment  "exiftool %s |grep \"^Comment\"|awk -F : '{print $2}'"
#define getComment  "exiv2 %s |grep \"^Exif comment\"|awk -F : '{print $2}'"
#define getMailFrom  "mu view %s |grep \"^From:\" |cut -c 6-"
#define getMailTo  "mu view %s |grep \"^To:\" |cut -c 4-"
#define getMailSubject "mu view %s |grep \"^Subject:\" |cut -c 9-"
#define getMailDate "mu view %s |grep \"^Date:\" |cut -c 6- | xargs -0 -I _ date -d _ '+%%Y/%%m/%%d-%%H:%%M:%%S'"
#define getMailAttachments  "mu view %s |grep \"^Attachments:\" |cut -c 13-"

//TODO:update it to your own Maildir or remove it if you don't have a Maidir
static const gchar* maildirs[] = { "/home/guyuming/Mail/", NULL };
static gboolean path_in_maildir(RFM_FileAttributes* fileAttribs){
	for (guint i=0; i<G_N_ELEMENTS(maildirs)-1; i++)
		if (fileAttribs==NULL ? (SearchResultViewInsteadOfDirectoryView ? TRUE : g_str_has_prefix(rfm_curPath,maildirs[i])) 
                                      : g_str_has_prefix(fileAttribs->path,maildirs[i])) 
			return TRUE;
        g_debug("path_in_maildir return false for %s", fileAttribs==NULL ? "NULL" : fileAttribs->path);
	return FALSE;
}

static RFM_treeviewColumn treeviewColumns[] = {
#ifdef GitIntegration
  {"Git",                     COL_GIT_STATUS_STR,         TRUE,   NULL, cur_path_is_git_repo, COL_GIT_STATUS_STR ,      NULL,            NULL,     "*",         "*",     FALSE,FALSE},
#endif
  {"Mode",                    COL_MODE_STR,               TRUE,   NULL, NULL,                 COL_MODE_STR,             NULL,            NULL,     "*",         "*",     FALSE,FALSE},
  //  COL_DISPLAY_NAME,
  {"FileName",                COL_FILENAME,               TRUE,   NULL, NULL,                 COL_FILENAME,             NULL,            NULL,     "*",         "*",     FALSE,FALSE},
#ifdef GitIntegration
  {"gitCommitMsg",            COL_GIT_COMMIT_MSG,         TRUE,   NULL, cur_path_is_git_repo, COL_GIT_COMMIT_MSG,       NULL,            NULL,     "*",         "*",     FALSE,FALSE},
#endif
  //  COL_FULL_PATH,
  //  COL_PIXBUF,
  //  COL_MTIME,
  {"MTime",                   COL_MTIME_STR,              TRUE,   NULL, NULL,                 COL_MTIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"Size",                    COL_SIZE,                   TRUE,   NULL, NULL,                 COL_SIZE,                 NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  // COL_ATTR,
  {"Owner",                   COL_OWNER,                  TRUE,   NULL, NULL,                 COL_OWNER,                NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"Group",                   COL_GROUP,                  TRUE,   NULL, NULL,                 COL_GROUP,                NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"LinkTarget",              COL_LINK_TARGET,            TRUE,   NULL, NULL,                 COL_LINK_TARGET,          NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"MIME_root",               COL_MIME_ROOT,              TRUE,   NULL, NULL,                 COL_MIME_SORT,            NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"MIME_sub",                COL_MIME_SUB,               TRUE,   NULL, NULL,                 COL_MIME_SUB,             NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  // COL_MIME_SORT, //mime root + sub for listview sort
  {"ATime",                   COL_ATIME_STR,              FALSE,  NULL, NULL,                 COL_ATIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"CTime",                   COL_CTIME_STR,              FALSE,  NULL, NULL,                 COL_CTIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE},
  {"C1",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C2",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C3",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C4",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C5",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C6",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C7",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C8",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C9",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"C10",                     NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE},
  {"ImageSize",               NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 getImageSize,    NULL,     "image",     "*",    FALSE,FALSE},
  {"Comment",                 NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 getComment,      NULL,     "image",     "*",    FALSE,FALSE},
  {"MailDate",                NUM_COLS,                   FALSE,  NULL, path_in_maildir,      NUM_COLS,                 getMailDate,     NULL,     "*",         "*",    FALSE,FALSE},
  {"MailFrom",                NUM_COLS,                   FALSE,  NULL, path_in_maildir,      NUM_COLS,                 getMailFrom,     NULL,     "*",         "*",    FALSE,FALSE},
  {"MailSubject",             NUM_COLS,                   FALSE,  NULL, path_in_maildir,      NUM_COLS,                 getMailSubject,  NULL,     "*",         "*",    FALSE,FALSE},
  {"MailAttachments",         NUM_COLS,                   FALSE,  NULL, path_in_maildir,      NUM_COLS,                 getMailAttachments,NULL,   "*",         "*",    FALSE,FALSE},
  {"MailTo",                  NUM_COLS,                   FALSE,  NULL, path_in_maildir,      NUM_COLS,                 getMailTo,       NULL,     "*",         "*",    FALSE,FALSE},
};

#define RFM_SearchResultLineSeperator ":"
static RFM_SearchResultType searchresultTypes[] = {
  {"default",                 ProcessOnelineForSearchResult }, //default searchresult, contain only filename list such as returned by locate, or filename followed by additional info such as returned by grep
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


static gchar shell_cmd_buffer[ARG_MAX]; //TODO: notice that this is shared single instance. But we only run shell command in sync mode now. so, no more than one thread will use this
static gchar* stdin_cmd_template_bash[]={"/bin/bash","-i","-c", shell_cmd_buffer, NULL};
static gchar* stdin_cmd_template_nu[]={"nu","-c", shell_cmd_buffer, NULL};
static gchar* stdin_cmd_template_bash_newVT_nonHold[] = { "$RFM_TERM", "/bin/bash", "-i", "-c", shell_cmd_buffer, NULL };
static gchar* stdin_cmd_template_nu_inNewVT_nonHold[] = { "$RFM_TERM", "nu", "-c", shell_cmd_buffer, NULL };
static gchar** stdin_command_bash(gchar* user_input_cmd, gboolean inNewVT) {
  if (inNewVT){
    sprintf(shell_cmd_buffer,"set -o history; %s;read -p '%s'; exit 2>/dev/null", strdup(user_input_cmd), strdup(PRESS_ENTER_TO_CLOSE_WINDOW));
    //without set -o history and "bash -i", bash builtin history command will not show results.
    //Seems to be by design, but why? for the nu shell, nu -c "history" just works
    //without exit, rfm will quit after commands such as ls, nano, but commands such as echo 1 will work. if you change exit to sleep 20, even echo 1 will make rfm terminate. Are there any race condition here?
    return stdin_cmd_template_bash_newVT_nonHold;
  }else {
    sprintf(shell_cmd_buffer,"set -o history; %s; exit 2>/dev/null",strdup(user_input_cmd));
    return stdin_cmd_template_bash;
  }
}
static gchar** stdin_command_nu(gchar* user_input_cmd, gboolean inNewVT) {
//TODO: add read -p into shell_cmd_buffer when inNewVT like stdin_command_bash
  sprintf(shell_cmd_buffer,"%s",g_strdup(user_input_cmd));
  return inNewVT? stdin_cmd_template_nu_inNewVT_nonHold : stdin_cmd_template_nu;
}


static stdin_cmd_interpretor stdin_cmd_interpretors[] = {
        //Name             activationKey          prompt            cmdTransformer
	{"Bash",           "b>",                  "b",              stdin_command_bash },
	{"NuShell",        "n>",                  "n",              stdin_command_nu   },
#ifdef PythonEmbedded
	{"PythonEmbedded", "p>",                  "p",              NULL               },
#endif
};


static gchar** rfmFileChooser_CMD(enum rfmTerminal startWithVT, gchar* search_cmd, gchar** defaultFileSelection, gchar* rfmFileChooserReturnSelectionIntoFilename){
    if (search_cmd == NULL || g_strcmp0(search_cmd,"")==0)
    	sprintf(shell_cmd_buffer, "rfm.sh -r %s", g_strdup(rfmFileChooserReturnSelectionIntoFilename));
    else
	sprintf(shell_cmd_buffer, "exec %s | rfm.sh -r %s -p", g_strdup(search_cmd), g_strdup(rfmFileChooserReturnSelectionIntoFilename));

    if (defaultFileSelection != NULL){
      strcat(shell_cmd_buffer, strdup(" -d"));
      for(int i=0; i<G_N_ELEMENTS(defaultFileSelection); i++)
	if (defaultFileSelection[i]!=NULL && strlen(defaultFileSelection[i])>0){
	  strcat(shell_cmd_buffer, strdup(" "));
	  strcat(shell_cmd_buffer, defaultFileSelection[i]);
	};
    };

    if (startWithVT == NEW_TERMINAL){
	return stdin_cmd_template_bash_newVT_nonHold;
    }else if (startWithVT == NO_TERMINAL){
	strcat(shell_cmd_buffer, strdup(" -t"));
        return stdin_cmd_template_bash;
    }else return stdin_cmd_template_bash;
}
