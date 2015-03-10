/*
--
-- SYNOPSIS
--      External declarations for dmake functions.
-- 
-- DESCRIPTION
--	ANSI is a macro that allows the proper handling of ANSI style
--	function declarations.
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

#ifndef EXTERN_h
#define EXTERN_h

/* For MSVC++ needs to include windows.h first to avoid problems with
 * type redefinitions. Include it also for MinGW for consistency. */
#if defined(__MINGW32__) || defined(_MSC_VER)
/* silence warnings about using null term strings in >= VC 2005
sysintf.c(918) : warning C4996: 'strcat' was declared deprecated
        Message: 'This function or variable may be unsafe. Consider using strcat
_s instead. To disable deprecation, use _CRT_SECURE_NO_DEPRECATE. See online hel
p for details.' */
#define _CRT_SECURE_NO_DEPRECATE
/* silence warnings about using POSIX name funcs in >= VC 2005
sysintf.c(1132) : warning C4996: 'unlink' was declared deprecated
        C:\Program Files\Microsoft Visual Studio 8\VC\INCLUDE\stdio.h(290) : see
 declaration of 'unlink'
        Message: 'The POSIX name for this item is deprecated. Instead, use the I
SO C++ conformant name: _unlink. See online help for details.' */
#define _CRT_NONSTDC_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "config.h"

/* Define this for the RS/6000 if it breaks something then we have to put a
 * #ifdef around it. */
#if defined(rs6000)
#define _POSIX_SOURCE
#endif

#include <stdio.h>
#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#else
# include <types.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if HAVE_UTIME_H
# include <utime.h>
#endif

#define DMPVOID void *

#include <signal.h>
#include "itypes.h"
#include "stdmacs.h"
#include "alloc.h"
#include "db.h"
#include "dstdarg.h"
#include "dmake.h"
#include "struct.h"
#include "vextern.h"
#include "public.h"

/* Include this last as it invalidates some functions that are defined
 * externally above and turns them into no-ops.  Have to do this after
 * the extern declarations however. */
#include "posix.h"



/* Common declarations
 * ===================
 * are better made here then in local public.h. So far dmake didn't follow
 * this strategy but new functions will be added here. */

/* Use our own implementation if no library function is present. */
#ifndef HAVE_STRLWR
/* from dmstring.c */
char *strlwr(char *p);
#endif

/* from function.c */
char *exec_normpath(char *args);

/* from make.c */
void Unmake(CELLPTR cp);

/* from path.c */
void Clean_path(char *path);
char *normalize_path(char *path);

/* from sysintf.c */
/* cygdospath()/DO_WINPATH() are only needed for the .WINPATH attribute
 * on cygwin. */
#if __CYGWIN__
char *cygdospath(char *src, int winpath);
# define DO_WINPATH(p) cygdospath(p, UseWinpath)
#else
# define DO_WINPATH(p) p
#endif


/* Define some useful macros. This is done here and not in config.h
 * to keep this changes useful even when not using the autotools based
 * build, i.e. using config.h files that are local to the architecture. */
#if defined(_WIN32) || defined(__CYGWIN__) || defined(MSDOS) || defined(OS2) || defined(__EMX__)
#  define HAVE_DRIVE_LETTERS 1
#endif

#if defined(_WIN32) || defined(MSDOS) || defined(OS2) && !defined(__CYGWIN__)
#  define NULLDEV "NUL"
#else
#  define NULLDEV "/dev/null"
#endif

/* For MSVC 6.0 and newer and MinGW use the CreateProcess() function. */
#if defined(__MINGW32__) || defined(_MSC_VER) && _MSC_VER >= 1200
#  define USE_CREATEPROCESS 1
#else
/* #undef USE_CREATEPROCESS */
#endif

/*  CreateProcess() is spawn-like. */
#if ENABLE_SPAWN && ( HAVE_SPAWN_H || __CYGWIN__ || __EMX__) || defined(USE_CREATEPROCESS)
#  define USE_SPAWN 1
#else
/* #undef USE_SPAWN */
#endif

/* Work around some of the functions that may or may not exist */
#if ! HAVE_TZSET
#if HAVE_SETTZ
#  define tzset() settz()
#else
#  warn "tzset is not supported, null out"
#  define tzset()
#endif
#endif

#ifdef _WIN32
#  define FileTimeTo_time_t(ft) ((time_t)((*((unsigned __int64 *)ft) - 116444736000000000ULL)/10000000ULL))
#endif

/* Get the working directory fall back code */
#if ! HAVE_GETCWD
#if HAVE_GETWD
#  define getcwd(buf,len) getwd(buf)
#else
#  error "You have no supported way of getting working directory"
#endif
#endif

/*  If setvbuf is not available, hope the OS has some buffering by default already */
#if ! HAVE_SETVBUF
#  define setvbuf(fp,bp,type,len)
#endif

/* coreleft is used in some debug macros. Only Turbo C seems to provide
 * this function. Define it here so that the code compiles. */
#ifdef DBUG
#define coreleft() 0L
#endif

/* generic implementation, override in platform specific sysintf.h
 * if needed with #undef #define */
#define DMPORTSTAT_T struct stat
#define DMPORTSTAT(path, buf) DMSTAT(path, buf)
#define DMPORTSTAT_SUCCESS(x) ((x) == 0)
#define DMPORTSTAT_MTIME(x) ((x)->st_mtime)
#define DMPORTSTAT_ISDIR(x) ((x)->st_mode & S_IFDIR)

#endif
