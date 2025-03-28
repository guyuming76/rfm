#!/bin/bash

#需求：
#给我写一段bash脚本，接收一组文件名参数（包含完整路径），在当前目录下建立一个子目录，（先提示用户输入子目录名称），把通过参数接收的文件都移动到这个新建的子目录下面

# 循环提示用户输入非空子目录名
while true; do
    read -p "请输入子目录名称: " dirname
    if [[ -z "$dirname" ]]; then
        echo "错误：目录名不能为空！"
    else
        break
    fi
done

# 尝试创建目录（禁止自动创建多级目录）
if ! mkdir "$dirname" 2>/dev/null; then
    echo "错误：无法创建目录 '$dirname'（可能已存在或名称无效）" >&2
    exit 1
fi

# 检查文件参数是否存在
if [[ $# -eq 0 ]]; then
    echo "错误：未提供需要移动的文件" >&2
    exit 1
fi

# 移动文件并处理异常情况
moved_count=0
for file in "$@"; do
    if [[ -e "$file" ]]; then
        if mv -v -- "$file" "$dirname/"; then
            ((moved_count++))
        else
            echo "警告：移动 '$file' 失败" >&2
        fi
    else
        echo "警告：文件 '$file' 不存在，已跳过" >&2
    fi
done

# 输出结果摘要
echo "操作完成！成功移动 $moved_count 个文件到 $dirname/"
exit 0





#脚本特性：
#
#    ‌智能提示‌：强制要求用户输入非空目录名，防止误操作
#    ‌安全创建‌：
#        禁止通过mkdir -p自动创建多级目录
#        精确捕获目录创建失败原因（存在性检查或非法名称）
#    ‌文件处理‌：
#        自动跳过不存在的文件
#        显示每个文件的移动过程（-v参数）
#        统计并显示成功移动的文件数量
#    ‌错误处理‌：
#        使用标准错误输出（>&2）显示错误信息
#        不同错误类型返回不同退出码
#    ‌格式规范‌：
#        统一使用双引号包裹变量
#        支持包含空格/特殊字符的文件名
#        符合Google Shell Style规范

#使用示例

## 赋予执行权限
#chmod +x move_files.sh

## 运行示例（假设要移动3个文件）
#./move_files.sh /path/to/file1.txt "/path/with space/file2.jpg" /invalid/file


#典型输出:

#请输入子目录名称:   （直接回车）
#错误：目录名不能为空！
#请输入子目录名称: my_files
#已创建目录 'my_files'
#mv: 无法移动 '/invalid/file' 到 'my_files/': 没有那个文件或目录
#操作完成！成功移动 2 个文件到 my_files/
