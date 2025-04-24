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
# grep 删除所有只包含空白字符（如空格或制表符）的行。 tr 删除空白，ID中不应该含空白字符
readarray -t HistoryID < <(extractContentFromMarkdownRecursive 0 "$INPUT_FILE" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]')

# Matched_IDs 数组在这个脚本没用，但可以作为 rfmAiFindRelatedMessages.sh 脚本的起点
Matched_IDs=("${HistoryID[@]}" "$ID")

# 查找所有.rfmAI.md文件
readarray -t FILES < <(locate "*.rfmAI.md")

# 1. 查找ID匹配HistoryID的文件，也就是输入参数对话（当前对话）引用的对话，即ID出现在HistoryID数组中的文件,下面为啥不直接在Files里面grep Matched_IDs 成员呢？ 因为那样可能会包含历史ID字段内容出现在 HistoryID 数组，但不是当前对话的对话，也就是和当前对话引用了同样对话的对话，这里不算直接关联的对话，但会算在（间接）关联里面
{
    for file in "${FILES[@]}"; do
        file_id=$(extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]')
        for hid in "${HistoryID[@]}"; do
            if [ "$file_id" = "$hid" ]; then
                echo "$file"
                break
            fi
        done
# 2. 查找包含 ID 的文件，也就是引用当前对话的对话，包括当前对话自己，ID可能出现在文件的历史ID标题下，注意这个不等同于上面的HistoryID数组内容，也可能出现文件其他部分如提示词里，用户在提示词里指明了引用
	grep -l "$ID" $file
    done
} | sort | uniq
