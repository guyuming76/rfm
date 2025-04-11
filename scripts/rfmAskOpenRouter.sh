#!/bin/bash

#gentoo上依赖 app-misc/jq

set -x

rfm_AI_MODEL_HOST_URL="$1"
rfm_AI_MODEL_NAME="$2"
rfm_AI_MODEL_TEMPERATURE=$3
rfm_AI_MODEL_API_KEY="$4"


# Read the user's prompt from standard input
PROMPT=$(cat)
if [ -z "$PROMPT" ]; then
  echo "Error: No prompt provided." >&2
  exit 1
fi

# Make the API call using curl
# Append the correct endpoint path to the base URL

# Build the JSON payload using the messages array format
JSON_PAYLOAD=$(jq -n \
  --arg model "$rfm_AI_MODEL_NAME" \
  --arg content "$PROMPT" \
  --argjson temp "$rfm_AI_MODEL_TEMPERATURE" \
  '{
    model: $model,
    messages: [ { role: "user", content: $content } ],
    stream: false,
    temperature: $temp
  }')

# The JSON payload includes the model name and the user's prompt
RESPONSE=$(curl -sS --fail-with-body -X POST "$rfm_AI_MODEL_HOST_URL" \
  -H "Authorization: Bearer $rfm_AI_MODEL_API_KEY" \
  -H "Content-Type: application/json" \
  -d "$JSON_PAYLOAD" 2>&1) # Capture stderr as well

# Check if curl failed (using --fail-with-body helps capture HTTP errors)
CURL_EXIT_CODE=$?
if [ $CURL_EXIT_CODE -ne 0 ]; then
    # Attempt to parse error from response body if curl failed
    ERROR_MSG=$(echo "$RESPONSE" | jq -r '.error.message // .detail // "Unknown curl error or non-JSON response"')
    echo "Error: curl command failed (Exit Code: $CURL_EXIT_CODE). $ERROR_MSG" >&2
    # Optionally print the raw response for debugging
    # echo "Raw Response: $RESPONSE" >&2
    exit 1
fi


# Parse the response using jq
# Extract the response content or the error message
# Handles standard OpenAI format and potential error structures
RESULT=$(echo "$RESPONSE" | jq -r $'
if .error then
  "Error: " + (.error.message // .error | tostring)
elif .choices and (.choices | length > 0) and .choices.message and .choices.message.content then
  .choices.message.content
elif .choices and (.choices | length > 0) and .choices.text then
  .choices.text
else
  .message // ("Error: Unexpected API response format: " + (tostring))
end')


# Display the result
echo "$RESULT"





# 仅当成功时保存文件
if ! [[ "$RESULT" =~ ^Error: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$rfm_AI_MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  filename="rfmAI_${sanitized_model}___${timestamp}.md"  # 使用清理后的名称

  cat <<EOF > "$filename"
## 用户提示词 ##
$PROMPT

## 模型回复 ##
$RESULT
EOF

  echo "回复已保存至: $filename" >&2
fi
