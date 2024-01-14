#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "请输入移动目的路径,绝对路径或相对与当前默认路径($(pwd)),直接回车表示默认路径并复制选择文件名至剪贴板:"
read -r input_destination

if [[ -z "$input_destination" ]]; then
	input_destination=$(pwd)
	# 用户很有可能是在查询结果视图里面选中文件,然后移动到当前目录的,所以目的地默认为当前目录还是很有用的
	# TODO: 判断文件来源目录和目的目录如果相同,类似windows,建个新文件名,或提示用户
	# if destination not entered, we copy the selected file names into clipboard so that user can paste in newly opened rfm
	echo "$@" | wl-copy
	# TODO: wl-copy is for wayland, what if x11?
	# TODO, 理想的状态应该是用户在下面 mv -i 命令里选择不 overwrite 同名文件后在复制此文件名至剪贴板,以便用户在新打开的rfm窗口里导航到合适的目录后选择粘贴或移动到此, 若是用户选择overwrite 文件,则此文件名就复制到剪贴板了. 但我现在不知道如何获知用户在 cp -i 命令里的选择
fi

if [[ ! -z "$input_destination" ]]; then
	destination="$(realpath -s $input_destination)"

	if [[ -e $destination ]]; then
		/bin/mv -v -i $@ $destination
		if [[ -d $destination ]]; then
			# $@ moved into $destination directory
			autoselection=""
			for i in $@; do
				autoselection+=" $destination/$(basename $i)"
				# my test show destination returned from realpath do not end with /
			done
		else
			# $@ overrided $destination file
			# TODO: can we know whether user selected to overwrite existing file or not?
			autoselection=$destination
		fi
	elif [[ ! -z "$destination" ]]; then
		# i ensure destination -z here to prevent that something wrong from transformation from input_destination to destination and cp use the last selected filename parameter as destination
		# since destionation does not exists before, my test show that there can only be one source file
		# it's not possible to copy multiple source items into a non-existing destionation
		/bin/mv -v $@ $destination
		autoselection=$destination
	else
		echo "$input_destination;$destination" > 2
		exit 1
	fi
	rfm -d $autoselection
else
       	echo "pwd return empty" > 2
       	exit 2
fi
