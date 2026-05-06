#define rfmLANGUAGE "rfmChinese"

#define RFM_DIMENSION_PROPERTIES_FILE_NAME "rfm维度属性"
#define RFM_DIMENSION_ID "唯一标识"
//原则上，名称就等于文件所在本层目录名，文件路径冲突时可以改目录名
#define RFM_DIMENSION_NAME "名称"
//当维度没有被跨仓库使用时，远程地址不必有。
#define RFM_DIMENSION_LOCAL_PATH "远程地址"

#define RFM_MSGBOX_THROTTLE_WARNING "系统显示对话框数量超限额，请查看系统日志"
#define RFM_ConfirmAddOnLoading "是否加载 %s 插件？"
#define RFM_ConcatVideos "合并视频(按文件名顺序)"

#define SrchRsltType_multitype "multitype"
#define SrchRsltType_current "currenttype"
#define MSG_cannot_enter_directory "\n\n不能进入目录!"
#define MSG_cannot_enter_deleted_directory "\n\n请使用工具栏上'git ls-files'命令查看已删除的目录内容"
#define PageUp  "上葉"
#define PageDown  "下葉"
#define Previous "后退"
#define Next "前进"
#define SwitchView "表/图"
#define Menu "菜单"
#define RunActionCopy "复制文件至目标路径或复制文件名至剪贴板"
#define RunActionMove "移动(改名)文件至目标路径或复制文件名至剪贴板"
#define RunActionDelete "删除"
#define RunActionSL "文件集合操作:创建指向选中文件的软链接"
#define RunActionFindSL "文件集合操作:在新窗口查找(find)指向选中文件的链接"
#define RunActionGitStage "git 暂存"
#define RunActionRefreshThumb "刷新缩略图"
#define RunActionMount "挂载(mount)"
#define RunActionUmount "取消挂载(umount)"
#define RunActionPlay "播放"
#define RunActionEdit "编辑"
#define ToSearchResult "文件集合操作:选中内容(包含目录子项)->查询结果(排序去重)"
#define UnionSearchResult  "文件集合操作:选中内容 合并 当前查询结果->新查询结果(排序去重)"
#define UnionSearchResultClipboard  "文件集合操作:剪贴板 合并 当前查询结果->新查询结果(排序去重)"
#define IntersectWithSearchResult  "文件集合操作:选中内容 交 当前查询结果->新查询结果"
#define SubtractSearchResult  "文件集合操作:当前查询结果 减去 选中内容->新查询结果"
#define RunActionFindSoftLink "文件集合操作:以环境变量RFM_GIT_ROOT值为范围,查找指向此单选文件的软链接"
#define Paste "粘帖"
#define MoveTo "移至此"
#define LinkTo "软链接"
#define RunActionChangeOwner "更改所有者"
#define RunActionFindSameFile "文件集合操作:以环境变量RFM_GIT_ROOT值为范围,查找同名文件(不含链接)并解析"
#define RunActionCD "进入选中文件所在目录"
#define RunActionMkdirMoveFiles "新建目录并把选中文件移入其中"
#define SwitchPipeDir  "管道/目录"
#define cpPath "复制目录路径至剪贴板"
#define MENU_hideThisColumn "隐藏此列"
#define MENU_InsertBeforeThisColumn "在此列前插入"
#define MENU_InsertAfterThisColumn "在此列后插入"
#define SAVESEARCHRESULT "保存（选中的）查询结果到文件"
#define FIND_FILES "find ."
#define LS_AI_DIALOG "ls AI对话"
#define FILTERSEARCHRESULT "过滤出选中查询结果"
#define PARSE_SEARCH_RESULT "用" SrchRsltType_multitype "查询结果类型解析当前查询结果"
#define SearchResult_contains_ColumnSeperator "查询结果内容包含当前设置的列分隔符 %s , 请使用SearchResultColumnSeperator命令设置一个和当前内容不冲突的列分割符，可使用多个连续字符以避免冲突. 冲突内容:\n%s"

#define PipeTitle "当前:%d/总数:%d, 每葉:%d"

#define AI_MODEL_AS_CMD_INTERPRETER "切换到当前AI模型回答模式，下面AI MODELS部分列出所有候选模型，首列是切换至此模型的命令"

#define TOOL_TIP_PathAndRepositoryNameDisplay "点击在目录视图和搜索结果间切换，或使用//命令; \n可用 pagesize xxx 命令设置搜索结果视图每葉条数"

#define PRESS_ENTER_TO_CLOSE_WINDOW "按回车键关闭当前窗口"

#define fld_FileName "文件名"
#define fld_thumbnail "缩略图"
#define fld_gitCommitID "git提交ID"
#define fld_gitCommitAuthor "git提交作者"
#define fld_gitCommitDate "git提交日期"
#define fld_gitCommitMsg "git提交摘要"
#define fld_MTime "MTime(修改时间)"
#define fld_Size "大小(字节)"
#define fld_HumanReadableSize "大小(易读格式)"
#define fld_Owner "所有者"
#define fld_Group "属组"
#define fld_LinkTarget "链接指向目标"
#define fld_MIME_root "MIME类型"
#define fld_MIME_sub "MIME子类型"
#define fld_ATime "ATime(访问时间)"
#define fld_CTime "CTime(创建时间)"
#define fld_AddOnStatus "插件状态"
#define fld_ImageSize "图片尺寸"
#define fld_Comment "图片备注"

#define BuiltInCmd_SearchResultColumnSeperator "SearchResultColumnSeperator"
#define BuiltInCmd_SearchResultColumnSeperator_Description "例如设置搜索结果列分割符为'&':  SearchResultColumnSeperator \"&\"   .不带参数则显示当前列分割符."
#define BuiltInCmd_toggleBlockGUI_Description "切换在gtk窗口线程或readline线程执行输入的命令,命令提示符相应显示 ] 或 > , ] 表示命令执行会阻塞图形窗口刷新"
#define BuiltInCmd_thumbnailsize               "thumbnailsize"
#define BuiltInCmd_thumbnailsize_Description  "设置文件图标(缩略图)大小,接受整数参数,如64,128或256"
#define BuiltInCmd_toggleInotifyHandler        "toggleInotifyHandler"
#define BuiltInCmd_toggleInotifyHandler_Description  "关/开自动刷新,比如当你浏览/dev目录时"
#define BuiltInCmd_toggleFileNameCompletionCaseSensitive        "toggleCaseSensitiveCompletion"
#define BuiltInCmd_toggleFileNameCompletionCaseSensitive_Description  "关/开文件名补全大小写敏感"
#define BuiltInCmd_pagesize                    "pagesize"
#define BuiltInCmd_pagesize_Description        "设置文件搜素结果视图每葉文件数, 如 pagesize 100"
#define BuiltInCmd_sort                        "sortby"
#define BuiltInCmd_sort_Description            "设置排序列,效果等同点击列标题"
#define BuiltInCmd_glog                        "glog"
#define BuiltInCmd_glog_Description            "开/关glib log在命令行窗口的显示,命令glog off关闭日志输出,glog on打开. 作用主要是日志输出有时会干扰编辑器如nano的显示"
#define BuiltInCmd_showcolumn                  "showcolumn"
#define BuiltInCmd_showcolumn_Description      "显示或隐藏列(若当前在列表视图),以及改变列显示顺序, 并且当参数为空的时候,把当前列表视图列设置作为一条showcolumn命令加入命令历史,方便通过命令历史记录恢复当前列设置"
#define BuiltInCmd_setenv                      "setenv"
#define BuiltInCmd_setenv_Description          "设置rfm进程环境变量值,输入" BuiltInCmd_setenv "命令查看使用方法,特别是设置系统调试日志输出类别的方法"
#define BuiltInCmd_cd                          "cd"
#define BuiltInCmd_cd_Description              "\n" \
"    cd 地址     跳转到地址, 注意 PWD 环境变量值不会改变.也可用来在rfm窗口选中地址对应的文件名.\n" \
"    cd ..       进入当前目录父目录\n" \
"    cd -        进入OLDPWD环境变量对于目录,即上次打开的目录\n" \
"    cd空格      选中目录, cd加空格进入此目录; 选中文件,cd加空格进入此文件所在目录\n" \
"    cd不加空格  单独cd命令不接参数或空格输出当前所在目录,注意和pwd命令区别\n"
#define BuiltInCmd_help                        "help"
#define BuiltInCmd_help_Description            "显示命令帮助,加命令名参数则仅显示这条命令的帮助"
#define BuiltInCmd_dimension                   "维度"
#define BuiltInCmd_set_dimension_value_description  "为选中文件设置维度值，也就是在相应维度值目录下创建指向选中文件的符号链接。命令参数（可通过TAB补全输入）: 维度名=维度值 .命令名可以省略,也就是说可以直接在命令行输入 维度名=维度值 ,就可以为选中文件创建符号链接"
#define BuiltInCmd_ESC_Description	       "取消当前文件选中;或在命令窗口使用esc键"
#define Up "父"
#define BuiltInCmd_Up_Description              "进入上级目录，可使用组合键 ALT+向上箭头（MOD1通常就是ALT键）"
#define BuiltInCmd_Previous_Description        "后退到历史路径，快捷键：ALT+左箭头"
#define BuiltInCmd_Next_Description            "前进到历史路径，快捷键：ALT+右箭头"
#define BuiltInCmd_add2search                  "添加完整路径文件名参数至查询结果视图，作用和文件上下文菜单中 \"" UnionSearchResult "\" 相同"
#ifdef reedline
	#define reedline_specific_help "    提示符后的'〉'表示当前用reedline库读取命令输入，支持多行编辑，ALT+Enter可以用来换行;如果Alt+Enter被占用（比如动态平铺窗口管理器可能用其来移动窗口），可以使用Ctrl+j.\n"
#else
	#define reedline_specific_help ""
#endif

#ifdef reedline
	#define rfmEdit_help "    \033[33mrfmEdit\033[0m     后接文本文件名，编辑文本文件。rfmEdit命令使用reedline库，工作在rfm进程内，可以使用文件名Tab补全，方便编辑markdown文件里的链接\n"
#else
	#define rfmEdit_help ""
#endif

#define builtinCMD_Help \
"\n\033[32m 系统视图: \033[0m\n" \
"    按照数据源分两种: 一种是当前目录文件视图,此时,工具栏左边第一按钮显示目录路径(和git分支). 另一种是文件搜索结果视图,此时工具栏左边提示当前条/结果总数,每葉条数, 点击此按钮在两种视图间切换.\n" \
"    在当前文件目录视图，隐藏文件是不显示的，但是会显示在查询结果视图，可以使用命令 ls -a>0显示当前目录隐藏文件.\n" \
"    查询结果视图的另一个好处是分葉显示，如果某个目录包含文件太多，可以使用查询结果视图显示.\n" \
"    按显示布局分为 图标 和 列表两种,图标视图显示文件缩略图\n" \
"\n\033[32m 命令提示: \033[0m\n" \
"    b*> 表示rfm视图有文件选中,且当前命令执行程序是Bash(命令执行程序在config.h里面可配置).\n" \
"        此时按TAB键,且TAB键前没有(非空格)文本,则自动插入当前选中文件完整路径,详见下面TAB补全一节.\n" \
"    b>  表示没有文件选中\n" \
"    b?> 表示正在刷新,无法确定选中文件数,刷新完成后回车重试\n" \
"    命令提示不随文件选中或取消自动更新, 用户可按回车键刷新\n" \
"    提示符 b*],b>,b?] 分别同上解释,区别在与,此时输入命令会由gtk窗口线程启动,而不是readline线程;也就是说,此时输入命令执行期间(比如编辑选中文件时),文件视图不会刷新,也不能更改选项\n" \
reedline_specific_help \
"    在命令输入窗口按ctrl-z会把rfm.sh进程放入后台运行，命令提示符会变成启动rfm.sh前的shell提示符。在此状态下，若运行htop等交互命令异常，可先运行reset重置终端模拟器。运行fg命令可将rfm.sh任务恢复到前台运行，命令提示符恢复成上述rfm提示符.\n" \
"\n\033[32m TAB补全: \033[0m\n" \
"    在config.mk中使用reedline库编译rfm,补全菜单交互较方便\n" \
"    1. 基于当前选中文件\n" \
"        当前有文件被选中，且光标前字符为空时，TAB不提示补全菜单，自动补全当前选中文件（若多选，只有一个有效）.\n" \
"    2.\033[32m文件名补全\033[0m\n" \
"        若没有文件选中，则显示文件名补全菜单，使用类似ls命令返回内容的颜色方案。默认内容来自当前目录， shift+TAB 则补全内容来自当前目录及子目录。可以使用"BuiltInCmd_toggleFileNameCompletionCaseSensitive"命令切换匹配文件名时是否区分大小写。同时补全菜单还会同时包含下面描述的黄色和红色内容\n" \
"    3.\033[31m基于查询结果页面的列标题和值\033[0m\n" \
"        当前为查询结果页面时，且包含config.h中定义的取值函数为getExtColumnValueFromHashTable的扩展列，当输入行光标前为空的时候，TAB补全菜单会出现\033[31m红色\033[0m内容的扩展标题列名。选中红色标题列名补全后，再继续TAB补全，则会出现红色的此列值的补全内容。在补全输入列标题名后，可以手工输入其他符号如等号(=),然后再按TAB补全列值。\n" \
"    4.\033[33m基于维度定义\033[0m\n" \
"        在文件目录内加入名为 " RFM_DIMENSION_PROPERTIES_FILE_NAME " 的文件，则此目录会被rfm定义唯一维度，其下子目录名都是此维度的成员。\n" \
"        维度名和成员在TAB补全菜单中显示为\033[33m黄色\033[0m。\n" \
"        TAB补全完成维度名输入后，接着按TAB键，会出现相应维度成员值补全内容，值前会自动加等号分隔，不要手工输入。\n" \
"\n\033[32m Menu键补全: \033[0m\n" \
"    在config.mk中使用reedline库编译rfm。在终端按Menu键，则会根据当前rfm图形窗口有无文件选中，相应弹出文件上下文菜单补全或工具栏按钮补全菜单，黄色显示，后面绿色项是内置命令列表。可以不离开终端窗口执行图形界面操作。\n" \
"\n\033[32m 命令历史记录: \033[0m\n" \
"    命令历史记录保存在~/.rfm_history文件,当前活动rfm进程输入的命令历史保持在内存中,退出rfm时才写入文件,下次启动rfm时再读入内存.\n" \
"    在命令输入窗口使用上下箭头按键可以获取命令历史,也可以输入命令部分前缀缩小查找范围.\n" \
"    使用grep命令浏览或搜索命令历史记录: grep -nh \"\" ~/.rfm_history ，或使用上述TAB补全\n" \
"\n\033[32m 创建Shell子进程执行的命令: \033[0m\n" \
"    非rfm内置命令会被发送到操作系统Shell执行,可在config.h内配置shell类型如bash, nushell等.\n" \
"    如果\033[33m命令末尾有空格\033[0m字符, 当前rfm视图选中文件会被添加到命令末尾. 例如你选中一个maildir目录里的邮件文件, 输入 mu view 并在回车前以空格结尾, 则可以查看选中邮件内容.\n" \
"    继续上面例子,如果你希望less查看邮件内容,可以选中邮件文件后输入 mu view %s|less .注意,回车前还是要以空格结束命令行,虽然实际替换文件名的位置在%s处,另外每个\033[33m%s\033[0m替换一个选中文件,若需多个文件名,可多加几个%s. 字符串%s也可以在config.h里定制成别的.\n" \
"    在上述非rfm内置命令后面加上\033[33m&\033[0m后缀,会打开新的终端模拟器窗口来执行此命令.注意这里&后缀的作用是rfm定制的,不是linux默认的把命令放在后台执行.也可以在config.h文件里把默认的&换成别的字符串\n" \
"    命令前缀\033[33mSearchResult|\033[0m, 表示把当前文件搜索结果视图内容(若有选中行,则只包括选中行)作为输入传递给后面的命令. 如 SearchResult|tee>test1.txt 表示将查询结果输出保存到test1.txt文件内. 注意，这里说的是当前搜索结果视图内容，而不是搜索结果，这么设计是因为如果要搜索结果，可以直接从命令历史记录里翻出搜索的命令，加上管道操作就可以了。另外，搜索结果视图上的数据是搜索结果通过搜索结果类型处理后的结果，内容也不同于搜索结果，参见查询结果类型描述\n" \
"\n\033[32m rfm内置命令: \033[0m\n" \
"    \033[33mquit\033[0m        退出rfm\n" \
"    \033[33mhelp\033[0m        输出本帮助信息\n" \
"    \033[33m连按两次回车键\033[0m刷新rfm显示内容\n" \
"    \033[33m/\033[0m           在 图标/列表 视图间切换,此操作不刷新数据\n" \
"    \033[33m//\033[0m          在当前目录和搜索结果视图间切换,此操作包含数据刷新\n" \
"    \033[33mpwd\033[0m         获取rfm进程 PWD 环境变量值\n" \
"    \033[33msetpwd\033[0m      设置rfm PWD 为当前显示目录\n" \
rfmEdit_help

#define rfmLaunchHelp \
"这里是shell中启动rfm的命令行参数说明, rfm启动后的命令窗口内另有帮助提示在那里可若是在搜索结果文件列表界面,并选中文件或目录,则cd后接空格直接进入此文件所在目录,此时cd命令无需地址参用的命令.\n" \
"用法: %s [-h] [-H] [-d <目录完整路径>] [-i] [-v] [-l] [-p<每页显示文件数>] [-p] [-s<逗号分隔的列号序列>] [-S] [-t] [-T<缩略图尺寸>]\n" \
"      rfm窗前按q键退出.\n" \
"-p       通过管道,从标准输入读取文件名列表,此参数可省略(rfm自行检测是否有文件名列表通过管道输入,但若同时包含 -d 参数设置默认值,则-p不可省,参见 -d 参数说明), 比如:\n" \
"         locate 20230420|grep .png|rfm\n" \
"-px      当通过管道输入读取文件名列表时, 每次只显示x个文件, 比如: -p9. 你也可以在rfm启动后在命令行窗口使用内置命令pagesize来设置此每页显示文件数\n" \
"-d       显示路径指定的目录,而不是当前启动rfm的目录, 比如 -d /home/somebody/maildir. 如果-d 后跟的是文件名而不是目录名,则打开文件所在目录,并自动选择定位到此文件. 注意, 若要在文件搜索结果视图通过 -d 设置默认选中值,则 -p 参数不可省略,并且-p参数要出现在 -d 参数前面. 若-d后面跟多个文件名,只有第一个文件名的目录名会作为rfm启动后进入的目录.\n" \
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
"         显示默认搜索结果列分割符并退出rfm. 向rfm提供搜索结果输出的程序可以通过此参数获取rfm要求的分割符\n" \
"--disable-addon   不自动加载插件\n" \
"--no-gui  从管道读取输入，直接在终端模拟器输出查询结果，而不启动rfm图形界面，可用在web服务器上处理用户查询,举例：locate rfm|head -n5|rfm.sh -l --no-gui 2>/dev/null\n\n" \
"通过设置环境变量显示调试信息:G_MESSAGES_DEBUG=rfm, 这里rfm定义为log_domain(参见项目Makefile);代码中还定义了subdomain如rfm-data, rfm-gspawn, rfm-column 等(在rfm_addon.h里查找完整subdomain列表), 所以,也可以设置环境变量G_MESSAGES_DEBUG=rfm-data,rfm-gspawn\n" \
"         若是在命令窗口看到gtk显示的一些critical及其它级别消息，可以设置G_DEBUG环境变量调试rfm,以获得显示消息的调用栈：G_DEBUG=fatal-warnings gdb rfm\n"

#define CURRENT_COLUMN_STATUS "当前列状态(负号表示隐藏),{v}或{^}表示排序方向(同列标题栏上的三角形小尖头)\n当前排序列:%d;排序类型:%d\n"

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
"用法:  "BuiltInCmd_setenv" EnvVarName Value\n" \
"例子:  "BuiltInCmd_setenv" G_MESSAGES_DEBUG all\n" \
"       "BuiltInCmd_setenv" G_MESSAGES_DEBUG rfm\n" \
"       "BuiltInCmd_setenv" G_MESSAGES_DEBUG rfm,rfm-data,rfm-data-ext,rfm-data-thumbnail,rfm-data-search,rfm-data-git,rfm-gspawn,rfm-column,rfm-column-verbose,rfm-gtk,rfm-readline,rfm-addon (分割符逗号不行就试空格，glib版本升级好像变过, rfm这些参数应该也要用双引号括起来吧？)\n" \
"       "BuiltInCmd_setenv" G_MESSAGES_DEBUG \"\"\n" \
"获取当前值:\n" \
"       env |grep G_MESSAGES_DEBUG\n" \
"       注意,env命令实际是在子进程执行的,返回的实际上是子进程环境变量值.但每次命令执行都会创建子进程,并继承父进程的环境变量,因此可以通过子进程的环境变量值来推断父进程的值\n"

#define RFM_SEARCH_RESULT_TYPES "查询结果类型"
#define RFM_SEARCH_RESULT_TYPES_HELP "    在输出文件名列表的命令后加上 \033[33m>\033[0m0, 如 locate 202309|grep png >0 , 则可以在文件搜索结果视图显示文件列表,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n 这里>符号是rfm定制的,不是shell的文件输出重定向,可以在config.h里面换成别的字符串. 0表示下面default的查询结果类型,序号为0. \n" \
"    类似MIME类型和文件名后缀,查询结果类型也可以在config.h中用来定义文件上下文菜单项和默认打开选中文件的方式,比如在下面muview查询结果类型显示里,选中一封邮件回车会自动用mu view 命令查看邮件.\n" \
"    简单地说，查询(或叫搜索)就是找到满足某种条件的文件，而查询结果类型是使用特定代码解析查询到的某个文件，获得数据列，理论上讲，也可以把解析文件的逻辑合并到查询代码中，返回文件的同时就返回数据列，但这样的一个问题是数据的列名比较难处理，在查询语句返回文件名，数据列的同时，还要返回每个数据列的列名。所以这里就采取了用查询，和查询结果类型两步处理的设计。0号默认查询结果类型可以从查询获得除文件名外的数据列，但不需要列名，采用C1,C2,C3,...这样的顺序列名.\n" \
"    下面是全部的查询结果类型:"

#define SearchResultType_default "0号查询结果类型读取命令输出的行,每行可由分隔符分成多列(类似csv格式,config.h里包含分隔符默认值,可由内置命令或rfm启动参数动态定制,以配合不同的命令输出). 所有行包含列数相同. 首列必须是文件路径名. 命令输出不包含标题行,除首列文件名外,后续列在查询结果视图里依次自动分配列名C1,C2,C3....命令举例:下面命令输出第二列是行号,选中一个文件回车打开文本编辑器会跳转到此行,是因为创建shell子进程执行文件操作时会生成同名环境变量C1,C2...把列值传递到子进程,打开文本编辑器的脚本可以使用继承的环境变量\n" \
"      locate /etc/portage|xargs grep -ns emacs>default"

#define SearchResultType_gkeyfile "首先调用default查询结果类型获取命令输出行列,再用gtk keyfile 文件格式(等号分割的键值对行)解析首列文件内容,键名显示为列名,键值为列值. 每个keyfile文件内可包含不同的键名.命令举例,在rfm项目git仓库下,我建了一个devPicAndVideo子仓库,里面包含项目开发过程中的以下文档,可用下面命令查询问题列表:\n" \
"      locate .rfmTODO.gkeyfile>gkeyfile\n" \
"    至此,提到两种我们称之为\033[32m扩展列\033[0m(相对于文件系统内置的文件元数据,如权限,修改时间,文件大小等)的来源,一是程序标准输出里包含的除文件路径名之外的列,二是解析自文件内容的键值对数据.这里我们顺便提一下还有第三种来源:可以在config.h里面定义,然后通过showcolumn命令添加到显示,如现有的ImageSize列,这种列定义会包含一个取值bash命令或是c语言函数,根据传入的文件路径名,获取列值"

#define SearchResultType_markdown1 "首先调用default结果类型获取命令输出行列,再使用rfm自带的独立程序extractKeyValuePairFromMarkdown,以markdown格式解析首列文件内容;以gkeyfile格式输出键值对：markdown文件一级标题=一级标题下面首行内容.最后同gkeyfile结果类型显示.命令举例:\n" \
"      find -name *.TODO.md>.md"


#define SearchResultType_TODO_md "首先调用default结果类型获取命令输出行列,再使用rfm自带的独立程序extractKeyValuePairFromMarkdown,以markdown格式解析首列文件内容. extractKeyValuePairFromMarkdown 程序包含参数列表,以匹配markdown文件的一级标题,并输出gkeyfile格式键值对: 匹配到的markdown一级标题=一级标题下第一行内容. 最后同gkeyfile结果类型显示. 这里TODO.md类型会去匹配'简述'和'问题状态'这两个一级标题. 类似地,可以在config.h中定义新的查询结果类型,显示不同的markdown文件一级标题下的内容. 命令举例:\n" \
"      locate .rfmTODO.md>TODO.md"


#define SearchResultType_multitype "首先调用default结果类型,然后使用首列文件的MIME类型和文件名后缀去匹配同名的查询结果类型,调用匹配到的查询结果类型去处理此行,以达到在同一个结果页面显示多种文件格式内容的目的.命令举例:\n" \
"      locate .rfmTODO.>"SrchRsltType_multitype"\n" \
"    最后,在查询结果视图选中一条文件, 执行前述rfm内置的cd后接空格命令,就可以直接切换到包含此文件的目录.上面例子里文件是一条rfm TODO 事项描述文件,目录可以是为此事项专门建立,用以保存此事项相关图片等文档. 对比常见的关系数据库应用,查询结果里的扩展列相当于表字段; 我们用来在locate命令里匹配文件路径名的字符串起着关系数据库名及表名的作用; 两条不同表的记录的关联可以通过多种方式实现,最简单直观的就是两个文件位于同一文件目录下面. 上面例子若是在电子邮件系统里实现, rfmTODO 事项描述文件内容通常放在邮件正文里, 而事项相关文档都需要通过附件形式打包到邮件内, 也不如在rfm里面简单放在一个文件目录下方便."

#define SearchResultType_default_with_title "类似0号default查询结果类型,区别仅在于首行包含的是列标题,而不是实际数据.举例,过滤出当前查询结果第一行，参数-n2是因为title占一行，加上一行数据：\n" \
"      SearchResult|head -n2>withTitle\n"

#define SearchResultType_current "当前查询结果视图的类型，如果当前为空，则使用默认类型"


#define RFM_AddOn_HELP "  rfm 使用符合命名规范的 .c 源码作为 AddOn 插件，提供界面定制和功能扩展。请使用工具栏中\033[33maddons\033[0m按钮来查看插件列表\n" \
"  插件类型：\n" \
"    - Global：全局插件，可在任何目录使用. 系统启动时使用 locate 命令在整个系统中查找符合命名规范的 .c 文件, 若其中包含 `const enum RFM_ADDON_TYPE addon_type = Global;` 则为全局插件\n" \
"    - Directory：目录插件，只在特定目录生效。当进入特定目录时，检查该目录中是否存在符合命名规范的 .c 文件\n" \
"  编译时机：\n" \
"    - 首次发现 .c 文件时\n" \
"    - .c 文件内容修改后（通过哈希值检测）\n" \
"    - rfm_addon.h 头文件修改后(通过安装新版本的rfm)\n" \
"  加载过程：\n" \
"    1. 检测 .c 文件并计算哈希值\n" \
"    2. 检查是否存在对应的 .so 文件\n" \
"    3. 如需编译，显示确认对话框\n" \
"    4. 编译生成 .so 文件\n" \
"    5. 加载 .so 文件并调用 enter 函数\n" \
"  插件功能：\n" \
"    - 定制界面（添加列、菜单等）\n" \
"    - 扩展功能（添加新命令、处理特定文件类型等）\n" \
"    - 目录特定操作（针对特定目录的自定义行为）\n" \
"  命名规范：\n" \
"    - 在 config.h 用 RFM_ADDON_FILENAME_SUFFIX 定义插件文件名后缀\n" \
"    - 当前值是:" RFM_ADDON_FILENAME_SUFFIX "\n" \
"  Hash 计算：\n" \
"    - 读取 /etc/machine-id 获取机器唯一标识\n" \
"    - 读取 .c 文件内容\n" \
"    - 合并 machine-id 和文件内容\n" \
"    - 计算 SHA256 hash\n" \
"    - 同一 .c 文件在不同机器上会有不同的 hash\n" \
"    - 更换机器后，即使 .so 文件存在也会重新编译\n"
