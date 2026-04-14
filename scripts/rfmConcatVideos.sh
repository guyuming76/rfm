#!/bin/bash

# 检查是否提供了参数
if [ $# -eq 0 ]; then
    echo "用法: $0 视频文件1 视频文件2 ..."
    echo "示例: $0 /path/to/video1.mp4 /path/to/video2.mp4"
    exit 1
fi

# 输出接收到的文件名列表
echo "您输入的待合并视频文件列表:"
echo "=========================="
for i in "$@"; do
    echo "$i"
done
echo "=========================="
echo ""

# 提示用户输入输出文件名
read -p "请输入合并后的输出文件名 [默认: output.mp4]: " output_filename

# 设置默认值
if [ -z "$output_filename" ]; then
    output_filename="output.mp4"
fi

# 确保文件名有正确的扩展名（可选）
if [[ ! "$output_filename" =~ \.(mp4|avi|mkv|mov|flv|wmv)$ ]]; then
    output_filename="${output_filename}.mp4"
fi

echo "输出文件将保存为: $output_filename"
echo ""

# 创建临时文件
temp_file=$(mktemp /tmp/videolist_XXXXXX.txt)

echo "正在创建临时列表文件: $temp_file"

# 将文件列表写入临时文件
for video in "$@"; do
    # 确保路径中的单引号被正确处理（转义单引号）
    safe_video=$(echo "$video" | sed "s/'/'\\\\''/g")
    echo "file '$safe_video'" >> "$temp_file"
done

echo "临时文件内容:"
cat "$temp_file"
echo ""

# 执行ffmpeg合并视频
echo "正在合并视频，请稍候..."
ffmpeg -f concat -safe 0 -i "$temp_file" -c copy -fflags +genpts "$output_filename"

# 检查ffmpeg执行结果
if [ $? -eq 0 ]; then
    echo "视频合并成功! 输出文件: $output_filename"
else
    echo "视频合并失败!"
fi

# 清理临时文件
echo "清理临时文件..."
rm -f "$temp_file"

read -p "操作完成!按回车关闭当前命令窗口"
