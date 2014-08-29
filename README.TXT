This is DMAKE version 4.12.2 compiled for Win32 using Microsoft Visual C++ 6.0.

For information on how to use DMAKE, see man\dmake.nc.

DMAKE needs a set of startup files that are located in a subdirectory called
'startup' where dmake.exe lives, or in a directory specified by a DMAKEROOT
environment variable.  So, to install it, just copy dmake.exe and the startup
subdirectory to some location on your path.

The default configuration in the startup\config.mk file is for MinGW/gcc on
Windows.  If you change this you'll need to make sure the subdirectories you
name in startup\config.mk are populated with setup files (you can use the
current hierarchy as a model).

NOTE: Previous versions of this build of DMAKE distributed on CPAN have been
configured for Microsoft Visual C++ on Windows by default.  This has now been
changed because most DMAKE users will be using MinGW/gcc; users of Visual C++
will most likely be using nmake.exe instead, although there's no reason why
dmake.exe can't be used with Visual C++.  The previous default configuration
settings (for Visual C++) are left commented-out in startup\config.mk to help
if you want to switch back to them.

The original sources of DMAKE were available from:

    http://dmake.wticorp.com

but this site has not been reachable for some time.  The sources were later
adopted by the Sun OpenOffice.org team and development was continued in their
'tools' project.  With the move of OpenOffice.org to the Apache Software
Foundation, all development on DMAKE has been abandoned.  The home page of the
OpenOffice.org 'tools' project is:

    http://tools.openoffice.org/

and details on the OpenOffice.org version of DMAKE can be found at:

    http://tools.openoffice.org/dmake/

As noted there, the DMAKE sources can still be found in the Apache-Extras site,
and can be downloaded from the stand-alone DMAKE repository at:

    http://svn.codespot.com/a/apache-extras.org/dmake/trunk/  

You can download the latest 'stable' source tarball from:

    http://code.google.com/a/apache-extras.org/p/dmake/downloads/list

This version of DMAKE is compiled from dmake-4.12.2.tar.bz2 taken from the above
downloads address, which actually links to:

    http://dmake.apache-extras.org.codespot.com/files/dmake-4.12.2.tar.bz2

The file PATCH.TXT contains the changes that I made to those sources, namely, a
change to the version string displayed by 'dmake -V', plus a couple of other
changes inspired by the changes made by Gurusamy Sarathy in his DMAKE 4.10, PL 1
(version 4.1, patch level 1) build which is available from:

    http://search.cpan.org/~gsar/dmake-4.1pl1-win32/

Enjoy!

Steve Hay
shay@cpan.org
03-Apr-2014
