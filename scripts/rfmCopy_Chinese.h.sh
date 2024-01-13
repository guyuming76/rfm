#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

read -p "请输入复制目的路径,绝对路径或相对与当前($(pwd)),若不输入路径,直接回车则复制选择文件名至剪贴板: " -r input_destination

if [[ ! -z "$input_destination" ]]; then
	destination="$(realpath -s $input_destination)"

	if [[ -e $destination ]]; then
		/bin/cp -p -r -i $@ $destination
		if [[ -d $destination ]]; then
			# $@ copied into $destination directory
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
		/bin/cp -p -r $@ $destination
		autoselection=$destination
	else
		echo "$input_destination;$destination" > 2
		exit 1
	fi
	rfm -d $autoselection
else
	# if destination not entered, we copy the selected file names into clipboard so that user can paste in newly opened rfm
	echo "$@" | wl-copy
	# TODO: wl-copy is for wayland, what if x11?
	rfm
fi
