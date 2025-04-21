#!/bin/bash

#gentoo上依赖:
	#app-misc/jq  生成和解析json

#set -x

source rfmAiGetParameterPromptAndFile.sh

source rfmAiAppendFileContentIntoPrompt.sh

source rfmAiJsonMessagesArray.sh

# Make the API call using curl
# Append the correct endpoint path to the base URL

# Add current prompt
MESSAGES_ARRAY=$(echo "$MESSAGES_ARRAY" | jq \
  --arg content "$PROMPT_FULL" \
  '. += [{"role": "user", "content": $content}]')

# Build the JSON payload using the messages array format
REQUEST_JSON=$(jq -n \
  --arg model "$rfm_AI_MODEL_NAME" \
  --argjson messages "$MESSAGES_ARRAY" \
  --argjson temp "$rfm_AI_MODEL_TEMPERATURE" \
  '{
    model: $model,
    messages: $messages,
    stream: false,
    temperature: $temp
  }')

echo $REQUEST_JSON > .rfmAI_latest_request.json

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

RESPONSE_ID=$(echo $RESPONSE | jq '.id')

source rfmAiDisplayResult.sh

source rfmAiSaveHistory.sh
