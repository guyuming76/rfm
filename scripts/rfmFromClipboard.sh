#!/bin/bash

if [ "$WAYLAND_DISPLAY" ] || [ "$(echo '$DISPLAY'|grep -qw 'wayland')" ]; then
    # "当前是Wayland环境" or "X服务器正在使用Wayland"
	wl-paste
else
    # "当前是X环境"
	xsel -o -b
fi
