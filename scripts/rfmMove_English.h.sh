#!/bin/bash
# accept filenames to copy as parameter

#set -x

echo "Please input the destination(default to current $(pwd)), selected filenames will be copied into clipboard if default:"
read -r input_destination
if [[ -z "$input_destination" ]]; then
       	input_destination=$(pwd)
       	echo "$@" | wl-copy
       	# TODO: wl-copy is for wayland, what if x11?
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
