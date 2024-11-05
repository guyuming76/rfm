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

#	if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_linj" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
#		if [[ -e $destination ]]; then
#			echo "目标路径已存在"
#			echo "若要使用'检查移动或复制造成的符号链接破坏并修复'功能"
#			echo "请先删除目标路径再尝试"
#			echo
#			echo -e "\e[1;33m 按回车关闭窗口 \e[0m"
#			read
#			exit 0
#		fi
#	fi

	if [[ -e "$destination" ]]; then
		if [[ -d "$destination" ]];then
			autoselection=""
			for i in "${@:2}"; do
				autoselection+=" $destination/$(basename $i)"
				# my test show destination returned from realpath do not end with /
			done
		else
			# TODO: can we know whether user selected to overwrite existing file or not?
			autoselection="$destination"
		fi
	elif [[ ! -z "$destination" ]]; then
		# i ensure destination -z here to prevent that something wrong from transformation from input_destination to destination and cp use the last selected filename parameter as destination
		# since destionation does not exists before, my test show that there can only be one source file
		# it's not possible to copy multiple source items into a non-existing destionation
		autoselection="$destination"
	else
		echo "$input_destination;$destination" >&2
		exit 1
	fi

	if [[ "$1" == "cp" ]];then
		cp -p -r -i "${@:2}" "$destination"
	elif [[ "$1" == "mv" ]];then
		if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_linj" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
			# 从第2个参数开始，每个参数循环一次
			for sourcefile in "${@:2}";do
				rfmMoveDirAndUpdateSymbolicLinks.sh "$sourcefile" "$destination"
			done
		else
			mv -v -i "${@:2}" "$destination"
		fi
	else
		echo "parameter 1 should be either cp or mv" >&2
		exit 2
	fi

	rfm -n -d "$autoselection"

fi
