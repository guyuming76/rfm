#!/bin/bash

if which bat >/dev/null 2>&1 || alias bat 2>/dev/null | grep -q "^alias bat="; then
	bat -p --paging=never --color=always "$@"
else
	# 执行 xdg-open 并将其放入后台，避免 GLib 错误和退出码问题, 详细参见commit comment
	xdg-open "$@" >/dev/null 2>&1 &
	# 立即返回成功
	exit 0
fi
