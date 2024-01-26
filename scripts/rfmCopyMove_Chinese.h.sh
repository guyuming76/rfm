#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "直接回车复制当前选中文件名至剪贴板;"
echo "或输入目的路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

input_destination="$(bash -c -i 'HISTFILE=~/.rfm_history_directory;
				HISTCONTROL=ignoreboth;
				history -r;
				history -s "$(pwd)";
				read -e -r input;
				history -s "$input";
				history -w;
				echo $input')"

# 用户很有可能是在查询结果视图里面选中文件,然后复制到当前目录的,所以目的地默认为当前目录还是很有必要,虽然会遇到来源文件也是当前目录的情况, 我测试的结果是cp -i 会直接忽略这个文件,并不提示

if [[ -z "$input_destination" ]]; then
	echo "${@:2}" | wl-copy
	# TODO: wl-copy is for wayland, what if x11?
else
	destination="$(realpath -s $input_destination)";

	if [[ -e $destination ]]; then
		if [[ -d $destination ]];then
			autoselection=""
			for i in ${@:2}; do
				autoselection+=" $destination/$(basename $i)"
				# my test show destination returned from realpath do not end with /
			done
		else
			# TODO: can we know whether user selected to overwrite existing file or not?
			autoselection=$destination
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
		/bin/cp -p -r -i ${@:2} $destination
	elif [[ "$1" == "mv" ]];then
		/bin/mv -v -i ${@:2} $destination
	else
		echo "parameter 1 should be either cp or mv" > 2
		exit 2
	fi

	rfm -d $autoselection

fi
