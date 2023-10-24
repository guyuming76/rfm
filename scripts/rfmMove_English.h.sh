#!/bin/bash

set -x

export destination=$(pwd)

read -p "Please input the move destination(default $destination ): " -r input_destination

[[ ! -z "$input_destination" ]] && destination=$input_destination

/bin/mv -i $@ $destination

read -p "enter 1 to launch new instance of rfm and open $destination, or just press enter to close this window: " -r next_action

[[ "$next_action" == "1" ]]  && rfm -d $destination

#[[ "$next_action" == "1" ]]  && rfm -d $destination 1>/tmp/rfm1.log 2>/tmp/rfm2.log &

#sleep 10s
