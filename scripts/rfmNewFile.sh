#!/bin/bash

set -x

read -p "Please input the new file name: $(pwd)/" -r newFileName

[[ ! -z "$newFileName" ]] && touch $newFileName || echo "new file name cannot be empty!"

read -p "press enter to close this window"
