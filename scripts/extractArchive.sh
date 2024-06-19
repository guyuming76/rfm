#! /bin/bash
# extractArchive V2.0
#  Script to extract the supplied archive to /tmp, then display
#  a file manager.
#
# This version extracts by mime type using the file command.
#
# Rodney Padgett 10-04-2013 <rod_padgett@hotmail.com>
#
# ChangeLog:
# 27-10-2016: For security, add random code and check for existance of $dirName and exit if it exists
# 03-07-2019: Change mimetype for gzip from application/x-gzip to application/gzip
# 16-07-2020: Add zstandard (zstd) decompression.

# Set filemanager command to display archive contents
filemanager_cmd="rfm -d"

# Set how messages are shown
show_msg() {
   gxmessage -fn monospace "$1" "$2"
}

pname=$(basename "$0")
rndCode=$(head -10 /dev/urandom | md5sum | cut -d " " -f 1)
dirName="/tmp/extractArchive.$$.$rndCode"
archiveName="$1"
tmpName=$(echo "$archiveName" | awk -F '[-. ]' '{print $1 }')
fileName=$(basename "$tmpName")
tarLog="$dirName/$fileName.log"

if [ $# -eq 0 ]; then
   echo "$0 usage:"
   echo "   $0 filename [preserve]"
   echo "Options:"
   echo "   preserve - don't delete the extracted archive on exit."
   exit 0
fi

mimeType=$(file -b --mime-type "$archiveName")

if [ -e "$dirName" ]; then
   show_msg "$pname: $dirName exists!"
   exit 1
fi

mkdir "$dirName"
chmod 700 "$dirName"

error=0

case "$mimeType" in
   application/x-xz)
      xzcat "$archiveName" > "$dirName/$fileName" 2> "$tarLog"
      error=$?
   ;;
   application/gzip)
      zcat "$archiveName" > "$dirName/$fileName" 2> "$tarLog"
      error=$?
   ;;
   application/x-bzip2)
      bzcat "$archiveName" > "$dirName/$fileName" 2> "$tarLog"
      error=$?
   ;;
   application/x-tar)
      tar -xf "$archiveName" -C "$dirName" > "$tarLog" 2>&1
      error=$?
   ;;
   application/zip)
      unzip "$archiveName" -d "$dirName" > "$tarLog" 2>&1
      error=$?
   ;;
   application/x-rpm)
      rpm2cpio "$archiveName" | bsdtar -xf - -C "$dirName" > "$tarLog" 2>&1
      error=$?
      ;;
   application/epub+zip)
      unzip "$archiveName" -d "$dirName" > "$tarLog" 2>&1
      error=$?
   ;;
   application/zstd)
      zstdcat "$archiveName" > "$dirName/$fileName" 2> "$tarLog"
      error=$?
   ;;
   *)
      echo "$pname: ERROR: Mime type $mimeType" > "$tarLog"
      error=1
esac

# Pass 2: Check for archive files
if [ -e "$dirName/$fileName" ]; then
   mimeType=$(file -b --mime-type "$dirName/$fileName")
   case "$mimeType" in
      application/x-tar)
         mv "$dirName/$fileName" "$dirName/$fileName.tar"
         tar -xf "$dirName/$fileName.tar" -C "$dirName" >> "$tarLog" 2>&1
         rm "$dirName/$fileName.tar"
         error=$?
      ;;
   esac
fi

if [ $error -ne 0 ]; then
   show_msg -file "$tarLog"
   rm -rf "$dirName"
   exit 1
fi

$filemanager_cmd "$dirName"
if [ $? -ne 0 ]; then
   show_msg "$pname: can't initialise filemanager! The archive has been extracted to $dirName"
   exit 1
fi

case "$2" in
   preserve)
      show_msg "$pname: The archive has been extracted to $dirName"
   ;;
   *)
      rm -rf "$dirName"
esac
