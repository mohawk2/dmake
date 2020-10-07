/*
--
-- SYNOPSIS
--      Configurarion include file.
-- 
-- DESCRIPTION
-- 	There is one of these for each specific machine configuration.
--	It can be used to further tweek the machine specific sources
--	so that they compile.
--
-- AUTHOR
--      Dennis Vadura, dvadura@dmake.wticorp.com
--
-- WWW
--      http://dmake.wticorp.com/
--
-- COPYRIGHT
--      Copyright (c) 1996,1997 by WTI Corp.  All rights reserved.
-- 
--      This program is NOT free software; you can redistribute it and/or
--      modify it under the terms of the Software License Agreement Provided
--      in the file <distribution-root>/readme/license.txt.
--
-- LOG
--      Use cvs log to obtain detailed change logs.
*/

/* Attention! In the UNIX like builds with the ./configure ; make
   procedure a config.h is generated. The autogenerated config.h
   must not be there to compile dmake with MSC and the
   "dmake\make.bat win95-vpp40" command. This file sets (among other
   things) the needed HAS_... and HAVE_... macros.

   Don't forget to update the PACKAGE and VERSION macros!
*/

#if defined (_MSC_VER)
# if _MSC_VER < 500
	Force a compile-time blowup.
	Do not define "#define _MSC_VER" for MSC compilers earlier than 5.0.
# endif
#endif

/* define this for configurations that don't have the coreleft function
 * so that the code compiles.  To my knowledge coreleft exists only on
 * Turbo C, but it is needed here since the function is used in many debug
 * macros. */
#define coreleft() 0L

/* MSC Version 4.0 doesn't understand SIGTERM, later versions do. */
/* config.h is included before signal.h therefore test MSC version */
#if _MSC_VER < 500
#   define SIGTERM SIGINT
#endif

/* Fixes unimplemented line buffering for MSC 5.x and 6.0.
 * MSC _IOLBF is the same as _IOFBF
 */
#if defined(MSDOS) && defined (_MSC_VER)
#   undef  _IOLBF
#   define _IOLBF   _IONBF
#endif

/* in alloc.h: size_t is redefined
 * defined in stdio.h which is included by alloc.h
 */
#if defined(MSDOS) && defined (_MSC_VER)
#   define _TYPES_
#endif

/* in sysintf.c: SIGQUIT is used, this is not defined in MSC */
#ifndef SIGQUIT
#   define SIGQUIT SIGTERM
#endif

/* MSC didn't seem to care about CONST in the past */
#ifndef CONST
#   define CONST
#endif

/* Build info string */
#define BUILDINFO "Windows / MS Visual C++ " dmstr2(_MSC_FULL_VER)

/* Assume case insensitive file system. */
#define CASE_INSENSITIVE_FS 1


/* These functions are available! (this is tested only with MSVC++ 6.0) */
#define HAVE_UTIME_NULL 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `setvbuf' function. */
#define HAVE_SETVBUF 1

/* These defines are needed for itypes.h! (this is tested only with MSVC++ 6.0) */
/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlwr' function. */
#define HAVE_STRLWR 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `tempnam' function. */
#define HAVE_TEMPNAM 1

/* Define to 1 if you have the `tzset' function. */
#define HAVE_TZSET 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Name of package */
#define PACKAGE "dmake"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "dmake"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "dmake 4.13"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "dmake"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.13"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

#ifndef MSDOS
#   define MSDOS 1
#endif

/* Version number of package */
#define VERSION "4.13"

/* a small problem with pointer to voids on some unix machines needs this */
#define DMPVOID void *

/* Use my own tempnam for MSC Version less than 6.0 */
#if _MSC_VER < 600
#   define tempnam dtempnam
#endif

