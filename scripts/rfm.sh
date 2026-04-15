#!/bin/bash

function find_ppid_for_pid_and_update_current_pid(){
	pid_found_in_ppids=0
	for item_in_pid_ppid_row_array in "${pid_ppid_row_array[@]}";do
		readarray -d ' ' pid_ppid_column_array < <(echo $item_in_pid_ppid_row_array)
		# 空格为分割符读取列数组
		if [[ $((${pid_ppid_column_array[0]})) == $(($current_pid)) ]];then
		#如果数组中的第一项等于current_pid
		#这里用　(()) 把进程号统一转换成数字再比较，否在不知为啥结果老不对
			current_pid=${pid_ppid_column_array[1]}
			#current_pid设成数组第二列，即ppid列
			pid_found_in_ppids=1
			break
		fi
	done
}

# TODO: 这个函数写死了一堆终端模拟器名称，总是不好，是否可以换成沿着当前进程父子链往上找，直到找到第一个tty为空的进程，我在htop 里观察，终端模拟器下的bash进程一般tty是pts/x,而到了终端模拟器进程本身，tty就为空了
get_terminal_emulator() {
    local pid=$$
    while [ "$pid" != "1" ]; do
        cmd=$(ps -o comm= -p "$pid" 2>/dev/null | tr -d ' ')
        case "$cmd" in
            gnome-terminal|kgx|konsole|qterminal|xterm|urxvt|alacritty|kitty|wezterm|terminator|mate-terminal|xfce4-terminal|foot|st|deepin-terminal)
                echo "$cmd"
                return 0
                ;;
        esac
        pid=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
        [ -z "$pid" ] && break
    done
    echo "unknown"
}


# 根据脚本所在目录设置 LD_LIBRARY_PATH
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

if [ "$SCRIPT_DIR" = "/usr/bin" ]; then
    # 脚本位于 /usr/bin，添加 /usr/lib64 到 LD_LIBRARY_PATH
    export LD_LIBRARY_PATH="/usr/lib64:$LD_LIBRARY_PATH"
    echo "设置 LD_LIBRARY_PATH 包含 /usr/lib64" >&2
elif [ "$SCRIPT_DIR" = "/usr/local/bin" ]; then
    # 脚本位于 /usr/local/bin，添加 /usr/local/lib64 到 LD_LIBRARY_PATH
    export LD_LIBRARY_PATH="/usr/local/lib64:$LD_LIBRARY_PATH"
    echo "设置 LD_LIBRARY_PATH 包含 /usr/local/lib64" >&2
fi

#应用奔溃时生成coredump
ulimit -c unlimited

#获取行数组
readarray pid_ppid_row_array < <(ps -o pid -o ppid --no-header)

####current_pid=$$
####pid_found_in_ppids=1
#因为$$总是可以被找到的，所以直接初始化为１，下面while;do相当于do while,bash里貌似没有do while

if [[ -z "$RFM_TERM" ]];then
	#从当前进程$$,开始，根据pid对应ppid关系，循环查找，直到找不到，最后一个pid就是terminal emulator对应pid
	####while [ $pid_found_in_ppids == 1 ];do
	####	find_ppid_for_pid_and_update_current_pid
	####done

	####export RFM_TERM=$(basename $(ps -p $current_pid -o cmd --no-header))
        # 在OpenKylin 上面，使用find_ppid_for_pid_and_update_current_pid 方法不行，所以换成下面使用get_terminal_emulator.
        export RFM_TERM=$(get_terminal_emulator)
	echo "env RFM_TERM set to current terminal emulator:" $RFM_TERM  >&2

	#如果环境变量 RFM_TERM_E 不存在，未定义，则 export RFM_TERM_E=“”
	if [ -z "${RFM_TERM_E+x}" ]; then
    		export RFM_TERM_E=""
	fi
	# 判断 RFM_TERM 是否等于 "alacritty" TODO: 用户如果还用其他终端模拟器使用 -e 参数或其他类似作用参数，可以这里添加
	if [ "$RFM_TERM" = "alacritty" ]; then
    		# 如果是，则设置 RFM_TERM_E 的值为 "-e"
    		RFM_TERM_E="-e"
	elif [ "$RFM_TERM" = "mate-terminal" ]; then
		RFM_TERM_E="-x"
	fi

else
	echo "env RFM_TERM is: " $RFM_TERM  >&2
fi

export rfmFontCSS="* { font-size: 16px; }"

#使用 GTK Inspector
#export GTK_DEBUG=interactive

# 检测是否在 XWayland 中运行.
# 检查是否设置了 DISPLAY 环境变量（X11 的标志）
if [[ -n "$DISPLAY" ]]; then
    # 如果在 X11 环境中，强制使用 X11 后端，否则rfm窗口会显示在wayland窗口如dwl下面，而不是xwayland root窗口里面
    echo "检测到设置了 DISPLAY 环境变量，强制 rfm 使用 GDK_BACKEND=x11" >&2
    export GDK_BACKEND=x11
fi

# 检查 imv 命令是否存在，如果不存在但 /usr/libexec/imv/imv 存在，则创建 alias
if ! which imv > /dev/null 2>&1; then
    if [ -x "/usr/libexec/imv/imv" ]; then
        echo "在子进程创建 imv alias 指向 /usr/libexec/imv/imv" >&2
        export RFM_ALIAS=$(mktemp --suffix _RFM_ALIAS.sh)
        echo "alias imv='/usr/libexec/imv/imv'" >> $RFM_ALIAS
    fi
fi

# 检查 bat 命令是否存在，如果不存在但 batcat 存在，则创建 alias
if ! which bat > /dev/null 2>&1; then
    if which batcat > /dev/null 2>&1; then
        echo "在子进程创建 bat alias 指向 batcat" >&2
        if [ -z "${RFM_ALIAS+x}" ]; then
            export RFM_ALIAS=$(mktemp --suffix _RFM_ALIAS.sh)
        fi
        echo "alias bat='batcat'" >> $RFM_ALIAS
    fi
fi

# 打印收到的参数（调试信息）
echo "rfm.sh 收到的参数: $@" >&2

# 处理 file:// 开头的参数,在deepin23上社区版上，通过桌面快捷方式鼠标右键选取rfm（rfm.desktop）打开目录，就会获得这样的参数
processed_args=()
for arg in "$@"; do
    if [[ "$arg" == file://* ]]; then
        # 移除 file:// 前缀
        path="${arg#file://}"
        # 添加 -d 选项
        processed_args+=("-d" "$path")
        echo "转换 file:// 参数为: -d $path" >&2
    else
        processed_args+=("$arg")
    fi
done

if [ -n "$VIRTUAL_ENV" ]; then
    # 提取虚拟环境的名称，通常是路径的最后一部分
    venv_name=$(basename "$VIRTUAL_ENV")
    echo "The current python virtual environment is named '$venv_name'." >&2
    # valgrind --tool=memcheck rfm

    # MALLOC_CHECK_=1 RUST_LOG=rfm=warn rfm "${processed_args[@]}" --InheritedPrompt "($venv_name) "
    MALLOC_CHECK_=1 RUST_LOG=reedline_wrapper=warn rfm "${processed_args[@]}" --InheritedPrompt "($venv_name) "
else
    MALLOC_CHECK_=1 RUST_LOG=reedline_wrapper=warn rfm "${processed_args[@]}"
fi
