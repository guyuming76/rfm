#!/bin/bash

find "$1" -exec test "$2"  -ef {} \; -print|xargs -r dirname|xargs -r realpath --relative-to="$1"
