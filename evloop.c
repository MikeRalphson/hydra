/* $Id: evloop.c,v 1.41 2002/06/20 01:52:48 bsd Exp $ */

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
#include <string.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>

#include "cmd.h"
#include "comserv.h"
#include "evloop.h"
#include "log.h"


#define CTRL(x) (x & 0x1f)


char * evloop_versionid = "$Id: evloop.c,v 1.41 2002/06/20 01:52:48 bsd Exp $";

extern volatile int hupped;


#define PRINTSTATE(s) \
  if (DEBUG_STATES()) { \
    msgout("fd=%d, %s/%s, devid=\"%s\", state=%d=%s\n", \
           s, \
           FD_ISSET(s, &rd) ? "RD" : "!rd", \
           FD_ISSET(s, &wr) ? "WR" : "!wr", \
           endpoint[s]->comserv->devid, \
           endpoint[s]->state, \
           statestr(endpoint[s]->state)); \
  }



int event_loop(void)
{
  int rc;
  int sock;
  int nfds;
  int newsock, newaddrlen;
  struct sockaddr_in newaddr;
  int rsock;
  struct timeval timeout;
  int timedout;
  COMSERV_PORT * xp;
  fd_set rd;
  fd_set wr;
  int loopcnt, loopmax = INT_MAX;

  FD_ZERO(&master_rd);
  FD_ZERO(&master_wr);
  
#ifdef ENABLE_TELNET_PORT
  if (endpoint[lsock] == NULL)
    endpoint[lsock] = new_endp();
  endpoint[lsock]->state   = ST_INCOMING;
  endpoint[lsock]->fd      = lsock;
  endpoint[lsock]->command = NULL;
  endpoint[lsock]->comserv = NULL;
  ENABLE_RD(lsock);
  maxfd = max(maxfd,lsock);
#endif
  
  /*
   * Open the configuration file for input, and prep it's endpoint as
   * if it were in command input mode.  Then, when we enter the
   * select() loop below, the configuration commands will be read and
   * processes as if it were associated with a live connection.  When
   * the config file has been read and processed, we'll see EOF, and
   * it will be closed through the normal mechanism
   */
  config_fd = open(config_file, O_RDONLY);
  if (config_fd >= 0) {
    if (endpoint[config_fd] == NULL)
      endpoint[config_fd] = new_endp();
    else
      init_endp(endpoint[config_fd]);
    endpoint[config_fd]->state = ST_CMDINPUT;
    endpoint[config_fd]->fd    = config_fd;
    set_command(endpoint[config_fd]);
    endpoint[config_fd]->command->h = NULL;
    snprintf(endpoint[config_fd]->command->name, MAX_HOST, "comserv init");
    ENABLE_RD(config_fd);
    maxfd = max(maxfd, config_fd);
  }
  else {
    msgout("WARNING: can't open config file \"%s\": %s\n",
           config_file, strerror(errno));
  }
  
  loopcnt = 0;

  timeout.tv_sec  = 0;
  timeout.tv_usec = 300000;  /* 0.3 second timeout */

  while (1) {

  reselect:

    bcopy(&master_rd, &rd, sizeof(master_rd));
    bcopy(&master_wr, &wr, sizeof(master_wr));

    timedout = 0;

    rc = select(maxfd+1, &rd, &wr, NULL, &timeout);
    
    if (hupped) {
      /*
       * we've been sent a HUP signal, restart
       */
      cmd_restart(NULL, NULL);
    }
    
    if (rc < 0) {
      if (errno == EINTR) {
        goto reselect;
      }
      else {
        msgout("select() errno=%d: %s\n", errno, strerror(errno));
        exit(1);
      }
    }

    nfds = rc;
    if (nfds == 0)
      timedout = 1;

    loopcnt += nfds;

    for (sock=0; nfds && (sock<FD_SETSIZE); sock++) {
      rsock = sock;
      
      if (FD_ISSET(sock, &rd)) {
        nfds--;
        
        if (FD_ISSET(sock, &wr)) {
          msgout("fd %d is both readable and writable\n", sock);
        }

        switch(endpoint[rsock]->state) {
          case ST_START:
            PRINTSTATE(rsock);
            msgout("unexpected activity on fd %d, shutting it down\n",
                   rsock);
            CLEANUP(endpoint[rsock]);
            break;

          case ST_SERVICE: {
            unsigned long a;
            struct hostent * h;
            PRINTSTATE(rsock);
            newaddrlen = sizeof(newaddr);
            newsock = accept(endpoint[rsock]->fd, 
                             (struct sockaddr *)&newaddr,
                             &newaddrlen);
            if (newsock < 0) {
              msgout("accept(): %s\n", strerror(errno));
              break;
            }
            if (endpoint[newsock] == NULL)
              endpoint[newsock] = new_endp();
            if (endpoint[newsock]->state != ST_START) {
              msgout("unexpected start state %d for new "
                     "connection on fd %d; shutting it down\n", 
                     endpoint[newsock]->state, newsock);
              break;
            }

            xp = endpoint[rsock]->comserv;
            
            memset(endpoint[newsock], 0, sizeof(*endpoint[newsock]));
            endpoint[newsock]->fd = newsock;
            endpoint[newsock]->state = ST_REMOTE;
            xp->re = endpoint[newsock];
            endpoint[newsock]->comserv = xp;
            if (DEBUG_BUFFER())
              msgout("evloop(): enabling %s.rd\n",
                     whichside(endpoint[newsock]->state));
            ENABLE_RD(newsock);

            /* 
             * if we have a connection to the other side and it has data to
             * write to us, select ourselves for writeability
             */
            if (xp->le && xp->le->bufcnt) {
              if (DEBUG_BUFFER())
                msgout("evloop(): enabling %s.wr\n",
                       whichside(endpoint[newsock]->state));
              ENABLE_WR(newsock);
            }
            
            maxfd = max(newsock, maxfd);
            
            a = newaddr.sin_addr.s_addr;
            h = gethostbyaddr((char *)&a, sizeof(a), AF_INET);
            if (h == NULL) {
              a = ntohl(a);
              snprintf(xp->host, MAX_HOST, "%ld.%ld.%ld.%ld",
                       (a >> 24) & 0x0ff, 
                       (a >> 16) & 0x0ff, 
                       (a >> 8) & 0x0ff,
                       a & 0x0ff);
              msgout("host lookup failure for address %s: %s\n",
                     xp->host, hstrerror(h_errno));
            }
            else {
              strncpy(xp->host, h->h_name, MAX_HOST-1);
              xp->host[sizeof(xp->host)-1] = 0;
            }
            msgout("incoming service request from %s\n",
                   xp->host);
          }
          break;

          case ST_INCOMING: {
            unsigned long a;
            PRINTSTATE(rsock);
            newaddrlen = sizeof(newaddr);
            newsock = accept(endpoint[rsock]->fd, 
                             (struct sockaddr *)&newaddr,
                             &newaddrlen);
            if (newsock < 0) {
              msgout("accept(): %s\n", strerror(errno));
              break;
            }
            if (endpoint[newsock] == NULL)
              endpoint[newsock] = new_endp();
            if (endpoint[newsock]->state != ST_START) {
              msgout("unexpected start state %d for new "
                     "connection on fd %d; shutting it down\n", 
                     endpoint[newsock]->state, newsock);
              break;
            }
            
            memset(endpoint[newsock], 0, sizeof(*endpoint[newsock]));
            endpoint[newsock]->fd = newsock;
            endpoint[newsock]->state = ST_CMDINPUT;
            rc = set_command(endpoint[newsock]);
            if (rc) {
              msgout("unable to enter command mode\n");
              CLEANUP(endpoint[newsock]);
            }
            ENABLE_RD(newsock);
            maxfd = max(newsock, maxfd);
            a = newaddr.sin_addr.s_addr;
            endpoint[newsock]->command->h = gethostbyaddr((char *)&a,
                                                          sizeof(a), AF_INET);
            if (endpoint[newsock]->command->h == NULL) {
              a = ntohl(a);
              snprintf(endpoint[newsock]->command->name, MAX_HOST,
                       "%ld.%ld.%ld.%ld",
                       (a >> 24) & 0x0ff, 
                       (a >> 16) & 0x0ff, 
                       (a >> 8) & 0x0ff,
                       a & 0x0ff);
              msgout("host lookup failure for address %s: %s\n",
                     endpoint[newsock]->command->name, hstrerror(h_errno));
            }
            else {
              strncpy(endpoint[newsock]->command->name, 
                      endpoint[newsock]->command->h->h_name, MAX_HOST-1);
            }
            msgout("incoming connection from %s\n",
                   endpoint[newsock]->command->name);
            fdprintf(newsock, "\nwelcome, %s\n", 
                     endpoint[newsock]->command->name);
            fdprintf(newsock, "%s", endpoint[newsock]->command->prompt);
          }
          break;

          case ST_CMDINPUT: {
            char ch;
            char lastc[2];
            ENDPOINT * e;
            PRINTSTATE(rsock);
            e = endpoint[rsock];

            rc = read(rsock, &ch, 1);
            if (rc < 0) {
              msgout("read() error on fd %d: %s\n", e->fd, strerror(errno));
              CLEANUP(e);
              break;
            }
            else if (rc == 0) {
              COMSERV_PORT * xp;
              int state;
              
              state = e->state;
              xp    = e->comserv;
              
              CLEANUP(e);
              
              if (xp)
                reconnect(state, xp, 1);
              
              break;
            }
            
            lastc[1] = e->command->lastc[1];
            lastc[0] = e->command->lastc[0];
            e->command->lastc[1] = e->command->lastc[0];
            e->command->lastc[0] = ch;

#if 0
            msgout("> 0x%02x", (unsigned)ch);
#endif
            
            switch(ch) {
              case '\r':
              case '\n':
                
                if ((lastc[1] == '\r') && (lastc[0] == '\n') && (ch == '\r')) {
                  break;
                }
                
                if (e->command->echo) {
                  rc = fdprintf(rsock, "\n");
                  if (rc <0)
                    break;
                }
                
                e->command->buf[e->command->bufcnt] = 0;
                
                if (e->command->buf[0] != 0) {
                  if (DEBUG_CMDS()) 
                    msgout("command=[%s]\n", e->command->buf);
                  rc = run_command(e);
                }
                
                /* 
                 * check to make sure that the connection is still
                 * there, i.e., the last command could have been a
                 * 'quit' command which would have terminated this
                 * connection 
                 */
                if (e->command == NULL)
                  break;
                
                /* reset command buffer */
                e->command->bufcnt = 0;
                e->command->buf[0] = 0;
                if (rsock != config_fd)
                  fdprintf(rsock, "%s", e->command->prompt);
                break;
                
              default:
                  switch (ch) {
                    case '\b':
                      if (e->command->bufcnt) {
                        e->command->buf[--e->command->bufcnt] = 0;
                        if (e->command->echo)
                          fdprintf(rsock, "\b \b");
                      }
                      break;
                    default:
                      if (e->command->bufcnt < MAX_RBUF) {
                        e->command->buf[e->command->bufcnt++] = ch;
                        if (e->command->echo)
                          fdprintf(rsock, "%c", ch);
                      }
                      else {
                        fdprintf(rsock, "command too long\n");
                        e->command->bufcnt = 0;
                        e->command->buf[0] = 0;
                        fdprintf(rsock, "%s", e->command->prompt);
                      }
                      break;
                  }
                  break;
                  
            }
            
          }
          
          break;
          
          case ST_LOCAL:
          case ST_REMOTE:
            PRINTSTATE(rsock);
            rc = data_ready(endpoint[rsock]);
            if (DEBUG_BUFFER()) 
              msgout("data_ready()=%d\n", rc);
            break;
            
          default :
            PRINTSTATE(rsock);
            msgout("invalid state %d for task fd %d\n",
                   endpoint[rsock]->state, rsock);
            CLEANUP(endpoint[rsock]);
            break;
            
        } /* switch */
        
      } /* if (FD_ISSET()) */
      
      
      
      if (FD_ISSET(sock, &wr)) {
        switch (endpoint[rsock]->state) {
          case ST_LOCAL:
          case ST_REMOTE:
            PRINTSTATE(rsock);
            rc = data_writeable(endpoint[rsock]);
            if (DEBUG_BUFFER()) {
              msgout("data_writeable(): rc=%d\n", rc);
            }
            break;
            
          case ST_CONNECTING:
            PRINTSTATE(rsock);
            /*
             * connection established, next state is ST_REMOTE
             */
            endpoint[rsock]->state = ST_REMOTE;
            ENABLE_RD(rsock);
            DISABLE_WR(rsock);
            /* 
             * if we have a connection to the other side and it has data to
             * write to us, select ourselves for writeability
             */
            if (endpoint[rsock]->comserv->le && 
                endpoint[rsock]->comserv->le->bufcnt) {
              if (DEBUG_BUFFER())
                msgout("evloop(): enabling %s.wr\n",
                       whichside(endpoint[rsock]->state));
              ENABLE_WR(rsock);
            }
            break;
            
          default:
            PRINTSTATE(rsock);
            msgout("unexpected state=%s for writeable fd, "
                   "disabling socket %d.wr\n",
                   statestr(endpoint[rsock]->state), rsock);
            DISABLE_WR(rsock);
            break;
        }
      }


    } /* for (sock=0;;sock++) ... */


    if (timedout || (loopcnt > loopmax)) {
      time_t now;
      loopcnt = 0;
      /*
       * look for any unconnected local or remote endpoints that need
       * a connect, and initiate a connection
       */
      xp = comserv_ports;
      now = time(NULL);
      while (xp) {
        if ((xp->re == NULL) && ((xp->flags & OPT_WAIT) == 0) &&
            ((xp->rflags & RFLAG_LISTEN) == 0) && 
            (now >= xp->reconnect_time)) {
          if (DEBUG_STATES())
            msgout("evloop(): reconnect(remote, %s)\n", xp->devid);

          /* 
           * Some old Xyplexes crash if you try to connect to them too
           * quickly in succession.  This is a hackish attempt to
           * throttle those back a little 
           */
          usleep(300000);

          reconnect(ST_REMOTE, xp, 0);

          xp->reconnect_time_incr = xp->reconnect_time_incr * 2;
          if (xp->reconnect_time_incr > RECONNECT_TIME_MAX)
            xp->reconnect_time_incr = RECONNECT_TIME_MAX;
          xp->reconnect_time = time(NULL) + xp->reconnect_time_incr;

          /* if it fails, we try again next time */
        }
        
        if (xp->le == NULL) {
          if (DEBUG_STATES())
            msgout("evloop(): reconnect(local, %s)\n", xp->devid);
          reconnect(ST_LOCAL, xp, 0);
          /* if it fails, we try again next time */
        }
        
        xp = xp->next;
      }
    }
    
    /*
     * When the config file is closed, set loopmax down to a small
     * value.  It starts out very large so that we delay connecting to
     * any endpoints until the config file is completely read and
     * processed.  This allows all options to be applied to device ids
     * before their connections are established.
     */
    if (config_fd == -1) {
      loopmax = 20;
    }


  } /* while (1) */
  
}


int data_ready(ENDPOINT * from)
{
  int rc;
  int logit;
  ENDPOINT * to;
  COMSERV_PORT * xp;

  xp = from->comserv;
  
  switch (from->state) {
    case ST_LOCAL:
    case ST_REMOTE:
      break;
    default:
      msgout("data_ready(): HELP! from->state=%d=%s\n",
             from->state, statestr(from->state));
      abort();
      break;
  }
  
  if (from->state == ST_LOCAL)
    to = xp->re;
  else
    to = xp->le;

  /* sanity check */
  if (to) {
    assert(xp == to->comserv);
  }

  rc = read(from->fd, from->buf, IOBUF_LEN);
  if (rc < 0) {
    if (errno == EAGAIN) {
      /* de-select for read and return */
      if (DEBUG_BUFFER())
        msgout("data_ready(): read=EAGAIN; disabling %s.rd\n",
               whichside(from->state));
      DISABLE_RD(from->fd);
      return -1;
    }
    else {
      /* system read error */
      msgout("data_ready(): device \"%s\" %s side read() error: %s\n",
             xp->devid, whichside(from->state), strerror(errno));
      CLEANUP(from);
#if 0
      CLEANUP(to);
#endif
      return -2;
    }
  }
  else if (rc == 0) {
    /* EOF - connection has been closed */
    if (verbose)
      msgout("data_ready(): read()=0, device \"%s\" %s side disconnected\n",
             xp->devid, whichside(from->state));
#if 0
    if (from->state == ST_LOCAL) {
      CLEANUP(to);
    }
#endif
    CLEANUP(from);
    return 0;
  }

  /* look for break character */
  if ((rc == 1) && (from->buf[0] == CTRL('b'))) {
    from->buf[0] = 0xff;
    from->buf[1] = 0xf3;
    rc = 2;
  }

  from->bufcnt = rc;
  from->bufptr = from->buf;
  if (from->state == ST_LOCAL)
    xp->n_le += from->bufcnt;
  else if (from->state == ST_REMOTE)
    xp->n_re += from->bufcnt;
  else
    msgout("data_ready(): SANITY CHECK; from->state=%d=%s\n",
           statestr(from->state));

  /*
   * write the data just read to the log file if logging is enabled
   */
  logit = 0;
  if (xp && xp->log) {
    if (xp->flags & OPT_LOGALL) {
      logit = 1;
    }
    else if (xp->re && from->state == ST_REMOTE) {
      logit = 1;
    }
  }

  if (logit) {
    if (xp->flags & OPT_LOGHEX) {
      hexdump_buf(xp->logfd, from->state==ST_LOCAL ? "L  " : "R  ",
                  0, from->bufptr, from->bufcnt);
    }
    else {
      rc = write(xp->logfd, from->bufptr, from->bufcnt);
      if (rc < 0) {
        msgout("data_ready(): log file write error, "
               "dev=%s fd=%d f=\"%s\": %s\n",
               xp->devid, xp->logfd, xp->logfile, strerror(errno));
        msgout("disabling logging on device \"%s\"\n", xp->devid);
        xp->log = 0;
        close(xp->logfd);
        xp->logfd = -1;
      }
      else if (rc != from->bufcnt) {
        msgout("data_ready(): short write to log file, %d < %d\n",
               rc, from->bufcnt);
      }
    }
  }
  
  /* 
   * We've now got 'bufcnt' bytes on our buffer that need to be
   * written to the other side of the connection.  Try to write them
   * immediately.  If any cannot be written, de-select this side for
   * readability, and select the other side for writeabity; the state
   * machine and data_writable() will take care of the rest.
   *
   * If the 'nobuffer' option has been selected and the data can't be
   * written for some reason, just throw the data away and don't hold
   * up the sending side.
   */
  
  if (to == NULL) {
    /* we can't write any right now */
    if (xp->flags & OPT_BLOCK) {
      if (DEBUG_BUFFER())
        msgout("data_ready(): can't send data, %s side is not connected; "
               "disabling %s.rd\n",
               from->state == ST_LOCAL ? "remote" : "local",
               whichside(from->state));
      DISABLE_RD(from->fd);
    }
    else {
      if (DEBUG_BUFFER())
        msgout("data_ready(): device \"%s\", %d bytes lost from %s side\n",
               xp->devid, from->bufcnt, whichside(from->state));
      from->bufcnt = 0;
    }
    return -3;
  }

#if 0
  msgout("data_ready(): writing\n");
  msgout("data_ready(): write(%d, 0x%08x, %d)\n",
         to->fd, from->bufptr, from->bufcnt);
#endif
  rc = write(to->fd, from->bufptr, from->bufcnt);
#if 0
  msgout("data_ready(): write()=%d\n", rc);
#endif
  if (rc < 0) {
    if (errno == EAGAIN) {
      /* can't write right now */
      if (xp->flags & OPT_BLOCK) {
        if (DEBUG_BUFFER())
          msgout("data_ready(): write()=EAGAIN, device \"%s\", "
                 "%s can't receive data; disabling %s.rd, enabling %s.wr\n",
                 xp->devid, whichside(to->state),
                 whichside(from->state), whichside(to->state));
        DISABLE_RD(from->fd);
        ENABLE_WR(to->fd);
      }
      else {
        if (DEBUG_BUFFER())
          msgout("data_ready(): device \"%s\", %d bytes lost from %s side\n",
                 xp->devid, from->bufcnt, whichside(from->state));
        from->bufcnt = 0;
      }
      return -3;
    }
    else {
      /* system write error */
      msgout("data_ready(): device \"%s\" %s side write() error: %s\n",
             xp->devid, whichside(to->state), strerror(errno));
      CLEANUP(to);
#if 0
      CLEANUP(from);
#endif
      return -4;
    }
  }

  if (rc != from->bufcnt) {
    /* we wrote some data, but not all */
    if (DEBUG_BUFFER())
      msgout("data_ready(): device \"%s\", %d of %d bytes written "
             "to %s side\n",
             xp->devid, rc, from->bufcnt, whichside(to->state));
    if (xp->flags & OPT_BLOCK) {
      if (DEBUG_BUFFER())
        msgout("data_ready(): device \"%s\", %s can't receive all data, "
               "disabling %s.rd, enabling %s.wr\n",
               xp->devid, whichside(from->state), whichside(to->state));
      DISABLE_RD(from->fd);
      ENABLE_WR(to->fd);
      from->bufptr += rc;
      from->bufcnt -= rc;
    }
    else {
      if (DEBUG_BUFFER())
        msgout("data_ready(): device \"%s\", %d bytes lost from %s side\n",
               xp->devid, from->bufcnt-rc, whichside(from->state));
      from->bufcnt = 0;
    }
    return rc;
  }
  
  if (DEBUG_BUFFER())
    msgout("data_ready(): all data written\n");

  /* we wrote _all_ of the data */
  from->bufcnt = 0;
  from->bufptr = from->buf;
  
  return rc;
}



int data_writeable(ENDPOINT * to)
{
  ENDPOINT * from;
  COMSERV_PORT * xp;
  int rc;
  
  xp = to->comserv;
  
  switch (to->state) {
    case ST_LOCAL:
    case ST_REMOTE:
      break;
    default:
      msgout("data_writeable(): HELP! to->state=%d=%s\n",
             to->state, statestr(to->state));
      abort();
      break;
  }
  
  if (to->state == ST_LOCAL)
    from = xp->re;
  else
    from = xp->le;
  
  /* sanity check */
  if (to) {
    assert(xp == to->comserv);
  }
  if (from) {
    assert(xp == from->comserv);
  }
  
#if 1
  /*
   * make sure that the other side is actually still there
   */
  if (!from) {
    if (DEBUG_BUFFER())
      msgout("data_writeable(): %s side unexpectedly disconnected; "
             "disabling %s.wr\n",
             to->state==ST_REMOTE ? "local" : "remote",
             whichside(to->state));
    DISABLE_WR(to->fd);
    return 0;
  }

  /* 
   * make sure there is actually data to be written
   */
  if (!from->bufcnt) {
    if (DEBUG_BUFFER())
      msgout("data_writeable(): no data to be written; disabling %s.wr\n",
             to->state==ST_REMOTE ? "local" : "remote",
             whichside(to->state));
    DISABLE_WR(to->fd);
    return 0;
  }
#endif
  
  /* 
   * We've now got 'bufcnt' bytes on our buffer that need to be
   * written to the other side of the connection.  Try to write them
   * all at once.  If any cannot be written, adjust bufcnt and bufptr
   * as necessary and continue without modifying our read/write
   * fd_sets because they should be correct.  Only if we write all of
   * the data out of the buffer do we adjust our read/write fd_sets.
   */
  
  rc = write(to->fd, from->bufptr, from->bufcnt);
  if (rc < 0) {
    if (errno == EAGAIN) {
      /* can't write right now, don't do anything */
      return -3;
    }
    else {
      /* system write error */
      msgout("data_writeable(): device \"%s\" %s side write() error: %s\n",
             xp->devid, whichside(to->state), strerror(errno));
      CLEANUP(to);
#if 0
      CLEANUP(from);
#endif
      return -4;
    }
  }
  
  if (rc != from->bufcnt) {
    /* we wrote some data, but not all */
    if (DEBUG_BUFFER())
      msgout("data_writeable(): device \"%s\", %d of %d bytes written "
             "to %s side\n",
             xp->devid, rc, from->bufcnt, whichside(to->state));
    from->bufptr += rc;
    from->bufcnt -= rc;
    return rc;
  }
  
  /* we wrote _all_ of the data */
  from->bufcnt = 0;
  from->bufptr = from->buf;

  if (DEBUG_BUFFER())
    msgout("data_writeable(): all data written; disable %s.wr, enable %s.rd\n",
           whichside(to->state), whichside(from->state));

  DISABLE_WR(to->fd);
  ENABLE_RD(from->fd);
  
  return rc;
}


