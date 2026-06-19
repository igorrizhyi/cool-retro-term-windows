# CI/CD & Build Setup Briefing
## Project: cool-retro-term-windows fork — menu bar removal + GitHub Actions

---

## 1. Project Overview

**Repository:** `https://github.com/pushingpandas/cool-retro-term-windows`  
**What it is:** A Windows 11 port of the retro CRT terminal emulator cool-retro-term, built with Qt6/QML and C++. It uses a custom Windows ConPTY backend replacing the POSIX PTY layer.  
**Goal:** Two things need to happen:
1. Remove the always-visible menu bar (one-line QML edit)
2. Set up GitHub Actions so every push automatically builds the app and produces a downloadable `.zip` release artifact

---

## 2. Repository Structure (relevant parts)

```
cool-retro-term-windows/
├── .github/                        ← workflows go here
├── app/
│   └── qml/
│       └── main.qml                ← THE FILE TO EDIT (menu bar change)
├── qmltermwidget/                  ← git submodule (terminal widget)
├── KDSingleApplication/            ← git submodule (single-instance helper)
├── build_win.bat                   ← existing build script (has hardcoded local paths)
├── compile_shaders.bat             ← shader compilation script
├── cool-retro-term.pro             ← qmake project file
└── deploy/                         ← deploy helpers
```

**Important:** The repo has **two git submodules** (`qmltermwidget` and `KDSingleApplication`). The clone command must use `--recurse-submodules`, otherwise the build will fail with missing files.

---

## 3. The Code Change (menu bar removal)

**File:** `app/qml/main.qml`  
**Find this line:**
```qml
menuBar: qtquickMenuLoader.item
```
**Replace with:**
```qml
menuBar: null
```

That's the entire code change. The `Loader { id: qtquickMenuLoader ... }` block below it should be left intact — only the assignment to the window's `menuBar` property is changed. This detaches the menu from the window without deleting it.

**Note:** All settings (colors, fonts, CRT effects, profiles) remain accessible via the **right-click context menu**, so nothing is lost functionally.

---

## 4. Tech Stack & Build Requirements

| Component | Detail |
|-----------|--------|
| Language | C++ (25.7%) + QML (72.6%) |
| Build system | qmake |
| Framework | Qt 6.8+ (MSVC 2022 64-bit build required) |
| Compiler | MSVC — Visual Studio 2022, "Desktop development with C++" workload |
| Shaders | 63 shader variants compiled with `qsb.exe` (Qt Shader Tools) |
| Qt modules needed | Qt 6.8+ MSVC 2022 64-bit, Qt5Compat (graphical effects), Qt Shader Tools |
| Platform | Windows 10 1809+ or Windows 11 |

---

## 5. GitHub Actions Workflow File

**Create this file at:** `.github/workflows/build.yml`

```yaml
name: Build Windows

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  workflow_dispatch:   # allows manual trigger from GitHub UI

jobs:
  build:
    runs-on: windows-2022

    steps:
      - name: Checkout repository with submodules
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Qt
        uses: jurplel/install-qt-action@v4
        with:
          version: '6.8.3'
          host: 'windows'
          target: 'desktop'
          arch: 'win64_msvc2022_64'
          modules: 'qt5compat qtshadertools'
          cache: true

      - name: Set up MSVC environment
        uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: x64

      - name: Build
        run: |
          qmake cool-retro-term.pro -spec win32-msvc CONFIG+=release
          nmake

      - name: Compile shaders
        run: |
          # qsb.exe is on PATH after jurplel/install-qt-action
          compile_shaders.bat
        shell: cmd

      - name: Deploy with windeployqt
        run: |
          mkdir deploy_out
          copy app\release\cool-retro-term.exe deploy_out\
          cd deploy_out
          windeployqt cool-retro-term.exe --qmldir ..\app\qml --release

          rem Copy QMLTermWidget plugin
          mkdir qt6\qml\QMLTermWidget
          copy ..\qmltermwidget\release\qmltermwidget.dll qt6\qml\QMLTermWidget\
          copy ..\qmltermwidget\src\qmldir qt6\qml\QMLTermWidget\
          copy ..\qmltermwidget\src\QMLTermScrollbar.qml qt6\qml\QMLTermWidget\
        shell: cmd

      - name: Package as zip
        run: |
          Compress-Archive -Path deploy_out\* -DestinationPath cool-retro-term-windows.zip

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: cool-retro-term-windows
          path: cool-retro-term-windows.zip
          retention-days: 30
```

**Notes on the workflow:**
- `jurplel/install-qt-action@v4` handles Qt download and PATH setup automatically — no manual Qt install needed on the runner
- `ilammy/msvc-dev-cmd@v1` sets up the Visual Studio compiler environment (equivalent to opening the "x64 Native Tools Command Prompt")
- `cache: true` on the Qt install step caches the Qt download between runs, making subsequent builds much faster
- The existing `build_win.bat` in the repo is **not used** here because it has hardcoded local paths (`D:\Qt\6.8.3\...`, `D:\Coding\...`) that won't exist on GitHub's runners. The workflow runs the same commands directly with correct paths.

---

## 6. Potential Issues to Watch For

**Shader compilation script path:**  
`compile_shaders.bat` may have a hardcoded path to `qsb.exe` (similar to `build_win.bat`). Check the file and replace any hardcoded Qt path with just `qsb` — after `jurplel/install-qt-action` runs, `qsb.exe` will already be on the system PATH.

**Qt version mismatch with existing release:**  
The published release was built with Qt 6.8.3 (MSVC). The workflow above uses 6.8.3 to match. If you want to use a newer Qt version (e.g. 6.11.1), change the `version:` field in the workflow — the app should build fine with newer Qt, but the DLL set will differ from the published release.

**windeployqt DLL layout:**  
The repo's README documents a specific DLL layout where Qt DLLs are moved into a `qt6/` subfolder. The deploy step above is a starting point — if the app fails to launch after packaging, compare the DLL folder structure against the published release zip and adjust accordingly.

**submodules:**  
If the build fails with errors about missing files in `qmltermwidget/` or `KDSingleApplication/`, the submodules weren't cloned. The `submodules: recursive` in the checkout step handles this — do not remove it.

---

## 7. Cost

**Free.** GitHub Actions is free for public repositories with no minute limits. The fork must remain public to keep this free. Windows runners consume minutes at 2x rate vs Linux, but the 2x multiplier only applies to private repos — irrelevant here.

---

## 8. Step-by-Step Summary for the Agent

1. **Fork** `https://github.com/pushingpandas/cool-retro-term-windows` on GitHub (keep public)
2. **Clone** the fork locally: `git clone --recurse-submodules https://github.com/YOUR_USERNAME/cool-retro-term-windows.git`
3. **Edit** `app/qml/main.qml`: change `menuBar: qtquickMenuLoader.item` → `menuBar: null`
4. **Check** `compile_shaders.bat`: replace any hardcoded `qsb.exe` path with just `qsb`
5. **Create** `.github/workflows/build.yml` with the content from Section 5 above
6. **Commit and push** all changes to the fork's `main` branch
7. **Go to** the fork's **Actions** tab on GitHub — the build will start automatically
8. When the build succeeds, the `.zip` artifact appears under the workflow run's **Artifacts** section — download and test it

---

## 9. References

- Original repo: https://github.com/pushingpandas/cool-retro-term-windows
- Qt install action: https://github.com/jurplel/install-qt-action
- MSVC dev cmd action: https://github.com/ilammy/msvc-dev-cmd
- GitHub Actions free tier: https://docs.github.com/en/billing/managing-billing-for-your-products/managing-billing-for-github-actions/about-billing-for-github-actions
