@echo off
rem ### this script was written by kmx ###

if not exist winnt\mingw\config.h echo You have to run this batch from the root of dmake package!!&pause&goto END

echo ### cleaning 'objects' directory
if exist objects rmdir /S /Q objects
mkdir objects

echo ### gonna compile *.c
set OPTS=-c -Iwinnt\mingw -Iwinnt -I. -O2
@echo on
gcc %OPTS% -o objects\infer.o infer.c
gcc %OPTS% -o objects\make.o make.c
gcc %OPTS% -o objects\stat.o stat.c
gcc %OPTS% -o objects\expand.o expand.c
gcc %OPTS% -o objects\dmstring.o dmstring.c
gcc %OPTS% -o objects\hash.o hash.c
gcc %OPTS% -o objects\dag.o dag.c
gcc %OPTS% -o objects\dmake.o dmake.c
gcc %OPTS% -o objects\path.o path.c
gcc %OPTS% -o objects\imacs.o imacs.c
gcc %OPTS% -o objects\sysintf.o sysintf.c
gcc %OPTS% -o objects\parse.o parse.c
gcc %OPTS% -o objects\getinp.o getinp.c
gcc %OPTS% -o objects\quit.o quit.c
gcc %OPTS% -o objects\state.o state.c
gcc %OPTS% -o objects\dmdump.o dmdump.c
gcc %OPTS% -o objects\macparse.o macparse.c
gcc %OPTS% -o objects\rulparse.o rulparse.c
gcc %OPTS% -o objects\percent.o percent.c
gcc %OPTS% -o objects\function.o function.c
gcc %OPTS% -o objects\dchdir.o win95/dchdir.c
gcc %OPTS% -o objects\switchar.o win95/switchar.c
gcc %OPTS% -o objects\dstrlwr.o msdos/dstrlwr.c
gcc %OPTS% -o objects\arlib.o msdos/arlib.c
gcc %OPTS% -o objects\dirbrk.o msdos/dirbrk.c
gcc %OPTS% -o objects\runargv.o unix/runargv.c
gcc %OPTS% -o objects\rmprq.o unix/rmprq.c
gcc %OPTS% -o objects\ruletab.o win95/microsft/ruletab.c
@echo off

echo ### gonna link dmake.exe
@echo on
gcc -g -O2 -o dmake.exe objects\infer.o objects\make.o objects\stat.o objects\expand.o objects\dmstring.o objects\hash.o objects\dag.o objects\dmake.o objects\path.o objects\imacs.o objects\sysintf.o objects\parse.o objects\getinp.o objects\quit.o objects\state.o objects\dmdump.o objects\macparse.o objects\rulparse.o objects\percent.o objects\function.o   objects\dchdir.o objects\switchar.o objects\dstrlwr.o objects\arlib.o objects\dirbrk.o objects\runargv.o objects\rmprq.o objects\ruletab.o
@echo off

echo ### cleaning 'output' directory
if exist output rmdir /S /Q output
mkdir output
mkdir output\startup
mkdir output\startup\winnt
mkdir output\startup\winnt\mingw

echo ### copying results into 'output' directory
copy .\dmake.exe .\output\dmake.exe
copy .\startup\startup.mk .\output\startup\startup.mk
copy .\winnt\mingw\config.mk .\output\startup\config.mk
copy .\startup\winnt\macros.mk .\output\startup\winnt\macros.mk
copy .\startup\winnt\recipes.mk .\output\startup\winnt\recipes.mk
copy .\startup\winnt\mingw\macros.mk .\output\startup\winnt\mingw\macros.mk 

echo ### done - see results in '%~dp0output\'

:END
