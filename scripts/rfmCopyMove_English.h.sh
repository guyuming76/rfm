#!/bin/bash
# Accept one or more filenames as parameters, currently receiving full path filenames from rfm context menu clicks

#set -x

echo "Press enter to copy selected filenames to clipboard;"
echo "Or input destination (fullpath or relative to current $(pwd)), use up/down arrow keys to browse input history:"

source rfmReadlineWithSpecificHistoryFile.sh "input_destination" ~/.rfm_history_directory
input_destination=${input_destination/#~/${HOME}}

# Users are likely to select files in search results view and copy to current directory.
# Default destination to current directory is necessary, though may encounter source files in same directory.
# Test shows cp -i will ignore existing files without prompting.

if [[ -z "$input_destination" ]]; then
	if [ -n "$G_MESSAGES_DEBUG" ]; then
		echo "${@:2}" >/tmp/test_rfmToClipboard.sh.txt
	fi
	echo "${@:2}" | xargs rfmToClipboard.sh
else
	read -p "Check and repair broken symbolic links after operation? (y/n, default y): " -r check_and_update_symbolic_link

        # -s 表示不解析符号链接
	destination="$(realpath -s $input_destination)";

	if [[ -e "$destination" ]]; then
		if [ -d "$destination" ] && [ ! -L "$destination" ];then
			read -p "Destination path exists and is a directory, overwrite? (y/n, default n). y=delete existing dir, n=use as parent dir: " -r rfm_overwrite_destination
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
			# If destination exists and is not a directory (file/symlink), must overwrite
			read -p "Destination exists (file/symlink), overwrite? (y/n, default n). y=delete existing, n=abort: " -r rfm_overwrite_destination
			if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
				autoselection="$destination"
			else
				exit 0
			fi

		fi
	elif [[ ! -z "$destination" ]]; then
		# Prevent issues when transforming input_destination to destination
		# When destination doesn't exist, only single source file allowed
		# Cannot copy multiple items to non-existing destination
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
			# Process each source file starting from 2nd parameter
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
		echo "parameter 1 should be either cp or mv" > 2
		exit 2
	fi

	rfm -n -d $autoselection

fi
