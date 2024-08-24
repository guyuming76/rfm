#!/bin/bash

rfmFindScope=$(git rev-parse --show-toplevel 2>/dev/null)
if [[ -z  "$rfmFindScope" ]]; then
	rfmFindScope="/"
fi

echo "输入查找(find命令)范围,直接按回车默认 $rfmFindScope,可用上下箭头方向键从历史输入中选择:"

rfmInput="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [[ ! -z "$rfmInput" ]]; then
	if [[ -d "$rfmInput" ]];then
		rfmFindScope=$rfmInput
	else
		read -p "输入目录无效,按回车退出!"
		exit
	fi
fi

for i in "$@";do
	read -p "是否(y/n)输出结果至剪贴板?默认是(n)" -r outputToClipBoard
	echo ""
	if [[ "$outputToClipBoard" == "y" || "$outputToClipBoard" == "Y" ]];then
		find "$rfmFindScope" -exec test "$i" -ef {} \; -print | tee /dev/tty | xargs rfmToClipboard.sh
	else
		find "$rfmFindScope" -exec test "$i" -ef {} \; -print
	fi
	echo ""
done

echo ""
read -p "按回车退出"
