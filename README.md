# Lumina Reader

<p align="center">
  <img src="assets/lumina-logo.png" alt="Lumina Reader logo" width="128">
</p>

[![Build](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml/badge.svg)](https://github.com/Caetas123/lumina-reader/actions/workflows/ci.yml)

Lumina Reader is a native, local-first reading library for Windows, macOS, and Linux. It helps you import documents, keep your reading progress, save passages and notes, and find that knowledge again across your library. It is built with C++20, Qt 6, Qt Quick/QML, SQLite, and CMake.

The project is privacy-focused: documents, reading data, OCR results, and search indexes stay on the device by default. The public release links below point to the synchronized `v0.2.5` release. Windows and Linux are the fully audited full-engine builds for this release; macOS remains a supported CI package with its platform-specific capability set.

## Download

Ready-to-run builds are published on [GitHub Releases](https://github.com/Caetas123/lumina-reader/releases). The links below are direct executable downloads; Lumina Reader does not require a portable ZIP for end users. Windows, Linux, and macOS packages for a release use the same Git tag, and CI rejects a release when its tag differs from `project-version.txt`.

| Platform | Download | Notes |
| --- | --- | --- |
| Windows x64 | [Download installer EXE (`v0.2.5`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.5/LuminaReader-Setup-windows-x64.exe) | Recommended. Run the `.exe` to install the application and its Qt dependencies. |
| macOS x86_64 | [Download DMG (`v0.2.5`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.5/LuminaReader-macos-x86_64.dmg) | Open the DMG and move Lumina Reader to Applications. The macOS workflow publishes the core Qt build; native macOS validation is tracked separately. |
| Linux x86_64 | [Download AppImage (`v0.2.5`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.5/LuminaReader-linux-x86_64.AppImage) | Make it executable with `chmod +x`, then run it. No installation or Qt setup is required. |
| Debian/Ubuntu x86_64 | [Download Debian package (`v0.2.5`)](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.5/LuminaReader-linux-x86_64.deb) | Install with your distribution's package manager. |

The Windows installer is a self-contained `.exe` that installs the Qt runtime, platform plugins, native document engines, and multilingual OCR runtime before launching Lumina Reader.
The Linux AppImage is a self-contained executable for 64-bit Linux systems and includes the Qt runtime, native document engines, speech runtime, and multilingual OCR runtime required by Lumina Reader.

See the [v0.2.5 release notes](https://github.com/Caetas123/lumina-reader/releases/tag/v0.2.5) for the packages and their SHA-256 checksums. New builds are added to Releases when a version tag is published.

## Principles

- Local-first: documents and reading data stay on the device by default.
- Private: no telemetry and no automatic document uploads.
- Native: no browser, Electron, Tauri, WebView, React, or HTML/CSS UI.
- Modular: UI, application, domain, infrastructure, PDF, and platform concerns remain separated.
- Safe: original PDFs are not modified implicitly, and errors are reported without exposing technical details in the user interface.
- Open source: third-party licenses remain separate from the project license.

## What it does today

The current application provides:

- a native Qt Quick desktop interface with light, dark, sepia, paper, gray, low-light, and high-contrast themes, reusable controls, keyboard focus, and responsive layouts;
- individual and multiple-file import, drag and drop, folder import, SHA-256 duplicate detection, and folder watching through the library model;
- library filters for favorites, to-read, reading, and finished documents;
- reading progress, last position, status, favorites, ratings, descriptions, language, genre, collections, tags, and reading history;
- generated or personalized covers, page rotation for visual documents, and backup/restoration of that reading metadata;
- TXT, RTF, HTML/HTM, FB2, unencrypted PalmDOC-compatible MOBI/AZW/AZW3, raster-image, EPUB, CBZ, and ZIP reading;
- PDF rendering through Qt PDF in the release build;
- single-page and two-page layouts for visual documents, with page-aware navigation;
- in-process CBR, CB7, CBT, RAR, and 7Z reading through the full release build's bundled libarchive engine;
- in-process DJVU/DJV page rendering and hidden-text extraction through the full release build's bundled DjVuLibre engine;
- asynchronous page and document OCR through the bundled multilingual Tesseract runtime, with local progress, cancellation, word coordinates, and image-region mapping;
- PDF export and system printing for the opened document, including saved text highlights and visual region annotations, without modifying the original file;
- in-document search for text pages and EPUB chapters;
- a Contents panel for multi-page documents, with EPUB chapter labels when the source provides them;
- scoped, local search history for the library, the open document, and the Knowledge center, with deduplication, retention, reopening, and clearing;
- persistent highlights, annotations, bookmarks, and a global FTS5 Knowledge center;
- saved image/PDF region annotations are restored as coordinate overlays when the page is reopened;
- Knowledge center undo/redo for highlight and annotation creation, editing, and removal, with Ctrl+Z/Ctrl+Y shortcuts;
- contextual copy, Google, integrated dictionary, translation, and highlight actions for selected text;
- explicit, user-triggered online dictionary and translation results shown inside the app, with local lookup history, reopen/clear actions, lookup favorites, and copy actions;
- system text-to-speech through Qt Speech, with word highlighting when the platform voice engine exposes word progress;
- Markdown, JSON, and plain-text knowledge export, library metadata export, database backup and validated database restoration, and metadata restoration;
- persisted reader preferences for font family, text width, margins, alignment, reading direction, image fitting, custom reader colors, and named reading profiles, with global, format, content-type, document, and profile scopes;
- plain-text export of the currently opened document when selectable text or OCR text is available;
- PDF export and system printing of the opened document when its reader engine is available;
- SQLite migrations, safe read-only archive handling, Zip Slip checks, archive entry limits, and Qt Test coverage;
- GitHub Actions validation for Linux, macOS, and Windows.

Linux releases provide a direct x86_64 AppImage and a Debian/Ubuntu-compatible `.deb` package. Both are built from the same release tag and version.

Development builds can intentionally omit third-party engines when configured without the full-engine gate. The Windows and Linux release packages linked above are built with all supported engines and OCR data. See [the format and feature matrix](docs/FORMAT_SUPPORT.md) for format-specific limitations; an extension in a file picker is never treated as proof that a reader exists.

## Build prerequisites

- C++20 compiler;
- CMake 3.24 or newer;
- Qt 6.5 or newer with `Core`, `Gui`, `Network`, `Widgets`, `PrintSupport`, `Qml`, `Quick`, `QuickControls2`, `Sql`, and `Test`;
- `Pdf`, `TextToSpeech`, `Multimedia`, and `CorePrivate` modules enable PDF rendering, system speech, audio-backed speech support, and EPUB/CBZ/ZIP reading;
- libarchive headers and library enable CBR/CB7/CBT/RAR/7Z reading;
- DjVuLibre headers and library enable DJVU/DJV reading;
- Tesseract OCR plus the requested language data enable image-based recognition without uploading documents;
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
