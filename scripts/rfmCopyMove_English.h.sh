#!/bin/bash
# Accept one or more filenames as parameters, currently receiving full path filenames from rfm context menu clicks

# Pause on error exit so the user can read the error message before the terminal closes
exit_with_pause_on_error() {
    local status=$?
    if [[ $status -ne 0 ]]; then
        echo
        read -p "Press Enter to close the window" -r
    fi
}
trap exit_with_pause_on_error EXIT

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
               	printf '%s\n' "${@:2}" >/tmp/test_rfmToClipboard.sh.txt
       	fi
	printf '%s\n' "${@:2}" | rfmToClipboard.sh
else
	if [[ "$1" == "sl" ]];then
		read -p "Use relative paths when creating symlinks? (y/n, default y): " -r useRelativeInSL

	        # -s 表示不解析符号链接
		destination="$(realpath -s $input_destination)";

		if [[ -e "$destination" ]]; then
			if [ ! -d "$destination" ];then
				echo "Destination exists but is not a directory, cannot create symlinks inside" >&2
				exit 3
			fi
			# Existing directory is used as the symlink parent directory
			sl_parent_dir="$destination"
		else
			read -p "Destination does not exist. Enter y (default) to create it as a directory and create a symlink named after each selected file inside it; enter n to treat this path as the symlink filename itself (only works for a single selected file). Create directory? (y/n, default y): " -r mkdir_input_destination
			if [[ -z "$mkdir_input_destination" || "$mkdir_input_destination" == "y" || "$mkdir_input_destination" == "Y" ]];then
				mkdir -p "$destination"
				sl_parent_dir="$destination"
			else
				# Treat the non-existing destination path as the symlink filename
				if [[ $# -gt 2 ]]; then
					echo "More than one file selected, cannot map multiple sources to a single symlink filename" >&2
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
		echo "parameter 1 should be either cp or mv or sl" > 2
		exit 2
	fi

	rfm -n -d $autoselection

fi
