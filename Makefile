#------------------------------------------------------------------------
#
# $Id: Makefile,v 1.35 2002/05/15 03:58:49 bsd Exp $
#
# Makefile
#

TARGET         = comservd

USE_LIBEFENCE  = NO
USE_DMALLOC    = NO

PREFIX         = /usr/local
BINDIR         = ${PREFIX}/sbin
CONFDIR        = ${PREFIX}/etc
RCDIR          = ${PREFIX}/etc/rc.d
MANDIR         = ${PREFIX}/man/man8
CONFIGFILE     = comservd.conf.sample
RCFILE         = comservd.sh.sample
MANUAL         = comservd.8

INSTALL        = /usr/bin/install -O root -G system

#
# Enabling -DENABLE_TELNET_PORT will cause comservd to accept incoming
# telnet connections and allow input to its local command mode interface.
# This is primarily intended for debugging and should not be enabled
# on a production system.  If you enable this for debugging purposes, you
# probably don't want to strip the binary at install time, so remove "-s"
# from INSTALL_PROGRAM below as well.
#
CC = gcc
CFLAGS = -g
#CFLAGS = -g -Wall -DENABLE_TELNET_PORT
#

INSTALL_PROGRAM = ${INSTALL} -M 555 -S
INSTALL_DATA    = ${INSTALL} -M 444
INSTALL_SCRIPT  = ${INSTALL} -M 555
INSTALL_MANUAL  = ${INSTALL_DATA}


include Makefile.inc

all :
	make depend
	make $(TARGET)

clean :
	rm -f *~ *.o *.core $(TARGET)

install : ${BINDIR}/$(TARGET)		\
	  ${CONFDIR}/${CONFIGFILE}	\
	  ${RCDIR}/${RCFILE}		\
	  ${MANDIR}/${MANUAL}.gz

$(TARGET) : $(OBJS) $(EXTRA_DEPS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBDIRS) $(LIBS)

${BINDIR}/$(TARGET) : $(TARGET)
	${INSTALL_PROGRAM} $(TARGET) ${BINDIR}

${CONFDIR}/${CONFIGFILE} : ${CONFIGFILE}
	${INSTALL_DATA} ${CONFIGFILE} ${CONFDIR}

${RCDIR}/${RCFILE} : ${RCFILE}
	${INSTALL_SCRIPT} ${RCFILE} ${RCDIR}

${MANDIR}/${MANUAL}.gz : ${MANUAL}
	${INSTALL_MANUAL} ${MANUAL} ${MANDIR}
	gzip -f ${MANDIR}/${MANUAL}

depend : .depend

