#!/bin/bash

HISTFILE=$2
HISTCONTROL=ignoreboth
history -r
read -e -r $1
history -s "$1"
history -w
