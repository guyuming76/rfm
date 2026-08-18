#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

# 脚本出错退出时暂停，等待用户按回车，以便在终端关闭前查看错误信息
exit_with_pause_on_error() {
    local status=$?
    if [[ $status -ne 0 ]]; then
        echo
        read -p "按回车关闭窗口" -r
    fi
}
trap exit_with_pause_on_error EXIT

#set -x

echo "直接回车复制当前选中文件名至剪贴板;"
echo "或输入目的路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

source rfmReadlineWithSpecificHistoryFile.sh "input_destination" ~/.rfm_history_directory
input_destination=${input_destination/#~/${HOME}}

# 用户很有可能是在查询结果视图里面选中文件,然后复制到当前目录的,所以目的地默认为当前目录还是很有必要,虽然会遇到来源文件也是当前目录的情况, 我测试的结果是cp -i 会直接忽略这个文件,并不提示

if [[ -z "$input_destination" ]]; then
    	if [ -n "$G_MESSAGES_DEBUG" ]; then
        	printf '%s\n' "${@:2}" >/tmp/test_rfmToClipboard.sh.txt
    	fi
    	printf '%s\n' "${@:2}" | rfmToClipboard.sh
else
	if [[ "$1" == "sl" ]];then
		read -p "是否(y/n)使用相对路径创建软链接?默认是(y)" -r useRelativeInSL

	        # -s 表示不解析符号链接
		destination="$(realpath -s $input_destination)";

		if [[ -e "$destination" ]]; then
			if [ ! -d "$destination" ];then
				echo "目的路径存在但不是目录，无法在其中创建软链接" >&2
				exit 3
			fi
			# 已存在的目录作为软链接父目录
			sl_parent_dir="$destination"
		else
			read -p "目的路径不存在。输入 y(默认) 则创建该目录，并在其中为每个选中文件创建同名软链接；输入 n 则把该路径当作要创建的软链接文件名(仅适用于单个选中文件)。是否创建目录? (y/n, 默认y)" -r mkdir_input_destination
			if [[ -z "$mkdir_input_destination" || "$mkdir_input_destination" == "y" || "$mkdir_input_destination" == "Y" ]];then
				mkdir -p "$destination"
				sl_parent_dir="$destination"
			else
				# 把不存在的目的路径当作软链接文件名
				if [[ $# -gt 2 ]]; then
					echo "选中的文件不止一个，无法用单个软链接文件名对应多个源文件" >&2
					exit 4
				fi
				sourcefile="${@:2:1}"
				mkdir -p "$(dirname "$destination")"
				if [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
					ln -sr "$sourcefile" "$destination" || exit
				else
					ln -s "$sourcefile" "$destination" || exit
				fi
				rfm -n -d "$destination"
				exit 0
			fi
		fi

		autoselection=""
		for sourcefile in "${@:2}"; do
			basename_sourcefile=$(basename "$sourcefile")
			if [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
				ln -sr "$sourcefile" "$sl_parent_dir/$basename_sourcefile" || exit
			else
				ln -s "$sourcefile" "$sl_parent_dir/$basename_sourcefile" || exit
			fi
			autoselection+=" $sl_parent_dir/$basename_sourcefile"
		done

		rfm -n -d $autoselection
		exit 0
	fi

	read -p "是否(y/n)检查移动或复制造成的符号链接破坏并修复?默认是(y)" -r check_and_update_symbolic_link

        # -s 表示不解析符号链接
	destination="$(realpath -s $input_destination)";

	if [[ -e "$destination" ]]; then
		if [ -d "$destination" ] && [ ! -L "$destination" ];then
			read -p "目的路径存在且是目录，是否(y/n)覆盖,即先删除现有目录? 直接回车默认否(n),表示输入路径为目的父路径" -r rfm_overwrite_destination
			export rfm_overwrite_destination
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
			read -p "目的路径存在且不是目录（是文件或符号链接,包括指向目录的符号链接），是否(y/n)覆盖，即先删除现有文件? 直接回车默认否(n)，退出当前操作" -r rfm_overwrite_destination
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
		if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
			rm -rf "$destination"
			cp -p -r -T -i ${@:2} "$destination"
		else
			cp -p -r -i ${@:2} "$destination"
		fi
	elif [[ "$1" == "mv" ]];then
		if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_link" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
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
			if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
				rm -rf "$destination"
				mv -T -v -i ${@:2} "$destination"
			else
				mv -v -i ${@:2} "$destination"
			fi
		fi
	else
		echo "parameter 1 should be either cp or mv or sl" > 2
		exit 2
	fi

	rfm -n -d $autoselection

fi
