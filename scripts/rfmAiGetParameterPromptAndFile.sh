#!/bin/bash

rfm_AI_MODEL_HOST_URL="$1"
rfm_AI_MODEL_NAME="$2"
rfm_AI_MODEL_TEMPERATURE=$3
rfm_AI_MODEL_API_KEY="$4"

# 读取用户输入
rfm_AI_MODEL_prompt=$(cat)

# 初始化文件列表
FILES_LIST=()

# 从环境变量获取文件列表
if [ -n "$RFM_SELECTED_FILES" ]; then
  while IFS= read -r filepath; do
    if [ -f "$filepath" ] && [ -r "$filepath" ]; then
      FILES_LIST+=("$filepath")
    else
      echo "警告：文件不存在或不可读，已跳过: $filepath" >&2
    fi
  done <<< "$RFM_SELECTED_FILES"
fi

# 从命令行参数获取文件列表
if [ $# -gt 4 ]; then
  shift 4
  for filepath in "$@"; do
    if [ -f "$filepath" ] && [ -r "$filepath" ]; then
      FILES_LIST+=("$filepath")
    else
      echo "警告：文件不存在或不可读，已跳过: $filepath" >&2
    fi
  done
fi

# 验证必须至少提供提示词或文件
if [ -z "$rfm_AI_MODEL_prompt" ] && [ ${#FILES_LIST[@]} -eq 0 ]; then
  echo "错误：必须提供提示词或至少一个文件" >&2
  exit 1
fi
