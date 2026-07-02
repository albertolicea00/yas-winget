# CLAUDE.md — YAS Winget

## What
Native GUI wrapper for **Windows Package Manager** (`winget`). Part of YAS suite.
Status: **scaffolded & unit-tested** — vendored core + adapter + QML shell compile, 3/3 QtTest suites pass (verified cross-compiling on macOS). Pending: build + QA on the real target platform.

## Stack
- C++20 + Qt 6.7+ (Qt Quick / QML), CMake ≥ 3.24, MSVC toolchain
- Native windowing via Qt QPA plugin: **windows** (Win32). WinRT only for optional extras (toasts, MSIX APIs).
- CLI execution: `QProcess` wrapping `winget`. Never bundle it.
- Architecture: **vendored core copy** (identical across suite, NO shared library by design) + `winget` adapter. Master template: `../yas-core/` local folder (not published). Core fixes must be replicated across repos.

## Target platform
Windows 10 1809+ / Windows 11 (winget requirement). x64 + arm64.

## winget specifics
- Runs as user; some installers self-elevate (UAC prompt comes from installer, not winget). Don't run whole GUI elevated.
- Key commands: `winget search`, `winget show`, `winget list`, `winget install/uninstall/upgrade`, `winget upgrade --all`, `winget pin add/remove`, `winget source list`.
- Output parsing is painful: table text, localized, truncated columns. Prefer `--disable-interactivity --accept-source-agreements`; check `winget export`/COM API (Microsoft.Management.Deployment WinRT API) as structured alternative to text scraping.
- First run requires accepting source agreements — handle in onboarding.
- msstore vs winget sources: UI must distinguish.

## Design (see DESIGN.md)
- Dark theme. Base `#212826`, accent **Blue `#0078D4`**, highlight `#0078D41A`, text `#F8F8F2` / `#ACADAD`.
- App tag: **WINGET**. Fonts: Outfit/Inter (UI), Fira Code or JetBrains Mono (CLI output).

## Conventions
- Conventional Commits (no co-author attribution), feature branches, PRs per CONTRIBUTING.md. Never push to origin without explicit ask.
- Planned layout (mirrors yas-brew, the reference scaffold): `src/core/` (vendored), `src/wingetadapter.*`, `src/main.cpp`, `qml/core/` (vendored) + `qml/Main.qml`, `tests/`, `assets/fonts/`, `icons/` (exists), CMakeLists.txt + CMakePresets.json.
- Packaging: MSIX or Inno Setup installer + windeployqt; distributable via winget manifest (dogfooding).

## Key files
README.md · DESIGN.md · CONTRIBUTING.md · EULA.md · SECURITY.md · icons/
