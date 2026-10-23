@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" >nul
pushd "%~dp0"

cl /nologo /O2 /W2 /MT /GX /D_USE_32BIT_TIME_T /DYZ1_STATIC /c yz1codec.cpp yz32lib.cpp YZ_MsgTbl.cpp
if errorlevel 1 goto :fail

lib /nologo /out:yz32lib.lib yz1codec.obj yz32lib.obj YZ_MsgTbl.obj
if errorlevel 1 goto :fail

if not exist test mkdir test
cl /nologo /O2 /W2 /MT /GX /D_USE_32BIT_TIME_T /DYZ1_STATIC /I. test\sdktest.cpp /Fetest\sdktest.exe yz32lib.lib
if errorlevel 1 goto :fail

popd
echo build ok
goto :eof

:fail
popd
echo build failed
exit /b 1
