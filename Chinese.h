#define PageUp  "上葉"
#define PageDown  "下葉"
#define Up "父"
#define SwitchView "表/图"
#define Menu "菜单"
#define RunActionCopy "复制"
#define RunActionMove "移动(改名)"
#define RunActionDelete "删除"
#define RunActionGitStage "git 暂存"
#define RunActionCopySelection "复制文件名至剪贴板"
#define Copy "复制文件"
#define Paste "粘帖"
#define MoveTo "移至此"
#define RunActionChangeOwner "更改所有者"
#define SwitchPipeDir  "管道/目录"

#define PipeTitle "当前:%d/总数:%d, 每页:%d"

#define builtinCMD_Help \
"系统视图:\n" \
"    按照数据源分两种: 一种是当前目录文件视图,此时,工具栏左边第一按钮显示目录路径(和git分支). 另一种是文件搜索结果视图,此时工具栏左边提示当前条/结果总数,每页条数, 点击此按钮在两种视图间切换.\n" \
"    按显示布局分为 图标 和 列表两种,图标视图显示文件缩略图\n" \
"命令提示:\n" \
"    b*> 表示rfm视图有文件选中,且当前命令执行程序是Bash(这个在config.h里面可配置)\n" \
"    b>  表示没有文件选中\n" \
"    b?> 表示正在刷新,无法确定选中文件数,刷新完成后回车重试\n" \
"    命令提示不随文件选中或取消自动更新, 用户可按回车键刷新\n" \
"rfm内置命令:\n" \
"    cd 地址     跳转到地址, 注意 PWD 环境变量值不会改变. 若是在搜索结果文件列表界面,并选中文件或目录,则cd直接进入此文件所在目录,此时cd命令无需地址参数\n" \
"    cd ..       进入当前目录父目录\n" \
"    cd -        进入OLDPWD环境变量对于目录,即上次打开的目录\n" \
"    quit        退出rfm\n" \
"    help        输出本帮助信息\n" \
"    连按两次回车键刷新rfm显示内容\n" \
"    /           在 图标/列表 视图间切换,此操作不刷新数据\n" \
"    //          在当前目录和搜索结果视图间切换,此操作包含数据刷新\n" \
"    pwd         获取rfm进程 PWD 环境变量值\n" \
"    setpwd      设置rfm PWD 为当前显示目录\n" \
"    pagesize    显示搜索结果数据时,设置每葉文件数, 如 pagesize 100\n" \
"    showcolumn  显示或隐藏列(若当前在列表视图)\n" \
"创建Shell子进程执行的命令:\n" \
"    非rfm内置命令会被发送到操作系统Shell执行,可在config.h内配置shell类型如bash, nushell等.\n" \
"    如果命令末尾有空格字符, 当前rfm视图选中文件会被添加到命令末尾. 例如你选中一个maildir目录里的邮件文件, 输入 mu view 并在回车前以空格结尾, 则可以查看选中邮件内容.\n" \
"    继续上面例子,如果你希望less查看邮件内容,可以选中邮件文件后输入 mu view %s|less .注意,回车前还是要以空格结束命令行,虽然实际替换文件名的位置在%s处,另外每个%s替换一个选中文件,若需多个文件名,可多加几个%s.\n" \
"    在输出文件名列表的命令后加上 >0, 如 locate 202309|grep png >0 , 则可以在rfm内显示,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n" \
"Config.h文件自定义命令:\n"

#define rfmLaunchHelp \
"这里是shell中启动rfm的命令行参数说明, rfm启动后的命令窗口内另有帮助提示在那里可用的命令.\n" \
"用法: %s [-h] || [-d <目录完整路径>] [-i] [-v] [-l] [-p<每页显示文件数>] [-p] [-s<逗号分隔的列号序列>]\n" \
"-p       通过管道,从标准输入读取文件名列表,此参数可省略(rfm自行检测是否有文件名列表通过管道输入,但若同时包含 -d 参数设置默认值,则-p不可省,参见 -d 参数说明), 比如:\n" \
"         locate 20230420|grep .png|rfm\n" \
"-px      当通过管道输入读取文件名列表时, 每次只显示x个文件, 比如: -p9. 你也可以在rfm启动后在命令行窗口使用内置命令pagesize来设置此每页显示文件数\n" \
"-d       显示路径指定的目录,而不是当前启动rfm的目录, 比如 -d /home/somebody/maildir. 如果-d 后跟的是文件名而不是目录名,则打开文件所在目录,并自动选择定位到此文件. 注意, 若要在文件搜索结果视图通过 -d 设置默认选中值,则 -p 参数不可省略,并且-p参数要出现在 -d 参数前面.\n" \
"-i       显示mime类型\n" \
"-l       启动rfm后显示列表视图,而不是默认的图标视图,你也可以在打开rfm后使用内置命令/切换列表和图标视图\n" \
"-s       指定rfm启动后列表视图显示的列号序列,比如: -s,20,21,22  ,参见showcolumn内置命令\n" \
"-r       表示退出rfm后,返回选中文件,用于把rfm作为文件选择器 rfmFileChooser 使用的场景,比如某应用的文件打开菜单对应功能\n" \
"-h       显示此帮助\n"

#define SHOWCOLUMN_USAGE \
"showncolumn 命令可包含一个或多个空格分隔的列号序列, 每个列号序列可以是单独的列号,或由逗号分隔的多个列号. 列号为负,表示不显示此列. 列号序列由逗号开头,表示第一个列号在第一列显示.列号序列由逗号结尾,表示最后一个列号后的列都不显示.showcolumn命令不带参数则显示当前列号列名显示状态和顺序.注意上述列号序列中数字表示的是列名对应的编号,而不是列显示的位置顺序号.\n\n" \
"举例:    showcolumn 10,9,-12   表示把9号列移到10号列后面显示,12号列移到9号列后并隐藏\n" \
"         showcolumn 1,3 12     表示把3号列移到1号列后面显示, 同时显示12号列,不移动12号列位置\n" \
"         showcolumn ,2         表示把2号列移到首位显示\n" \
"         showcolumn 2,         表示2号列后面的列都不显示\n"

#define VALUE_MAY_NOT_LOADED \
"列号%d(%s)的值可能还没有加载,你可能需要刷新以便显示此列. 注意,如果切换列表/图标视图,此列可能出现在列表视图中,但如果不刷新,显示的可能是空数据\n" 
