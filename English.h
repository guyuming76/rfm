#define PageUp  "PageUp"
#define PageDown  "PageDown"
#define Up "UP"
#define SwitchView "List/Icon"
#define Menu "Menu"
#define RunActionCopy "Copy files to destination, or copy filenames to clipboard"
#define RunActionMove "Move(rename) files to destination, or copy filenames to clipboard"
#define RunActionDelete "Delete"
#define RunActionSL "Create SoftLink to selected files"
#define RunActionFindSL "Find all links to the same file"
#define RunActionGitStage "git stage"
#define RunActionRefreshThumb "Refresh Thumbnail"
#define Paste "Paste"
#define MoveTo "MoveToHere"
#define LinkTo "SoftLink"
#define RunActionChangeOwner "ChangeOwner"
#define SwitchPipeDir  "Pipe/Dir"
#define cpPath "copy directory name to clipboard"

#define PipeTitle "Current:%d/Total:%d, PageSize:%d"

#define PRESS_ENTER_TO_CLOSE_WINDOW   "Press Enter to close current window"

#define BuiltInCmd_SearchResultColumnSeperator "SearchResultColumnSeperator"
#define BuiltInCmd_SearchResultColumnSeperator_Description "例如设置搜索结果列分割符为'&':  SearchResultColumnSeperator \"&\"   .不带参数则显示当前列分割符."
#define BuiltInCmd_toggleBlockGUI_Description "切换在gtk窗口线程或readline线程执行输入的命令,命令提示符相应显示 ] 或 > , ] 表示命令执行会阻塞图形窗口刷新"
#define BuiltInCmd_thumbnailsize               "thumbnailsize"
#define BuiltInCmd_thumbnailsize_Description  "set thumbnail size in icon view with integer such as 64,128 or 256"
#define BuiltInCmd_toggleInotifyHandler        "toggleInotifyHandler"
#define BuiltInCmd_toggleInotifyHandler_Description  "stop/start auto refresh, when you are view /dev directory for example"
#define BuiltInCmd_pagesize                    "pagesize"
#define BuiltInCmd_pagesize_Description        "set files shown per page in search result view. for example, pagesize 100"
#define BuiltInCmd_sort                        "sort"
#define BuiltInCmd_sort_Description            "show or set sorting column"
#define BuiltInCmd_glog                        "glog"
#define BuiltInCmd_glog_Description            "开/关glib log在命令行窗口的显示,命令glog off关闭日志输出,glog on打开. 作用主要是日志输出有时会干扰编辑器如nano的显示"
#define BuiltInCmd_showcolumn                  "showcolumn"
#define BuiltInCmd_showcolumns                 "showcolumns"
#define BuiltInCmd_showcolumn_Description      "show or hide column (if currently in listview), or change column displaying order; current treeview column layout will be added in to command history if no parameter follows showcolumn"
#define BuiltInCmd_showcolumns_Description     "alias of showcolumn"
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
#define BuiltInCmd_help_Description            "显示命令帮助,加命令名参数则仅显示这条命令的帮助"

#define builtinCMD_Help \
"\n\033[32m system views: \033[0m\n" \
"    with difference in data source, two types of view can be shown, one is current directory view, with the left-most toolbar button showing its path(and git branch). The other is search result view. You can switch between these two views with the left-most toolbar button.\n" \
"    with difference in layout, there are also two types of view: icon view and list view, icons such as picture thumbnails are shown in icon view.\n" \
"\n\033[32m command prompt: \033[0m\n" \
"    b*> means there is selected file(s) in rfm view, and current command interpreter is Bash, which can be configured in config.h\n" \
"        press TAB with empty prefix will insert the current selected file fullpath; press TAB with non-empty prefix will call readline default completion function.\n" \
"    b>  means no selected files\n" \
"    b?> means in refreshing and file selection cannot be determined, try press enter after refresh.\n" \
"    prompt won't update when selection changes in rfm view, press Enter to refresh\n" \
"    prompt b*],b>,b?] differ with explanation above only in that commands inputted in stdin is started in gtk thread instead of readline thread. When ] is displayed instead of >, gtk window will not refresh and file selection cannot be changed when command is executing(such as when selected file is being editted).\n" \
"\n\033[32m 命令历史记录: \033[0m\n" \
"    命令历史记录保存在~/.rfm_history文件,当前活动rfm进程输入的命令历史保持在内存中,退出rfm时才写入文件,下次启动rfm时再读入内存.\n" \
"    在命令输入窗口使用上下箭头按键可以获取命令历史,也可以输入命令部分前缀缩小查找范围.\n" \
"    使用grep命令浏览或搜索命令历史记录: grep -nh \"\" ~/.rfm_history\n" \
"\n\033[32m Shell commands: \033[0m\n" \
"    non-builtin commands will be sent to shell to execute.\n" \
"    if there is \033[33mending space\033[0m in command entered, selected filename(s) will be appended at the end. for example, you can view currently selected maildir mail file with `mu view `, with ending space before return.\n" \
"    continue with the example above, if you want to view with less, you can use `mu view %s|less `. although \033[33m%s\033[0m in the command is replaced with selected filename, you still have to end the whole command line with space to trigger the filename replacing. one %s for one selected filename, if you choose multiple filenames, you can add more %s. %s can be configured in config.h" \
"    add \033[33m&\033[0m suffix at the end of non-builtin commands discussed above, a new terminal emulator will be opened to run this command. Note that this is rfm defined behavior. The linux default behavior for & suffix in command line means running command in background. & can be configured in config.h\n" \
"    命令前缀\033[33mSearchResult|\033[0m, 表示把当前文件搜索结果视图内容(若有选中行,则只包括选中行)作为输入传递给后面的命令. 如 SearchResult|tee>test1.txt 表示将查询结果输出保存到test1.txt文件内.\n" \
"\n\033[32m rfm builtin commands: \033[0m\n" \
"    \033[33mquit\033[0m        quit rfm\n" \
"    \033[33mhelp\033[0m        print this message\n" \
"    \033[33mpress Enter key twice (double enter)\033[0m to refresh rfm view\n" \
"    \033[33m/\033[0m           to switch between icon/list view, data not refreshed\n" \
"    \033[33m//\033[0m          to switch between current directory and search result view, with data refreshed\n" \
"    \033[33mpwd\033[0m         get rfm env PWD\n" \
"    \033[33msetpwd\033[0m      set rfm env PWD with current directory\n" \

#define rfmLaunchHelp \
"This is the help for command line argumants you can use to launch program with, there is another help in command window for commands you can use there.\n" \
"Usage: %s [-h] [-H] [-d <full path>] [-i] [-v] [-l] [-p<custom pagesize>] [-p] [-s] [-S] [-t] [-T<thumbnailsize>]\n" \
"       press q in rfm window to quit.\n" \
"-p       read file name list from StdIn, through pipeline, this -p can be omitted, for example:\n           locate 20230420|grep .png|rfm\n" \
"-px      when read filename list from pipeline, show only x number of items in a batch, for example: -p9. you can also set this in command window with builtin cmd pagesize\n" \
"-d       specify full path to show, such as -d /home/somebody/maildir, instead of default current working directory. If file name follows -d, the directory of the file will be opened and the file selected in view. If multiple path follows -d, rfm will enter the first directory name after start.\n" \
"-i       show mime type\n" \
"-l       open with listview instead of iconview,you can also switch view with toolbar button or builtin cmd /\n" \
"-s       specify columns to show in listview and their order, refer to builtin command showcolumn for detail.\n" \
"-S       default to started inputted command execution from gtk window thread\n" \
"-r       followed by named pipe through which file selection is returned. Used when rfm works as file chooser, such as with program file open menu.\n" \
"         if no named pipe follows, selection will be returned through printf, and rfm will start without readline thread, as if -t parameter is also added.\n" \
"-t       start rfm without readline thread, neither show command prompt nor accept command line input.\n" \
"-T       set RFM_THUMBNAIL_SIZE, same as thumbnailsize command above.\n" \
"-h       show this help\n" \
"-H       stop auto refresh\n" \
"-x       启动rfm后自动执行命令,例如 rfm.sh -x \"" \
BuiltInCmd_SearchResultColumnSeperator \
" '&'\"\n" \
"--" \
BuiltInCmd_SearchResultColumnSeperator \
"         显示默认搜索结果列分割符并退出rfm. 向rfm提供搜索结果输出的程序可以通过此参数获取rfm要求的分割符\n\n" \
"Show debug message by setting environmental variable: G_MESSAGES_DEBUG=rfm,rfm-data,rfm-gspawn,rfm-column, etc.,please refer to config.h for log subdomain definition\n"

#define CURRENT_COLUMN_STATUS "current column status(negative means invisible), {v}or{^} means sort ascending or descending (same as the little icon in column header)\n"

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
"For example: setenv G_MESSAGES_DEBUG rfm\n" \
"             setenv G_MESSAGES_DEBUG rfm,rfm-data,rfm-data-ext,rfm-data-thumbnail,rfm-data-search,rfm-data-git,rfm-gspawn,rfm-column,rfm-column-verbose,rfm-gtk\n" \
"             setenv G_MESSAGES_DEBUG \"\"\n" \
"Get current value with:\n" \
"             env |grep G_MESSAGES_DEBUG\n" \
"             note that env command will be run in as subprocess, so it actually return the env value of subprocess. However, since a new subprocess is created for each command, inheriting the environment value of parent, we can know the value in parent process by checking value in subprocess\n"

#define RFM_SEARCH_RESULT_TYPES "SearchResultTypes"
#define RFM_SEARCH_RESULT_TYPES_HELP "    在输出文件名列表的命令后加上 \033[33m>\033[0m0, 如 locate 202309|grep png >0 , 则可以在文件搜索结果视图显示文件列表,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n 这里>符号是rfm定制的,不是shell的文件输出重定向,可以在config.h里面换成别的字符串. 0表示下面default的查询结果类型,序号为0. \n" \
"    类似MIME类型和文件名后缀,查询结果类型也可以在config.h中用来定义文件上下文菜单项和默认打开选中文件的方式,比如在下面muview查询结果类型显示里,选中一封邮件回车会自动用mu view 命令查看邮件.\n" \
"    下面是全部的查询结果类型:"

#define SearchResultType_default "0号查询结果类型读取命令输出的行,每行可由分隔符分成多列(类似csv格式,config.h里包含分隔符默认值,可由内置命令或rfm启动参数动态定制,以配合不同的命令输出). 所有行包含列数相同. 首列必须是文件路径名. 命令输出不包含标题行,除首列文件名外,后续列在查询结果视图里依次自动分配列名C1,C2,C3...命令举例:下面命令输出第二列是行号,选中一个文件回车打开文本编辑器会跳转到此行,是因为创建shell子进程执行文件操作时会生成同名环境变量C1,C2...把列值传递到子进程,打开文本编辑器的脚本可以使用继承的环境变量\n" \
"      locate /etc/portage|xargs grep -ns emacs>default"

#define SearchResultType_gkeyfile "首先调用default查询结果类型获取命令输出行列,再用gtk keyfile 文件格式(等号分割的键值对行)解析首列文件内容,键名显示为列名,键值为列值. 每个keyfile文件内可包含不同的键名.命令举例,在rfm项目git仓库下,我建了一个devPicAndVideo子仓库,里面包含项目开发过程中的以下文档,可用下面命令查询问题列表:\n" \
"      locate .rfmTODO.gkeyfile>gkeyfile\n" \
"    至此,提到两种我们称之为\033[32m扩展列\033[0m(相对于文件系统内置的文件元数据,如权限,修改时间,文件大小等)的来源,一是程序标准输出里包含的除文件路径名之外的列,二是解析自文件内容的键值对数据.这里我们顺便提一下还有第三种来源:可以在config.h里面定义,然后通过showcolumn命令添加到显示,如现有的ImageSize列,这种列定义会包含一个取值bash命令或是c语言函数,根据传入的文件路径名,获取列值"

#define SearchResultType_muview "首先调用default查询结果类型获取命令输出行列,再使用 mu view 命令读取首列邮件文件内容,把邮件头,转换成前面keyfile要求的键值对格式,最后如同gkeyfile结果类型一样显示.命令举例(注意:Maidir中的文件名会包含冒号,同默认的SearchResultColumnSeperator冲突,要给文件名加引号解决.另外,下面我用cd 加ls而不是locate主要是考虑输出顺序, 而下面{}里的cd是运行于rfm创建的shell子进程,并不是前述rfm内置cd命令):\n" \
"      { cd /home/guyuming/Mail/139INBOX/cur; ls -1td \"$PWD\"/*; }>muview\n" \
"      上面命令也可以简化成如下形式,我这里保留上面作为一个使用bash {} 语法执行命令序列的备忘\n" \
"      ls -1td \"/home/guyuming/Mail/139INBOX/cur\"/*>muview"

#define SearchResultType_TODO_md "首先调用default结果类型获取命令输出行列,再使用rfm自带的独立程序extractKeyValuePairFromMarkdown,以markdown格式解析首列文件内容. extractKeyValuePairFromMarkdown 程序包含参数列表,以匹配markdown文件的一级标题,并输出gkeyfile格式键值对: 匹配到的markdown一级标题=一级标题下第一行内容. 最后同gkeyfile结果类型显示. 这里TODO.md类型会去匹配'简述'和'问题状态'这两个一级标题. 类似地,可以在config.h中定义新的查询结果类型,显示不同的markdown文件一级标题下的内容. 命令举例:\n" \
"      locate .rfmTODO.md>TODO.md"

#define SearchResultType_multitype "首先调用default结果类型,然后使用首列文件的MIME类型和文件名后缀去匹配同名的查询结果类型,调用匹配到的查询结果类型去处理此行,以达到在同一个结果页面显示多种文件格式内容的目的.命令举例:\n" \
"      locate .rfmTODO.>multitype\n" \
"    最后,在查询结果视图选中一条文件, 执行前述rfm内置的cd后接空格命令,就可以直接切换到包含此文件的目录.上面例子里文件是一条rfm TODO 事项描述文件,目录可以是为此事项专门建立,用以保存此事项相关图片等文档. 对比常见的关系数据库应用,查询结果里的扩展列相当于表字段; 我们用来在locate命令里匹配文件路径名的字符串起着关系数据库名及表名的作用; 两条不同表的记录的关联可以通过多种方式实现,最简单直观的就是两个文件位于同一文件目录下面. 上面例子若是在电子邮件系统里实现, rfmTODO 事项描述文件内容通常放在邮件正文里, 而事项相关文档都需要通过附件形式打包到邮件内, 也不如在rfm里面简单放在一个文件目录下方便."

#define SearchResultType_default_with_title "类似0号default查询结果类型,区别仅在于首行包含的是列标题,而不是实际数据"
