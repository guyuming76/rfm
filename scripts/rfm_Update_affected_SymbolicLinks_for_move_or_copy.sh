#!/bin/bash

#set -x

#如果文件 $3 是符号链接,且指向某目录 $2 下的文件,则更新符号链接,指向新的路径 $1 (因为目录 $2 将会被移动到 $1)

# 源自传递给 rfmMoveDirAndUpdateSymbolicLink.sh 的传入参数 Destination, 再由 rfmMoveDirAndUpdateSymbolicLink.sh 传入本脚本
NewAddress=$1

# 源自传递给 rfmMoveDirAndUpdateSymbolicLink.sh 的传入参数 Source, 再由 rfmMoveDirAndUpdateSymbolicLink.sh 传入本脚本
OldAddress=$2

# 源自 rfmMoveDirAndUpdateSymbolicLink.sh 内 find -exec 中的 {}
SymbolicLink=$3

if [[ ${NewAddress:0:1} != "/" || ${OldAddress:0:1} != "/" || ${SymbolicLink:0:1} != "/" ]]; then
        echo "absolute address is required for parameters" >&2
        exit 1;
fi

#如果 NewAddress 存在且为目录，则表示 OldAddress 移入 NewAddress 下面，相当与 OldAddress 重命名为$NewAddress/$(basename "$OldAddress")
#如果 NewAddress 不存在，则表示 OldAddress 重命名为 NewAddress
if [[ -d "$NewAddress" ]];then
	if [[ -n "$rfm_overwrite_destination" ]] && [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
        # 相当于 mv ~/mineral/images/spImg ~/mineral/矿物名称 这种情况，矿物名称 符号链接存在，会被 矿物名称 目录覆盖
		DestinationWithBasename=$NewAddress
	else
        # 相当于 mv ~/mineral/images/spImg ~/mineral 这种情况， NewAddress 是 ～/mineral, DestinationWithOldBasename=～/mineral/spImg
		DestinationWithBasename=$NewAddress/$(basename "$OldAddress")
	fi
else
        # 相当于 mv ~/mineral/images/spImg ~/mineral/矿物名称 这种情况，矿物名称目录还不存在。也可以理解为前一种情况加上 spImg 到 矿物名称的改名
        DestinationWithBasename=$NewAddress
fi


#如果$3是符号链接; 事实上,这个判断总是为真,因为rfmMoveDirAndUpdateSymbolicLink.sh的find 循环里就限定了 -type l
if [[ -L "$SymbolicLink" ]]; then
	#link_target 为符号链接只解析一层,
	#也就是说如果 SymbolicLink 指向L2, L2也是符号链接指向L1,下面link_target应该返回L2,而不是L1
	#在mineral 仓库里,用的是相对路径
	link_target=$(readlink "$SymbolicLink")
	if [[ -d "$SymbolicLink" ]]; then
    		link_target_fullpath=$(cd "$(dirname "$SymbolicLink")" && cd "$link_target" && pwd)
	else
    		link_target_fullpath="$(cd "$(dirname "$SymbolicLink")" && cd "$(dirname "$link_target")" && pwd)/$(basename "$link_target")"
	fi
# 下为什么在嵌套的命令替换中**不需要**转义内层的双引号。
# 在Bash中，命令替换 `$()` 创建了一个**新的解析上下文**。当Bash解析：
# $(cd "$(dirname "$SymbolicLink")" && cd "$link_target" && pwd)
# 解析过程是这样的：
# 1. **外层命令替换**：`$(...)` 开始一个新的解析上下文
# 2. **内层命令替换**：`$(dirname "$SymbolicLink")` 中的 `"$SymbolicLink"` 是在内层上下文中，所以双引号不需要转义
# 3. **变量扩展**：`"$link_target"` 也是在外层命令替换的上下文中，所以也不需要转义

	#link_target_fullpath=$(realpath --relative-base=$(dirname "$SymbolicLink") "$link_target")

	if [[ "$NewAddress" =~ ":" || "$OldAddress" =~ ":" ]]; then
		echo "Error: filepath contain :, which is used by sed in this script" >&2
		exit 2
	fi

	#echo "check symbolic link: " "$SymbolicLink"
	#echo "  link target full:  " "$link_target_fullpath"
	#echo "  ?under OldAddress: " "$OldAddress"

	# 如果 $link_target_fullpath 以 $OldAddress 开头
	echo "$link_target_fullpath" | grep -q "^$OldAddress"
	#如果上面令返回 0
	if [ $? -eq 0 ]; then
		if [ -n "$G_MESSAGES_DEBUG" ]; then
			echo "update symbolic link:" "$SymbolicLink"
			echo "  link target:       " "$link_target"
			echo "  link target full:  " "$link_target_fullpath"
		fi

		# 检查当前处理的符号链接是否就是目标位置将要被覆盖的符号链接
		if [[ "$SymbolicLink" == "$DestinationWithBasename" ]]; then
		    # 如果这个符号链接就是目标位置，移动后它将被物理文件覆盖，所以不需要更新
		    if [ -n "$G_MESSAGES_DEBUG" ]; then
			echo "跳过目标位置的同名符号链接，移动后将被覆盖: $SymbolicLink"
		    fi
		    exit 0
		fi

		# 如果移动的(OldAddress)是目录, 那么所有指向 OldAddress 目录内文件的符号链接都要被更新
		# 此时 link_target_fullpath 可以等于 OldAddress, 但不一定, 也可以是 link_target_fullpath 位于 OldAddress 下面
		new_link_target_fullpath=$(echo "$link_target_fullpath" | sed "s:^""$OldAddress"":""$DestinationWithBasename"":")

		if [ -n "$G_MESSAGES_DEBUG" ]; then
			echo "  OldAddress:        " "$OldAddress"
			echo "  NewAddress:        " "$NewAddress"
			echo "  Destination:       " "$DestinationWithBasename"
			echo "  By replacing OldAddress with Destination, we get:"
			echo "  new target full:   " "$new_link_target_fullpath"
		fi

		if [[ -e "$new_link_target_fullpath" ]]; then
		        rm "$new_link_target_fullpath" # 如果和$new_link_target_fullpath 的同名符号链接存在，不删除得话，貌似下面ln命令会生成的相对路径链接实际指向的不是$new_link_target_fullpath, 而是$new_link_target_full 指向的文件，所以先把这个同名文件删除
			echo "  new target full ($new_link_target_fullpath) already exists, deleted here."
		fi

		echo "$SymbolicLink" | grep -q "^$OldAddress"
		#目前的使用场景都是在git仓库内部,所以下面建立符号链接使用相对地址, 但如果$SymbolicLink位于OldAddress下面,也就是说结下来会被移动或复制,那么移动后的符号链接相对路径就失效了,所以我们用绝对路径,移动后再更新为相对路径
		#即使现在mv还没有发生, new_link_target_fullpath 文件还不存在, 但我运行下来以这个还不存在的路径作为目标建立符号链接是可以的

		if [ $? -eq 0 ]; then
			if ! ln -sfT "$new_link_target_fullpath" "$SymbolicLink"; then
				echo "创建绝对路径符号链接失败: $SymbolicLink -> $new_link_target_fullpath" >&2
				exit 10
			fi
			if [ -n "$G_MESSAGES_DEBUG" ]; then
				echo "  target will be changed back to relative after move or copy"
			fi
		else
		#如果NewAddress已存在，也就是说在mv/cp后会被覆盖，这时生成的new relative target 会错误, 如devPicAndVideo/20241106_04h01m30s_grim.png 所示 
		#本commit解决办法是在运行本脚本前 git rm 掉 $Destination
			if ! ln -srfT "$new_link_target_fullpath" "$SymbolicLink"; then
				echo "创建相对路径符号链接失败: $SymbolicLink -> $new_link_target_fullpath" >&2
				exit 11
			fi
			if [ -n "$G_MESSAGES_DEBUG" ]; then
				echo "  new relativ target:" $(readlink "$SymbolicLink")
			fi
		fi

	else
	        #TODO：本commit添加了 [[ -e "$new_link_target_fullpath" ]] 的判断，如果已存在，删除。这里这个else分支，是否要类似处理 $link_target_fullpath, 防止下面ln语句结果不正确？ 这会儿还没遇到use case,先不动。
		echo "$SymbolicLink" | grep -q "^$OldAddress"
		# 如果SymbolicLink本身位于$OldAddress下面,尽管不是指向$OldAddress下面,也需要重建,因为目前在git仓库里,我们都使用相对路径建立符号链接
		# TODO: 目前,我们假定OldAddress会被move,在mv之前,先把符号链接更新为绝对路径, 移动之后,再更新为相对路径,否则咋搞? 若是copy, 复制完后,两份符号链接都得再改回相对路径
		if [ $? -eq 0 ]; then
			ln -sfT "$link_target_fullpath" "$SymbolicLink"
			if [ -n "$G_MESSAGES_DEBUG" ]; then
				echo "recreate symlink before move:  " "$SymbolicLink"
				echo "  old relative target:         " "$link_target"
				echo "  new fullpath target:         " "$link_target_fullpath"
				echo "  target will be changed back to relative after move or copy"
			fi
		fi
	fi
fi
