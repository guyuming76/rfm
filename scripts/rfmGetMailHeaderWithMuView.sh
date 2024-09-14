#!/bin/bash

echo "[https://discourse.gnome.org/t/gkeyfile-to-handle-conf-without-groupname/23080/3]"
echo
while read OneLine; do
	if [[ -z "$OneLine" ]]; then
		exit 0
	else
		echo $OneLine
	fi
done
