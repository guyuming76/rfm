#!/bin/bash

if [ $# -eq 0 ]; then
	rfm -r 0>/dev/null 1>/dev/null
else
	rfm -r -d "$@" 0>/dev/null 1>/dev/null
fi

## TODO:
## why did i add 0>/dev/null 1>/dev/null here?
## i cannot recall now. does the output to stdout cause trouble to g_spawn in caller?
