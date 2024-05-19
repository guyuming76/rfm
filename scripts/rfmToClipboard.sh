#!/bin/bash

echo "$@" >>/tmp/test_rfmToClipboard.sh.txt

if [ "$WAYLAND_DISPLAY" ] || [ "$(echo '$DISPLAY'|grep -qw 'wayland')" ]; then
    # "当前是Wayland环境" or "X服务器正在使用Wayland"
	echo "$@" | wl-copy
else
    # "当前是X环境"
	echo "$@" | xsel -i -b
fi
