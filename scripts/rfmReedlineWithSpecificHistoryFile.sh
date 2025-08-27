#!/bin/bash

#第一个参数是变量名字符串，rfmReedline 获取的输入先写进 namedpipe, 然后再更新以此为名的变量
#第二个参数是一个可选的历史文件路径字符串

export namedpipe="/tmp/rfmFileChooser_$$"
if [[ ! -p $namedpipe ]];then
	mkfifo $namedpipe
	export $1=$(cat $namedpipe) &
	rfmReedline $namedpipe "$2"
	rm -f $namedpipe
else
        echo "$namedpipe already exists" >&2
        exit 4
fi
