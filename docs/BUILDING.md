# Building Lumina Reader

## Requirements

- C++20 compiler;
- CMake 3.24 or newer;
- Qt 6.5 or newer with `Core`, `Gui`, `Widgets`, `PrintSupport`, `Qml`, `Quick`, `QuickControls2`, `Sql`, and `Test`;
- `Pdf` for PDF rendering, `TextToSpeech` for system speech, and `CorePrivate` for the in-process EPUB/CBZ/ZIP reader. These are optional at configure time.
- `Widgets` and `PrintSupport` are required for the native print dialog and PDF export path.
- `libarchive` is optional. When its headers and library are discoverable by CMake, it enables in-process read-only CBR/CB7/CBT/RAR/7Z comic archives. The build remains valid without it.
- DjVuLibre is optional. When `libdjvu/ddjvuapi.h` and the DjVuLibre library are discoverable by CMake, it enables in-process read-only DJVU/DJV page rendering and hidden-text extraction. The build remains valid without it.
- Tesseract is an optional runtime dependency for OCR. Install it separately and expose `tesseract` on `PATH` (or set `LUMINA_TESSERACT_PATH`) to enable the reader's **Read image** action.
- Ninja for the checked-in default preset.

The project does not require a server, browser runtime, JavaScript engine, or network service to configure and test the local reader.

## Standard build

Run from the repository root:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

The default preset creates a local `build/default` directory, which is ignored by Git.

## Qt discovery

If CMake cannot find Qt, point it at the Qt kit prefix. The exact path depends on the operating system and compiler kit:

```sh
cmake --preset default -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit>
```

For Windows, use a Developer Command Prompt that provides the MSVC environment before configuring. For macOS, select the intended Xcode command-line tools and Qt kit. For Linux, install the compiler, Ninja, OpenGL development packages required by Qt, and the Qt development modules.

## Tests

The current tests are Qt Test executables registered with CTest:

- `lumina_smoke_tests` checks that the application version is defined;
- `lumina_database_tests` checks SQLite migration, FTS5 table creation, idempotence, and invalid input handling;
- `lumina_library_tests` covers import, duplicate detection, metadata, generated/custom covers, favorites, ratings, collections, tags, smart views, progress, rotation, highlights, annotations, OCR word coordinates, and bookmarks;
- `lumina_reader_tests` covers pagination, position persistence, document search, text/image PDF export with saved annotations, RTF/FB2/HTML text extraction, and safe HTML import/search;
- `lumina_archive_tests` covers EPUB/CBZ navigation and archive path-safety behavior when Qt Core private archive support is present;
- `lumina_settings_tests` covers persisted reading preferences;
- `lumina_backup_tests` covers knowledge/library export, metadata fields, metadata restoration, and integrity-checked database backup/restore.
- `lumina_ocr_tests` covers the optional OCR provider boundary and invalid-image handling; OCR word-coordinate persistence is covered by `lumina_library_tests`.

Tests use an in-memory SQLite database and do not use the internet or user documents.

For a headless QML startup check on a desktop kit, set `QT_QPA_PLATFORM=offscreen` and launch the built `lumina_reader` executable. The application should remain running instead of exiting during QML creation.

## Local development data

The running application stores its SQLite database under Qt's `AppDataLocation`. This data is local runtime state and must not be committed. Delete it only through an explicit user action or a platform-appropriate data-management workflow; do not add deletion commands to build scripts.
