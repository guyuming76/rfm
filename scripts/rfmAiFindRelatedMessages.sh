#!/bin/bash

set -x

# 验证输入参数为 *.rfmAI.md 文件
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 file.rfmAI.md" >&2
    exit 1
fi

case "$1" in
    *.rfmAI.md) INPUT_FILE="$1" ;;
    *) echo "Error: Input must be a *.rfmAI.md file." >&2; exit 2 ;;
esac

# 首先获取直接关联的文件和ID
readarray -t matched_files < <(scripts/rfmAiFindRelatedMessages_Direct.sh "$INPUT_FILE")
readarray -t matched_ids < <(
    for file in "${matched_files[@]}"; do
        # 提取ID
        extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]'
        # 提取历史ID
        extractContentFromMarkdownRecursive 0 "$file" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]'
    done | sort | uniq
)

# 递归查找间接关联的文件
while : ; do
    # 找出未处理的新ID
    readarray -t new_ids < <(
        for id in "${matched_ids[@]}"; do
            grep -l "$id" "${matched_files[@]}" >/dev/null || echo "$id"
        done | sort | uniq
    )
    
    [ ${#new_ids[@]} -eq 0 ] && break  # 没有新ID则退出

    # 查找包含新ID的文件
    readarray -t new_files < <(
        for id in "${new_ids[@]}"; do
            locate "*.rfmAI.md" | xargs grep -l "$id" 2>/dev/null
        done | sort | uniq
    )
    
    # 过滤掉已经处理过的文件
    readarray -t new_files < <(
        comm -23 <(printf "%s\n" "${new_files[@]}" | sort) \
                 <(printf "%s\n" "${matched_files[@]}" | sort) | grep -v '^[[:space:]]*$'
    )
    
    [ ${#new_files[@]} -eq 0 ] && break  # 没有新文件则退出

    # 合并新文件
    matched_files+=("${new_files[@]}")
    
    # 从新文件中提取ID和历史ID
    readarray -t new_ids_from_files < <(
        for file in "${new_files[@]}"; do
            # 提取ID
            extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]'
            # 提取历史ID
            extractContentFromMarkdownRecursive 0 "$file" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]'
        done | sort | uniq
    )
    
    # 合并新ID
    matched_ids+=("${new_ids_from_files[@]}")
    readarray -t matched_ids < <(printf "%s\n" "${matched_ids[@]}" | sort | uniq)
done

# 输出最终结果
printf "%s\n" "${matched_files[@]}"
