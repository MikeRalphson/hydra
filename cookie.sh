#! /bin/sh
# This is a shell archive, meaning:
# 1. Remove everything above the #! /bin/sh line.
# 2. Save the resulting text in a file.
# 3. Execute the file with /bin/sh (not csh) to create the files:
# Makefile
# README
# cookhash.c
# cookie.1
# cookie.c
# cookie.h
# This archive created: Tue Nov 13 13:31:02 1990
export PATH; PATH=/bin:$PATH
if test -f 'Makefile'
then
 echo shar: will not over-write existing file "'Makefile'"
else
cat << \SHAR_EOF > 'Makefile'
# makefile for karl's PD fortune cookie program

all: cookie cookhash

cookie: cookie.h
 cc -O -o cookie cookie.c

cookhash:
 cc -O -o cookhash cookhash.c

install:
 cp cookie cookhash /usr/local/bin
SHAR_EOF
fi # end of overwriting check
if test -f 'README'
then
 echo shar: will not over-write existing file "'README'"
else
cat << \SHAR_EOF > 'README'

The third release of "cookie"   20-Oct-1990
-------------------------------------------

Cookie is a program to randomly retrieve cookies from a fortune cookie file.
Each cookie can contain an arbitrary number of lines of text.  By generating
a small "hash" file containing the addresses of each cookie in the cookie
file, cookie has a high-performance way to look up cookies, plus it is "fair,"
which is to say that short cookies are equally as likely to be selected as 
long ones.  (The technique of simply randomly seeking into the cookie file
and looking backwards and forwards from there to find the beginning and
end of the cookie text would bias the cookie program toward large cookies.)

This is the same code as in my second release, but it's been almost two years
since that release and I've gotten numerous requests for the program, plus
I'm about to pump out a batch of new cookies, plus there wern't alt.sources
archives (that I'm aware of) then, plus there's a manpage now, blah blah
blah.  Flames via email or alt.sources.d if you must...

The code is again released into the public domain without restriction.
I ask that you retain my name in the source code if you redistribute
this stuff,
and that you redistribute source along with binaries.

No warranties are expressed or implied -- this is free code.  We do not
have
a contract.

The code is written for System V but the only area of incompatibility
should be
the rand() function.  Only minor hacking should be necessary to port to
BSD,
for example.  (If someone gets the urge, please make a version that
works
with #ifdefs for both, test the BSD version and forward it back to me,
OK?)

It should be possible to make cookie work pretty painlessly on MS-DOS,
Minix,
etc.  Tested updates for popular and not-overwhelmingly-twisted systems,
with
#ifdefs, are solicited.  I seem to recall that cookie works as-is on the
Amiga.

To use, unshar this archive and do a 'make' to compile 'cookie' and
'cookhash'.

Then collect a bunch of cookies.  Eric Townsend just posted quite a few
to
alt.sources.  I will be posting a couple of new batches.  I hope to
post
information soon to alt.sources.d as to where an archive site for all
my
cookies (pushing 500 KB) can be found.

Cookies are separated by lines containing two percent-signs and nothing
else, for example:

"I just thought of something funny, your mother."
-- Cheech Marin
%%
"He can shout, don't hear you."
-- The Firesign Theatre
%%

...and so forth.

The include file "cookie.h" defines the location of the cookie file as
being
"/usr/local/lib/sayings".  If you want to put it elsewhere, change
cookie.h
and rebuild 'cookie'.  

Cookie needs a hash file for the cookie file, by default called
"/usr/local/lib/sayhash".  This is created by the 'cookhash' program.
Cookhash is simply a filter that reads a cookie file in as stdin and
writes
a cookie hash file to stdout.  Thus, if you've moved the cookie file to
/usr/local/lib, 'cd' there and do a "cookhash <sayings >sayhash" to
create the 
hash file.

After that, 'cookie' should produce a cookie.  Cookie can also be
executed
with two arguments, the name of a cookie file followed by the name of
its hash file, useful for creating aliases for alternate cookie files
so
you can get just Zippy the Pinhead quotes, for example.

If you find quotes in the file that are unattributed and you know the
attributions, please mail them to karl@sugar.hackercorp.com or
uunet!sugar!karl

I also collect cookies.  If you see good ones, please forward them.  (If
you
got them from me, please don't!)

A lot of people think it's fun to get a cookie every time they log in,
so 
they put "cookie" in their .profile or .login file.

Regards,
Karl (karl@sugar.hackercorp.com) @ The Hacker's Haven, Missouri City,
Texas 
(The name will be changing because hacker has lost its old meaning --
sigh)
SHAR_EOF
fi # end of overwriting check
if test -f 'cookhash.c'
then
 echo shar: will not over-write existing file "'cookhash.c'"
else
cat << \SHAR_EOF > 'cookhash.c'
/* cookhash - read a sayings file and generate an index file
 * by Karl Lehenbauer (karl@sugar.uu.net, uunet!sugar!karl)
 *  cookhash.c  1.1  1/12/89
 */

#include <stdio.h>

#define YES 1
#define NO 0
#define METACHAR '%'

main(argc,argv)
int argc;
char *argv[];
{
 int c, sawmeta;
 long charpos = 0;

 if (argc != 1)
 {
  fprintf(stderr,"usage: cookhash <cookiefile >hashfile\n");
  exit(1);
 }

 /* write out the "address" of the first cookie */
 puts("000000");

 /* read the cookie until the end,
  *   whenever the end-of-cookie ("%%") sequence is found,
  *   the "address" (file position) of the first byte following
  *   it (start of next cookie) is written to the index (hash) file
  */
 while ((c = getchar()) != EOF)
 {
  if (c == METACHAR)
  {
   if (sawmeta)
   {
    printf("%06lx\n",charpos+2);
    sawmeta = NO;
   }
   else
    sawmeta = YES;
  }
  else
   sawmeta = NO;
  charpos++;
 } exit(0);
}

/* end of cookhash.c */
SHAR_EOF
fi # end of overwriting check
if test -f 'cookie.1'
then
 echo shar: will not over-write existing file "'cookie.1'"
else
cat << \SHAR_EOF > 'cookie.1'
.TH COOKIE 6
.SH NAME
cookie \- show a fortune cookie
.SH SYNOPSIS
.B cookie
[ cookie-file hash-file ]
.sp
.B /usr/local/lib/cookhash
< sayings > hash-file
.SH DESCRIPTION
.I Cookie
shows the user a randomly chosen quote from a file containing
fortune cookies, sayings, jokes, aphorisms, quotes and so on.
.P
Cookie can also be executed with two arguments, the name of a cookie
file
followed by the name of its hash file.
.P
To create the hash file, use
.I /usr/local/lib/cookhash
which reads the sayings from standard input and writes the hash values
to
standard output. (The hash file contains the ASCII-formatted addresses
of
the cookies found in the cookie file.  It is used by 
.I cookie
to look up cookies quickly and to help to insure that all cookies are
equally likely to be chosen.)
.P
Sayings within the input file are separated by lines containing 
.B %% .
.SH SEE ALSO
fortune(6)
.SH FILES
/usr/local/lib/sayings, /usr/local/lib/sayhash
.SH AUTHOR
Karl Lehenbauer (karl@sugar.hackercorp.com),
Missouri City, Texas, USA

SHAR_EOF
fi # end of overwriting check
if test -f 'cookie.c'
then
 echo shar: will not over-write existing file "'cookie.c'"
else
cat << \SHAR_EOF > 'cookie.c'
/* cookie - print out an entry from the sayings file
 * by Karl Lehenbauer (karl@sugar.uu.net, uunet!sugar!karl)
 *  cookie.c  1.1  1/12/89
 */

#include <stdio.h>
#include "cookie.h"

#define ENTSIZE 7L
#define METACHAR '%'
#define YES 1
#define NO 0

char *sccs_id = "@(#) fortune cookie program 1.1 1/12/89 by K.
Lehenbauer";

extern long lseek(), time();
extern int rand();

char *cookiename = COOKIEFILE;
char *hashname = HASHFILE;

/* really_random - insure a good random return for a range, unlike an
arbitrary
 * random() % n, thanks to Ken Arnold, Unix Review, October 1987
 * ...likely needs a little hacking to run under Berkely
 */
#define RANDOM_RANGE ((1 << 15) - 1)
int really_random(my_range)
int my_range;
{
 int max_multiple, rnum;

 max_multiple = RANDOM_RANGE / my_range;
 max_multiple *= my_range;
 while ((rnum = rand()) >= max_multiple)
  continue;
 return(rnum % my_range);
}

main(argc,argv)
int argc;
char *argv[];
{
 int nentries, oneiwant, c, sawmeta = 0;
 FILE *hashf, *cookief;
 long cookiepos;

 /* if we got exactly three arguments, use the cookie and hash
  * files specified
  */
 if (argc == 3)
 {
  cookiename = argv[1];
  hashname = argv[2];
 }
 /* otherwise if argc isn't one (no arguments, specifying the
  * default cookie file), barf
  */
 else if (argc != 1)
 {
  fputs("usage: cookie cookiefile hashfile\n",stderr);
  exit(1);
 }

 /* open the cookie file for read */
 if ((cookief = fopen(cookiename,"r")) == NULL)
 {
  perror(cookiename);
  exit(2);
 }

 /* open the hash file for read */
 if ((hashf = fopen(hashname,"r")) == NULL)
 {
  perror(hashname);
  exit(2);
 }

 /* compute number of cookie addresses in the hash file by
  * dividing the file length by the size of a cookie address
  */
 if (fseek(hashf,0L,2) != 0)
 {
  perror(hashname);
  exit(3);
 }
 nentries = ftell(hashf) / 7L;

 /* seed the random number generator with time in seconds plus
  * the program's process ID - it yields a pretty good seed
  * again, thanks to Ken Arnold
  */
 srand(getpid() + time(NULL));

 /* generate a not really random number */
 oneiwant = really_random(nentries);

 /* locate the one I want in the hash file and read the
  * address found there
  */
 fseek(hashf,(long)oneiwant * ENTSIZE, 0);
 fscanf(hashf,"%lx",&cookiepos);

 /* seek cookie file to cookie starting at address read from hash */
 fseek(cookief,cookiepos,0);

 /* get characters from the cookie file and write them out
  * until finding the end-of-fortune sequence, '%%'
  */
 while ((c = fgetc(cookief)) != EOF && sawmeta < 2)
 {
  if (c != METACHAR)
  {
   if (sawmeta)
    putchar(METACHAR);
   putchar(c);
   sawmeta = 0;
  }
  else
   sawmeta++;
 } exit(0);
}

/* end of cookie.c */
SHAR_EOF
fi # end of overwriting check
if test -f 'cookie.h'
then
 echo shar: will not over-write existing file "'cookie.h'"
else
cat << \SHAR_EOF > 'cookie.h'
/* cookie.h - include file for karl's PD fortune cookie program 
 * by Karl Lehenbauer (karl@sugar.uu.net, uunet!sugar!karl)
 * cookie.h 1.1 1/12/89
 */

#define COOKIEFILE "/usr/local/lib/sayings"
#define HASHFILE "/usr/local/lib/sayhash"

/* end of cookie.h */
SHAR_EOF
fi # end of overwriting check
# End of shell archive
exit 0
