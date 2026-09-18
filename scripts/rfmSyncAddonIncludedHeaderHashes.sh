#!/bin/sh
# 把 rfm_addon.h 所 include 的头文件的 md5 同步到 rfm_addon.h 顶部的一个注释区块里。
#
# 目的（为什么需要它）：
#   被 rfm_addon.h include 的头文件（rfmLanguage.h、rfm_pybridge.h ...）一变
#     ⇒ rfm_addon.h 的内容变
#     ⇒ RFM_AddOn（= rfm_addon.h 的 md5，见 config.mk）变，并被 -DRFM_AddOn 烤进 rfm 二进制
#     ⇒ 运行时 rfm 拿自己二进制里的新 md5 与插件 .so 里记录的旧 md5 比较，不等 ⇒ 触发插件重编
#   这样就不必把每个头文件的内容都塞进 rfm_addon.h，也能让"头文件变了"促发插件重编。
#   本脚本只改注释文本，不影响编译结果；内容没变时不动文件（不改 mtime，避免每次 make 都触发重编）。
#
# 用法：rfmSyncAddonIncludedHeaderHashes.sh <rfm_addon.h> <label>=<headerpath> [<label>=<headerpath> ...]
#   label 用该头文件被 include 时使用的名字（如 rfmLanguage.h），方便一眼看出对应关系
#   新增被 include 的头文件时，只要在 config.mk 的调用里再追加一项即可。
#   headerpath 不存在时记作 MISSING 并告警，但不影响构建。
set -e

ADDON_H="$1"; shift
if [ -z "$ADDON_H" ] || [ $# -eq 0 ]; then
    echo "用法: $0 <rfm_addon.h> <label>=<headerpath> [...]" >&2
    exit 1
fi
if [ ! -f "$ADDON_H" ]; then
    echo "$0: 找不到 $ADDON_H" >&2
    exit 1
fi

BEGIN='// RFM_INCLUDED_HEADER_HASHES_BEGIN'
END='// RFM_INCLUDED_HEADER_HASHES_END'
LEGACY='// RFM_LANGUAGE_H_HASH:'

# 1. 生成期望的新区块
BLOCK=$( {
    printf '%s\n' "$BEGIN"
    printf '//   本区块由 config.mk 调用 scripts/rfmSyncAddonIncludedHeaderHashes.sh 自动生成，请勿手工编辑。\n'
    printf '//   作用：这里列出的头文件一旦变化，rfm_addon.h 的内容就变化 ⇒ RFM_AddOn(=rfm_addon.h 的 md5) 变\n'
    printf '//         ⇒ 重编 rfm ⇒ 运行时 rfm 用新 md5 与各插件 .so 里记录的旧值比对 ⇒ 触发所有插件重编。\n'
    for pair in "$@"; do
        label=${pair%%=*}
        hdr=${pair#*=}
        if [ -f "$hdr" ]; then
            hash=$(md5sum "$hdr" | cut -d' ' -f1)
        else
            hash=MISSING
            echo "$0: 警告：找不到 $hdr（对应 $label），记作 MISSING" >&2
        fi
        printf '// RFM_INCLUDED_HEADER_HASH: %s %s    <- %s\n' "$label" "$hash" "$hdr"
    done
    printf '%s' "$END"
} )

# 2. 取出文件里现有的区块（若有）
OLD_BLOCK=$(awk -v b="$BEGIN" -v e="$END" '
    $0 ~ b {inb=1}
    inb    {print}
    $0 ~ e {inb=0}
' "$ADDON_H")

# 3. 已经是最新（区块内容一致、且没有旧版脚本留下的单行注释）就什么都不做
if [ "$OLD_BLOCK" = "$BLOCK" ] && ! grep -q "^$LEGACY" "$ADDON_H"; then
    exit 0
fi

# 4. 去掉旧区块、旧版单行注释后的剩余内容，把新区块放到文件最前面
REST=$(awk -v b="$BEGIN" -v e="$END" -v legacy="$LEGACY" '
    $0 ~ b {inb=1; next}
    inb    {if ($0 ~ e) inb=0; next}
    index($0, legacy) == 1 {next}
    {print}
' "$ADDON_H")

NEW=$(printf '%s\n%s' "$BLOCK" "$REST")
printf '%s\n' "$NEW" > "$ADDON_H"     # 用 > 截断写回，保持原 inode / 权限
echo "$0: 已更新 $ADDON_H 里的头文件 hash 区块" >&2
