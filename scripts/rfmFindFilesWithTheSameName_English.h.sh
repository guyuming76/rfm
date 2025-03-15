#!/bin/bash

# 获取选中的文件列表
selected_files=("$@")

# 如果没有选中文件,退出
if [ ${#selected_files[@]} -eq 0 ]; then
    echo "please select at least one file"
    exit 1
fi

# 遍历每个选中的文件
for file in "${selected_files[@]}"; do
    # 获取文件名(不含路径)
    filename=$(basename "$file")
    
    # 使用find命令查找同名文件
    find / -name "$filename" 2>/dev/null | while read found_file; do
        echo "$found_file"
    done
done