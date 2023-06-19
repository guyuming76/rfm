#!/bin/bash

set -x

export destination=$(pwd)

read -p "请输入移动目的地址(默认 $destination ): " -r input_destination

[[ ! -z "$input_destination" ]] && destination=$input_destination

/bin/mv -i $@ -t $destination

read -p "输入 1 在新窗口用 rfm 打开路径 $destination, 或回车关闭此窗口: " -r next_action

[[ "$next_action" == "1" ]]  && rfm -d $destination

#[[ "$next_action" == "1" ]]  && rfm -d $destination 1>/tmp/rfm1.log 2>/tmp/rfm2.log &

#我本希望上面一行新的rfm启动后，本脚本就终结了，本窗口也就会关闭，但是新的 rfm 作为子进程能够继续。
#但是我机器上的结果是 新的 rfm 作为本进程的子进程也随着本窗口的关闭立刻关闭了。
#我下面加了一句sleep, 目的是让我可以观查到新的 rfm 窗口成功打开了，然后10秒后，随着本窗口的关闭而关闭。

#我若是直接在foot里面启动 rfm& ,然后在foot里输入 exit 退出foot，rfm窗口是会保持打开的;但我若是用 MOD+SHIFT+c (DWL 关闭窗口组合键)关闭foot窗口，rfm会随之关闭。

#我直接在foot里面执行  /usr/local/bin/rfmVTforCMD.sh /usr/local/bin/rfmMove.sh /home/guyuming/htopQtWeb1.txt 
#新的rfm窗口还是会自动关闭，也就是说似乎和我在父进程里面启动本窗口的 g_spawn 参数没关系

#我不知道如何才能在这里让新的 rfm 在本脚本结束后继续保持打开。我如果可以把新的rfm的父进程id，PPID设为和原有rfm进程的PPID相同，能否解决？
#为此，两个设想：1. 需要一个命令，可以在本脚本里面执行，用以设置新rfm进程的PPID, 同时，老rfm进程的PPID作为命令参数或环境变量传入本脚本。或者 2. 把设置PPID的功能直接用c 语言写在rfm 里面，同时老的 rfm 在间接launch 本脚本的时候，把PPID传过来。两个设想相比，我觉得有专门的修改PPID的命令会比较好，符合专门的工具做专门的事的哲学，不至于把 rfm 代码搞得过于复杂。但我还没搜到这样的命令。


#貌似我改不了PPID！

#The parent process id (ppid) of a process cannot be changed outside of the kernel; there is no setppid system call. The kernel will only change the ppid to (pid) 1 after the processes parent has terminated
#https://unix.stackexchange.com/questions/193902/change-the-parent-process-of-a-process

#a process can change its children's PGID if they're still running the original process image (i.e. they haven't called execve to run a different program).
#https://unix.stackexchange.com/questions/462188/is-there-a-way-to-change-the-process-group-of-a-running-process

#sleep 10s
