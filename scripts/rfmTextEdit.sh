#!/bin/bash
#你可以在rfm命令输入窗口运行 rfmTextEdit.sh &
#以便在新的虚拟终端用 emacs -nw 打开当前选中文件

#TODO:emacs 加了-nw 参数后,目前通过用文件上下文edit菜单调用本脚本,会提示不是terminal环境,报错.如何在这个脚本里面识别是否是Terminal环境并且选择是否为emacs加-nw参数呢?

if [ -n "$RFM_grepMatch" ]; then
	# 使用:分割,输出第一个字串
	LineNumber=$(echo $RFM_grepMatch | cut -d ":" -f 1)
	# 正则表达式判读是否是数字
	if [[ $LineNumber =~ ^[0-9]+$ ]]; then
		emacs -nw --no-splash +$LineNumber "$@" 
		exit
	fi
fi

emacs -nw --no-splash "$@"
