@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\VC98\BIN\VCVARS32.BAT" >nul 2>&1
pushd "%TEMP%\yzcrack"
cl /nologo /O2 /MT yzcrack.c
echo CL_EXIT=%ERRORLEVEL%
popd