#!/bin/bash

#gentoo上依赖:
	#app-misc/jq  生成和解析json

#set -x

source rfmAiGetParameterPromptAndFile.sh

source rfmAiAppendFileContentIntoPrompt.sh


# Make the API call using curl
# Append the correct endpoint path to the base URL

# Build the JSON payload using the messages array format
REQUEST_JSON=$(jq -n \
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
  -d "$REQUEST_JSON" 2>&1) # Capture stderr as well

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

source rfmAiDisplayResult.sh

source rfmAiSaveHistory.sh
