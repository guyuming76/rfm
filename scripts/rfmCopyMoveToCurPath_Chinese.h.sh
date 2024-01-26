#!/bin/bash
#set -x

target=$(pwd)
# rfm使用g_spawn执行脚本时,第一个参数working directory会传入 rfm_curPath, 我的理解这里 pwd 就会得到 rfm_curPath 的值
if [[ -z "$target" ]]; then
	echo "pwd return empty" > 2
	exit 2
fi

if [[ "$1" != "cp" && "$1" != "mv" ]]; then
	echo "parameter 1 should be either cp or mv" > 2
	exit 5
fi

sourcefiles=$(wl-paste)
# TODO: wl-paste is for wayland, what if x11?

clipboardContent=0
for src in $sourcefiles;do
	echo $src
	clipboardContent+=1
done

if [[ $clipboardContent -eq 0 ]];then
	echo "请输入源路径(键盘上下箭头选取历史输入):"
else
	echo "直接回车从上面显示剪贴板内容读取源文件名;"
	echo "或输入源路径(键盘上下箭头选取历史输入):"
fi

source="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [[ ! -z "$source" ]];then

	if [[ -d $source ]];then
		read -p "输入路径是个目录,为此目录新开rfm窗口以便选择源文件,还是复制整个目录(Y/n)?回车默认新开rfm,输入n复制整个目录" -r new_rfm
	fi

	if [[ -d $source && ( -z "$new_rfm" || "$new_rfm" == "Y" || "$new_rfm" == "y" ) ]];then
		export namedpipe="/tmp/rfmFileChooser_$$"
		if [[ ! -p $namedpipe ]];then
			#trap "rm -f $namedpipe" EXIT
		        mkfifo $namedpipe; \
			while read line ;do \
				if [[ "$1" == "cp" ]];then \
					cp -v -r $line $target; \
				else \
					mv -v $line $target; \
				fi \
			done <$namedpipe &
			rfm -d $source -r $namedpipe
			wait
			rm -f $namedpipe
		else
			echo "$namedpipe already exists" > 2
			exit 4
		fi
	elif [[ -f $source || "$new_rfm" == "n" ]];then
		if [[ "$1" == "cp" ]];then
			cp -i -v -r $source $target
		else
			mv -i -v $source $target
		fi
	else
		echo "$source not directory or file" > 2
		exit 3
	fi
else
	for sourcefile in $sourcefiles;do
		if [[ "$1" == "cp" ]];then
			cp -i -v -r $sourcefile $target
		else
			mv -i -v $sourcefile $target
		fi
	done
fi
