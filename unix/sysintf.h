/* RCS  $Id: sysintf.h,v 1.4 2007-10-15 15:53:38 ihi Exp $
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

#ifdef _WIN32
#  error This is unix/sysintf.h , this can not be included on Win32
#endif
#define DMSTAT stat
#define VOID_LCACHE(l,m) (void) void_lcache(l,m)
#define Hook_std_writes(A)
#define GETPID getpid()

#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif

#ifndef S_IFMT
#define S_IFMT 0170000
#endif

/*
** standard C items
*/

/*
** DOS interface standard items
*/
#define	getswitchar()	'-'

/*
** Make parameters
*/
