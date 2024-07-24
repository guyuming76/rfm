#!/bin/bash

if [[ ! -e "$(which emacs)" ]];then
       	xdg-open "$@"
       	exit
fi

#你可以在rfm命令输入窗口运行 rfmTextEdit.sh &
#以便在新的虚拟终端用 emacs -nw 打开当前选中文件

#TODO:emacs 加了-nw 参数后,目前通过用文件上下文edit菜单调用本脚本,会提示不是terminal环境,报错.如何在这个脚本里面识别是否是Terminal环境并且选择是否为emacs加-nw参数呢?

#这里假设 grep -nH 返回结果文件名排第一列,对应rfm搜索结果视图FileName列,第二列行号,搜索结果C1
if [ -n "$C1" ]; then
	# 正则表达式判读是否是数字
	if [[ $C1 =~ ^[0-9]+$ ]]; then
		emacs -nw --no-splash +$C1 "$@"
	else
		emacs -nw --no-splash "$@"
	fi
fi
