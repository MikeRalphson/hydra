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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "xmalloc.h"
#include "common.h"

int overwrite_files = 0;
int didchat;

/* The name of the file we're writing */
static char *output_fname = 0;

/* Characters that can be in filenames */
#define GOODCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" \
                  "0123456789-_^$~!#%&{}@`'()"

/* Returns nonzero if "fname" is one of those magic device filenames */
static int
bad_filename(fname)
char *fname;
{
    int len = strlen(fname);
    if (len == 3) {
	if (!cistrcmp(fname, "aux") || !cistrcmp(fname, "con") ||
	    !cistrcmp(fname, "nul") || !cistrcmp(fname, "prn")) return 1;
    }
    if (len == 4) {
	if (!cistrcmp(fname, "com1") || !cistrcmp(fname, "com2") ||
	    !cistrcmp(fname, "com3") || !cistrcmp(fname, "com4") ||
	    !cistrcmp(fname, "lpt1") || !cistrcmp(fname, "lpt2") ||
	    !cistrcmp(fname, "lpt3")) return 1;
    }
    return 0;
}

/* Generate a message-id */
char *os_genid()
{
    static time_t curtime;
    static char *hostname;
    char *result;

    if (curtime == 0) {
	time(&curtime);

	hostname = getenv("HOSTNAME");
	if (!hostname) hostname="random-pc";
    }

    result = xmalloc(25+strlen(hostname));
    sprintf(result, "%lu@%s", curtime++, hostname);
    return result;
}

/* Create and return directory for a message-id */
char *os_idtodir(id)
char *id;
{
    static char buf[256];
    int len = 0;
    char *p;

    if (p = getenv("TMP")) {
	strncpy(buf, p, 201);
	buf[200] = '\0';		/* Ensure sufficiently short */
    }
    else {
	strcpy(buf, "\\tmp");
	(void)mkdir(buf);
    }
    strcat(buf, "\\parts");
    (void)mkdir(buf);

    p = buf + strlen(buf);
    *p++ = '\\';

    while (*id && len < 11) {
	if (strchr(GOODCHARS, *id)) {
	    if (len++ == 8) *p++ = '.';
	    *p++ = *id;
	}
	id++;
    }
    *p = '\0';
    if (!len || bad_filename(p-len)) {
	*p++ = 'x';
	*p = '\0';
    }
    if (mkdir(buf) == -1 && errno != EACCES) {
	perror(buf);
	return 0;
    }
    *p++ = '\\';
    *p = '\0';
    return buf;
}

/*
 * We are done with the directory returned by os_idtodir()
 * Remove it
 */
os_donewithdir(dir)
char *dir;
{
    char *p;

    /* Remove trailing slash */
    p = dir + strlen(dir) - 1;
    *p = '\0';

    rmdir(dir);
}

/*
 * Create a new file, with suggested filename "fname".
 * "fname" may have come from an insecure source, so clean it up first.
 * It may also be null.
 * "contentType" is passed in for use by systems that have typed filesystems.
 * "flags" contains a bit pattern describing attributes of the new file.
 */
FILE *os_newtypedfile(fname, contentType, flags, contentParams)
char *fname;
char *contentType;
int flags;
params contentParams;
{
    char *p, *q;
    int len, sawdot;
    static int filesuffix=0;
    char buf[128], *descfname=0;
    FILE *outfile = 0;

    if (!fname) fname = "";

    /* Chop off any leading drive specifier, convert / to \ */
    if (*fname && fname[1] == ':') fname +=2;
    for (p = fname; *p; p++) if (*p == '/') *p = '\\';

    /* If absolute path name, chop to tail */
    if (*fname == '\\') {
	p = strrchr(fname, '\\');
	fname = p+1;
    }

    /* Clean out bad characters, create directories along path,
     * trim names to 8.3
     */
    for (p=q=fname, len=sawdot=0; *p; p++) {
	if (*p == '\\') {
	    /* Don't let them go up a directory: change ".." to "XX" */
	    if (!strncmp(p, "\\..\\", 4)) {
		p[1] = p[2] = 'X';
	    }
	    *q = '\0';

	    /* Check for magic device filename */
	    if (!sawdot && bad_filename(q-len)) {
		*--q = '\0';
	    }

	    (void) mkdir(fname);
	    *q++ = '\\';
	    len = sawdot = 0;
	}
	else if (*p == '.' && !sawdot) {
	    /* First check for magic device filenames */
	    *q = '\0';
	    if (bad_filename(q-len)) q--;
	    
	    *q++ = '.';
	    sawdot++;
	    len = 0;
	}
	else if (len < (sawdot ? 3 : 8) && strchr(GOODCHARS, *p)) {
	    *q++ = *p;
	    len++;
	}
    }
    *q = '\0';

    /* Check for magic device filename */
    if (!sawdot && bad_filename(q-len)) {
	*--q = '\0';
    }

    if (!fname[0]) {
	do {
	    if (outfile) fclose(outfile);
	    sprintf(buf, "part%d", ++filesuffix);
	} while (outfile = fopen(buf, "r"));
	fname = buf;
    }
    else if (!overwrite_files && (outfile = fopen(fname, "r"))) {
	/* chop off suffix */
	p = strrchr(fname, '\\');
	if (!p) p = fname;
	p = strchr(p, '.');
	if (p) *p = '\0';

	/* append unique number */
	do {
	    fclose(outfile);
	    sprintf(buf, "%s.%d", fname, ++filesuffix);
	 
	} while (outfile = fopen(buf, "r"));
	fname = buf;
    }

    outfile = fopen(fname, (flags & FILE_BINARY) ? "wb" : "w");
    if (!outfile) {
	perror(fname);
    }

    if (output_fname) free(output_fname);
    output_fname = strsave(fname);

    if (strlen(fname) > sizeof(buf)-6) {
	descfname = xmalloc(strlen(fname)+6);
    }
    else {
	descfname = buf;
    }
    strcpy(descfname, fname);

    p = strrchr(descfname, '\\');
    if (!p) p = descfname;
    if (p = strrchr(p, '.')) *p = '\0';

    strcat(descfname, ".dsc");
    (void) rename(TEMPFILENAME, descfname);
    if (descfname != buf) free(descfname);
    
    fprintf(stdout, "%s (%s)\n", output_fname, contentType);
    didchat = 1;

    return outfile;
}

/*
 * Close a file opened by os_newTypedFile()
 */
os_closetypedfile(outfile)
FILE *outfile;
{
    fclose(outfile);
}

/*
 * (Don't) Handle a BinHex'ed file
 */
int
os_binhex(inpart, part, nparts)
struct part *inpart;
int part;
int nparts;
{
    return 1;
}

/*
 * Warn user that the MD5 digest of the last file created by os_newtypedfile()
 * did not match that supplied in the Content-MD5: header.
 */
os_warnMD5mismatch()
{
    char *warning;

    warning = xmalloc(strlen(output_fname) + 100);
    sprintf(warning, "%s was corrupted in transit",
	    output_fname);
    warn(warning);
    free(warning);
}

/*
 * Report an error (in errno) concerning a filename
 */
os_perror(file)
char *file;
{
    perror(file);
}
