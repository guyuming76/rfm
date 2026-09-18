VERSION = 2.80.0
# I change version to 2.80 since it need glib >=2.80

PREFIX = /usr/local
# if installed with gentoo ebuild, the ebuild script will remove /local so that files will be installed into /usr/bin instead of /usr/local/bin

rfmBinPath =\"${PREFIX}/bin/\"
#rfmBinPath = \"\"
# you can set rfmBinPath to \"\" if you want to use executable or scripts in search path ?

languageInclude = Chinese.h
#languageInclude = English.h
#This decides UI language compiled into rfm. I use .h file instead of the "resource" file in other system

GitIntegration = -DGitIntegration
#GitIntegration =

#READLINE_LIB = -Dreedline
# if you don't use rust on your system, comment out, reedline and use GNU_readline below. you will lose the multiline editting and interactive menu selection with arrow keys in TAB completion in command input.
#READLINE_LIB = -DGNU_readline
READLINE_LIB = -Dprompt_toolkit
# Use Python prompt_toolkit instead of Rust reedline. Requires PythonEmbedded and prompt_toolkit package.

PythonEmbedded = -DPythonEmbedded
#PythonEmbedded =
# http and ftp share is currently implemented with PythonEmbedded

RFM_FILE_CHOOSER = -DRFM_FILE_CHOOSER
#RFM_FILE_CHOOSER =

CmarkTools = -DCmarkTools
#CmarkTools =
# CmarkTools is used to extract markdown title and content, for example, returned by AI LLM

# ┌─ 把 rfm_addon.h 所 include 的头文件的 hash 同步进 rfm_addon.h ────────────────────────────
# 被 rfm_addon.h include 的头文件（rfmLanguage.h ← languageInclude、rfm_pybridge.h ...）一变
#   ⇒ rfm_addon.h 内容变 ⇒ RFM_AddOn（= rfm_addon.h 的 md5，见下一行）变，并被烤进 rfm 二进制
#   ⇒ 运行时 rfm 用新的 md5 与插件 .so 里记录的旧 md5 比对，不等 ⇒ 触发所有插件重编。
# 于是 rfm_addon.h 可以直接 include 别的头文件，而不必把这些头的内容都搬进来。
# 用 := 立即求值，保证在本文件下面求 rfm_addon.h 的 md5 之前先执行；脚本在内容无变化时不改文件。
# 要新增"被 include 的头"，只需在下面的调用里再追加一项 <label>=<headerpath>。
# 注意：Makefile 里 rfm.o 必须把 rfm_addon.h 及这些头列为依赖，否则新 md5 不会被烤进二进制。
RFM_SyncIncludedHeaderHashes := $(shell scripts/rfmSyncAddonIncludedHeaderHashes.sh rfm_addon.h rfmLanguage.h=$(languageInclude) rfm_pybridge.h=rfm_pybridge.h)

RFM_AddOn = -DRFM_AddOn=\"$(shell md5sum rfm_addon.h | cut -d' ' -f1)\"

#RFM_AddOn =            #注释掉上面的 RFM_AddOn 即禁用 addon（老名字叫 RFM_LOCAL，已废弃）
#features such as LLM AI client is implemented as addon

#CFLAGS = -fsanitize=address

#LDFLAGS = -lasan

