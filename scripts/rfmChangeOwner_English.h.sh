#!/bin/bash

set -x

export currentUserName=$(id -un)

read -p "输入新所有者用户名，默认为 $currentUserName" -r New_Owner

[[ -z "$New_Owner" ]] && New_Owner=$currentUserName

sudo chown -R $New_Owner "$@"

read -p "输入回车关闭当前窗口"
