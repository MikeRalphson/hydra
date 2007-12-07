# Makefile for charcon
BIN_DIR = /usr/local/bin
MAN_DIR = /usr/local/man
#MAN_DIR = /usr/man
MANEXT = 1
#
SHELL=/bin/sh
CC=gcc
CFLAGS= #-O

LFLAGS=

CFILES= charconv.c getopt.c
OFILES= charconv.o getopt.o

CFILES2= char_arr.c getopt.c
OFILES2= char_arr.o getopt.o

.c.o:
	$(CC) $(CFLAGS) -c $<

charconv: $(OFILES)
	$(CC) $(CFLAGS) -o charconv $(OFILES) $(LFLAGS)

char_arr: $(OFILES2)
	$(CC) $(CFLAGS) -o char_arr $(OFILES2) $(LFLAGS)

charconv.o: charconv.c charstab.h sys_def.h

char_arr.o: char_arr.c charstab.h sys_def.h

getopt.o: sys_def.h

install: charconv
	-if [ ! -d $(BIN_DIR) ] ; then mkdir -p $(BIN_DIR); fi
	strip charconv
	cp charconv $(BIN_DIR)

install.man: charconv.man
	-if [ ! -d $(MAN_DIR) ] ; then mkdir -p $(MAN_DIR); fi
	-if [ ! -d $(MAN_DIR)/man$(MANEXT) ]; \
	  then mkdir -p $(MAN_DIR)/man$(MANEXT); fi
	cp charconv.man $(MAN_DIR)/man$(MANEXT)/charconv.$(MANEXT)

clean:
	rm -f *.o charconv char_arr

dist:
	rm -f *.o charconv char_arr
	cd .. ; tar cvf charconv.tar ./charconv ; compress charconv.tar ; \
	  cd charconv
