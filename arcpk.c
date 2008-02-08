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
/* this is almost exactly the same as dospk.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "version.h"
#include "xmalloc.h"

#define MAXADDRESS 100

/* dummy defn, since arcos.c calls this function */
char *getParam(cParams, key) {
    return;
}

extern int optind;
extern char *optarg;

main(argc, argv)
int argc;
char **argv;
{
    int opt;
    char *fname = 0;
    char *subject = 0;
    char *descfname = 0;
    long maxsize = 0;
    char *outfname = 0;
    char *ctype = 0;
    char *headers = 0;
    int i;
    char *p;
    char sbuf[1024];
    char fnamebuf[4096];
    int part;
    FILE *infile;
    FILE *descfile = 0;

    if ((p = getenv("SPLITSIZE")) && *p >= '0' && *p <= '9') {
	maxsize = atoi(p);
    }

    while ((opt = getopt(argc, argv, "s:d:m:c:o:n:")) != EOF) {
	switch (opt) {
	case 's':
	    subject = optarg;
	    break;

	case 'd':
	    descfname = optarg;
	    break;

	case 'm':
	    maxsize = atol(optarg);
	    break;

	case 'c':
	    ctype = optarg;
	    break;

	case 'o':
	    outfname = optarg;
	    break;

	default:
	    usage();

	}
    }

    if (ctype) {
	if (!cistrncmp(ctype, "text/", 5)) {
	    fprintf(stderr, "This program is not appropriate for encoding textual data\n");
	    exit(1);
	}
	if (cistrncmp(ctype, "application/", 12) && cistrncmp(ctype, "audio/", 6) &&
	    cistrncmp(ctype, "image/", 6) && cistrncmp(ctype, "video/", 6)) {
	    fprintf(stderr, "Content type must be subtype of application, audio, image, or video\n");
	    exit(1);
	}
    }

    if (!outfname) {
	fprintf(stderr, "The -o switch is required\n");
	usage();
    }

    if (optind == argc) {
	fprintf(stderr, "An input file must be specified\n");
	usage();
    }
    fname = argv[optind++];
    for (p = fname; *p; p++) {
	if (isupper(*p)) *p = tolower(*p);
    }

    /* Must have -o */
    if (optind != argc) {
	fprintf(stderr, "Too many arguments\n");
	usage();
    }

    if (!subject) {
	fputs("Subject: ", stdout);
	fflush(stdout);
	if (!fgets(sbuf, sizeof(sbuf), stdin)) {
	    fprintf(stderr, "A subject is required\n");
	    usage();
	}
	if (p = strchr(sbuf, '\n')) *p = '\0';
	subject = sbuf;
    }

    infile = fopen(fname, "rb");
    if (!infile) {
	os_perror(fname);
	exit(1);
    }

    if (descfname) {
	descfile = fopen(descfname, "r");
	if (!descfile) {
	    os_perror(descfname);
	    exit(1);
	}
    }

    if (encode(infile, (FILE *)0, fname, descfile, subject, headers,
	       maxsize, ctype, outfname)) exit(1);

    exit(0);
}

usage()
{
    fprintf(stderr, "mpack version %s\n", MPACK_VERSION);
#ifdef TEST_VERSION
    fprintf(stderr, "*** Arc "TEST_VERSION".  Report problems to olly@mantis.co.uk ***\n");
#endif
    fprintf(stderr,
"usage: mpack [-s subj] [-d file] [-m maxsize] [-c content-type] -o output file\n");
    exit(1);
}

warn()
{
    abort();
}
