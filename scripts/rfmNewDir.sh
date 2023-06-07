#!/bin/bash

set -x

read -p "Please input the new directory name: $(pwd)/" -r newDirName

[[ ! -z "$newDirName" ]] && mkdir $newDirName || echo "new directory name cannot be empty!"

read -p "press enter to close this window"
