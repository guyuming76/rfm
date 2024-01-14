#!/bin/bash
#set -x

sourcefiles=$(wl-paste)
# TODO: wl-paste is for wayland, what if x11?
target=$(pwd)
# rfm使用g_spawn执行脚本时,第一个参数working directory会传入 rfm_curPath, 我的理解这里 pwd 就会得到 rfm_curPath 的值

if [[ ! -z "$target" ]]; then
	for sourcefile in $sourcefiles;do
		mv -i -v $sourcefile $target
	done
	read -p "press enter to close current window"
else
	echo "pwd return empty"
	exit 2
fi
