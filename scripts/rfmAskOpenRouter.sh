#!/bin/bash

#gentoo上依赖 app-misc/jq

rfm_AI_MODEL_HOST_URL="$1"
rfm_AI_MODEL_NAME="$2"
rfm_AI_MODEL_TEMPERATURE=$3
rfm_AI_MODEL_API_KEY="$4"

# 读取用户输入
rfm_AI_MODEL_prompt=$(cat)
if [ -z "$rfm_AI_MODEL_prompt" ]; then
  echo "错误：未提供输入提示词" >&2
  exit 1
fi

# 发送请求并捕获原始 JSON 响应
response=$(curl -sS --fail-with-body "$rfm_AI_MODEL_HOST_URL" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $rfm_AI_MODEL_API_KEY"
  -d "$(jq -n \
    --arg model "$rfm_AI_MODEL_NAME" \
    --arg prompt "$rfm_AI_MODEL_prompt" \
    '{
      model: $model,
      prompt: $prompt,
      stream: false,
      options: {temperature: 0.7}
    }')" 2>&1)  # 捕获标准错误和输出

# 解析响应
result=$(echo "$response" | jq -r '
  if .error then
    "错误: " + .error
  else
    .response
  end')

# 显示结果
echo "$result"

# 仅当成功时保存文件
if ! [[ "$result" =~ ^错误: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  filename="rfmAI_${sanitized_model}___${timestamp}.md"  # 使用清理后的名称

  cat <<EOF > "$filename"
## 用户提示词 ##
$prompt

## 模型回复 ##
$result
EOF

  echo "回复已保存至: $filename" >&2
fi
