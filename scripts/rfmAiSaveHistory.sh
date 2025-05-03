#!/bin/bash

echo
echo
echo "回车保存对话记录至当前目录"
echo "或输入目的路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

dirname="$(rfmReadlineWithSpecificHistoryFile.sh ~/.rfm_history_directory)"

if [ ! -z "$dirname" ];then
    dirname=${dirname/#~/${HOME}}
    cd "$dirname"
fi

echo $RESPONSE > .rfmAI_latest_response.json

# 清理模型名称中的非法字符（如冒号）
sanitized_model=$(echo "$rfm_AI_MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
timestamp=$(date +%Y%m%d_%H%M%S)
filename="rfmAI_${sanitized_model}___${timestamp}.rfmAI.md"  # 使用清理后的名称，开头的前缀便于在目录内排序，后缀便于满足 filenameSuffix 条件
json_response_file_name="rfmAI_${sanitized_model}___${timestamp}.rfmAI.json"

# 仅当成功时保存文件
if ! [[ "$RESULT" =~ ^Error: ]]; then

    cat <<EOF > "$filename"
# 用户提示词 #
$rfm_AI_MODEL_prompt

# 附加文件 #
$(printf '%s\n' "${FILES_LIST[@]}")

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

# 反馈 #
EOF

  echo
  echo
  echo "回复已保存至: $filename" >&2
fi

echo

# displayJSON 变量值源于 rfmAiDisplayResult.sh 脚本
if [[ "$displayJSON" =~ ^[Yy]$ ]]; then
    read -p "是否还要单独保存原始回复json? [y/N] " saveJSON
    saveJSON=${saveJSON:-N}
    if [[ "$saveJSON" =~ ^[Yy]$ ]]; then
	echo $RESPONSE > $json_response_file_name
    fi
fi
