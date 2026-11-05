# Building Lumina Reader

Lumina Reader is a native C++20/Qt 6 desktop application. The repository contains a development build and a full-engine release build. The release gate is intentionally strict: it stops at configure time when a document engine required by the published package is missing.

## Common requirements

- C++20 compiler;
- CMake 3.24 or newer;
- Ninja;
- Qt 6.8.3 or newer with `Core`, `Gui`, `Widgets`, `PrintSupport`, `Network`, `Qml`, `Quick`, `QuickControls2`, `QuickDialogs2`, `Sql`, `Test`, `Pdf`, `TextToSpeech`, `Multimedia`, and `CorePrivate`;
- libarchive;
- DjVuLibre;
- libwebp;
- libavif;
- Tesseract OCR plus the 125-file manifest in `tools/tesseract-languages.txt` and the pinned `tessdata_fast` commit in `tools/tessdata-fast-ref.txt` for the full package.

The application has no server, browser runtime, Electron, WebView, or JavaScript service dependency. Online dictionary and translation calls are explicit user actions; normal reading and OCR remain local.

## Development build

Use the development build when working on the application without every native engine installed:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

If Qt is not on CMake's search path, provide its kit prefix:

```sh
cmake --preset default -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit>
```

Development configuration may omit `Pdf`, `TextToSpeech`, `Multimedia`, libarchive, DjVuLibre, libwebp, or libavif. The application then hides the unavailable format instead of claiming that an extension is supported.

## Full-engine release build

The release workflows configure with `-DLUMINA_REQUIRE_FULL_ENGINES=ON`. Use the same flag locally after installing the dependencies above:

```sh
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit> \
  -DLUMINA_REQUIRE_FULL_ENGINES=ON
cmake --build build/release --parallel
ctest --test-dir build/release --output-on-failure
```

CMake verifies Qt PDF/Speech/Multimedia, libarchive, DjVuLibre, libwebp, and libavif before generating the release build. A successful compile is therefore also evidence that those engines were detected by the configure step, not merely installed somewhere on the machine.

## Windows

Use an MSVC Developer Command Prompt with the Windows SDK and Qt's `msvc2022_64` kit. The release workflow uses vcpkg for libarchive, libwebp, libavif, and JPEG, builds DjVuLibre from the pinned revision in `tools/build-djvulibre-windows.ps1`, and stages Tesseract with `tools/stage-tesseract-windows.ps1`.

The graphical installer is built from the staged application directory. The ZIP created during that workflow is an internal installer input only; it is not an end-user download.

## Linux

On Ubuntu/Debian, install the release dependencies with:

```sh
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libarchive-dev libavif-dev libdjvulibre-dev libwebp-dev \
  libpulse-dev libpipewire-0.3-0 libspeechd2 speech-dispatcher \
  tesseract-ocr
```

The AppImage and Debian package copy Tesseract, all models in the language
manifest, `tessdata/configs/tsv`, and its non-glibc runtime dependencies into
the deployed application. The application looks for `usr/bin/ocr/tesseract`
beside the deployed executable before falling back to `PATH`.

The release runner is Ubuntu 22.04 so linuxdeployqt's portable-glibc guard is
enabled by default. When validating the script on a newer local distribution,
set `LUMINA_ALLOW_NEW_GLIBC=1`; this is an explicit local-only opt-in and is not
used by the release workflow.

## Tests

The Qt Test suite is registered with CTest. It covers database migrations and FTS5, library import/search/metadata, document engines and archive safety, reader persistence/export, lookup mocks, settings and named profiles, backup/restore, OCR boundaries, image codecs, and performance benchmarks. Tests use temporary or in-memory data and do not upload user documents.

For a headless QML startup check:

```sh
QT_QPA_PLATFORM=offscreen ./build/release/lumina_reader
```

The process should remain alive after QML creation. GUI validation must still be performed with the real desktop executable; an offscreen launch is only a startup check.

## Local data

The running application stores its SQLite database under Qt's `AppDataLocation`. This is user runtime state and must not be committed or deleted by build scripts.
