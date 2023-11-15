#!/bin/bash

if [ $# -eq 0 ]; then
	rfm -r 0>/dev/null 1>/dev/null
else
	rfm -r -d "$@" 0>/dev/null 1>/dev/null
fi
