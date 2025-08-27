#!/bin/bash

#第一个参数是脚本名字符串
#第二个参数是历史文件路径字符串
#第三个及其后面的参数都要传递给第一个参数表示的脚本

export namedpipe="/tmp/rfmReedline_$$"
if [[ ! -p $namedpipe ]];then
	mkfifo $namedpipe
	while read rfmReadlineUserInputVar; do $1 $rfmReadlineUserInputVar ${@:3}; done < $namedpipe &
	rfmReedline $namedpipe "$2"
	rm -f $namedpipe
else
        echo "$namedpipe already exists" >&2
        exit 4
fi
