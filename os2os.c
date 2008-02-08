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
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "xmalloc.h"
#include "common.h"

#ifdef __EMX__		/* This is for EMX + GCC */

#define EACCESS EACCES



#else			/* This is for IBM C++ */

#include <direct.h>
#define ENAMETOOLONG EOS2ERR

#endif			/* EMX or IBM CSet++ */

int overwrite_files = 0;
int didchat;
int mime_eas = 0;

/* The name of the file we're writing */
static char *output_fname = 0;

/* Characters that can be in filenames */
#define GOODCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" \
                  "0123456789-_^$~!#%&{}@`'()"

int my_mkdir(char *filename)
{
    PEAOP2 EABuf;
    APIRET rc;

    EABuf = 0;
    rc = DosCreateDir(filename,EABuf);
    
    switch (rc) {
	case NO_ERROR: 
	    return 0;
	case ERROR_FILENAME_EXCED_RANGE:
	    errno = EACCESS;
	    return 1;
	case ERROR_PATH_NOT_FOUND:
	    errno = ENOENT;
	    return -1;
	default:
	    errno = EACCESS;
	    return -1;
    }
}

void crunch_file(char *p, char **q)
{
    int len, sawdot;

    for (*q=p, len=sawdot=0; *p; p++) {
	if (*p == '.') {
	    if (!sawdot) {
		*(*q)++ = '.';
		sawdot++;
		len = 0;
	    }
	} else if (len < (sawdot ? 3:8)) {
	    *(*q)++ = *p;
	    len++;
	}
    }
    **q = '\0';
}

typedef struct
{
    ULONG cbList;
    ULONG oNext;
    BYTE fEA;
    BYTE cbName;
    USHORT cbValue;
    BYTE NameBuffer[CCHMAXPATH*2+8];
}
FEALST;

void EA_attach(int fd, char *name, char *value)
{
    EAOP2 eaop;
    FEALST fealst;
    int len;

    eaop.fpFEA2List = (PFEA2LIST) &fealst;
    eaop.fpGEA2List = NULL;
    eaop.oError = 0;

    fealst.oNext = 0;
    fealst.fEA = 0;

    fealst.cbName = strlen(name) ;
    fealst.cbValue = strlen(value) +4;

    len = fealst.cbName +1;
    strcpy(fealst.NameBuffer,name);

    *((USHORT *) &(fealst.NameBuffer [len])) = 0xFFFD;
    len += sizeof(USHORT);
    *((USHORT *) &(fealst.NameBuffer [len])) = strlen(value);
    len += sizeof(USHORT);
    strcpy(&(fealst.NameBuffer[len]), value);

    fealst.cbList = fealst.cbName+fealst.cbValue+9;
  
    DosSetFileInfo(fd, FIL_QUERYEASIZE, &eaop, sizeof(eaop));
}

/* Generate a message-id */
char *os_genid()
{
    static time_t curtime;
    static char *hostname;
    static char *domain;
    char *result;

    if (curtime == 0) {
	time(&curtime);

	hostname = getenv("HOSTNAME");
	if (!hostname) hostname="random-pc";
	domain = getenv("DOMAINNAME");
	if (!domain) domain="random-domain";
    }

    if (strchr(hostname,'.')) {
	result = xmalloc(25+strlen(hostname));
	sprintf(result, "%lu@%s", curtime++, hostname);
    } else {
	if (domain[0] == '.') domain++;
	result = xmalloc(25+strlen(hostname)+strlen(domain));
	sprintf(result, "%lu@%s.%s", curtime++, hostname, domain);
    }
    return result;
}

/* Create and return directory for a message-id */
char *os_idtodir(id)
char *id;
{
    static char buf[512];
    int len = 0;
    int cs = 0;
    int rc;
    char *p, *q;

    if (p = getenv("TMP")) {
	strncpy(buf, p, 201);
	p[200] = '\0';		/* Ensure sufficiently short */
    }
    else {
	strcpy(buf, "\\tmp");
	(void)my_mkdir(buf);
    }
    if (buf[strlen(buf)-1] != '\\')
	strcat(buf, "\\");

    strcat(buf, "parts");
    (void)my_mkdir(buf);

    p = buf + strlen(buf);
    *p++ = '\\';
    q = p;

    while (*id && len < 200) {
	if (strchr(GOODCHARS, *id)) {
	    *p++ = *id;
            len++;
	    cs += *id * len;
	}
	id++;
    }
    *p = '\0';
    rc = my_mkdir(buf);
    if (rc == 1) {
	char csarr[] = GOODCHARS;
	/* maybe 8.3 restriction. Take 8 char + 3 checksum */
	p = q;
	len = 0;
	while ( *p && len < 8) { len++; p++; }
	*p++ = '.';
	*p++ = csarr[cs % sizeof(csarr)]; 
	cs = cs / sizeof(csarr);
	*p++ = csarr[cs % sizeof(csarr)]; 
	cs = cs / sizeof(csarr);
	*p++ = csarr[cs % sizeof(csarr)];
	*p = '\0';
	rc = my_mkdir(buf);
    }
    if ((rc == -1) && (errno != EACCESS)) {
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
    char *p, *q, *r;
    int len=0;
    int fd=0;
    int filesuffix=0;
    int uselong=0;
    static int partno=0;
    char buf[256], lbuf[256];
    char *descfname=0;
    FILE *outfile = 0;
    FILESTATUS3 FileInfoBuf;

    if (!fname) fname = "";

    /* Chop off any drive specifier, convert / to \ */
    if (*fname && fname[1] == ':') fname +=2;

    for (p = fname; *p; p++) if (*p == '/') *p = '\\';

    /* If absolute path name, chop to tail */
    if (*fname == '\\') {
	p = strrchr(fname, '\\');
	fname = p+1;
    }

    /* Clean out bad characters, create directories along path */
    for (p=q=r=fname; *p; p++) {
	if (*p == '\\') {
	    if (!strncmp(p, "\\..\\", 4)) {
		p[1] = p[2] = 'X';
	    }
	    *q = '\0';
	    if (my_mkdir(fname) == 1) {
		crunch_file(r,&q);
		(void) my_mkdir(fname);
	    }
	    *q++ = '\\';
	    r = q;
	}
	else if ( (len < 200) && strchr(GOODCHARS ".", *p)) {
	    *q++ = *p;
	    len++;
	}
    }
    *q = '\0';

    if (!fname[0]) {
	sprintf(buf, "part%d",++partno);
	fname = buf;
    }

    do {
	errno = 0;
	fd = open(fname, O_CREAT | O_RDWR | (overwrite_files ? O_TRUNC : O_EXCL) | 
		   ((flags & FILE_BINARY) ? O_BINARY : O_TEXT), S_IREAD | S_IWRITE);

	if ((fd == -1) && (errno == ENAMETOOLONG)) {
	    r = strrchr(fname, '\\');
	    if (!r) r = fname;
	    strcpy(lbuf,r);
	    uselong=1;
	    crunch_file(r,&q);
	    fd = open(fname, O_CREAT | O_RDWR | (overwrite_files ? O_TRUNC : O_EXCL) | 
		       ((flags & FILE_BINARY) ? O_BINARY : O_TEXT), S_IREAD | S_IWRITE);
	}

	/* simulate sharing violation, if character device found */

	if (fd != -1) {
	    if (DosQueryFileInfo(fd, FIL_STANDARD, &FileInfoBuf, sizeof(FILESTATUS3))) {
		close(fd);
		fd = -1;
		errno = EACCESS; 
	    }
	}

	if ((fd == -1) && ((errno == EEXIST) || (errno == EACCESS))) {
	    /* chop off suffix */
	    if (fname != buf) {
		strcpy(buf,fname);
		fname=buf;
	    }

	    r = strrchr(fname, '\\');
	    if (!r) r = fname;
	    q = strchr(r, '.');
	    if (q) {
		if (*q) *q = '\0';
	    }
	    q = fname + strlen(fname);
	    sprintf(q, ".%d", ++filesuffix);
	    errno = 0;
	} 
    } while ((fd == -1) && (!errno));

    if (fd == -1) {
	perror(fname);
    } else {

	if (uselong) EA_attach(fd,".LONGNAME",lbuf);
	if (mime_eas) EA_attach(fd,"MIME-TYPE",contentType);
	outfile = fdopen(fd, (flags & FILE_BINARY) ? "wb" : "w");
	if (!outfile) {
	    close(fd);
	    perror(fname);
	}
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
os_binhex(infile, part, nparts)
FILE *infile;
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
