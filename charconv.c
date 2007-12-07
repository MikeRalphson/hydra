/* charconv
 * Filter for conversion of character tables
 * Dr. Burkhard Kirste 1993/07/17, 1996/06/14
 * kirste@chemie.fu-berlin.de
 * usage: charconv [-h|-v] [-d|-m|-u] [-f from_table] [-t to_table]
 *          [[-i] input_file [[-o] output_file]]
 * requires: sys_def.h, getopt.c, charstab.h
 * cc charconv.c getopt.c -o char_arr
 */
#define VERSION "1.12"
#include "sys_def.h"
#include <string.h>
#include <ctype.h>
#ifdef __ANSI
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

#define TABLELEN 256		/* size of conversion table */
#define BUFLEN 1024		/* size of input buffer */
#define MAXTOK 40		/* maximum size of token */
#define OFFSET 1024		/* used in fgetit */


/* prototypes */
#ifdef __ANSI
#define PROTO(x) x
#else
#define PROTO(x) ()
#endif

int getopt      PROTO((int argc, char **argv, char *opts));
int do_table    PROTO((char **from_table, char **to_table));
int do_str      PROTO((char **from_table, int to_type));
int fgetit      PROTO((int from_type, char **to_table, FILE *fp));
int find_str    PROTO((char *ptr, char **to_table));

#undef PROTO

/* globals */
static char     sccsid[] = "@(#)charconv character conversion. 96/06/14 BKi";
extern char    *optarg;		/* option arg. (getopt) */
extern int      optind, optopt;	/* option index (getopt) */
char            buffer[BUFLEN];	/* input buffer */
char            conv_table[TABLELEN];	/* character conversion table */
char           *trans_ptr[TABLELEN];	/* string output */

int
do_table(from_table, to_table)
  char          **from_table, **to_table;

/*
 * Fill character conversion table
 * 1993/07/11 BKi
 * char **from_table: source character table
 * char **to_table: aim character table
 * global: conv_table[TABLELEN]
 */
{
  int             i, j;
  char           *ptr;

  for (i = 0; i < TABLELEN; i++)
  {
    ptr = from_table[i];
    for (j = 0; j < TABLELEN; j++)
      if (strcmp(ptr, to_table[j]) == 0)
	break;
    if (j >= TABLELEN)
      j = i;
    conv_table[i] = j;
  }				/* for i */
  return 0;
}				/* do_table */

int
do_str(from_table, to_type)
  char          **from_table;
  int             to_type;

/*
 * Find corresponding string
 * 1993/07/11 BKi
 * char **from_table: source character table
 * int to_type: type (column) in trans_string
 * global: trans_string[TRANS_ROW][TRANS_COL]
 *         trans_ptr[TABLELEN]
 */
{
  int             i, j;
  char           *ptr;

  for (i = 0; i < TABLELEN; i++)
  {
    ptr = from_table[i];
    for (j = 0; j < TRANS_ROW; j++)
      if (strcmp(ptr, trans_string[j][0]) == 0)
      {
	trans_ptr[i] = trans_string[j][to_type];
	break;
      }
    if (j >= TRANS_ROW)
    {
      if (i == '\t')
	trans_ptr[i] = "\t";
      else if (i == '\n')
	trans_ptr[i] = "\n";
      else if (i == '\r')
	trans_ptr[i] = "\r";
      else if (i < 32)
	trans_ptr[i] = "";
      else
	trans_ptr[i] = " ";
    }
  }				/* for i */
  return 0;
}				/* do_str */

int
find_str(ptr, to_table)
  char           *ptr;
  char          **to_table;

/*
 * Find character code corresponding to string
 * 1993/07/11 BKi
 * char *ptr: string
 * char **to_table: character table
 * return character code
 */
{
  int             i;

  for (i = 0; i < TABLELEN; i++)
  {
    if (strcmp(ptr, to_table[i]) == 0)
      return i;
  }				/* for i */
  /* if not found, use blank */
  return 0x20;
}				/* find_str */

int
fgetit(from_type, to_table, fp)
  int             from_type;
  char          **to_table;
  FILE           *fp;

/*
 * Parse file fp, identify tokens, return character
 * 1993/07/12, 1996/06/14 BKi
 * int from_type: type of tokens (column)
 * char **to_type: aim character table (or NULL)
 * FILE *fp: input stream
 * global: trans_string[TRANS_ROW][TRANS_COL]
 *         buffer[BUFLEN]
 * return character_code || trans_string_row + OFFSET || EOF
 */
{
  int             c;
  int             i;
  static int      len = 0;
  static int      special = 0;
  static int      glatexflag = 0;
  static int      sgmlflag = 0;
  static int      slashflag = 0;
  static int      from_other = 0;

  if (special < 2)
    c = fgetc(fp);
  if (c == EOF)
    return c;
  if (from_type == 0)
  {
    /* special coding */
    do
    {
      if (special == 0)
      {
	if (c != '\\')
	  return c;
	else
	{
	  ++special;
	  buffer[len++] = c;
	  buffer[len] = '\0';
	}
      } else if (special == 1)
      {
	buffer[len++] = c;
	buffer[len] = '\0';
	if (c == '\\' || len >= MAXTOK)
	  ++special;
      }
      if (special > 1)
      {
	c = buffer[0];
	for (i = 0; i < len; i++)
	  buffer[i] = buffer[i + 1];
	--len;
	if (buffer[0] == '\\')
	  special = 1;
	if (len == 0)
	  special = 0;
	return c;
      }
      if (special == 1)
      {
	if (len > 2)
	{
	  /* identify token */
	  for (i = 0; i < TRANS_ROW; i++)
	    if (strcmp(buffer, trans_string[i][from_type]) == 0)
	    {
	      len = 0;
	      special = 0;
	      buffer[0] = '\0';
	      if (to_table != NULL)
		return find_str(trans_string[i][0], to_table);
	      else
		return i + OFFSET;
	    }
	}
	c = fgetc(fp);
	if (c == EOF)
	  return c;
      }
    } while (TRUE);
  } else if (from_type >= 1 && from_type <= 3)
  {
    /* TeX */
    if (from_type == 3)
    {
      glatexflag = 1;
      from_other = 1;
    } else if (from_type == 2)
    {
      from_other = 1;
    } else if (from_type == 1)
    {
      from_other = 2;
    }
    do
    {
      if (special == 2)
      {
	c = buffer[0];
	special = 0;
	len = 0;
	buffer[0] = '\0';
      }
      if (special == 0)
      {
	if (c == '%')
	{
	  do
	  {
	    c = fgetc(fp);
	    if (c == EOF)
	      return c;
	  } while (c != '\r' && c != '\n');
	  return c;
	} else if (c == '~')
	{
	  c = ' ';
	  return c;
	} else if (c == '{' || c == '}' || c == '$')
	{
	  c = fgetc(fp);
	  if (c == EOF)
	    return c;
	  continue;
	} else if (c != '\\' && c != '\'' && c != '?' && c != '!'
		   && c != '`' && (c != '"' || !glatexflag))
	  return c;
	else
	{
	  special = 1;
	  buffer[len++] = c;
	  buffer[len] = '\0';
	}
      } else if (special == 1)
      {
	if (len == 1 && buffer[0] == '\\' && c == '-')
	{
	  /* optional hyphen \- */
	  len = 0;
	  special = 0;
	  buffer[0] = '\0';
	  c = fgetc(fp);
	  if (c == EOF)
	    return c;
	  continue;
	}
	if (len == 1 && buffer[0] != '\\')
	{
	  /* check for '' ?` !` */
	  buffer[len++] = c;
	  buffer[len] = '\0';
	  /*
	   * if (strcmp(buffer, "``") == 0) strcpy(buffer, "''");
	   */
	  /* identify token */
	  for (i = 0; i < TRANS_ROW; i++)
	    if (strcmp(buffer, trans_string[i][from_type]) == 0
		|| strcmp(buffer, trans_string[i][from_other]) == 0)
	    {
	      len = 0;
	      special = 0;
	      buffer[0] = '\0';
	      if (to_table != NULL)
		return find_str(trans_string[i][0], to_table);
	      else
		return i + OFFSET;
	    }
	  /* otherwise nothing special */
	  c = buffer[0];
	  buffer[0] = buffer[1];
	  buffer[1] = '\0';
	  len = 1;
	  special = 2;
	  return c;
	}
	if (len >= MAXTOK)
	{
	  len = 0;
	  special = 0;
	  buffer[0] = '\0';
	  continue;
	} else if (len == 1 && c == '3' && buffer[0] == '\\')
	{
	  /* short for German sz */
	  strcpy(buffer, "\\ss{}");
	  len = 5;
	} else if (len == 1 && c == '/' && buffer[0] == '\\')
	{
	  /* italic extra space */
	  len = 0;
	  special = 0;
	  buffer[0] = '\0';
	  c = fgetc(fp);
	  if (c == EOF)
	    return c;
	  continue;
	} else
	{
	  buffer[len++] = c;
	  buffer[len] = '\0';
	}
      }
      if (special == 1)
      {
	if (len >= 2)
	{
	  /* identify token */
	  for (i = 0; i < TRANS_ROW; i++)
	    if (strncmp(buffer, trans_string[i][from_type], len) == 0
		|| strncmp(buffer, &trans_string[i][from_type][1], len) == 0
		|| strncmp(buffer, trans_string[i][from_other], len) == 0
	      || strncmp(buffer, &trans_string[i][from_other][1], len) == 0)
	      break;
	  if (i >= TRANS_ROW)
	  {
	    /* token not found */
	    if (isalpha(c) && isalpha(buffer[1]))
	    {
	      /* get whole token */
	      do
	      {
		c = fgetc(fp);
		if (len < MAXTOK)
		{
		  buffer[len++] = c;
		  buffer[len] = '\0';
		}
	      } while (isalpha(c) && c != EOF);
	      if (c == EOF)
		return c;
	    }
	    /* skip \begin{}, \end{} (LaTeX) */
	    if (strcmp(buffer, "\\begin{") == 0 ||
		strcmp(buffer, "\\end{") == 0)
	    {
	      do
		c = fgetc(fp);
	      while (c != '}' && c != EOF);
	      if (c == EOF)
		return c;
	    }
	    if (c == ' ')
	    {
	      len = 0;
	      special = 0;
	      buffer[0] = '\0';
	    } else
	    {
	      /* substitute unknown token by space */
	      len = 1;
	      special = 2;
	      buffer[0] = c;
	      buffer[1] = '\0';
	      c = ' ';
	    }
	    return c;
	  }
	  /* some LaTeX problems */
	  if ((strncmp(buffer, "\\verb", 4) == 0 && len < 8)
	      || (strncmp(buffer, "\\pounds", 6) == 0 && len < 7))
	  {
	    c = fgetc(fp);
	    if (c == EOF)
	      return 0;
	    continue;
	  }
	  if (strcmp(buffer, trans_string[i][from_type]) == 0
	      || trans_string[i][from_type][len + 1] == '$'
	      || trans_string[i][from_type][len] == '}'
	      || ((trans_string[i][from_type][len] == '{'
		   || trans_string[i][from_type][0] == '{')
		  && trans_string[i][from_type][len + 1] == '}')
	      || strcmp(buffer, trans_string[i][from_other]) == 0
	      || trans_string[i][from_other][len + 1] == '$'
	      || trans_string[i][from_other][len] == '}'
	      || ((trans_string[i][from_other][len] == '{'
		   || trans_string[i][from_other][0] == '{')
		  && trans_string[i][from_other][len + 1] == '}'))
	  {
	    if (isalpha(c) &&
		(trans_string[i][from_type][len + 1] == '$' ||
		 ((trans_string[i][from_type][len] == '{'
		   || trans_string[i][from_type][0] == '{')
		  && trans_string[i][from_type][len + 1] == '}'))
		|| (trans_string[i][from_other][len + 1] == '$' ||
		    ((trans_string[i][from_other][len] == '{'
		      || trans_string[i][from_other][0] == '{')
		     && trans_string[i][from_other][len + 1] == '}')))
	    {
	      c = fgetc(fp);
	      if (isalpha(c))
		continue;
	      else if (c != ' ')
	      {
		special = 2;
		buffer[0] = c;
		buffer[1] = '\0';
		len = 1;
	      }
	    }
	    if (special != 2)
	    {
	      len = 0;
	      special = 0;
	      buffer[0] = '\0';
	    }
	    if (to_table != NULL)
	      return find_str(trans_string[i][0], to_table);
	    else
	      return i + OFFSET;
	  }
	}
	c = fgetc(fp);
	if (c == EOF)
	  return c;
      }
    } while (TRUE);
  } else if (from_type >= 4 && from_type <= 5)
  {
    /* SGML, HTML */
    if (from_type == 4)
    {
      sgmlflag = 1;
    }
    do
    {
      if (special == 0)
      {
	/* take care of SGML tags <bf/bold/ */
	if (slashflag && c == '/')
	{
	  slashflag = 0;
	  c = fgetc(fp);
	  if (c == EOF)
	    return c;
	}
	if (c != '&' && c != '<')
	  return c;
	else
	{
	  ++special;
	  buffer[len++] = c;
	  buffer[len] = '\0';
	}
      } else if (special == 1)
      {
	buffer[len++] = c;
	buffer[len] = '\0';
	if (c == '&' || len >= MAXTOK)
	  ++special;
      }
      if (special > 1)
      {
	c = buffer[0];
	for (i = 0; i < len; i++)
	  buffer[i] = buffer[i + 1];
	--len;
	if (buffer[0] == '&' || buffer[0] == '<')
	  special = 1;
	if (len == 0)
	  special = 0;
	return c;
      }
      if (special == 1)
      {
	if (buffer[0] == '<' && (isalpha(buffer[1]) || buffer[1] == '/'
				 || buffer[1] == '!'))
	{
	  /* skip HTML tag */
	  do
	  {
	    c = fgetc(fp);
	    if (c == EOF)
	      return c;
	    /* take care of SGML tags <bf/bold/ */
	    if (sgmlflag && c == '/' && len > 1 && buffer[1] != '/')
	    {
	      slashflag = 1;
	      for (i = 1; i < len; i++)
		if (!isalpha(buffer[i]))
		{
		  slashflag = 0;
		  break;
		}
	    }
	    if (c == '>' || slashflag)
	    {
	      special = 0;
	      len = 0;
	      buffer[0] = '\0';
	      break;
	    }
	  } while (TRUE);
	  c = fgetc(fp);
	  if (c == EOF)
	    return c;
	  continue;
	}
	/* SGML: allow for missing ; in character entities */
	if (sgmlflag && len > 1 && buffer[0] == '&' && !isalpha(c) && c != ';')
	{
	  c = ';';
	  buffer[len++] = c;
	  buffer[len] = '\0';
	}
	if (c == ';')
	{
	  /* identify token */
	  for (i = 0; i < TRANS_ROW; i++)
	    if (strcmp(buffer, trans_string[i][from_type]) == 0)
	    {
	      len = 0;
	      special = 0;
	      buffer[0] = '\0';
	      if (to_table != NULL)
		return find_str(trans_string[i][0], to_table);
	      else
		return i + OFFSET;
	    }
	  if (strcmp(buffer, "&quot;") == 0)
	    c = '"';
	  else
	    c = ' ';
	  len = 0;
	  special = 0;
	  buffer[0] = '\0';
	  return c;
	}
	c = fgetc(fp);
	if (c == EOF)
	  return c;
      }
    } while (TRUE);
  }
  return c;
}				/* fgetit */

int
main(argc, argv)
  int             argc;
  char          **argv;
{
  static char     opts[] = "hvdmuf:t:i:o:";	/* options (getopt) */
  static char     allowed[] = " aAcdeghHlLmrsStz";	/* known table types */
  int             optcheck = 0;	/* check validitity of of options */
  int             eol = 0;	/* end-of-line option */
  int             rtf_flag = 0;	/* flag for RTF output (if 1) */
  int             c;		/* character */
  int             lastc = ' ';
  int             i;
  int             in_type = 0, out_type = 0;	/* 0: no conv., 1: char, 2:
						 * string */
  int             from_type, to_type;	/* source and aim type of conversion */
  char          **from_table, **to_table;	/* character conversion
						 * tables */
  char            inname[MAXPATHLEN];	/* input file name */
  char            outname[MAXPATHLEN];	/* output file name */
  FILE           *fpin = NULL;	/* input file */
  FILE           *fpout = NULL;	/* output file */

  /* get options */
  inname[0] = outname[0] = '\0';
  from_type = to_type = ' ';
  from_table = to_table = NULL;
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
      optcheck = 1;
      break;
    case 'd':
      eol = 1;
      break;
    case 'm':
      eol = 2;
      break;
    case 'u':
      eol = 3;
      break;
    case 'f':
      from_type = (int) optarg[0] & 0xff;
      break;
    case 't':
      to_type = (int) optarg[0] & 0xff;
      break;
    case 'i':
      strncpy(inname, optarg, MAXPATHLEN);
      inname[MAXPATHLEN - 1] = '\0';
      break;
    case 'o':
      strncpy(outname, optarg, MAXPATHLEN);
      outname[MAXPATHLEN - 1] = '\0';
      /* break; */
    }				/* options switch */
    if (c == '?')
      break;
  } while (TRUE);
  if (optcheck == 0)
  {
    /* check validity of arguments */
    if (strchr(allowed, from_type) == NULL)
    {
      fprintf(stderr, "Unknown character table '%c' (-f)\n",
	      (char) from_type);
      optcheck = 1;
    }
    if (strchr(allowed, to_type) == NULL)
    {
      fprintf(stderr, "Unknown character table '%c' (-t)\n",
	      (char) to_type);
      optcheck = 1;
    }
  }
  if (optcheck == 1)
  {
    fprintf(stderr,
	    "usage: charconv [-d|-m|-u] [-f from_table] [-t to_table]\n");
    fprintf(stderr,
	    "                [[-i] input_file [-o] output_file]\n");
    fprintf(stderr, "  -d - create MS DOS end-of-line (CRLF)\n");
    fprintf(stderr, "  -m - create Macintosh end-of-line (CR)\n");
    fprintf(stderr, "  -u - create Unix end-of-line (LF)\n");
    fprintf(stderr, "  -f, -t - 'from'/'to' character table\n");
    fprintf(stderr, "  where from_table/to_table is one of:\n");
    fprintf(stderr, "  a - ASCII (7 bit) (*)\n");
    fprintf(stderr, "  c - transcription (*)\n");
    fprintf(stderr, "  d - DOS code page 437\n");
    fprintf(stderr, "  e - EBCDIC (only for ASCII <-> EBCDIC!)\n");
    fprintf(stderr, "  g - German LaTeX (cf. TeX) (*)\n");
    fprintf(stderr, "  h - HTML (hypertext) (*)\n");
    fprintf(stderr, "  H - HTML (keep < & >) (*)\n");
    fprintf(stderr, "  l - ISO Latin 1 (Unix, ANSI, MS Windows)\n");
    fprintf(stderr, "  L - LaTeX long (\\\"{a}) (cf. TeX) (*)\n");
    fprintf(stderr, "  m - Apple Macintosh\n");
    fprintf(stderr, "  r - RTF (Rich Text Format) (output only!)\n");
    fprintf(stderr, "  s - SGML (Standard Generalized Markup Language) (*)\n");
    fprintf(stderr, "  S - Symbol font\n");
    fprintf(stderr, "  t - TeX (*)\n");
    fprintf(stderr, "  z - Atari ST\n");
    fprintf(stderr, "    (*) string code\n");
    return 1;
  }
  if (optind < argc)
  {
    strncpy(inname, argv[optind++], MAXPATHLEN);
    inname[MAXPATHLEN - 1] = '\0';
  }
  if (optind < argc)
  {
    strncpy(outname, argv[optind++], MAXPATHLEN);
    outname[MAXPATHLEN - 1] = '\0';
  }
  /* find conversion tables */
  switch (from_type)
  {
  case 'a':
    in_type = 3;
    from_table = NULL;
    break;
  case 'd':
    in_type = 1;
    from_table = pc_table;
    break;
  case 'e':
    in_type = 4;
    if (out_type == 0)
      out_type = 3;
    from_table = NULL;
    break;
  case 'l':
    in_type = 1;
    from_table = iso_table;
    break;
  case 'm':
    in_type = 1;
    from_table = mac_table;
    break;
  case 'z':
    in_type = 1;
    from_table = st_table;
    break;
  case 'S':
    in_type = 1;
    from_table = sym_table;
    break;
  case 'c':
    in_type = 2;
    from_type = 0;
    from_table = NULL;
    break;
  case 't':
    in_type = 2;
    from_type = 1;
    from_table = NULL;
    break;
  case 'L':
    in_type = 2;
    from_type = 2;
    from_table = NULL;
    break;
  case 'g':
    in_type = 2;
    from_type = 3;
    from_table = NULL;
    break;
  case 's':
    in_type = 2;
    from_type = 4;
    from_table = NULL;
    break;
  case 'h':
  case 'H':
    in_type = 2;
    from_type = 5;
    from_table = NULL;
    break;
  default:
    from_table = NULL;
  }				/* switch */
  switch (to_type)
  {
  case 'a':
    out_type = 2;
    to_type = 6;
    to_table = NULL;
    break;
  case 'A':
    out_type = 1;
    to_table = iso_table;
    break;
  case 'd':
    out_type = 1;
    to_table = pc_table;
    break;
  case 'e':
    out_type = 4;
    to_table = NULL;
    break;
  case 'r':
    rtf_flag = 1;
  case 'l':
    out_type = 2;
    to_type = 7;
    to_table = NULL;
    break;
  case 'm':
    out_type = 1;
    to_table = mac_table;
    break;
  case 'z':
    out_type = 1;
    to_table = st_table;
    break;
  case 'S':
    out_type = 1;
    to_table = sym_table;
    break;
  case 'c':
    out_type = 2;
    to_type = 0;
    to_table = NULL;
    break;
  case 't':
    out_type = 2;
    to_type = 1;
    to_table = NULL;
    break;
  case 'L':
    out_type = 2;
    to_type = 2;
    to_table = NULL;
    break;
  case 'g':
    out_type = 2;
    to_type = 3;
    to_table = NULL;
    break;
  case 's':
    out_type = 2;
    to_type = 4;
    to_table = NULL;
    break;
  case 'h':
    out_type = 2;
    to_type = 5;
    to_table = NULL;
    break;
  case 'H':
    out_type = 2;
    to_type = 5;
    to_table = NULL;
    /* modify < & > in table trans_string */
    for (i = 0; i < TRANS_ROW; i++)
      if (strcmp(trans_string[i][to_type], "&amp;") == 0)
      {
        strcpy(trans_string[i][to_type], "&");
        break;
      }
    for (i = 0; i < TRANS_ROW; i++)
      if (strcmp(trans_string[i][to_type], "&lt;") == 0)
      {
        strcpy(trans_string[i][to_type], "<");
        break;
      }
    for (i = 0; i < TRANS_ROW; i++)
      if (strcmp(trans_string[i][to_type], "&gt;") == 0)
      {
        strcpy(trans_string[i][to_type], ">");
        break;
      }
    break;
  default:
    to_table = NULL;
  }				/* switch */
  /* optionally, open files */
  if (strlen(inname))
  {
    fpin = fopen(inname, "rb");
    if (fpin == NULL)
    {
      fprintf(stderr, "Error opening input file %s\n", inname);
      exit(1);
    }
  } else
    fpin = stdin;
  if (strlen(outname))
  {
    if (strlen(inname) && strcmp(inname, outname) == 0)
    {
      fprintf(stderr, "Error, 'output' must differ from 'input'\n");
      fpout = NULL;
    } else
      fpout = fopen(outname, "wb");
    if (fpout == NULL)
    {
      fprintf(stderr, "Error opening output file %s\n", outname);
      exit(1);
    }
  } else
    fpout = stdout;
  /* prepare conversions */
  if (in_type == 1 && out_type == 0)
  {
    out_type = 1;
#ifdef __PC
    to_type = 'd';
    to_table = pc_table;
#else
#ifdef __ATARI
    to_type = 'r';
    to_table = st_table;
#else
    to_type = 'l';
    to_table = iso_table;
#endif
#endif
  }
  if (in_type == 1 && out_type == 1)
  {
    if (from_table == to_table)
      out_type = 0;
    else
      do_table(from_table, to_table);
  } else if (in_type == 4)
  {
    /* EBCDIC to ASCII */
    in_type = 1;
    out_type = 1;
    for (i = 0; i < TABLELEN; i++)
      conv_table[i] = ebc2asc[i];
  } else if (out_type == 4)
  {
    /* ASCII to EBCDIC */
    in_type = 1;
    out_type = 1;
    for (i = 0; i < TABLELEN; i++)
      conv_table[i] = asc2ebc[i];
  }
  if (out_type == 2 && in_type != 1 && in_type != 2)
  {
    in_type = 1;
#ifdef __PC
    from_type = 'd';
    from_table = pc_table;
#else
#ifdef __ATARI
    from_type = 'r';
    from_table = st_table;
#else
    from_type = 'l';
    from_table = iso_table;
#endif
#endif
  }
  if (out_type == 2 && in_type == 1)
    do_str(from_table, to_type);
  if (in_type == 2)
  {
    buffer[0] = '\0';
    if (out_type == 0)
    {
      out_type = 1;
#ifdef __PC
      to_type = 'd';
      to_table = pc_table;
#else
#ifdef __ATARI
      to_type = 'r';
      to_table = st_table;
#else
      to_type = 'l';
      to_table = iso_table;
#endif
#endif
    }
  }
  if (rtf_flag == 1)
  {
    do
    {
      if (in_type < 2)
      {
	if ((c = fgetc(fpin)) == EOF)
	  break;
	if (in_type == 1 && out_type == 1)
	  c = (int) conv_table[c] & 0xff;
	else
	  c &= 0xff;
      } else if (in_type == 2)
      {
	if ((c = fgetit(from_type, to_table, fpin)) == EOF)
	  break;
	c &= 0xff;
      }
      if (c <= 0)
	c = ' ';
      if (c < 128)
      {
	if (fputc(c, fpout) == EOF)
	  break;
      } else
      {
	if (fputs(rtf_table[c], fpout) == EOF)
	  break;
      }
    } while (TRUE);
  } else if (in_type == 1 && out_type == 1)
  {
    do
    {
      if ((c = fgetc(fpin)) == EOF)
	break;
      if (eol == 0)
      {
	c = (int) conv_table[c] & 0xff;
	if (fputc(c, fpout) == EOF)
	  break;
      } else
      {
	if (from_type == 'e')
	  c = (int) conv_table[c] & 0xff;
	if (c == '\n' && lastc == '\r')
	{
	  /* CRLF */
	  lastc = c;
	  continue;
	}
	lastc = c;
	if (c == '\n' || c == '\r')
	{
	  /* EOL */
	  if (eol == 1)
	  {
	    /* DOS */
	    c = '\r';
	    if (to_type == 'e')
	      c = (int) conv_table[c] & 0xff;
	    if (fputc(c, fpout) == EOF)
	      break;
	    c = '\n';
	  } else if (eol == 2)
	    c = '\r';
	  else if (eol == 3)
	    c = '\n';
	}
	if (from_type != 'e')
	  c = (int) conv_table[c] & 0xff;
	if (fputc(c, fpout) == EOF)
	  break;
      }
    } while (TRUE);
  } else if (in_type == 0 && out_type == 0)
  {
    do
    {
      if ((c = fgetc(fpin)) == EOF)
	break;
      if (eol == 0)
      {
	if (fputc(c, fpout) == EOF)
	  break;
      } else
      {
	if (c == '\n' && lastc == '\r')
	{
	  /* CRLF */
	  lastc = c;
	  continue;
	}
	lastc = c;
	if (c == '\n' || c == '\r')
	{
	  /* EOL */
	  if (eol == 1)
	  {
	    /* DOS */
	    c = '\r';
	    if (fputc(c, fpout) == EOF)
	      break;
	    c = '\n';
	  } else if (eol == 2)
	    c = '\r';
	  else if (eol == 3)
	    c = '\n';
	}
	if (fputc(c, fpout) == EOF)
	  break;
      }
    } while (TRUE);
  } else
    do
    {
      if (in_type == 2)
      {
	if ((c = fgetit(from_type, to_table, fpin)) == EOF)
	  break;
      } else if ((c = fgetc(fpin)) == EOF)
	break;
      if (eol == 0)
      {
	if (out_type == 2)
	{
	  if (c >= OFFSET)
	  {
	    if (fputs(trans_string[c - OFFSET][to_type], fpout) == EOF)
	      break;
	  } else if (in_type == 2)
	  {
	    if (fputc(c, fpout) == EOF)
	      break;
	  } else
	  {
	    if (fputs(trans_ptr[c], fpout) == EOF)
	      break;
	  }
	} else if (fputc(c, fpout) == EOF)
	  break;
      } else
      {
	if (c == '\n' && lastc == '\r')
	{
	  /* CRLF */
	  lastc = c;
	  continue;
	}
	lastc = c;
	if (c == '\n' || c == '\r')
	{
	  /* EOL */
	  if (eol == 1)
	  {
	    /* DOS */
	    c = '\r';
	    if (fputc(c, fpout) == EOF)
	      break;
	    c = '\n';
	  } else if (eol == 2)
	    c = '\r';
	  else if (eol == 3)
	    c = '\n';
	  if (fputc(c, fpout) == EOF)
	    break;
	} else if (out_type == 2)
	{
	  if (c >= OFFSET)
	  {
	    if (fputs(trans_string[c - OFFSET][to_type], fpout) == EOF)
	      break;
	  } else if (in_type == 2)
	  {
	    if (fputc(c, fpout) == EOF)
	      break;
	  } else
	  {
	    if (fputs(trans_ptr[c], fpout) == EOF)
	      break;
	  }
	} else if (fputc(c, fpout) == EOF)
	  break;
      }
    } while (TRUE);
  if (fpout != stdout && fpout != NULL)
    fclose(fpout);
  if (fpin != stdin && fpin != NULL)
    fclose(fpin);
  return 0;
}				/* main */
