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

RFM_AddOn = -DRFM_AddOn=\"$(shell md5sum rfm_addon.h | cut -d' ' -f1)\"
# 语言头（上面的 languageInclude）的 hash 写进 rfm_addon.h 首行注释：
# 语言头变化（或换语言）⇒ rfm_addon.h 内容变 ⇒ RFM_AddOn（rfm_addon.h 的 md5）变
# ⇒ rfm 触发所有插件重编。直接复用既有的 rfm_addon.h 哈希机制，rfm.c 无需改动。
# 注意：本行会（在内容变化时）自动改写 rfm_addon.h 首行，别手动编辑那一行。
#$(shell scripts/rfmUpdateAddonHeaderLanguageHash.sh $(languageInclude) rfm_addon.h)
#RFM_LOCAL =
#features such as LLM AI client is implemented as addon

#CFLAGS = -fsanitize=address

#LDFLAGS = -lasan

