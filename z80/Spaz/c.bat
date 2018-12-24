@echo off
C:
cd "C:\CVProg\z80\Spaz"\
..\22nice\22nice.com
copy "C:\CVProg\z80\Spaz"\*.h ..
copy "C:\CVProg\z80\Spaz"\*.c ..
copy "C:\CVProg\z80\Spaz"\*.as ..
cd ..
C -C -V -W1 -O spaz.c
copy spaz.obj "C:\CVProg\z80\Spaz"\spaz.obj
del spaz.c
cd "C:\CVProg\z80\Spaz"\
