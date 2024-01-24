#!/bin/bash
# accept filenames to copy as parameter

#set -x

historyFile=~/.rfm_history_directory
history -r

echo "Please input the copy destination(fullpath or relative to current $(pwd))"
echo "selected filenames will be copied into clipboard if enter pressed without input:"
read -e -r input_destination

history -s "$input_destination"
history -w

if [[ -z "$input_destination" ]]; then
       	input_destination=$(pwd)
       	# 用户很有可能是在查询结果视图里面选中文件,然后复制到当前目录的,所以目的地默认为当前>
       	# if destination not entered, we copy the selected file names into clipboard so that>
       	echo "$@" | wl-copy
       	# TODO: wl-copy is for wayland, what if x11?
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
