#define rfmLANGUAGE "rfmEnglish"

#define SrchRsltType_multitype "multitype"
#define MSG_cannot_enter_directory "\n\nCan't enter directory!"
#define MSG_cannot_enter_deleted_directory "\n\nPlease show content of deleted directory with the 'git ls-files' in toolbar"
#define PageUp  "PageUp"
#define PageDown  "PageDown"
#define Previous "Back"
#define Next "Forward"
#define SwitchView "List/Icon"
#define Menu "Menu"
#define RunActionCopy "Copy files to destination, or copy filenames to clipboard"
#define RunActionMove "Move(rename) files to destination, or copy filenames to clipboard"
#define RunActionDelete "Delete"
#define RunActionSL "File Set Operation: Create SoftLink to selected files"
#define RunActionFindSL "File Set Operation: Find soft links to files in new term"
#define RunActionGitStage "git stage"
#define RunActionRefreshThumb "Refresh Thumbnail"
#define RunActionMount "mount"
#define RunActionUmount "umount"
#define ToSearchResult "File Set Operation: To SearchResult"
#define UnionSearchResult  "File Set Operation: Union SearchResult"
#define IntersectWithSearchResult  "File Set Operation: Intersect With SearchResult"
#define SubtractSearchResult  "File Set Operation: Subtract SearchResult"
#define RunActionFindSoftLink "File Set Operation: find soft links to file within RFM_GIT_ROOT(env variable) directory"
#define RunActionFindSameFile "File Set Operation: Find and parse files with the same name (exclude links) within RFM_GIT_ROOT directory"
#define Paste "Paste"
#define MoveTo "MoveHere"
#define LinkTo "SoftLink"
#define RunActionChangeOwner "ChangeOwner"
#define RunActionCD "Enter parent directory of file"
#define RunActionMkdirMoveFiles "new directory and move files into it"
#define SwitchPipeDir  "Pipe/Dir"
#define cpPath "copy directory name to clipboard"
#define MENU_hideThisColumn "hide this column"
#define MENU_InsertBeforeThisColumn "insert before this column"
#define MENU_InsertAfterThisColumn "insert after this column"
#define START_MENU "Start menu"
#define SAVESEARCHRESULT "save (selected) search result into file"
#define FIND_FILES "find ."
#define LS_AI_DIALOG "ls AI dialog"
#define FILTERSEARCHRESULT "filter selected searchresult"
#define PARSE_SEARCH_RESULT "parse current search result with the " SrchRsltType_multitype " search result type"
#define SearchResult_contains_ColumnSeperator "查询结果内容包含当前设置的列分隔符 %s , 请使用SearchResultColumnSeperator命令设置一个和当前内容不冲突的列分割符，可使用多个连续字符以避免冲突.冲突内容:\n%s"

#define PipeTitle "Current:%d/Total:%d, PageSize:%d"
#define AI_MODEL_AS_CMD_INTERPRETER "Switch to the current AI model. The section below lists all candidate models. The first column is the command"

#define TOOL_TIP_PathAndRepositoryNameDisplay "click to switch between directory and search result view, or use // command; \npagesize xxx command can set file number per page at search result view"

#define PRESS_ENTER_TO_CLOSE_WINDOW   "Press Enter to close current window"

#define fld_FileName "file name"
#define fld_thumbnail "thumbnail"
#define fld_gitCommitID "git commit ID"
#define fld_gitCommitAuthor "git commit author"
#define fld_gitCommitDate "git commit date"
#define fld_gitCommitMsg "git commit comment"
#define fld_MTime "MTime"
#define fld_Size "size(byte)"
#define fld_HumanReadableSize "human readable size"
#define fld_Owner "owner"
#define fld_Group "group"
#define fld_LinkTarget "link target"
#define fld_MIME_root "MIME type"
#define fld_MIME_sub "MIME subtype"
#define fld_ATime "ATime"
#define fld_CTime "CTime"
#define fld_AddOnStatus "addon status"
#define fld_ImageSize "image size"
#define fld_Comment "image comment"

#define BuiltInCmd_SearchResultColumnSeperator "SearchResultColumnSeperator"
#define BuiltInCmd_SearchResultColumnSeperator_Description "for example, set search result column seperator to '&':  SearchResultColumnSeperator \"&\"   .show current search result column seperator if no parameter follows."
#define BuiltInCmd_toggleBlockGUI_Description "switch between running commands in gtk main loop or in readline thread. Accordingly, command prompt will show ] or  > , ] means that command runs in gtk main loop and will block the interaction in rfm main window"
#define BuiltInCmd_thumbnailsize               "thumbnailsize"
#define BuiltInCmd_thumbnailsize_Description  "set thumbnail size in icon view with integer such as 64,128 or 256"
#define BuiltInCmd_toggleInotifyHandler        "toggleInotifyHandler"
#define BuiltInCmd_toggleInotifyHandler_Description  "stop/start auto refresh, when you are view /dev directory for example"
#define BuiltInCmd_pagesize                    "pagesize"
#define BuiltInCmd_pagesize_Description        "set files shown per page in search result view. for example, pagesize 100"
#define BuiltInCmd_sort                        "sortby"
#define BuiltInCmd_sort_Description            "show or set sorting column"
#define BuiltInCmd_glog                        "glog"
#define BuiltInCmd_glog_Description            "turn on/off displaying of glib log in command window. command glog off turn off log, glog on turns on. interactive command such as nano won't be affected by gtk log if turned off."
#define BuiltInCmd_showcolumn                  "showcolumn"
#define BuiltInCmd_showcolumn_Description      "show or hide column (if currently in listview), or change column displaying order; current treeview column layout will be added in to command history if no parameter follows showcolumn"
#define BuiltInCmd_setenv                      "setenv"
#define BuiltInCmd_setenv_Description          "set environment varaiable for rfm, input setenv to see usage"
#define BuiltInCmd_cd                          "cd"
#define BuiltInCmd_cd_Description              "\n" \
"    cd address  go to address, note that PWD is not changed, just open address in rfm. can also be used to select file in rfm window.\n" \
"    cd ..       go up to parent directory.\n" \
"    cd -        go to directory in OLDPWD environment variable, that is, previously opened directory.\n" \
"    cd          cd followed by space key with directory selected will enter that directory, and will enter parent directory of a file if file selected.\n" \
"                cd without parameter or space just output current directory, which can be different from the output of pwd command.\n"
#define BuiltInCmd_help                        "help"
#define BuiltInCmd_help_Description            "show this command help, add command name as parameter to show help for that command only"
#define BuiltInCmd_dimension                   "dimension"
#define BuiltInCmd_set_dimension_value_description  "set dimension value for selected files,i.e.,create symbolic link to selected files under specific dimension value directory。argument(can work with TAB completion）: dimension name=dimension value"
#define BuiltInCmd_ESC_Description             "Unselect currently selected files; or press Alt + e in term window; the same effect as ESC in GUI"
#define Up "Up"
#define BuiltInCmd_Up_Description              "go to parent directory，short cut key: ALT + Up arrow（MOD1 usually is ALT）"
#define BuiltInCmd_Previous_Description        "Navigate to previous path，shortcut: ALT+Left arrow"
#define BuiltInCmd_Next_Description            "Navigate to next path，shortcut: ALT+Right arrow"

#ifdef reedline
	#define reedline_specific_help "    The '〉' after the prompt shows that input is read with the reedline library，which supports multiline edit，and ALT+Enter can be used for new line.\n"
#else
	#define reedline_specific_help ""
#endif

#ifdef reedline
       #define rfmEdit_help "    \033[33mrfmEdit\033[0m     followed by text filename to edit。rfmEdit works in rfm process with rust reedline crate，filename Tab completion is supported to make it easy to edit link in markdown\n"
#else
       #define rfmEdit_help ""
#endif

#define builtinCMD_Help \
"\n\033[32m system views: \033[0m\n" \
"    with difference in data source, two types of view can be shown, one is current directory view, with the left-most toolbar button showing its path(and git branch). The other is search result view. You can switch between these two views with the left-most toolbar button.\n" \
"    hidden files will not appear in current directory view, to see them, use command ls -a>0to show them in search result view.\n" \
"    search result view show files in pages, if some directory contains to many files, view them in search result view to improve performance. \n" \
"    with difference in layout, there are also two types of view: icon view and list view, icons such as picture thumbnails are shown in icon view.\n" \
"\n\033[32m command prompt: \033[0m\n" \
"    b*> means there is selected file(s) in rfm view, and current command interpreter is Bash, which can be configured in config.h\n" \
"        press TAB with empty prefix will insert the current selected file fullpath; otherwise, system TAB completion function such as filename completion will be called.\n" \
"    b>  means no selected files\n" \
"    b?> means in refreshing and file selection cannot be determined, try press enter after refresh.\n" \
"    prompt won't update when selection changes in rfm view, press Enter to refresh\n" \
"    prompt b*],b>,b?] differ with explanation above only in that commands inputted in stdin is started in gtk thread instead of readline thread. When ] is displayed instead of >, gtk window will not refresh and file selection cannot be changed when command is executing(such as when selected file is being editted).\n" \
reedline_specific_help \
"    ctrl-z in command window will send rfm.sh to background, and the command prompt will change to the shell prompt before rfm.sh launch. fg command can bring rfm.sh back to foreground the the command prompt will change to rfm prompt described above.\n" \
"\n\033[32m command history: \033[0m\n" \
"    command history is saved in file ~/.rfm_history. command history for current running rfm is kept in memory and saved into file on quit, and will be read into memory on next launch.\n" \
"    commond in history will be shown with uppper/down arrow key. enter command prefix can narrow down the scope.\n" \
"    search command history with grep: grep -nh \"\" ~/.rfm_history , or use TAB completion described above.\n" \
"\n\033[32m Shell commands: \033[0m\n" \
"    non-builtin commands will be sent to shell to execute.\n" \
"    if there is \033[33mending space\033[0m in command entered, selected filename(s) will be appended at the end. for example, you can view currently selected maildir mail file with `mu view `, with ending space before return.\n" \
"    continue with the example above, if you want to view with less, you can use `mu view %s|less `. although \033[33m%s\033[0m in the command is replaced with selected filename, you still have to end the whole command line with space to trigger the filename replacing. one %s for one selected filename, if you choose multiple filenames, you can add more %s. %s can be configured in config.h" \
"    add \033[33m&\033[0m suffix at the end of non-builtin commands discussed above, a new terminal emulator will be opened to run this command. Note that this is rfm defined behavior. The linux default behavior for & suffix in command line means running command in background. & can be configured in config.h\n" \
"    command prefix \033[33mSearchResult|\033[0m, means send current displaying search result with pipeline as input into following command (if files selected in search result view, only the selection will be sent). for example: SearchResult|tee>test1.txt  will save current displaying search result into file test1.txt.\n" \
"\n\033[32m rfm builtin commands: \033[0m\n" \
"    \033[33mquit\033[0m        quit rfm\n" \
"    \033[33mhelp\033[0m        print this message\n" \
"    \033[33mpress Enter key twice (double enter)\033[0m to refresh rfm view\n" \
"    \033[33m/\033[0m           to switch between icon/list view, data not refreshed\n" \
"    \033[33m//\033[0m          to switch between current directory and search result view, with data refreshed\n" \
"    \033[33mpwd\033[0m         get rfm env PWD\n" \
"    \033[33msetpwd\033[0m      set rfm env PWD with current directory\n" \
rfmEdit_help

#define rfmLaunchHelp \
"This is the help for command line argumants you can use to launch program with, there is another help in command window for commands you can use there.\n" \
"Usage: %s [-h] [-H] [-d <full path>] [-i] [-v] [-l] [-p<custom pagesize>] [-p] [-s] [-S] [-t] [-T<thumbnailsize>]\n" \
"       press q in rfm window to quit.\n" \
"-p       read file name list from StdIn, through pipeline, this -p can be omitted, for example:\n           locate 20230420|grep .png|rfm\n" \
"-px      when read filename list from pipeline, show only x number of items in a batch, for example: -p9. you can also set this in command window with builtin cmd pagesize\n" \
"-d       specify full path to show, such as -d /home/somebody/maildir, instead of default current working directory. If file name follows -d, the directory of the file will be opened and the file selected in view. If multiple path follows -d, rfm will enter the first directory name after start.\n" \
"-l       open with listview instead of iconview,you can also switch view with toolbar button or builtin cmd /\n" \
"-s       specify columns to show in listview and their order, refer to builtin command showcolumn for detail.\n" \
"-S       default to started inputted command execution from gtk window thread\n" \
"-r       followed by named pipe through which file selection is returned. Used when rfm works as file chooser, such as with program file open menu.\n" \
"         if no named pipe follows, selection will be returned through printf, and rfm will start without readline thread, as if -t parameter is also added.\n" \
"-t       start rfm without readline thread, neither show command prompt nor accept command line input.\n" \
"-T       set RFM_THUMBNAIL_SIZE, same as thumbnailsize command above.\n" \
"-h       show this help\n" \
"-H       stop auto refresh\n" \
"-x       automatically run command after rfm launch, for example: rfm.sh -x \"" \
BuiltInCmd_SearchResultColumnSeperator \
" '&'\"\n" \
"--" \
BuiltInCmd_SearchResultColumnSeperator \
"         show default search result column seperator and quit rfm. program that provide search result for rfm can use this command to get search result column seperator first.\n" \
"--disable-addon   do not auto load addons\n" \
"--no-gui  read filename list from pipe，and show processed result directly in stdout, without starting gtk GUI. can be used to process query on web server. example：locate rfm|head -n5|rfm.sh -l --no-gui 2>/dev/null\n\n" \
"Show debug message by setting environmental variable: G_MESSAGES_DEBUG=rfm,rfm-data,rfm-gspawn,rfm-column, etc.,please refer to config.h for log subdomain definition\n"

#define CURRENT_COLUMN_STATUS "current column status(negative means invisible), {v}or{^} means sort ascending or descending (same as the little icon in column header)\nCurrent sort column:%d;sort type:%d\n"

#define SHOWCOLUMN_USAGE \
"showncolumn command can have mulitple arguments deliminated with space, argument can be positive or negative column number, or (negative)column numbers connected with ',' or ';'. showcolumn without any argument display column name and number mapping, and current displaying status. Note that numbers in showcolumn argument are column number for column name, not the displaying position.\n\n" \
"Usage example: showcolumn 10,9,-12   means to move column 9 right after 10, and hide 12 right after 9\n" \
"               showcolumn 1,3 12     means to move column 3 right after 1, and set column 12 visible without changing its position\n" \
"               showcolumn ,2         means show column 2 as the first column\n" \
"               showcolumn 2,         means hide columns after 2\n" \
"besides clicking column header, sort column can be set with sort command, followed by column number or column name.\n" \
"set env variable G_MESSAGES_DEBUG=rfm-column-verbose and view log to get insight into the algorithm\n"

#define VALUE_MAY_NOT_LOADED \
"Value for column %d(%s) may has not been loaded yet, refresh needed. Note that if you just switch list/icon view, the column will appear, but with empty value before data refreshed\n"

#define SETENV_USAGE \
"Usage:       setenv EnvVarName Value\n" \
"For example: setenv G_MESSAGES_DEBUG all\n" \
"             setenv G_MESSAGES_DEBUG rfm\n" \
"             setenv G_MESSAGES_DEBUG rfm,rfm-data,rfm-data-ext,rfm-data-thumbnail,rfm-data-search,rfm-data-git,rfm-gspawn,rfm-column,rfm-column-verbose,rfm-gtk,rfm-readline\n" \
"             setenv G_MESSAGES_DEBUG \"\"\n" \
"Get current value with:\n" \
"             env |grep G_MESSAGES_DEBUG\n" \
"             note that env command will be run in as subprocess, so it actually return the env value of subprocess. However, since a new subprocess is created for each command, inheriting the environment value of parent, we can know the value in parent process by checking value in subprocess\n"

#define RFM_SEARCH_RESULT_TYPES "SearchResultTypes"
#define RFM_SEARCH_RESULT_TYPES_HELP "    search result can be displayed in search result view by adding \033[33m>\033[0m0 after search command, for example: locate 202309|grep png >0 ,which is the same as:locate 202309|grep png|rfm\n > here is defined in rfm, not shell. it can be changed to other charactor in config.h. 0 here means default search result type. \n" \
"    Similiar as MIME type and file name suffix, search result type can be used to define default file openning behavior in config.h. for example, in muview search result, select a mail and press enter will open mail with mu view command.\n" \
"    What follows are search result types:"

#define SearchResultType_default "search result type number 0 read command output, which can be seperated into columns(like csv format. the default seperator is defined in config.h, and can be set with builtin rfm command or startup parameter). All rows in search result have the same number of columns, and the first column must contain filename with full path. There will be no column title line in command output for this type and columns after Filename will be named C1,C2,C3... Example:the second column in the following command is line number. select one row in search result and press enter, file will be opened in editor and it will jump to the specific line number, because environment variables named C1,C2,C3... are created with the same value as the columns, and these environment variables will be inherited by the child process which opens the file with editor\n" \
"      locate /etc/portage|xargs grep -ns emacs>default"

#define SearchResultType_gkeyfile "default search result type will be called first to get command output, then the file in first column (Filename column) will be parsed with gtk keyfile format(key value pairs seperated by =). The parsed key value pair will be added as column title and value in search result. keyfiles in the same search result can have different keys. For example, I created a submodule named devPicAndVideo under rfm git repository to store documents and screenshots during development, where issue list can be queried with the following command:\n" \
"      locate .rfmTODO.gkeyfile>gkeyfile\n" \
"      So far, two kinds(or sources) of \033[32mextended column\033[0m(as opposed to the builtin file metadata columns such as permission, modification time,size,etc.) are mentioned, one is the columns included in command output. The other is the key value pair extracted from inside the file. By the way, there is a third type of extend column: defined in config.h, and added into view with showcolumn command. For example the ImageSize column in config.h"

#define	SearchResultType_markdown1 "default search result type will be called first to get command output, standalone programe extractKeyValuePairFromMarkdown in rfm project will be called to parse the file in search result row with markdown format; it will output key value pairs in gkeyfile format： markdown level one title = first line content under level one title. Finally, result will be show in the same way as gkeyfile result type. For example:\n" \
"      find -name *.TODO.md>.md"


#define SearchResultType_TODO_md "work in the same way as .md search result type except that only level one title '简述' and '问题状态' will be queried. For example:\n" \
"      locate .rfmTODO.md>TODO.md"

#define SearchResultType_multitype "default search result type will be called first to get command output, MIME type and filename suffix will be used to match search result type, and matched search result type will be called to process this row of search result. With this type, we can display content of different file formats in same view. For example:\n" \
"      locate .rfmTODO.>"SrchRsltType_multitype"\n" \
"      by the way, select one file in search result view and run rfm builtin cd command ending with space, we can go into the directory containning this selected file."

#define SearchResultType_default_with_title "The same as the default search result type, except that the first row in command output contains column title.for example, to filter out the first row in current search result:\n" \ 
"      SearchResult|head -n2>withTitle"




#define RFM_DIMENSION_PROPERTIES_FILE_NAME "rfmDimensionProperties"
#define RFM_DIMENSION_ID "UniqueID"
#define RFM_DIMENSION_NAME "Name"
#define RFM_DIMENSION_LOCAL_PATH "RemoteAddress"
