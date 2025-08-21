@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

:: --- Unpack Arguments ----------------------------------------
for %%a in (%*) do set "%%a=1"
if not "%release%"=="1" set debug=1
if "%debug%"=="1" set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]

:: --- Define Compile/Link Lines -------------------------------
set cl_common=  /I..\src\ /nologo /FC /Z7 /W4 /wd4200 /wd4201
set cl_debug=   call cl /Od /Ob1 /DBUILD_DEBUG=1 %cl_common%
set cl_release= call cl /O2 /DBUILD_DEBUG=1 %cl_common%
set cl_dll=     /LD
set cl_link=    /link /INCREMENTAL:NO /OPT:REF
set cl_out=     /out:

:: --- Choose Compile Lines ------------------------------------
set compile_debug=%cl_debug%
set compile_release=%cl_release%
set compile_dll=%cl_dll%
set compile_link=%cl_link%
set compile_out=%cl_out%

if "%debug%"=="1"   set compile=%compile_debug%
if "%release%"=="1" set compile=%compile_release%

:: --- Prep Directories ----------------------------------------
if not exist build mkdir build

:: --- Build Everything ----------------------------------------
pushd build
%compile% ..\src\genesis.c %compile_link% %compile_out%genesis.exe || exit /b 1
popd
