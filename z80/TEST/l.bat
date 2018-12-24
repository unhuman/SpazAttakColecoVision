@echo off
C:
cd "C:\CVProg\z80\TEST"\
..\22nice\22nice.com
cd ..
cls
echo DO PASTE HERE!
link
copy o "C:\CVProg\z80\TEST"\result.rom
del o
copy m "C:\CVProg\z80\TEST"\map.txt
del m
