#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

create_next="y"
while [[ "${create_next,,}" =~ "y" ]]; do


echo "输入目录路径,在其中创建指向选中文件的软链接。可以输入绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

source rfmReadlineWithSpecificHistoryFile.sh "input_destination" ~/.rfm_history_directory
input_destination=${input_destination/#~/${HOME}}

if [[ ! -d "$input_destination" ]]; then
	read -p "输入路径不存在,是否(y/n)创建目录?默认是(y)" -r mkdir_input_destination
	if [[ -z "$mkdir_input_destination" || "$mkdir_input_destination" == "y" || "$mkdir_input_destination" == "Y" ]];then
		mkdir -p $input_destination
	fi
fi

if [[ -d "$input_destination" ]]; then
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


read -p "是否(y/n)继续创建下一个指向此选中文件的软链接?默认否(n):" create_next
done
