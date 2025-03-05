#!/bin/bash

#这个脚本首先检查是否传入了正确的参数，然后根据文件的扩展名选择适当的解压工具。对于`.gz`或`.tgz`文件，它会询问用户是将文件解压到当前目录[>
#请注意，这个脚本假设你的系统中已经安装了`tar`和`unzip`命令。如果你的系统上没有这些命令，你需要根据你的系统环境和需求来调整脚本。

# 检查参数数量
if [ $# -ne 1 ]; then
    echo "Usage: $0 <file_path>"
    exit 1
fi

FILE_PATH=$1

# 检查文件是否存在
if [ ! -e "$FILE_PATH" ]; then
    echo "File not found."
    exit 1
fi

# 获取文件扩展名
EXTENSION="${FILE_PATH##*.}"

case $EXTENSION in
    gz|tgz)
        read -p "Do you want to extract '$FILE_PATH' to the current directory? (y/n): " choice
        case $choice in
            y|Y )
                tar xf "$FILE_PATH"
                ;;
            n|N )
                read -p "Enter extraction path: " EXTRACT_PATH
                if [ ! -d "$EXTRACT_PATH" ]; then
                    echo "'$EXTRACT_PATH' is not a valid directory."
                    exit 1
                fi
                tar xf "$FILE_PATH" -C "$EXTRACT_PATH"
                ;;
            * )
                echo "Invalid choice."
                exit 1
                ;;
        esac
        ;;

    zip)
        read -p "Do you want to extract '$FILE_PATH' to the current directory? (y/n): " choice
        case $choice in
            y|Y )
                unzip "$FILE_PATH"
                ;;
            n|N )
                read -p "Enter extraction path: " EXTRACT_PATH
                if [ ! -d "$EXTRACT_PATH" ]; then
                    echo "'$EXTRACT_PATH' is not a valid directory."
                    exit 1
                fi
                unzip "$FILE_PATH" -d "$EXTRACT_PATH"
                ;;
            * )
                echo "Invalid choice."
                exit 1
                ;;
        esac
        ;;

    *)
        echo "Unsupported file type."
        exit 1
        ;;
esac
