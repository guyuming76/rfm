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
input_file_ID=$(extractContentFromMarkdownRecursive 0 "$INPUT_FILE" "ID" | tr -d '[:space:]')

if [ -z "$input_file_ID" ]; then
    echo "Error: Could not find ID in $INPUT_FILE" >&2
    exit 3
fi

# 初始状态
matched_file_count=0
matched_ids=("$input_file_ID")
matched_id_count=1

# 递归查找相关ID
while : ; do
    # 为每个pattern构建grep命令
    GREP_CMD="grep -l"
    for pattern in "${matched_ids[@]}"; do
        GREP_CMD+=" -e \"$pattern\""
    done

    # 在找到的文件中搜索匹配pattern的内容，注意，这一步匹配到的pattern可能出现在文件中的任何地方，可以是ID,历史ID,或是模型回复内容等
    readarray -t matched_files < <(locate "*.rfmAI.md" | xargs -I {} sh -c "$GREP_CMD {} 2>/dev/null" | sort | uniq)
    #TODO:现在这么做有点低效，应该区分这轮循环得到的文件和上一轮已经存在的，只在新的文件中提取ID和"历史ID",这些ID中也只有新增的部分才用来生成下一轮GREP_CMD，以便得到新的匹配文件；这些新的匹配文件还是有可能和已有匹配文件重复的，要挑出新增的部分去提取ID。 而不是像现在这样：每次都合并 matched_ids 产生新的 GREP_CMD 然后全新的matched_files,造成上一步已经出现在 matched_files 里面的文件又要重新提取一遍历史ID和ID,然后加入matched_ids，肯定是重复的。

    
    # 如果没有匹配到新文件，退出循环,注意，实际上不可能匹配不到新文件，unqiue_patterns 至少包含脚本输入参数文件的ID
    [ -z "$matched_files" ] && break
    # 这一轮没有新的匹配文件，则退出循环
    new_matched_file_count=${#matched_files[@]}
    [ $matched_file_count -eq $new_matched_file_count ] && break
    matched_file_count=$new_matched_file_count
    
    # 从匹配文件中提取新ID
    for file in "${matched_files[@]}"; do
        # 提取ID并合并入matched_ids
        id=$(extractContentFromMarkdownRecursive 0 "$file" "ID" | tr -d '[:space:]')
        [ -n "$id" ] && matched_ids+=("$id")
        
        # 提取历史ID并合并入matched_ids
        readarray -t history_ids < <(extractContentFromMarkdownRecursive 0 "$file" "历史ID" | grep -v '^[[:space:]]*$' | tr -d '[:space:]')
	matched_ids=("${matched_ids[@]}" "${history_ids[@]}")
    done

    #去重
    readarray -t matched_ids < <(for id in ${matched_ids[@]}; do echo $id; done | sort | uniq )
    
    # 如果没有新ID加入，退出循环
    new_matched_id_count=${#matched_ids[@]}
    if [ $matched_id_count -eq $new_matched_id_count ]; then
	# 输出最终匹配的文件
	for file in "${matched_files[@]}"; do echo "$file"; done
	break;
    else
	matched_id_count=$new_matched_id_count
    fi
done
