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
#include <errno.h>
#include "version.h"

extern int didchat;
extern int overwrite_files;

#if defined(__SASC) && (__VERSION__ > 5) && (__REVISION__ > 50)
static const char DOSId[] = "\0$VER: MPack " MPACK_VERSION " " __AMIGADATE__ ;
#else
static const char DOSId[] = "\0$VER: MPack " MPACK_VERSION " (" __DATE__ ")" ;
#endif

int quiet ;

#define TEMPLATE        "Files/M,-f=Overwrite/S,-C=Directory/K,-q=Quiet/S,-t=Text/S"
enum { FILES, OVERWRITE, DIRECTORY, QUIET, EXTRACTTEXT, OPT_COUNT } ;
static struct RDArgs *args = NULL, *my_args = NULL ;
BPTR    OldDir = NULL ;
struct NetSupportLibrary *NetSupportBase;

#define HELPSTRING "munpack version " MPACK_VERSION "\n\
Unpack input files. If no files are present, reads from standard in. The\n\
arguments other than file names are:\n\
-f=Overwrite/S          Causes the unpacked file to overwrite any existing\n\
                        files of the same name. Otherwise, an extension\n\
                        with a period and a number is added to the file\n\
                        name.\n\
-C=Directory/K          Change to the given directory before unpacking.\n\
                        Path names will be interpreted relative to the this\n\
                        directory, not the current one.\n\
-q=Quiet/S              Supress progress statements.\n\
-t=Text/S               Unpack the Text part fo multipart messages.\n"

void warn(char *) ;
void chat(char *) ;

void
FreeSystem(void) {

        if (NetSupportBase) {
                UnLockFiles() ;
                CloseLibrary((struct Library *) NetSupportBase) ;
                }
        if (args) FreeArgs(args) ;
        if (my_args) FreeDosObject(DOS_RDARGS, args) ;
        if (OldDir) UnLock(CurrentDir(OldDir)) ;
        }

main(int argc, char **argv) {
        long opts[OPT_COUNT] ;
        FILE *file ;
        int goodenough, extracttext ;
        extern struct ExecBase *SysBase ;

        NetSupportBase = (struct NetSupportLibrary *) OldOpenLibrary(NETSUPPORTNAME) ;
        goodenough = SysBase->LibNode.lib_Version > 36 ;

        /* Do the 2.x argument parsing stuff */
        if (!goodenough) opts[FILES] = argc ;
        else {
                onexit(FreeSystem) ;
                memset((char *) opts, 0, sizeof(opts)) ;
                if (!(my_args = AllocDosObject(DOS_RDARGS, NULL))) {
                        PrintFault(IoErr(), *argv) ;
                        exit(RETURN_FAIL) ;
                        }
                my_args->RDA_ExtHelp = HELPSTRING ;
                if (!(args = ReadArgs(TEMPLATE, opts, my_args))) {
                        PrintFault(IoErr(), *argv) ;
                        exit(RETURN_FAIL) ;
                        }
                overwrite_files = opts[OVERWRITE] ;
                quiet = opts[QUIET] ;
                extracttext = opts[EXTRACTTEXT] ;
                if (opts[DIRECTORY])
                        if (OldDir = Lock((char *) opts[DIRECTORY], SHARED_LOCK))
                                OldDir = CurrentDir(OldDir) ;
                        else PrintFault(IoErr(), (char *) opts[DIRECTORY]) ;

                argv = ((char **) opts[FILES]) - 1 ;
                }
        if (!NetSupportBase)
                fprintf(stdout, "Couldn't open NetSupport.Library: Can't parse configfiles.\n");

        if (!opts[FILES]) {
                fprintf(stderr, "reading from standard input\n");
                didchat = 0;
                handleMessage(part_init(stdin), "text/plain", 0, extracttext,
                        (struct boundary *) NULL) ;
                if (!didchat)
                    fprintf(stdout,
                            "Did not find anything to unpack from standard input\n");
                }
        else {
                while (*++argv)
                        if (!(file = fopen(*argv, "r")))
                                os_perror(*argv) ;
                        else {
                                didchat = 0 ;
                                handleMessage(part_init(file), "text/plain", 0,
                                extracttext, (struct boundary *) NULL) ;
                                fclose(file) ;
                                if (!didchat)
                                        fprintf(stdout,
                                                "Did not find anything to unpack from %s\n", *argv);
                                }
                        }

        exit(0) ;
        }

void
warn(char *s) {
    fprintf(stderr, "munpack: warning: %s\n", s);
}

void
chat(char *s) {
        didchat = 1;
        if (!quiet) fprintf(stderr, "munpack: %s\n", s);
        }
