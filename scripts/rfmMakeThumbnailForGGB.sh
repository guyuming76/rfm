#!/bin/bash

thumbnailsize_str=$1
ggbFile=$2
thumbnailFile=$3

# add $$ in path to avoid conflict on local machine with multiple instance running
mkdir /tmp/$$
#echo $ggbFile >> /tmp/$$/log.txt
#echo $thumbnailFile >> /tmp/$$/log.txt
unzip "$ggbFile" geogebra_thumbnail.png -d /tmp/$$
convert /tmp/$$/geogebra_thumbnail.png -thumbnail $thumbnailsize_str -gravity center -extent $thumbnailsize_str "$thumbnailFile"
rm -f /tmp/$$
