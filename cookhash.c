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
