@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET INCLUDE=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include;^
C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\include;^
C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\um;^
C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\shared;^
C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\winrt;^
include;^
source;^
meta\win\include;

SET LIB=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib;^
C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\lib;^
C:\Program Files (x86)\Windows Kits\10\lib\10.0.16299.0\ucrt\x86;^
C:\Program Files (x86)\Windows Kits\10\lib\10.0.16299.0\um\x86;^
meta\win\lib;

SET PATH=^
%SystemRoot%\system32;^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin;^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE;^
C:\Program Files (x86)\Windows Kits\10\bin\10.0.16299.0\x86;^
meta\win\bin;

SET TARGET_NAME=HatchGameEngine
SET TARGET_FOLDER=builds\win\
SET TARGET_SUBSYSTEM=/SUBSYSTEM:WINDOWS
SET TARGET_SUBSYSTEM=/SUBSYSTEM:CONSOLE

SET SRC_FOLDER=%~dp0source\
SET OBJ_FOLDER=out\win\
SET OBJ_LIST=

IF NOT EXIST %OBJ_FOLDER% MKDIR %OBJ_FOLDER%
IF NOT EXIST %TARGET_FOLDER% MKDIR %TARGET_FOLDER%

ECHO Compiling: %TARGET_NAME%...

CD "tools"
"makeheaders.exe" ../source
CD ..

IF EXIST meta\icon.ico (
   ECHO 101 ICON "icon.ico"> meta\icon.rc
   RC /nologo meta\icon.rc
   SET OBJ_LIST=meta\icon.res !OBJ_LIST!
   DEL /F meta\icon.rc
)

FOR /f %%j IN ('dir /s /b %SRC_FOLDER%*.cpp') DO (
   SET SRC_FILE=%%j
   SET OBJ_FILE=!SRC_FILE:%SRC_FOLDER%=%OBJ_FOLDER%!
   SET OBJ_FILE=!OBJ_FILE:.cpp=.obj!
   SET OBJ_LIST=!OBJ_FILE! !OBJ_LIST!
   FOR %%k IN (!OBJ_FILE!) DO (
      IF NOT EXIST %%~dpk MKDIR %%~dpk
   )
   CL "!SRC_FILE!" /nologo /c /DWIN32 /DGLEW_STATIC /DCURL_STATICLIB /DUSING_LIBAV /DTARGET_NAME=\"!TARGET_NAME!\" /EHsc /FS /Gd /MD /Fo!OBJ_FILE! /Fd%OBJ_FOLDER%%TARGET_NAME%.pdb
   IF NOT !errorlevel! == 0 (
      GOTO END_OF_BAT
   )
)
LINK ^
   /OUT:"!TARGET_FOLDER!!TARGET_NAME!.exe" ^
   !TARGET_SUBSYSTEM! ^
   /MACHINE:X86 ^
   /NOLOGO !OBJ_LIST! ^
   Ws2_32.lib ^
   Wldap32.lib ^
   Advapi32.lib ^
   Crypt32.lib ^
   Normaliz.lib ^
   libcurl.lib ^
   zlibstat.lib ^
   glew32s.lib ^
   opengl32.lib ^
   freetype.lib ^
   libjpeg.lib ^
   libpng16.lib ^
   libogg_static.lib ^
   libvorbis_static.lib ^
   libvorbisfile_static.lib ^
   avcodec.lib ^
   avformat.lib ^
   avutil.lib ^
   swscale.lib ^
   swresample.lib ^
   SDL2.lib ^
   SDL2main.lib ^
   kernel32.lib ^
   user32.lib

ECHO Finishing: %TARGET_NAME%.exe
:END_OF_BAT
PAUSE
