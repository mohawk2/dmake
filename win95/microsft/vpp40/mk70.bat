if "%1" == "" goto optimized
if "%1" == "rel" goto optimized

set "c_opt=-Od -Gy -GF"
set "ld_opt="
goto build

:optimized
set "c_opt=-O2 -GL"
set "ld_opt=-ltcg"

:build
if exist objects rd /S /Q objects
if exist config.h del config.h
if exist dmake.exe del dmake.exe
md objects
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\infer.obj infer.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\make.obj make.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\stat.obj stat.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\expand.obj expand.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dmstring.obj dmstring.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\hash.obj hash.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dag.obj dag.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dcache.obj unix\dcache.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dmake.obj dmake.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\path.obj path.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\imacs.obj imacs.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\sysintf.obj sysintf.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\parse.obj parse.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\getinp.obj getinp.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\quit.obj quit.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\state.obj state.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dmdump.obj dmdump.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\macparse.obj macparse.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\rulparse.obj rulparse.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\percent.obj percent.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\function.obj function.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\switchar.obj win95\switchar.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dstrlwr.obj msdos\dstrlwr.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\arlib.obj msdos\arlib.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\dirbrk.obj msdos\dirbrk.c
rem Not needed for MSVC 6 and up. Lesser versions not supported
rem cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\tempnam.obj tempnam.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\ruletab.obj win95\microsft\ruletab.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\runargv.obj unix\runargv.c
cl -c %c_flg% -I. -Iwin95 -Iwin95\microsft -Iwin95\microsft\vpp40 /nologo %c_opt% -Zi -Foobjects\rmprq.obj unix\rmprq.c

:link
rem link /nologo /out:dmake.exe @fix95nt\win95\microsft\vpp40\obj.rsp
link /out:dmake.exe -debug -opt:icf,ref %ld_opt% @.\win95\microsft\vpp40\obj.rsp
copy win95\microsft\vpp40\template.mk startup\config.mk

rem clean up our env vars so they dont leak to cmd shell
set "c_opt="
set "ld_opt="
