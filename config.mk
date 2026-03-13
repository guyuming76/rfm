PREFIX = /usr/local
# if installed with gentoo ebuild, the ebuild script will remove /local so that files will be installed into /usr/bin instead of /usr/local/bin

rfmBinPath =\"${PREFIX}/bin/\"
#rfmBinPath = \"\"
# you can set rfmBinPath to \"\" if you want to use executable or scripts in search path

languageInclude = Chinese.h
#languageInclude = English.h

GitIntegration = -DGitIntegration
#GitIntegration =

READLINE_LIB = -Dreedline
# if you don't use rust on your system, comment out, reedline and use GNU_readline below. you will lose the multiline editting in command input.
#READLINE_LIB = -DGNU_readline

PythonEmbedded = -DPythonEmbedded
#PythonEmbedded =

RFM_FILE_CHOOSER = -DRFM_FILE_CHOOSER
#RFM_FILE_CHOOSER =

CmarkTools = -DCmarkTools
#CmarkTools =

RFM_AddOn = -DRFM_AddOn=\"rfm_local.so\"
#RFM_LOCAL =

#CFLAGS = -fsanitize=address

#LDFLAGS = -lasan

