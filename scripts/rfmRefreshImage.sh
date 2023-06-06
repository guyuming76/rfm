#!/bin/bash

imv_pid=$(ps -u|grep -e imv -e for_rfm |grep -v grep|awk '{print $2}')

if test -z "$imv_pid" 
then  
    imv $1 for_rfm 
else   
    imv-msg $imv_pid close
    imv-msg $imv_pid open $1
fi 

#[[ -z "$imv_pid" ]] && echo "empty" || echo $imv_pid
