#!/bin/bash

#set -x

# 验证输入参数为 *.rfmAI.md 文件，如果不是，报错退出，如果是，保存至变量
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 file.rfmAI.md" >&2
    exit 1
fi

case "$1" in
    *.rfmAI.md) INPUT_FILE="$1" ;;
    *) echo "Error: Input must be a *.rfmAI.md file." >&2; exit 2 ;;
esac

# 获取传入 rfmAI.md 文件中 ID 内容
ID=$(extractContentFromMarkdownRecursive 0 "$INPUT_FILE" "ID" | tr -d '[:space:]')

if [ -z "$ID" ]; then
    echo "Error: Could not find ID in $INPUT_FILE" >&2
    exit 3
fi

# HistoryID是个数组，过滤空行和空白
readarray -t HistoryID < <(extractContentFromMarkdownRecursive 0 "$INPUT_FILE" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]')

# 查找所有.rfmAI.md文件
FILES=$(locate "*.rfmAI.md")

# 1. 查找ID匹配HistoryID的文件
{
    for file in $FILES; do
        file_id=$(extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]')
        for hid in "${HistoryID[@]}"; do
            if [ "$file_id" = "$hid" ]; then
                echo "$file"
                break
            fi
        done
    done
    
    # 2. 查找内容包含ID的文件
    grep -l "$ID" $FILES
} | sort | uniq
