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

#应用奔溃时生成coredump
ulimit -c unlimited

#获取行数组
readarray pid_ppid_row_array < <(ps -o pid -o ppid --no-header)

current_pid=$$
pid_found_in_ppids=1
#因为$$总是可以被找到的，所以直接初始化为１，下面while;do相当于do while,bash里貌似没有do while

if [[ -z "$RFM_TERM" ]];then
	#从当前进程$$,开始，根据pid对应ppid关系，循环查找，直到找不到，最后一个pid就是terminal emulator对应pid
	while [ $pid_found_in_ppids == 1 ];do
		find_ppid_for_pid_and_update_current_pid
	done

	export RFM_TERM=$(basename $(ps -p $current_pid -o cmd --no-header))
        export RFM_TERM_E=""
	echo "env RFM_TERM set to current terminal emulator:" $RFM_TERM  >&2
	# 判断 RFM_TERM 是否等于 "alacritty" TODO: 用户如果还用其他终端模拟器使用 -e 参数或其他类似作用参数，可以这里添加
	if [ "$RFM_TERM" = "alacritty" ]; then
    		# 如果是，则设置 RFM_TERM_E 的值为 "-e"
    		RFM_TERM_E="-e"
	fi

else
	echo "env RFM_TERM is: " $RFM_TERM  >&2
fi

export rfmFontCSS="* { font-size: 16px; }"

if [ -n "$VIRTUAL_ENV" ]; then
    # 提取虚拟环境的名称，通常是路径的最后一部分
    venv_name=$(basename "$VIRTUAL_ENV")
    echo "The current python virtual environment is named '$venv_name'." >&2
    # valgrind --tool=memcheck rfm
    MALLOC_CHECK_=1 RUST_LOG=reedline_wrapper=debug rfm "$@" --InheritedPrompt "($venv_name) "
else
    MALLOC_CHECK_=1 RUST_LOG=reedline_wrapper=debug rfm "$@"
fi

exit_status=$?
# 139 == 128 + 11, 11 意思是 sigsegv
if [ $exit_status -eq 139 ]; then
    echo
    echo "请使用 rfm.sh --disable-addon 排查是否是addon造成了rfm程序崩溃"
    echo "如果最近更新了rfm程序，请尝试使用如下命令删除所有为旧版本编译的addon rfm_local.so，以便重新自动编译addon"
    echo "sudo updatedb"
    echo "locate rfm_local.so"
    echo "locate rfm_local.so|xargs rm"
    echo "sudo updatedb"
fi
