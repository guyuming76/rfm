#!/bin/bash

#gentoo上依赖:
	#app-misc/jq  生成和解析json
	#sys-apps/bat 语法高亮显示markdown

#set -x

rfm_AI_MODEL_HOST_URL="$1"
rfm_AI_MODEL_NAME="$2"
rfm_AI_MODEL_TEMPERATURE=$3
rfm_AI_MODEL_API_KEY="$4"


# Always read the user's prompt
PROMPT=$(cat)

# Handle file uploads if any
PROMPT_FILES_CONTENT=""
FILES_LIST=""
if [ -n "$RFM_SELECTED_FILES" ]; then
    # Process each selected file
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            PROMPT_FILES_CONTENT+="=== 以下是文件内容 ===\n\n=== 文件: $filename ===\n"
            PROMPT_FILES_CONTENT+=$(cat "$file")
            PROMPT_FILES_CONTENT+="\n\n"
            FILES_LIST+="$file\n"
        fi
    done <<< "$RFM_SELECTED_FILES"
    
    # Show which files will be uploaded
    echo "以下文件将被上传给AI模型:" >&2
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            echo "  - $file" >&2
        fi
    done <<< "$RFM_SELECTED_FILES"
    echo >&2
fi

# Combine files content with original prompt
if [ -n "$PROMPT_FILES_CONTENT" ]; then
    PROMPT_FULL="$PROMT$PROMPT_FILES_CONTENT"
fi

# Only exit if both prompt and file content are empty
if [ -z "$PROMPT" ] && [ -z "$PROMPT_FILES_CONTENT" ]; then
    echo "Error: No prompt or file content provided." >&2
    exit 1
fi

# Make the API call using curl
# Append the correct endpoint path to the base URL

# Build the JSON payload using the messages array format
JSON_PAYLOAD=$(jq -n \
  --arg model "$rfm_AI_MODEL_NAME" \
  --arg content "$PROMPT_FULL" \
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
elif .choices and (.choices | length > 0) and .choices[0].message and .choices[0].message.content then
  .choices[0].message.content
elif .choices and (.choices | length > 0) and .choices[0].text then
  .choices[0].text
else
  .message // ("Error: Unexpected API response format: " + (tostring))
end')


# Display the result
if which bat >/dev/null 2>&1; then
    echo "$RESULT" | bat -p --language=md
else
    echo "$RESULT"
    echo
    echo
    echo "安装 bat 软件(gentoo 上的包是sys-apps/bat)可以语法高亮显示当前markdown内容"
fi


# 仅当成功时保存文件
if ! [[ "$RESULT" =~ ^Error: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$rfm_AI_MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  filename="rfmAI_${sanitized_model}___${timestamp}.rfmAI.md"  # 使用清理后的名称，开头的前缀便于在目录内排序，后缀便于满足 filenameSuffix 条件

  cat <<EOF > "$filename"
# 用户提示词 #
$PROMPT

# 附加文件 #
$(echo -e "$FILES_LIST")

# 模型回复内容 #
$RESULT

# 请求URL #
$rfm_AI_MODEL_HOST_URL

# 模型名称 #
$rfm_AI_MODEL_NAME

# 温度 #
$rfm_AI_MODEL_TEMPERATURE
EOF


# 直接通过写回复json字符串遇到问题，改用下面方法追加
#echo '```' >> "$filename"
# 然后将 $RESPONSE 的内容追加到文件，避免 heredoc 解析问题
#echo "$RESPONSE" >> "$filename"
# 最后追加结束代码块的标记
#echo '```' >> "$filename"

  echo
  echo
  echo "回复已保存至: $filename" >&2
fi
