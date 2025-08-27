#!/bin/bash

set -x

#echo "rfmReadlineUserInputVar" $rfmReadlineUserInputVar
#第一个参数是从 rfmReedlineWithSpecificHistoryFile.sh 里传过来的 rfmReadlineUserInputVar,TODO: 要是能直接通过进程继承环境变量更好，下面就可以用$@,而不是${@:2}了

input_destination="$1"
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

	for i in "${@:2}"; do
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
