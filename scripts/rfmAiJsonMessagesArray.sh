#!/bin/bash

# Prepare conversation messages array
MESSAGES_ARRAY=""
RESPONSE_ID_ARRAY=()

# Add history messages if any
if [ ${#MESSAGES_LIST[@]} -gt 0 ]; then
    for history_file in "${MESSAGES_LIST[@]}"; do
	echo "$history_file"
    done
    echo
    # 用户确认
    read -p "确认上传这些对话历史吗? [Y/n] " confirm
    confirm=${confirm:-Y}
    if [[ "$confirm" =~ ^[Yy]$ ]]; then
	for history_file in "${MESSAGES_LIST[@]}"; do
	    echo "从对话记录文件中读取历史消息: $history_file" >&2

	    # Extract user prompt from history file
	    user_prompt=$(extractContentFromMarkdownRecursive 0 "$history_file" "用户提示词")
	    if [ -n "$user_prompt" ]; then
		MESSAGES_ARRAY+=$(echo -e "$user_prompt" | jq -sR '{"role": "user", "content": .}')
	    fi

	    # Extract AI response from history file
	    ai_response=$(extractContentFromMarkdownRecursive 0 "$history_file" "模型回复内容")
	    if [ -n "$ai_response" ]; then
		MESSAGES_ARRAY+=$(echo -e "$ai_response" | jq -sR '{"role": "assistant", "content": .}')
	    fi

	    responseid=$(extractContentFromMarkdownRecursive 0 "$history_file" "ID")
	    if [ -n "$responseid" ]; then
		RESPONSE_ID_ARRAY+=("$responseid")
	    fi
	done
    fi
fi
