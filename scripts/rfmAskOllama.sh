#!/bin/bash

#给baidu deepseek-r1 的需求： bash 脚本如何和 LLM 大模型交互， 给个简单脚本例子，读取用户输入的prompt, 发送给ollama 本地运行大模型，显示返回的内容
#deepseek第一次给的回答用grep,sed,awk处理json,比较难读，但提到jq工具
#jq这样的JSON处理工具， 简单介绍下，使用这个工具改写下前面的例子怎么样？

#gentoo上依赖 app-misc/jq

OLLAMA_HOST="http://localhost:11434"

# 检查模型名称是否为空
if [ -z "$1" ]; then
  echo "错误：未提供模型名称" >&2
  exit 1
fi
MODEL_NAME="$1"

# 从标准输入读取所有内容作为提示词
prompt=$(cat)

# 检查输入是否为空
if [ -z "$prompt" ]; then
  echo "错误：未提供输入提示词" >&2
  exit 1
fi

# 发送请求并处理响应
curl -sS --fail-with-body "$OLLAMA_HOST/api/generate" \
  -H "Content-Type: application/json" \
  -d "$(jq -n \
    --arg model "$MODEL_NAME" \
    --arg prompt "$prompt" \
    '{
      model: $model,
      prompt: $prompt,
      stream: false,
      options: {
        temperature: 0.7,
        num_ctx: 4096
      }
    }')" \
  | jq -r '
    if .error then 
      "错误: " + .error 
    else 
      .response 
    end'





# 支持多种输入方式：
# # 管道输入
# echo "解释量子计算" | ./ask_ollama.sh

# # 文件重定向
# ./ask_ollama.sh < prompt.txt

# # 多行输入
# ./ask_ollama.sh <<EOF
# 请用markdown格式编写一个bash脚本教程，
# 要求包含以下内容：
# 1. 变量声明
# 2. 函数定义
# 3. 错误处理
# EOF


# 特殊字符处理‌
# jq 自动处理以下内容：
# # 包含双引号
# echo '打印"Hello World"' | ./ask_ollama.sh

# # 包含反斜线
# echo "正则表达式示例: ^\d+$" | ./ask_ollama.sh




# 典型使用场景：

# 1.批量处理文件‌
# # 处理整个代码库
# find src/ -type f -exec cat {} \; | ./ask_ollama.sh

# # 带上下文的多文件分析
# (cat README.md; echo "====="; cat main.py) | ./ask_ollama.sh

# 2.集成到CI/CD‌
# # 检查代码规范
# git diff HEAD~1 | ./ask_ollama.sh


# 性能优化技巧：

# 1.流式响应模式‌
# 修改 stream: true 并实时显示结果：
# ...
# -d "$(jq -n \
#   --arg model "$MODEL_NAME" \
#   --arg prompt "$prompt" \
#   '{
#     model: $model,
#     prompt: $prompt,
#     stream: true  # 启用流式
#   }')" \
# | jq -r --unbuffered '.response? // .error'

# 2.输入缓存机制‌
# # 缓存最近输入的提示词
# prompt=$(cat | tee /tmp/last_prompt.txt)
