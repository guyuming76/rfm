#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "直接回车复制当前选中文件名至剪贴板;"
echo "或输入目的路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

input_destination="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

# 用户很有可能是在查询结果视图里面选中文件,然后复制到当前目录的,所以目的地默认为当前目录还是很有必要,虽然会遇到来源文件也是当前目录的情况, 我测试的结果是cp -i 会直接忽略这个文件,并不提示

if [[ -z "$input_destination" ]]; then
	if [ -n "$G_MESSAGES_DEBUG" ]; then
		echo "${@:2}" >/tmp/test_rfmToClipboard.sh.txt
	fi
	echo "${@:2}" | xargs rfmToClipboard.sh
else
	read -p "是否(y/n)检查移动或复制造成的符号链接破坏并修复?默认是(y)" -r check_and_update_symbolic_link

        # -s 表示不解析符号链接
	destination="$(realpath -s $input_destination)";

	export rfm_overwrite_destination="y"

	if [[ -e "$destination" ]]; then
		if [ -d "$destination" ] && [ ! -L "$destination" ];then
			read -p "目的路径存在且是目录，是否(y/n)覆盖,即先删除现有目录?默认是(y);否(n)表示输入路径为目的父路径" -r rfm_overwrite_destination
			# y 对应cp/mv -T, n 对应cp/mv -t?
			if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
				autoselection="$destination"
			else
				autoselection=""
				for i in ${@:2}; do
					basename_i=$(basename "$i")
					autoselection+=" $destination/$basename_i"
					# my test show destination returned from realpath do not end with /
				done
			fi
		else
			# 如果目的地存在且不是目录，只能是覆盖，mv 命令后如果有 -f 则不提示，否则 mv 命令会提示
			read -p "目的路径存在且不是目录（是文件或符号链接,包括指向目录的符号链接），是否(y/n)覆盖，即先删除现有?默认是(y)，否(n)则退出" -r rfm_overwrite_destination
			if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
				autoselection="$destination"
			else
				exit 0
			fi

		fi
	elif [[ ! -z "$destination" ]]; then
		# i ensure destination -z here to prevent that something wrong from transformation from input_destination to destination and cp use the last selected filename parameter as destination
		# since destionation does not exists before, my test show that there can only be one source file
		# it's not possible to copy multiple source items into a non-existing destionation
		autoselection=$destination
	else
		echo "$input_destination;$destination" > 2
		exit 1
	fi

	if [[ "$1" == "cp" ]];then
		#TODO:考虑rfm_overwrite_destination
		cp -p -r -i ${@:2} "$destination"
	elif [[ "$1" == "mv" ]];then
		if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_linj" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
			# 从第2个参数开始，每个参数循环一次
			for sourcefile in ${@:2};do
				if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
					rfmMoveDirAndUpdateSymbolicLinks.sh "$sourcefile" "$destination"
				else
					basename_sourcefile=$(basename "$sourcefile")
					rfmMoveDirAndUpdateSymbolicLinks.sh "$sourcefile" "$destination/$basename_sourcefile"
				fi
			done
		else
			#TODO:考虑rfm_overwrite_destination
			mv -v -i ${@:2} "$destination"
		fi
	else
		echo "parameter 1 should be either cp or mv" > 2
		exit 2
	fi

	rfm -n -d $autoselection

fi
