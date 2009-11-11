/* $Id: pty.c,v 1.6 2002/04/24 13:59:02 bsd Exp $ */

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

#include <fcntl.h>
#include <string.h>


char * pty_versionid = "$Id: pty.c,v 1.6 2002/04/24 13:59:02 bsd Exp $";


int allocate_pty(int * bank, int * unit)
{
  int fd;
  char pty [ 32 ];
  char banks[] = "ZYXWVUTSRQPzyxwvutsrqp";
  char units[] = "vutsrqponmlkjihgfedcba9876543210";
  int b, u, bno, uno;
  
  bno = 8;
  uno = 9;
  
  strncpy(pty, "/dev/ptyp0", sizeof(pty)-1);
  
  for (b=0; b<sizeof(banks); b++) {
    for (u=0; u<sizeof(units); u++) {
      pty[bno] = banks[b];
      pty[uno] = units[u];
      fd = open(pty, O_RDWR|O_NONBLOCK);
      if (fd >= 0) {
        *bank = banks[b];
        *unit = units[u];
        return fd;
      }
    }
  }
  
  return -1;
}

