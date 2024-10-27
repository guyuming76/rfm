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

	# 下面 \; 里的\用来防止shell expansion 处理了; 而;这里是find命令读取,用来标记 -exec的终止的,参见 man find
	# 因为 $OldAdress 是绝对路径, 所以假定 {} 也是绝对路径
	#if find "$OldAddress" -exec test "$link_target_fullpath"={} \; -print | read -r fileUnderOldAddress; then
	# 下面这一步 if find 就是为了判断 $link_target_fullpath 是否位于 $OldAddress 目录下,包括 $OldAddress 本身, 有没有简单些的写法?比如路径左边匹配?
	if find "$OldAddress" -exec bash -c 'if [[ "$0" == "$link_target_fullpath" ]]; then echo "$0"; fi' {} \; | read; then
		echo "update symbolic link:" "$SymbolicLink" >&2
		echo "  link target:       " "$link_target" >&2
		echo "  link target full:  " "$link_target_fullpath" >&2
		#echo "  =find {}:  " "$fileUnderOldAddress" >&2

		# 如果移动的(OldAddress)是目录, 那么说有指向 OldAddress 目录内文件的符号链接都要被更新
		# 此时 link_target_fullpath 可以等于 OldAddress, 但不一定, 也可以是 link_target_fullpath 位于 OldAddress 下面
		if [[ -d "$OldAddress" ]]; then
			echo "  duo to dir move:   " "$OldAddress" >&2
			echo "  to dir:            " "$NewAddress" >&2

			# sed 里面引用变量,算是疑难杂症, 下面用""表示双引号.
			# 又因为变量值包含路径符号/, 所以sed 换用:作为分隔符
			new_link_target_fullpath=$(echo "$link_target_fullpath" | sed "s:^""$OldAddress"":""$NewAddress"":")
		# 如果移动的(OldAddress)是文件,
		else
			#当 OldAddress 是一个文件, 而不是目录时, 要替换的是 OldAddress 的parent目录
			Source=$(dirname "$OldAddress")
			echo "  duo to file move:  " "$OldAddress" >&2
			echo "  from dir:          " "$Source" >&2
			echo "  to dir:            " "$NewAddress" >&2
			new_link_target_fullpath=$(echo "$link_target_fullpath" | sed "s:^""$Source"":""$NewAddress"":")
		fi
		#TODO:目前的使用场景都是在git仓库内部,所以下面建立符号链接使用相对地址,如有需求扩展,再做增强
		ln -srf "$new_link_target_fullpath" "$SymbolicLink"
		echo "  new target full:   " "$new_link_target_fullpath" >&2
	fi
fi
