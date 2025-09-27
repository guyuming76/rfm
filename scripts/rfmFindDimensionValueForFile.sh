#!/bin/bash

#find -L "$1" -exec test "$2"  -ef {} \; -print|xargs -r dirname|xargs -r realpath --no-symlinks --relative-to="$1"

# TODO:

# 目录 “/mineral/晶系/三方” 下面有一个符号链接 “海蓝宝” 指向目录 "/mineral/矿物名称/海蓝宝"； 目录 “/mineral/矿物名称/海蓝宝” 下面有文件 pic1.jpg. 如下命令：
# find -L "/mineral/晶系" -exec test "/mineral/矿物名称/海蓝宝/pic1.jpg"  -ef {} \; -print|xargs -r dirname|xargs -r realpath --no-symlinks --relative-to="/mineral/晶系"
# 输出：三方/海蓝宝
# 如果希望输出： 三方
# 也就是说从查找开始的 “/mineral/晶系” 到 “/mineral/晶系/三方/海蓝宝/pic1.jpg” 的相对路径 “三方/海蓝宝/pic1.jpg” 上的非符号链接目录。
# 如果相对路径结构是 “三方/某某/海蓝宝/pic1.jpg”，而“某某”是目录而非符号链接，则返回 “三方/某某”。也就是说，我希望返回的是相对路径上截至最后一个非符号链接目录左边的字符串，包括这最后一个非符号链接目录。
# 代码要如何改？



# 根据您的需求，您需要找到从起始目录到目标文件路径上的最后一个非符号链接目录。以下是修改后的代码：

# ```bash
# find -L "/mineral/晶系" -exec test "/mineral/矿物名称/海蓝宝/pic1.jpg" -ef {} \; -print | \
# xargs -r dirname | \
# xargs -r -I {} sh -c '
#     file_path="{}"
#     start_dir="/mineral/晶系"
#     relative_path=$(realpath --relative-to="$start_dir" "$file_path")
    
#     # 逐级检查路径组件
#     current_path="$start_dir"
#     result_path=""
#     IFS="/" read -ra parts <<< "$relative_path"
    
#     for part in "${parts[@]}"; do
#         if [ -n "$part" ]; then
#             current_path="$current_path/$part"
#             # 检查当前路径是否是符号链接
#             if [ -L "$current_path" ]; then
#                 break
#             else
#                 result_path="${result_path:+$result_path/}$part"
#             fi
#         fi
#     done
    
#     echo "$result_path"
# '
# ```

# 这个脚本的工作原理：

# 1. 首先使用 `find` 找到目标文件
# 2. 获取文件所在目录路径
# 3. 计算相对于起始目录的路径
# 4. 逐级检查路径中的每个目录组件：
#    - 如果不是符号链接，就添加到结果路径中
#    - 如果遇到符号链接目录，就停止并输出当前结果

# 对于您的示例：
# - 输入路径：`/mineral/晶系/三方/海蓝宝/pic1.jpg`
# - 检查过程：
#   - `/mineral/晶系/三方` - 不是符号链接，添加到结果
#   - `/mineral/晶系/三方/海蓝宝` - 是符号链接，停止检查
# - 输出：`三方`

# 对于另一个示例 `三方/某某/海蓝宝/pic1.jpg`：
# - 检查过程：
#   - `三方` - 不是符号链接，添加到结果
#   - `某某` - 不是符号链接，添加到结果  
#   - `海蓝宝` - 是符号链接，停止检查
# - 输出：`三方/某某`

# 如果您想要更简洁的一行版本：

# ```bash
# find -L "/mineral/晶系" -exec test "/mineral/矿物名称/海蓝宝/pic1.jpg" -ef {} \; -print | \
# xargs -r dirname | \
# xargs -r -I {} sh -c 'p="{}"; s="/mineral/晶系"; r=$(realpath --relative-to="$s" "$p"); IFS="/" read -ra a <<< "$r"; path="$s"; result=""; for i in "${a[@]}"; do [ -n "$i" ] && path="$path/$i" && [ ! -L "$path" ] && result="${result:+$result/}$i" || break; done; echo "$result"'
# ```

# 这样就能正确返回从起始目录到目标文件路径上的最后一个非符号链接目录的相对路径。









# 看到错误信息是 `realpath: '': 没有那个文件或目录`，这意味着传递给 `realpath` 的文件路径是空的。问题出现在脚本中的变量替换。

# 主要问题有两个：
# 1. 在 `sh -c` 命令中，`$1` 变量没有被正确传递
# 2. 需要处理空路径的情况

# 这里是修正后的脚本：

# ```bash
# #!/bin/bash

# start_dir="$1"
# target_file="$2"

# # 导出变量以便在子shell中使用
# export start_dir

# find -L "$start_dir" -exec test "$target_file" -ef {} \; -print | \
# xargs -r dirname | \
# xargs -r -I {} sh -c '
#  file_path="{}"
#  if [ -z "$file_path" ]; then
#      exit 0
#  fi
#  relative_path=$(realpath --relative-to="$start_dir" "$file_path" 2>/dev/null)
 
#  if [ -z "$relative_path" ]; then
#      exit 0
#  fi
 
#  # 逐级检查路径组件
#  current_path="$start_dir"
#  result_path=""
#  IFS="/" read -ra parts <<< "$relative_path"
 
#  for part in "${parts[@]}"; do
#      if [ -n "$part" ]; then
#          current_path="$current_path/$part"
#          # 检查当前路径是否是符号链接
#          if [ -L "$current_path" ]; then
#              break
#          else
#              result_path="${result_path:+$result_path/}$part"
#          fi
#      fi
#  done
 
#  if [ -n "$result_path" ]; then
#      echo "$result_path"
#  fi
# '
# ```

# 主要修改：

# 1. **导出变量**：使用 `export start_dir` 确保子shell能访问到正确的变量值
# 2. **空值检查**：在关键位置添加空值检查，避免传递空路径给 `realpath`
# 3. **错误重定向**：`realpath` 命令添加 `2>/dev/null` 来抑制错误输出
# 4. **提前退出**：如果路径为空或处理失败，提前退出避免后续错误

# 如果还有问题，可以尝试这个更健壮的版本，添加更多调试信息：

# ```bash
#!/bin/bash

start_dir="$1"
target_file="$2"

echo "Debug: start_dir = $start_dir" >&2
echo "Debug: target_file = $target_file" >&2

export start_dir

find -L "$start_dir" -exec test "$target_file" -ef {} \; -print | \
xargs -r dirname | \
xargs -r -I {} sh -c '
 echo "Debug: Processing path: {}" >&2
 file_path="{}"
 if [ -z "$file_path" ]; then
     echo "Debug: Empty file path, skipping" >&2
     exit 0
 fi
 if [ ! -e "$file_path" ]; then
     echo "Debug: Path does not exist: $file_path" >&2
     exit 0
 fi
 relative_path=$(realpath --no-symlinks --relative-to="$start_dir" "$file_path" 2>/dev/null)
 
 if [ -z "$relative_path" ]; then
     echo "Debug: realpath failed for: $file_path" >&2
     exit 0
 fi
 
 echo "Debug: relative_path = $relative_path" >&2
 
 # 逐级检查路径组件
 current_path="$start_dir"
 result_path=""
 IFS="/" read -ra parts <<< "$relative_path"
 
 for part in "${parts[@]}"; do
     if [ -n "$part" ]; then
         current_path="$current_path/$part"
         echo "Debug: Checking $current_path" >&2
         # 检查当前路径是否是符号链接
         if [ -L "$current_path" ]; then
             echo "Debug: Found symlink at $current_path" >&2
             break
         else
             result_path="${result_path:+$result_path/}$part"
             echo "Debug: Added to result: $part" >&2
         fi
     fi
 done
 
 if [ -n "$result_path" ]; then
     echo "Debug: Final result: $result_path" >&2
     echo "$result_path"
 else
     echo "Debug: Empty result" >&2
 fi
'
# ```

# 调试版本会在标准错误输出中显示详细的处理过程，帮助你定位问题所在。
# ~
