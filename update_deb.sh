#!/bin/bash

# 脚本用于更新 RFM 项目的 .deb 包

set -e

echo "=== RFM .deb 包更新脚本 ==="

# 检查 PREFIX 是否包含 /local
echo "检查 PREFIX 配置..."
PREFIX=$(grep "^PREFIX" config.mk | sed 's/.*= //')
echo "当前 PREFIX: $PREFIX"

if [[ "$PREFIX" == *"/local"* ]]; then
    echo "错误: PREFIX 包含 /local，这会导致 deb 包安装到 /usr/local 目录"
    echo "请修改 config.mk 文件中的 PREFIX 值，去掉 /local，例如将 '/usr/local' 改为 '/usr'"
    echo "然后make clean, make 重新编译项目，再运行此脚本"
    exit 1
fi

# 从 config.mk 文件中提取版本号
echo "提取版本号..."
VERSION=$(grep "^VERSION" config.mk | sed 's/.*= //')
echo "版本号: $VERSION"

# 从 config.mk 文件中提取语言配置
echo "提取语言配置..."
languageInclude=$(grep "^languageInclude" config.mk | sed 's/.*= //')
# 提取语言名称（去掉 .h 后缀），例如 Chinese.h -> Chinese
LANGUAGE=$(echo "$languageInclude" | sed 's/\.h$//')
echo "语言: $LANGUAGE"

# 设置包名（包含语言标识）
PKG_NAME="rfm_${VERSION}_${LANGUAGE}_amd64"
echo "包名: $PKG_NAME"

# 清理之前的构建文件
echo "清理之前的构建文件..."
rm -rf rfm_*_amd64
rm -f rfm_*_amd64.deb

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

# 检查 languageInclude 是否已定义
if [ -z "$languageInclude" ]; then
    echo "错误: 无法从 config.mk 中提取 languageInclude"
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
mkdir -p ${PKG_NAME}/DEBIAN \
         ${PKG_NAME}/usr/bin \
         ${PKG_NAME}/usr/lib64 \
         ${PKG_NAME}/usr/include \
         ${PKG_NAME}/usr/share/icons/hicolor/scalable/apps \
         ${PKG_NAME}/usr/share/applications

# 创建控制文件
# jq, curl 包用在 AI addon 里, gcc 用于编译addon
echo "创建控制文件..."
cat > ${PKG_NAME}/DEBIAN/control << EOF
Package: rfm
Version: ${VERSION}
Architecture: amd64
Maintainer: guyuming <gu_yu_ming@139.com>
Depends: libgtk-3-0, libjson-glib-1.0-0, libreadline8, libcmark0.30.2, bash
Recommends: git, python3, locate, gcc, jq, curl
Suggests: imv, bat, mpv
Section: utils
Priority: optional
Homepage: https://gitee.com/guyuming76/rfm
Description: A powerful file manager with AI integration
 RFM is a feature-rich file manager with
 integrated AI capabilities, Git integration, and Python support.
 It provides a user-friendly interface for managing files and directories,
 with advanced features like file preview, batch operations, and more
EOF

# 从 config.mk 文件中提取其他配置选项
echo "加载其他配置选项..."
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
echo "  语言: $LANGUAGE"
echo "  RFM_FILE_CHOOSER: $RFM_FILE_CHOOSER"
echo "  READLINE_LIB: $READLINE_LIB"
echo "  CmarkTools: $CmarkTools"
echo "  PythonEmbedded: $PythonEmbedded"

# 复制文件到 deb 包目录
echo "复制文件到 deb 包目录..."

# 复制主可执行文件
cp rfm ${PKG_NAME}/usr/bin/

# 复制库文件（如果 RFM_FILE_CHOOSER 启用）
if [ ! -z "$RFM_FILE_CHOOSER" ]; then
    cp librfm.so.${VERSION} ${PKG_NAME}/usr/lib64/
    ln -s librfm.so.${VERSION} ${PKG_NAME}/usr/lib64/librfm.so.1
    ln -s librfm.so.${VERSION} ${PKG_NAME}/usr/lib64/librfm.so
fi

# 复制 Reedline 相关文件（如果 READLINE_LIB 为 -Dreedline）
if [ "$READLINE_LIB" = "-Dreedline" ]; then
    if [ $REEDLINE_AVAILABLE -eq 1 ]; then
        cp reedline_wrapper/target/debug/libreedline_wrapper.so ${PKG_NAME}/usr/lib64/
    fi
    if [ $RFMREEDLINE_AVAILABLE -eq 1 ]; then
        cp rfmReedline/target/debug/rfmReedline ${PKG_NAME}/usr/bin/
    fi
    cp scripts/rfmReedlineWithSpecificHistoryFile.sh ${PKG_NAME}/usr/bin/rfmReadlineWithSpecificHistoryFile.sh
else
    cp scripts/rfmBashReadWithSpecificHistoryFile.sh ${PKG_NAME}/usr/bin/rfmReadlineWithSpecificHistoryFile.sh
fi

# 复制 Bash 读取脚本
cp scripts/rfmBashReadWithSpecificHistoryFile.sh ${PKG_NAME}/usr/bin/

# 复制 Cmark 工具（如果 CmarkTools 启用）
if [ ! -z "$CmarkTools" ]; then
    for tool in extractKeyValuePairFromMarkdown extractContentFromMarkdownRecursive extractJsonFromMarkdown; do
        if [ -f "$tool" ]; then
            cp "$tool" ${PKG_NAME}/usr/bin/
        else
            echo "警告: $tool 不存在，将跳过"
        fi
    done
fi

# 复制脚本文件
cp scripts/rfm.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmRefreshImage.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmVTforCMD.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmCopyMove_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmCopyMove.sh
cp scripts/rfmRemove_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmRemove.sh
cp scripts/rfmProperties.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmToClipboard.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmFromClipboard.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmOpenWith_.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmTextEdit.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmTextOpen.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmCopyMoveToCurPath_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmCopyMoveToCurPath.sh
cp scripts/rfmCreateSoftLink_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmCreateSoftLink.sh
cp scripts/rfmFindLinksToTheSameFile_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmFindLinksToTheSameFile.sh
cp scripts/rfmGitCommit_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmGitCommit.sh
cp scripts/rfmChangeOwner_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmChangeOwner.sh
cp scripts/rfmFindFilesWithTheSameName_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmFindFilesWithTheSameName.sh
cp scripts/rfmPressEnterToCloseWindowAfterCMD.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmExtractArchive_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmExtractArchive.sh
cp scripts/rfmMakeThumbnailForGGB.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmMoveDirAndUpdateSymbolicLinks.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmCopyDirAndUpdateSymbolicLinks.sh ${PKG_NAME}/usr/bin/
cp scripts/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh ${PKG_NAME}/usr/bin/
cp scripts/rfm_mkdir_and_move_selected_files_into_it_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfm_mkdir_and_move_selected_files_into_it.sh
cp scripts/rfmSaveSearchResultToFile_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmSaveSearchResultToFile.sh
cp scripts/rfmFindDimensionValueForFile.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmShowAllValuesForOneDimension.sh ${PKG_NAME}/usr/bin/
cp scripts/rfmMount_${languageInclude}.sh ${PKG_NAME}/usr/bin/rfmMount.sh
cp scripts/rfmFind_DIMENSION_PROPERTIES_FILEs.sh ${PKG_NAME}/usr/bin/

# 复制共享目录脚本（根据 PythonEmbedded 是否启用）
if [ ! -z "$PythonEmbedded" ]; then
    cp scripts/rfmShareDir.py ${PKG_NAME}/usr/bin/
else
    cp scripts/rfmShareDir.sh ${PKG_NAME}/usr/bin/
fi

# 复制头文件
cp rfm_addon.h ${PKG_NAME}/usr/include/

# 复制桌面文件和图标
cp rfm.svg ${PKG_NAME}/usr/share/icons/hicolor/scalable/apps/
cp rfm.desktop ${PKG_NAME}/usr/share/applications/

# 设置权限
echo "设置权限..."
chmod 755 ${PKG_NAME}/usr/bin/*
chmod 755 ${PKG_NAME}/usr/lib64/*
chmod 644 ${PKG_NAME}/usr/include/*
chmod 644 ${PKG_NAME}/usr/share/icons/hicolor/scalable/apps/*
chmod 644 ${PKG_NAME}/usr/share/applications/*
chmod 644 ${PKG_NAME}/DEBIAN/control

# 创建 deb 包
echo "创建 deb 包..."
dpkg-deb --build ${PKG_NAME}

# 检查创建的 deb 包
echo "检查创建的 deb 包..."
ls -la ${PKG_NAME}.deb

echo "=== .deb 包更新完成 ==="
echo "可以使用以下命令安装："
echo "sudo dpkg -i ${PKG_NAME}.deb"
echo "sudo apt-get install -f  # 自动安装依赖项"
echo "或:"
echo "sudo apt install --install-recommends ./${PKG_NAME}.deb"
