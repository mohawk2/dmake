# Dmake

Dmake is a make utility similar to GNU make or the Workshop dmake. This
utility has an irregular syntax but is available for FreeBSD, Linux,
Solaris, Win32 and other platforms. It is used by the OpenOffice.org
build system, although for some time now Apache OpenOffice.Org and
its derivatives have been considering replacing it definitely with a
GNUmake-only build system.

This version of dmake is a modified version of Dennis Vadura's GPL'ed
dmake. The original sources were available on WTIcorp.com. As this
site has not been reachable for some time the SUN OpenOffice.org team
adopted this utility and continued its development in OOo's Version
Control System. With the move of OOo to the Apache Software Foundation,
this software is completely abandoned and not recommended for general use.

Added features in dmake:

* smaller/greater arithmetic like: 

```
.IF 400<=200
```

* Boolean expressions "or", "and" and nesting thereof: 

```
.IF (("$(OS)"=="MACOSX"&&"$(COM)"=="GCC")||"$(OS)"=="LINUX"||"$(OS)"=="SOLARIS") && "$(GUIBASE)"=="unx"
```

Those are only two examples, read the NEWS file for more features
and changes. Note: Beside fixed bugs the dmake versions are downward
compatible.

# Building from a git checkout

```
./autogen.sh
make # must be GNU make
make -j1 check # fails on parallel
```

To run a single test:

```
./prove tests/targets-14
```
