#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "press enter to copy selected filenames into clipboard;"
echo "or input the destination(fullpath or relative to current $(pwd)), up and down arrow keys can be used to choose from input history:"

input_destination="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [[ -z "$input_destination" ]]; then
	echo "${@:2}" | wl-copy
	# TODO: wl-copy is for wayland, what if x11?
else
	destination="$(realpath -s $input_destination)"

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
