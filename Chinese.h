#define PageUp  "上葉"
#define PageDown  "下葉"
#define Up "父"
#define SwitchView "表/图"
#define Menu "菜单"
#define RunActionCopy "复制文件至目标路径或复制文件名至剪贴板"
#define RunActionMove "移动(改名)文件至目标路径或复制文件名至剪贴板"
#define RunActionDelete "删除"
#define RunActionSL "创建指向选中文件的软链接"
#define RunActionFindSL "查找(Find)指向同一文件的链接"
#define RunActionGitStage "git 暂存"
#define RunActionRefreshThumb "刷新缩略图"
#define Paste "粘帖"
#define MoveTo "移至此"
#define LinkTo "软链接"
#define RunActionChangeOwner "更改所有者"
#define SwitchPipeDir  "管道/目录"
#define cpPath "复制目录路径至剪贴板"

#define PipeTitle "当前:%d/总数:%d, 每葉:%d"

#define PRESS_ENTER_TO_CLOSE_WINDOW "按回车键关闭当前窗口"

#define BuiltInCmd_SearchResultColumnSeperator "SearchResultColumnSeperator"
#define BuiltInCmd_SearchResultColumnSeperator_Description "例如设置搜索结果列分割符为'&':  SearchResultColumnSeperator \"&\"   .不带参数则显示当前列分割符."
#define BuiltInCmd_toggleBlockGUI_Description "切换在gtk窗口线程或readline线程执行输入的命令,命令提示符相应显示 ] 或 > , ] 表示命令执行会阻塞图形窗口刷新"
#define BuiltInCmd_thumbnailsize               "thumbnailsize"
#define BuiltInCmd_thumbnailsize_Description  "设置文件图标(缩略图)大小,接受整数参数,如64,128或256"
#define BuiltInCmd_toggleInotifyHandler        "toggleInotifyHandler"
#define BuiltInCmd_toggleInotifyHandler_Description  "关/开自动刷新,比如当你浏览/dev目录时"
#define BuiltInCmd_pagesize                    "pagesize"
#define BuiltInCmd_pagesize_Description        "设置文件搜素结果视图每葉文件数, 如 pagesize 100"
#define BuiltInCmd_sort                        "sort"
#define BuiltInCmd_sort_Description            "设置排序列,效果等同点击列标题"
#define BuiltInCmd_glog                        "glog"
#define BuiltInCmd_glog_Description            "开/关glib log在命令行窗口的显示,命令glog off关闭日志输出,glog on打开. 作用主要是日志输出有时会干扰编辑器如nano的显示"
#define BuiltInCmd_showcolumn                  "showcolumn"
#define BuiltInCmd_showcolumns                 "showcolumns"
#define BuiltInCmd_showcolumn_Description      "显示或隐藏列(若当前在列表视图),以及改变列显示顺序, 并且当参数为空的时候,把当前列表视图列设置作为一条showcolumn命令加入命令历史,方便通过命令历史记录恢复当前列设置"
#define BuiltInCmd_showcolumns_Description     "showcolumn别名"
#define BuiltInCmd_setenv                      "setenv"
#define BuiltInCmd_setenv_Description          "设置rfm进程环境变量值,输入setenv命令查看使用方法"
#define BuiltInCmd_cd                          "cd"
#define BuiltInCmd_cd_Description              "\n" \
"    cd 地址     跳转到地址, 注意 PWD 环境变量值不会改变.也可用来在rfm窗口选中地址对应的文件名.\n" \
"    cd ..       进入当前目录父目录\n" \
"    cd -        进入OLDPWD环境变量对于目录,即上次打开的目录\n" \
"    cd空格      选中目录, cd加空格进入此目录; 选中文件,cd加空格进入此文件所在目录\n" \
"    cd不加空格  单独cd命令不接参数或空格输出当前所在目录,注意和pwd命令区别\n"
#define BuiltInCmd_help                        "help"
#define BuiltInCmd_help_Description            "显示命令帮助,加命令名参数则仅显示这条命令的帮助"

#define builtinCMD_Help \
"\n\033[32m 系统视图: \033[0m\n" \
"    按照数据源分两种: 一种是当前目录文件视图,此时,工具栏左边第一按钮显示目录路径(和git分支). 另一种是文件搜索结果视图,此时工具栏左边提示当前条/结果总数,每页条数, 点击此按钮在两种视图间切换.\n" \
"    按显示布局分为 图标 和 列表两种,图标视图显示文件缩略图\n" \
"\n\033[32m 命令提示: \033[0m\n" \
"    b*> 表示rfm视图有文件选中,且当前命令执行程序是Bash(命令执行程序在config.h里面可配置).\n" \
"        此时按TAB键,且TAB键前没有(非空格)文本,则自动插入当前选中文件完整路径, TAB前有非空字符,则调用readline默认的补全功能\n" \
"    b>  表示没有文件选中\n" \
"    b?> 表示正在刷新,无法确定选中文件数,刷新完成后回车重试\n" \
"    命令提示不随文件选中或取消自动更新, 用户可按回车键刷新\n" \
"    提示符 b*],b>,b?] 分别同上解释,区别在与,此时输入命令会由gtk窗口线程启动,而不是readline线程;也就是说,此时输入命令执行期间(比如编辑选中文件时),文件视图不会刷新,也不能更改选项\n" \
"\n\033[32m 创建Shell子进程执行的命令: \033[0m\n" \
"    非rfm内置命令会被发送到操作系统Shell执行,可在config.h内配置shell类型如bash, nushell等.\n" \
"    如果\033[33m命令末尾有空格\033[0m字符, 当前rfm视图选中文件会被添加到命令末尾. 例如你选中一个maildir目录里的邮件文件, 输入 mu view 并在回车前以空格结尾, 则可以查看选中邮件内容.\n" \
"    继续上面例子,如果你希望less查看邮件内容,可以选中邮件文件后输入 mu view %s|less .注意,回车前还是要以空格结束命令行,虽然实际替换文件名的位置在%s处,另外每个\033[33m%s\033[0m替换一个选中文件,若需多个文件名,可多加几个%s. 字符串%s也可以在config.h里定制成别的.\n" \
"    在上述非rfm内置命令后面加上\033[33m&\033[0m后缀,会打开新的终端模拟器窗口来执行此命令.注意这里&后缀的作用是rfm定制的,不是linux默认的把命令放在后台执行.也可以在config.h文件里把默认的&换成别的字符串\n" \
"\n\033[32m rfm内置命令: \033[0m\n" \
"    \033[33mquit\033[0m        退出rfm\n" \
"    \033[33mhelp\033[0m        输出本帮助信息\n" \
"    \033[33m连按两次回车键\033[0m刷新rfm显示内容\n" \
"    \033[33m/\033[0m           在 图标/列表 视图间切换,此操作不刷新数据\n" \
"    \033[33m//\033[0m          在当前目录和搜索结果视图间切换,此操作包含数据刷新\n" \
"    \033[33mpwd\033[0m         获取rfm进程 PWD 环境变量值\n" \
"    \033[33msetpwd\033[0m      设置rfm PWD 为当前显示目录\n" \

#define rfmLaunchHelp \
"这里是shell中启动rfm的命令行参数说明, rfm启动后的命令窗口内另有帮助提示在那里可若是在搜索结果文件列表界面,并选中文件或目录,则cd后接空格直接进入此文件所在目录,此时cd命令无需地址参用的命令.\n" \
"用法: %s [-h] [-H] [-d <目录完整路径>] [-i] [-v] [-l] [-p<每页显示文件数>] [-p] [-s<逗号分隔的列号序列>] [-S] [-t] [-T<缩略图尺寸>]\n" \
"      rfm窗前按q键退出.\n" \
"-p       通过管道,从标准输入读取文件名列表,此参数可省略(rfm自行检测是否有文件名列表通过管道输入,但若同时包含 -d 参数设置默认值,则-p不可省,参见 -d 参数说明), 比如:\n" \
"         locate 20230420|grep .png|rfm\n" \
"-px      当通过管道输入读取文件名列表时, 每次只显示x个文件, 比如: -p9. 你也可以在rfm启动后在命令行窗口使用内置命令pagesize来设置此每页显示文件数\n" \
"-d       显示路径指定的目录,而不是当前启动rfm的目录, 比如 -d /home/somebody/maildir. 如果-d 后跟的是文件名而不是目录名,则打开文件所在目录,并自动选择定位到此文件. 注意, 若要在文件搜索结果视图通过 -d 设置默认选中值,则 -p 参数不可省略,并且-p参数要出现在 -d 参数前面. 若-d后面跟多个文件名,只有第一个文件名的目录名会作为rfm启动后进入的目录.\n" \
"-i       显示mime类型\n" \
"-l       启动rfm后显示列表视图,而不是默认的图标视图,你也可以在打开rfm后使用内置命令/切换列表和图标视图\n" \
"-s       指定rfm启动后列表视图显示的列号序列,比如: -s,20,21,22  ,参见showcolumn内置命令\n" \
"-S       默认由gtk窗口线程启动输入命令\n" \
"-r       后接命名管道名称,表示退出rfm后,通过命名管道返回选中文件,用于把rfm作为文件选择器 rfmFileChooser 使用的场景,比如某应用的文件打开菜单对应功能\n" \
"         若后面没有命名管道名称,则通过printf输出选中文件.此时为了防止用户执行的其他命令输出干扰返回文件名,rfm启动后不建立readline线程,不接受命令输入,效果类似加了下面 -t 参数 \n" \
"-t       启动rfm后不创建readline线程,不显示命令输入提示,不接受命令输入.\n" \
"-T       设置缩略图尺寸,同上述thumbnailsize命令.\n" \
"-h       显示此帮助\n" \
"-H       关闭自动刷新\n" \
"-x       启动rfm后自动执行命令,例如 rfm.sh -x \"" \
BuiltInCmd_SearchResultColumnSeperator \
" '&'\"\n" \
"--" \
BuiltInCmd_SearchResultColumnSeperator \
"         显示默认搜索结果列分割符并退出rfm. 向rfm提供搜索结果输出的程序可以通过此参数获取rfm要求的分割符\n\n" \
"通过设置环境变量显示调试信息:G_MESSAGES_DEBUG=rfm, 这里rfm定义为log_domain(参见项目Makefile);代码中还定义了subdomain如rfm-data, rfm-gspawn, rfm-column 等(在config.h里查找完整subdomain列表), 所以,也可以设置环境变量G_MESSAGES_DEBUG=rfm-data,rfm-gspawn\n"

#define CURRENT_COLUMN_STATUS "当前列状态(负号表示隐藏),{v}或{^}表示排序方向(同列标题栏上的三角形小尖头)\n"

#define SHOWCOLUMN_USAGE \
"showncolumn 命令可包含一个或多个空格分隔的列号序列, 每个列号序列可以是单独的列号,或由逗号分隔的多个列号. 列号为负,表示不显示此列. 列号序列由逗号开头,表示第一个列号在第一列显示.列号序列由逗号结尾,表示最后一个列号后的列都不显示.showcolumn命令不带参数则显示当前列号列名显示状态和顺序.注意上述列号序列中数字表示的是列名对应的编号,而不是列显示的位置顺序号.\n\n" \
"举例:    showcolumn 10,9,-12   表示把9号列移到10号列后面显示,12号列移到9号列后并隐藏\n" \
"         showcolumn 1,3 12     表示把3号列移到1号列后面显示, 同时显示12号列,不移动12号列位置\n" \
"         showcolumn ,2         表示把2号列移到首位显示\n" \
"         showcolumn 2,         表示2号列后面的列都不显示\n" \
"除了点击列标题栏,还可以使用sort命令设置排序列, sort 后可以跟列号或列名;当sort后列号和当前排序列相同,则改变当前排序方向.\n" \
"设置环境变量G_MESSAGES_DEBUG=rfm-column-verbose 查看日志以便了解算法\n"

#define VALUE_MAY_NOT_LOADED \
"列号%d(%s)的值可能还没有加载,你可能需要刷新以便显示此列. 注意,如果切换列表/图标视图,此列可能出现在列表视图中,但如果不刷新,显示的可能是空数据\n"

#define SETENV_USAGE \
"用法:  setenv EnvVarName Value\n" \
"例子:  setenv G_MESSAGES_DEBUG rfm\n" \
"       setenv G_MESSAGES_DEBUG rfm,rfm-data,rfm-data-ext,rfm-data-thumbnail,rfm-data-search,rfm-data-git,rfm-gspawn,rfm-column,rfm-column-verbose,rfm-gtk\n" \
"       setenv G_MESSAGES_DEBUG \"\"\n" \
"获取当前值:\n" \
"       env |grep G_MESSAGES_DEBUG\n" \
"       注意,env命令实际是在子进程执行的,返回的实际上是子进程环境变量值.但每次命令执行都会创建子进程,并继承父进程的环境变量,因此可以通过子进程的环境变量值来推断父进程的值\n"

#define RFM_SEARCH_RESULT_TYPES "查询结果类型"
#define RFM_SEARCH_RESULT_TYPES_HELP "    在输出文件名列表的命令后加上 \033[33m>\033[0m0, 如 locate 202309|grep png >0 , 则可以在文件搜索结果视图显示文件列表,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n 这里>符号是rfm定制的,不是shell的文件输出重定向,可以在config.h里面换成别的字符串. 0表示下面default的查询结果类型,序号为0. \n" \
"    类似MIME类型和文件名后缀,查询结果类型也可以在config.h中用来定义文件上下文菜单项和默认打开选中文件的方式,比如在下面muview查询结果类型显示里,选中一封邮件回车会自动用mu view 命令查看邮件.\n" \
"    下面是全部的查询结果类型:"

#define SearchResultType_default "0号查询结果类型读取命令输出的行,每行可由分隔符分成多列(类似csv格式,config.h里包含分隔符默认值,可由内置命令或rfm启动参数动态定制,以配合不同的命令输出). 所有行包含列数相同. 首列必须是文件路径名. 命令输出不包含标题行,除首列文件名外,后续列在查询结果视图里依次自动分配列名C1,C2,C3....命令举例:下面命令输出第二列是行号,选中一个文件回车打开文本编辑器会跳转到此行,是因为创建shell子进程执行文件操作时会生成同名环境变量C1,C2...把列值传递到子进程,打开文本编辑器的脚本可以使用继承的环境变量\n" \
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
