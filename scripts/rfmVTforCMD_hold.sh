#!/bin/bash

#LIBGL_ALWAYS_SOFTWARE=1 /usr/bin/alacritty --hold -e "$@"

foot -t xterm-256color  --hold --log-level=warning "$@"
