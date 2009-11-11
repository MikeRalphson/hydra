
/* $Id: endp.c,v 1.27 2002/05/15 02:05:36 bsd Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <termios.h>
#include <unistd.h>

#include "comserv.h"
#include "cmd.h"
#include "endp.h"
#include "log.h"
#include "pty.h"


char * endp_versionid = "$Id: endp.c,v 1.27 2002/05/15 02:05:36 bsd Exp $";


ENDPOINT ** endpoint     = NULL;
int         max_endpoint = 0;


int endp_init(void)
{
  int maxfiles;
  int i;
  
  maxfiles = FD_SETSIZE;
  
  endpoint = (ENDPOINT **)malloc(sizeof(ENDPOINT *)*maxfiles);
  if (endpoint == NULL) {
    msgout("endp_init(): can't alloc %d bytes of memory for endpoint array\n",
           sizeof(ENDPOINT)*maxfiles);
    return -2;
  }
  
  max_endpoint = maxfiles;
  
  for (i=0; i<max_endpoint; i++) {
    endpoint[i] = NULL;
  }
  
  return maxfiles;
}


void init_endp(ENDPOINT * e)
{
  memset(e, 0, sizeof(*e));
  e->state  = ST_START;
  e->fd     = -1;
  e->bufcnt = 0;
  e->bufptr = e->buf;

  return;
}



ENDPOINT * new_endp(void)
{
  ENDPOINT * e;
  static int ne = 0;
  
  ne++;
  
  e = (ENDPOINT *)malloc(sizeof(ENDPOINT));
  if (e == NULL) {
    msgout("out of memory allocating the endpoint no. %d\n", ne);
    exit(1);
  }

  init_endp(e);
  
  return e;
}



int endp_attr(ENDPOINT * e)
{
  int rc;
  struct termios termios;
  int err;

  if (!isatty(e->fd))
    return 0;

  /*
   * initialize terminal modes
   */
  rc = tcgetattr(e->fd, &termios);
  if (rc < 0) {
    err = errno;
    msgout("tcgetattr() failed, %s",
           strerror(err));
    return -err;
  }

  termios.c_iflag = 0;
  termios.c_oflag = 0;
  termios.c_cflag &= ~ (PARENB | CSIZE | CSTOPB);
  termios.c_cflag |=   (CS8 | HUPCL | CREAD | CLOCAL);
  termios.c_lflag = 0;

  termios.c_cc[VEOF] = _POSIX_VDISABLE;
#if 1
  termios.c_cc[VEOL] = _POSIX_VDISABLE;
#endif
  termios.c_cc[VEOL2] = _POSIX_VDISABLE;
  termios.c_cc[VERASE] = _POSIX_VDISABLE;
  termios.c_cc[VWERASE] = _POSIX_VDISABLE;
  termios.c_cc[VKILL] = _POSIX_VDISABLE;
  termios.c_cc[VREPRINT] = _POSIX_VDISABLE;
  termios.c_cc[VINTR] = _POSIX_VDISABLE;
  termios.c_cc[VQUIT] = _POSIX_VDISABLE;
  termios.c_cc[VSUSP] = _POSIX_VDISABLE;
  termios.c_cc[VDSUSP] = _POSIX_VDISABLE;
  termios.c_cc[VSTART] = _POSIX_VDISABLE;
  termios.c_cc[VSTOP] = _POSIX_VDISABLE;
  termios.c_cc[VLNEXT] = _POSIX_VDISABLE;
  termios.c_cc[VDISCARD] = _POSIX_VDISABLE;
  termios.c_cc[VMIN] = _POSIX_VDISABLE;
  termios.c_cc[VTIME] = _POSIX_VDISABLE;
  termios.c_cc[VSTATUS] = _POSIX_VDISABLE;
#if 0
  termios.c_cc[VMIN]  = 1;
  termios.c_cc[VTIME] = 0;
#endif

  rc = tcsetattr(e->fd, TCSANOW, &termios);
  if (rc < 0) {
    err = errno;
    msgout("tcsetattr() failed, %s",
           strerror(err) );
    return -err;
  }

  return 0;
}


void cleanup(ENDPOINT * r, fd_set * masterrd, fd_set * masterwr, 
             int * maxfd)
{
  if (r == NULL)
    return;

  if (r->fd > 0) {
    close (r->fd);
    if (r->fd == config_fd)
      config_fd = -1;
    FD_CLR(r->fd,masterrd);
    FD_CLR(r->fd,masterwr);
    if (r->fd == *maxfd) {
      *maxfd = *maxfd - 1;
    }
  }

  if (r->command != NULL) {
    if (r->command->buf != NULL) {
      free(r->command->buf);
    }
    free(r->command);
    r->command = NULL;
  }
  
  if (r->comserv != NULL) {
    if (r == r->comserv->le) {
      unlink(r->comserv->localpath);
      r->comserv->le = NULL;
    }
    else if (r == r->comserv->re) {
      r->comserv->re = NULL;
    }
  }

  bzero(r,sizeof(*r));
  
  r->fd     = -1;
  r->state  = ST_START;
  r->bufcnt = 0;
  r->bufptr = r->buf;
}



int set_blocking(ENDPOINT * e, int blocking, int doretry)
{
  int rc;
  int flags;
  
  rc = fcntl(e->fd, F_GETFL);
  if (rc == -1) {
    msgout("WARNING: can't get fd %d flags: %s\n", e->fd, strerror(errno));
    return -1;
  }
  
  flags = rc;
  
  if (blocking)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;
  
  rc = fcntl(e->fd, F_SETFL, flags);
  if (rc)
    return -1;
  else
    return 0;
  
  return 0;
}


char * whichside(int state)
{
  switch (state) {
    case ST_LOCAL    : return "local"; break;
    case ST_CMDINPUT : return "local"; break;
    case ST_REMOTE   : return "remote"; break;
    default          : return "unknown"; break;
  }
}



int set_command(ENDPOINT * e)
{
  char * bp;
  
  bp = (char *) malloc(MAX_RBUF);
  if (bp == NULL) {
    msgout("out of memory allocating local mode command buffer\n");
    return -1;
  }
  
  e->command = new_cmd();
  e->command->buf = bp;
  snprintf(e->command->prompt, MAX_PROMPT, "%s> ", progname);
  e->command->echo = 0;
  e->command->lastc[1] = '\r';
  e->command->lastc[0] = '\n';
  
  return 0;
}



