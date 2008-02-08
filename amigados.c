/* (c) Copyright 1993 by Mike W. Meyer
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Mike W.
 * Meyer not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Mike W. Meyer makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MIKE W. MEYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MIKE W. MEYER BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <exec/types.h>
#include <exec/exec.h>
#include <dos/dos.h>

#include <libraries/netsupport.h>

#ifdef __SASC
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/netsupport.h>
#else
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/netsupport_protos.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>
#include <ctype.h>
#include <errno.h>

#include "xmalloc.h"
#include "common.h"

#define BADCHARS        "#?()|[]%<>:;$&*\\\ \t\`\""
#define FILELEN         29              /* Max file name length */

int overwrite_files = 0 ;
static char *output_fname = NULL;
extern int errno, _OSERR ;
void os_perror(char *) ;
char *myGetConfig(char *, char *) ;
int __buffsize = 8192 ;
int didchat;

/* Generate a message-id */
char *
os_genid(void) {
        static struct Task *task = NULL ;
        static time_t now ;
        static char hostname[32], domainname[32] ;
        char *result ;

        if (task == NULL) {
                task = FindTask(NULL) ;
                time(&now) ;

                strcpy(hostname, myGetConfig(NODENAME, "random-amiga"));
                if (strchr(hostname, '.')) domainname[0] = '\0' ;
                else strcpy(domainname, myGetConfig(DOMAINNAME, ".random-domain"));
                if (domainname[0] && domainname[0] != '.') strcat(hostname, ".") ;
                }
        result = malloc(25 + strlen(hostname) + strlen(domainname)) ;
        sprintf(result, "%d.%d@%s%s", (long) task, now++, hostname, domainname) ;
        return result ;
        }

/* Create and return a directory for a message-id */
char *
os_idtodir(char *id) {
static char buf[512] ;
        char *p, save ;

        strcpy(buf, myGetConfig("METAMAIL_P_DIR", "T:"));

        if (!(p = myGetConfig("USERNAME", NULL)))
                p = myGetConfig("USER", "anonymous");
        AddPart(buf, p, sizeof(buf)) ;

        if (mkdir(buf) == -1 && errno != EEXIST && _OSERR != ERROR_OBJECT_EXISTS) {
                os_perror(buf) ;
                return NULL ;
                }

        p = buf + strlen(buf) ;
        *p++ = '/' ;
        save = id[FILELEN] ;
        id[FILELEN] = '\0' ;
        while (*id) {
                if (isprint(*id) && !strchr(BADCHARS, *id)) *p++ = *id ;
                id += 1 ;
                }
        *p  = '\0' ;
        id[FILELEN] = save ;

        if (mkdir(buf) == -1 && errno != EEXIST && _OSERR != ERROR_OBJECT_EXISTS) {
                os_perror(buf) ;
                return NULL ;
                }
        strcpy(p, "/") ;
        return buf ;
        }

/* Delete the directory created by os_idtodir() */
void
os_donewithdir(char *dir) {

        /* Chop off trailing slash */
        dir[strlen(dir) - 1] = '\0' ;
        rmdir(dir) ;
        }

/* Create a new file of name "fname". Clean up the name first */
FILE *
os_newtypedfile(char *fname, char *contentType, int binary, params contentParams) {
        char *p, *name, *description, buf[512] ;
        FILE *outfile ;
        static int filesuffix = 0 ;

        name = FilePart(fname) ;

        /* No BADCHARS */
        for (p = name; *p; p += 1)
                if (!isprint(*p) || strchr(BADCHARS, *p)) *p = 'X' ;

        /* Add a count if we don't have a name, or we aren't overwriting */
        if (!*fname || !overwrite_files) {
                if (*name) strcpy(buf, name) ;
                else {
                        name = "part" ;
                        sprintf(buf, "part.%d", ++filesuffix) ;
                        }
                while (outfile = fopen(buf, "r")) {
                        if (outfile) fclose(outfile) ;
                        sprintf(buf, "%s.%d", name, ++filesuffix) ;
                        }
                name = buf ;
                fclose(outfile) ;
                }

        if (!(outfile = fopen(name, "w"))) os_perror(name) ;

        if (output_fname) free(output_fname) ;
        output_fname = strsave(name) ;
        description = xmalloc(strlen(name) + 6) ;
        strcpy(description, name) ;
        strcat(description, ".desc") ;
        (void) rename(TEMPFILENAME, description) ;
        free(description) ;

        fprintf(stdout, "%s (%s)\n", output_fname, contentType) ;
        didchat = 1;
        return outfile ;
        }

/* Warn user that MD5 digest didn't match */
void
os_warnMD5mismatch(void) {
        char *warning;

        warning = xmalloc(strlen(output_fname) + 100);
        sprintf(warning, "%s was corrupted in transit", output_fname);
        warn(warning);
        free(warning);
        }

/* Report an error (in errno) concerning a filename */
void
os_perror(char *file) {
        if (errno != EOSERR) perror(file);
        else poserr(file) ;
        }

/*
 * This getenv will use the WB2.0 calls if you have the 2.0
 * rom. If not, it resorts to looking in the ENV: directory.
 */

char *
getenv (const char *name) {
        FILE *fp;
        char *ptr;
        static char value[256] ;
        static char buf[256] ;

        /* 2.0 style? */
        if (DOSBase->dl_lib.lib_Version >= 36) {
                if (GetVar ((char *)name,value,256,0L) == -1) return NULL;
        } else {
                if (strlen (name) > 252) return NULL;
                strcpy (buf,"ENV:");
                strcpy (&buf[4],name);
                if (! (fp = fopen(buf,"r"))) return NULL;
                for (ptr = value; (*ptr=getc(fp))!=EOF
                        && *ptr != '\n'
                        && ++ptr < &value[256];);
                fclose(fp);
                *ptr = 0;
                }
        return value;
        }

/* Get configuarion, either from a file or from a variable */

char * myGetConfig(char *keyword, char *def) {
        char *entry;

        if (NetSupportBase)
                return (char *) GetConfig(NULL, keyword, NULL, def);


        entry = getenv(keyword);  /* Library is NOT available */
        return ((entry) ? entry : def);
        }

/* (Don't) Handle a BinHex'ed file */

int
os_binhex(FILE *infile, int dummy1, int dummy2) {
        return 1;
        }
/*
 * Close a file opened by os_newTypedFile()
 */
int
os_closetypedfile(FILE *outfile) {
        return fclose(outfile);
        }
