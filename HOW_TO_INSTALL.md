# How to Install

## Option 1: Download Pre-built Release (Recommended)

No installation required — cool-retro-term is fully portable.

1. Go to the [Releases](https://github.com/pushingpandas/cool-retro-term-windows/releases) page
2. Download the latest `cool-retro-term-windows-*.zip`
3. Extract the zip to any folder (e.g., `C:\Programs\cool-retro-term\`)
4. Run `cool-retro-term.exe`

That's it. All Qt6 DLLs and plugins are included in the zip.

### System Requirements
- Windows 10 version 1809 or later (for ConPTY API)
- Windows 11 (recommended)
- No additional software or runtime needed

---

## Option 2: Build from Source

### Prerequisites

1. **Visual Studio 2022** (Community or Build Tools)
   - Download: https://visualstudio.microsoft.com/
   - During installation, select the **"Desktop development with C++"** workload

2. **Qt 6.8+** for MSVC 2022 64-bit
   - Download: https://www.qt.io/download
   - Install the **MSVC 2022 64-bit** kit
   - Also install the **Qt5Compat** and **Qt ShaderTools** modules

3. **Git** (to clone the repository)
   - Download: https://git-scm.com/download/win

### Clone the Repository

```batch
git clone https://github.com/pushingpandas/cool-retro-term-windows.git
cd cool-retro-term-windows
```

### Build

Open a **x64 Native Tools Command Prompt for VS 2022**, then run:

```batch
rem Set up the MSVC compiler environment (skip if using VS command prompt)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

rem Add Qt to PATH (adjust path to match your Qt installation)
set PATH=D:\Qt\6.8.3\msvc2022_64\bin;%PATH%

rem Generate makefiles and build
qmake cool-retro-term.pro -spec win32-msvc
nmake
```

The executable will be at: `cool-retro-term.exe` (project root) or `app\release\cool-retro-term.exe`

### Deploy

To create a standalone distributable folder:

```batch
mkdir deploy
copy cool-retro-term.exe deploy\
cd deploy

rem Deploy Qt runtime DLLs
windeployqt cool-retro-term.exe --qmldir ..\app\qml

rem Copy the QMLTermWidget plugin
mkdir qml\QMLTermWidget
copy ..\qmltermwidget\release\qmltermwidget.dll qml\QMLTermWidget\
copy ..\qmltermwidget\src\qmldir qml\QMLTermWidget\
copy ..\qmltermwidget\src\QMLTermScrollbar.qml qml\QMLTermWidget\

rem Copy color schemes and keyboard layouts
xcopy /E ..\qmltermwidget\lib\color-schemes qml\QMLTermWidget\color-schemes\
xcopy /E ..\qmltermwidget\lib\kb-layouts qml\QMLTermWidget\kb-layouts\

rem Copy extra Qt modules (from your Qt installation bin/ directory)
copy D:\Qt\6.8.3\msvc2022_64\bin\Qt6Core5Compat.dll .
copy D:\Qt\6.8.3\msvc2022_64\bin\Qt6ShaderTools.dll .
```

### Compile Shaders (Optional)

Only needed if you modify the CRT effect shaders:

```batch
rem Edit compile_shaders.bat to set your Qt qsb.exe path, then:
compile_shaders.bat
```

---

## Troubleshooting

### "Qt6Core.dll not found" or similar DLL errors
All Qt6 DLLs must be in the same folder as `cool-retro-term.exe`. If you built from source, run `windeployqt` as described above.

### Black screen / no CRT effects
The app requires OpenGL. Make sure your GPU drivers are up to date. The app forces OpenGL rendering to avoid D3D11 shader compatibility issues.

### Terminal doesn't accept input
Make sure you're running the freshly built exe, not a stale copy. Rebuild with `nmake` and copy the new exe to your deploy folder.

### PowerShell doesn't start
cool-retro-term looks for shells in this order: `pwsh` (PowerShell Core) → `powershell` (Windows PowerShell) → `cmd.exe`. Make sure at least one is available on your system PATH.

---

## Uninstall

cool-retro-term is portable — just delete the folder. No registry entries or system files are modified.

Settings are stored in `%APPDATA%\cool-retro-term\` — delete that folder to remove all saved preferences.
