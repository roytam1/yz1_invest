@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" >nul
cd /d "%~dp0"
cl /nologo /O2 /W2 %1.c /Fe%1.exe
