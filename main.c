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

/*
 * Name    : comservd
 *
 * Purpose : Program for communicating with terminal server devices,
 *           such as the Xyplex MAXserver 1600.  This program allows
 *           programs like 'tip' to connect to serial devices
 *           connected to the terminal server by providing an
 *           interface between pseudo ttys and network terminal server
 *           serial ports.
 *
 * Author  : Brian Dean, bsd@FreeBSD.org, bsd@bsdhome.com
 * Date    : 25 June, 2000
 * 
 */


/* $Id: main.c,v 1.20 2002/04/30 15:34:33 bsd Exp $ */


#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "comserv.h"
#include "endp.h"
#include "evloop.h"
#include "log.h"


#ifdef LIBEFENCE
/* for controlling ElectricFence */
extern int EF_PROTECT_FREE;
extern int EF_PROTECT_BELOW;
extern int EF_ALIGNMENT;
extern int EF_ALLOW_MALLOC_0;
extern int EF_FILL;
#endif



COMSERV_PORT * comserv_ports=NULL;   /* linked list of defined terminal
                                        server ports that we know about */

COMSERV_PORT global;

int maxfd;        /* max file descriptor currently in use */

fd_set master_rd; /* the master set of file descriptor bits that are
                     selected for readability */
fd_set master_wr; /* the master set of file descriptor bits that are
                     selected for writeability */

#ifdef ENABLE_TELNET_PORT
int lsock;        /* listening socket for telnet command interface */
#endif

int config_fd;    /* file descriptor for the configuration file */

char * progname;

char * cvsvers = "$Id: main.c,v 1.20 2002/04/30 15:34:33 bsd Exp $";

char module_timestamp[64];

int verbose = 0;

unsigned int debug_flags = 0;

volatile int hupped = 0;


char prog_img [ PATH_MAX ];
char ** prog_argv;

char * main_versionid = "$Id: main.c,v 1.20 2002/04/30 15:34:33 bsd Exp $";

extern char * cmd_versionid;
extern char * comserv_versionid;
extern char * endp_versionid;
extern char * evloop_versionid;
extern char * log_versionid;
extern char * main_versionid;
extern char * pty_versionid;

#define N_MODULES 7
char ** modules[N_MODULES] = { 
  &cmd_versionid, 
  &comserv_versionid, 
  &endp_versionid, 
  &evloop_versionid, 
  &log_versionid, 
  &main_versionid, 
  &pty_versionid
};


int print_module_versions(FILE * outf, char * timestamp);


void sighup(int sig)
{
  hupped = 1;
}



void sigpipe(int sig)
{
  msgout("caught SIGPIPE!\n");
  return;
}




int main(int argc, char * argv [])
{
  int rc;
  int c;
  
#ifdef LIBEFENCE
  EF_PROTECT_FREE=1;
  EF_PROTECT_BELOW=1;
#endif

  progname = rindex(argv[0],'/');
  if (progname) {
    progname++;
  }
  else {
    progname = argv[0];
  }
  
  prog_argv = argv;
  if (realpath(argv[0], prog_img) == NULL) {
    prog_img[0] = 0;
  }

  memset(&global, 0, sizeof(global));
  global.flags = OPT_DEFAULT;
  
  strncpy(config_file, DEFAULT_CONFIG, PATH_MAX);
  config_file[PATH_MAX-1] = 0;
  
  while ((c = getopt(argc, argv, "f:")) != -1) {
    switch (c) {
      case 'f':
        strncpy(config_file, optarg, PATH_MAX);
        config_file[PATH_MAX-1] = 0;
        break;
        
      case '?':
      default :
        fprintf(stderr, "%s: invalid command line option \"%s\"\n",
                progname, argv[optind]);
        exit(1);
        break;
    }
  }
  
#if CONSOLE == 0
  openlog(progname, LOG_PID, LOG_DAEMON);

  rc = daemon(0, 0);
  if (rc) {
    msgout("can't become daemon: %s\n", strerror(errno));
    exit(1);
  }
#endif

  strcpy(module_timestamp, "indeterminate");
  
  msgout("starting\n");
  print_module_versions(stderr, module_timestamp);
  msgout("revision timestamp %s\n", module_timestamp);
  
  rc = access(prog_img, X_OK);
  if (rc < 0) {
    msgout("can't determine image file; restart feature won't work\n");
    prog_img[0] = 0;
  }
  else {
    msgout("image file = %s\n", prog_img);
  }

  /*
   * initialize command interface
   */
  cmd_init();

  /*
   * initialize endpoints
   */
  rc = endp_init();
  if (rc < 0) {
    msgout("terminating\n");
    exit(1);
  }
  
  msgout("max # endpoints is %d\n", max_endpoint);
  
#ifdef ENABLE_TELNET_PORT
  lsock = sock_bind_service(COMSERV, 0);
  if (lsock < 0) {
    if (lsock == -1) {
      msgout("can't establish service for \"%s\"\n", COMSERV);
      msgout("add service \"%s  N/tcp\" to /etc/services\n", COMSERV);
    }
    else {
      msgout("failed to establish telnet command interface\n");
      msgout("telnet command interface is disabled\n");
    }
    lsock = -1;
  }
#endif
  
  comserv_ports = NULL;
  
  signal(SIGHUP, sighup);
  signal(SIGPIPE, sigpipe);
  
  event_loop();
  
  msgout("exiting\n");
  
  return 0;
}



int parse_cvsid(char * cvsid, char * name, char * rev, char * datetime)
{
  int i, j;
  
  if (!((strncmp(cvsid,"$Id: ",5) != 0) ||
        (strncmp(cvsid,"$FreeBSD: ",10) != 0)))
    return -1;
  
  name[0]     = 0;
  rev[0]      = 0;
  datetime[0] = 0;
  
  i = 0;
  j = 0;
  while (cvsid[j] && cvsid[j] != ' ')
    j++;
  
  while (cvsid[j] && cvsid[j] != ',')
    name[i++] = cvsid[j++];
  name[i] = 0;
  
  while (cvsid[j] && cvsid[j] != ' ')
    j++;
  
  if (cvsid[j])
    j++;
  
  i = 0;
  while (cvsid[j] && cvsid[j] != ' ')
    rev[i++] = cvsid[j++];
  rev[i] = 0;
  
  if (cvsid[j])
    j++;
  
  i = 0;
  while (cvsid[j] && cvsid[j] != ' ')
    datetime[i++] = cvsid[j++];
  if (cvsid[j] == ' ') {
    datetime[i++] = cvsid[j++];
    while (cvsid[j] && cvsid[j] != ' ')
      datetime[i++] = cvsid[j++];
  }
  datetime[i] = 0;
  
  return 0;
}


int print_module_versions(FILE * outf, char * timestamp)
{
  char name[64], rev[16], datetime[64];
  int y, m, d, h, min, s;
  int i;
  int rc;
  int maxtime;
  struct tm t;
  time_t now;
  
  maxtime = 0;
  for (i=0; i<N_MODULES; i++) {
    parse_cvsid(*modules[i], name, rev, datetime);
    rc = sscanf(datetime, "%d/%d/%d %d:%d:%d", &y, &m, &d, &h, &min, &s);
    if (rc != 6) {
      fprintf(stderr, "%s: module version scan error, rc=%d\n", progname, rc);
    }
    else if (timestamp) {
      now = time(NULL);
      gmtime_r(&now, &t);
      t.tm_sec  = s;
      t.tm_min  = min;
      t.tm_hour = h;
      t.tm_mday = d;
      t.tm_mon  = m-1;
      t.tm_year = y-1900;
      now = timegm(&t);
      if (now > maxtime) {
        maxtime = now;
        strcpy(timestamp, datetime);
        strcat(timestamp, " GMT");
      }
    }
    if (outf)
      fprintf(outf, "  %-10s  %-5s  %s\n", name, rev, datetime);
  }
  
  if (outf)
    fprintf(outf, "\n");
  
  if (outf)
    fprintf(outf, "\n");

  return 0;
}



