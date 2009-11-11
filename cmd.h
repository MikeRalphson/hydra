/* $Id: cmd.h,v 1.13 2002/05/14 22:00:22 bsd Exp $ */

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

#ifndef __cmd_h__
#define __cmd_h__


#include "endp.h"


#define MAX_COMMAND_LEN   32 /* max command name length */

#define MAX_HOST         128 /* max host name length */

#define MAX_RBUF        1024 /* buffer size for command port command line */

#define MAX_PROMPT        32 /* command connection prompt length */

#define MAX_CMD           64 /* max command name length */


/*
 * data structure for commands
 */
typedef struct {
  char * command;
  int    (*function)(ENDPOINT * e, char * cmdline);
  char * desc;
} COMMAND;


/*
 * Data structure for managing command connections
 */
struct command_port {
  struct hostent * h;                  /* remote host */
  char             name[MAX_HOST];     /* remote host */
  int              bufcnt;             /* remote host command buffer length */
  char           * buf;                /* remote host command buffer */
  char             lastc[2];           /* previous character(s) recieved */
  char             prompt[MAX_PROMPT]; /* remote host command prompt */
  int              echo;               /* boolean: echo chars in command mode */
};


extern COMMAND commands[];
#define N_COMMANDS (sizeof(commands)/sizeof(commands[0]))

/*
 * prototypes for commands implemented by the command interface
 */
int cmd_init      (void);
int hexdump_buf   (int fd, char * prefix, int startaddr, char * buf, int len);
int cmd_add       (ENDPOINT * e, char * cmdline);
int cmd_ctl       (ENDPOINT * e, char * cmdline);
int cmd_close     (ENDPOINT * e, char * cmdline);
int cmd_debug     (ENDPOINT * e, char * cmdline);
int cmd_devdir    (ENDPOINT * e, char * cmdline);
int cmd_endpoints (ENDPOINT * e, char * cmdline);
int cmd_help      (ENDPOINT * e, char * cmdline);
int cmd_list      (ENDPOINT * e, char * cmdline);
int cmd_logdir    (ENDPOINT * e, char * cmdline);
int cmd_restart   (ENDPOINT * e, char * cmdline);
int cmd_serve     (ENDPOINT * e, char * cmdline);
int cmd_set       (ENDPOINT * e, char * cmdline);
int cmd_show      (ENDPOINT * e, char * cmdline);
int cmd_shutdown  (ENDPOINT * e, char * cmdline);
int cmd_status    (ENDPOINT * e, char * cmdline);
int cmd_version   (ENDPOINT * e, char * cmdline);

COMMAND_PORT * new_cmd      (void);
COMMAND      * find_cmd     (char * cmd);
COMSERV_PORT * locate_devid (char * d);
int            run_command  (ENDPOINT * e);

#endif
