@rem Launcher for cool-retro-term Windows port
@rem Copyright (C) 2024 pushingpandas
@echo off
set "SCRIPT_DIR=%~dp0"
set "PATH=%SCRIPT_DIR%qt6;%PATH%"
start "" "%SCRIPT_DIR%cool-retro-term.exe" %*
