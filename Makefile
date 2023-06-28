# Makefile for RFM

include config.mk

VERSION = 1.9.4

# Edit below for extra libs (e.g. for thumbnailers etc.)
#LIBS = -L./libdcmthumb -lm -ldcmthumb
GTK_VERSION = gtk+-3.0
CPPFLAGS = -DrfmBinPath=\"${PREFIX}/bin\" --include ${languageInclude} -DG_LOG_DOMAIN=\"rfm\" ${GitIntegration}

# Uncomment the line below if compiling on a 32 bit system (otherwise stat() may fail on large directories; see man 2 stat)
CPPFLAGS += -D_FILE_OFFSET_BITS=64

SRC = rfm.c
OBJ = ${SRC:.c=.o}
INCS = -I. -I/usr/include
LIBS += -L/usr/lib `pkg-config --libs ${GTK_VERSION}`
CPPFLAGS += -DVERSION=\"${VERSION}\"
GTK_CFLAGS = `pkg-config --cflags ${GTK_VERSION}`
CFLAGS = -g -Wall -std=c11 -O0 ${GTK_CFLAGS} ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS}

# compiler and linker
CC = gcc

all: options rfm

options:
	@echo rfm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

rfm: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f rfm ${OBJ}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f rfm ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmRefreshImage.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmVTforCMD.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmVTforCMD_hold.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopy.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmMove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmMove.sh
	@cp -f scripts/rfmRemove_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@cp -f scripts/rfmProperties.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmOpenWith_.sh ${DESTDIR}${PREFIX}/bin
	@cp -f scripts/rfmCopySelectionToClipboard_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCopySelectionToClipboard.sh
	@cp -f scripts/rfmCopyClipboardToCurPath_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmCopyClipboardToCurPath.sh
	@cp -f scripts/rfmMoveClipboardToCurPath_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmMoveClipboardToCurPath.sh
	@cp -f scripts/rfmGitShowPictures_${languageInclude}.sh ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
#	@cp -f scripts/rfmNewFile.sh ${DESTDIR}${PREFIX}/bin
#	@cp -f scripts/rfmNewDir.sh ${DESTDIR}${PREFIX}/bin
#	@cp -f rfm.desktop /usr/share/applications/rfm.desktop  
# .desktop is installed in gentoo .ebuild
	@chmod 755 ${DESTDIR}${PREFIX}/bin/rfm
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmRefreshImage.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmVTforCMD.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmVTforCMD_hold.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopy.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopySelectionToClipboard.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmCopyClipboardToCurPath.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmMoveClipboardToCurPath.sh
	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
#	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmNewFile.sh
#	@chmod +x ${DESTDIR}${PREFIX}/bin/rfmNewDir.sh

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRefreshImage.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmVTforCMD.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmVTforCMD_hold.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCoy.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmMove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmRemove.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmProperties.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmOpenWith_.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopySelectionToClipboard.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmCopyClipboardToCurPath.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmMoveClipboardToCurPath.sh
	@rm -f ${DESTDIR}${PREFIX}/bin/rfmGitShowPictures.sh
#	@rm -f ${DESTDIR}${PREFIX}/bin/rfmNewFile.sh
#	@rm -f ${DESTDIR}${PREFIX}/bin/rfmNewDir.sh

.PHONY: all options clean install uninstall
