#!/bin/bash

# 获取选中的文件列表
selected_files=("$@")

# 如果没有选中文件,退出
if [ ${#selected_files[@]} -eq 0 ]; then
    echo "请选择至少一个文件" >&2
    exit 1
fi

# 遍历每个选中的文件
for file in "${selected_files[@]}"; do
    # 获取文件名(不含路径)
    filename=$(basename "$file")
    parent_dir=$(dirname filename)

    # 检查给定的目标位置是否处于某个Git仓库内部
    inside_work_tree=$(git -C "$parent_dir" rev-parse --is-inside-work-tree)

    if [[ "$inside_work_tree" == "true" ]]; then
    	repo_root=$(git -C "$parent_dir" rev-parse --show-toplevel)
    	echo "文件'$filename'位于git仓库内,系统使用find命令在仓库内查找" >&2
    	# 使用find命令查找同名文件
    	find "$repo_root" -not -path '*/.git/*' -name "$filename" 2>/dev/null | while read found_file; do
        	echo "$found_file"
    	done
    else
    	echo "系统使用locate命令查找同名文件,你有可能需要使用updatedb命令更新文件索引" >&2
	locate $(filename)
    fi
done
