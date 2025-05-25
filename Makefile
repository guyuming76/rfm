# Makefile for RFM

include config.mk

VERSION = 2.80.0
# I change version to 2.80 since it need glib >=2.80

ifneq ($(RFM_FILE_CHOOSER),)
LINKERNAME = librfm.so
SONAME = librfm.so.1
LIBNAME = ${LINKERNAME}
LIBNAME := ${LIBNAME}.${VERSION}
endif

# Edit below for extra libs (e.g. for thumbnailers etc.)
#LIBS = -L./libdcmthumb -lm -ldcmthumb
GTK_VERSION = gtk+-3.0
# what shall be minimun gtk+ version for glib 2.80?

CPPFLAGS = -DrfmBinPath=${rfmBinPath} --include ${languageInclude} -DG_LOG_DOMAIN=\"rfm\" ${GitIntegration} ${PythonEmbedded} ${RFM_FILE_CHOOSER} ${MU_VIEW} ${READLINE_LIB} ${AiIntegration} ${RFM_LOCAL} ${CmarkTools}
# 检查glib版本是否≥2.80
ifeq ($(shell pkg-config --atleast-version=2.80 glib-2.0 && echo yes),yes)
CPPFLAGS += -DGLIB280PLUS
endif

# Uncomment the line below if compiling on a 32 bit system (otherwise stat() may fail on large directories; see man 2 stat)
CPPFLAGS += -D_FILE_OFFSET_BITS=64

INCS = -I. -I/usr/include
LIBS += -L/usr/lib `pkg-config --libs ${GTK_VERSION} gio-unix-2.0`
ifeq (${READLINE_LIB},-DGNU_readline)
LIBS += `pkg-config --libs readline`
endif
ifeq (${READLINE_LIB},-DLINENOISE)
LIBS += -I/usr/local/include -L/usr/local/lib -llinenoise 
endif
ifneq (${CmarkTools},)
LIBS += `pkg-config --libs libcmark`
endif
CPPFLAGS += -DVERSION=\"${VERSION}\"
GTK_CFLAGS = `pkg-config --cflags ${GTK_VERSION}`
CFLAGS += -g -Wall -std=c11 -O0 -fPIC -pie -Wno-error=incompatible-pointer-types ${GTK_CFLAGS} ${INCS} ${CPPFLAGS}
LDFLAGS += -g ${LIBS}

ifneq (${PythonEmbedded}, )
CFLAGS += `python3.11-config --cflags`
LDFLAGS += `python3.11-config --ldflags --embed`
endif

ifneq (${RFM_LOCAL},)
LDFLAGS += -rdynamic
endif

# compiler and linker
CC = gcc

ifneq ($(RFM_FILE_CHOOSER),)
all: options rfm ${LIBNAME} cmark_tools
else
all: options rfm cmark_tools
endif

ifneq (${CmarkTools},)
cmark_tools: extractKeyValuePairFromMarkdown extractContentFromMarkdownRecursive
else
cmark_tools:
endif

options:
	@echo rfm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

ifneq ($(RFM_FILE_CHOOSER),)
rfm.o: config.h rfmFileChooser.h
else
rfm.o: config.h
endif

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

rfm: rfm.o
	@echo CC -o $@
	@${CC} -o $@ rfm.o -lstdc++ ${LDFLAGS}

ifneq ($(RFM_FILE_CHOOSER),)
${LIBNAME}: rfm.o
	@echo CC -o $@
	@${CC} -o $@ rfm.o ${LDFLAGS} -shared -Wl,-soname=${SONAME},-E
#	@${CC} -o $@ ${OBJ} ${LDFLAGS} -shared -Wl,-E,-e,main
# i tried to use librfm.so as both lib and exec and set entry point for librfm.so, however, i got segfault after launching librfm.so
# https://unix.stackexchange.com/questions/223385/why-and-how-are-some-shared-libraries-runnable-as-though-they-are-executables
endif

ifneq (${CmarkTools},)
extractKeyValuePairFromMarkdown: extractKeyValuePairFromMarkdown.o
	@echo CC -o $@
	@${CC} -o $@ extractKeyValuePairFromMarkdown.o ${LDFLAGS}

extractContentFromMarkdownRecursive: extractContentFromMarkdownRecursive.o
	@echo CC -o $@
	@${CC} -o $@ extractContentFromMarkdownRecursive.o ${LDFLAGS}
endif

clean:
	@echo cleaning
ifneq ($(RFM_FILE_CHOOSER),)
	@rm -f rfm ${LIBNAME} rfm.o
else
	@rm -f rfm rfm.o
endif
ifneq (${CmarkTools},)
	@rm -f extractKeyValuePairFromMarkdown extractKeyValuePairFromMarkdown.o extractContentFromMarkdownRecursive extractContentFromMarkdownRecursive.o
endif

install: all
	@echo installing files to ${DESTDIR}${PREFIX}/{bin,lib}
	@mkdir -p ${DESTDIR}${PREFIX}/bin
ifneq ($(RFM_FILE_CHOOSER),)
	@mkdir -p ${DESTDIR}${PREFIX}/lib64
	@cp -f ${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${LIBNAME}
	@ln -sf ./${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${SONAME}
	@ln -sf ./${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${LINKERNAME}
endif
ifneq (${CmarkTools},)
	@cp -f extractKeyValuePairFromMarkdown ${DESTDIR}${PREFIX}/bin
	@cp -f extractContentFromMarkdownRecursive ${DESTDIR}${PREFIX}/bin
endif
	@cp -f rfm_local.h /usr/include
	@cp -f rfm ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmRefreshImage.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmVTforCMD.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmReadlineWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopyMove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@cp -f scripts/rfmRemove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@cp -f scripts/rfmProperties.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmToClipboard.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmFromClipboard.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmOpenWith_.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmTextEdit.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmShareDir.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopyMoveToCurPath_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCopyMoveToCurPath.sh
	@cp -f scripts/rfmCreateSoftLink_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCreateSoftLink.sh
	@cp -f scripts/rfmFindLinksToTheSameFile_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmFindLinksToTheSameFile.sh
#	@cp -f scripts/rfmGitShowPictures_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
	@cp -f scripts/rfmGitCommit_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmGitCommit.sh
	@cp -f scripts/rfmChangeOwner_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmChangeOwner.sh
	@cp -f scripts/rfmFindFilesWithTheSameName_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmFindFilesWithTheSameName.sh
#	@cp -f scripts/rfmNewFile.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmPressEnterToCloseWindowAfterCMD.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmExtractArchive_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmExtractArchive.sh
	@cp -f scripts/rfmGetMailHeaderWithMuView.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmMakeThumbnailForGGB.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmMoveDirAndUpdateSymbolicLinks.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopyDirAndUpdateSymbolicLinks.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm_mkdir_and_move_selected_files_into_it_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@cp -f scripts/rfmSaveSearchResultToFile_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@cp -f scripts/rfmLocateGlobalAddons.sh ${DESTDIR}${PREFIX}/bin
ifneq ($(AiIntegration),)
	@cp -f scripts/rfmAskOllama.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAskModelScope.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAskOpenRouter.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAskOpenRouter_FileArray.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiAppendFileContentIntoPrompt.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiGetParameterPromptAndFile.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiDisplayResult.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiSaveHistory.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiSaveHistory_WithDirectory.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiJsonMessagesArray.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiFindRelatedMessages_Direct.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmAiFindRelatedMessages.sh ${DESTDIR}${PREFIX}/bin
endif
ifneq ($(CalledByEbuild),YES)
# the CalledByEbuild variable is exported in rfm ebuild
# there will be sandbox error in gentoo ebuild run
	-cp -f rfm.svg /usr/share/icons/hicolor/scalable/apps/rfm.svg
	-gtk-update-icon-cache -f /usr/share/icons/hicolor
	-cp -f rfm.desktop /usr/share/applications/rfm.desktop
	-xdg-desktop-menu install --novendor rfm.desktop
	-xdg-mime default rfm.desktop inode/directory
	-update-desktop-database
	ldconfig
endif
ifneq ($(RFM_FILE_CHOOSER),)
	@chmod 755 ${DESTDIR}${PREFIX}/lib64/${LIBNAME}
endif
ifneq (${CmarkTools},)
	@chmod +x ${DESTDIR}${PREFIX}/bin/extractKeyValuePairFromMarkdown
	@chmod +x ${DESTDIR}${PREFIX}/bin/extractContentFromMarkdownRecursive
endif
	@chmod 755 ${DESTDIR}${PREFIX}/bin/rfm
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfm.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmRefreshImage.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmVTforCMD.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmToClipboard.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFromClipboard.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmTextEdit.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmShareDir.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyMoveToCurPath.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCreateSoftLink.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFindLinksToTheSameFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmGitCommit.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmChangeOwner.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFindFilesWithTheSameName.sh
#	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
#	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmNewFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmPressEnterToCloseWindowAfterCMD.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmExtractArchive.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmGetMailHeaderWithMuView.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMakeThumbnailForGGB.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMoveDirAndUpdateSymbolicLinks.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyDirAndUpdateSymbolicLinks.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmLocateGlobalAddons.sh
ifneq ($(AiIntegration),)
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAskOllama.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAskModelScope.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAskOpenRouter.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAskOpenRouter_FileArray.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiAppendFileContentIntoPrompt.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiGetParameterPromptAndFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiDisplayResult.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiSaveHistory.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiSaveHistory_WithDirectory.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiJsonMessagesArray.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiFindRelatedMessages_Direct.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmAiFindRelatedMessages.sh
endif
	@echo
	@echo "***please copy the .inputrc file into your home directory (~) manually."

uninstall:
	@echo removing files from ${DESTDIR}${PREFIX}/{bin,lib}
ifneq ($(RFM_FILE_CHOOSER),)
	@rm -f ${DESTDIR}${PREFIX}/lib64/${LIBNAME}
endif
ifneq (${CmarkTools},)
	@rm -f ${DESTDIR}${PREFIX}/bin/extractKeyValuePairFromMarkdown
	@rm -f ${DESTDIR}${PREFIX}/bin/extractContentFromMarkdownRecursive
endif
	@rm -f /usr/include/rfm_local.h
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRefreshImage.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmVTforCMD.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmToClipboard.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFromClipboard.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmTextEdit.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmShareDir.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyMoveToCurPath.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCreateSoftLink.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFindLinksToTheSameFile.sh
#	@rm -f ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmGitCommit.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmChangeOwner.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFindFilesWithTheSameName.sh
#	@rm -f ${DESTDIR}${PREFIX}/bin/rfmNewFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmPressEnterToCloseWindowAfterCMD.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmExtractArchive.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmGetMailHeaderWithMuView.sh
	@rm -f ${DESTDIR}${PREFIX}/bin//rfmMakeThumbnailForGGB.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmMoveDirAndUpdateSymbolicLinks.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyDirAndUpdateSymbolicLinks.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmLocateGlobalAddons.sh
ifneq ($(AiIntegration),)
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAskOllama.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAskModelScope.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAskOpenRouter.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAskOpenRouter_FileArray.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiAppendFileContentIntoPrompt.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiGetParameterPromptAndFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiDisplayResult.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiSaveHistory.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiSaveHistory_WithDirectory.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiJsonMessagesArray.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiFindRelatedMessages_Direct.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmAiFindRelatedMessages.sh
endif
ifneq ($(CalledByEbuild),YES)
	-xdg-desktop-menu uninstall rfm.desktop
	-rm -f /usr/share/applications/rfm.desktop
	-rm -f /usr/share/icons/hicolor/scalable/apps/rfm.svg
	-gtk-update-icon-cache -f /usr/share/icons/hicolor
	-update-desktop-database
	ldconfig
endif

.PHONY: all options clean install uninstall
