#!/bin/bash

set -x

/bin/echo $@ | wl-copy

read -p "按回车关闭当前窗口"
