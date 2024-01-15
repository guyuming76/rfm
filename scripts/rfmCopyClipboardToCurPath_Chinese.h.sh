#!/bin/bash
#set -x

#read -p "从剪贴板读入源文件名还是打开新rfm窗口选取源文件,默认从剪贴板读,若选择新rfm窗口,则输入源文件所在目录" sourceDirectory
#TODO: 完称上面提示后续代码

sourcefiles=$(wl-paste)
# TODO: wl-paste is for wayland, what if x11?
target=$(pwd)
# rfm使用g_spawn执行脚本时,第一个参数working directory会传入 rfm_curPath, 我的理解这里 pwd 就会得到 rfm_curPath 的值

if [[ ! -z "$target" ]]; then
	for sourcefile in $sourcefiles;do
		cp -i -v -r $sourcefile $target
	done
	read -p "按回车关闭当前窗口"
else
	echo "pwd return empty"
	exit 2
fi
