# Lumina Reader final audit checklist

Audit date: 2026-11-03
Target release: `0.2.3`
Local platform: Windows x64
Validation executable: `C:\Users\caeta\AppData\Local\Temp\lumina-reader-ui-build-20261103\lumina_reader.exe`

This checklist is evidence-bounded. `PASS` means the behavior was exercised in the current Windows build or covered by a passing automated test. `CONDITIONAL` means the code path exists but depends on an optional module or external provider that was not available on this host. It is not reported as a local pass. `REMOTE` is reserved for GitHub Actions and release evidence.

## Build and automated tests

- [x] `PASS` C++20/CMake/Qt 6 build completed with MSVC and Qt 6.8.3.
- [x] `PASS` QML cache generation completed during the build.
- [x] `PASS` CTest passed all 10 targets: smoke, database, lookup, library, reader, archive, settings, backup, OCR, and performance.
- [x] `PASS` `git diff --check` is clean for the release changes.
- [x] `PASS` OCR command construction is unit-tested with Tesseract's standard `-l <language>` option.
- [x] `PASS` archive safety tests cover normalized paths and Zip Slip rejection.

## Format matrix

- [x] `PASS` TXT: imported and opened through the GUI; text search and reading were exercised.
- [x] `PASS` RTF: imported and shown in the library; the text reader path is covered by the parser tests.
- [x] `PASS` HTML/HTM: imported and shown in the library; safe script/style removal is covered by the decoder tests.
- [x] `PASS` FB2: imported and shown in the library; XML text extraction is covered by the decoder tests.
- [x] `PASS` PNG/JPG/JPEG image path: PNG was imported and opened as a visual document through the GUI.
- [x] `PASS` EPUB: imported and opened through the GUI; the chapter text and page count were visible.
- [x] `PASS` CBZ: imported and opened through the GUI; a two-page image archive was navigated with Next.
- [x] `PASS` ZIP comics: the same image-page archive was accepted by the ZIP path. A text-only ZIP was deliberately rejected because ZIP support is defined as image-page archive support.
- [x] `PASS` unencrypted MOBI/AZW/AZW3 decoding is covered by automated reader tests.
- [x] `PASS` PDF: a two-page fixture was imported and opened through the native Windows GUI with Qt PDF 6.8.3; page rendering, page navigation, zoom, selectable extracted text, and in-document search were exercised. After the search-selection fix, the GUI reported `1/1` on the matching page, `No matches` on the second page, and `1/1` again after returning to the first page.
- [ ] `CONDITIONAL` CBR/CB7/CBT/RAR/7Z: libarchive was not available in this Windows validation kit.
- [ ] `CONDITIONAL` DJVU/DJV: DjVuLibre was not available in this Windows validation kit.
- [ ] `CONDITIONAL` WEBP/AVIF/GIF: availability depends on the Qt image plugins shipped by the target package.

## Library and reading workflow

- [x] `PASS` add documents through the native file picker;
- [x] `PASS` duplicate detection, metadata cards, status, progress, ratings, favorites, and reading history are covered by the library tests and GUI cards;
- [x] `PASS` grid and list view document opening use the same reader path;
- [x] `PASS` library search filters the visible cards;
- [x] `PASS` reader page controls, previous/next navigation, search, export controls, bookmarks, focus/full-screen controls, and visual layout controls are present in the executable;
- [x] `PASS` saved highlights refresh in Knowledge center after leaving the reader;
- [x] `PASS` History is a centered, themed dialog and exposes saved local searches;
- [x] `PASS` Settings General, Reading, Appearance, Files & library, Privacy, and About render as separate sections and can return to the library.

## OCR, selection, and external actions

- [x] `PASS` the OCR UI is non-blocking and reports `Tesseract OCR is not installed. Install it and make it available on PATH.` on this host;
- [x] `PASS` the missing-provider path does not crash or falsely claim that recognition succeeded;
- [x] `PASS` Tesseract language argument construction and coordinate output parsing are covered by unit tests;
- [ ] `CONDITIONAL` real OCR recognition, page/document-wide OCR, selectable OCR words, and OCR-backed highlights require a Tesseract executable plus language data;
- [ ] `CONDITIONAL` Google/dictionary/translation actions require an explicit user action and network/provider availability; no online action was triggered during this audit.

## Design and responsive behavior

- [x] `PASS` the validated executable uses the approved dark navy/purple design tokens, logo, empty-library artwork, themed controls, selected/hover states, cards, and centered dialogs;
- [x] `PASS` the 980×620 compact state was inspected through the GUI with no horizontal clipping in the library or Settings pages;
- [x] `PASS` the reader toolbar wraps to multiple rows at compact width instead of hiding controls;
- [x] `PASS` the 1100×700 normal state and maximized state were checked for the main library layout;
- [ ] `CONDITIONAL` native Linux rendering was not executable in this Windows workspace; Linux GUI parity is gated by the Linux CI/package job and should be checked from its artifact on a Linux desktop before calling it a visual pass;
- [ ] `CONDITIONAL` native macOS rendering was not executable in this Windows workspace.

## Safety, privacy, and performance

- [x] `PASS` archive access is read-only and enforces entry count, entry-size, total-size, normalized-path, symlink, and malformed-reader checks;
- [x] `PASS` HTML/HTM content is sanitized to local text; scripts, styles, frames, objects, and embeds are not executed;
- [x] `PASS` library and Knowledge data remain local by default and export/backup paths are covered by tests;
- [x] `PASS` performance benchmarks for refresh/search, text decoding, and representative image decoding completed in the test suite.

## Packaging and release gate

- [x] `PASS` `project-version.txt` is the single source of the `0.2.3` release version;
- [x] `PASS` Windows workflow builds the graphical installer and publishes the direct `.exe` asset, with its internal ZIP kept out of the end-user download table;
- [x] `PASS` the Windows installer script was run locally for `0.2.1`; the generated installer was copied to `C:\Users\caeta\Downloads\LuminaReader-Setup-windows-x64.exe` and its SHA-256 matched the repository staging copy;
- [x] `PASS` Linux workflow builds both the direct x86_64 AppImage and Debian package from the same tag;
- [x] `PASS` tag/version verification is present in the Windows, Linux, and macOS packaging workflows;
- [x] `REMOTE` GitHub Actions CI passed on the release commit `2d4fc66` in run `41700907239` (Windows, Linux, and macOS jobs); the code-fix commit `00c18b3` also passed in run `41699599226`.
- [x] `REMOTE` tag-triggered packaging passed for `v0.2.3`: Windows run `41700952929`, Linux run `41700953148`, and macOS run `41700952934`.
- [x] `REMOTE` `v0.2.3` is published at [GitHub Releases](https://github.com/Caetas123/lumina-reader/releases/tag/v0.2.3) with direct assets, no end-user ZIP, and verified SHA-256 digests:
  - Windows [installer EXE](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.3/LuminaReader-Setup-windows-x64.exe): `c8624a73ef5c53974ba1362b9d53e8da554d7754d8ccad3398923b0910b16ff4`;
  - Linux [AppImage](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.3/LuminaReader-linux-x86_64.AppImage): `9718aff79c834ea607b53a6df37b33d153372f92a87db16c4630c2f743b2ff8d`;
  - Linux [Debian package](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.3/LuminaReader-linux-x86_64.deb): `f0b5c2807618c882445435f002b1d3f07593510fee50d012e2fdf852b02a8fb0`;
  - macOS [DMG](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.3/LuminaReader-macos-x86_64.dmg): `9d6fa0f96340520fbdc4738ce072792f4bb65318445f442b466a77976c331914`.

## Audit conclusion

The Windows build is locally validated for the core reader/library flow and the formats listed as `PASS`, including a real two-page PDF opened and searched through the GUI. The `v0.2.3` release commit, CI, Windows installer, Linux AppImage/`.deb`, and macOS DMG are remotely validated with the exact runs and digests above. Optional libarchive, DjVu, Tesseract, and native Linux/macOS visual evidence remains explicitly conditional because those dependencies or GUI environments were not available for direct local interaction in this Windows session. This checklist does not call those areas `100%` by inference.
