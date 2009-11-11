/* $Id: endp.h,v 1.19 2002/06/20 01:52:48 bsd Exp $ */

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

#ifndef __endp_h__
#define __endp_h__

#include <sys/types.h>

/*
 * need forward declarations
 */
struct comserv_port; typedef struct comserv_port COMSERV_PORT;
struct command_port; typedef struct command_port COMMAND_PORT;


#define IOBUF_LEN 512


/*
 * Endpoint data structure to manage multiple connections and varying
 * connection types.  And endpoint fd can be one of the following:
 *
 *   1) master side of a master-slave pty pair
 *   2) socket to terminal server serial port
 *   3) serial port device
 *   4) socket to remote user
 *
 * Endpoints are paired up with each other.  Data arriving on one
 * endpoint is sent to its peer.  Usually, types 1 & 2 are paired
 * together, and types 3 & 4 are paired together.
 */
typedef struct {
  int              fd;      /* fd associated with this endpoint */
  int              state;   /* endpoint task state */
  char             buf[IOBUF_LEN+5]; /* endpoint data buffer */
  int              bufcnt;         /* amount of data in buf */
  char *           bufptr;         /* pointer to data remaining in buf */
  COMSERV_PORT   * comserv; /* comserv port associated with this endpoint */
  COMMAND_PORT   * command; /* command data if this is a command connection */
} ENDPOINT;


extern ENDPOINT ** endpoint;
extern int         max_endpoint;


int        endp_init         (void);
void       init_endp         (ENDPOINT * e);
ENDPOINT * new_endp          (void);
int        endp_attr         (ENDPOINT * e);
void       cleanup           (ENDPOINT * r, fd_set * masterrd, 
                              fd_set * masterwr, int * maxfd);
int        sock_bind_service (char * service, int srvport);
int        lread             (int fd, char * buf, int len);
int        set_blocking      (ENDPOINT * e, int blocking, int doretry);
char     * whichside         (int state);
int        set_command       (ENDPOINT * e);
int        revert_command    (ENDPOINT * e);

#endif
