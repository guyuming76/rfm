#!/bin/bash

if [ -n "$RFM_grepMatch" ]; then
	# 使用:分割,输出第一个字串
	LineNumber=$(echo $RFM_grepMatch | cut -d ":" -f 1)
	# 正则表达式判读是否是数字
	if [[ $LineNumber =~ ^[0-9]+$ ]]; then
		emacs --no-splash +$LineNumber "$@" 
		exit
	fi
fi

emacs --no-splash "$@"
