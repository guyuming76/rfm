#!/bin/bash

#set -x

# Source 和 Destination 需要是不同的目录
# 或者 Source 是文件, 而Destination 是目录,如同mv 命令参数
# 从 Source 移动到 Destination
# 把原指向 Source 目录内 (包括Source本身) 的符号链接更新指向 Desitination 目录内
# Source 目录内的相对路径符号链移动前要使用绝对路径重建，移动后再使用相对路径重建

Source=$1
Destination=$2

if [[ ${Source:0:1} != "/" || ${Destination:0:1} != "/" ]]; then
	echo "absolute address is required for parameters" >&2
	exit 1;
elif [ -n "$G_MESSAGES_DEBUG" ]; then
	echo "----------------------------"
	echo "Source:" "$Source" "     Destination:" "$Destination"
fi

if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]]; then
        rm -rf "$Destination" || { echo "删除目标目录失败" >&2; exit 4; }
fi

# 获取查找范围,参见rfmFindLinksToTheSameFile_Chinese.h.sh
# rfmFindScope 会是绝对路径形式
# 获取源和目标的git仓库根目录
if [[ -d "$Source" ]];then
	sourceGitRoot=$(cd "$Source" && git rev-parse --show-toplevel 2>/dev/null)
else
	sourceGitRoot=$(cd $(dirname "$Source") && git rev-parse --show-toplevel 2>/dev/null)
fi
if [[ -d "$Destination" ]];then
	destGitRoot=$(cd "$Destination" && git rev-parse --show-toplevel 2>/dev/null)
else
	destGitRoot=$(cd $(dirname "$Destination") && git rev-parse --show-toplevel 2>/dev/null)
fi

if [[ -n "$sourceGitRoot" && -n "$destGitRoot" && "$sourceGitRoot" == "$destGitRoot" ]]; then
    rfmFindScope="$sourceGitRoot"
    echo "提示: 源和目标在同一git仓库，将在仓库内($rfmFindScope)和维度目录内查找指向源($Source)(目录内)文件的软链接，而不是全文件系统查找，以节省时间"
else
    rfmFindScope="/"
    echo "提示: 需要在整个文件系统(/)范围查找指向源($Source)(目录内)文件的软链接，可能耗时比较长"
fi

	# 下面 SpecificDir 实际指的是 Source
	# 因为 $rfmFindScope 是绝对路径形式, find 的输出也是绝对路径形式
#find命令在遇到目录权限不够的情况会导致返回值非0，不能因此退出脚本
if [ "$rfmFindScope" = "/" ]; then
  find "$rfmFindScope" \
    \( -path "/proc" -o -path "/sys" -o -path "/dev" -o -path "/run" -o -path "/var/run" -o -path "/var/lock" -o -path "/tmp" -o -path "/var/tmp" -o -path "/lost+found" \) -prune -o \
    -type l -exec rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh "$Destination" "$Source" {} \;
else
  find "$rfmFindScope" -type l -exec rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh "$Destination" "$Source" {} \;
  if [[ -n "$RFM_DIMENSION_PROPERTIES_FILE_NAME" ]]; then
	readarray rfmDimensionArray < <(locate "$RFM_DIMENSION_PROPERTIES_FILE_NAME")
	for rfmDimension in "${rfmDimensionArray[@]}";do
		rfmDimensionDir="$(dirname "$rfmDimension")"
		if [[ "$rfmDimensionDir" != "$rfmFindScope"* ]]; then
			echo "在维度 $rfmDimensionDir 下查找并处理指向 $Source 的符号链接"
			find "$rfmDimensionDir" -type l -exec rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh "$Destination" "$Source" {} \;
		fi
	done
  fi
fi

if [[ "$rfm_overwrite_destination" == "y" || "$rfm_overwrite_destination" == "Y" ]]; then
        mv -T -f "$Source" "$Destination" || { echo "强制移动失败" >&2; exit 5; }
else
        mv "$Source" "$Destination" || { echo "普通移动失败" >&2; exit 6; }
fi


if [ -n "$G_MESSAGES_DEBUG" ]; then
	echo "###########################"
fi

#find "$Destination" -type l | while read -r symlink; do symlink_target=$(readlink "$symlink"); if [ -n "$G_MESSAGES_DEBUG" ]; then echo "#####:" "$symlink"; echo "$symlink_target"; fi; ln -srfn "$symlink_target" "$symlink"; done
# Destination 下面所有符号链接，若是使用绝对路径（以 / 开头）， 使用相对路径重建
symbolic_link_errors=0
find "$Destination" -type l | while read -r symlink; do 
    if ! symlink_target=$(readlink "$symlink"); then
        echo "读取符号链接失败: $symlink" >&2
        ((symbolic_link_errors++))
        continue
    fi
    
    if [[ ${symlink_target:0:1} == "/" ]]; then 
        if ! ln -srfn "$symlink_target" "$symlink"; then
            echo "重建符号链接失败: $symlink" >&2
            ((symbolic_link_errors++))
        fi
    fi
done

# 检查错误计数并退出
if (( symbolic_link_errors > 0 )); then
    echo "发现 $symbolic_link_errors 个符号链接处理错误" >&2
    exit 8
fi
