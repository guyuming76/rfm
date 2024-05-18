#!/bin/bash

#LIBGL_ALWAYS_SOFTWARE=1 /usr/bin/alacritty -e "$@"
#foot -t xterm-256color --log-level=warning "$@"

if [[ -n "$RFM_TERM" ]];then
	cmd="$RFM_TERM "$@""
	eval $cmd
else
	echo "environmental variable RFM_TERM is empty, please set it with terminial emulator name and parameters" > 2
	exit 1
fi
