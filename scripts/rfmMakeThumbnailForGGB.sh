#!/bin/bash

ggbFile=$1
thumbnailFile=$2
## TODO: use thumbnail size definition in config.h

## add $$ in path to avoid conflict on local machine with multiple instance running
mkdir /tmp/$$
#echo $ggbFile >> /tmp/$$/log.txt
#echo $thumbnailFile >> /tmp/$$/log.txt
unzip "$ggbFile" geogebra_thumbnail.png -d /tmp/$$
convert /tmp/$$/geogebra_thumbnail.png -thumbnail 128x128^ -gravity center -extent 128x128 "$thumbnailFile"
rm -f /tmp/$$
