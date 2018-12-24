@echo off
C:
cd "C:\CVProg\z80\Spaz"\
..\22nice\22nice.com
cd ..
cls
echo DO PASTE HERE!
link
copy o "C:\CVProg\z80\Spaz"\result.rom
del o
copy m "C:\CVProg\z80\Spaz"\map.txt
del m
