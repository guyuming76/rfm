#!/bin/bash

# Display the result
if which bat >/dev/null 2>&1; then
    echo "$RESULT" | bat -p --language=md
else
    echo "$RESULT"
    echo
    echo
    echo "安装 bat 软件(gentoo 上的包是sys-apps/bat)可以语法高亮显示当前markdown内容"
fi

read -p "是否显示原始回复json? [y/N] " displayJSON
displayJSON=${displayJSON:-N}
if [[ "$displayJSON" =~ ^[Yy]$ ]]; then
    echo $RESPONSE|jq
fi
