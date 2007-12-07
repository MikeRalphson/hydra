/* char_arr
 * Create C header files for the conversion of character tables
 * Dr. Burkhard Kirste 1993/07/13, 1996/06/14
 * kirste@chemie.fu-berlin.de
 * usage: char_arr [-h] -f from_table -t to_table [[-o] output_file]]
 * requires: sys_def.h, getopt.c, charstab.h
 * cc char_arr.c getopt.c -o char_arr
 */
#define VERSION "1.01"
#include "sys_def.h"
#ifdef __ANSI
#include <string.h>
#include <stdlib.h>
#else
#ifdef sun
#include <stdlib.h>
#endif
#endif				/* stdlib */
#include "charstab.h"		/* character tables */

#define getopt mygetopt
#ifndef EOF
#define EOF -1
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAXPATHLEN
#define MAXPATHLEN 128		/* max. length of path names */
#endif

#define NPERLINE 8		/* array: numbers per line */


/* prototypes */
#ifdef __ANSI
#define PROTO(x) x
#else
#define PROTO(x) ()
#endif

int getopt      PROTO((int argc, char **argv, char *opts));
int write_table 
PROTO((FILE *fp, char **from_table, char **to_table,
       int nperline));
  int write_str   PROTO((FILE *fp, char **from_table, int to_type,
			                 int nperline));
  int make_table  PROTO((char ***tablep, int table_type));

#undef PROTO

/* globals */
  static char     sccsid[] = "@(#)char_arr create headers. 96/06/14 BKi";
  extern char    *optarg;	/* option arg. (getopt) */
  extern int      optind, optopt;	/* option index (getopt) */

  int
                  write_table(fp, from_table, to_table, nperline)
  FILE           *fp;
  char          **from_table, **to_table;
  int             nperline;

/*
 * write header for character conversion table
 * 1993/07/10 BKi
 * FILE *fp: output file
 * char **from_table: source character table
 * char **to_table: aim character table
 * int nperline: entries per line
 */
{
  int             i, j;
  char           *ptr;

  fprintf(fp, "char ctable[] = {\n  ");
  for (i = 0; i < 256; i++)
  {
    ptr = from_table[i];
    for (j = 0; j < 256; j++)
      if (strcmp(ptr, to_table[j]) == 0)
	break;
    if (j >= 256)
      j = i;
    if (i > 0)
    {
      if (i % nperline == 0)
        fprintf(fp, ",\n  ");
      else
        fprintf(fp, ",");
    }
    fprintf(fp, "0x%02x", j);
  }				/* for i */
  fprintf(fp, "};\n");
  fprintf(fp, "\n/* %d entries */\n", i);
  return 0;
}				/* write_table */

int
write_str(fp, from_table, to_type, nperline)
  FILE           *fp;
  char          **from_table;
  int             to_type;
  int             nperline;

/*
 * write header for string-coded character table
 * 1993/07/10 BKi
 * FILE *fp: output file
 * char **from_table: source character table
 * int to_type: aim (0 ... 6)
 * int nperline: entries per line
 * global: trans_string
 */
{
  int             i, j;
  char           *ptr;

  fprintf(fp, "char *stable[] = {\n  ");
  for (i = 0; i < 256; i++)
  {
    ptr = from_table[i];
    if (to_type > 0)
    {
      for (j = 0; j < TRANS_ROW; j++)
	if (strcmp(ptr, trans_string[j][0]) == 0)
	{
	  ptr = trans_string[j][to_type];
	  break;
	}
      if (j >= TRANS_ROW)
	ptr = NULL;
    }
    if (i % nperline == 0)
      fprintf(fp, ",\n  ");
    else
      fprintf(fp, ",");
    if (ptr != NULL)
    {
      fputc('"', fp);
      for (j = 0; j < strlen(ptr); j++)
      {
	if (ptr[j] == '"' || ptr[j] == '\\')
	  fputc('\\', fp);
	fputc(ptr[j], fp);
      }
      fputc('"', fp);
    } else
      fprintf(fp, "\"\\\\%03o\"", i);
  }				/* for i */
  fprintf(fp, "};\n");
  fprintf(fp, "\n/* %d entries */\n", i);
  return 0;
}				/* write_str */

int
make_table(tablep, table_type)
  char         ***tablep;
  int             table_type;

/*
 * make ASCII or EBCDIC table
 * 1993/07/10 BKi
 * char ***tablep: pointer to table of strings (output)
 * int table_type: 0 (ASCII) or 1 (EBCDIC)
 * global: char *iso_table[], char ebc2asc[]
 */
{
  int             i, j;
  char          **table;

  table = (char **) calloc(256, sizeof(char *));
  if (table == NULL)
  {
    fprintf(stderr, "make_table: memory allocation error.\n");
    return 1;
  }
  for (i = 0; i < 256; i++)
  {
    if (table_type == 1)
      j = (int) ebc2asc[i] & 0xff;
    else
      j = i;
    if (j < 128)
    {
      table[i] = (char *) malloc(sizeof(iso_table[j]));
      if (table[i] == NULL)
      {
	fprintf(stderr, "make_table (2): memory allocation error.\n");
	return 1;
      }
      strcpy(table[i], iso_table[j]);
    } else
    {
      table[i] = (char *) malloc(5);
      if (table[i] == NULL)
      {
	fprintf(stderr, "make_table (3): memory allocation error.\n");
	return 1;
      }
      sprintf(table[i], "\\%03o", j);
    }
  }				/* for i */
  *tablep = table;
  return 0;
}				/* make_table */

int
main(argc, argv)
  int             argc;
  char          **argv;
{
  static char     opts[] = "hvf:t:o:";	/* options (getopt) */
  static char     allowed[] = "acdeghlLmrsSt";	/* known table types */
  int             optcheck = 0;	/* check validity of options */
  int             c;		/* character */
  int             i;
  int             from_type, to_type;	/* source and aim type of conversion */
  char            fname[MAXPATHLEN];	/* output file name */
  char          **from_table, **to_table;	/* character conversion
						 * tables */
  char          **asc_table = NULL;	/* optional ASCII table */
  char          **ebc_table = NULL;	/* optional EBCDIC table */
  FILE           *fp = NULL;	/* output file */

  /* get options */
  fname[0] = '\0';
  do
  {
    if ((c = getopt(argc, argv, opts)) == EOF)
      break;
    switch (c)
    {
    case 'v':
      fprintf(stderr, "%s %s\n", VERSION, sccsid);
    case 'h':
    case '?':
      c = '?';
      optcheck = 0;
      break;
    case 'f':
      from_type = (int) optarg[0] & 0xff;
      optcheck |= 1;
      break;
    case 't':
      to_type = (int) optarg[0] & 0xff;
      optcheck |= 2;
      break;
    case 'o':
      strncpy(fname, optarg, MAXPATHLEN);
      fname[MAXPATHLEN - 1] = '\0';
      /* break; */
    }				/* options switch */
    if (c == '?')
      break;
  } while (TRUE);
  if (optcheck == 3)
  {
    /* check validity of arguments */
    if (strchr(allowed, from_type) == NULL)
    {
      fprintf(stderr, "Unknown character table '%c' (-f)\n",
	      (char) from_type);
      optcheck = 0;
    }
    if (strchr(allowed, to_type) == NULL)
    {
      fprintf(stderr, "Unknown character table '%c' (-t)\n",
	      (char) to_type);
      optcheck = 0;
    }
  }
  if (optcheck != 3)
  {
    fprintf(stderr,
	  "usage: char_arr -f from_table -t to_table [[-o] output_file]\n");
    fprintf(stderr, "  where from_table/to_table is one of:\n");
    fprintf(stderr, "  a - ASCII (7 bit)\n");
    fprintf(stderr, "  c - transcript (*)\n");
    fprintf(stderr, "  d - DOS code page 437\n");
    fprintf(stderr, "  e - EBCDIC\n");
    fprintf(stderr, "  g - German LaTeX (cf. TeX) (*)\n");
    fprintf(stderr, "  h - HTML (hypertext) (*)\n");
    fprintf(stderr, "  l - ISO Latin 1 (Unix, ANSI, MS Windows)\n");
    fprintf(stderr, "  L - LaTeX long (\\\"{a}) (cf. TeX) (*)\n");
    fprintf(stderr, "  m - Apple Macintosh\n");
    fprintf(stderr, "  r - Atari ST\n");
    fprintf(stderr, "  s - SGML (Standard Generalized Markup Language) (*)\n");
    fprintf(stderr, "  S - Symbol font\n");
    fprintf(stderr, "  t - TeX (*)\n");
    fprintf(stderr, "    (*) string code, output (-t) only!\n");
    return 1;
  }
  if (optind < argc)
  {
    strncpy(fname, argv[optind++], MAXPATHLEN);
    fname[MAXPATHLEN - 1] = '\0';
  }
  /* find conversion tables */
  switch (from_type)
  {
  case 'a':
    if (asc_table == NULL)
    {
      if (make_table(&asc_table, 0) == 0)
	from_table = asc_table;
      else
	from_table = NULL;
    } else
      from_table = asc_table;
    break;
  case 'd':
    from_table = pc_table;
    break;
  case 'e':
    if (ebc_table == NULL)
    {
      if (make_table(&ebc_table, 1) == 0)
	from_table = ebc_table;
      else
	from_table = NULL;
    } else
      from_table = ebc_table;
    break;
  case 'l':
    from_table = iso_table;
    break;
  case 'm':
    from_table = mac_table;
    break;
  case 'r':
    from_table = st_table;
    break;
  case 'S':
    from_table = sym_table;
    break;
  default:
    from_table = NULL;
  }				/* switch */
  switch (to_type)
  {
  case 'a':
    if (asc_table == NULL)
    {
      if (make_table(&asc_table, 0) == 0)
	to_table = asc_table;
      else
	to_table = NULL;
    } else
      to_table = asc_table;
    break;
  case 'd':
    to_table = pc_table;
    break;
  case 'e':
    if (ebc_table == NULL)
    {
      if (make_table(&ebc_table, 1) == 0)
	to_table = ebc_table;
      else
	to_table = NULL;
    } else
      to_table = ebc_table;
    break;
  case 'l':
    to_table = iso_table;
    break;
  case 'm':
    to_table = mac_table;
    break;
  case 'r':
    to_table = st_table;
    break;
  case 'S':
    to_table = sym_table;
    break;
  default:
    to_table = NULL;
  }				/* switch */
  if (from_table == NULL)
    exit(1);
  if (strlen(fname))
  {
    fp = fopen(fname, "w");
    if (fp == NULL)
    {
      fprintf(stderr, "Error opening output file %s\n", fname);
      exit(1);
    }
  } else
    fp = stdout;
  if (to_table != NULL)
    write_table(fp, from_table, to_table, (int) NPERLINE);
  else
  {
    switch (to_type)
    {
    case 'c':
      to_type = 0;
      break;
    case 't':
      to_type = 1;
      break;
    case 'L':
      to_type = 2;
      break;
    case 'g':
      to_type = 3;
      break;
    case 's':
      to_type = 4;
      break;
    case 'h':
      to_type = 5;
      break;
    default:
      to_type = -1;
    }
    if (to_type >= 0)
      write_str(fp, from_table, to_type, (int) NPERLINE);
  }
  if (fp != stdout && fp != NULL)
    fclose(fp);
  if (ebc_table != NULL)
  {
    for (i = 255; i >= 0; i--)
      if (ebc_table[i] != NULL)
	free(ebc_table[i]);
    if (ebc_table != NULL)
      free(ebc_table);
  }
  if (asc_table != NULL)
  {
    for (i = 255; i >= 0; i--)
      if (asc_table[i] != NULL)
	free(asc_table[i]);
    if (asc_table != NULL)
      free(asc_table);
  }
  return 0;
}				/* main */
