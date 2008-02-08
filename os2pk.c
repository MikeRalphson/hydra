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
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "version.h"
#include "xmalloc.h"

#define MAXADDRESS 100

extern int optind;
extern char *optarg;

main(argc, argv)
int argc;
char **argv;
{
    int opt;
    char *fname = 0;
    char *from = 0;
    char *subject = 0;
    char *descfname = 0;
    long maxsize = 0;
    char *outfname = 0;
    char *newsgroups = 0;
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

    if (!(from = getenv("LOGNAME"))) from="postmaster";

    while ((opt = getopt(argc, argv, "s:d:m:c:o:n:f:")) != EOF) {
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

	case 'n':
	    newsgroups = optarg;
	    break;

	case 'c':
	    ctype = optarg;
	    break;

        case 'f':
            from = optarg;
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

    if (optind == argc) {
	fprintf(stderr, "An input file must be specified\n");
	usage();
    }
    fname = argv[optind++];

    /* Must have exactly one of -o, -n, or destination addrs */
    if (optind == argc) {
	if (outfname && newsgroups) {
	    fprintf(stderr, "The -o and -n switches are mutually exclusive.\n");
	    usage();
	}
	if (!outfname && !newsgroups) {
	    fprintf(stderr, "Either an address or one of the -o or -n switches is required\n");
	    usage();
	}
	if (newsgroups) {
	    headers = xmalloc(strlen(newsgroups) + strlen(from) +50);
	    sprintf(headers, "Newsgroups: %s\nFrom: %s\n", newsgroups, from);
	}
    }
    else {
	if (outfname) {
	    fprintf(stderr, "The -o switch and addresses are mutually exclusive.\n");
	    usage();
	}
	if (newsgroups) {
	    fprintf(stderr, "The -n switch and addresses are mutually exclusive.\n");
	    usage();
	}
	headers = xmalloc(strlen(argv[optind]) + 50);
	sprintf(headers, "To: %s", argv[optind]);
	for (i = optind+1; i < argc; i++) {
	    headers = xrealloc(headers, strlen(headers)+strlen(argv[i]) + 25);
	    strcat(headers, ",\n\t");
	    strcat(headers, argv[i]);
	}
	strcat(headers, "\n");
        strcat(headers, "From: ");
        strcat(headers, from);
        strcat(headers, "\n");
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

    if (!outfname) {
	if (getenv("TMP")) {
	    strcpy(fnamebuf, getenv("TMP"));
	}
	else {
	    strcpy(fnamebuf, "c:\\tmp");
	}
	strcat(fnamebuf, "\\mpack.000");
	outfname = strsave(fnamebuf);
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

    if (optind < argc || newsgroups) {
	for (part = 0;;part++) {
	    sprintf(fnamebuf, "%s.%02d", outfname, part);
	    infile = fopen(part ? fnamebuf : outfname, "r");
	    if (!infile) {
		if (part) break;
		continue;
	    }
	    if (newsgroups) {
		inews(infile, newsgroups);
	    }
	    else {
		sendmail(infile, argv, argc, optind, from);
	    }
	    fclose(infile);
	    remove(part ? fnamebuf : outfname);
	}
    }

    exit(0);
}

usage()
{
    fprintf(stderr, "mpack version %s\n", MPACK_VERSION);
    fprintf(stderr,
"usage: mpack [-s subj] [-d file] [-m maxsize] [-c content-type] [-f address] file address...\n");
    fprintf(stderr,
"       mpack [-s subj] [-d file] [-m maxsize] [-c content-type] -o file file\n");
    fprintf(stderr,
"       mpack [-s subj] [-d file] [-m maxsize] [-c content-type] [-f address] -n groups file\n");
    exit(1);
}

sendmail(infile, addr, cnt, start, from)
FILE *infile;
char **addr;
int cnt;
int start;
char *from;
{
    int status;
    int pid;
    int oldfh;
    int i;
    char *p;
    char **argv;
    
    argv = (char **) xmalloc( (cnt-start+5)*sizeof(char *));
    i = 0;

    if(!(p = getenv("SENDMAIL"))) p="sendmail.exe";

    argv[i++] = p;
    argv[i++] = "-f";
    argv[i++] = from;
/*  argv[i++] = "-oi"; */
/* Currently not supported by IBM sendmail, but ignore dot is default */ 

    while (cnt-start) argv[i++] = addr[start++];
    argv[i] = NULL;
    oldfh = dup(0);
    dup2(fileno(infile), 0);
    fclose(infile);
    spawnvp(P_WAIT,p, argv);
    dup2(oldfh,0);
    close(oldfh);
    free(argv);
}

inews(infile)
FILE *infile;
{
    int status;
    int pid;
    int oldfh;
    char *p;

    if (!(p=getenv("NEWSPOST"))) p="inews.exe";

    oldfh = dup(0);
    dup2(fileno(infile), 0);
    fclose(infile);
    spawnlp(P_WAIT,p , p, "-h", (char *)0);
    dup2(oldfh,0);
    close(oldfh);
}

warn()
{
    abort();
}

