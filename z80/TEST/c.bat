@echo off
C:
cd "C:\CVProg\z80\TEST"\
..\22nice\22nice.com
copy "C:\CVProg\z80\TEST"\*.h ..
copy "C:\CVProg\z80\TEST"\*.c ..
copy "C:\CVProg\z80\TEST"\*.as ..
cd ..
C -C -V -W1 -O music.c
copy music.obj "C:\CVProg\z80\TEST"\music.obj
del music.c
C -C -V -W1 -O test.c
copy test.obj "C:\CVProg\z80\TEST"\test.obj
del test.c
cd "C:\CVProg\z80\TEST"\
