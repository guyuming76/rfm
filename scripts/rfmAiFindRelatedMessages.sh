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

# 将所有模式合并为一个数组，并去重
declare -A unique_patterns
unique_patterns["$ID"]=1
for hid in "${HistoryID[@]}"; do
    [ -n "$hid" ] && unique_patterns["$hid"]=1
done

# 递归查找相关ID
while : ; do
    # 为每个pattern构建grep命令
    GREP_CMD="grep -l"
    for pattern in "${!unique_patterns[@]}"; do
        GREP_CMD+=" -e \"$pattern\""
    done

    # 在找到的文件中搜索匹配pattern的内容
    matched_files=$(locate "*.rfmAI.md" | xargs -I {} sh -c "$GREP_CMD {} 2>/dev/null" | sort | uniq)
    
    # 如果没有匹配到新文件，退出循环
    [ -z "$matched_files" ] && break

    # 从匹配文件中提取新ID
    new_ids=0
    for file in $matched_files; do
        # 提取ID
        id=$(extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]')
        [ -n "$id" ] && [ -z "${unique_patterns[$id]}" ] && unique_patterns["$id"]=1 && ((new_ids++))
        
        # 提取历史ID
        readarray -t history_ids < <(extractContentFromMarkdownRecursive 0 "$file" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]')
        for hid in "${history_ids[@]}"; do
            [ -n "$hid" ] && [ -z "${unique_patterns[$hid]}" ] && unique_patterns["$hid"]=1 && ((new_ids++))
        done
    done

    # 如果没有新ID加入，退出循环
    [ $new_ids -eq 0 ] && break
done

# 输出最终匹配的文件
echo -e $matched_files

#for pattern in "${!unique_patterns[@]}"; do
#    grep -l -e "$pattern" $matched_files 2>/dev/null
#done | sort | uniq
