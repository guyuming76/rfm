#!/bin/bash

# 获取变量名参数
var_name="$1"
HISTFILE="$2"
HISTCONTROL=ignoreboth

# 读取历史记录
history -r

# 读取用户输入到指定的变量
# 注意：rfm 代理模式(exec_stdin_command_in_new_VT_with_proxy)会把 stdin/stdout/stderr 都重定向到文件，
# read -e (readline) 需要三个流都连终端才能正常工作，全部重定向到 /dev/tty
read -e -r "$var_name" 0</dev/tty 1>/dev/tty 2>/dev/tty

# 获取用户实际输入的值
user_input="${!var_name}"

# 只有非空输入才保存到历史
if [ -n "$user_input" ]; then
    history -s "$user_input"
    history -w
fi
