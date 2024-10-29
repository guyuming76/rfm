#!/bin/bash

# Source 和 Destination 需要是不同的目录
# 或者 Source 是文件, 而Destination 是目录,如同mv 命令参数
# 从 Source 复制到 Destination
# 把原指向 Source 目录内 (包括Source本身) 的符号链接更新指向 Desitination 目录内

Source=$1
Destination=$2

if [[ ${Source:0:1} != "/" || ${Destination:0:1} != "/" ]]; then
	echo "absolute address is required for parameters" >&2
	exit 1;
else
	echo "----------------------------"
	echo "Source:" "$Source" "     Destination:" "$Destination" >&2
fi

# 获取查找范围,参见rfmFindLinksToTheSameFile_Chinese.h.sh
# rfmFindScope 会是绝对路径形式
rfmFindScope=$(git rev-parse --show-toplevel 2>/dev/null)
if [[ -z  "$rfmFindScope" ]]; then
       	rfmFindScope="/"
fi

#if [[ -d "$Source" ]]; then
	# 下面 SpecificDir 实际指的是 Source
	# 因为 $rfmFindScope 是绝对路径形式, find 的输出也是绝对路径形式
	find "$rfmFindScope" -type l -exec rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh "$Destination" "$Source" {} \;
#fi

if [[ "$rfmFindScope"=="/" ]]; then
	mv "$Source" "$Destination"
else
	git mv "$Source" "$Destination"
fi
