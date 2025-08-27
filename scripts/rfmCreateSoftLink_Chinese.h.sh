#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

#set -x

echo "输入为选中文件创建软链接的目录路径,可以是绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"

rfmReadlineWithSpecificHistoryFile.sh "rfmCreateSoftLink_DoAfterUserInput.sh"  ~/.rfm_history_directory "$@"
