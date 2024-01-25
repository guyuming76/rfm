#!/bin/bash
#set -x

target=$(pwd)

if [[ -z "$target" ]]; then
	echo "pwd return empty" > 2
	exit 2
fi

if [[ "$1" != "cp" && "$1" != "mv" ]]; then
	echo "parameter 1 should be either cp or mv" > 2
	exit 5
fi


historyFile=~/.rfm_history_directory
history -r

sourcefiles=$(wl-paste)
# TODO: wl-paste is for wayland, what if x11?

clipboardContent=0
for src in $sourcefiles;do
	echo $src
	clipboardContent+=1
done

if [[ $clipboardContent -eq 0 ]];then
	echo "Please input source path(use up and down arrow key to choose from input history):"
else
	echo "Press enter to read source from clipboard content shown above;"
	echo "or input source path(use up and down arrow key to choose from input history):"
fi

read -e source

if [[ ! -z "$source" ]];then

	history -s "$source"
	history -w

	if [[ -d $source ]];then
		read -p "Path inputted is a directory, press enter to launch new rfm window to choose source from this directory, or press n to use the directory as source" -r new_rfm
	fi

	if [[ -d $source && ( -z "$new_rfm" || "$new_rfm" == "Y" || "$new_rfm" == "y" ) ]];then
		export namedpipe="/tmp/rfmFileChooser_$$"
		if [[ ! -p $namedpipe ]];then
			#trap "rm -f $namedpipe" EXIT
		        mkfifo $namedpipe; \
			while read line ;do \
				if [[ "$1" == "cp" ]];then \
					cp -v -r $line $target; \
				else \
					mv -v $line $target; \
				fi \
			done <$namedpipe &
			rfm -d $source -r $namedpipe
			wait
			rm -f $namedpipe
		else
			echo "$namedpipe already exists" > 2
			exit 4
		fi
	elif [[ -f $source || "$new_rfm" == "n" ]];then
		if [[ "$1" == "cp" ]];then
			cp -i -v -r $source $target
		else
			mv -i -v $source $target
		fi
	else
		echo "$source not directory or file" > 2
		exit 3
	fi
else
	for sourcefile in $sourcefiles;do
		if [[ "$1" == "cp" ]];then
			cp -i -v -r $sourcefile $target
		else
			mv -i -v $sourcefile $target
		fi
	done
fi
