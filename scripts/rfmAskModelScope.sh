#!/bin/bash

source rfmAiGetParameterPromptAndFile.sh

#source rfmAiAppendFileContentIntoPrompt.sh

#source rfmAiJsonMessagesArray.sh

# Make the API call using curl
# Append the correct endpoint path to the base URL

# Build the JSON payload using the messages array format
# REQUEST_JSON="{\"input\": {\"text\": \"$rfm_AI_MODEL_prompt\"}, \"parameters\": {\"tokenizer\": \"xglm\", \"batch_size\": 2}}"

#rfm_AI_MODEL_HOST_URL='https://api-inference.modelscope.cn/api-inference/v1/models/iic/cv_diffusion_text-to-image-synthesis'
REQUEST_JSON="{\"model\": \"$rfm_AI_MODEL_NAME\",\"prompt\": \"$rfm_AI_MODEL_prompt\"}"
#rfm_AI_MODEL_API_KEY='3742b964-fc09-444d-b274-b0f96adcd2e9'


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
    echo $REQUEST_JSON > .rfmAI_latest_request.json
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


RESPONSE_ID=$(echo $RESPONSE | jq -r '.request_id')

source rfmAiDisplayResult.sh

source rfmAiSaveHistory_WithDirectory.sh

