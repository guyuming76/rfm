#!/bin/sh
# Helper script for rfm: Display info about input files
#
# Copyright (c) 2012 Rodney Padgett <rod_padgett@hotmail.com>
# See LICENSE for details.
#
# ChangeLog:
#   29-01-2013: Changed listing method for multiple files.
#   06-03-2014: Added file_path: useful for copying path to a shell command.
#   13-06-2014: Use ls instead of stat
#   09-09-2017: Added ffprobe to display media info

show_info() {
   printf "\tPerms : $1\n"
   # Column 2 is number of hard links; ignore this one!
   printf "\tOwner : $3\n"
   printf "\tGroup : $4\n"
   printf "\tSize  : $5\n"
   printf "\tDate  : $6\n"
   printf "\tmtime : $7\n"
   printf "\n"
   printf "GroupMembers:\n"
   set -x
   grep ^$4 /etc/group
   set +x
}

if [ $# -eq 1 ]; then
   ls_info=$(ls -ahld --time-style="+%d-%m-%Y %H:%M" "$1")
   file_info=$(file -b "$1" | sed 's/\&/\&amp\;/g; s/</\&lt\;/g; s/>/\&gt\;/g')
   mime_type=$(file -b -i "$1")
   if [ -x /usr/bin/ffprobe ]; then
      echo "$mime_type" | grep -e audio -e video >/dev/null 2>&1
      [ $? -eq 0 ] && media_info=$(/usr/bin/ffprobe -hide_banner "$1" 2>&1 | grep Stream)
   fi
   # Escape pango markup characters in filename: replace & with &amp; < with &lt; and > with &gt;
   file_name=$(echo $(basename "$1") | sed 's/\&/\&amp\;/g; s/</\&lt\;/g; s/>/\&gt\;/g')
   if [ -d "$1" ]; then
      file_path=$(echo "$1" | sed 's/\&/\&amp\;/g; s/</\&lt\;/g; s/>/\&gt\;/g')
   else
      file_path=$(echo $(dirname "$1") | sed 's/\&/\&amp\;/g; s/</\&lt\;/g; s/>/\&gt\;/g')
   fi

   printf "Properties for:\n"
   printf "\t$file_name\n"
   printf "\n"
   printf "Path:\n"
   printf "\t$file_path\n"
   printf "\n"
   printf "Info:\n"
   show_info $ls_info
   printf "\n"
   printf "Mime type:\n"
   printf "\t $mime_type\n"
   printf "\n"
   printf "Contents Indicate:\n"
   printf "\t $file_info\n"
   [ ! -z media_info ] && printf "$media_info\n"
else
   dir_name=$(dirname "$1")
   cd "$dir_name"

   printf "Properties for selected items\n"
   printf "\n"
   for file in "$@"; do
      ls -hld "$(basename "$file")"
   done
fi

read -p "Press enter to quit"
