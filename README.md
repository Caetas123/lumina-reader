# Lumina Reader

<p align="center">
  <img src="assets/lumina-logo.png" alt="Lumina Reader logo" width="128">
</p>

[![Build](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml/badge.svg)](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml)

Lumina Reader is a native, local-first reading library for Windows, macOS, and Linux. It helps you import documents, keep your reading progress, save passages and notes, and find that knowledge again across your library. It is built with C++20, Qt 6, Qt Quick/QML, SQLite, and CMake.

The project is privacy-focused: documents, reading data, and search indexes stay on the device by default. The repository is actively developed; the public release links below point to the latest tagged build, while the current branch may contain newer features that have not been published yet.

## Download

Ready-to-run builds are published on [GitHub Releases](https://github.com/Caetas123/lumina-reader/releases). The links below are direct executable downloads; Lumina Reader does not require a portable ZIP for end users.

| Platform | Download | Notes |
| --- | --- | --- |
| Windows x64 | [Download installer EXE (`v0.1.6`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.1.6/LuminaReader-Setup-windows-x64.exe) | Recommended. Run the `.exe` to install the application and its Qt dependencies. |
| macOS x86_64 | Not included in the currently published `v0.1.6` release | The next tagged release will include a direct `LuminaReader-macos-x86_64.dmg` download. |
| Linux x86_64 | [Download AppImage (`v0.1.6`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.1.6/LuminaReader-linux-x86_64.AppImage) | Make it executable with `chmod +x`, then run it. No installation or Qt setup is required. |

The Windows installer is a self-contained `.exe` that installs the Qt runtime and platform plugins before launching Lumina Reader.
The Linux AppImage is a self-contained executable for 64-bit Linux systems and includes the Qt runtime required by Lumina Reader.

See the [v0.1.6 release notes](https://github.com/Caetas123/lumina-reader/releases/tag/v0.1.6) for the currently published Windows and Linux packages and their SHA-256 checksums. New builds are added to Releases when a version tag is published; this branch does not push or publish automatically.

The macOS release workflow builds a self-contained DMG for Intel Macs. It is ready for the next tagged release but is not attached to the older `v0.1.6` release yet.

## Principles

- Local-first: documents and reading data stay on the device by default.
- Private: no telemetry and no automatic document uploads.
- Native: no browser, Electron, Tauri, WebView, React, or HTML/CSS UI.
- Modular: UI, application, domain, infrastructure, PDF, and platform concerns remain separated.
- Safe: original PDFs are not modified implicitly, and errors are reported without exposing technical details in the user interface.
- Open source: third-party licenses remain separate from the project license.

## What it does today

The current application provides:

- a native Qt Quick desktop interface with light, dark, sepia, paper, gray, low-light, and high-contrast themes;
- individual and multiple-file import, drag and drop, folder import, SHA-256 duplicate detection, and optional folder watching through the library model;
- library filters for favorites, to-read, reading, and finished documents;
- reading progress, last position, status, favorites, ratings, descriptions, language, genre, collections, tags, and reading history;
- generated or personalized covers, page rotation for visual documents, and backup/restoration of that reading metadata;
- TXT, RTF, FB2, unencrypted PalmDOC-compatible MOBI/AZW/AZW3, raster-image, EPUB, CBZ, and ZIP reading when the matching Qt capabilities are present;
- optional PDF rendering through Qt PDF in the CI/release build;
- optional in-process CBR, CB7, CBT, RAR, and 7Z reading when libarchive is available at configure time;
- optional in-process DJVU/DJV page rendering and hidden-text extraction when DjVuLibre is available at configure time;
- optional asynchronous page OCR through Tesseract when it is installed on the system;
- in-document search for text pages and EPUB chapters;
- a Contents panel for multi-page documents, with EPUB chapter labels when the source provides them;
- scoped, local search history for the library, the open document, and the Knowledge center, with deduplication, retention, reopening, and clearing;
- persistent highlights, annotations, bookmarks, and a global FTS5 Knowledge center;
- Knowledge center undo/redo for highlight and annotation creation, editing, and removal, with Ctrl+Z/Ctrl+Y shortcuts;
- contextual copy, Google, integrated dictionary, translation, and highlight actions for selected text;
- optional explicit online dictionary and translation results shown inside the app, with local lookup history, reopen/clear actions, lookup favorites, and copy actions;
- optional system text-to-speech through Qt Speech;
- Markdown, JSON, and plain-text knowledge export, library metadata export, database backup, and metadata restoration;
- SQLite migrations, safe read-only archive handling, Zip Slip checks, archive entry limits, and Qt Test coverage;
- GitHub Actions validation for Linux, macOS, and Windows.

The exact capability set depends on the Qt modules present in the build. See [the format and feature matrix](docs/FORMAT_SUPPORT.md) before relying on a format such as MOBI, RAR, 7Z, DJVU, or OCR. An extension in a file picker is never treated as proof that a reader exists.

## Build prerequisites

- C++20 compiler;
- CMake 3.24 or newer;
- Qt 6.5 or newer with `Core`, `Gui`, `Network`, `Qml`, `Quick`, `QuickControls2`, `Sql`, and `Test`;
- optional `Pdf`, `TextToSpeech`, and `CorePrivate` modules enable PDF rendering, system speech, and EPUB/CBZ/ZIP reading respectively;
- optional libarchive headers and library enable CBR/CB7/CBT/RAR/7Z reading;
- optional DjVuLibre headers and library enable DJVU/DJV reading;
- Tesseract OCR is optional at runtime; when available on `PATH`, image-based pages can be recognized without uploading documents;
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
  domain/reader/     Format-independent document engine contracts
  infrastructure/    SQLite, logging, filesystem, and external adapters
  infrastructure/reader/ Document engine adapters and format selection
  infrastructure/text/ Text and lightweight RTF/FB2 decoding
  pdf/               PDF-specific contracts kept behind the reader engine boundary
  ui/qml/            Qt Quick views and design tokens
tests/unit/           Qt Test suites
docs/                 Architecture, build, privacy, release, and format notes
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
