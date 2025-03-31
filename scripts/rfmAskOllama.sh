#!/bin/bash

#给baidu deepseek-r1 的需求： bash 脚本如何和 LLM 大模型交互， 给个简单脚本例子，读取用户输入的prompt, 发送给ollama 本地运行大模型，显示返回的内容
#deepseek第一次给的回答用grep,sed,awk处理json,比较难读，但提到jq工具
#jq这样的JSON处理工具， 简单介绍下，使用这个工具改写下前面的例子怎么样？

#gentoo上依赖 app-misc/jq



OLLAMA_HOST="http://localhost:11434"

# 检查模型参数
if [ -z "$1" ]; then
  echo "错误：未提供模型名称" >&2
  exit 1
fi
MODEL_NAME="$1"

# 读取用户输入
prompt=$(cat)
if [ -z "$prompt" ]; then
  echo "错误：未提供输入提示词" >&2
  exit 1
fi

# 发送请求并捕获原始 JSON 响应
response=$(curl -sS --fail-with-body "$OLLAMA_HOST/api/generate" \
  -H "Content-Type: application/json" \
  -d "$(jq -n \
    --arg model "$MODEL_NAME" \
    --arg prompt "$prompt" \
    '{
      model: $model,
      prompt: $prompt,
      stream: false,
      options: {temperature: 0.7}
    }')" 2>&1)  # 捕获标准错误和输出

# 解析响应
result=$(echo "$response" | jq -r '
  if .error then
    "错误: " + .error
  else
    .response
  end')

# 显示结果 渲染markdown
# 依赖 gentoo 依赖 dev-python/rich 或 pip install rich
# echo "from rich.markdown\n    import Markdown print(Markdown(r'''$result'''))" | python
echo "$result"

# 仅当成功时保存文件
if ! [[ "$result" =~ ^错误: ]]; then
  # 清理模型名称中的非法字符（如冒号）
  sanitized_model=$(echo "$MODEL_NAME" | sed 's/[:/\\*?<>|"]/_/g')  # 关键修改点
  timestamp=$(date +%Y%m%d_%H%M%S)
  filename="rfmAI_${sanitized_model}___${timestamp}.md"  # 使用清理后的名称

  cat <<EOF > "$filename"
## 用户提示词 ##
$prompt

## 模型回复 ##
$result
EOF

  echo "回复已保存至: $filename" >&2
fi



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



# 关键优化点说明‌

#     ‌不可见分隔符提升安全性‌


#     read -r -d '' has_error result < <(
#       jq -r '..."\u0000..."'  # 使用 Unicode 空字符分隔
#     )

#         用 \u0000（ASCII 空字符）分隔状态码和内容，‌彻底避免内容本身包含分隔符‌的风险
#         -d '' 设置 read 以空字符为分隔符

#     ‌直接传递错误状态码‌


#     if [ "$has_error" -eq 0 ]; then  # 0 表示成功

#         has_error 明确为 0（成功）或 1（错误），逻辑更清晰
#         完全消除对内容前缀的依赖，即使回复包含 错误: 也不影响判断

#     ‌保留完整错误信息‌

    
#     echo "$result"  # 直接输出原始错误信息或回复

#         终端显示逻辑不变，用户感知无差异
#         保存文件前通过状态码过滤，确保只有成功响应才保存

# ‌优化后逻辑流程图‌


# 用户输入 --> 脚本
#   ↓
# 构造 JSON 请求 --> 调用 Ollama API
#   ↓
# 解析响应 --jq提取--> 状态码 (0/1) + 内容
#   ↓
# 显示内容 --> 终端
#   ↓
# 状态码=0? --是--> 保存 Markdown 文件
#           --否--> 跳过保存

# 此方案彻底分离 ‌状态判断‌ 和 ‌内容处理‌，是更健壮的实现方式。
