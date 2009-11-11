
/* $Id: log.c,v 1.13 2002/02/02 19:12:43 bsd Exp $ */

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
#include <syslog.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "comserv.h"
#include "log.h"


char * log_versionid = "$Id: log.c,v 1.13 2002/02/02 19:12:43 bsd Exp $";




int lwrite(int fd, char * buffer, int len)
{
  fd_set mw;
  fd_set w;
  int rc;
  struct timeval timeout;
  int nw;
  int n;
  
  FD_ZERO(&mw);
  FD_SET(fd, &mw);
  
  /* allow up to 0.25 seconds for the data to be written */
  timeout.tv_sec  = 0;
  timeout.tv_usec = 250000;
  
  nw = 0;
  n  = len;
  
  while (len) {
    bcopy(&mw, &w, sizeof(w));
    
  reselect:
    rc = select(fd+1, NULL, &w, NULL, &timeout);
    if (rc == 0) {
      /* timeout */
      msgout("lwrite(): timeout, %d bytes of %d lost\n",
             len, n);
      msgout("lwrite(): lost data = \"%s\"\n",
             &buffer[nw]);
      return nw;
    }
    else if (rc < 0) {
      if (errno == EINTR) {
        goto reselect;
      }
      else {
        return rc;
      }
    }
    
    rc = write(fd, &buffer[nw], len);
    if (rc < 0) {
      if (errno != EAGAIN) {
        /* write error */
        return rc;
      }
    }
    else {
      if (rc != len) {
        /* we wrote some data but not all of it */
        if (DEBUG_BUFFER()) {
          msgout("lwrite(): %d of %d bytes written\n", 
                 rc, len);
        }
      }
      nw  += rc;
      len -= rc;
    }
  }
  
  return nw;
}



#define FDPRINTF_MAX (64*1024)

static char __fdprintf_buffer[FDPRINTF_MAX];

int fdprintf(int fd, char * format, ...)
{
  va_list ap;
  char * buffer = __fdprintf_buffer;
  int rc;
  int fdstate;
  int nw;
  COMSERV_PORT * xp;
  
  va_start(ap, format);
  rc = vsnprintf(buffer, FDPRINTF_MAX, format, ap);
  va_end(ap);
  if (rc >= sizeof(__fdprintf_buffer)) {
    msgout("HELP: rc(%d) >= sizeof(__fdprintf_buffer)(%d)\n",
           rc, sizeof(buffer));
    abort();
  }
  
  nw = strlen(buffer);
  rc = lwrite(fd, buffer, nw);
  if (rc < 0) {
    switch (errno) {
      
      case EAGAIN:
        /* 
         * XXX should we even get this now that we are calling
         * lwrite() instead of write() directly? 
         */
        if (endpoint[fd]->fd == fd && endpoint[fd]->comserv) {
          msgout("'fdprintf' error writing to fd %d (%s): %s\n",
                 fd, endpoint[fd]->comserv->devid, 
                 strerror(errno));
        }
        else {
          msgout("'fdprintf' error writing to fd %d: %s\n",
                 fd, strerror(errno));
        }
        break;
        
      case EBADF:
        msgout(buffer);
        break;
        
      default:
        msgout("'fdprintf' error writing to fd %d: %s\n",
               fd, strerror(errno));
        fdstate = endpoint[fd]->state;
        xp      = endpoint[fd]->comserv;
        CLEANUP(endpoint[fd]);
        if (xp) {
          /* 
           *  drop and reconnect the peer side of the connection for
           *  proper hangup delivery 
           */
          reconnect(fdstate, xp, 1);
        }
        break;
    }
  }
  else if (rc != nw) {
    fprintf(stderr, "fdprintf(): only wrote %d of %d bytes\n", rc, nw);
  }
  
  return rc;
}


int msgout(char * format, ...)
{
  va_list ap;
  
  va_start(ap, format);
  
#if (CONSOLE == 0)
  vsyslog(LOG_ERR, format, ap);
#elif (CONSOLE == 1)
  vfprintf(stderr, format, ap);
  fflush(stderr);
#else
  invalid CONSOLE value
#endif
    
    va_end(ap);
  
  return 0;
}

