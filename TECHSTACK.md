# Tech Stack

## Languages
- **C++17** — Core terminal emulation engine, ConPTY backend, process management
- **QML / JavaScript** — UI layer, CRT effects, settings, menus
- **GLSL / HLSL 5.1** — CRT shader effects (scanlines, bloom, screen curvature, phosphor glow)
- **Batch** — Build and deployment scripts (Windows)

## Frameworks & Libraries
- **Qt 6.8+** — Application framework (MSVC 2022 64-bit build)
  - **Qt Quick / QML** — Declarative UI
  - **Qt Quick Controls 2** — Buttons, menus, dialogs
  - **Qt OpenGL** — GPU-accelerated CRT rendering
  - **Qt5Compat.GraphicalEffects** — Blur, glow, and image effects
  - **Qt ShaderTools** — Runtime shader compilation (qsb)
  - **Qt Network** — Network support
- **QMLTermWidget** — QML port of Konsole's terminal widget (bundled, not a submodule)
- **KDSingleApplication** — Single-instance application support

## Windows Terminal Backend
- **Windows ConPTY API** — Pseudo Console (CreatePseudoConsole, ResizePseudoConsole)
- **Win32 Pipes** — Anonymous pipes for terminal I/O (ReadFile / WriteFile)
- **Threaded I/O** — ReaderThread / WriterThread (Windows pipes don't support QSocketNotifier)
- **VirtualAlloc** — mmap emulation for terminal history scrollback (win32_compat.h)

## Build System
- **qmake** — Project build configuration (.pro / .pri files)
- **MSVC 2022** — Microsoft Visual C++ compiler (nmake)
- **windeployqt** — Qt deployment tool for packaging runtime DLLs

## Rendering
- **OpenGL** — Forced as graphics backend (D3D11's FXC shader compiler can't handle CRT shaders)
- **Qt Quick Scene Graph** — Hardware-accelerated rendering pipeline
- **63 shader variants** — Pre-compiled with qsb (GLSL 100es + HLSL 5.1 + SPIR-V + MSL 1.2)

## Architecture

```
┌──────────────────────────────────────────┐
│              QML UI Layer                │
│  (menus, settings, CRT effects, tabs)    │
├──────────────────────────────────────────┤
│           QMLTermWidget                  │
│  (terminal display, keyboard, colors)    │
├──────────────────────────────────────────┤
│      KSession / Session / Pty            │
│  (session management, shell lifecycle)   │
├──────────────────────────────────────────┤
│          ConPtyProcess                   │
│  (CreateProcessW + STARTUPINFOEX)        │
├──────────────────────────────────────────┤
│          ConPtyDevice                    │
│  (QIODevice + ReaderThread/WriterThread) │
├──────────────────────────────────────────┤
│            ConPty                        │
│  (CreatePseudoConsole / Win32 pipes)     │
└──────────────────────────────────────────┘
```

## Shell Detection (Windows)
Priority order: PowerShell Core (`pwsh`) → Windows PowerShell (`powershell`) → `cmd.exe`

## Key Project Files
| Area | Files |
|------|-------|
| ConPTY backend | `qmltermwidget/lib/conpty.*`, `conptydevice.*`, `conptyprocess.*` |
| Win32 compat | `qmltermwidget/lib/win32_compat.h`, `ringbuffer.h` |
| Terminal emulation | `qmltermwidget/lib/Pty.*`, `Session.*`, `Emulation.*`, `Vt102Emulation.*` |
| CRT effects | `app/qml/PreprocessedTerminal.qml`, `BurnInEffect.qml` |
| Shaders | `app/qml/shaders/` (63 variants) |
| Settings | `app/qml/ApplicationSettings.qml`, `Settings*Tab.qml` |
| Build | `cool-retro-term.pro`, `qmltermwidget.pro`, `lib.pri`, `app.pro` |
