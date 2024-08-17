#!/bin/bash

#本脚用法,比如在mineral目录下,运行下面这行, 找到所有链接文件,通过管道传递给本程序
# find images/spImg -type l |  ~/rfm/scripts/UpdateSymbolicLinkAbsoluteToRelative.sh

while
	read SymbolicLink; do
	echo --- $SymbolicLink
	Link=$(readlink "$SymbolicLink")
	echo $Link

	if [[ -f "$Link" && ! -d "$Link" ]]; then
		echo "update"
		rm $SymbolicLink
		ln -sr $Link $SymbolicLink
	else
		echo "skip"
	fi
done
