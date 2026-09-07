# Makefile for RFM

include config.mk

ifneq ($(RFM_FILE_CHOOSER),)
LINKERNAME = librfm.so
SONAME = librfm.so.1
LIBNAME := ${LINKERNAME}.${VERSION}
endif
#git blame 里面显示 https://forums.gentoo.org/viewtopic-t-1147467-start-0.html ,soname 链接应由上游 Makefile/install 脚本创建，gentoo Portage 不负责生成

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
#ifeq (${READLINE_LIB},-DGNU_readline)  在使用 reedline 时，我们还在使用gnu readline 的rl_completion_matches函数, 所以我们总是加上gnu readline
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

# prompt_toolkit 依赖 PythonEmbedded
ifeq (${READLINE_LIB},-Dprompt_toolkit)
ifneq (${PythonEmbedded},-DPythonEmbedded)
$(error prompt_toolkit requires PythonEmbedded. Set "PythonEmbedded = -DPythonEmbedded" in config.mk)
endif
endif

ifneq (${PythonEmbedded}, )
CFLAGS += `python3-config --cflags`
LDFLAGS += `python3-config --ldflags --embed`
endif

ifneq (${RFM_AddOn},)
LDFLAGS += -rdynamic
CPPFLAGS += -DRFM_AddOn_H_filepath=\"${DESTDIR}${PREFIX}/include/rfm_addon.h\"
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
ifneq ($(CalledByEbuild),YES)
rfm.o: build_reedline
endif
endif

ifeq (${READLINE_LIB},-Dprompt_toolkit)
rfm: rfm_pt_wrapper.o
rfm_pt_wrapper.o: rfm_pt_wrapper.c prompt_toolkit_wrapper.h
	@echo CC $<
	@${CC} -c ${CFLAGS} $<
endif

# 公共 C↔Python 嵌入胶水层：编译进 rfm 主程序，插件通过 -rdynamic 导出的符号调用
ifeq (${PythonEmbedded}, )
PYBRIDGE_OBJ =
else
PYBRIDGE_OBJ = rfm_pybridge.o
rfm_pybridge.o: rfm_pybridge.c rfm_addon.h
	@echo CC $<
	@${CC} -c ${CFLAGS} $<
endif

ifneq ($(RFM_FILE_CHOOSER),)
rfm.o: config.h rfmFileChooser.h
else
rfm.o: config.h
endif

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

rfm: rfm.o ${PYBRIDGE_OBJ}
	@echo CC -o $@
ifeq (${READLINE_LIB},-Dprompt_toolkit)
	@${CC} -o $@ rfm.o rfm_pt_wrapper.o ${PYBRIDGE_OBJ} -lstdc++ ${LDFLAGS}
else
	@${CC} -o $@ rfm.o ${PYBRIDGE_OBJ} -lstdc++ ${LDFLAGS}
endif

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
	@echo "Cleaning rfmReedline..."
	@cd rfmReedline && cargo clean && cd ..
endif
ifeq ($(READLINE_LIB),-Dprompt_toolkit)
	@rm -f rfm_pt_wrapper.o
	@rm -rf rfm_prompt_toolkit/__pycache__
endif
ifneq (${PythonEmbedded}, )
	@rm -f rfm_pybridge.o
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
else ifeq ($(READLINE_LIB),-Dprompt_toolkit)
	@mkdir -p ${DESTDIR}${PREFIX}/bin/rfm_prompt_toolkit
	@cp -rf rfm_prompt_toolkit/*.py ${DESTDIR}${PREFIX}/bin/rfm_prompt_toolkit/
	@cp -f scripts/rfmReedlineWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
else
	@cp -f scripts/rfmBashReadWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
endif
	@cp -f scripts/rfmBashReadWithSpecificHistoryFile.sh ${DESTDIR}${PREFIX}/bin
ifneq (${CmarkTools},)
	@cp -f extractKeyValuePairFromMarkdown ${DESTDIR}${PREFIX}/bin
	@cp -f extractContentFromMarkdownRecursive ${DESTDIR}${PREFIX}/bin
	@cp -f extractJsonFromMarkdown ${DESTDIR}${PREFIX}/bin
endif
	@cp -f rfm ${DESTDIR}${PREFIX}/bin
	@cp -f rfm.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmRefreshImage.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmVTforCMD.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopyMove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@cp -f scripts/rfmRemove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@cp -f scripts/rfmProperties.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmToClipboard.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmFromClipboard.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmOpenWith_.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmTextEdit.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmTextOpen.sh ${DESTDIR}${PREFIX}/bin
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
	@cp -f scripts/rfmFindDimensionValueForFile.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmShowAllValuesForOneDimension.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmMount_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmMount.sh
	@cp -f scripts/rfmFind_DIMENSION_PROPERTIES_FILEs.sh ${DESTDIR}${PREFIX}/bin/
	@cp -f scripts/rfmConcatVideos.sh ${DESTDIR}${PREFIX}/bin/
ifneq (${PythonEmbedded}, )
	@cp -f rfm_pybridge.py ${DESTDIR}${PREFIX}/bin/
endif
ifneq (${RFM_AddOn},)
	@cp -f scripts/rfmEnsurePythonVenv.sh ${DESTDIR}${PREFIX}/bin/
endif

ifneq ($(CalledByEbuild),YES)
# the CalledByEbuild variable is exported in rfm ebuild
# there will be sandbox error in gentoo ebuild run
	@cp -f rfm_addon.h ${DESTDIR}${PREFIX}/include
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
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmBashReadWithSpecificHistoryFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmToClipboard.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFromClipboard.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmTextEdit.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmTextOpen.sh
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
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFindDimensionValueForFile.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmShowAllValuesForOneDimension.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMount.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmFind_DIMENSION_PROPERTIES_FILEs.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmConcatVideos.sh
ifneq (${RFM_AddOn},)
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmEnsurePythonVenv.sh
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
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRefreshImage.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmVTforCMD.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmReadlineWithSpecificHistoryFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmBashReadWithSpecificHistoryFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyMove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmToClipboard.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFromClipboard.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmTextEdit.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmTextOpen.sh
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
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFindDimensionValueForFile.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmShowAllValuesForOneDimension.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmMount.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmFind_DIMENSION_PROPERTIES_FILEs.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmConcatVideos.sh
ifneq (${PythonEmbedded}, )
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm_pybridge.py
endif
ifneq (${RFM_AddOn},)
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmEnsurePythonVenv.sh
endif

ifneq ($(CalledByEbuild),YES)
	@rm -f ${DESTDIR}${PREFIX}/include/rfm_addon.h
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


#Makefile中rfm主程序编译也使用了-fPIC, rfm_addon.h 中没有那些 extern 函数的声明。如果把extern函数的声明加入rfm_addon.h中，主程序编译就可以去掉-fPIC? 如果可以这两种方法各有啥优劣？
## **当前做法（情况1）：主程序使用`-fPIC`，头文件无extern声明**

### **工作原理**
#1. **主程序编译**：`-fPIC`使主程序成为位置无关代码，所有全局符号默认导出
#2. **动态库编译**：在`rfm_local.c`中手动声明`extern`函数
#3. **运行时解析**：动态链接器查找已加载模块的符号表

### **优势**
#1. **符号控制灵活**：主程序不需要预先知道哪些函数会被插件使用
#2. **接口最小化**：`rfm_addon.h`保持简洁，只定义结构体和回调函数
#3. **向后兼容**：新增的插件可以访问主程序的所有全局函数，无需重新编译主程序
#4. **插件开发便捷**：插件开发者可以根据需求自由声明需要的函数

### **劣势**
#1. **性能开销**：`-fPIC`会增加一些性能开销（通过GOT/PLT间接寻址）
#2. **安全性**：所有全局符号都暴露给插件，可能被恶意插件滥用
#3. **调试困难**：插件开发者需要查看源代码才能知道可用的函数
