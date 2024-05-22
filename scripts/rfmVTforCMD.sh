#!/bin/bash

#LIBGL_ALWAYS_SOFTWARE=1 /usr/bin/alacritty -e "$@"
#foot -t xterm-256color --log-level=warning "$@"

echo "$@" > /tmp/test_rfmVTforCMD.sh.txt
for arg in "$@"; do
        echo $arg >> /tmp/test_rfmVTforCMD.sh.txt
done

if [[ -n "$RFM_TERM" ]];then
	cmd="$RFM_TERM "$@""
	echo $cmd >> /tmp/test_rfmVTforCMD.sh.txt
	eval $cmd
else
	echo "environmental variable RFM_TERM is empty, please set it with terminial emulator name and parameters" >&2
	exit 1
fi
