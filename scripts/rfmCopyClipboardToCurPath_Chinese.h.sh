#!/bin/bash
#set -x

target=$(pwd)
# rfm使用g_spawn执行脚本时,第一个参数working directory会传入 rfm_curPath, 我的理解这里 pwd 就会得到 rfm_curPath 的值

read -p "从剪贴板读入源文件名还是打开新rfm窗口选取源文件,默认从剪贴板读,若选择新rfm窗口,则输入源文件所在目录" sourceDirectory
#TODO: 完称上面提示后续代码

if [[ ! -z "$sourceDirectory" ]];then
	if [[ -e $sourceDirectory ]];then
		namedpipe="/tmp/rfmFileChooser_$$"
		if [[ ! -p $namedpipe ]];then
			mkfifo $namedpipe
			#trap "rm -f $namedpipe" EXIT
			while read line;do cp -i -v -r $line $target; done < $namedpipe &
			rfm -d $sourceDirectory -r $namedpipe
			#rm -f $namedpipe
		else
			echo "$namedpipe 已存在" > 2
			exit 4
		fi
	else
		echo "输入目录或文件不存在" > 2
		exit 3
	fi
else

	sourcefiles=$(wl-paste)
	# TODO: wl-paste is for wayland, what if x11?

	if [[ ! -z "$target" ]]; then
		for sourcefile in $sourcefiles;do
			cp -i -v -r $sourcefile $target
		done
	else
		echo "pwd return empty" > 2
		exit 2
	fi
fi
