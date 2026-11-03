# Lumina Reader

<p align="center">
  <img src="assets/lumina-logo.png" alt="Lumina Reader logo" width="128">
</p>

[![Build](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml/badge.svg)](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml)

Lumina Reader is a native, local-first PDF reader foundation for Windows, macOS, and Linux. It is built with C++20, Qt 6, Qt Quick/QML, SQLite, and CMake.

The project is intentionally incremental and privacy-focused: documents and reading data are designed to stay on the device by default. The repository is currently in **Phase 0 (early development)**. The public build validates the desktop foundation; PDF rendering, library import, annotations, OCR, and dictionary providers are planned for later phases.

## Download

Ready-to-run builds are published on [GitHub Releases](https://github.com/Caetas123/lumina-reader/releases).

| Platform | Download | Notes |
| --- | --- | --- |
| Windows x64 | [Download installer EXE (`v0.1.4`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.1.4/LuminaReader-Setup-windows-x64.exe) | Recommended. Run it to install the application and its Qt dependencies. |
| macOS | Not packaged yet | [Build from source](docs/BUILDING.md). |
| Linux x86_64 | [Download AppImage (`v0.1.5`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.1.5/LuminaReader-linux-x86_64.AppImage) | Make it executable with `chmod +x`, then run it. No installation or Qt setup is required. |

The Windows installer is a self-contained `.exe` that installs the Qt runtime and platform plugins before launching Lumina Reader.
The Linux AppImage is a self-contained executable for 64-bit Linux systems and includes the Qt runtime required by Lumina Reader.

See the [Linux v0.1.5 release notes](https://github.com/Caetas123/lumina-reader/releases/tag/v0.1.5) for the AppImage and its SHA-256 checksum. Windows artifacts remain available in the [v0.1.4 release](https://github.com/Caetas123/lumina-reader/releases/tag/v0.1.4).

## Principles

- Local-first: documents and reading data stay on the device by default.
- Private: no telemetry and no automatic document uploads.
- Native: no browser, Electron, Tauri, WebView, React, or HTML/CSS UI.
- Modular: UI, application, domain, infrastructure, PDF, and platform concerns remain separated.
- Safe: original PDFs are not modified implicitly, and errors are reported without exposing technical details in the user interface.
- Open source: third-party licenses remain separate from the project license.

## What it does today

Phase 0 is in progress. The current implementation provides:

- CMake 3.24+ project configuration with C++20;
- Qt Quick/QML application bootstrap;
- base QML design tokens;
- local application data directory initialization;
- SQLite connection handling and versioned migrations;
- SQLite FTS5 search-table foundation;
- categorized Qt logging without file-based document logging;
- typed domain errors with separate user and diagnostic messages;
- initial Qt Test coverage;
- a replaceable `PdfEngine` interface;
- GitHub Actions build/test workflow for Windows, macOS, and Linux;
- open-source contribution and security documentation.

The PDF adapter is not implemented yet. Defining the boundary first keeps future Qt PDF/PDFium work isolated from the UI and domain layers.

## Build prerequisites

- C++20 compiler;
- CMake 3.24 or newer;
- Qt 6.5 or newer with `Core`, `Gui`, `Qml`, `Quick`, `QuickControls2`, `Sql`, and `Test`;
- Ninja, or another generator configured through a user preset.

See [docs/BUILDING.md](docs/BUILDING.md) for platform-specific notes.

## Configure, build, and test

From the repository root:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

If Qt is not on CMake's search path, provide its installation prefix in a user preset or on the command line, for example:

```sh
cmake --preset default -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit>
```

Do not commit `CMakeUserPresets.json`, local build directories, or user data.

## Repository layout

```text
src/
  application/       Application services and UI-facing orchestration
  domain/            Business types and rules
  infrastructure/    SQLite, logging, filesystem, and external adapters
  pdf/               PdfEngine abstraction and future PDF adapters
  ui/qml/            Qt Quick views and design tokens
tests/unit/           Qt Test suites
docs/                 Architecture, build, privacy, and contribution notes
.github/              CI and issue/pull-request templates
```

The QML layer communicates with C++ controllers and models. It does not access SQLite directly.

## Development workflow

Keep changes small and thematic. Run the relevant tests, `git diff --check`, and a build before committing when the local toolchain is available. Use English Conventional Commit messages, for example:

```text
feat(library): import PDF files
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full workflow.

## License

Project-owned code is released under the [MIT License](LICENSE). Dependencies keep their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
