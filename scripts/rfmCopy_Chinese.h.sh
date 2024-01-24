#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

historyFile=~/.rfm_history_directory
history -r

echo "请输入复制目的路径,绝对路径或相对与当前默认路径($(pwd))"
echo "直接回车复制当前选中文件名至剪贴板:"
read -e -r input_destination

history -s "$input_destination"
history -w

if [[ -z "$input_destination" ]]; then
	input_destination=$(pwd)
	# 用户很有可能是在查询结果视图里面选中文件,然后复制到当前目录的,所以目的地默认为当前目录还是很有必要,虽然会遇到来源文件也是当前目录的情况, 我测试的结果是cp -i 会直接忽略这个文件,并不提示
	# TODO: 判断文件来源目录和目的目录如果相同,类似windows,建个新文件名,或提示用户
	# if destination not entered, we copy the selected file names into clipboard so that user can paste in newly opened rfm
	echo "$@" | wl-copy
	# TODO: wl-copy is for wayland, what if x11?
	# TODO, 理想的状态应该是用户在下面 cp -i 命令里选择不 overwrite 同名文件后在复制此文件名至剪贴板,以便用户在新打开的rfm窗口里导航到合适的目录后选择粘贴或移动到此, 若是用户选择overwrite 文件,则此文件名就复制到剪贴板了. 但我现在不知道如何获知用户在 cp -i 命令里的选择
else
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

fi
