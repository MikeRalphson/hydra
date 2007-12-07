/* @(#)getopt
 * parse command options
 * originally from comp.sources.unix/volume3/att_getopt
 * BKi 1993/07/04
 */

/*
* int getopt PROTO((int argc, char **argv, char *opts));
*   argc: number of parameters passed
*   argv[]: parameter string
*   opts: string of allowed parameters,
*     followed by ':' if argument required
*   return option (or EOF)
*     (option argument in: char *optarg)
*
* global: char *optarg: argument
*         int optind: number of argument
*         int optopt
*
* extern int      strcmp();
* extern char    *strchr();
*/
#include "sys_def.h"
#include <string.h>

#define getopt mygetopt
#ifndef EOF
#define EOF -1
#endif
#ifndef NULL
#define NULL 0L
#endif

int             optind = 1;
int             optopt;
char           *optarg;

int
getopt(argc, argv, opts)
  int             argc;
  char          **argv, *opts;
{
  static int      sp = 1;
  register int    c;
  register char  *cp;

  if (sp == 1)
    if (optind >= argc ||
	argv[optind][0] != '-' || argv[optind][1] == '\0')
      return (EOF);
    else if (strcmp(argv[optind], "--") == 0)
    {
      optind++;
      return (EOF);
    }
  optopt = c = argv[optind][sp];
  if (c == ':' || (cp = strchr(opts, c)) == NULL)
  {
    fprintf(stderr, "%c: illegal option -- \n", c);
    if (argv[optind][++sp] == '\0')
    {
      optind++;
      sp = 1;
    }
    return ('?');
  }
  if (*++cp == ':')
  {
    if (argv[optind][sp + 1] != '\0')
      optarg = &argv[optind++][sp + 1];
    else if (++optind >= argc)
    {
      fprintf(stderr, "%c: option requires an argument -- \n", c);
      sp = 1;
      return ('?');
    } else
      optarg = argv[optind++];
    sp = 1;
  } else
  {
    if (argv[optind][++sp] == '\0')
    {
      sp = 1;
      optind++;
    }
    optarg = NULL;
  }
  return (c);
} /* getopt */
