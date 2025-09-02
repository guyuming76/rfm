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

ifneq ($(RFM_AddOn),)
    RFM_ADDON_SO_FILE := $(subst \",,$(RFM_AddOn))       # 移除所有转义双引号
    RFM_ADDON_SO_FILE := $(subst -DRFM_AddOn=,,$(RFM_ADDON_SO_FILE)) # 移除开头的编译选项
endif


# Edit below for extra libs (e.g. for thumbnailers etc.)
#LIBS = -L./libdcmthumb -lm -ldcmthumb
GTK_VERSION = gtk+-3.0
# what shall be minimun gtk+ version for glib 2.80?

CPPFLAGS = -DrfmBinPath=${rfmBinPath} --include ${languageInclude} -DG_LOG_DOMAIN=\"rfm\" ${GitIntegration} ${PythonEmbedded} ${RFM_FILE_CHOOSER} ${READLINE_LIB} ${RFM_AddOn} ${CmarkTools}
# 检查glib版本是否≥2.80
ifeq ($(shell pkg-config --atleast-version=2.80 glib-2.0 && echo yes),yes)
CPPFLAGS += -DGLIB280PLUS
endif

# Uncomment the line below if compiling on a 32 bit system (otherwise stat() may fail on large directories; see man 2 stat)
CPPFLAGS += -D_FILE_OFFSET_BITS=64

INCS = -I. -I/usr/include
LIBS += -L/usr/lib `pkg-config --libs ${GTK_VERSION} gio-unix-2.0 json-glib-1.0`
#ifeq (${READLINE_LIB},-DGNU_readline)  在使用 reedline 时，我们还是会使用gnu readline 的filename completion, 不考虑 linenoise, 所以我们总是加上gnu readline
LIBS += `pkg-config --libs readline`
#endif
ifeq (${READLINE_LIB},-Dreedline)
LIBS += -L./reedline_wrapper/target/debug -lreedline_wrapper
endif
ifneq (${CmarkTools},)
LIBS += `pkg-config --libs libcmark`
endif
CPPFLAGS += -DVERSION=\"${VERSION}\"
GTK_CFLAGS = `pkg-config --cflags ${GTK_VERSION} json-glib-1.0`
CFLAGS += -g -Wall -std=c11 -O0 -fPIC -pie -Wno-error=incompatible-pointer-types ${GTK_CFLAGS} ${INCS} ${CPPFLAGS}
LDFLAGS += -g ${LIBS}

ifneq (${PythonEmbedded}, )
CFLAGS += `python-config --cflags`
LDFLAGS += `python-config --ldflags --embed`
endif

ifneq (${RFM_AddOn},)
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
cmark_tools: extractKeyValuePairFromMarkdown extractContentFromMarkdownRecursive extractJsonFromMarkdown
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

ifeq (${READLINE_LIB},-Dreedline)
rfm.o: build_reedline
endif

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

extractJsonFromMarkdown: extractJsonFromMarkdown.o
	@echo CC -o $@
	@${CC} -o $@ extractJsonFromMarkdown.o ${LDFLAGS}
endif

clean:
	@echo cleaning
ifneq ($(RFM_FILE_CHOOSER),)
	@rm -f rfm ${LIBNAME} rfm.o
else
	@rm -f rfm rfm.o
endif
ifneq (${CmarkTools},)
	@rm -f extractKeyValuePairFromMarkdown extractKeyValuePairFromMarkdown.o extractContentFromMarkdownRecursive extractContentFromMarkdownRecursive.o extractJsonFromMarkdown extractJsonFromMarkdown.o
endif
ifeq ($(READLINE_LIB),-Dreedline)
	@echo "Cleaning reedline_wrapper..."
	@cd reedline_wrapper && cargo clean && cd ..
endif

install:
	@echo installing files to ${DESTDIR}${PREFIX}/{bin,lib}
	@mkdir -p ${DESTDIR}${PREFIX}/bin
ifneq ($(RFM_FILE_CHOOSER),)
	@mkdir -p ${DESTDIR}${PREFIX}/lib64
	@cp -f ${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${LIBNAME}
	@ln -sf ./${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${SONAME}
	@ln -sf ./${LIBNAME} ${DESTDIR}${PREFIX}/lib64/${LINKERNAME}
endif
ifeq ($(READLINE_LIB),-Dreedline)
	@mkdir -p ${DESTDIR}${PREFIX}/lib64
	# deepseek说 先cp 再mv 可以让正在运行的rfm继续使用现有libreedline_wrapper,防止crash，这叫 原子替换
	@cp -f reedline_wrapper/target/debug/libreedline_wrapper.so ${DESTDIR}${PREFIX}/lib64/libreedline_wrapper.so.new
	@mv -f ${DESTDIR}${PREFIX}/lib64/libreedline_wrapper.so.new ${DESTDIR}${PREFIX}/lib64/libreedline_wrapper.so
	@cp -f rfmReedline/target/debug/rfmReedline ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmReedlineWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
else
	@cp -f scripts/rfmReadlineWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin
endif
ifneq (${CmarkTools},)
	@cp -f extractKeyValuePairFromMarkdown ${DESTDIR}${PREFIX}/bin
	@cp -f extractContentFromMarkdownRecursive ${DESTDIR}${PREFIX}/bin
	@cp -f extractJsonFromMarkdown ${DESTDIR}${PREFIX}/bin
endif
	@cp -f rfm_addon.h /usr/include
	@cp -f rfm ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmRefreshImage.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmVTforCMD.sh ${DESTDIR}${PREFIX}/bin
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
	@cp -f scripts/rfmMakeThumbnailForGGB.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmMoveDirAndUpdateSymbolicLinks.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopyDirAndUpdateSymbolicLinks.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfm_mkdir_and_move_selected_files_into_it_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@cp -f scripts/rfmSaveSearchResultToFile_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@cp -f scripts/rfmLocateGlobalAddons.sh ${DESTDIR}${PREFIX}/bin

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
	@chmod +x ${DESTDIR}${PREFIX}/bin/extractJsonFromMarkdown
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
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMakeThumbnailForGGB.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMoveDirAndUpdateSymbolicLinks.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyDirAndUpdateSymbolicLinks.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmLocateGlobalAddons.sh

ifneq ($(RFM_AddOn),)
	@echo
	-locate $(RFM_ADDON_SO_FILE)
	@echo "***please remove the files above returned by locate if any."
#-locate $(RFM_ADDON_SO_FILE) | xargs rm -i
#删除RFM_ADDON_SO_FILE, ADDON机制只会在rfm_local.c文件hash变化时重新编译，如果rfm_addon.h变了，rfm.c重新编译了，而rfm_local.c不重新编译，会出错
endif

uninstall:
	@echo removing files from ${DESTDIR}${PREFIX}/{bin,lib}
ifneq ($(RFM_FILE_CHOOSER),)
	@rm -f ${DESTDIR}${PREFIX}/lib64/${LIBNAME}
endif
ifeq ($(READLINE_LIB),-Dreedline)
	@rm -f ${DESTDIR}${PREFIX}/lib64/libreedline_wrapper.so
endif
ifneq (${CmarkTools},)
	@rm -f ${DESTDIR}${PREFIX}/bin/extractKeyValuePairFromMarkdown
	@rm -f ${DESTDIR}${PREFIX}/bin/extractContentFromMarkdownRecursive
	@rm -f ${DESTDIR}${PREFIX}/bin/extractJsonFromMarkdown
endif
	@rm -f /usr/include/rfm_addon.h
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
	@rm -f ${DESTDIR}${PREFIX}/bin//rfmMakeThumbnailForGGB.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmMoveDirAndUpdateSymbolicLinks.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyDirAndUpdateSymbolicLinks.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm_Update_affected_SymbolicLinks_for_move_or_copy.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm_mkdir_and_move_selected_files_into_it.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmSaveSearchResultToFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmLocateGlobalAddons.sh

ifneq ($(CalledByEbuild),YES)
	-xdg-desktop-menu uninstall rfm.desktop
	-rm -f /usr/share/applications/rfm.desktop
	-rm -f /usr/share/icons/hicolor/scalable/apps/rfm.svg
	-gtk-update-icon-cache -f /usr/share/icons/hicolor
	-update-desktop-database
	ldconfig
endif

build_reedline:
	@echo "Building reedline_wrapper..."
	@cd reedline_wrapper && cargo build && cd ..
	@echo "reedline_wrapper built successfully."
	@cd rfmReedline && cargo build && cd ..
	@echo "rfmReedline built successfully."

.PHONY: all options clean install uninstall build_reedline
