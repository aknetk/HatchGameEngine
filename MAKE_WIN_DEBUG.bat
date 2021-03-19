@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET DEFINES=
SET LIBRARIES=

SET INCLUDE=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\um;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\shared;^
C:\Program Files (x86)\Windows Kits\10\include\10.0.16299.0\winrt;^
include;^
source;^
%~dp0meta\win\include;

SET LIB=^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib;^
C:\Program Files (x86)\Windows Kits\10\lib\10.0.16299.0\ucrt\x86;^
C:\Program Files (x86)\Windows Kits\10\lib\10.0.16299.0\um\x86;^
%~dp0meta\win\lib;

SET PATH=^
%SystemRoot%\system32;^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin;^
C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE;^
C:\Program Files (x86)\Windows Kits\10\bin\10.0.16299.0\x86;^
%~dp0meta\win\bin;

SET SRC_FOLDER=%~dp0source\

SET TARGET_NAME=HatchGameEngine
SET TARGET_FOLDER=%temp%\HatchGameEngine\builds\win\
SET TARGET_SUBSYSTEM=/SUBSYSTEM:WINDOWS
SET OBJ_FOLDER=%temp%\HatchGameEngine\out\win\
SET OBJ_LIST=
SET ICO_FOLDER=%~dp0meta\win\
SET ICO_FOLDER=%~dp0source_galactic\Meta\win\

SET USING_CONSOLE_WINDOW=1
SET USING_COMPILER_OPTS=0
SET USING_LIBAV=0
SET USING_CURL=0
SET USING_LIBPNG=1
SET USING_LIBJPEG=1
SET USING_LIBOGG=1
SET USING_FREETYPE=1

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
   SET DEFINES=!DEFINES! /DUSING_CURL
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
IF EXIST %ICO_FOLDER%icon.ico (
   DEL /F %ICO_FOLDER%icon.res
   RC /nologo %ICO_FOLDER%icon.rc
   SET OBJ_LIST=%ICO_FOLDER%icon.res !OBJ_LIST!
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
   CL "!SRC_FILE!" /nologo /c /DWIN32 /DGLEW_STATIC /DCURL_STATICLIB /DTARGET_NAME=\"!TARGET_NAME!\" /DDEBUG %DEFINES% /EHsc /FS /Gm /Gd /MD /Zi /Fo!OBJ_FILE! /Fd%OBJ_FOLDER%%TARGET_NAME%.pdb
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
copy %TARGET_FOLDER%%TARGET_NAME%.exe %~dp0builds\win\%TARGET_NAME%.exe
copy %TARGET_FOLDER%%TARGET_NAME%.exe C:\Users\Justin\GitRepos\SonicLegends\%TARGET_NAME%.exe
:END_OF_BAT
PAUSE
