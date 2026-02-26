@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=D:\Qt\6.8.3\msvc2022_64\bin;%PATH%
cd /d D:\Coding\ClaudeCode\RetroTerminal\cool-retro-term
echo Regenerating makefiles...
qmake.exe cool-retro-term.pro -spec win32-msvc
echo Building...
nmake 2>&1
echo NMAKE_EXIT=%ERRORLEVEL%
copy /Y app\release\cool-retro-term.exe deploy\cool-retro-term.exe
echo DONE
