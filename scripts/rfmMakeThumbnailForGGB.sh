#!/bin/bash

ggbFile=$1
thumbnailFile=$2
## TODO: use thumbnail size definition in config.h
## TODO: 如果没有thumbnailFile参数,也就是说从文件上下文菜单选 刷新缩略图 调用的本功能,需要根据 ggbFile 计算得到缩略图文件名
## add $$ in path to avoid conflict on local machine with multiple instance running
mkdir /tmp/$$
#echo $ggbFile >> /tmp/$$/log.txt
#echo $thumbnailFile >> /tmp/$$/log.txt
unzip "$ggbFile" geogebra_thumbnail.png -d /tmp/$$
convert /tmp/$$/geogebra_thumbnail.png -thumbnail 128x128^ -gravity center -extent 128x128 "$thumbnailFile"
rm -f /tmp/$$
