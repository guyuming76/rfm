#!/bin/bash

# 用户提示词 #
# 接收参数 $RFM_DIMENSION_PROPERTIES_FILE_NAME
# 当环境变量 $RFM_GIT_ROOT 不等于"/" 时:
#    返回 $RFM_GIT_ROOT 下所有的 $RFM_DIMENSION_PROPERTIES_FILE_NAME 文件完整路径
#    从 $RFM_GIT_ROOT 目录向上层目录找, 在每一层找当前层的第一层子目录的 $RFM_DIMENSION_PROPERTIES_FILE_NAME 文件, 这里第一层子目录可以是指向别的目录的符号链接. 比如 /dir1/dir2/dir3/rfm_git_root; dir2 中第一层子目录 /dir1/dir2/subdir2 下面也包含$RFM_DIMENSION_PROPERTIES_FILE_NAME 文件,则返回其完整路径. 又如 /dir1/subdir1 是个符号链接,指向一个目录,这个目录下面包含$RFM_DIMENSION_PROPERTIES_FILE_NAME 文件,则返回 /dir1/subdir1/$RFM_DIMENSION_PROPERTIES_FILE_NAME. 注意,查找文件必须位于subdir1或subdir2这样的目录或目录符号链接内,而不包括subdir1或subdir2下的子目录内

# 使用上面逻辑查找 rfm维度属性 文件有下列好处:
# 之前直接 locate rfm维度属性 有两个问题:
# 	1.在所有目录都会找到相同的维度定义,比如我在 ~/rfm 仓库内补全时也会出现 ~/mineral 仓库内的维度定义
#       2.locate命令不支持对维度定义目录的符号链接,比如,我在~/mineral仓库内为 产地 目录建立一个符号链接 产地1, locate 命令就无法返回产地1. 
# 全盘用 find 还是会有上面问题1,并且速度慢,现在的算法(或者说是维度作用域)避免了上面问题1,同时提高了速度

# 检查是否提供了文件名参数
if [ $# -ne 1 ]; then
    echo "用法: $0 <RFM_DIMENSION_PROPERTIES_FILE_NAME>"
    exit 1
fi

RFM_DIMENSION_PROPERTIES_FILE_NAME="$1"

# 检查 RFM_GIT_ROOT 环境变量
if [ -z "$RFM_GIT_ROOT" ]; then
    echo "错误: 环境变量 RFM_GIT_ROOT 未设置"
    exit 1
fi

if [ "$RFM_GIT_ROOT" = "/" ]; then
    echo "RFM_GIT_ROOT 是根目录，不执行查找"
    exit 0
fi

# 检查 RFM_GIT_ROOT 是否存在
if [ ! -d "$RFM_GIT_ROOT" ]; then
    echo "错误: RFM_GIT_ROOT 目录不存在: $RFM_GIT_ROOT"
    exit 1
fi

# 函数：在指定目录的第一层子目录中查找文件
find_files_in_first_level_dirs() {
    local search_dir="$1"
    local target_file="$2"
    
    # 遍历第一层子目录（包括符号链接）
    for subdir in "$search_dir"/*; do
        # 检查是否是目录或符号链接（指向目录）
        if [ -d "$subdir" ] || [ -L "$subdir" ] && [ -d "$(readlink -f "$subdir")" ]; then
            local target_path="$subdir/$target_file"
            if [ -f "$target_path" ]; then
                echo "$target_path"
            fi
        fi
    done
}

# 函数：在指定目录内递归查找文件（包括软链接指向的目录）
find_files_recursively() {
 local search_dir="$1"
 local target_file="$2"
 
 # 使用 find 命令递归查找，包括跟随符号链接
 find -L "$search_dir" -name "$target_file" -type f 2>/dev/null
}

# 首先在 RFM_GIT_ROOT 目录内递归查找（包括软链接指向的目录）
echo "在 $RFM_GIT_ROOT 目录内递归查找..." >&2
find_files_recursively "$RFM_GIT_ROOT" "$RFM_DIMENSION_PROPERTIES_FILE_NAME"

# 从 RFM_GIT_ROOT 的父目录开始向上遍历目录
current_dir="$(dirname "$RFM_GIT_ROOT")"

while [ "$current_dir" != "/" ]; do
    echo "在目录 $current_dir 中查找..." >&2
    
    # 在当前目录的第一层子目录中查找文件
    find_files_in_first_level_dirs "$current_dir" "$RFM_DIMENSION_PROPERTIES_FILE_NAME"
    
    # 向上移动到父目录
    current_dir="$(dirname "$current_dir")"
done

# 处理根目录的情况（如果需要的话）
if [ "$RFM_GIT_ROOT" != "/" ]; then
    echo "在根目录 / 中查找..." >&2
    find_files_in_first_level_dirs "/" "$RFM_DIMENSION_PROPERTIES_FILE_NAME"
fi
