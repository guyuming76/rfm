#define PageUp  "上葉"
#define PageDown  "下葉"
#define Up "父"
#define SwitchView "表/图"
#define Menu "菜单"
#define RunActionCopy "复制"
#define RunActionMove "移动"
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
"命令提示:\n" \
"    *> 表示rfm视图有文件选中\n" \
"    >  表示没有文件选中\n" \
"    命令提示不随文件选中或取消自动更新, 用户可按回车键刷新\n" \
"rfm内置命令:\n" \
"    cd 地址     跳转到地址, 注意 PWD 环境变量值不会改变\n" \
"    quit        退出rfm\n" \
"    help        输出本帮助信息\n" \
"    连按两次回车键刷新rfm显示内容\n" \
"    /           在 图标/列表 视图间切换\n" \
"    //          在当前目录和管道输入间切换\n" \
"    pwd         获取rfm进程 PWD 环境变量值\n" \
"    setpwd      设置rfm PWD 为当前显示目录\n" \
"    pagesize    当视图分葉显示管道数据时,设置每葉文件数, 如 pagesize 100\n" \
"    showcolumn  显示或隐藏列(若当前在列表视图)\n" \
"Shell 命令:\n" \
"    非rfm内置命令会被发送到操作系统Shell执行.\n" \
"    如果命令末尾有空格字符, 当前rfm视图选中文件会被添加到命令末尾\n" \
"    在输出文件名列表的命令后加上 >0, 如 locate 202309|grep png >0 , 则可以在rfm内显示,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n" \
"Config.h文件自定义命令:\n"

#define rfmLaunchHelp \
"这里是shell中启动rfm的命令行参数说明, rfm启动后的命令窗口内另有帮助提示在那里可用的命令.\n" \
"用法: %s [-h] || [-d <目录完整路径>] [-i] [-v] [-l] [-p<每页显示文件数>] [-p] [-s<逗号分隔的列号序列>]\n" \
"-p       通过管道,从标准输入读取文件名列表,此参数可省略(rfm自行检测是否有文件名列表通过管道输入), 比如:\n" \
"         locate 20230420|grep .png|rfm\n" \
"-px      当通过管道输入读取文件名列表时, 每次只显示x个文件, 比如: -p9. 你也可以在rfm启动后在命令行窗口使用内置命令pagesize来设置此每页显示文件数\n" \
"-d       显示路径指定的目录,而不是当前启动rfm的目录, 比如 -d /home/somebody/maildir\n" \
"-i       显示mime类型\n" \
"-l       启动rfm后显示列表视图,而不是默认的图标视图,你也可以在打开rfm后使用内置命令/切换列表和图标视图\n" \
"-s       指定rfm启动后列表视图显示的列号序列,比如: -s,20,21,22  ,参见showcolumn内置命令\n" \
"-h       显示此帮助\n"

#define SHOWCOLUMN_USAGE \
"showncolumn 命令可包含一个或多个空格分隔的列号序列, 每个列号序列可以是单独的列号,或由逗号分隔的多个列号. 列号为负,表示不显示此列. 列号序列由逗号开头,表示第一个列号在第一列显示.列号序列由逗号结尾,表示最后一个列号后的列都不显示.showcolumn命令不带参数则显示当前列号列名显示状态和顺序.注意上述列号序列中数字表示的是列名对应的编号,而不是列显示的位置顺序号.\n\n" \
"举例:    showcolumn 10,9,-12   表示把9号列移到10号列后面显示,12号列移到9号列后并隐藏\n" \
"         showcolumn 1,3 12     表示把3号列移到1号列后面显示, 同时显示12号列,不移动12号列位置\n" \
"         showcolumn ,2         表示把2号列移到首位显示\n" \
"         showcolumn 2,         表示2号列后面的列都不显示\n"
