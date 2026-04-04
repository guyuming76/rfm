#!/bin/bash

if which bat >/dev/null 2>&1 || alias bat 2>/dev/null | grep -q "^alias bat="; then
	bat -p --paging=never --color=always "$@"
else
	xdg-open "$@"
fi
