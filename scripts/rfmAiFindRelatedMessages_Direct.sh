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

# 为每个pattern构建grep命令
GREP_CMD="grep -l"
for pattern in "${!unique_patterns[@]}"; do
    GREP_CMD+=" -e \"$pattern\""
done

# 在找到的文件中搜索匹配pattern的内容
locate "*.rfmAI.md" | xargs -I {} sh -c "$GREP_CMD {} 2>/dev/null" | sort | uniq
