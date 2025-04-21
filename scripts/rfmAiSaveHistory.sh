#!/bin/bash

echo $RESPONSE > .rfmAI_latest_response.json

# 仅当成功时保存文件
if ! [[ "$RESULT" =~ ^Error: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$rfm_AI_MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  filename="rfmAI_${sanitized_model}___${timestamp}.rfmAI.md"  # 使用清理后的名称，开头的前缀便于在目录内排序，后缀便于满足 filenameSuffix 条件

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
