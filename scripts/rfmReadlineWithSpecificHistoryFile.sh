#!/bin/bash

HISTFILE=$1
HISTCONTROL=ignoreboth
history -r
read -e -r input
history -s "$input"
history -w
echo $input
