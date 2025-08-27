#!/bin/bash

#第一个参数是变量名字符串，rfmReedline 获取的输入先写进 namedpipe, 然后再更新以此为名的变量
#第二个参数是一个可选的历史文件路径字符串

export namedpipe="/tmp/rfmReedline_$$"
if [[ ! -p $namedpipe ]];then
	mkfifo $namedpipe
	rfmReedline $namedpipe "$2" &
	read -r input_destination<$namedpipe
	rm -f $namedpipe
else
        echo "$namedpipe already exists" >&2
        exit 4
fi
