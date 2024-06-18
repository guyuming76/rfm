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
	echo "env RFM_TERM set to current terminal emulator:" $RFM_TERM

else
	echo "env RFM_TERM is: " $RFM_TERM
fi

rfm "$@"
