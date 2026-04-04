#!/bin/bash

# 检查 emacs 是否存在
if ! which emacs >/dev/null 2>&1; then
    # 执行 xdg-open 打开文件
    echo "Opening with xdg-open: $@"
    xdg-open "$@" &
    # 等待一下，确保 xdg-open 有足够时间启动, 否则，非常诡异，执行mate-terminal -x /usr/bin/rfmTextEdit.sh /home/guyuming/rfm/update_deb.sh 这个命令，啥也不会发生，TRAE AI给出的解释是脚本执行得太快，还没等xdg-open 打开文档刚打开的新终端就又关了，界面看上去就像啥也没发生一样，真见鬼了，还有这样的，TRAE AI给后面加了sleep 1, 居然这句命令就好了！
    sleep 1
    exit 0
fi

# 如果有 C1 环境变量（来自 rfm 搜索结果）
if [ -n "$C1" ]; then
    # 正则表达式判读是否是数字
    if [[ $C1 =~ ^[0-9]+$ ]]; then
        emacs -nw --no-splash +$C1 "$@"
    else
        emacs -nw --no-splash "$@"
    fi
else
    # 直接打开文件
    emacs -nw --no-splash "$@"
fi
