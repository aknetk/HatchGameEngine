@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET INCLUDE=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\um;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\shared;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\winrt;^
include;^
source;^
meta\win\include;

SET LIB=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib;^
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
SET TARGET_FOLDER=%temp%\HatchGameEngine\builds\win\
SET TARGET_SUBSYSTEM=/SUBSYSTEM:CONSOLE
SET TARGET_SUBSYSTEMSSS=/SUBSYSTEM:WINDOWS

SET SRC_FOLDER=%~dp0source\
SET OBJ_FOLDER=out\win\
SET OBJ_FOLDER=%temp%\HatchGameEngine\out\win\
SET OBJ_LIST=

SET ICO_FOLDER=meta\
SET ICO_FOLDER=source_galactic\Meta\win\

SET LIB_LIBAV=avcodec.lib avformat.lib avutil.lib swscale.lib swresample.lib /DUSING_LIBAV libcurl.lib /DUSING_CURL

IF NOT EXIST %OBJ_FOLDER% MKDIR %OBJ_FOLDER%
IF NOT EXIST %TARGET_FOLDER% MKDIR %TARGET_FOLDER%

ECHO Compiling: %TARGET_NAME%...

CD "tools"
"makeheaders.exe" ../source
CD ..

IF EXIST %ICO_FOLDER%icon.ico (
   DEL /F %ICO_FOLDER%icon.res
   RC /nologo %ICO_FOLDER%icon.rc
   SET OBJ_LIST=%ICO_FOLDER%icon.res !OBJ_LIST!
)

FOR /f %%j IN ('dir /s /b %SRC_FOLDER%*.cpp') DO (
   SET SRC_FILE=%%j
   SET OBJ_FILE=!SRC_FILE:%SRC_FOLDER%=%OBJ_FOLDER%!
   SET OBJ_FILE=!OBJ_FILE:.cpp=.obj!
   SET OBJ_LIST=!OBJ_FILE! !OBJ_LIST!
   FOR %%k IN (!OBJ_FILE!) DO (
      IF NOT EXIST %%~dpk MKDIR %%~dpk
   )
   CL "!SRC_FILE!" /nologo /c /DWIN32 /DGLEW_STATIC /DCURL_STATICLIB /DTARGET_NAME=\"!TARGET_NAME!\" /DDEBUG /EHsc /O2 /FS /Gm /Gd /MD /Zi /Fo!OBJ_FILE! /Fd%OBJ_FOLDER%%TARGET_NAME%.pdb
   IF NOT !errorlevel! == 0 (
      GOTO END_OF_BAT
   )
)

ECHO   Linking: %TARGET_NAME%...

LINK ^
   /OUT:"!TARGET_FOLDER!!TARGET_NAME!.exe" ^
   !TARGET_SUBSYSTEM! ^
   /MACHINE:X86 ^
   /NOLOGO !OBJ_LIST! ^
   /DEBUG ^
   /PDB:%TARGET_FOLDER%%TARGET_NAME%.pdb ^
   Ws2_32.lib ^
   Wldap32.lib ^
   Advapi32.lib ^
   Crypt32.lib ^
   Normaliz.lib ^
   zlibstat.lib ^
   glew32s.lib ^
   opengl32.lib ^
   freetype.lib ^
   libjpeg.lib ^
   libpng16.lib ^
   libogg_static.lib ^
   libvorbis_static.lib ^
   libvorbisfile_static.lib ^
   SDL2.lib ^
   SDL2main.lib ^
   kernel32.lib ^
   user32.lib

ECHO Finishing: %TARGET_NAME%.exe
copy %TARGET_FOLDER%%TARGET_NAME%.exe builds\win\%TARGET_NAME%.exe
:END_OF_BAT
PAUSE
