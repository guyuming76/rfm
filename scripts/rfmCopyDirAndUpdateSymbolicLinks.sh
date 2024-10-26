#!/bin/bash

# Source 和 Destination 需要是不同的目录
# 从 Source 复制到 Destination
# 把原指向 Source 目录内的符号链接更新指向 Desitination 目录内

Source=$1
Destination=$2

# 获取查找范围,参见rfmFindLinksToTheSameFile_Chinese.h.sh
rfmFindScope=$(git rev-parse --show-toplevel 2>/dev/null)
if [[ -z  "$rfmFindScope" ]]; then
       	rfmFindScope="/"
fi

# -p 保持元数据
cp -p "$Source" "$Destination"

#if [[ -d "$Source" ]]; then
	# 下面 SpecificDir 实际指的是 Source
	find "$rfmFindScope" -type l -exec rfm_Update_If_SymbolicLink_PointTo_FileUnderSpecificDir.sh "$Destination" "$Source" {} \;
#fi
