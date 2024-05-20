#!/bin/bash

if [[ -z "$RFM_TERM" ]];then
	# 从 ps -o ppid --no-header 中找出不包含在 ps -o pid --no-header 中的进程号
	# 通常，这个进程号在 ps -o -ppid --no-header 中排第一，但不一定，如果出现紫禁城号小于父进程号的情况会怎么样？所以不能直接取排第一的
	readarray -t ppid_array < <(ps -o ppid --no-header)
	readarray -t pid_array < <(ps -o pid --no-header)

	for item_in_ppid_array in "${ppid_array[@]}"; do
    		found=0
    		for item_in_pid_array in "${pid_array[@]}"; do
			if [[ $item_in_ppid_array==$item_in_pid_array ]];then
				found=1
				break
			fi
    		done
    		if [[ $found==0 ]];then
			export RFM_TERM=$(basename $(ps -p $item_in_ppid_array -o cmd --no-header))
			break
    		fi
	done
fi

rfm "$@"
