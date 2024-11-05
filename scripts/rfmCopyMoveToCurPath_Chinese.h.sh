#!/bin/bash
#set -x

target=$(pwd)
# rfm使用g_spawn执行脚本时,第一个参数working directory会传入 rfm_curPath, 我的理解这里 pwd 就会得到 rfm_curPath 的值
if [[ -z "$target" ]]; then
	echo "pwd return empty" >2
	exit 2
fi

if [[ "$1" != "cp" && "$1" != "mv" && "$1" != "sl" ]]; then
	echo "parameter 1 should be either cp or mv or sl(soft link)" >2
	exit 5
fi

sourcefiles=$(rfmFromClipboard.sh)

clipboardContent=0
for src in "$sourcefiles";do
	echo "$src"
	clipboardContent+=1
done

if [[ "$clipboardContent" -eq 0 ]];then
	echo "请输入源路径(键盘上下箭头选取历史输入):"
else
	echo "直接回车从上面显示剪贴板内容读取源文件名;"
	echo "或输入源路径(键盘上下箭头选取历史输入):"
fi

source="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [[ "$1" == "sl" ]];then
	read -p "是否(y/n)使用相对路径创建软链接?默认是(y)" -r useRelativeInSL
fi

read -p "是否(y/n)检查移动或复制造成的符号链接破坏并修复?默认是(y)" -r check_and_update_symbolic_link


if [[ ! -z "$source" ]];then

	if [[ -d "$source" ]];then
		read -p "输入路径是个目录,为此目录新开rfm窗口以便选择源文件,还是复制整个目录(Y/n)?回车默认新开rfm,输入n复制整个目录" -r new_rfm
	fi

	if [[ -d "$source" && ( -z "$new_rfm" || "$new_rfm" == "Y" || "$new_rfm" == "y" ) ]];then
		export namedpipe="/tmp/rfmFileChooser_$$"
		if [[ ! -p $namedpipe ]];then
			#trap "rm -f $namedpipe" EXIT
		        mkfifo "$namedpipe"; \
			while read line ;do \
				basename_line=$(basename "$line")
				target_with_basename_line=$(echo "$target"/"$basename_line")
				if [[ "$1" == "cp" ]];then \
					cp -v -r "$line" "$target"; \
				elif [[ "$1" == "mv" ]];then \
					mv -v "$line" "$target"; \
				elif [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then \
					ln -sr "$line $target_with_basename_line"; \
				else \
					ln -s "$line $target_with_basename_line"; \
				fi \
			done <$namedpipe &
			rfm -d "$source" -r $namedpipe
			wait
			rm -f $namedpipe
		else
			echo "$namedpipe already exists" >2
			exit 4
		fi
	elif [[ -f "$source" || "$new_rfm" == "n" ]];then
		basename_source=$(basename "$source")
		target_with_basename_source=$(echo "$target"/"$basename_source")
		if [[ "$1" == "cp" ]];then
			cp -i -v -r "$source" "$target"
		elif [[ "$1" == "mv" ]];then
			mv -i -v "$source" "$target"
		elif [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
			ln -sr "$source" "$target_with_basename_source")
		else
			ln -s "$source" "$target_with_basename_source")
		fi
	else
		echo "$source not directory or file" >2
		exit 3
	fi
else
	for sourcefile in "$sourcefiles";do
		basename_sourcefile=$(basename "$sourcefile")
		target_with_basename_sourcefile=$(echo "$target"/"$basename_sourcefile")
		if [[ "$1" == "cp" ]];then
			cp -i -v -r "$sourcefile" "$target"
		elif [[ "$1" == "mv" ]];then
		        if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_linj" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
		                rfmMoveDirAndUpdateSymbolicLinks.sh "$sourcefile" "$target"
			else
			        mv -i -v "$sourcefile" "$target"
			fi
		elif [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
			ln -sr "$sourcefile" "$target_with_basename_sourcefile")
		else
			ln -s "$sourcefile" "$target_with_basename_sourcefile")
		fi
	done
fi

if [[ -z "$check_and_update_symbolic_link" || "$check_and_update_symbolic_linj" == "y" || "$check_and_update_symbolic_link" == "Y" ]];then
        if [ -n "$G_MESSAGES_DEBUG" ];then
	        echo
	        echo -e "\e[1;33m 按回车关闭窗口 \e[0m"
	        read
	fi
fi
