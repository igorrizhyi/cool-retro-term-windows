# cool-retro-term (Windows Port)

|> Default Amber|C:\ IBM DOS|$ Default Green|
|---|---|---|
|![Default Amber Cool Retro Term](https://user-images.githubusercontent.com/121322/32070717-16708784-ba42-11e7-8572-a8fcc10d7f7d.gif)|![IBM DOS](https://user-images.githubusercontent.com/121322/32070716-16567e5c-ba42-11e7-9e64-ba96dfe9b64d.gif)|![Default Green Cool Retro Term](https://user-images.githubusercontent.com/121322/32070715-163a1c94-ba42-11e7-80bb-41fbf10fc634.gif)|

## Description
cool-retro-term is a terminal emulator which mimics the look and feel of the old cathode tube screens.
It has been designed to be eye-candy, customizable, and reasonably lightweight.

This is the **Windows 11 port** by [pushingpandas](https://github.com/pushingpandas), based on the original [cool-retro-term](https://github.com/Swordfish90/cool-retro-term) by Filippo Scognamiglio.

It uses the QML port of qtermwidget (Konsole) with a custom Windows ConPTY backend replacing the POSIX PTY layer.

Settings such as colors, fonts, and effects can be accessed via the menu bar or context menu (right-click).

## Download

Grab the latest build from the [Releases](https://github.com/pushingpandas/cool-retro-term-windows/releases) page. Extract the zip and run `cool-retro-term.bat`.

> After building from source, the executable is at `app\release\cool-retro-term.exe` (created by `nmake`). The `app\release\` folder is only generated after a successful build. See [Building on Windows](#building-on-windows) below for full build and deployment steps.

## Screenshots
![Image](<https://i.imgur.com/TNumkDn.png>)
![Image](<https://i.imgur.com/hfjWOM4.png>)
![Image](<https://i.imgur.com/GYRDPzJ.jpg>)

## Windows Port Details

### What was changed
- **ConPTY backend**: New `conpty.cpp`, `conptydevice.cpp`, `conptyprocess.cpp` replace POSIX PTY for terminal I/O on Windows using the Windows Pseudo Console API (ConPTY)
- **Threaded pipe I/O**: Reader/Writer threads with `ReadFile`/`WriteFile` (Windows anonymous pipes don't work with `QSocketNotifier`)
- **mmap emulation**: `win32_compat.h` provides `mmap`/`munmap`/`getpagesize` via `VirtualAlloc` for terminal history
- **OpenGL rendering**: Forces OpenGL backend since D3D11's FXC shader compiler can't handle the CRT effect shaders
- **Shell detection**: Finds PowerShell Core (`pwsh`) > Windows PowerShell (`powershell`) > `cmd.exe`
- **Consolas fallback font**: Uses Consolas as the monospace fallback on Windows
- **Menu bar**: Always visible on Windows (File, Edit, View, Profiles, Help)
- **Shader compilation**: All 63 shader variants recompiled with HLSL 5.1 support

### Requirements
- Windows 10 version 1809+ (for ConPTY API) or Windows 11
- Qt 6.8+ (MSVC 2022 build)
- Visual Studio 2022 (Build Tools or Community)

## Building on Windows

### Prerequisites
1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community edition or Build Tools) with "Desktop development with C++" workload
2. Install [Qt 6.8+](https://www.qt.io/download) for MSVC 2022 64-bit
3. Make sure `qmake` is in your PATH (e.g., `D:\Qt\6.8.3\msvc2022_64\bin`)

### Build
```batch
rem Open a "x64 Native Tools Command Prompt for VS 2022" or run:
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=D:\Qt\6.8.3\msvc2022_64\bin;%PATH%

cd cool-retro-term
qmake cool-retro-term.pro -spec win32-msvc
nmake
```

Or use the provided `build_win.bat` (edit paths to match your Qt installation first).

### Compile Shaders
```batch
rem Edit compile_shaders.bat to set your Qt qsb.exe path, then:
compile_shaders.bat
```

### Deploy
```batch
mkdir deploy
copy app\release\cool-retro-term.exe deploy\
cd deploy
windeployqt cool-retro-term.exe

rem Copy QMLTermWidget plugin
mkdir qt6\qml\QMLTermWidget
copy ..\qmltermwidget\release\qmltermwidget.dll qt6\qml\QMLTermWidget\
copy ..\qmltermwidget\src\qmldir qt6\qml\QMLTermWidget\
copy ..\qmltermwidget\src\QMLTermScrollbar.qml qt6\qml\QMLTermWidget\

rem Move Qt6 DLLs to qt6/ subfolder for a clean layout
mkdir qt6
move Qt6*.dll qt6\
move D3Dcompiler_47.dll qt6\
move opengl32sw.dll qt6\
rem Also move: generic, iconengines, imageformats, networkinformation,
rem platforms, qml, qmltooling, sqldrivers, styles, tls, translations into qt6/

rem Install Qt5Compat.GraphicalEffects (download from Qt if not present)
rem Install Qt6ShaderTools.dll into qt6/
```

### Run
Launch via `cool-retro-term.bat` (sets DLL paths) or run `cool-retro-term.exe` directly if Qt6 DLLs are in PATH.

## Original Project

Based on [cool-retro-term](https://github.com/Swordfish90/cool-retro-term) by Filippo Scognamiglio.
For Linux and macOS builds, see the [original repository](https://github.com/Swordfish90/cool-retro-term).

## License
GPL v3 - see [LICENSE](LICENSE) file.

Windows port Copyright (C) 2024 pushingpandas.
Original Copyright (C) 2013-2021 Filippo Scognamiglio.
