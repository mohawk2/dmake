/* RCS  $Id: sysintf.h,v 1.3 2007-01-18 09:50:52 vg Exp $
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

#define HAVE_DMPORTSTAT 1
/* override default implementation */
#undef DMPORTSTAT_T
#undef DMPORTSTAT
#undef DMPORTSTAT_SUCCESS
#undef DMPORTSTAT_MTIME
#undef DMPORTSTAT_ISDIR
#define DMPORTSTAT_T WIN32_FILE_ATTRIBUTE_DATA
#define DMPORTSTAT(path, buf) GetFileAttributesEx((path), GetFileExInfoStandard, (buf))
#define DMPORTSTAT_SUCCESS(x) ((x) != 0)
#define DMPORTSTAT_MTIME(x) (FileTimeTo_time_t(&((x)->ftLastWriteTime)))
#define DMPORTSTAT_ISDIR(x) ((x)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)

#define VOID_LCACHE(l,m)
#define GETPID _psp
#define Hook_std_writes(A)

extern char * tempnam();
extern char * getcwd();

/*
** standard C items
*/

/*
** make parameters
*/
#ifdef _POSIX_NAME_MAX
#undef  _POSIX_NAME_MAX
#endif
#define _POSIX_NAME_MAX _MAX_FNAME

#ifdef _POSIX_PATH_MAX
#undef _POSIX_PATH_MAX
#endif
#define _POSIX_PATH_MAX _MAX_PATH
