#!/bin/bash
# 接受一个或多个文件名作为参数,目前从rfm上下文菜单点击操作里面传出的都是完整路径文件名

# 脚本修改deepseek提示词：
# 脚本会在bash 交互模式下运行，接受多个完整路径文件名参数，并提示用户输入一个目录路径input_destination, 然后会在此目录下创建指向脚本输入参数文件路径的软链接。
# 现在我想要让脚本也能在bash非交互模式下运行，在非交互模式下，input_destination 变量值从脚本最后一个输入参数获得。并使后续read命令给用户的选项在非交互模式下都取默认值，也就是如同在交互模式下用户直接输入回车一样的执行效果。
# 同时还要保持脚本在bash 交互模式下运行效果和修改前一样，要怎么修改脚本？


#set -x

# 检查是否是交互模式（是否可以从tty获取输入）
if [ -t 0 ]; then
    NON_INTERACTIVE=0
else
    NON_INTERACTIVE=1
    # 在非交互模式下，最后一个参数是目标目录
    input_destination="${@: -1}"
    # 移除最后一个参数（目录路径）
    set -- "${@:1:$(($#-1))}"
fi

create_next="y"
while [[ "${create_next,,}" =~ "y" ]]; do

if [[ $NON_INTERACTIVE -eq 0 ]]; then
    # 交互模式：提示用户输入目录
    echo "输入目录路径,在其中创建指向选中文件的软链接。可以输入绝对路径或相对与当前路径($(pwd)),可用上下箭头方向键从历史输入中选择:"
    
    source rfmReadlineWithSpecificHistoryFile.sh "input_destination" ~/.rfm_history_directory
    input_destination=${input_destination/#~/${HOME}}
fi

if [[ ! -d "$input_destination" ]]; then
    if [[ $NON_INTERACTIVE -eq 1 ]]; then
        # 非交互模式：自动创建目录
        mkdir -p $input_destination
    else
        # 交互模式：询问用户是否创建目录
        read -p "输入路径不存在,是否(y/n)创建目录?默认是(y)" -r mkdir_input_destination
        if [[ -z "$mkdir_input_destination" || "$mkdir_input_destination" == "y" || "$mkdir_input_destination" == "Y" ]];then
            mkdir -p $input_destination
        fi
    fi
fi

if [[ -d "$input_destination" ]]; then
    if [[ $NON_INTERACTIVE -eq 1 ]]; then
        # 非交互模式：使用默认值
        useRelativeInSL=""
        autoGitStage=""
    else
        # 交互模式：询问用户选项
        read -p "是否(y/n)使用相对路径创建软链接?默认是(y)" -r useRelativeInSL
        read -p "是否(y/n)自动 git stage 创建的软链接文件?默认是(y)" -r autoGitStage
    fi
    
    sl_list=""

    for i in "$@"; do
        sl_basename=$(basename "$i")
        
        if [[ $NON_INTERACTIVE -eq 1 ]]; then
            # 非交互模式：使用默认文件名
            input_sl_basename=""
        else
            # 交互模式：询问用户软链接名
            read -p "输入要创建的软链接名,回车取默认值 $sl_basename" -r input_sl_basename
        fi
        
        if [[ ! -z "$input_sl_basename" ]]; then
            sl_basename=$input_sl_basename
        fi

        if [[ -z "$useRelativeInSL" || "$useRelativeInSL" == "y" || "$useRelativeInSL" == "Y" ]];then
            ln -sr "$i" $(echo $input_destination/$sl_basename)
        else
            ln -s "$i" $(echo $input_destination/$sl_basename)
        fi

        if [[ -z "$autoGitStage" || "$autoGitStage" == "y" || "$autoGitStage" == "Y" ]];then
            git stage $(echo $input_destination/$sl_basename)
        fi

        sl_list+=" $input_destination/$sl_basename"
    done
fi

if [[ $NON_INTERACTIVE -eq 1 ]]; then
    # 非交互模式：不继续创建，退出循环
    create_next="n"
else
    # 交互模式：询问用户是否继续
    read -p "是否(y/n)继续创建下一个指向此选中文件的软链接?默认否(n):" create_next
fi

done
