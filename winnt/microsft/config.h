/* RCS  $Id: config.h,v 1.9 2008-03-05 18:41:51 kz Exp $
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

/* Name and version number of this package */
#define PACKAGE "dmake"
#define VERSION "4.12"
#define BUILDINFO "Windows / MS Visual C++"

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
/*
#ifndef SIGTERM
#   define SIGTERM SIGINT
#endif
*/

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

#ifndef MSDOS
#   define MSDOS 1
#endif

/* a small problem with pointer to voids on some unix machines needs this */
#define DMPVOID void *

/* Use my own tempnam */
#define tempnam dtempnam

