/* (C) Copyright 1993 by Mike W. Meyer
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
#include <exec/execbase.h>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "xmalloc.h"
#include "version.h"

#if defined(__SASC) && (__VERSION__ > 5) && (__REVISION__ > 50)
static const char DOSId[] = "\0$VER: MPack " MPACK_VERSION " " __AMIGADATE__ ;
#else
static const char DOSId[] = "\0$VER: MPack " MPACK_VERSION " (" __DATE__ ")" ;
#endif

#define MAXADDRESS 100

extern char *myGetConfig(char *, char *);
extern int errno;
extern int optind;
extern char *optarg;
struct NetSupportLibrary *NetSupportBase;

#define TEMPLATE "From/A,Dest=Destination/M,-o=To/K,-s=Subject/K,-d=Description/K,-c=Contents/K,-m=SplitSize/K/N,-n=News/S"
enum {
        FROM ,
        DESTINATION ,
        TO ,
        SUBJECT ,
        DESCRIPTION ,
        CONTENTS ,
        SPLITSIZE ,
        NEWS ,
        OPT_COUNT
        } ;

#define HELPSTRING "mpack version " MPACK_VERSION "\n\
Pack the given file into a MIME message for sending to a remote machine\n\
by either mail or news. Automatically splits the file into a multi-part\n\
message if it is larger than splitsize. The options are:\n\
From/A                  The file you are going to send.\n\
Dest=Destination/M      One or more electronic mail addresses or newsgroups.\n\
                        May not be used with the To option.\n\
-o=To/K                 A file  to output to. The message will be written to\n\
                        the given file name. If more than one message is\n\
                        needed, this is the base name, and a sequence number\n\
                        is provided as a suffix.\n\
-s=Subject/K            Subject of the mail message or article. Will be put\n\
                        in each message sent, with sequence numbers appended\n\
                        for more than one messages.\n\
-d=Description/K        The name of a file that describes the file being\n\
                        sent. Add more here later, Mike.\n\
-c=Contents/K           Mime content-type field for the file being sent. The\n\
                        default is application/octet-stream (binary data)\n\
                        but some other MIME types will be recognized.\n\
-m=SplitSize/K/N        Maximum size of a single message in bytes. The\n\
                        default is taken from the environment variable\n\
                        SPLITSIZE. If that is not defined, then there is no\n\
                        limit.\n\
-n=News/S               Causes the destinations to be interpreted as\n\
                        newsgroups to be posted to instead of electronic mail\n\
                        addresses. Has no effect if there are no destinations\n\
                        (i.e. - To is used to write to a file).\n"

/* The one thing we have to fre by hand */
static struct RDArgs *my_args = NULL, *args = NULL ;

/* A simple utilities */
void post(char *, char *) ;
void warn(char *) ;

void
FreeSystem(void) {

        if (NetSupportBase) {
                UnLockFiles() ;
                CloseLibrary((struct Library *) NetSupportBase) ;
                }
        if (args) FreeArgs(args) ;
        if (my_args) FreeDosObject(DOS_RDARGS, my_args) ;
        }

int
main(int argc, char **argv) {
        FILE *infile, *descfile;
        char *p, **pp, *from, *to, *subject, *description, *contents ;
        char *header = NULL ;
        char **destination ;
        long news, count, splitsize = 0 ;
        char buffer[512] ;
        long part, opts[OPT_COUNT] ;

        if (!(NetSupportBase = (struct NetSupportLibrary *) OldOpenLibrary(NETSUPPORTNAME)))
                fprintf(stderr,
                        "No NetSupport.Library: Can't parse configfiles.\n");

        onexit(FreeSystem) ;
        memset((char *) opts, 0, sizeof(opts)) ;
        if ((p = myGetConfig("SPLITSIZE", NULL)) && *p >= '0' && *p <= '9')
                splitsize = atoi(p) ;

        opts[SPLITSIZE] = (long) &splitsize ;

        if (!(my_args = AllocDosObject(DOS_RDARGS, NULL))) {
                PrintFault(IoErr(), *argv) ;
                exit(RETURN_FAIL) ;
                }
        my_args->RDA_ExtHelp = HELPSTRING ;
        if (!(args = ReadArgs(TEMPLATE, opts, my_args))) {
                PrintFault(IoErr(), *argv) ;
                exit(RETURN_FAIL) ;
                }

        from = (char *) opts[FROM] ;
        to = (char *) opts[TO] ;
        subject = (char *) opts[SUBJECT] ;
        description = (char *) opts[DESCRIPTION] ;
        contents = (char *) opts[CONTENTS] ;
        news = opts[NEWS] ;
        splitsize = *((long *) opts[SPLITSIZE]) ;
        destination = (char **) opts[DESTINATION] ;

        /* Make sure we can open the description file. */
        if (description) {
                if (!(descfile = fopen(description, "r"))) {
                        fprintf(stderr, "Can't open the description file \"%s\"!\n", description);
                        exit(RETURN_ERROR) ;
                }
        }
        else
                descfile = NULL;

        /* Make sure we're sending something reasonable. */
        if (contents) {
                if (!cistrncmp(contents, "text/", 5)) {
                        fprintf(stderr, "This program is not appropriate for encoding textual data\n") ;
                        exit(RETURN_ERROR) ;
                        }
                if (cistrncmp(contents, "application/", 12)
                && cistrncmp(contents, "audio/", 6)
                && cistrncmp(contents, "image/", 6)
                && cistrncmp(contents, "video/", 6)) {
                        fprintf(stderr, "Content type must be subtype of application, audio, image, or video\n") ;
                        exit(RETURN_ERROR) ;
                        }
                }

        /* Gotta have something to send! */
        if (!from) {
                fprintf(stderr, "The From argument is required\n") ;
                exit(RETURN_ERROR) ;
                }

        /* We must have either To or Destinations, but not both! */
        if (to && destination) {
                fprintf(stderr, "The To keyword and Destination are mutually exclusive.\n") ;
                exit(RETURN_ERROR) ;
                }
        else if (!to && !destination) {
                fprintf(stderr, "Either a destination or the To keyword is required\n");
                exit(RETURN_ERROR) ;
                }

        /* And we gotta have a subject! */
        if (!subject) {
                fputs("Subject: ", stdout) ;
                fflush(stdout) ;
                if (!fgets(buffer, sizeof(buffer), stdin)) {
                        fprintf(stderr, "A subject is required\n") ;
                        exit(RETURN_ERROR) ;
                        }
                if (p = strchr(buffer, '\n')) *p = '\0' ;
                subject = buffer ;
                }

        /* Build the To: or Newsgroups: line */
        if (destination) {
                for (count = 25, pp = destination; *pp; pp += 1)
                        count += strlen(*pp) + 3 ;
                header = xmalloc(count) ;
                p = stpcpy(header, news ? "Newsgroups: " : "To: ") ;
                p = stpcpy(p, *destination) ;
                for (pp = destination + 1; *pp; pp += 1) {
                        p = stpcpy(p, news ? "," : ",\n\t") ;
                        p = stpcpy(p, *pp) ;
                        }
                stpcpy(p, "\n") ;
                }

        /* Get a name to put the output into */
        if (!to) to = tmpnam(NULL) ;

        /* Sigh */
        infile = fopen(from, "r");
        if (!infile) {
                os_perror(from);
                exit(1);
                }

        if (encode(infile, (FILE *) 0, from, descfile, subject, header, splitsize, contents, to))
                exit(RETURN_FAIL) ;

        /* Hey, we did it. Now send it if we need to */
        if (destination)
                if (!access(to, R_OK)) {
                        post(to, news ? POSTNEWS : SENDMAIL) ;
                        remove(to) ;
                        }
                else
                        for (part = 1;; part += 1) {
                                sprintf(buffer, "%s.%02d", to, part) ;
                                if (access(buffer, R_OK)) break ;
                                post(buffer, news ? POSTNEWS : SENDMAIL) ;
                                remove(buffer) ;
                                }

        exit(RETURN_OK) ;
        }

void
post(char *name, char *command) {
        char buffer[512] ;

        sprintf(buffer, "%s < %s", myGetConfig(command, command), name);
        system(buffer) ;
        }

void
warn(char *s) {
        abort() ;
        }
