#!/bin/bash

if [[ ! -e "$(which imv-msg)" ]];then
	xdg-open "$@"
	exit
fi

# imv 有时启动子进程 imv-wayland 来显示图片，imv-msg需要和这个子进程号通信，所以我下面用了 tail -n1 来选取最后一个满足条件的进程
# 当config.def.h 里面选RFM_EXEC_NONE时,${PPID}会是1;选RFM_EXEC_STDOUT时,${PPID}会是rfm的进程PID

imv_pid=$(ps -u|grep imv |grep for_rfm${PPID} |grep -v grep|tail -n1|awk '{print $2}')

if test -z "$imv_pid"
then
	# 避免使用 xargs，因为 xargs 会启动新进程，无法继承当前 shell 的 alias
	# 直接使用 imv 命令，让当前 shell 解析 alias
	imv for_rfm${PPID} "$@"
else
    	imv-msg $imv_pid close all
	imv-msg $imv_pid open "$@"
fi

