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
#define Copy "复制"
#define Paste "粘帖"
#define MoveTo "移至此"
#define RunActionChangeOwner "更改所有者"
#define SwitchPipeDir  "管道/目录"

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
"Shell 命令:\n" \
"    非rfm内置命令会被发送到操作系统Shell执行.\n" \
"    如果命令末尾有空格字符, 当前rfm视图选中文件会被添加到命令末尾\n" \
"    在输出文件名列表的命令后加上 >0, 如 locate 202309|grep png >0 , 则可以在rfm内显示,效果等同启动rfm时前面有管道输入:locate 202309|grep png|rfm\n" \
"Config.h文件自定义命令:\n"
