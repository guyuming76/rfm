#!/bin/bash

if [ $# -gt 1 ]
then
	#rfm传入多个文件，则全部通过管道传给imv，用户使用键盘左右键翻图片
	echo "$@"|xargs imv
else
	#rfm传入单个文件，则找到现有 imv 进程，刷新显示图片
	imv_pid=$(ps -u|grep -e imv -e for_rfm |grep -v grep|awk '{print $2}')

	if test -z "$imv_pid" 
	then  
    	imv $1 for_rfm 
	else   
    	imv-msg $imv_pid close
    	imv-msg $imv_pid open $1
	fi 

	#[[ -z "$imv_pid" ]] && echo "empty" || echo $imv_pid

fi
