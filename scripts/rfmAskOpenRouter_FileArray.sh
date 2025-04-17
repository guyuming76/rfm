#!/bin/bash

#gentoo上依赖:
	#app-misc/jq  生成和解析json

#set -x

source rfmAiGetParameterPromptAndFile.sh

#source rfmAiAppendFileContentIntoPrompt.sh

# 构建JSON请求体(使用attachments数组)
REQUEST_JSON=$(jq -n \
  --arg prompt "$rfm_AI_MODEL_prompt" \
  --argjson temp "$rfm_AI_MODEL_TEMPERATURE" \
  --arg model "$rfm_AI_MODEL_NAME" \
  '{
    model: $model,
    messages: [{
      role: "user",
      content: $prompt,
      attachments: []
    }],
    stream: false,
    temperature: $temp
  }')

# 添加文件内容到attachments数组
for filepath in "${FILES_LIST[@]}"; do
    filename=$(basename "$filepath")
    content=$(cat "$filepath" 2>/dev/null | jq -Rs .)  # 将内容转为JSON字符串
    if [ $? -ne 0 ]; then
        echo "错误：无法读取文件内容: $filepath" >&2
        exit 3
    fi
    REQUEST_JSON=$(echo "$REQUEST_JSON" | jq \
        --arg name "$filename" \
        --arg path "$filepath" \
        --argjson content "$content" \
        '.messages[0].attachments += [{
          name: $name,
          path: $path, 
          content: $content,
          mimeType: "text/plain"
        }]')
done

# The JSON payload includes the model name and the user's prompt
RESPONSE=$(curl -sS --fail-with-body -X POST "$rfm_AI_MODEL_HOST_URL" \
  -H "Authorization: Bearer $rfm_AI_MODEL_API_KEY" \
  -H "Content-Type: application/json" \
  -d "$REQUEST_JSON" 2>&1) # Capture stderr as well

# Check if curl failed (using --fail-with-body helps capture HTTP errors)
CURL_EXIT_CODE=$?
if [ $CURL_EXIT_CODE -ne 0 ]; then
    echo "Raw REQUEST_JSON for debug : $REQUEST_JSON" >&2
    echo "Raw RESPONSE for debug : $RESPONSE" >&2
    exit 4
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

source rfmAiDisplayResult.sh

source rfmAiSaveHistory.sh
