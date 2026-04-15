#!/bin/bash

# 从标准输入读取内容
input_content=$(cat)


if [ "$WAYLAND_DISPLAY" ] || [ "$(echo '$DISPLAY'|grep -qw 'wayland')" ]; then
 # "当前是Wayland环境" or "X服务器正在使用Wayland"
     # 调试：将输入内容写入临时文件
    if [ -n "$G_MESSAGES_DEBUG" ]; then
	echo "$input_content" >>/tmp/test_rfmToClipboard_wayland.sh.txt
    fi
    echo "$input_content" | wl-copy
else
    # "当前是X环境"
    if [ -n "$G_MESSAGES_DEBUG" ]; then
    	echo "$input_content" >>/tmp/test_rfmToClipboard_x11.sh.txt
    fi
    # echo "$input_content" | xsel -i -b
    echo "$input_content" | xclip -selection clipboard -in
    # 在openkylin 上，如果没有下面sleep 2, 上面那句 xclip 似乎就不能成功执行
    sleep 2
fi
