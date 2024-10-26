#!/bin/bash

#如果文件 $3 是符号链接,且指向某目录 $2 下的文件,则更新符号链接,指向新的路径 $1 (因为目录 $2 将会被移动到 $1)

NewAddress=$1

# 源自传递给 rfmMoveDirAndUpdateSymbolicLink.sh 的传入参数 Source, 再由 rfmMoveDirAndUpdateSymbolicLink.sh 传入本脚本
OldAddress=$2
SymbolicLink=$3

#如果$3是符号链接
if [[ -L "$SymbolicLink" ]]; then
	real_path=$(realpath "$SymbolicLink")

	if [[ "$NewAddress" =~ ":" || "$OldAddress" =~ ":" || "$real_path" =~ ":" ]]; then
		echo "Error: filepath contain :, which is used by sed in this script" >&2
		exit 1
	fi

	# test 命令比较文件都是dereference 符号链接的
	# 下面 \; 里的\用来防止shell expansion 处理了; 而;这里是find命令读取,用来标记 -exec的终止的,参见 man find
	if find "$OldAddress" -exec test {} -ef "$SymbolicLink" \; -print | read; then
		echo "update symbolic link:" "$SymbolicLink" >&2
		echo "	old target:" "$real_path" >&2
		if [[ -d "$OldAddress" ]]; then
			echo "	replace:   " "$OldAddress" >&2
			echo "	with:      " "$NewAddress" >&2

			# sed 里面引用变量,算是疑难杂症, 下面用""表示双引号.
			# 又因为变量值包含路径符号/, 所以sed 换用:作为分隔符
			new_real_path=$(echo "$real_path" | sed "s:^""$OldAddress"":""$NewAddress"":")
		else
			#当 OldAddress 是一个文件, 而不是目录时, 要替换的是 OldAddress 的parent目录
			Source=$(dirname "$OldAddress")
			echo "	replace:   " "$Source" >&2
			echo "	with:      " "$NewAddress" >&2
			new_real_path=$(echo "$real_path" | sed "s:^""$Source"":""$NewAddress"":")
		fi
		ln -sf "$new_real_path" "$SymbolicLink"
		echo "	new target:" "$new_real_path" >&2
	fi
fi
