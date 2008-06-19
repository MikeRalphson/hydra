/*---------------------------------------------------------------------------+
 |     Copyright (c) 1992 Oracle Corporation Belmont, California, USA        |
 |                       All rights reserved                                 |
 +---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------+
 |   FILENAME                                                                |
 |      hide.c                                                               |
 |   DESCRIPTION                                                             |
 |      Hides arguments for programs on UNIX systems.                        |
 |      Can be used as a program prefix: hide program arguments              |
 |      or as a symbolic link.  If this program is not invoked as hide, it   |
 |      will hide its arguments and invoke the program name.hide             |
 |      The best way to use this is to rename your critical programs to      |
 |      program.hide, and create a symbolic link program to hide.            |
 |      mv sqlplus sqlplus.hide; ln -s hide sqlplus                          |
 |      Thus when sqlplus is invoked, its arguments will be hidden           |
 |   NOTES                                                                   |
 |      This program works by padding 3000 '/' chars in argv[0].  This fools |
 |      all known ps's.  This will reduce the argument capacity of your      |
 |      program by 3000 chars.  A good enhancement would be to reduce the    |
 |      padding if needed so that no arguments are lost - would require a    |
 |      method of determining the max argument size on the system.  Some     |
 |      system's provide the E2BIG error on exec.                            |
 |      There is some performace penalty for using this program, but it is   |
 |      minimal because this program is so small - the biggest cost is the   |
 |      extra exec required to get this program started.                     |
 |   HISTORY                                                                 |
 |       09/15/92  R Brodersen  Created, based on D Beusee's hideargs()      |
 |       09/17/92  D Beusee     Fixed to compile on any system               |
 +---------------------------------------------------------------------------*/

/*
 * $Header: /local/bin/RCS/hide.c,v 1.6 1992/09/22 22:37:17 dbeusee Exp $
 *
 * $Log: hide.c,v $
 * Revision 1.6  1992/09/22  22:37:17  dbeusee
 * Added exit(1) when cannot execvp the program.
 *
 * Revision 1.5  1992/09/22  11:28:44  dbeusee
 * SOme BSD systems have memset(), so add a #define memset MEMSET to fix
 * compilation errors (like on ultrix).
 *
 * Revision 1.4  1992/09/22  06:34:57  dbeusee
 * BSD systems need memset routine.
 *
 * Revision 1.3  1992/09/22  06:05:13  dbeusee
 * Set JUNK_CHAR to ' ' but force last junk char to '/'.  This looks prettier
 * when doing 'ps'.  Also do not show full path of the program.  Also do not
 * show .hide if prog is a symlink to hide.
 *
 * Revision 1.2  1992/09/22  05:52:26  dbeusee
 * If hide could not execvp the program, give an error message.
 * if hide was invoked with a full path (e.g. /usr/local/bin/hide),
 * do not try to invoke PATH/hide.hide.
 *
 *
 */

/*
 * #include "os.h"
 */

#include <stdio.h>
#ifdef SYS5
#include <string.h>
#else
#include <strings.h>
#define strrchr rindex
#define memset MEMSET /* some BSD systems have a memset() */
char *memset();
#endif
/*
 * #define JUNK_SIZE 3000
 */
#define JUNK_SIZE 128
#define JUNK_CHAR ' '

char arg0buf[4096];
char progbuf[4096];
char errbuf[4096];

int main(argc, argv)
int argc;
char *argv[];
{
    char *name, *base;
    int firstarg;

    if (!(name = strrchr(argv[0], '/')))
	name = argv[0];
    else
	name ++; /* get past '/' */

    firstarg = (!strcmp(name, "hide")) ? 1 : 0;

    if (firstarg && (argc == 1))
    {
	fprintf(stderr, "Usage: hide program arguments\n");
	fprintf(stderr, "   ie: hide sqlplus username/password\n");
	fprintf(stderr, "if hide is not named hide, it will execute name.hide (useful as a symbolic link)\n");
	exit(1);
    }

    /* Build program name.  If symbolic link mode, use argv[0] || .hide */
    strcpy(progbuf, argv[firstarg]);
    if (!(base = strrchr(argv[firstarg], '/')))
	base = argv[firstarg];
    else
	base ++; /* get past '/' */
    if (!firstarg) strcat(progbuf, ".hide");

    /* Build arg0 buffer.  First, fill it with junk */
    memset((void *)arg0buf, JUNK_CHAR, JUNK_SIZE);
    arg0buf[JUNK_SIZE-1] = '/'; /* set last char to '/' */
    /* Prepend real program name - so ps can see what prog is running */
    strncpy(arg0buf, base, strlen(base));
    /* Append real program name - so prog can see what prog is running */
    strcpy(arg0buf + JUNK_SIZE, argv[firstarg]);
    /* Assign new arg0 buffer to the argv array */
    argv[firstarg] = arg0buf;

    /* Start the new program with the shifted arguments */
    execvp(progbuf, argv + firstarg);

    sprintf(errbuf, "Could not execvp '%s'", progbuf);
    perror(errbuf);
    exit(1);
}

#ifndef SYS5
char *
memset(s, c, n)
	register char *s;
	register c, n;
{
	register char *p = s;

	while (n-- > 0)
		*s++ = c;

	return (p);
}
#endif /* ifndef SYS5 */
