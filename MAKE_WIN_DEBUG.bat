@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

REM User define-able options
REM =================================
REM =================================

REM --------====> NOTE: <====--------
REM All directories must end in an "\"
REM =================================

REM ====> Directory where Visual C is installed.
SET VC_DIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\

REM ====> Directory where Windows SDK is installed.
SET WIN_SDK_DIR=C:\Program Files (x86)\Windows Kits\10\

REM ====> Version of Windows SDK.
SET WIN_SDK_VER=10.0.16299.0

REM ====> Name of .EXE and name of .LOG file when game is run.
SET TARGET_NAME=HatchGameEngine

REM ====> Temporary directory to store .EXE file.
SET TARGET_FOLDER=%temp%\HatchGameEngine\builds\win\

REM ====> Temporary directory to store compilation files.
SET OBJ_FOLDER=%temp%\HatchGameEngine\out\win\

REM ====> Directory of where to place the finished .EXE file.
SET BUILD_OUTPUT_FOLDER=%~dp0builds\win\

REM ====> Directory of where to find program libraries, and create Windows .EXE metadata and icons.
SET META_WIN_FOLDER=%~dp0meta\win\

SET USING_CONSOLE_WINDOW=1
SET USING_COMPILER_OPTS=0
SET USING_LIBAV=0
SET USING_CURL=0
SET USING_LIBPNG=1
SET USING_LIBJPEG=1
SET USING_LIBOGG=1
SET USING_FREETYPE=1

REM Automatically made variables
REM =================================
REM =================================
SET DEFINES=/DDEBUG /Zi /Gm
SET LIBRARIES=
SET OBJ_LIST=

SET INCLUDE=^
%VC_DIR%include;^
%WIN_SDK_DIR%Include\%WIN_SDK_VER%\um;^
%WIN_SDK_DIR%Include\%WIN_SDK_VER%\shared;^
%WIN_SDK_DIR%Include\%WIN_SDK_VER%\winrt;^
include;^
source;^
%~dp0meta\win\include;

SET LIB=^
%VC_DIR%lib;^
%WIN_SDK_DIR%Lib\%WIN_SDK_VER%\ucrt\x86;^
%WIN_SDK_DIR%Lib\%WIN_SDK_VER%\um\x86;^
%~dp0meta\win\lib;

SET PATH=^
%SystemRoot%\system32;^
%VC_DIR%bin;^
%WIN_SDK_DIR%Bin\%WIN_SDK_VER%\x86;^
%~dp0meta\win\bin;

SET SRC_FOLDER=%~dp0source\
SET TARGET_SUBSYSTEM=/SUBSYSTEM:WINDOWS

IF %USING_CONSOLE_WINDOW%==1 (
   SET TARGET_SUBSYSTEM=/SUBSYSTEM:CONSOLE
)
IF %USING_COMPILER_OPTS%==1 (
   SET DEFINES=!DEFINES! /DUSING_COMPILER_OPTS /O2
)
IF %USING_LIBAV%==1 (
   SET DEFINES=!DEFINES! /DUSING_LIBAV
   SET LIBRARIES=!LIBRARIES! avcodec.lib avformat.lib avutil.lib swscale.lib swresample.lib
)
IF %USING_CURL%==1 (
   SET DEFINES=!DEFINES! /DUSING_CURL /DCURL_STATICLIB
   SET LIBRARIES=!LIBRARIES! libcurl.lib
)
IF %USING_LIBPNG%==1 (
   SET DEFINES=!DEFINES! /DUSING_LIBPNG "/DUSING_LIBPNG_HEADER=<png.h>"
   SET LIBRARIES=!LIBRARIES! libpng16.lib
)
IF %USING_LIBJPEG%==1 (
   SET DEFINES=!DEFINES! /DUSING_LIBJPEG
   SET LIBRARIES=!LIBRARIES! libjpeg.lib
)
IF %USING_LIBOGG%==1 (
   SET DEFINES=!DEFINES! /DUSING_LIBOGG
   SET LIBRARIES=!LIBRARIES! libogg_static.lib libvorbis_static.lib libvorbisfile_static.lib
)
IF %USING_FREETYPE%==1 (
   SET DEFINES=!DEFINES! /DUSING_FREETYPE
   SET LIBRARIES=!LIBRARIES! freetype.lib
)

REM Create necessary folders
IF NOT EXIST %OBJ_FOLDER% MKDIR %OBJ_FOLDER%
IF NOT EXIST %TARGET_FOLDER% MKDIR %TARGET_FOLDER%

ECHO Compiling: %TARGET_NAME%...

CD "tools"
"makeheaders.exe" ../source
CD ..

REM Make Icon and Executable Info if needed
IF EXIST %META_WIN_FOLDER%icon.ico (
   DEL /F %META_WIN_FOLDER%icon.res
   RC /nologo %META_WIN_FOLDER%icon.rc
   SET OBJ_LIST=%META_WIN_FOLDER%icon.res !OBJ_LIST!
)

FOR /f %%j IN ('dir /s /b %SRC_FOLDER%*.cpp') DO (
   SET SRC_FILE=%%j
   SET OBJ_FILE=!SRC_FILE:%SRC_FOLDER%=%OBJ_FOLDER%!
   SET OBJ_FILE=!OBJ_FILE:.cpp=.obj!
   SET OBJ_FILE2=!SRC_FILE:%SRC_FOLDER%=!
   SET OBJ_FILE2=!OBJ_FILE2:.cpp=.obj!
   SET OBJ_LIST="!OBJ_FILE2!" !OBJ_LIST!
   FOR %%k IN (!OBJ_FILE!) DO (
      IF NOT EXIST %%~dpk MKDIR %%~dpk
   )
   CL "!SRC_FILE!" /nologo /c /DWIN32 /DTARGET_NAME=\"!TARGET_NAME!\" /DGLEW_STATIC %DEFINES% /EHsc /FS /Gd /MD /Fo!OBJ_FILE! /Fd%OBJ_FOLDER%%TARGET_NAME%.pdb
   IF NOT !errorlevel! == 0 (
      GOTO END_OF_BAT
   )
)

ECHO   Linking: %TARGET_NAME%...

CD %OBJ_FOLDER%
LINK ^
   /OUT:"!TARGET_FOLDER!!TARGET_NAME!.exe" ^
   !TARGET_SUBSYSTEM! ^
   /MACHINE:X86 ^
   !OBJ_LIST! ^
   /DEBUG ^
   /NOLOGO ^
   /PDB:%TARGET_FOLDER%%TARGET_NAME%.pdb ^
   Ws2_32.lib ^
   Wldap32.lib ^
   Advapi32.lib ^
   Crypt32.lib ^
   Normaliz.lib ^
   zlibstat.lib ^
   glew32s.lib ^
   opengl32.lib ^
   !LIBRARIES! ^
   SDL2.lib ^
   SDL2main.lib ^
   kernel32.lib ^
   user32.lib

ECHO Finishing: %TARGET_NAME%.exe
IF NOT !TARGET_FOLDER! == !BUILD_OUTPUT_FOLDER! (
   COPY %TARGET_FOLDER%%TARGET_NAME%.exe %BUILD_OUTPUT_FOLDER%%TARGET_NAME%.exe
)
:END_OF_BAT
PAUSE
