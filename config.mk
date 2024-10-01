PREFIX = /usr/local
# if installed with gentoo ebuild, the ebuild script will remove /local so that files will be installed into /usr/bin instead of /usr/local/bin

rfmBinPath =\"${PREFIX}/bin/\"
#rfmBinPath = \"\"
# you can set rfmBinPath to \"\" if you want to use executable or scripts in search path

languageInclude = Chinese.h
#languageInclude = English.h

GitIntegration = -DGitIntegration
#GitIntegration =

PythonEmbedded = -DPythonEmbedded
#PythonEmbedded =

RFM_FILE_CHOOSER = -DRFM_FILE_CHOOSER
#RFM_FILE_CHOOSER =

ParseMarkdown = -DParseMarkdown
#ParseMarkdown =

#CFLAGS = -fsanitize=address

#LDFLAGS = -lasan
