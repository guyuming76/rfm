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

READLINE_LIB = -Dreedline
# if you don't use rust on your system, comment out, reedline and use GNU_readline below. you will lose the multiline editting and interactive menu selection with arrow keys in TAB completion in command input.
#READLINE_LIB = -DGNU_readline

PythonEmbedded = -DPythonEmbedded
#PythonEmbedded =
# http and ftp share is currently implemented with PythonEmbedded

RFM_FILE_CHOOSER = -DRFM_FILE_CHOOSER
#RFM_FILE_CHOOSER =

CmarkTools = -DCmarkTools
#CmarkTools =
# CmarkTools is used to extract markdown title and content, for example, returned by AI LLM

RFM_AddOn = -DRFM_AddOn=\"$(shell md5sum rfm_addon.h | cut -d' ' -f1)\"
#RFM_LOCAL =
#features such as LLM AI client is implemented as addon

#CFLAGS = -fsanitize=address

#LDFLAGS = -lasan

