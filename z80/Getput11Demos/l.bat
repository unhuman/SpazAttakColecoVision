@echo off
C:
cd "C:\CVProg\z80\Getput11Demos"\
..\22nice\22nice.com
cd ..
cls
echo DO PASTE HERE!
link
copy o "C:\CVProg\z80\Getput11Demos"\result.rom
del o
copy m "C:\CVProg\z80\Getput11Demos"\map.txt
del m
