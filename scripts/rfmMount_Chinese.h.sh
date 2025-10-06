#!/bin/bash
#这个脚本的功能包括：

#1. **参数检查**：要求提供一个目录路径作为挂载点
#2. **磁盘信息显示**：先执行 `lsblk` 和 `sudo blkid` 显示当前磁盘配置
#3. **用户交互**：
#   - 输入要挂载的设备路径
#   - 选择所有者（root用户或当前用户）
#   - 选择挂载模式（只读或读写）
#4. **命令构建**：根据用户选择构建相应的 `mount` 命令选项
#5. **命令执行**：使用 `set -x` 显示实际执行的命令，然后执行挂载操作
#6. **结果验证**：检查挂载是否成功并显示结果


# 检查是否提供了目录参数
if [ $# -ne 1 ]; then
 echo "用法: $0 <挂载目录路径>"
 exit 1
fi

MOUNT_DIR="$1"

# 检查挂载目录是否存在
if [ ! -d "$MOUNT_DIR" ]; then
 echo "错误: 目录 '$MOUNT_DIR' 不存在"
 exit 1
fi

# 显示当前磁盘配置
echo "=== 当前磁盘设备列表 (lsblk) ==="
lsblk

echo -e "\n=== 磁盘文件系统信息 (sudo blkid),需输入密码 ==="
sudo blkid

echo -e "\n======================================"

# 提示用户输入要挂载的设备
read -p "请输入要挂载的设备 (例如: /dev/sdb1): " DEVICE

# 检查设备是否存在
if [ ! -b "$DEVICE" ]; then
 echo "错误: 设备 '$DEVICE' 不存在或不是块设备"
 exit 1
fi

# 提问所有者选项（默认当前用户）
echo -e "\n选择挂载后的目录所有者:"
echo "1) root 用户"
echo "2) 当前用户 ($USER) [默认]"
read -p "请选择 [1/2] (默认2): " OWNER_CHOICE

# 设置默认值为当前用户
if [ -z "$OWNER_CHOICE" ]; then
 OWNER_CHOICE="2"
fi

# 提问读写权限（默认读写）
echo -e "\n选择挂载模式:"
echo "1) 只读 (ro)"
echo "2) 读写 (rw) [默认]"
read -p "请选择 [1/2] (默认2): " MODE_CHOICE

# 设置默认值为读写
if [ -z "$MODE_CHOICE" ]; then
 MODE_CHOICE="2"
fi

# 构建mount选项
MOUNT_OPTIONS=""

case $OWNER_CHOICE in
 1)
 # root用户，不需要额外选项
 ;;
 2)
 # 当前用户（默认）
 MOUNT_OPTIONS="uid=$(id -u),gid=$(id -g)"
 ;;
 *)
 echo "错误: 无效的所有者选择"
 exit 1
 ;;
esac

case $MODE_CHOICE in
 1)
 MOUNT_OPTIONS="${MOUNT_OPTIONS},ro"
 ;;
 2)
 MOUNT_OPTIONS="${MOUNT_OPTIONS},rw"
 ;;
 *)
 echo "错误: 无效的模式选择"
 exit 1
 ;;
esac

# 移除开头的逗号（如果有）
MOUNT_OPTIONS=$(echo "$MOUNT_OPTIONS" | sed 's/^,//')

echo -e "\n======================================"
echo "挂载配置摘要:"
echo "设备: $DEVICE"
echo "挂载点: $MOUNT_DIR"
echo "挂载选项: $MOUNT_OPTIONS"
echo -e "======================================\n"

# 确认操作（默认为Y）
read -p "确认执行挂载操作? [Y/n]: " CONFIRM
if [[ ! $CONFIRM =~ ^[Yy]$ ]] && [[ ! -z "$CONFIRM" ]]; then
 echo "操作已取消"
 exit 0
fi

# 执行挂载命令（显示实际执行的命令）
set -x
sudo mount -o "$MOUNT_OPTIONS" "$DEVICE" "$MOUNT_DIR"
set +x

# 检查挂载是否成功
if mount | grep -q "$MOUNT_DIR"; then
 echo -e "\n✅ 挂载成功!"
 echo "挂载信息:"
 mount | grep "$MOUNT_DIR"
else
 echo -e "\n❌ 挂载失败!"
 exit 1
fi
