@echo off
C:
cd "C:\CVProg\z80\Getput11Demos"\
..\22nice\22nice.com
copy "C:\CVProg\z80\Getput11Demos"\*.h ..
copy "C:\CVProg\z80\Getput11Demos"\*.c ..
copy "C:\CVProg\z80\Getput11Demos"\*.as ..
cd ..
C -C -V -W1 -O icvgm.c
copy icvgm.obj "C:\CVProg\z80\Getput11Demos"\icvgm.obj
del icvgm.c
C -C -V -W1 -O main.c
copy main.obj "C:\CVProg\z80\Getput11Demos"\main.obj
del main.c
C -C -V -W1 -O sounds.c
copy sounds.obj "C:\CVProg\z80\Getput11Demos"\sounds.obj
del sounds.c
C -C -V -W1 -O title.c
copy title.obj "C:\CVProg\z80\Getput11Demos"\title.obj
del title.c
cd "C:\CVProg\z80\Getput11Demos"\
