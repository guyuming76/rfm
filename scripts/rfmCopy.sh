#!/bin/bash
# accept filenames to copy as parameter

set -x

export destination=$(pwd)

read -p "Please input the copy destination(default $destination ): " -r input_destination

[[ ! -z "$input_destination" ]] && destination=$input_destination && (rfm -d $destination &)
#TODO: autoselect the copied filenames in newly opened rfm window

/bin/cp -p -r -i $@ $destination

read -p "press enter to close this window"
