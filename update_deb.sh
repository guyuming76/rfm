#!/bin/bash

# 脚本用于更新 RFM 项目的 .deb 包

set -e

echo "=== RFM .deb 包更新脚本 ==="

# 从 config.mk 文件中提取版本号
echo "提取版本号..."
VERSION=$(grep "^VERSION" config.mk | sed 's/.*= //')
echo "版本号: $VERSION"

# 清理之前的构建文件
echo "清理之前的构建文件..."
rm -rf rfm_${VERSION}_amd64
rm -f rfm_${VERSION}_amd64.deb

# 检查必要的文件是否存在
echo "检查必要的文件..."
if [ ! -f "rfm" ]; then
    echo "警告: rfm 可执行文件不存在"
    echo "请在实际系统中编译项目后再运行此脚本"
    echo "或者确保 rfm 可执行文件存在于当前目录"
    exit 1
fi

if [ ! -f "librfm.so.${VERSION}" ]; then
    echo "警告: librfm.so.${VERSION} 库文件不存在"
    echo "请在实际系统中编译项目后再运行此脚本"
    echo "或者确保 librfm.so.${VERSION} 库文件存在于当前目录"
    exit 1
fi

if [ ! -d "reedline_wrapper/target/debug" ] || [ ! -f "reedline_wrapper/target/debug/libreedline_wrapper.so" ]; then
    echo "警告: reedline_wrapper 库文件不存在，将跳过"
    REEDLINE_AVAILABLE=0
else
    REEDLINE_AVAILABLE=1
fi

if [ ! -d "rfmReedline/target/debug" ] || [ ! -f "rfmReedline/target/debug/rfmReedline" ]; then
    echo "警告: rfmReedline 可执行文件不存在，将跳过"
    RFMREEDLINE_AVAILABLE=0
else
    RFMREEDLINE_AVAILABLE=1
fi

# 创建 deb 包目录结构
echo "创建 deb 包目录结构..."
mkdir -p rfm_${VERSION}_amd64/DEBIAN \
         rfm_${VERSION}_amd64/usr/bin \
         rfm_${VERSION}_amd64/usr/lib64 \
         rfm_${VERSION}_amd64/usr/include \
         rfm_${VERSION}_amd64/usr/share/icons/hicolor/scalable/apps \
         rfm_${VERSION}_amd64/usr/share/applications

# 创建控制文件
# jq, curl 包用在 AI addon 里, gcc 用于编译addon
echo "创建控制文件..."
cat > rfm_${VERSION}_amd64/DEBIAN/control << EOF
Package: rfm
Version: ${VERSION}
Architecture: amd64
Maintainer: guyuming <gu_yu_ming@139.com>
Depends: libgtk-3-0, libjson-glib-1.0-0, libreadline8, libcmark0.30.2, python3, bash, locate, gcc, jq, curl
Recommends: imv, bat, mpv
Section: utils
Priority: optional
Homepage: https://gitee.com/guyuming76/rfm
Description: A powerful file manager with AI integration
 RFM is a feature-rich file manager with
 integrated AI capabilities, Git integration, and Python support.
 It provides a user-friendly interface for managing files and directories,
 with advanced features like file preview, batch operations, and more
EOF

# 从 config.mk 文件中提取配置选项
echo "加载配置文件..."
# 提取 languageInclude
languageInclude=$(grep "^languageInclude" config.mk | sed 's/.*= //')
# 提取 RFM_FILE_CHOOSER
RFM_FILE_CHOOSER=$(grep "^RFM_FILE_CHOOSER" config.mk | sed 's/.*= //')
# 提取 READLINE_LIB
READLINE_LIB=$(grep "^READLINE_LIB" config.mk | sed 's/.*= //')
# 提取 CmarkTools
CmarkTools=$(grep "^CmarkTools" config.mk | sed 's/.*= //')
# 提取 PythonEmbedded
PythonEmbedded=$(grep "^PythonEmbedded" config.mk | sed 's/.*= //')

# 输出提取的配置值
echo "配置选项:"
echo "  languageInclude: $languageInclude"
echo "  RFM_FILE_CHOOSER: $RFM_FILE_CHOOSER"
echo "  READLINE_LIB: $READLINE_LIB"
echo "  CmarkTools: $CmarkTools"
echo "  PythonEmbedded: $PythonEmbedded"

# 复制文件到 deb 包目录
echo "复制文件到 deb 包目录..."

# 复制主可执行文件
cp rfm rfm_${VERSION}_amd64/usr/bin/

# 复制库文件（如果 RFM_FILE_CHOOSER 启用）
if [ ! -z "$RFM_FILE_CHOOSER" ]; then
    cp librfm.so.${VERSION} rfm_${VERSION}_amd64/usr/lib64/
    ln -s librfm.so.${VERSION} rfm_${VERSION}_amd64/usr/lib64/librfm.so.1
    ln -s librfm.so.${VERSION} rfm_${VERSION}_amd64/usr/lib64/librfm.so
fi

# 复制 Reedline 相关文件（如果 READLINE_LIB 为 -Dreedline）
if [ "$READLINE_LIB" = "-Dreedline" ]; then
    if [ $REEDLINE_AVAILABLE -eq 1 ]; then
        cp reedline_wrapper/target/debug/libreedline_wrapper.so rfm_${VERSION}_amd64/usr/lib64/
    fi
    if [ $RFMREEDLINE_AVAILABLE -eq 1 ]; then
        cp rfmReedline/target/debug/rfmReedline rfm_${VERSION}_amd64/usr/bin/
    fi
    cp scripts/rfmReedlineWithSpecificHistoryFile.sh rfm_${VERSION}_amd64/usr/bin/rfmReadlineWithSpecificHistoryFile.sh
else
    cp scripts/rfmBashReadWithSpecificHistoryFile.sh rfm_${VERSION}_amd64/usr/bin/rfmReadlineWithSpecificHistoryFile.sh
fi

# 复制 Bash 读取脚本
cp scripts/rfmBashReadWithSpecificHistoryFile.sh rfm_${VERSION}_amd64/usr/bin/

# 复制 Cmark 工具（如果 CmarkTools 启用）
if [ ! -z "$CmarkTools" ]; then
    for tool in extractKeyValuePairFromMarkdown extractContentFromMarkdownRecursive extractJsonFromMarkdown; do
        if [ -f "$tool" ]; then
            cp "$tool" rfm_${VERSION}_amd64/usr/bin/
        else
            echo "警告: $tool 不存在，将跳过"
        fi
    done
fi

# 复制脚本文件
cp scripts/rfm.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmRefreshImage.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmVTforCMD.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmCopyMove_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmCopyMove.sh
cp scripts/rfmRemove_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmRemove.sh
cp scripts/rfmProperties.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmToClipboard.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmFromClipboard.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmOpenWith_.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmTextEdit.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmCopyMoveToCurPath_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmCopyMoveToCurPath.sh
cp scripts/rfmCreateSoftLink_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmCreateSoftLink.sh
cp scripts/rfmFindLinksToTheSameFile_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmFindLinksToTheSameFile.sh
cp scripts/rfmGitCommit_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmGitCommit.sh
cp scripts/rfmChangeOwner_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmChangeOwner.sh
cp scripts/rfmFindFilesWithTheSameName_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmFindFilesWithTheSameName.sh
cp scripts/rfmPressEnterToCloseWindowAfterCMD.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmExtractArchive_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmExtractArchive.sh
cp scripts/rfmMakeThumbnailForGGB.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmMoveDirAndUpdateSymbolicLinks.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmCopyDirAndUpdateSymbolicLinks.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfm_mkdir_and_move_selected_files_into_it_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfm_mkdir_and_move_selected_files_into_it.sh
cp scripts/rfmSaveSearchResultToFile_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmSaveSearchResultToFile.sh
cp scripts/rfmFindDimensionValueForFile.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmShowAllValuesForOneDimension.sh rfm_${VERSION}_amd64/usr/bin/
cp scripts/rfmMount_${languageInclude}.sh rfm_${VERSION}_amd64/usr/bin/rfmMount.sh
cp scripts/rfmFind_DIMENSION_PROPERTIES_FILEs.sh rfm_${VERSION}_amd64/usr/bin/

# 复制共享目录脚本（根据 PythonEmbedded 是否启用）
if [ ! -z "$PythonEmbedded" ]; then
    cp scripts/rfmShareDir.py rfm_${VERSION}_amd64/usr/bin/
else
    cp scripts/rfmShareDir.sh rfm_${VERSION}_amd64/usr/bin/
fi

# 复制头文件
cp rfm_addon.h rfm_${VERSION}_amd64/usr/include/

# 复制桌面文件和图标
cp rfm.svg rfm_${VERSION}_amd64/usr/share/icons/hicolor/scalable/apps/
cp rfm.desktop rfm_${VERSION}_amd64/usr/share/applications/

# 设置权限
echo "设置权限..."
chmod 755 rfm_${VERSION}_amd64/usr/bin/*
chmod 755 rfm_${VERSION}_amd64/usr/lib64/*
chmod 644 rfm_${VERSION}_amd64/usr/include/*
chmod 644 rfm_${VERSION}_amd64/usr/share/icons/hicolor/scalable/apps/*
chmod 644 rfm_${VERSION}_amd64/usr/share/applications/*
chmod 644 rfm_${VERSION}_amd64/DEBIAN/control

# 创建 deb 包
echo "创建 deb 包..."
dpkg-deb --build rfm_${VERSION}_amd64

# 检查创建的 deb 包
echo "检查创建的 deb 包..."
ls -la rfm_${VERSION}_amd64.deb

echo "\n=== .deb 包更新完成 ==="
echo "可以使用以下命令安装："
echo "sudo dpkg -i rfm_${VERSION}_amd64.deb"
echo "sudo apt-get install -f  # 自动安装依赖项"
