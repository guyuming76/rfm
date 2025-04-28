#!/bin/bash


echo "直接回车保存对话记录至当前目录,即在当前目录创建包含对话记录文件的子目录，这个子目录的命名格式是固定的"
echo "或输入创建子目录的目的路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

dirname="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [ ! -z "$dirname" ];then
    cd "$dirname"
fi

# 仅当成功时保存文件
if ! [[ "$RESULT" =~ ^Error: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$rfm_AI_MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  dirname="rfmAI_${sanitized_model}___${timestamp}"
  filename="rfmAI.md"  # 使用清理后的名称，开头的前缀便于在目录内排序，后缀便于满足 filenameSuffix 条件

  mkdir "$dirname"
  cd "$dirname"

  echo $RESPONSE > .rfmAI_latest_response.json
  echo $REQUEST_JSON > .rfmAI_latest_request.json

  cat <<EOF > "$filename"
# 用户提示词 #
$rfm_AI_MODEL_prompt

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

# 历史ID #
$(echo "$RESPONSE_ID_ARRAY")

# ID #
$(if [ -z "$RESPONSE_ID" ]; then
    echo "警告: 未提供RESPONSE_ID, 自动生成UUID" >&2
    uuidgen
  else
    echo "$RESPONSE_ID"
  fi)
EOF

  echo
  echo
  echo "回复已保存至: $filename" >&2
fi


# Extract the image URL
IMAGE_URL=$(echo "$RESPONSE" | jq -r '.images[0].url')

if [ ! -z "$IMAGE_URL" ]; then
# Download the image
   echo "Downloading image from: $IMAGE_URL"
   # You can use either wget or curl to download the image
   # wget -q "$IMAGE_URL" -O "image_$RESPONSE_ID.png"
   curl -sS -o "image_$RESPONSE_ID.png" "$IMAGE_URL"
   echo "Image downloaded as image_$RESPONSE_ID.png"
else
   echo "No image URL found in the response."
fi
