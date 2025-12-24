#!/bin/bash

# 获取变量名参数
var_name="$1"
HISTFILE="$2"
HISTCONTROL=ignoreboth

# 读取历史记录
history -r

# 读取用户输入到指定的变量
read -e -r "$var_name"

# 获取用户实际输入的值
user_input="${!var_name}"

# 只有非空输入才保存到历史
if [ -n "$user_input" ]; then
    history -s "$user_input"
    history -w
fi
