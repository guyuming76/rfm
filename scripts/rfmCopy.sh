#!/bin/bash

set -x

export destination=$(pwd)

read -p "Please input the copy destination(default $destination ): " -r input_destination

[[ ! -z "$input_destination" ]] && destination=$input_destination

/bin/cp -p -r -i $@ -t $destination

read -p "press enter to close this window"
