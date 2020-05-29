@ECHO OFF

SET OBJ_FOLDER=out\nx\
IF NOT EXIST %OBJ_FOLDER% MKDIR %OBJ_FOLDER%

CD "tools"
"makeheaders.exe" ../source
CD ..

make -f Makefile.nx
if NOT %errorlevel% == 0 goto :FINISHED

COPY /Y "builds\nx\HatchGameEngine.nro" "Z:\switch\Binj\HatchGameEngine.nro"
C:\devkitPro\tools\bin\nxlink.exe builds/nx/HatchGameEngine.nro -s

:FINISHED
pause
