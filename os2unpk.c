/* (C) Copyright 1993,1994 by Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Carnegie
 * Mellon University not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  Carnegie Mellon University makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#include <os2.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "version.h"
#include "part.h"
#include "xmalloc.h"

#ifndef __EMX__
#include <direct.h>
#endif

extern int optind;
extern char *optarg;

extern int overwrite_files;
extern int didchat;
extern int mime_eas;
int quiet;

main(argc, argv)
int argc;
char **argv;
{
    int opt;
    FILE *file;
    char *p, *q, *path = 0;
    int extractText = 0;
    int done, gotone;
    HDIR Findhandle;
    ULONG FindCount;
    FILEFINDBUF3 ffblk;
    
    DosError(0); /* switch off these annoying OS/2 dialog boxes */

    while ((opt = getopt(argc, argv, "eqftC:")) != EOF) {
	switch (opt) {
	case 'f':
	    overwrite_files = 1;
	    break;

	case 'q':
	    quiet = 1;
	    break;

	case 'e':
	    mime_eas = 1;
	    break;

	case 't':
	    extractText = 1;
	    break;

	case 'C':
	    if (chdir(optarg)) {
		perror(optarg);
		exit(1);
	    }
	    break;

	default:
	    usage();
	}
    }

    if (optind == argc) {
	fprintf(stderr, "munpack: reading from standard input\n");
	didchat = 0;
	handleMessage(part_init(stdin), "text/plain", 0, extractText);
	if (!didchat) {
	    fprintf(stdout,
		    "Did not find anything to unpack from standard input\n");
	}
	exit(0);
    }

    while (argv[optind]) {
	if (path) free(path);
	path = 0;

	p = argv[optind];
	if (p[1] == ':') p += 2;
	if (q = strrchr(p, '\\')) p = q+1;
	if (q = strrchr(p, '/')) p = q+1;

	if (p != argv[optind]) {
	    path = xmalloc(strlen(argv[optind])+20);
	    strcpy(path, argv[optind]);
	    p = path + (p - argv[optind]);
	}

        FindCount=1;
        Findhandle=1;
	gotone = 0;
	done = DosFindFirst(argv[optind], &Findhandle, 0, &ffblk,sizeof(ffblk),
                            &FindCount,FIL_STANDARD);
	while (!done) {
	    if (path) strcpy(p, ffblk.achName);
	    file = fopen(path ? path : ffblk.achName, "r");
	    if (!file) {
		perror(path ? path : ffblk.achName);
	    }
	    else {
	        didchat = 0;
		handleMessage(part_init(file), "text/plain", 0, extractText);
		if (!didchat) {
		    fprintf(stdout, 
			    "Did not find anything to unpack from %s\n",
			    argv[optind]);
		}
		fclose(file);
	    }
	    gotone = 1;
	    done = DosFindNext(Findhandle,&ffblk,sizeof(ffblk),&FindCount);
	}
	if (!gotone) {
	    perror(argv[optind]);
	}
	optind++;
    }
    exit(0);
}

usage() {
    fprintf(stderr, "munpack version %s\n", MPACK_VERSION);
    fprintf(stderr, "usage: munpack [-f] [-q] [-t] [-e] [-C directory] [files...]\n");
    exit(1);
}

warn(s)
char *s;
{
    fprintf(stderr, "munpack: warning: %s\n", s);
}

chat(s)
char *s;
{
    didchat = 1;
    if (!quiet) fprintf(stdout, "%s\n", s);
}
