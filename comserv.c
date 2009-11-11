
/* $Id: comserv.c,v 1.27 2002/06/20 01:52:48 bsd Exp $ */

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


#include "comserv.h"
#include "log.h"
#include "pty.h"


char * comserv_versionid = "$Id: comserv.c,v 1.27 2002/06/20 01:52:48 bsd Exp $";

char config_file [ PATH_MAX ];


char * statestr(int state)
{
  switch(state) {
    case ST_START      : return "START"; break;
    case ST_INCOMING   : return "INCOMING"; break;
    case ST_CMDINPUT   : return "CMDINPUT"; break;
    case ST_REMOTE     : return "REMOTE"; break;
    case ST_LOCAL      : return "LOCAL"; break;
    case ST_CONNECTING : return "CONNECTING"; break;
    default            : return "<unknown>"; break;
  }
}


/*
 * sock_bind_service
 *
 * Initialize a connection and bind to the specified service.  
 */
#define SBS_MAXTRIES 30
int sock_bind_service(char * service, int srvport)
{
  int sockfd;
  int rc;
  struct servent *ps;
  struct sockaddr_in sin;
  short port;
  int tries=SBS_MAXTRIES;
  int on;
  
  if (service) {
    if (isdigit(service[0])) {
      port = (short)atol(service);
    }
    else {
      ps = getservbyname(service, "tcp");
      if (ps == NULL)  {
        msgout("cannot locate \"%s\" service, telnet port disabled\n", 
               service);
        return -1;
      }
      port = ntohs(ps->s_port);
    }
  }
  else {
    port = srvport;
  }
  
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)  {
    msgout("socket(): %s; telnet port disabled\n",
           strerror(errno));
    return -2;
  }

  on = 1;
  rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (rc < 0) {
    msgout("setsockopt(SO_RESUSEADDR): %s\n", strerror(errno));
  }

  rc = bind(sockfd, (struct sockaddr *)&sin, sizeof(sin));
  if (rc < 0)  {
    msgout("bind(): port=%d: %s\n", port, strerror(errno));
    close(sockfd);
    return -3;
  }

  rc = listen(sockfd, 20);
  if (rc < 0) {
    msgout("listen(): %s; telnet port disabled\n", strerror(errno));
    close(sockfd);
    return -4;
  }

  if (service)
    msgout("comservd listening on service \"%s\"\n", service);
  else
    msgout("comservd listening on port %d\n", srvport);

  return sockfd;
}



/*
 * sock_bind_connect
 *
 * Open a connection to the specified host and service.
 */
int sock_bind_connect(char * host, char * service, int srvport, int * ready)
{
  int sockfd;
  int rc;
  struct hostent * h;
  struct servent * ps;
  struct sockaddr_in lh; /* local host */
  short lp;              /* local port */
  struct sockaddr_in rh; /* remote host */
  short rp;              /* remote port */
  int ioc;

  *ready = 0;

  if (service) {
    if (isdigit(service[0])) {
      rp = (short)atol(service);
    }
    else {
      ps = getservbyname(service, "tcp");
      if (ps == NULL)  {
        msgout("cannot get %s service\n", service);
        exit (1);
      }
      rp = ntohs(ps->s_port);
    }
  }
  else {
    rp = srvport;
  }
  
  lp                 = 0; /* use an ephemeral port */
  lh.sin_family      = AF_INET;
  lh.sin_addr.s_addr = INADDR_ANY;
  lh.sin_port        = htons(lp);
  
  
  h = gethostbyname(host);
  if (h == NULL) {
    msgout("unknown host %s\n", host);
    return -1;
  }
  
  rh.sin_family      = AF_INET;
  rh.sin_addr.s_addr = *(unsigned int *)h->h_addr_list[0];
  rh.sin_port        = htons(rp);
  
  if (DEBUG_CONNECT()) {
    msgout("host %s is 0x%08x\n", host, rh.sin_addr.s_addr);
  }
  
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)  {
    msgout("socket(): %s\n", strerror(errno));
    return -2;
  }

  rc = bind(sockfd, (struct sockaddr *)&lh, sizeof(lh));
  if (rc < 0)  {
    msgout("bind(host=0x%08lx,port=%d): %s\n",
           ntohl(lh.sin_addr.s_addr), ntohs(lh.sin_port), 
           strerror(errno));
    close(sockfd);
    return -4;
  }

  /*
   * set the socket to non-blocking so the connect doesn't wait
   * forever if the terminal server is down.  
   */
  ioc = 1;
  rc = ioctl(sockfd, FIONBIO, &ioc);
  if (rc < 0) {
    msgout("ioctl(fd=%d, FIONBIO, 1): %s\n",
           sockfd, strerror(errno));
    close(sockfd);
    return -3;
  }

  *ready = 1;

  rc = connect(sockfd, (struct sockaddr *)&rh, sizeof(rh));
  if (rc < 0) {
    if (errno == EINPROGRESS) {
      *ready = 0;
    }
    else {
      if (DEBUG_CONNECT()) {
        msgout("connect(host=0x%08lx,port=%d): %s\n",
               ntohl(rh.sin_addr.s_addr), ntohs(rh.sin_port), 
               strerror(errno));
      }
      close(sockfd);
      return -5;
    }
  }
  
  return sockfd;
}



int connect_server(COMSERV_PORT * xp)
{
  int fd;
  int ready;
  
  /*
   * initialize REMOTE side of the connection
   */
  if (DEBUG_CONNECT()) {
    msgout("establishing connection to %s:%d (serial port %d)\n",
           xp->host, xp->port, xp->serport);
  }

  xp->re = NULL;

  fd = sock_bind_connect(xp->host, NULL, xp->port, &ready);
  if (fd < 0) {
    if (DEBUG_CONNECT()) {
      msgout("can't open connection to %s TCP/IP port %d "
             "(serial port %d)\n",
             xp->host, xp->port, xp->serport);
      }
    return -1;
  }
  
  if (fd >= FD_SETSIZE) {
    msgout("connect_server(): possible descriptor leak; "
           "fd=%d has exceeded FD_SETSIZE\n",
           fd);
    abort();
  }
  
  if (endpoint[fd] == NULL)
    endpoint[fd] = new_endp();
  else
    init_endp(endpoint[fd]);

  endpoint[fd]->fd            = fd;
  xp->re                      = endpoint[fd];
  endpoint[fd]->state         = ST_REMOTE;
  endpoint[fd]->comserv       = xp;
  if (DEBUG_BUFFER())
    msgout("connect_server(): enabling %s.rd\n",
           whichside(endpoint[fd]->state));
  ENABLE_RD(fd);

  /*
   * if the connection did not complete immediately, select the socket
   * for writability and adjust the state to reflect that it is in the
   * process of connecting 
   */
  if (!ready) {
    DISABLE_RD(fd);
    ENABLE_WR(fd);
    endpoint[fd]->state = ST_CONNECTING;
  }
  else {
    /* 
     * if we have a connection to the other side and it has data to
     * write to us, select ourselves for writeability
     */
    if (xp->le && xp->le->bufcnt) {
      if (DEBUG_BUFFER())
        msgout("connect_server(): enabling %s.wr\n",
               whichside(endpoint[fd]->state));
      ENABLE_WR(fd);
    }

    xp->reconnect_time_incr = 1;
    xp->reconnect_time = time(NULL) + xp->reconnect_time_incr;
  }

  maxfd = max(fd,maxfd);
  
  return fd;
}




int connect_user(COMSERV_PORT * xp, int doretry)
{
  int rc;
  int fd;
  int bank, unit;
  char tty [ 32 ];
  
  /*
   * initialize LOCAL side of the connection
   */
  
  if (DEBUG_CONNECT())
    msgout("configuring %s port %d on %s\n", 
           xp->host, xp->port, xp->localpath);

  if ((xp->rflags & RFLAG_LISTEN) == 0) {
    fd = allocate_pty(&bank, &unit);
    if (fd < 0) {
      msgout("out of ptys\n");
      exit(1);
    }
  
    strncpy(tty, "/dev/ttyp0", sizeof(tty)-1);
    tty[8] = bank;
    tty[9] = unit;
    unlink(xp->localpath);
    rc = symlink(tty, xp->localpath);
    if (rc < 0) {
      msgout("can't create symbolic link from %s to %s: %s\n",
             xp->localpath, tty, strerror(errno));
      exit(1);
    }
  }
  else {
    fd = open(xp->localpath, O_RDWR);
    if (fd < 0) {
      msgout("open(\"%s\"): %s\n", xp->localpath, strerror(errno));
      msgout("terminating\n");
      exit(1);
    }
  }

  /*
   * initialize the user side of the connection
   */
  
  if (fd >= FD_SETSIZE) {
    msgout("connect_user(): possible descriptor leak; "
           "fd=%d has exceeded FD_SETSIZE\n",
           fd);
    abort();
  }
  
  if (endpoint[fd] == NULL)
    endpoint[fd] = new_endp();
  else
    init_endp(endpoint[fd]);

  endpoint[fd]->fd            = fd;
  endpoint[fd]->state         = ST_LOCAL;
  endpoint[fd]->comserv       = xp;
  xp->le                      = endpoint[fd];
  set_blocking(endpoint[fd], 0, doretry);
  if (DEBUG_BUFFER())
    msgout("connect_user(): enabling %s.rd\n",
           whichside(endpoint[fd]->state));
  ENABLE_RD(fd);

  if (xp->rflags & RFLAG_LISTEN) {
    rc = endp_attr(endpoint[fd]);
    if (rc != 0) {
      msgout("connect_user(): endp_attr(): %s\n",
             strerror(-rc));
    }
  }

  /* 
   * if we have a connection to the other side and it has data to
   * write to use, select ourselves for writeability 
   */
  if (xp->re && xp->re->bufcnt) {
    msgout("connect_user(): enabling %s.wr\n",
           whichside(endpoint[fd]->state));
    ENABLE_WR(fd);
  }
  
  maxfd = max(fd,maxfd);
  
  if (xp->control) {
    rc = make_ctl_port(xp);
    if (rc < 0) {
      msgout("connect_user(): can't initialize control port, rc=%d\n", rc);
      CLEANUP(xp->le);
      return -1;
    }
  }
  
  return fd;
}





void reconnect(int state, COMSERV_PORT * xp, int dopeer)
{
  int fd;
  int peer;
  
  if (DEBUG_CONNECT()) {
    msgout("re-establishing %s side of %s\n", 
           whichside(state), xp->localpath);
  }
  
  switch (state) {
    case ST_LOCAL    :
    case ST_CMDINPUT :
      fd = connect_user(xp, 1);
      if (fd < 0) {
        msgout("can't establish user connection to %s\n", xp->localpath);
        break;
      }
      else {
        if (dopeer && !xp->control) {
          /* the control port doesn't have a peer connection */
          if (xp->re) {
            peer = xp->re->state;
            CLEANUP(xp->re);
          }
          reconnect(ST_REMOTE, xp, 0);
        }
      }
      break;
      
    case ST_REMOTE :
      if ((xp->rflags & RFLAG_LISTEN) == 0) {
        /* try to connect right now */
        fd = connect_server(xp);
        if (fd) {
          if (dopeer) {
            /* 
             * only attempt a reconnect if the local side was not
             * already connected 
             */
            if (!xp->le) {
              reconnect(ST_LOCAL, xp, 0);
            }
          }
        }
      }
      break;
      
    default :
      msgout("reconnect(): invalid state=%d\n", state);
      exit(1);
      break;
  }
}



int make_ctl_port(COMSERV_PORT * xp)
{
  int rc;
  
  /* set this if not already set */
  xp->control = 1;
  
  rc = set_command(xp->le);
  if (rc)
    return -1;
  
  xp->flags             = OPT_WAIT;
  xp->re                = NULL;
  xp->le->state         = ST_CMDINPUT;
  xp->le->command->echo = 1;
  xp->le->command->h    = NULL;
  snprintf(xp->le->command->name, MAX_HOST, "control");
  
  return 0;
}


COMSERV_PORT * new_comserv_port(void)
{
  COMSERV_PORT * xp;
  
  xp = (COMSERV_PORT *) malloc(sizeof(COMSERV_PORT));
  if (xp == NULL) {
    msgout("out of memory allocating COMSERV_PORT\n");
    exit(1);
  }
  
  bzero(xp, sizeof(*xp));
  
  return xp;
}



void free_comserv_port(COMSERV_PORT * xp)
{
  COMSERV_PORT * xp1, * xp2;

  /*
   * unlink xp from the list of device nodes
   */
  xp1 = comserv_ports;
  xp2 = NULL;
  while (xp1) {
    if (xp1 == xp) {
      if (xp2) {
        /* not first in the list */
        xp2->next = xp1->next;
      }
      else {
        /* first in the list */
        comserv_ports = xp1->next;
      }
      break;
    }
    xp2 = xp1;
    xp1 = xp1->next;
  }

  if (xp->le) {
    CLEANUP(xp->le);
  }
  if (xp->re) {
    CLEANUP(xp->re);
  }
  if (xp->listen) {
    CLEANUP(xp->listen);
  }

  if (xp->log)
    close(xp->logfd);

  free(xp);
}

