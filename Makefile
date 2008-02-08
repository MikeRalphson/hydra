CFLAGS=$(OPT) $(DEFINES)
OPT=-O


#Uncomment the following line if your system does not have strchr()
#DEFINES=-Dstrchr=index -Dstrrchr=rindex


#Uncomment the following lines to compile on SCO Unix
#OPT=-O2
#DEFINES=-DSCO
#CC=gcc
#LIBS=-lsocket


#Uncomment the following line on SunOS 4
#LIBS=-lresolv

#Uncomment the following on Solaris 2
#LIBS=-lsocket -lnsl -lresolv

#Uncomment the following lines on SVR4
#LIBS=-lsocket -lnsl


PACKOBJS = unixpk.o encode.o codes.o magic.o unixos.o string.o \
		xmalloc.o md5c.o getopt.o
UNPACKOBJS = unixunpk.o decode.o uudecode.o codes.o unixos.o string.o \
		part.o xmalloc.o md5c.o getopt.o

DESTDIR=/usr/local

all: mpack munpack

mpack: $(PACKOBJS)
	$(CC) $(CFLAGS) -o mpack $(PACKOBJS) $(LIBS)

munpack: $(UNPACKOBJS)
	$(CC) $(CFLAGS) -o munpack $(UNPACKOBJS) $(LIBS)

install: all
	-mkdir $(DESTDIR)/bin
	-mkdir $(DESTDIR)/man
	-mkdir $(DESTDIR)/man/man1
	install -s -m 755 mpack $(DESTDIR)/bin
	install -s -m 755 munpack $(DESTDIR)/bin
	install -m 644 unixpk.man $(DESTDIR)/man/man1/mpack.1
	install -m 644 unixunpk.man $(DESTDIR)/man/man1/munpack.1

clean:
	rm -f *.o mpack munpack

readme:
	./mkreadme.pl unix >README.unix
	./mkreadme.pl dos >README.dos
	./mkreadme.pl os2 >README.os2
	./mkreadme.pl mac >README.mac
	./mkreadme.pl amiga >README.amiga
	./mkreadme.pl arc archimedes >README.arc

l_pack:
	#load unixpk.c encode.c codes.c magic.c unixos.c string.c \
		xmalloc.c md5c.c getopt.c

l_unpack:
	#load unixunpk.c decode.c uudecode.c codes.c unixos.c string.c \
		xmalloc.c md5c.c getopt.c

depend:
	grep '^#[ 	]*include[ 	]*"' *.[ch] | \
	sed -e 's/:[^"]*"\([^"]*\)"/:	\1/' -e 's/\.c/.o/' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '' >> Makefile
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it


codes.o:	xmalloc.h md5.h
decode.o:	xmalloc.h common.h
dosos.o:	xmalloc.h common.h
dospk.o:	version.h xmalloc.h
dosunpk.o:	version.h
macmpack.o:	macnapp.h macmpack.h version.h
macnapp.o:	macnapp.h
macnclip.o:	macnapp.h
macndlog.o:	macnapp.h
macninit.o:	macnapp.h
macnte.o:	macnapp.h
macos.o:	common.h macnapp.h macmpack.h
macpcstr.o:	macnapp.h
md5c.o:	md5.h
unixos.o:	xmalloc.h common.h
unixpk.o:	common.h version.h xmalloc.h
unixunpk.o:	version.h
uudecode.o:	xmalloc.h common.h

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
