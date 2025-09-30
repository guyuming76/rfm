#!/bin/bash

# 获取选中的文件列表
selected_files=("$@")

# 如果没有选中文件,退出
if [ ${#selected_files[@]} -eq 0 ]; then
 echo "请选择至少一个文件" >&2
 exit 1
fi

# 函数：获取文件的真实inode（解析符号链接）
get_real_inode() {
    local file="$1"
    local real_path
    local inode
    
    # 如果文件是符号链接，解析到实际文件
    if [ -L "$file" ]; then
        real_path=$(readlink -f "$file" 2>/dev/null)
        if [ -n "$real_path" ] && [ -e "$real_path" ]; then
            file="$real_path"
        fi
    fi
    
    # 获取inode
    inode=$(stat -c %i "$file" 2>/dev/null)
    if [ -z "$inode" ]; then
        # 如果上面的命令失败，尝试使用不同的stat格式（适用于macOS）
        inode=$(stat -f %i "$file" 2>/dev/null)
    fi
    
    echo "$inode"
}

# 遍历每个选中的文件
for file in "${selected_files[@]}"; do
    # 先输出选中文件本身，TODO: 如果选中的多个文件本身是指向同一inode的，这里不去除，这逻辑似乎也说得过去?
    echo $(readlink -f "$file")

    # 获取文件名(不含路径)
    filename=$(basename "$file")
    
    # 获取原始文件的真实inode号（解析符号链接）
    original_inode=$(get_real_inode "$file")
    
    if [ -z "$original_inode" ]; then
        echo "无法获取文件'$file'的inode信息" >&2
        continue
    fi

    # 使用find命令查找同名文件
    find "$RFM_GIT_ROOT" -not -path '*/.git/*' -name "$filename" 2>/dev/null | while read found_file; do
        # 获取找到的文件的真实inode（解析符号链接）
        found_inode=$(get_real_inode "$found_file")
        
        # 只有当inode不同时才输出（不是同一个文件）
        if [ -n "$found_inode" ] && [ "$found_inode" != "$original_inode" ]; then
            echo "$found_file"
        fi
    done
done
