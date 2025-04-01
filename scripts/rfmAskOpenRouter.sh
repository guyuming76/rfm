#!/bin/bash

#gentoo上依赖 app-misc/jq

# 检查模型参数
if [ -z "$1" ]; then
  echo "错误：未提供模型名称" >&2
  exit 1
fi
MODEL_NAME="$1"

# 读取用户输入
prompt=$(cat)
if [ -z "$prompt" ]; then
  echo "错误：未提供输入提示词" >&2
  exit 1
fi

# 发送请求并捕获原始 JSON 响应
response=$(curl -sS --fail-with-body "https://openrouter.ai/api/v1/chat/completions" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $OPENROUTER_API_KEY"
  -d "$(jq -n \
    --arg model "$MODEL_NAME" \
    --arg prompt "$prompt" \
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
