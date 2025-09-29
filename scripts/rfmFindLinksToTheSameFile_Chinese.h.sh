#!/bin/bash

rfmFindScope=$(git rev-parse --show-toplevel 2>/dev/null)
if [[ -z  "$rfmFindScope" ]]; then
	rfmFindScope="/"
fi

echo "输入查找(find命令)范围,直接按回车默认 $rfmFindScope,可用上下箭头方向键从历史输入中选择:"

source rfmReadlineWithSpecificHistoryFile.sh "rfmInput" ~/.rfm_history_directory
rfmInput=${rfmInput/#~/${HOME}}

if [[ ! -z "$rfmInput" ]]; then
	if [[ -d "$rfmInput" ]];then
		rfmFindScope=$rfmInput
	else
		read -p "输入目录无效,按回车退出!"
		exit
	fi
fi

for i in "$@";do
	read -p "是否(y/n)输出结果至剪贴板?默认否(n)" -r outputToClipBoard
	read -p "查找过程中间节点是否(y/n)包含软链接(即find命令是否带-L参数)?默认包含(y)" includeSL

	echo ""
	if [[ "$outputToClipBoard" == "y" || "$outputToClipBoard" == "Y" ]];then
		if [[ "${includeSL,,}" == "n" ]]; then
			find "$rfmFindScope" -not -path '*/.git/*'  -exec test "$i" -ef {} \; -print | tee /dev/tty | xargs rfmToClipboard.sh
		else
			find -L "$rfmFindScope" -not -path '*/.git/*'  -exec test "$i" -ef {} \; -print | tee /dev/tty | xargs rfmToClipboard.sh
		fi
	else
		if [[ "${includeSL,,}" == "n" ]]; then
			find "$rfmFindScope" -not -path '*/.git/*'  -exec test "$i" -ef {} \; -print
			#find "$rfmFindScope" -samefile "$i" 命令和上面命令结果不同
		else
			find -L "$rfmFindScope" -not -path '*/.git/*'  -exec test "$i" -ef {} \; -print
			#find -L "$rfmFindScope" -samefile "$i" 命令和上面命令得到同样的结果
		fi
	fi
	echo ""
done

echo ""
read -p "按回车退出"
