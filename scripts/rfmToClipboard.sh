#!/bin/bash

# 从标准输入读取内容
input_content=$(cat)

# 调试：将输入内容写入临时文件
echo "$input_content" >>/tmp/test_rfmToClipboard.sh.txt

if [ "$WAYLAND_DISPLAY" ] || [ "$(echo '$DISPLAY'|grep -qw 'wayland')" ]; then
 # "当前是Wayland环境" or "X服务器正在使用Wayland"
    echo "$input_content" | wl-copy
else
 # "当前是X环境"
    echo "$input_content" | xsel -i -b
fi
