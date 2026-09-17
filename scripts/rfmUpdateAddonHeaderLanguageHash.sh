#!/bin/sh
# 把语言头（config.mk 的 languageInclude：Chinese.h / English.h）的 md5 同步到
# rfm_addon.h 的首行注释里。
#
# 目的：语言头（或换语言）一变 ⇒ rfm_addon.h 内容变 ⇒ RFM_AddOn（= rfm_addon.h
# 的 md5，见 config.mk）变 ⇒ rfm 触发所有插件重编。
# 这样直接复用既有的 "rfm_addon.h 哈希" 机制，rfm.c 完全不必知道语言头的存在。
#
# 用法：rfmUpdateAddonHeaderLanguageHash.sh <语言头文件> <rfm_addon.h>
#   - 内容没变时什么都不做（不动 mtime，避免每次 make 都改写文件）
#   - 语言头不存在时只告警，不影响构建
set -e

LANG_H="$1"
ADDON_H="$2"
if [ -z "$LANG_H" ] || [ -z "$ADDON_H" ]; then
    echo "用法: $0 <语言头文件> <rfm_addon.h>" >&2
    exit 1
fi
if [ ! -f "$LANG_H" ]; then
    echo "$0: 找不到语言头 $LANG_H，跳过同步（rfm_addon.h 保持不变）" >&2
    exit 0
fi
if [ ! -f "$ADDON_H" ]; then
    echo "$0: 找不到 $ADDON_H" >&2
    exit 1
fi

HASH=$(md5sum "$LANG_H" | cut -d' ' -f1)
LINE="// RFM_LANGUAGE_H_HASH: $HASH  —— 由 config.mk 依据 languageInclude=$LANG_H 自动生成，请勿手动编辑本行（语言头变化会改变本行，从而触发所有插件重编）"

if [ "$(head -n 1 "$ADDON_H")" = "$LINE" ]; then
    exit 0                                  # 已经是最新，什么都不做
fi

TMP="$ADDON_H.tmp.$$"
if head -n 1 "$ADDON_H" | grep -q '^// RFM_LANGUAGE_H_HASH:'; then
    # 已有本脚本生成的首行：替换它
    { printf '%s\n' "$LINE"; tail -n +2 "$ADDON_H"; } > "$TMP"
else
    # 首次：插到文件最前面（在 #ifndef RFM_ADDON_H 之前）
    { printf '%s\n' "$LINE"; cat "$ADDON_H"; } > "$TMP"
fi
cat "$TMP" > "$ADDON_H"                     # 用 cat 覆盖，保持原 inode / 权限
rm -f "$TMP"
echo "$0: 已把 $LANG_H 的 hash（$HASH）写入 $ADDON_H 首行"
