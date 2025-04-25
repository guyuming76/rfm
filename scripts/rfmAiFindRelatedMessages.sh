#!/bin/bash

#set -x

# 首先获取直接关联的文件和ID
readarray -t new_matched_files < <(source scripts/rfmAiFindRelatedMessages_Direct.sh "$@")
# 建立matched_files初始值
matched_files=("${new_matched_files[@]}")


# 循环查找间接关联的文件
while : ; do
    # 从新文件中提取ID和历史ID
    readarray -t ids_from_new_matched_files < <(
        for file in "${new_matched_files[@]}"; do
            # 提取ID
            extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]'
	    echo # 确保每个ID后有换行
            # 提取历史ID
            extractContentFromMarkdownRecursive 0 "$file" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]'
	    echo  # 确保每个ID后有换行
        done | sort | uniq
    )

    # 通过比较两个排序数组，找出新ID
    readarray -t new_ids < <(
        comm -23 <(printf "%s\n" "${ids_from_new_matched_files[@]}" | sort) \
                 <(printf "%s\n" "${Matched_IDs[@]}" | sort) | grep -v '^[[:space:]]*$'
    )
    
    [ ${#new_ids[@]} -eq 0 ] && break  # 没有新ID则退出

    # 查找包含新ID的文件
    readarray -t matched_files_from_new_ids < <(
        for id in "${new_ids[@]}"; do
            locate "*.rfmAI.md" | xargs grep -l "$id" 2>/dev/null
        done | sort | uniq
    )
    
    # 过滤掉已经处理过的文件
    readarray -t new_matched_files < <(
        comm -23 <(printf "%s\n" "${matched_files_from_new_ids[@]}" | sort) \
                 <(printf "%s\n" "${matched_files[@]}" | sort) | grep -v '^[[:space:]]*$'
    )
    
    [ ${#new_matched_files[@]} -eq 0 ] && break  # 没有新文件则退出

    # 合并新文件
    matched_files+=("${new_matched_files[@]}")

    # 合并新ID, Matched_IDs 初始值源自 rfmAiFindRelatedMessages_Direct.sh
    Matched_IDs+=("${ids_from_new_matched_files[@]}")
    readarray -t Matched_IDs < <(printf "%s\n" "${Matched_IDs[@]}" | sort | uniq)
done

# 输出最终结果
printf "%s\n" "${matched_files[@]}"
