#!/bin/bash

if [ $# -eq 0 ]; then
	rfm -r -p
else
	rfm -r -p -d "$@"
fi

#
# rfm -p -d /home/guyuming/rfm/rfm.c  < <(locate rfm.c)

#rfmFileChooserNoVT4searchResult.sh /home/guyuming/rfm/rfm.c < <(locate rfm.c)
