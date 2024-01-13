#!/bin/bash
# accept filenames to copy as parameter

#set -x

export destination=$(pwd)

read -p "Please input the copy destination(default $destination ): " -r input_destination

[[ ! -z "$input_destination" ]] && destination=$input_destination && cpcmd="/bin/cp -p -r -i '$@' $destination" && rfm -d $destination -x "$cpcmd"
#TODO: autoselect the copied filenames in newly opened rfm window
