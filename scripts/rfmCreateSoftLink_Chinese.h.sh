#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "输入为选中文件创建软链接的目录路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

input_destination="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [[ ! -d "$input_destination" ]]; then
	echo "输入路径无效!"
else
	read -p "是否(y/n)使用相对路径创建软链接?默认是(y)" -r useRelativeInSL
	read -p "是否(y/n)自动 git stage 创建的软链接文件?默认是(y)" -r autoGitStage
	sl_list=""

	for i in "$@"; do
		sl_basename=$(basename "$i")
		read -p "输入要创建的软链接名,回车取默认值 $sl_basename" -r input_sl_basename
		if [[ ! -z "$input_sl_basename" ]]; then
			sl_basename=$input_sl_basename
		fi

               	if [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
                        ln -sr "$i" $(echo $input_destination/$sl_basename)
		else
			ln -s "$i" $(echo $input_destination/$sl_basename)
		fi

		if [[ -z "$autoGitStage" || "$autoGitStage" == "y" || "$autoGitStage" == "Y" ]];then
			git stage $(echo $input_destination/$sl_basename)
		fi

		sl_list+=" $input_destination/$sl_basename"
	done
fi
