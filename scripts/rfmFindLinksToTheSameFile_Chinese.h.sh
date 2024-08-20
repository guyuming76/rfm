#!/bin/bash

if [[ "$#" -gt 1 ]]; then
	echo "本命令只作用于第一条选中文件,多选文件被忽略!"
fi

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

echo ""
find "$rfmFindScope" -exec test "$1" -ef {} \; -print

echo ""
read -p "按回车退出"
