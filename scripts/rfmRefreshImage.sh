#!/bin/bash

# imv 有时启动子进程 imv-wayland 来显示图片，imv-msg需要和这个子进程号通信，所以我下面用了 tail -n1 来选取最后一个满足条件的进程
imv_pid=$(ps -u|grep imv |grep for_rfm |grep -v grep|tail -n1|awk '{print $2}')

if test -z "$imv_pid"
then
	echo "$@" | xargs imv for_rfm
else
    	imv-msg $imv_pid close all
	imv-msg $imv_pid open "$@"
fi
