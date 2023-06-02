# Makefile for RFM
VERSION = 1.9.4

# Edit below for extra libs (e.g. for thumbnailers etc.)
#LIBS = -L./libdcmthumb -lm -ldcmthumb
GTK_VERSION = gtk+-3.0
CPPFLAGS =

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
PREFIX = /usr/local
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
	@cp -f scripts/RefreshImage.sh ${DESTDIR}${PREFIX}/bin
#	@cp -f rfm.desktop /usr/share/applications/rfm.desktop 
	@chmod 755 ${DESTDIR}${PREFIX}/bin/rfm
	@chmod +x ${DESTDIR}${PREFIX}/bin/RefreshImage.sh

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/rfm
	@rm -f ${DESTDIR}${PREFIX}/bin/RefreshImage.sh

.PHONY: all options clean install uninstall
