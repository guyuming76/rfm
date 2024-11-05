#!/bin/bash

# Source 和 Destination 需要是不同的目录
# 或者 Source 是文件, 而Destination 是目录,如同mv 命令参数
# 从 Source 移动到 Destination
# 把原指向 Source 目录内 (包括Source本身) 的符号链接更新指向 Desitination 目录内
# Source 目录内的相对路径符号链移动前要使用绝对路径重建，移动后再使用相对路径重建

Source=$1
Destination=$2

if [[ ${Source:0:1} != "/" || ${Destination:0:1} != "/" ]]; then
	echo "absolute address is required for parameters" >&2
	exit 1;
elif [ -n "$G_MESSAGES_DEBUG" ]; then
	echo "----------------------------"
	echo "Source:" "$Source" "     Destination:" "$Destination"
fi

# 获取查找范围,参见rfmFindLinksToTheSameFile_Chinese.h.sh
# rfmFindScope 会是绝对路径形式
rfmFindScope=$(git rev-parse --show-toplevel 2>/dev/null)
if [[ -z  "$rfmFindScope" ]]; then
       	rfmFindScope="/"
fi


	# 下面 SpecificDir 实际指的是 Source
	# 因为 $rfmFindScope 是绝对路径形式, find 的输出也是绝对路径形式
find "$rfmFindScope" -type l -exec rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh "$Destination" "$Source" {} \;

if [[ "$rfmFindScope"=="/" ]]; then
	mv "$Source" "$Destination"
else
	git mv "$Source" "$Destination"
# 在把 mineral/images/spImg 目录移动到  mineral/矿物名称 目录的案例中，我们需要先删除 矿物名称 符号链接，然后再选中 spImg 目录，调用文件上下文菜单 ”移动（改名）... 功能；尝试不删除符号链接，使用下面 git mv -f 移动，但未成功，可能是上面调用脚本里对 $Destination 的判断“
#	git mv -f "$Source" "$Destination"
fi

if [ -n "$G_MESSAGES_DEBUG" ]; then
	echo "###########################"
fi

#find "$Destination" -type l | while read -r symlink; do symlink_target=$(readlink "$symlink"); if [ -n "$G_MESSAGES_DEBUG" ]; then echo "#####:" "$symlink"; echo "$symlink_target"; fi; ln -srfn "$symlink_target" "$symlink"; done
# Destination 下面所有符号链接，若是使用绝对路径（以 / 开头）， 使用相对路径重建
find "$Destination" -type l | while read -r symlink; do symlink_target=$(readlink "$symlink"); if [[ ${symlink_target:0:1} == "/" ]]; then ln -srfn "$symlink_target" "$symlink"; fi; done
