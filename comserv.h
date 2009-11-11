
/* $Id: comserv.h,v 1.22 2002/06/20 01:52:48 bsd Exp $ */

/*
 * Copyright 2000, 2001, 2002 Brian S. Dean <bsd@bsdhome.com>
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN S. DEAN ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL BRIAN S. DEAN BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 */

#ifndef __comserv_h__
#define __comserv_h__

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#include "cmd.h"


/* 
 * debugging options, these are applied to the global 'debug_flags' to
 * determine which debug messages should be output 
 */
#define D_BUFFER  0x0001
#define D_STATES  0x0002
#define D_CONNECT 0x0004
#define D_CMDS    0x0008

#define DEBUG_BUFFER()  (debug_flags & D_BUFFER)
#define DEBUG_STATES()  (debug_flags & D_STATES)
#define DEBUG_CONNECT() (debug_flags & D_CONNECT)
#define DEBUG_CMDS()    (debug_flags & D_CMDS)

extern unsigned int debug_flags;


/*
 * ENABLE_TELNET_PORT allows incoming telnet connects to issue local
 * mode commands.  This is dangerous and should only be used if you
 * are in a secure environment or for debugging purposes.
 */

#if ENABLE_TELNET_PORT

#define COMSERV "comserv"  /* /etc/services entry for incoming telnet
                              command connections */
#endif


#define DEFAULT_CONFIG "/usr/local/etc/comservd.conf" /* default
                                                         config file
                                                         name */

#define DEFAULT_LOGDIR "/usr/local/comserv/log" /* default directory
                                                   for session log
                                                   files */

#define DEFAULT_DEVDIR "/usr/local/comserv/dev" /* default directory
                                                   for device symlinks */


#define MAX_HOST     128  /* max hostname length */

#define MAX_DEVID     64  /* max device id length  */

#define MAX_SET      512  /* maximum var=value text length for the
                             'set' command */


#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif



/*
 * states for the state machine, also serves as an identification of
 * the connection type
 */
enum {
  ST_START,
  ST_INCOMING,
  ST_SERVICE,
  ST_CMDINPUT,
  ST_LOCAL,
  ST_REMOTE,
  ST_CONNECTING
};


/*
 * Bit masks for modifying various operational behaviours of struct
 * compserv_port instances.  These apply to the 'flags' field.  
 */
#define OPT_WAIT    0x0001 /* 1=don't connect to remote side until
                              data is available on the local side */
#define OPT_LOGALL  0x0002 /* if logging is enabled, 1=log data from
                              both sides of the connection; 0=log data
                              only from the remote side */
#define OPT_LOGHEX  0x0004 /* if logging is enabled, 1=output log data
                              in hex and ascii format and identify
                              data coming from the remote vs the local
                              side of the connection; 0=display data
                              in ascii only and don't differentiate
                              the source */
#define OPT_BLOCK  0x0008  /* 1=block the source if the receiver is
                              not ready; 0=data is lost if the
                              receiver is not ready, no data is lost
                              from the log file */

#define OPT_DEFAULT (OPT_BLOCK) /* default options */

#define RFLAG_LISTEN 0x00001 /* remote endpoint is served via a listen
                                socket; the port is the TCP port
                                number of the listen socket and
                                localpath[] is the name of the /dev
                                serial port being served */

#define RECONNECT_TIME_MAX (10*60) /* every 10 minutes */

/*
 * Data structure for associated pathnames with terminal server ports 
 */
struct comserv_port {
  COMSERV_PORT * next;                      /* next in linked list */
  char           devid[MAX_DEVID];          /* device id for this connection */
  char           localpath [ PATH_MAX ]; /* path name to user connection */
  char           host [ MAX_HOST ];         /* terminal server host name */
  int            log;                       /* boolean: log or not */
  char           logfile [ PATH_MAX ];      /* name of this port's log file */
  int            logfd;                     /* log file fd */
  int            serport;                   /* TS serial port number */
  int            port;                      /* TS TCP/IP port */
  int            control;                   /* 1=control connection, 0=not */
  ENDPOINT     * le;                        /* local connection endpoint */
  unsigned int   flags; /* options flags, see OPT_* #defines */
  unsigned int   rflags;/* flags specific to the remote endpoint */
  ENDPOINT     * re;    /* remote connection endpoint */
  ENDPOINT     * listen;/* listen socket when serving local device */
  unsigned long  n_le;  /* N bytes received from local endpoint */
  unsigned long  n_re;  /* N bytes received from remote endpoint */
  time_t         reconnect_time;
  int            reconnect_time_incr;
};


#define ENABLE_RD(fd)    FD_SET(fd, &master_rd)
#define ISENABLED_RD(fd) FD_ISSET(fd, &master_rd)
#define ENABLE_WR(fd)    FD_SET(fd, &master_wr)
#define ISENABLED_WR(fd) FD_ISSET(fd, &master_wr)
#define DISABLE_RD(fd)   FD_CLR(fd, &master_rd)
#define DISABLE_WR(fd)   FD_CLR(fd, &master_wr)

extern COMSERV_PORT * comserv_ports;   /* linked list of defined terminal
                                          server ports that we know about */

extern COMSERV_PORT global; /* default settings for new comserv ports */

extern int maxfd;                    /* max file descriptor currently in use */

extern fd_set master_rd;             /* the master set of file descriptor
                                 bits that are selected for
                                 readability */
extern fd_set master_wr;             /* the master set of file descriptor
                                 bits that are selected for
                                 writeability */

extern int config_fd; /* file descriptor for the configuration file */

extern char * progname;

extern char config_file[]; /* name of the config file */


extern int verbose;

#ifdef ENABLE_TELNET_PORT
extern int    lsock;          /* listening socket, listens for incoming
                                 command connections */
#endif




#define CLEANUP(remote) cleanup(remote,&master_rd,&master_wr,&maxfd)


char         * statestr          (int state);
int            sock_bind_service (char * service, int srvport);
int            sock_bind_connect (char * host, char * service, int srvport, 
                                  int * ready);
int            lread             (int fd, char * buf, int len);
int            connect_server    (COMSERV_PORT * xp);
int            connect_user      (COMSERV_PORT * xp, int doretry);
void           reconnect         (int state, COMSERV_PORT * xp, int dopeer);
int            make_ctl_port     (COMSERV_PORT * xp);
COMSERV_PORT * new_comserv_port  (void);
void           free_comserv_port (COMSERV_PORT * xp);

#endif

