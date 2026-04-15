#!/bin/bash

if [ "$WAYLAND_DISPLAY" ] || [ "$(echo '$DISPLAY'|grep -qw 'wayland')" ]; then
    # "当前是Wayland环境" or "X服务器正在使用Wayland"
	wl-paste
else
    # "当前是X环境"
    #  xsel -o -b
    # 在openkylin2.0sp2上，上面这句xsel不行
    xclip -selection clipboard -out
fi
