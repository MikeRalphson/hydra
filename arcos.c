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
/* Based on dosos.c */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "xmalloc.h"
#include "common.h"

char *getParam();

int overwrite_files = 0;
int didchat;

/* The name of the file we're writing */
static char *output_fname = 0;

/* Characters that can be in filenames */
#define GOODCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_!'()+/=?[]`{|}~" /* some of those symbols are a tad dubious */

static unsigned long load, exec, acc;
static fLoad, fExec, fAcc; /* flags to say if each set */

/* RISC OS needs this to find filetype, etc */
/* Quite likely to be an empty function for many ports */
/* no longer a hook function - rework at some point */
os_parameterhook(contentParams)
params contentParams;
{
    fLoad=fExec=fAcc=0;
    if (contentParams) {
        char *p;
        p = getParam(contentParams, "load");
        if (p) {
            if (*p=='&')
                p++;
            load=strtoul(p,NULL,16);
            fLoad=1;
        }
        p = getParam(contentParams, "exec");
        if (p) {
            if (*p=='&')
                p++;
            exec=strtoul(p,NULL,16);
            fExec=1;
        }
        p = getParam(contentParams, "access");
        if (p) {
            if (*p=='&')
                p++;
            acc=strtoul(p,NULL,16);
            fAcc=1;
        }
    }
}

static char *dir=NULL;
static int offset[10];
static int fFirst=0;

os_boundaryhookopen(depth)
int depth;
{
/*    printf("((( %d '%s'\n",depth,dir?dir:"(null)"); */
    if (dir) {
        offset[depth]=strlen(dir);
    } else {
        offset[depth]=0;
    }
    if (depth>1)
        fFirst=1;
}

os_boundaryhookclose(depth)
int depth;
{
/*    printf("))) %d '%s'\n",depth,dir?dir:"(null)"); */
    if (dir) {
        dir[offset[depth+1]]='\0';
    }
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
	if (!hostname) hostname="random-arc";
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

    if (p = getenv("Wimp$ScrapDir")) {
	strncpy(buf, p, 201);
	buf[200] = '\0';		/* Ensure sufficiently short */
    }
    else {
	strcpy(buf, "/tmp");
	(void)mkdir(buf);
    }
    strcat(buf, "/mimeparts");
    (void)mkdir(buf);

    p = buf + strlen(buf);
    *p++ = '/';

    while (*id && len < 10) {
	if (strchr(GOODCHARS, *id)) {
/*	    if (len++ == 8) *p++ = '.'; */
	    *p++ = *id;
	}
	id++;
    }
    *p = '\0';
    if (mkdir(buf) == -1 && errno != EACCES) {
	perror(buf);
	return 0;
    }
    *p++ = '/';
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
    int fRISCOS=0;
    extern int __uname_control;
    int uname_control_old;

    os_parameterhook(contentParams);

    if (!fname) fname = "";
    len=strlen(fname);

    if (cistrcmp(contentType,"APPLICATION/RISCOS")==0) {
        fRISCOS=1; /* It's definitely RISC OS */
        /* check for directory */
        if (len>5) {
            if (strcmp(fname+len-5,",1000")==0 ||
                strcmp(fname+len-5,",2000")==0) {
                /* OK -- it's a RISC OS directory */
                int lendir=0;
                fname[len-5]='\0';
                if (dir) {
                    lendir=strlen(dir);
                    dir=xrealloc(dir,lendir+len-3);
                    if (lendir)
                        dir[lendir++]='.';
                    strcpy(dir+lendir,fname);
                } else {
                    dir=xmalloc(len);
                    strcpy(dir,fname);
                }
                {
                    int r[6];
                    r[4]=0;
                    if (os_file (0x08, dir, r)) {
                        printf("couldn't create '%s'\n",dir);
                    }
                }
/*                printf("&&& just mkdir(%s)-ed -- fFirst=%d\n",dir,fFirst); */
                if (!fFirst) {
                    if (lendir) lendir--;
                    dir[lendir]='\0';
                }
                fFirst=0;
                return NULL; /* ignore dummy message body */
            }
        }
    }
/*    printf("*** file '%s'\n*** type '%s'\n",fname,contentType?contentType:"(null)"); */
    fFirst=0;
    /* Turn of UnixLib name mangling as it goes wrong for "foo.c.bar" */
    uname_control_old=__uname_control;
    __uname_control|=1;

    /* lose any RISC OS filetype (3 hex digits) appended after a ',' */
    if (len>4 && fname[len-4]==',') {
        int x;
        fRISCOS=1;
        for( x=len-3 ; x<len ; x++ )
            if (!isxdigit(fname[x]))
                fRISCOS=0;
        if (fRISCOS)
            fname[len-4]='\0';
    }
    if (!fRISCOS) {
        /* Chop off any drive specifier, convert \ to / */
        if (*fname && fname[1] == ':') fname +=2;
        for (p = fname; *p; p++) if (*p == '\\') *p = '/';

        /* If absolute path name, chop to tail */
        if (*fname == '/') {
            p = strrchr(fname, '/');
            fname = p+1;
        }
    }
    /* similarly for RISC OS pathnames */
    if (strchr(fname,'$') || strchr(fname,'&') || strchr(fname,':')) {
        fRISCOS=1;
	p = strrchr(fname, '.');
	fname = p+1;
    }

    if (fRISCOS && dir && *dir) {
        char *p;
        p=xmalloc(strlen(fname)+strlen(dir)+2);
        strcpy(p,dir);
        strcat(p,".");
        strcat(p,fname);
        fname=p;
    }

    if (!fRISCOS) {
    /* Clean out bad characters, create directories along path */
    for (p=q=fname, len=sawdot=0; *p; p++) {
	if (*p == '/') {
	    if (!strncmp(p, "/../", 4)) {
		p[1] = p[2] = 'X';
	    }
	    *q = '\0';
	    (void) mkdir(fname);
	    *q++ = '/';
	    len = sawdot = 0;
	}
	else if (*p == '.' && !sawdot) {
	    *q++ = '.';
	    sawdot++;
	    len = 0;
	}
	else if (/*len < (sawdot ? 3 : 8) &&*/ strchr(GOODCHARS, *p)) {
	    *q++ = *p;
	    len++;
	}
    }
    *q = '\0';
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
	p = strrchr(fname, '/');
	if (!p) p = fname;
	p = strchr(p, '.');
	if (p) *p = '\0';

	/* append unique number */
	do {
	    fclose(outfile);
	    sprintf(buf, "%s/%d", fname, ++filesuffix);

	} while (outfile = fopen(buf, "r"));
	fname = buf;
    }

    outfile = fopen(fname, (flags & FILE_BINARY) ? "wb" : "w");
    if (!outfile) {
        printf("++++ wouldn't open!!!!\n");
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

    p = strrchr(descfname, '/');
    if (!p) p = descfname;
    if (p = strrchr(p, '.')) *p = '\0';

    strcat(descfname, "/dsc");
    (void) rename(TEMPFILENAME, descfname);
    if (descfname != buf) free(descfname);

    fprintf(stdout, "%s (%s)\n", output_fname, contentType);
    didchat = 1;

    __uname_control=uname_control_old;
    return outfile;
}

/*
 * Close a file opened by os_newTypedFile()
 */
os_closetypedfile(outfile)
FILE *outfile;
{
    fclose(outfile);

    if (fLoad || fExec || fAcc) {
        unsigned long args[6];
        char *p;
        args[2]=load;
        args[3]=exec;
        args[5]=acc;
        if (fLoad && fExec && fAcc) {
/*            printf("%x %x %x\n",load,exec,acc); */
            p=(char*)os_file(1,output_fname,args);
            if (p) printf("Failed to set file type/date/access '%s'\n",p+4);
        } else {
            if (fLoad) {
                p=(char*)os_file(2,output_fname,args);
                if (p) printf("Failed to set file load addr '%s'\n",p+4);
            }
            if (fExec) {
                p=(char*)os_file(3,output_fname,args);
                if (p) printf("Failed to set file exec addr '%s'\n",p+4);
            }
            if (fAcc) {
                p=(char*)os_file(5,output_fname,args);
                if (p) printf("Failed to set file access perms '%s'\n",p+4);
            }
        }
    }
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
