/* Config file for rfm */

/*#define RFM_ICON_THEME "elementary"*/
/*#define RFM_SINGLE_CLICK "True"*/
#include <linux/limits.h>
#define RFM_TOOL_SIZE 22
#define RFM_ICON_SIZE 48
#define PIPE_SZ 65535      /* Kernel pipe size */ //TODO：如果 locate rfmTODO|rfm.sh 通过pipe传递的数据比这个大会怎样？
//#define RFM_CACHE_THUMBNAIL_IN_MEM TRUE
#define RFM_MX_MSGBOX_CHARS 1500 /* Maximum chars for show_msg output; messages exceeding this will be displayed in parent stdout */
#define RFM_MX_ARGS 128 /* Maximum allowed number of command line arguments in action commands below */
#define RFM_MOUNT_MEDIA_PATH "/run/media" /* Where specified mount handler mounts filesystems (e.g. udisksctl mount) */
#define RFM_MTIME_OFFSET 60      /* Display modified files as bold text (age in seconds) */
#define RFM_INOTIFY_TIMEOUT 500  /* ms between inotify events after which a full refresh is done */
#define RFM_DATETIME_FORMAT "%Y-%m-%d,%H:%M:%S"
#define RFM_HISTORY_SIZE 10000
#define RFM_AUTOSELECT_OLDPWD_IN_VIEW TRUE
#define RFM_FILE_CHOOSER_NAMED_PIPE_PREFIX "/tmp/rfmFileChooser_"
#define RFM_ENV_VAR_NAME_PREFIX ""  //change this if your extented column name conflicts with existing ENV Variable name
#define RFM_SEARCH_RESULT_AS_INPUT_PREFIX "SearchResult|"
#define RFM_SearchResultTypeNamePrefix ">"
#define RFM_NEW_TERM_SUFFIX "&"
#define RFM_selected_filename_placeholder "%s"
#define RFM_selected_filename_list_placeholder "%%"
//#define SEARCH_RFM_CMD_HISTORY "cat %s|grep -ih ^%s|sort|uniq"

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
#define RFM_LOG_GSPAWN_READ_PIPE "rfm-gspawn-readpipe"
#define RFM_LOG_COLUMN "rfm-column"
#define RFM_LOG_COLUMN_VERBOSE "rfm-column-verbose"
#define RFM_LOG_GTK "rfm-gtk"
#define RFM_LOG_READLINE "rfm-readline"
#define RFM_LOG_DIMENSION "rfm-dimension"

static gint RFM_THUMBNAIL_SIZE = 128; /* Maximum size for thumb dir normal is 128 */
static gboolean UseTargetAddressWhenEnterSymbloicLinkForDir=TRUE;
static gboolean keep_selection_on_view_across_refresh = TRUE;
static gchar* dumb_keyfile_groupname = "https://discourse.gnome.org/t/gkeyfile-to-handle-conf-without-groupname/23080/3";

//do not change the order of the following array,or change with care, it's referenced as stdin_cmd_interpretor[0] in code below
const stdin_cmd_interpretor stdin_cmd_interpretors[] = {
        //Name             activationKey          prompt            cmdTransformer       HistoryFilename       customCallbackFunc
	{"Bash",           "b>",                  "b",              stdin_command_bash,  ".rfm_history",       NULL },
	{"NuShell",        "n>",                  "n",              stdin_command_nu,    ".rfm_history",       NULL },
#ifdef PythonEmbedded
        {"PythonEmbedded", "p>",                  "p",              NULL,                ".rfm_history_python",NULL },
#endif
};

#ifdef RFM_AddOn
#define RFM_LOCAL_C_FILE "rfm_local.c"
#define RFM_LOCAL_ENTER_FUNC "rfm_local_enter"
#define RFM_LOCAL_LEAVE_FUNC "rfm_local_leave"
#define RFM_ADDON_RESOURCE_STRUCT "addon_resource"
#define RFM_ADDON_TYPE_VAR_NAME "addon_type"
// 上面这组定义对应的是rfm_local.c里面的变量和方法名，如果把这些定义放在rfm_local.h里面得话，可以实现
// rfm.c 和 rfm_local.c
// 的命名同步。但我觉得命名同步意义不大，写死不变代码其实更简练易读
// 留在这里 rfm.c 可以访问但rfm_local.c 编译时不能访问，我觉得也许可以通过单方面改变这里的定义值实现一些比如本地的版本升级控制，安全控制等，区分不同的 rfm_local.c, 名称和这个不同的就不能顺利加载。我并不是推荐这种容易造成混乱的方法，我觉得只是在不得已的情况下提供一种可能性。
static const gchar *global_addons_locating_cmd_template[] = { "rfmLocateGlobalAddons.sh",RFM_LOCAL_C_FILE,NULL };
static const gchar *rfm_local_c_compile_cmd_template = "gcc -O0 -g -shared -fPIC $(pkg-config --cflags --libs gtk+-3.0 json-glib-1.0) %s %s -D%s -D'RFM_SearchResultTypeNamePrefix=\"%s\"' -D'RFM_NEW_TERM_SUFFIX=\"%s\"' -D'RFM_selected_filename_placeholder=\"%s\"' -D'RFM_SEARCH_RESULT_AS_INPUT_PREFIX=\"%s\"' -o %s %s %s";
static const gchar *locate_addons[] = { stdin_cmd_interpretors[0].name, "locate --regex /" RFM_LOCAL_C_FILE "$|xargs grep 'const enum RFM_ADDON_TYPE addon_type ='" RFM_SearchResultTypeNamePrefix RFM_LOCAL_C_FILE };
#endif

static const gchar *global_dimension_properties_files_locating_cmd[] = {"rfmFind_DIMENSION_PROPERTIES_FILEs.sh",RFM_DIMENSION_PROPERTIES_FILE_NAME,NULL};

#ifdef CmarkTools
static gchar* MD_parsing_cmdTemplate[] = { stdin_cmd_interpretors[0].name, "extractJsonFromMarkdown 0 %s", NULL };
static gchar* TODO_MD_parsing_cmdTemplate[] = { stdin_cmd_interpretors[0].name, "extractJsonFromMarkdown 0 %s", NULL };
#endif
#ifdef RFM_AddOn
static gchar* AddOnStatusColumns[] = { fld_AddOnStatus,NULL };
#endif
static RFM_SearchResultType searchresultTypes[] = {
//name	       SearchResultLineProcessingFunc                  showcolumn     SearchResultColumnSeperator  cmdTemplate Description
  {"default",  ProcessOnelineForSearchResult,                  NULL,          NULL, NULL,                            SearchResultType_default }, //default searchresult, contain only filename list such as returned by locate, or filename followed by additional info such as returned by grep
  {SrchRsltType_multitype,CallMatchingProcessorForSearchResultLine,NULL,      NULL, NULL,                            SearchResultType_multitype },
  {"gkeyfile", ProcessKeyValuePairInFilesFromSearchResult,     NULL,          NULL, NULL,                            SearchResultType_gkeyfile},
#ifdef CmarkTools
  {".md",      ProcessJsonInCmdOutputFromSearchResult,         NULL,          NULL, MD_parsing_cmdTemplate,          SearchResultType_markdown1 },
  {"TODO.md",  ProcessJsonInCmdOutputFromSearchResult,         NULL,          NULL, TODO_MD_parsing_cmdTemplate,     SearchResultType_TODO_md },
#endif
  {"withTitle",ProcessOnelineForSearchResult_with_title,       NULL,          NULL, NULL,                            SearchResultType_default_with_title },
  {".json",    ProcessJsonFileFromSearchResult,                NULL,          NULL, NULL,                            "json key value"},
#ifdef RFM_AddOn
  {RFM_LOCAL_C_FILE,ProcessOnelineForSearchResult,        AddOnStatusColumns, NULL, NULL,                            "rfm addon status"},
#endif
};


//注意，下面命令中rfm选中文件占位符 RFM_selected_filename_placeholde 要配合结尾空格使用
static const char *parse_searchresult_with_multitype[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "tee" RFM_SearchResultTypeNamePrefix SrchRsltType_multitype ,NULL };
const char *Union_SearchResult[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "(tee;find -L "RFM_selected_filename_list_placeholder" -exec readlink -f {} \\;)|sort|uniq " RFM_SearchResultTypeNamePrefix "0 ",NULL };
const char *Union_SearchResult_multitype[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "(tee;find -L "RFM_selected_filename_list_placeholder" -exec readlink -f {} \\;)|sort|uniq " RFM_SearchResultTypeNamePrefix SrchRsltType_multitype " ",NULL };
static const char *Subtract_SearchResult[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "sort|comm -23 - <(find -L "RFM_selected_filename_list_placeholder" -exec readlink -f {} \\;|sort) "RFM_SearchResultTypeNamePrefix "0 ",NULL };
static const char *Intersect_With_SearchResult[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "sort|comm -12 - <(find -L "RFM_selected_filename_list_placeholder" -exec readlink -f {} \\;|sort) "RFM_SearchResultTypeNamePrefix "0 ",NULL };
static const char *To_SearchResult[] = { stdin_cmd_interpretors[0].name, "find -L "RFM_selected_filename_list_placeholder" -exec readlink -f {} \\;|sort|uniq "RFM_SearchResultTypeNamePrefix "0 ", NULL};
static const char *find_same_name[] = { stdin_cmd_interpretors[0].name, rfmBinPath "rfmFindFilesWithTheSameName.sh "RFM_SearchResultTypeNamePrefix SrchRsltType_multitype" ", NULL };

static const char *cd_in_searchresult[] = { stdin_cmd_interpretors[0].name, "cd ", NULL };
static const char *start_menu[] = { stdin_cmd_interpretors[0].name, "cd /usr/share/applications", NULL };
static const char *find_files[] = { stdin_cmd_interpretors[0].name,"find . -follow"RFM_SearchResultTypeNamePrefix"0", NULL };
static const char *extract_archive[] = { stdin_cmd_interpretors[0].name, "rfmExtractArchive.sh "RFM_NEW_TERM_SUFFIX" ", NULL };
static const char *mkdir_and_move_files_into_it[] = { stdin_cmd_interpretors[0].name, "rfm_mkdir_and_move_selected_files_into_it.sh "RFM_NEW_TERM_SUFFIX" ",NULL };
static const char *save_searchresult_to_file[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "tee>rfmSearchResult.csv", NULL };
static const char *filter_selected_searchresult[] = { stdin_cmd_interpretors[0].name, RFM_SEARCH_RESULT_AS_INPUT_PREFIX "tee>withTitle", NULL };

//there are two types of command definition string array here:
//1. those above, the first array item specify the command interpretor, and the second is the string you can input and run in rfm standard input command window, you can put everything in one string, and ending space means that selected file nanme will be appended, just as what happens when you input command in rfm standard input.
//2. those below, which is inherited from Rodney. You don't need to pass those array to a shell such as bash to run, but you need to separate string command into chunks. 
//see the code in file_menu_exec function for detail

/* rfmBinPath is passed in by compiler via Makefile*/
static const char *f_rm[]   = { "$RFM_TERM",rfmBinPath "rfmRemove.sh",NULL };
static const char *f_cp[]   = { "$RFM_TERM",rfmBinPath "rfmCopyMove.sh","cp", NULL };
static const char *f_mv[]   = { "$RFM_TERM",rfmBinPath "rfmCopyMove.sh", "mv",NULL };
static const char *cp_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "rfmCopyMoveToCurPath.sh","cp",NULL };
static const char *mv_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "rfmCopyMoveToCurPath.sh","mv",NULL };
static const char *ln_clipboard_to_curPath[] = { "$RFM_TERM",rfmBinPath "rfmCopyMoveToCurPath.sh","sl",NULL };
static const char *create_softlink_for_dimension[] = { rfmBinPath "rfmCreateSoftLink.sh",NULL};
static const char *create_softlink[] = { stdin_cmd_interpretors[0].name, "rfmCreateSoftLink.sh "RFM_NEW_TERM_SUFFIX" ",NULL };
static const char *find_softlink[] = { "$RFM_TERM",rfmBinPath "rfmFindLinksToTheSameFile.sh",NULL };
static const char *ShowSoftLinkToFileInSearchResult[] = { stdin_cmd_interpretors[0].name, "find -L \"$RFM_GIT_ROOT\" -not -path '*/.git/*' -exec test %s  -ef {} \\; -print"RFM_SearchResultTypeNamePrefix"0 ",NULL};
static const char *findDimensionValueForFile[] = { rfmBinPath "rfmFindDimensionValueForFile.sh",NULL};
static const char *change_owner[] = { "$RFM_TERM", rfmBinPath "rfmChangeOwner.sh",NULL };
static const char *play_video[] = { "/usr/bin/xdg-open", NULL };
static const char *play_audio[] = { "/usr/bin/xdg-open", NULL };
static const char *av_info[]    = { "/usr/bin/mediainfo", "-f", NULL };
static const char *textEdit[]   = { "$RFM_TERM", rfmBinPath "rfmTextEdit.sh", NULL };
static const char *bat[]        = { stdin_cmd_interpretors[0].name, "bat -p --paging=never ", NULL };
//static const char *pdf[]        = { "/usr/bin/xdg-open", NULL };
static const char *pdf[] = { "firefox-bin", "-P","pdf",NULL };
//open pdf with firefox-bin. you will be prompted to create a new firefox profile named pdf for the first run.
//Without a seperate profile, you will be asked to close existing instance of firefox, which is the reason why i don't use xdg-open now
//but i can start only one instance of firefox with this pdf profile, it is said that firefox will lock the profile
//i use binary package of firefox on gentoo, so firefox-bin, change it to something like firefox on your system.
static const char *mupdf[]      = { "/usr/bin/evince", NULL };
static const char *show_image[] = { rfmBinPath "rfmRefreshImage.sh", NULL };
static const char *soffice[]    = { "/usr/bin/xdg-open", NULL };
static const char *create_archive[]  = { rfmBinPath "createArchive.sh", NULL };
static const char *list_archive[] = { rfmBinPath "rfmVTforCMD.sh", "tar","--list","--file", NULL};
static const char *metaflac[] = { "/usr/bin/metaflac", "--list", "--block-type=VORBIS_COMMENT", NULL };
static const char *du[]       = { "/usr/bin/du", "-s", "-h", NULL };
static const char *mount[]    = { "$RFM_TERM", rfmBinPath "rfmMount.sh", NULL };
static const char *umount[]   = { "$RFM_TERM", "sudo","umount", NULL };
static const char *properties[]     = { "$RFM_TERM", rfmBinPath "rfmProperties.sh", NULL };
static const char *www[]        = { "/usr/bin/xdg-open", NULL };
static const char *man[]        = { "/usr/bin/groff", "-man", "-Tutf8", NULL };
static const char *java[]       = { "/usr/bin/java", "-jar", NULL };
static const char *audioSpect[] = { "/usr/local/bin/spectrogram.sh", NULL };
static const char *open_with[]  = { rfmBinPath "open_with_dmenu.sh", NULL };
static const char *gnumeric[]   = { "/usr/bin/gnumeric", NULL };
static const char *ftview[] = { "/usr/bin/ftview", "14", NULL }; /* pacman -S freetype2-demos */
static const char *ffmpegThumb[] =  { "/usr/bin/ffmpeg", "-i",RFM_selected_filename_placeholder, "-frames", "1", "-y","-s", thumbnailsize_str,NULL  };
static const char *geogebraThumb[] = { rfmBinPath "rfmMakeThumbnailForGGB.sh",thumbnailsize_str,NULL };
static const char *term_cmd[]  = { "$RFM_TERM", NULL };
static const char *new_rfm[]  = { "$RFM_TERM", rfmBinPath "rfm", NULL };
#ifndef PythonEmbedded
static const char *share_dir[] = { "$RFM_TERM", rfmBinPath "rfmShareDir.sh", NULL };
#endif
static const char *launch_desktop[] = { "dex", "--term", "$RFM_TERM", NULL };
//TODO: to get thumbnail from .desktop file
#ifdef GitIntegration
static const char *git_ls_files_cmd[] = {"git", "ls-files", "--deduplicate",NULL};
static const char *git_ls_files_untracked_cmd[] = { "git", "ls-files", "-o",NULL};
static const char *git_ls_files[] = { stdin_cmd_interpretors[0].name, "git ls-files --deduplicate ;git ls-files -o;git status --porcelain --no-rename|grep '^D'|awk '{print substr($0,4)}'"RFM_SearchResultTypeNamePrefix"0", NULL };
static const char *git_modified_staged_info_cmd[] = {"git","status","--porcelain","--no-rename",NULL};
static const char *git_stage_cmd[] = {"/usr/bin/git","stage",NULL};
static const char *git_root_cmd[] = { "bash","-c", "git rev-parse --is-inside-work-tree --show-toplevel --abbrev-ref HEAD --show-prefix; exit 0",NULL};
static const char *git_latest_log_cmd[] = {"/usr/bin/git","log","-1","--date=iso",NULL};
static const char *git_log_cmd[] = { rfmBinPath "rfmVTforCMD.sh","/usr/bin/git","log",NULL};
//现在只有很少的像上面这行的情况需要rfmVTforCMD_hold.sh脚本，如果想去掉它，统一用rfmVTforCMD.sh,就得想办法把类似read -p 这种代码插到上面的数组里，如果直接把 read 命令放在git log后面，NULL的前面，目前就会牺牲可以在末尾追加任意个选中文件的功能，我记得加一个""数组项可以表示替换一个文件
//若利用stdin_command_bash 这样的返回数组的函数，就意味着要增强文件上下文菜单的配置功能，让其能够接受函数，但这样会增加代码的复杂性，我认为上下文菜单应该保持只接受静态数组这么一种相对简单的配置方式
//TODO：所以，现在要么保持有些冗余的rfmVTforCMD_hold.sh脚本，要么增强把静态数组和当前选中文件列表合并的方式，增加一个在中间合并进全部文件的符号，这个可以和命令行中间使用%s符号替换文件名一起考虑
//static const char *tig_cmd[] = { "$RFM_TERM","/usr/bin/tig",NULL};
static const char *git_show_pics_cmd[] = { "$RFM_TERM", rfmBinPath "rfmGitShowPictures.sh",NULL};
//static const char *git_commit[] = { "$RFM_TERM", rfmBinPath "rfmGitCommit.sh",NULL};
#endif

static RFM_builtinCMD builtinCMD[] = {
  //	{"q",      gtk_main_quit,         "alias for quit"},  //quit call cleanup now, not the same as q. we will solve this after we can pass parameters in function here
	{BuiltInCmd_help,                      stdin_command_help,                     BuiltInCmd_help_Description},
	{BuiltInCmd_cd,                        cmd_cd,                                 BuiltInCmd_cd_Description},
        {"exit",                               cleanup,                                "alias for quit"},
#ifdef PythonEmbedded
        {"py",     startPythonEmbedding,    "Embedding Python" },
        {"pyq",    endPythonEmbedding,      "quit Python Embedding" },
        {"startshare",                         cmdStartShare,                          "start inprocess http/ftp server. ftp username 'rfm' hardcoded: startshare [--http] [--ftp] [--port xxxx(default 8080/2121 depends on http/ftp)] [--password xxxx(default empty)] [--no_firewall] [--perm xxxx(default elradfmw for ftp)]"},
        {"stopshare",                          cmdStopShare,                           "stop inprocess http/ftp server"},
#endif
#ifdef RFM_FILE_CHOOSER
       	{"test_rfmFileChooser",Test_rfmFileChooser,"test rfmFileChooser" },
#endif
	{"onebyone",toggle_insert_fileAttributes_into_store_one_by_one,"toggle loading file one by one"},
        {BuiltInCmd_SearchResultColumnSeperator,cmdSearchResultColumnSeperator,BuiltInCmd_SearchResultColumnSeperator_Description},
	{"toggleBlockGUI", toggle_exec_stdin_cmd_sync_by_calling_g_spawn_in_gtk_thread, BuiltInCmd_toggleBlockGUI_Description},
	{BuiltInCmd_thumbnailsize,              cmdThumbnailsize,                       BuiltInCmd_thumbnailsize_Description},
	{BuiltInCmd_toggleInotifyHandler,       cmdToggleInotifyHandler,                BuiltInCmd_toggleInotifyHandler_Description},
	{BuiltInCmd_pagesize,                   cmdPagesize,                            BuiltInCmd_pagesize_Description},
	{BuiltInCmd_sort,                       cmdSort,                                BuiltInCmd_sort_Description},
	{BuiltInCmd_glog,                       cmd_glog,                               BuiltInCmd_glog_Description},
	{BuiltInCmd_showcolumn,                 cmd_showcolumn,                         BuiltInCmd_showcolumn_Description},
	{BuiltInCmd_setenv,                     cmd_setenv,                             BuiltInCmd_setenv_Description},
	{BuiltInCmd_dimension,                  cmd_set_dimension_value_for_selected_files, BuiltInCmd_set_dimension_value_description},
	{"esc",                                 cmd_handle_escape_key,                  BuiltInCmd_ESC_Description},
	{ Up,					up_clicked,				BuiltInCmd_Up_Description},
        {Previous,                             prev_path_clicked,                       BuiltInCmd_Previous_Description},
        {Next,                                 next_path_clicked,                       BuiltInCmd_Next_Description},
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
   /* name           mime root        mime sub type            filenameSuffix           OrSearchResultType	func		runCmd            		showCondition	*/
   { RunActionCopy,  "*",              "*",                    NULL,			NULL,			NULL,		f_cp,             		NULL },
   { RunActionMove,  "*",              "*",                    NULL,			NULL,	refresh_store_on_searchresult,  f_mv,             		NULL },
   { RunActionDelete, "*",             "*",                    NULL,			NULL,   refresh_store_on_searchresult,	f_rm,             		NULL },
   { RunActionCD,    "*",              "*",                    NULL,                    NULL,                   NULL,           cd_in_searchresult,             InSearchResultView },
#ifdef GitIntegration
   { RunActionGitStage,  "*",          "*",                    NULL,			NULL,			refresh_store,  git_stage_cmd,    		show_gitMenus },
   { "git log",      "*",              "*",                    NULL,			NULL,			NULL,		git_log_cmd,      		show_gitMenus },
// { "tig",          "*",              "*",                    NULL,			NULL,			NULL,		tig_cmd,          		show_gitMenus },
#endif
   { "Properties",   "*",              "*",                    NULL,			NULL,			NULL,		properties,       		NULL },
   { "Open with...", "*",              "*",                    NULL,			NULL,			NULL,		open_with,        		NULL },
   { RunActionChangeOwner,"*",         "*",                    NULL,			NULL,			NULL,		change_owner,                   NULL },
   { RunActionMkdirMoveFiles,"*",      "*",                    NULL,                    NULL,                   NULL,           mkdir_and_move_files_into_it,   NULL },
   { "Open",         "image",          "*",                    NULL,			NULL,			NULL,		show_image,       		NULL },
   { "Open",         "application",    "x-desktop",            NULL,                    NULL,                   NULL,           launch_desktop,                 NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.text",          NULL,	NULL,			NULL,		soffice,  			NULL },
   { "Open",         "application",    "vnd.oasis.opendocument.spreadsheet",   NULL,	NULL,			NULL,		soffice,  			NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.wordprocessingml.document",NULL, NULL,NULL,		soffice,  			NULL },
   { "Open",         "application",    "vnd.openxmlformats-officedocument.spreadsheetml.sheet",NULL,       NULL,NULL,		soffice,  			NULL },
   { "Open",         "application",    "msword",               NULL,			NULL,			NULL,		soffice,          		NULL },
   { "Open",         "application",    "x-gnumeric",           NULL,			NULL,			NULL,		gnumeric,         		NULL },
   { "Open",         "application",    "vnd.ms-excel",         NULL,			NULL,			NULL,		gnumeric,         		NULL },
   { "Open",         "application",    "vnd.ms-excel",         NULL,			NULL,			NULL,		soffice,          		NULL },
   { "view archive", "application",    "x-compressed-tar",     NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "view archive", "application",    "x-bzip-compressed-tar",NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "view archive", "application",    "x-xz-compressed-tar",  NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "view archive", "application",    "x-zstd-compressed-tar",NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "view archive", "application",    "zip",                  NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "view archive", "application",    "x-rpm",                NULL,			NULL,			NULL,		list_archive,                   NULL },
   { "extract",      "application",    "x-compressed-tar",     NULL,			NULL,			NULL,		extract_archive,  		NULL },
   { "extract",      "application",    "x-bzip-compressed-tar",NULL,			NULL,			NULL,		extract_archive,  		NULL },
   { "extract",      "application",    "x-xz-compressed-tar",  NULL,			NULL,			NULL,		extract_archive,  		NULL },
   { "extract",      "application",    "x-zstd-compressed-tar",NULL,			NULL,			NULL,		extract_archive, 		NULL },
   { "extract",      "application",    "zip",                  NULL,			NULL,			NULL,		extract_archive,  		NULL },
   { "extract",      "application",    "x-rpm",                NULL,			NULL,			NULL,		extract_archive,  		NULL },
   { "View",         "application",    "pdf",                  NULL,			NULL,			NULL,		pdf,             		NULL },
   { "View",         "application",    "epub+zip",             NULL,			NULL,			NULL,		mupdf,            		NULL },
   { "View",         "application",    "x-troff-man",          NULL,			NULL,			NULL,		man,              		NULL },
   { "Run",          "application",    "x-java-archive",       NULL,			NULL,			NULL,		java,             		NULL },
   { "Open as text", "application",    "*",                    NULL,			NULL,			NULL,		textEdit,         		NULL },
   { "Play",         "audio",          "*",                    NULL,			NULL,			NULL,		play_audio,       		NULL },
   { "flac info",    "audio",          "flac",                 NULL,			NULL,			NULL,		metaflac,         		NULL },
   { "info",         "audio",          "*",                    NULL,			NULL,			NULL,		av_info,          		NULL },
   { "stats",        "audio",          "*",                    NULL,			NULL,			NULL,		audioSpect,       		NULL },
   { "count",        "inode",          "directory",            NULL,			NULL,			NULL,		du,               		NULL },
   { "archive",      "inode",          "directory",            NULL,			NULL,			NULL,		create_archive,   		NULL },
   { ToSearchResult, "*",              "*",                    NULL,			NULL,			NULL,		To_SearchResult,   		NULL },
   { UnionSearchResult,"*",            "*",                    NULL,			NULL,			InIconView,	Union_SearchResult,   	        InDirectoryView },
   { IntersectWithSearchResult,"*",    "*",                    NULL,			NULL,			InIconView,	Intersect_With_SearchResult,   	InDirectoryView },
   { SubtractSearchResult,"*",         "*",                    NULL,			NULL,			InIconView,	Subtract_SearchResult,   	InDirectoryView },
   { RunActionFindSoftLink,"*",        "*",                    NULL,			NULL,			NULL,		ShowSoftLinkToFileInSearchResult,NULL },
   { RunActionFindSL,"*",              "*",                    NULL,			NULL,			NULL,		find_softlink,                  NULL },
   { RunActionSL,    "*",              "*",                    NULL,			NULL,			NULL,		create_softlink,                NULL },
   { RunActionFindSameFile, "*",       "*",                    NULL,			NULL,			NULL,		find_same_name,                 NULL },
   { RunActionMount, "inode",          "directory",            NULL,			NULL,			NULL,		mount,            		NULL },
   { RunActionUmount,"inode",          "mount-point",          NULL,			NULL,			NULL,		umount,           		NULL },
   { "View",         "text",           "*",                    NULL,			NULL,			NULL,		bat,         		        NULL },
   { "edit",         "text",           "*",                    NULL,			NULL,			NULL,		textEdit,         		NULL },
   { "Open",         "text",           "html",                 NULL,			NULL,			NULL,		www,              		NULL },
   { "Play",         "video",          "*",                    NULL,			NULL,			NULL,		play_video,       		NULL },
   { "info",         "video",          "*",                    NULL,			NULL,			NULL,		av_info,          		NULL },
   { "View",         "font",           "*",                    NULL,			NULL,			NULL,		ftview,           		NULL },
   {RunActionRefreshThumb,"application","zip",                 ".ggb",			NULL,			refreshThumbnail,NULL,		                NULL },
   {RunActionRefreshThumb,"video",     "mp4",                  NULL,			NULL,			refreshThumbnail,NULL,		                NULL },
   {RunActionRefreshThumb,"image",     "*",                    NULL,			NULL,			refreshThumbnail,NULL,		                NULL },
#ifdef RFM_AddOn
   { "unload addon", "text",           "x-csrc",               RFM_LOCAL_C_FILE,        NULL,         unload_addon_filemenu_func,NULL,                          NULL },
#endif
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
   { Paste,          NULL,                      NULL,                   cp_clipboard_to_curPath, FALSE,    TRUE,      0,                     NULL,                    NULL},
   { MoveTo,         NULL,                      NULL,                   mv_clipboard_to_curPath, FALSE,    TRUE,      0,                     NULL,                    NULL},
   { LinkTo,         NULL,                      NULL,                   ln_clipboard_to_curPath, FALSE,    TRUE,      0,                     NULL,                    NULL},
   { PageUp,         NULL,                      PreviousPage,           NULL,              TRUE,           FALSE,     GDK_KEY_Page_Up,       "MOD1(usually ALT)+PgUp",NULL},
   { PageDown,       NULL,                      NextPage,               NULL,              TRUE,           FALSE,     GDK_KEY_Page_Down,     "MOD1(usually ALT)+PgDn",NULL},
   { SwitchView,     NULL,                     switch_iconview_treeview, NULL,             TRUE,           TRUE,      GDK_KEY_slash,         "MOD1(usually ALT)+/",   NULL},
#ifdef GitIntegration
// { "gitCommit",    NULL,                      refresh_store,          git_commit,        FALSE,          TRUE,      0,                      NULL,                   cur_path_is_git_repo  },
   { "git ls-files", NULL,                      NULL,                   git_ls_files,      TRUE,           TRUE,      0,                      NULL,                   show_gitMenus },
#endif
   { Up,             "go-up",                   up_clicked,             NULL,              FALSE,          TRUE,      GDK_KEY_Up,            BuiltInCmd_Up_Description,NULL},
   { Previous,       "go-previous",             prev_path_clicked,      NULL,              FALSE,          TRUE,      GDK_KEY_Left,          BuiltInCmd_Previous_Description,NULL},
   { Next,           "go-next",                 next_path_clicked,      NULL,              FALSE,          TRUE,      GDK_KEY_Right,         BuiltInCmd_Next_Description,NULL},
   { "Refresh",      "view-refresh",            refresh_store,          NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { FIND_FILES,     NULL,                      NULL,                   find_files,        FALSE,          TRUE,      0,                      NULL,                   NULL},
#ifndef PythonEmbedded
   { "Share",        NULL,                      NULL,                   share_dir,         FALSE,          TRUE,      0,                      "share directory",      NULL},
#endif
// { "Terminal",     "utilities-terminal",      NULL,                   term_cmd,          TRUE,           TRUE,      0,                      NULL,                   NULL},
// { "rfm",          "system-file-manager",     NULL,                   new_rfm,           TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Stop",         "process-stop",            rfm_stop_all,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
   { "Info",         "dialog-information",      info_clicked,           NULL,              TRUE,           TRUE,      0,                      NULL,                   NULL},
// { "Home",         "go-home",                 home_clicked,           NULL,              FALSE,          TRUE,      0,                      NULL,                   NULL},
   { cpPath,         NULL,                      copy_curPath_to_clipboard, NULL,           TRUE,           TRUE,      0,                      "copy current path to clipboard", NULL},
   { SAVESEARCHRESULT,NULL,                     NULL,                   save_searchresult_to_file,TRUE,    FALSE,     0,                      NULL,                   NULL },
   { FILTERSEARCHRESULT,NULL,                   NULL,                filter_selected_searchresult,TRUE,    FALSE,     0,                      NULL,                   InTreeView },
   { START_MENU,     NULL,                      NULL,                   start_menu,        FALSE,          TRUE,      0,                      NULL,                  NULL},
   { PARSE_SEARCH_RESULT, NULL,                 InIconView,        parse_searchresult_with_multitype, TRUE,FALSE,     0,                      NULL,                  NULL},
#ifdef RFM_AddOn
   { "addons",       NULL,                      NULL,                   locate_addons,     TRUE,           TRUE,      0,                      NULL,                  NULL},
#endif
// { "mounts",       "drive-harddisk",          show_disk_devices,      dev_disk_path },
};


//exiftool is based on perl while exiv2 c++
//exiv2 -M'set Exif.Photo.UserComment 翠铜矿' filename   //the command used to set comment
#define getImageSize  "exiftool %s |grep -i '^Image Size'|cut -d':' -f2"
//#define getImageSize  "exiv2 %s |grep \"^Image size\"|awk -F : '{print $2}'"
#define getComment  "exiftool %s |grep '^Comment'|awk -F : '{print $2}'"
//#define getComment  "exiv2 %s |grep \"^Exif comment\"|awk -F : '{print $2}'"


static RFM_treeviewColumn treeviewColumns_init[] = {
// title                      enumCol                    show     gtkCol showCondition       enumSortCol                ValueCmd        ValueFunc  MIME_root  MIME_sub  markup tooltip filenamesuffix 
#ifdef GitIntegration
  {"Git",                     COL_GIT_STATUS_STR,         TRUE,   NULL, show_gitColumns,      COL_GIT_STATUS_STR ,      NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
#endif
  {fld_thumbnail,             COL_PIXBUF,                 FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
  {"Mode",                    COL_MODE_STR,               TRUE,   NULL, NULL,                 COL_MODE_STR,             NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
  //  COL_DISPLAY_NAME,
  {fld_FileName,              COL_FILENAME,               TRUE,   NULL, NULL,                 COL_FILENAME,             NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
#ifdef GitIntegration
  {fld_gitCommitID,           COL_GIT_COMMIT_ID,          FALSE,  NULL, show_gitColumns,      COL_GIT_COMMIT_MSG,       NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
  {fld_gitCommitAuthor,       COL_GIT_AUTHOR,             FALSE,  NULL, show_gitColumns,      COL_GIT_COMMIT_MSG,       NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
  {fld_gitCommitDate,         COL_GIT_COMMIT_DATE,        FALSE,  NULL, show_gitColumns,      COL_GIT_COMMIT_MSG,       NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
  {fld_gitCommitMsg,          COL_GIT_COMMIT_MSG,         FALSE,  NULL, show_gitColumns,      COL_GIT_COMMIT_MSG,       NULL,            NULL,     "*",         "*",     FALSE,FALSE,NULL},
#endif
  //  COL_FULL_PATH,
  //  COL_PIXBUF,
  //  COL_MTIME,
  {fld_MTime,                 COL_MTIME_STR,              TRUE,   NULL, NULL,                 COL_MTIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_Size,                  COL_SIZE,                   FALSE,  NULL, NULL,                 COL_SIZE,                 NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_HumanReadableSize,     COL_SIZE_DISPLAY,           TRUE,   NULL, NULL,                 COL_SIZE,                 NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  // COL_ATTR,
  {fld_Owner,                 COL_OWNER,                  TRUE,   NULL, NULL,                 COL_OWNER,                NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_Group,                 COL_GROUP,                  TRUE,   NULL, NULL,                 COL_GROUP,                NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_LinkTarget,            COL_LINK_TARGET,            TRUE,   NULL, NULL,                 COL_LINK_TARGET,          NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_MIME_root,             COL_MIME_ROOT,              TRUE,   NULL, NULL,                 COL_MIME_SORT,            NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_MIME_sub,              COL_MIME_SUB,               TRUE,   NULL, NULL,                 COL_MIME_SUB,             NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  // COL_MIME_SORT, //mime root + sub for listview sort
  {fld_ATime,                 COL_ATIME_STR,              FALSE,  NULL, NULL,                 COL_ATIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
  {fld_CTime,                 COL_CTIME_STR,              FALSE,  NULL, NULL,                 COL_CTIME_STR,            NULL,            NULL,     "*",         "*",    FALSE,FALSE,NULL},
#ifdef RFM_AddOn
  {fld_AddOnStatus,           NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,            AddOnStatus,"text",   "x-csrc",FALSE,FALSE,RFM_LOCAL_C_FILE},
#endif
  {fld_ImageSize,             NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 getImageSize,    NULL,     "image",     "*",    FALSE,FALSE,NULL},
  {fld_Comment,               NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 getComment,      NULL,     "image",     "*",    FALSE,FALSE,NULL},
  {"C1",                      NUM_COLS,                   FALSE,  NULL, NULL,                 NUM_COLS,                 NULL,getExtColumnValueFromHashTable,"*","*",    FALSE,FALSE,NULL},
  //后面C2,C3,....在初始化时添加，注意C1 一定要是最后一个项, 虽然C1这一行放在这里有些奇怪，但如果放代码里初始化值，没有放这里静态数组里直观
};

static RFM_treeviewColumn treeviewColumns[G_N_ELEMENTS(treeviewColumns_init) + NUM_Ext_Columns - 1]; // 因为C1已经包含在treeviewColumns_init 里了，所以要-1

static char SearchResultColumnSeperator[32]={':', 0}; //this is the default value and can be override by specific searchresulttype and can also be set manually with builtin cmd



/* Thumbnailers
 * There must be at least one entry in the thumbnailers array.
 * To disable thumbnailing use:
 * static const RFM_Thumbnailers thumbnailers[]={{ NULL, NULL, NULL }};
 * Use thumbnail function set to NULL for built-in thumbnailer (gdk_pixbuf):
 * static const RFM_Thumbnailers thumbnailers[]={{ "image", "*", NULL }};
 * For other thumbnails, const gchar *thumbCmd  may be defined to handle that kind of thumbnail with shell command.
*/
static const RFM_Thumbnailer thumbnailers[] = {
    /* mime root      mime sub type    filenameSuffix    thumbCmd          check_tEXt */
    {"image",         "*",             NULL,             NULL,             TRUE  },
    {"video",         "mp4",           NULL,             ffmpegThumb,      TRUE  },
    //{ "application",  "dicom",       NULL,             dcmThumb     },
    {"application",   "zip",           ".ggb",           geogebraThumb,    FALSE },
};

#define Allow_Thumbnail_Without_tExtThumbMTime
#define MOD_KEY GDK_SUPER_MASK // the windows logo key
//#define MOD_KEY GDK_META_MASK // the Alt key

static gboolean ignored_filename(const gchar *name){
  if (basename(name)[0]=='.') return TRUE; /* Don't show hidden files */
  if (strstr(name,"/.")!=NULL) return TRUE;
  if (strcmp(name, "gmon.out")==0) return TRUE;
  return FALSE;
}



static RFM_cmd_regex_rule regex_rules[] = {
// pattern                          replacement                          action                           pattern_compiled
  { "^ \\+",                        "",                                  set_stdin_cmd_leading_space_true,NULL}, //这条规则会删除开始的空格
  { "^[S|s]earch[R|r]esult \\+|",   RFM_SEARCH_RESULT_AS_INPUT_PREFIX,   NULL,NULL },// \\+ 中第一个\是字符串里面用来escape第二个\的,\+是basic regular expression 要求的,相当于extended regular express 里面的+,表示一个或多个
  { "^[S|s]how[C|c]olumns* ",       "showcolumn ",                       NULL,NULL },
  { "^[S|s]how[C|c]olumns*$",       "showcolumn",                        NULL,NULL },
  { "^[E|e]build[S|s]amples*$",     "locate .ebuild|xargs grep inherit|grep cmake|cut -d: -f1 |xargs grep CMAKE_CXX_FLAGS", NULL,NULL},
  { "fzf",                          "fzf --multi --bind ctrl-a:select-all,ctrl-d:deselect-all",NULL,NULL },
  { "^[E|e][S|s][C|c]$",            "esc",                               NULL,NULL},
  { "^[E|e]$",                      "esc",                               NULL,NULL},
  { "^[U|u]$",			    Up,                                  NULL,NULL},
  { "^[L|l]$",                      Previous,                            NULL,NULL },
  { "^[R|r]$",                      Next,                                NULL,NULL },
};

//TODO: 增加一个类似前面定义 alias 的功能，或直接在 alias 功能里扩展， 看能不能结合 readline 库，输入一个alias, 直接替换成一个常用命令，可以编辑再执行，而不是向前面直接就给执行了但不显示真实命令
//locate .ebuild|xargs grep -il "inherit cmake" |xargs grep CMAKE_CXX_FLAGS>0
