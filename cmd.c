/* $Id: cmd.c,v 1.35 2002/06/20 01:52:48 bsd Exp $ */

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
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "cmd.h"
#include "comserv.h"
#include "log.h"


extern char prog_img[];
extern char ** prog_argv;
extern char module_timestamp[];


char * cmd_versionid = "$Id: cmd.c,v 1.35 2002/06/20 01:52:48 bsd Exp $";


/*
 * valid commands accepted on the command port
 */
COMMAND commands[] = {
  { "?",         cmd_help,      "print command list" },
  { "help",      cmd_help,      "print command list" },
  { "add",       cmd_add,       "add device id" },
  { "ctl",       cmd_ctl,       "add control device id" },
  { "devdir",    cmd_devdir,  "specify default directory for device entries" },
  { "endpoints", cmd_endpoints, "show local and remote connections" },
  { "list",      cmd_list,      "list device ids" },
  { "logdir",    cmd_logdir,    "specify the default log directory" },
  { "quit",      cmd_close,     "close the command connection" },
  { "restart",   cmd_restart,   "restart daemon" },
  { "serve",     cmd_serve,     "add a local service" },
  { "set",       cmd_set,       "set parameters" },
  { "show",      cmd_show,      "show parameters" },
  { "shutdown",  cmd_shutdown,  "shutdown the daemon" },
  { "status",    cmd_status,    "print daemon status" },
  { "version",   cmd_version,   "print version timestamp" }
};
#define N_COMMANDS (sizeof(commands)/sizeof(commands[0]))


char cmd_logdir_buf[PATH_MAX];

char cmd_devdir_buf[PATH_MAX];



int cmd_init(void)
{
  strcpy(cmd_logdir_buf, DEFAULT_LOGDIR);
  strcpy(cmd_devdir_buf, DEFAULT_DEVDIR);

  return 0;
}


COMMAND_PORT * new_cmd(void)
{
  COMMAND_PORT * c;
  
  c = (COMMAND_PORT *) malloc(sizeof(COMMAND_PORT));
  if (c == NULL) {
    msgout("out of memory allocating command structure\n");
    exit(1);
  }
  
  bzero(c,sizeof(*c));
  
  return c;
}



COMMAND * find_cmd(char * cmd)
{
  int i;
  
  /*
   * find the command in the command table
   */
  for (i=0; i<N_COMMANDS; i++) {
    if (strcmp(cmd,commands[i].command)==0) {
      return &commands[i];
    }
  }
  
  return NULL;
}




COMSERV_PORT * locate_devid(char * d)
{
  COMSERV_PORT * xp;
  
  for (xp = comserv_ports; xp; xp = xp->next) {
    if (strcmp(xp->devid,d)==0)
      return xp;
  }
  
  return NULL;
}



int run_command(ENDPOINT * e)
{
  char * p;
  char cmd[MAX_CMD];
  int i;
  COMMAND * c;
  
  p = e->command->buf;
  while (*p && isspace(*p))
    p++;
  
  if (!*p)
    return 0;
  
  if (*p == '#')
    return 0;
  
  i = 0;
  while (*p && (i < 64) && !(isspace(*p))) {
    cmd[i++] = *p++;
  }
  if (i == 64) {
    fdprintf(e->fd, "command too long\n");
    return -1;
  }
  cmd[i] = 0;
  
  while (*p && isspace(*p))
    p++;
  
  c = find_cmd(cmd);
  if (c == NULL) {
    fdprintf(e->fd, "command \"%s\" not found\n", cmd);
    return -2;
  }
  
  if (DEBUG_CMDS())
    msgout("exec(\"%s\",\"%s\")\n", cmd, p);
  
  c->function(e, p);
  
  return 0;
}


char * debugstr(unsigned int flags, char * buf)
{
  static char sbuf[128];
  char * b;
  int len;
  
  if (!buf)
    b = sbuf;
  else
    b = buf;
  
  b[0] = 0;
  
  if (flags & D_BUFFER)
    strcat(b, "buffer, ");
  else
    strcat(b, "no buffer, ");
  
  if (flags & D_STATES)
    strcat(b, "states, ");
  else
    strcat(b, "no states, ");
  
  if (flags & D_CONNECT)
    strcat(b, "connect, ");
  else
    strcat(b, "no connect, ");
  
  if (flags & D_CMDS)
    strcat(b, "cmds, ");
  else
    strcat(b, "no cmds, ");
  
  len = strlen(b);
  if (len >=2 && (b[len-1] == ' ') && (b[len-2] == ','))
    b[len-2] = 0;
  
  return b;
}


#define DBG_CHK(str, flag)    \
  if (strcmp(b, str) == 0) {  \
    match = 1;                \
    if (neg) {                \
      debug_flags &= ~flag;   \
    }                         \
    else {                    \
      debug_flags |= flag;    \
    }                         \
  }


int set_debug(ENDPOINT * e, COMSERV_PORT * xp, char * value)
{
  char buf[MAX_SET];
  char * p, * b;
  int i;
  int neg;
  int match;

  p = value;

  if (!*p) {
    fdprintf(e->fd, "debug flags = %s\n", 
             debugstr(debug_flags, NULL));
    return 0;
  }

  while (*p) {

    while (*p && (isspace(*p) || (*p == ',')))
      p++;

    i = 0;
    while (*p && (i<sizeof(buf)) && !(isspace(*p) || (*p == ',')))
      buf[i++] = *p++;
    
    if (i == sizeof(buf)) {
      buf[i-1] = 0;
      fdprintf(e->fd, "debugging flag \"%s\" is too long\n", buf);
      return -1;
    }

    buf[i] = 0;

    neg = 0;
    b = buf;
    if (strncmp(b, "no", 2) == 0) {
      neg = 1;
      b += 2;
    }

    match = 0;

    DBG_CHK("buffer", D_BUFFER);
    DBG_CHK("states", D_STATES);
    DBG_CHK("connect", D_CONNECT);
    DBG_CHK("cmds", D_CMDS);

    if (!match)
      fdprintf(e->fd, "unrecognized debugging flag \"%s\"\n", b);

  }
  
  return 0;
}



int cmd_debug(ENDPOINT * e, char * cmdline)
{
  set_debug(e, NULL, cmdline);
  return 0;
}



int cmd_endpoints(ENDPOINT * e, char * cmdline)
{
  int i;
  
  for (i=0; i<max_endpoint; i++) {
    if (endpoint[i]) {
      if (endpoint[i]->fd != -1) {
        fdprintf(e->fd, "fd:[%03d] ", i);
#ifdef ENABLE_TELNET_PORT
        if (i == lsock) {
          fdprintf(e->fd, "INCOMING TELNET CONNECTIONS\n");
        }
        else 
#endif
          if (endpoint[i]->comserv) {
            if (endpoint[i]->comserv->control) {
              fdprintf(e->fd, "%-23s -> ", "CONTROL");
            }
            else {
              fdprintf(e->fd, "%-15s Port %02d -> ",
                       endpoint[i]->comserv->host, 
                       endpoint[i]->comserv->serport);
            }
            fdprintf(e->fd, "%s", endpoint[i]->comserv->localpath);
            if (endpoint[i]->comserv->le && 
                (i == endpoint[i]->comserv->le->fd)) {
              fdprintf(e->fd, " (L)");
            }
            else if (endpoint[i]->comserv->re && 
                     (i == endpoint[i]->comserv->re->fd)) {
              fdprintf(e->fd, " (R)");
            }
            else if (endpoint[i]->comserv->listen && 
                     (i == endpoint[i]->comserv->listen->fd)) {
              fdprintf(e->fd, " (LISTEN)");
            }
            fdprintf(e->fd, "\n");
          }
          else if (endpoint[i]->command) {
            fdprintf(e->fd, "COMMAND: %s", endpoint[i]->command->name);
            if (i == e->fd) {
              fdprintf(e->fd, " (this is you)\n");
            }
            else {
              fdprintf(e->fd, "\n");
            }
          }
      }
    }
  }  /* for */

  return 0;
}



int cmd_close(ENDPOINT * e, char * cmdline)
{
  msgout("%s is exiting command mode\n",
         endpoint[e->fd]->command->name);
  
  fdprintf(e->fd, "goodbye\n");
  CLEANUP(endpoint[e->fd]);
  
  return 0;
}



int cmd_help(ENDPOINT * e, char * cmdline)
{
  int j;
  
  fdprintf(e->fd, "\nvalid commands:\n");
  for (j=0; j<N_COMMANDS; j++) {
    fdprintf(e->fd, "  %-15s - %s\n",
             commands[j].command, commands[j].desc);
  }
  fdprintf(e->fd, "\n");
  
  return 0;
}



int cmd_shutdown(ENDPOINT * e, char * cmdline)
{
  int j;
  
  msgout("shutting down\n");
  
  for (j=0; j<FD_SETSIZE; j++) {
    if (ISENABLED_RD(j) || ISENABLED_WR(j)) {
      if (endpoint[j]->state == ST_CMDINPUT) {
        fdprintf(j, "shutting down, goodbye\n");
      }
      CLEANUP(endpoint[j]);
    }
  }
  
  exit(0);
}


int get_int(int fd, char * b, int * v)
{
  int iv;
  char * ep, * q;
  
  iv = strtol(b, &ep, 0);
  if ((ep == b) || !((*ep == 0)||isspace(*ep)||(*ep == ','))) {
    fdprintf(fd, "invalid numeric escape code \"");
    q = b;
    while (*q && !(isspace(*ep)||(*ep == ','))) {
      fdprintf(fd, "%c", *q);
      q++;
    }
    fdprintf(fd, "\"\n");
    return -1;
  }
  
  *v = iv;
  return 0;
}



int set_options(ENDPOINT * e, COMSERV_PORT * xp, char * opts)
{
  char * p;
  char buf[MAX_CMD];
  int i;

  p = opts;
  while (*p) {
    i = 0;
    while (*p && (i < MAX_CMD) && !((*p == ',') || isspace(*p)))
      buf[i++] = *p++;
    
    if (i == MAX_CMD) {
      buf[i-1] = 0;
      fdprintf(e->fd, "option name \"%s\" too long\n", buf);
      return -1;
    }
    
    buf[i] = 0;
    
    if (strcmp(buf,"wait")==0) {
      if (xp) {
        xp->flags |= OPT_WAIT;
      }
    }
    else if (strcmp(buf,"nowait")==0) {
      if (xp) {
        xp->flags &= ~OPT_WAIT;
      }
    }
    else if (strcmp(buf,"logall")==0) {
      if (xp) {
        xp->flags |= OPT_LOGALL;
      }
    }
    else if (strcmp(buf,"nologall")==0) {
      if (xp) {
        xp->flags &= ~OPT_LOGALL;
      }
    }
    else if (strcmp(buf,"loghex")==0) {
      if (xp) {
        xp->flags |= OPT_LOGHEX;
      }
    }
    else if (strcmp(buf,"nologhex")==0) {
      if (xp) {
        xp->flags &= ~OPT_LOGHEX;
      }
    }
    else if (strcmp(buf,"block")==0) {
      if (xp) {
        xp->flags |= OPT_BLOCK;
      }
    }
    else if (strcmp(buf,"noblock")==0) {
      if (xp) {
        xp->flags &= ~OPT_BLOCK;
        if (xp->re) {
          /* 
           * this option must take effect immediately, so handle that
           * here
           */
          if (!ISENABLED_RD(xp->re->fd)) {
            if (DEBUG_BUFFER())
              msgout("device \"%s\", option \"nobuffer\" set, "
                     "enabling %s.rd\n",
                     xp->devid, whichside(xp->re->state));
            ENABLE_RD(xp->re->fd);
            if (xp->re->bufcnt) {
              xp->re->bufcnt = 0;
              xp->re->bufptr = xp->re->buf;
            }
          }
        }
      }
    }
    else {
      fdprintf(e->fd, "WARNING: device \"%s\", unrecognized option \"%s\"\n",
               xp ? xp->devid : "global", buf);
    }
    
    while (*p && isspace(*p))
      p++;
    
    if (*p == ',') {
      p++;
    }
    
    while (*p && isspace(*p))
      p++;
    
  }
  
  return 0;
}


int cmd_restart(ENDPOINT * e, char * cmdline)
{
  int rc;
  int i;
  
  if (prog_img[0] == 0) {
    if (e)
      fdprintf(e->fd, "can't determine my image file, can't restart\n");
    else
      msgout("can't determine my image file, can't restart\n");
    return -1;
  }
  
  msgout("restarting %s ...\n", prog_img);
  
  for (i=0; i<FD_SETSIZE; i++) {
    close(i);
  }
  
  prog_argv[0] = prog_img;
  rc = execv(prog_img, prog_argv);
  
  msgout("can't re-exec \"%s\": %s\n", prog_img, strerror(errno));
  
  exit(1);
}


int cmd_set(ENDPOINT * e, char * cmdline)
{
  char buf[MAX_SET];
  char * var;
  char * value;
  char * ep;
  long ival;
  char devid[MAX_DEVID];
  char * p;
  int i;
  COMSERV_PORT * xp;
  
  i = 0;
  p = cmdline;
  while (*p && (i < MAX_DEVID) && !isspace(*p))
    devid[i++] = *p++;
  if (i == MAX_DEVID) {
    devid[i-1] = 0;
    fdprintf(e->fd, "devid \"%s\" is too long\n", devid);
    return -1;
  }
  devid[i] = 0;
  
  if (strcmp(devid,"global") == 0) {
    xp = &global;
  }
  else {
    xp = locate_devid(devid);
    if (xp == NULL) {
      fdprintf(e->fd, "devid id \"%s\" not found, use \"add DeviceId ...\"\n",
               devid);
      return -2;
    }
  }
  
  while (*p && isspace(*p))
    p++;

  buf[MAX_SET-1] = 0;
  strncpy(buf,p,MAX_SET-1);
  if (buf[MAX_SET-1] != 0) {
    fdprintf(e->fd, "set: var=value text is too long\n");
    return -1;
  }
  
  var = buf;
  p   = var;
  while (*p && !((*p == '=') || isspace(*p)))
    p++;
  
  if (isspace(*p)) {
    *p = 0;
    p++;
  }

  while (*p && (*p != '='))
    p++;
  
  if ((*p == 0) || (*p != '=')) {
    fdprintf(e->fd, "expecting '='\n");
    fdprintf(e->fd, "Usage: set DeviceId VAR=VALUE\n");
  }

  *p = 0;
  p++;
  
  while (*p && isspace(*p))
    p++;

  if (*p == 0) {
    fdprintf(e->fd, "expecting a value\n");
    fdprintf(e->fd, "Usage: set DeviceId VAR=VALUE\n");
  }
  
  value = p;
  
  if (strcmp(var,"verbose")==0) {
    ival = strtol(value, &ep, 0);
    if (*ep) {
      fdprintf(e->fd, "set: couldn't parse %s value \"%s\"\n", var, value);
      return -1;
    }

    if (xp == &global) {
      verbose = ival;
    }
    
    return 0;
  }
  else if (strcmp(var,"options")==0) {
    set_options(e, xp, value);
    return 0;
  }
  else if (strcmp(var, "debug") == 0) {
    set_debug(e, xp, value);
    return 0;
  }
  else {
    fdprintf(e->fd, "set: don't know about the \"%s\" variable\n", var);
  }

  return -2;
}



int hexdump_line(char * buffer, unsigned char * p, int n, int pad)
{
  char * hexdata = "0123456789abcdef";
  char * b;
  int i, j;
  
  b = buffer;
  
  j = 0;
  for (i=0; i<n; i++) {
    if (i && ((i % 8) == 0))
      b[j++] = ' ';
    b[j++] = hexdata[(p[i] & 0xf0) >> 4];
    b[j++] = hexdata[(p[i] & 0x0f)];
    if (i < 15)
      b[j++] = ' ';
  }
  
  for (i=j; i<pad; i++)
    b[i] = ' ';
  
  b[i] = 0;
  
  for (i=0; i<pad; i++) {
    if (!((b[i] == '0') || (b[i] == ' ')))
      return 0;
  }
  
  return 1;
}


int chardump_line(char * buffer, unsigned char * p, int n, int pad)
{
  int i;
  char b [ 128 ];
  
  for (i=0; i<n; i++) {
    memcpy(b, p, n);
    buffer[i] = '.';
    if (isalpha(b[i]) || isdigit(b[i]) || ispunct(b[i]))
      buffer[i] = b[i];
    else if (isspace(b[i]))
      buffer[i] = ' ';
  }
  
  for (i=n; i<pad; i++)
    buffer[i] = ' ';
  
  buffer[i] = 0;
  
  return 0;
}

int hexdump_buf(int fd, char * prefix, int startaddr, char * buf, int len)
{
  int addr;
  int i, n;
  unsigned char * p;
  char dst1[80];
  char dst2[80];
  
  addr = startaddr;
  i = 0;
  p = (unsigned char *)buf;
  while (len) {
    n = 16;
    if (n > len)
      n = len;
    hexdump_line(dst1, p, n, 48);
    chardump_line(dst2, p, n, 16);
    fdprintf(fd, "%s%s  |%s|\n", prefix, dst1, dst2);
    len -= n;
    addr += n;
    p += n;
  }
  
  return 0;
}

char * optionsstr(unsigned int flags, char * buf)
{
  static char sbuf[128];
  char * b;
  int len;
  
  if (!buf)
    b = sbuf;
  else
    b = buf;
  
  b[0] = 0;
  
  if (flags & OPT_WAIT)
    strcat(b, "wait, ");
  else
    strcat(b, "no wait, ");
  
  if (flags & OPT_LOGALL)
    strcat(b, "logall, ");
  else
    strcat(b, "no logall, ");
  
  if (flags & OPT_LOGHEX)
    strcat(b, "loghex, ");
  else
    strcat(b, "no loghex, ");
  
  if (flags & OPT_BLOCK)
    strcat(b, "block, ");
  else
    strcat(b, "no block, ");
  
  len = strlen(b);
  if (len >=2 && (b[len-1] == ' ') && (b[len-2] == ','))
    b[len-2] = 0;
  
  return b;
}


int show_devid(int fd, COMSERV_PORT * xp)
{
  int buflen;
  
  fdprintf(fd, "Device Id: %-8s\n", xp->devid);
  fdprintf(fd, "  Local   : %s\n", xp->localpath);
  fdprintf(fd, "  Remote  : %s %2d %5d\n", xp->host, xp->serport, xp->port);
  fdprintf(fd, "  LogFile : %s\n",
           xp->log ? xp->logfile : "N/A");
  fdprintf(fd, "  Options : %s\n",
           optionsstr(xp->flags, NULL));
  fdprintf(fd, "  Data Tx : Local = %8d    Remote = %8d\n",
           xp->n_le, xp->n_re);

  if (xp->le) {
    buflen = xp->le->bufcnt;
    if (buflen) {
      fdprintf(fd, "  LBuffer : len=%d (contents below)\n", buflen);
      hexdump_buf(fd, "  ", 0, xp->le->bufptr, buflen);
    }
    else {
      fdprintf(fd, "  LBuffer : len=%d\n", buflen);
    }
  }
  
  if (xp->re) {
    buflen = xp->re->bufcnt;
    if (buflen) {
      fdprintf(fd, "  RBuffer : len=%d (contents below)\n", buflen);
      hexdump_buf(fd, "  ", 0, xp->re->bufptr, buflen);
    }
    else {
      fdprintf(fd, "  RBuffer : len=%d\n", buflen);
    }
  }
  
  return 0;
}



int cmd_show(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp;
  
  if (strcmp(cmdline,"global")==0) {
    fdprintf(e->fd, 
             "verbose     = %d\n"
             "debug flags = %s\n",
             verbose, debugstr(debug_flags, NULL));
    show_devid(e->fd, &global);
  }
  else {
    xp = locate_devid(cmdline);
    if (xp == NULL) {
      fdprintf(e->fd, "Device Id \"%s\" not found\n", cmdline);
      return -1;
    }
    
    show_devid(e->fd, xp);
  }
  
  return 0;
}



int print_endpoint_status(int fd, ENDPOINT * e)
{
  char * conn;

  if (e) {
    if (e->state == ST_CONNECTING)
      conn = "no";
    else
      conn = "yes";

    fdprintf(fd, "%4s ", conn);
    
    fdprintf(fd, "%3d ", e->fd);
    
    if (ISENABLED_RD(e->fd))
      fdprintf(fd, " X ");
    else
      fdprintf(fd, "   ");
    
    if (ISENABLED_WR(e->fd))
      fdprintf(fd, " X ");
    else
      fdprintf(fd, "   ");
    
    fdprintf(fd, "%4d ", e->bufcnt);
  }
  else {
    fdprintf(fd, "%4s %15s", "no", " ");
  }
  
  return 0;
}



int varw_field(char * b1, char * b2, char * b3, char * b4, int max, 
               int wid, char * title, char * format)
{
  int i;
  int len;
  
  len = strlen(title);
  if (wid < len)
    wid = len;
  
  if (wid > (max-1))
    wid = max-1;
  
  
  for (i=0; i<wid; i++) {
    b1[i] = ' ';
    b3[i] = '-';
  }
  b1[i] = 0;
  b3[i] = 0;
  
  strncpy(b2,title,max-1);
  len = strlen(b2);
  for (i=len; i<wid; i++)
    b2[i] = ' ';
  b2[i] = 0;
  
  snprintf(b4, max, "%%-%d%s", wid, format);
  
  return 0;
}


int print_comserv_status(int fd, int devid_width, int host_width, 
                         int dev_width, COMSERV_PORT * xp, int dvdir)
{
  char id1[32], id2[32], id3[32], id4[8];
  char dev1[64], dev2[64], dev3[64], dev4[8];
  char h1[MAX_HOST], h2[MAX_HOST], h3[MAX_HOST], h4[8];
  char * p;
  
  varw_field(id1, id2, id3, id4, sizeof(id1), devid_width, "Id", "s");
  varw_field(dev1, dev2, dev3, dev4, sizeof(dev1), dev_width, "Device", "s");
  varw_field(h1, h2, h3, h4, sizeof(h1), host_width, "Host", "s");
  
  if (xp == NULL) {
    fdprintf(fd, 
             "%s %s Srl  TCP   %s Local Endpoint      Remote Endpoint\n"
             "%s %s Port Port  %s Conn Fd  Rd Wr Data Conn Fd  Rd Wr Data\n"
             "%s %s ---- ----- %s ---- --- -- -- ---- ---- --- -- -- ----\n",
             id1, h1, dev1, id2, h2, dev2, id3, h3, dev3);
    return 0;
  }

  p = xp->localpath;
  if (strncmp(cmd_devdir_buf, xp->localpath, dvdir) == 0) {
    p = rindex(xp->localpath, '/');
    if (p) {
      p++;
    }
    else {
      p = xp->localpath;
    }
  }

  snprintf(dev1, sizeof(dev1), dev4, p);
  snprintf(id1, sizeof(id1), id4, xp->devid);
  snprintf(h1, sizeof(h1), h4, xp->host);
  
  fdprintf(fd, "%s %s %4d %5d %s ",
           id1, h1, xp->serport, xp->port, dev1);
  
  print_endpoint_status(fd, xp->le);
  print_endpoint_status(fd, xp->re);
  
  fdprintf(fd, "\n");
  
  return 0;
}



int cmd_list(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp;
  
  xp = comserv_ports;
  while (xp) {
    fdprintf(e->fd, "%-15s %s %s\n",
             xp->devid, xp->localpath,
             xp->log ? xp->logfile : "<no-log>");
    xp = xp->next;
  }
  
  return 0;
}



int cmd_status(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp;
  int dev_width, len, devid_width, host_width;
  char * p;
  int dvdir;
  
  fdprintf(e->fd, 
           "COMSERV revision timestamp %s\n"
           "logdir = %s\n"
           "devdir = %s\n",
           module_timestamp, cmd_logdir_buf, cmd_devdir_buf);
  
  dvdir = strlen(cmd_devdir_buf);
  dev_width   = 0;
  devid_width = 0;
  host_width  = 0;
  xp = comserv_ports;
  while (xp) {

    if (strncmp(cmd_devdir_buf, xp->localpath, dvdir) == 0) {
      p = rindex(xp->localpath, '/');
      if (p) {
        p++;
        len = strlen(p);
      }
      else {
        len = strlen(xp->localpath);
      }
    }
    else {
      len = strlen(xp->localpath);
    }
    if (len > dev_width)
      dev_width = len;

    len = strlen(xp->devid);
    if (len > devid_width)
      devid_width = len;

    len = strlen(xp->host);
    if (len > host_width)
      host_width = len;

    xp = xp->next;
  }
  
  print_comserv_status(e->fd, devid_width, host_width, dev_width, NULL, dvdir);
  
  xp = comserv_ports;
  while (xp) {
    print_comserv_status(e->fd, devid_width, host_width, dev_width, xp, dvdir);
    xp = xp->next;
  }
  
  return 0;
}


int cmd_version(ENDPOINT * e, char * cmdline)
{
  fdprintf(e->fd, "revision timestamp = %s\n", module_timestamp);
  return 0;
}






int cmd_devdir(ENDPOINT * e, char * cmdline)
{
  char * p;
  
  p = cmdline;
  while (*p && isspace(*p))
    p++;
  
  if (*p) {
    strncpy(cmd_devdir_buf, cmdline, PATH_MAX);
    cmd_devdir_buf[PATH_MAX-1] = 0;
  }
  
  return 0;
}



int cmd_logdir(ENDPOINT * e, char * cmdline)
{
  char * p;
  
  p = cmdline;
  while (*p && isspace(*p))
    p++;
  
  if (*p) {
    strncpy(cmd_logdir_buf, p, PATH_MAX);
    cmd_logdir_buf[PATH_MAX-1] = 0;
  }
  
  return 0;
}



int cmd_add(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp, * xp2;
  char devid [ 1024 ];
  char host  [ 1024 ];
  char localpath [ 1024 ];
  char logopt [ PATH_MAX ];
  int port;
  int serialport;
  int n;
  
  n = sscanf(cmdline, "%s %s %s %d %d %s",
             devid, localpath, host, &serialport, &port, logopt);
  if (n != 6) {
    fdprintf(e->fd, 
             "invalid number of parameters (%d) to the 'add' command\n", n);
    fdprintf(e->fd, "Usage: add DevId LocalDev Host Port TCPPort LogOption\n");
    return -1;
  }
  
  xp2 = locate_devid(devid);
  if (xp2 != NULL) {
    fdprintf(e->fd,"Device Id \"%s\" already present\n", devid);
    return 0;
  }
  
  xp = new_comserv_port();
  strncpy(xp->devid, devid, MAX_DEVID-1);
  strncpy(xp->host, host, MAX_HOST-1);
  xp->serport = serialport;
  xp->port    = port;
  xp->logfd   = -1;
  xp->reconnect_time_incr = 1;
  xp->reconnect_time = time(NULL) + xp->reconnect_time_incr;
  
  /* default options flags */
  xp->flags = global.flags;
  xp->rflags = 0;
  
  if (localpath[0] == '/') {
    xp->localpath[0] = 0;
  }
  else {
    strcpy(xp->localpath, cmd_devdir_buf);
    strcat(xp->localpath, "/");
  }
  strncat(xp->localpath, localpath, PATH_MAX-1);
  
  if (strcasecmp(logopt, "log")==0) {
    xp->log = 1;
    strcpy(xp->logfile, cmd_logdir_buf);
    strcat(xp->logfile, "/");
    strcat(xp->logfile, xp->devid);
  }
  else if (strcasecmp(logopt, "nolog")==0) {
    xp->log = 0;
    xp->logfile[0] = 0;
  }
  else if (logopt[0] != '/') {
    xp->log = 1;
    strcpy(xp->logfile, cmd_logdir_buf);
    strcat(xp->logfile, "/");
    strcat(xp->logfile, logopt);
  }
  else {
    xp->log = 1;
    strcpy(xp->logfile, logopt);
  }
  

  if (!comserv_ports) {
    comserv_ports = xp;
  }
  else {
    for (xp2=comserv_ports; xp2->next; xp2 = xp2->next)
      ;
    xp2->next = xp;
  }
  
  /*
   * configure the newly added device id
   */
  
  if (verbose >= 1) {
    fdprintf(e->fd, "configuring device id \"%s\"\n", xp->devid);
  }

  /*
   * Open the log file if requested
   */
  if (xp->log) {
    mode_t mode;
    mode = S_IRUSR|S_IWUSR|S_IRGRP;
    xp->logfd = open(xp->logfile, O_CREAT|O_WRONLY|O_APPEND, mode);
    if (xp->logfd < 0) {
      fdprintf(e->fd,
               "failed to open log file \"%s\": %s; " 
               "logging on this port disabled\n",
               xp->logfile, strerror(errno));
      xp->log = 0;
      xp->logfd = -1;
    }
  }
  
  if (verbose >= 1) {
    fdprintf(e->fd, "%s -> %s:%d, L[%d]<-->R[%d] logs->%s\n", 
             xp->localpath,
             xp->host, xp->port,
             xp->le  ? xp->le->fd : -1,
             xp->re ? xp->re->fd : -1,
             xp->log ? xp->logfile : "<none>");
  }
  
  return 0;
}


/*
 *
 * serve devid /dev/cuaa0 1 2100 log
 */


int cmd_serve(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp, * xp2;
  char devid [ 1024 ];
  char localpath [ 1024 ];
  char logopt [ PATH_MAX ];
  int port;
  int serialport;
  int n;
  int sock;
  
  n = sscanf(cmdline, "%s %s %d %d %s",
             devid, localpath, &serialport, &port, logopt);
  if (n != 5) {
    fdprintf(e->fd, 
             "invalid number of parameters (%d) to the 'serve' command\n", n);
    fdprintf(e->fd, "Usage: serve DevId LocalDev TCPPort LogOption\n");
    return -1;
  }
  
  xp2 = locate_devid(devid);
  if (xp2 != NULL) {
    fdprintf(e->fd,"Device Id \"%s\" already present\n", devid);
    return 0;
  }

  xp = new_comserv_port();
  strncpy(xp->devid, devid, MAX_DEVID-1);
  strncpy(xp->host, "localhost", MAX_HOST-1);
  xp->serport = serialport;
  xp->port    = port;
  xp->logfd   = -1;

  /* default options flags */
  xp->flags = global.flags;
  xp->rflags = RFLAG_LISTEN;

  if (localpath[0] == '/') {
    xp->localpath[0] = 0;
  }
  else {
    strcpy(xp->localpath, cmd_devdir_buf);
    strcat(xp->localpath, "/");
  }
  strncat(xp->localpath, localpath, PATH_MAX-1);
  
  if (strcasecmp(logopt, "log")==0) {
    xp->log = 1;
    strcpy(xp->logfile, cmd_logdir_buf);
    strcat(xp->logfile, "/");
    strcat(xp->logfile, xp->devid);
  }
  else if (strcasecmp(logopt, "nolog")==0) {
    xp->log = 0;
    xp->logfile[0] = 0;
  }
  else if (logopt[0] != '/') {
    xp->log = 1;
    strcpy(xp->logfile, cmd_logdir_buf);
    strcat(xp->logfile, "/");
    strcat(xp->logfile, logopt);
  }
  else {
    xp->log = 1;
    strcpy(xp->logfile, logopt);
  }


  /*
   * configure the newly added device id
   */

  if (verbose >= 1) {
    fdprintf(e->fd, "configuring device id \"%s\"\n", xp->devid);
  }

  sock = sock_bind_service(NULL, xp->port);
  if (sock < 0) {
    if (sock == -1) {
      msgout("can't establish service for \"%s\" port %d\n",
             xp->devid, xp->port);
    }
    else {
      msgout("failed to establish service for \"%s\" port %d\n",
             xp->devid, xp->port);
    }
    free_comserv_port(xp);
    return -1;
  }

  /*
   * add this device node to the list
   */
  if (!comserv_ports) {
    comserv_ports = xp;
  }
  else {
    for (xp2=comserv_ports; xp2->next; xp2 = xp2->next)
      ;
    xp2->next = xp;
  }

  if (endpoint[sock] == NULL)
    endpoint[sock] = new_endp();
  init_endp(endpoint[sock]);
  endpoint[sock]->state   = ST_SERVICE;
  endpoint[sock]->fd      = sock;
  endpoint[sock]->command = NULL;
  endpoint[sock]->comserv = xp;
  ENABLE_RD(sock);
  maxfd = max(maxfd,sock);
  xp->listen = endpoint[sock];


  /*
   * Open the log file if requested
   */
  if (xp->log) {
    mode_t mode;
    mode = S_IRUSR|S_IWUSR|S_IRGRP;
    xp->logfd = open(xp->logfile, O_CREAT|O_WRONLY|O_APPEND, mode);
    if (xp->logfd < 0) {
      fdprintf(e->fd,
               "failed to open log file \"%s\": %s; " 
               "logging on this port disabled\n",
               xp->logfile, strerror(errno));
      xp->log = 0;
      xp->logfd = -1;
    }
  }
  
  if (verbose >= 1) {
    fdprintf(e->fd, "%s -> %s:%d, L[%d]<-->R[%d] logs->%s\n", 
             xp->localpath,
             xp->host, xp->port,
             xp->le  ? xp->le->fd : -1,
             xp->re ? xp->re->fd : -1,
             xp->log ? xp->logfile : "<none>");
  }
  
  return 0;
}



int cmd_ctl(ENDPOINT * e, char * cmdline)
{
  COMSERV_PORT * xp, * xp2;
  char devid [ 1024 ];
  char localpath [ 1024 ];
  int n;
  int xfd;
  
  n = sscanf(cmdline, "%s %s",
             devid, localpath);
  if (n != 2) {
    fdprintf(e->fd, 
             "invalid number of parameters (%d) to the 'ctl' command\n", 
             n);
    fdprintf(e->fd, "Usage: ctl DevId LocalDev\n");
    return -1;
  }
  
  xp2 = locate_devid(devid);
  if (xp2 != NULL) {
    fdprintf(e->fd, "Device Id \"%s\" already present\n", devid);
    return 0;
  }
  
  xp = new_comserv_port();
  strncpy(xp->devid, devid, MAX_DEVID-1);
  xp->host[0] = 0;
  xp->serport = 0;
  xp->port    = 0;
  xp->flags   = OPT_WAIT;
  xp->control = 1;  /* identify this as a control port */
  
  if (localpath[0] == '/') {
    xp->localpath[0] = 0;
  }
  else {
    strcpy(xp->localpath, cmd_devdir_buf);
    strcat(xp->localpath, "/");
  }
  strncat(xp->localpath, localpath, PATH_MAX-1);
  
  /* 
   * add the control port to the list of device ids 
   */
  if (!comserv_ports) {
    comserv_ports = xp;
  }
  else {
    for (xp2=comserv_ports; xp2->next; xp2 = xp2->next)
      ;
    xp2->next = xp;
  }
  
  /*
   * configure the newly added device id
   */
  
  if (verbose >= 1) {
    fdprintf(e->fd, "configuring device id \"%s\"\n",
             xp->devid);
  }
  
  /*
   * connect_user() will do the rest
   */
  xfd = connect_user(xp, 0);
  if (xfd < 0) {
    fdprintf(e->fd, "failed to initialize control connection\n");
    exit(1);
  }
  
  if (isatty(xp->le->fd)) {
    fdprintf(xp->le->fd, "\nwelcome, %s\n", 
             xp->le->command->name);
    fdprintf(xp->le->fd, "%s", xp->le->command->prompt);
  }
  
  if (verbose >= 1) {
    msgout("%s -> control, L[%d]<-->control\n", 
           xp->localpath,
           xp->le  ? xp->le->fd : -1);
  }
  
  return 0;
}


