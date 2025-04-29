#!/bin/bash

# Initialize PROMPT_FULL with the base prompt
PROMPT_FULL="$rfm_AI_MODEL_prompt"

# Function to check if file is text
is_text_file() {
    file -b --mime-type "$1" | grep -q '^text/'
}


if [ -n "$FILES_LIST" ]; then
    for filepath in "${FILES_LIST[@]}"; do
	echo "$filepath"
    done
    echo
    # 用户确认
    read -p "确认上传这些文件吗? [Y/n] " confirm
    confirm=${confirm:-Y}
    if [[ "$confirm" =~ ^[Yy]$ ]]; then

        # Process each file in FILES_LIST
	for filepath in "${FILES_LIST[@]}"; do
	    if ! is_text_file "$filepath"; then
		echo "错误：文件不是文本格式: $filepath" >&2
		exit 2
	    fi

	    # Add file separator and content
	    filename=$(basename "$filepath")
	    PROMPT_FULL+="\n\n=== 文件: $filename ===\n"
	    file_content=$(cat "$filepath" 2>/dev/null)
	    if [ $? -ne 0 ]; then
		echo "错误：无法读取文件内容: $filepath" >&2
		exit 3
	    fi
	    PROMPT_FULL+="$file_content"
	done

	PROMPT_FULL=$(echo "$PROMPT_FULL" | jq -Rs .)
    fi
fi
