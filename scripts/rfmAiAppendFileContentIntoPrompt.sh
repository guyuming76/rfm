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
    echo "上述选中文件将被上传到Ai模型,5秒内按 ctrl+c 取消"
    sleep 5
    echo "开始上传文件并请求大模型, 按ctrl+c终止执行"
#用命令 foot script.sh 打开一个新的终端模拟器foot, 并在启动运行脚本 script.sh. 脚本中保护命令 read -p "blablabla" confirm. 执行的时候 "blablabla" 并未显示，系统也没停留在这里等待输入confirm的值，就好像这一行read命令根本就未执行，为啥？
#Ai给的建议是 foot --hold 启动脚本，我这里选择提示用户终止
#   read -p "确认上传这些文件吗? [Y/n] " confirm   
#   confirm=${confirm:-Y}
#   if [[ "$confirm" =~ ^[Yy]$ ]]; then
#
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
#    fi
fi
