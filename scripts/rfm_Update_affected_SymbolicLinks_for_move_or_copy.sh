#!/bin/bash

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
	if [[ -z "$rfm_overwrite_destination" || "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]];then
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
		link_target_fullpath=$(cd $(dirname "$SymbolicLink"); cd "$link_target"; pwd)
	else
		link_target_fullpath=$(cd $(dirname "$SymbolicLink"); cd $(dirname "$link_target"); pwd)/$(basename "$link_target")
	fi
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

		echo "$SymbolicLink" | grep -q "^$OldAddress"
		#目前的使用场景都是在git仓库内部,所以下面建立符号链接使用相对地址, 但如果$SymbolicLink位于OldAddress下面,也就是说结下来会被移动或复制,那么移动后的符号链接相对路径就失效了,所以我们用绝对路径,移动后再更新为相对路径
		#即使现在mv还没有发生, new_link_target_fullpath 文件还不存在, 但我运行下来以这个还不存在的路径作为目标建立符号链接是可以的
		if [ $? -eq 0 ]; then
			ln -sfT "$new_link_target_fullpath" "$SymbolicLink"
			if [ -n "$G_MESSAGES_DEBUG" ]; then
				echo "  target will be changed back to relative after move or copy"
			fi
		else
			ln -srfT "$new_link_target_fullpath" "$SymbolicLink"
			if [ -n "$G_MESSAGES_DEBUG" ]; then
				echo "  new relativ target:" $(readlink "$SymbolicLink")
			fi
		fi

	else
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
