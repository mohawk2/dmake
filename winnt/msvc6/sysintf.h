/* RCS  $Id: sysintf.h,v 1.3 2007-01-18 09:53:49 vg Exp $
--
-- SYNOPSIS
--      Interfaces for sysintf.c
--
-- DESCRIPTION
--      Abstractions of functions in sysintf.c
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

#define DMSTAT stat
#define VOID_LCACHE(l,m)
#define GETPID _psp
#define Hook_std_writes(A)

extern char * tempnam();
extern char * getcwd();

/*
** standard C items
*/

/* in sysintf.c: SIGQUIT is used, this is not defined in MinGW */
#ifndef SIGQUIT
#   define SIGQUIT SIGTERM
#endif

/*
** make parameters
*/
#ifdef _POSIX_NAME_MAX
#undef  _POSIX_NAME_MAX
#endif
#define _POSIX_NAME_MAX 12

#ifdef _POSIX_PATH_MAX
#undef _POSIX_PATH_MAX
#endif
#define _POSIX_PATH_MAX _MAX_PATH

#if _MSC_VER >= 1400
#  define fputc(_c,_stream) _fputc_nolock(_c,_stream)
#endif
