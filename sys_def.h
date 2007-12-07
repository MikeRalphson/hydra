/* stdio and system definition. 1993/04/23 BKi */
#ifndef SYS_DEF_H
#define SYS_DEF_H
#include <stdio.h>
#ifdef __TURBOC__
#define __ANSI
#ifdef __TOS__
#define __ATARI
#else
#define __PC
#endif				/* __TOS__ */
#else
#ifdef _STDIO_
#define __MIPS
#define __UNIX
#else
#ifdef _H_STDIO
#define __RS6000
#define __UNIX
#else
#ifdef _h_STDIO
#define __RT
#define __UNIX
#else
#ifdef sun
#define __UNIX
#else
#ifdef sgi
#define __UNIX
#define __ANSI
#else
#ifdef ns32000
#define __UNIX
#define __X32
#else
/* #ifdef __FILE */
#ifdef __STDIO_LOADED
#define __VAX
#define __ANSI
#else
#define __UNIX
#endif				/* __FILE __VAX */
#endif				/* X32 */
#endif				/* sgi */
#endif				/* sun */
#endif				/* _h_STDIO __RT */
#endif				/* _H_STDIO __RS6000 */
#endif				/* _STDIO_ __MIPS */
#endif				/* __TURBOC__ */
#ifndef __ANSI
#ifdef __GNUC__
#define __ANSI
#endif
#endif				/* gcc */
#endif				/* SYS_DEF_H */
