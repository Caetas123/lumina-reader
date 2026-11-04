# Changelog

All notable changes to Lumina Reader will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows Semantic Versioning where a public release version exists.

## [0.2.5] - 2026-11-04

This release completes the full-engine packaging work for Windows and Linux and adds persisted reading profiles. The release assets are built from the same tag and include the OCR runtime required by the image-reading workflow.

### Added

- Named reading profiles with independent persisted preferences, profile switching, creation, and deletion from Settings > General.
- Full-engine release configuration that fails early when PDF, speech, libarchive, DjVuLibre, WebP, or AVIF support is missing.
- Bundled multilingual Tesseract OCR data in the Windows installer and Linux packages.

### Fixed

- Linux CI and release jobs now install the PulseAudio, PipeWire, and Speech Dispatcher dependencies required by Qt Multimedia and Qt Speech.
- Linux AppImage staging now carries the non-glibc Tesseract runtime libraries instead of assuming that the user's system has them.
- Linux OCR packaging now locates both modern and Ubuntu-style Tesseract tessdata directories and validates `configs/tsv` before deployment.
- Windows release tests now put the vcpkg and DjVu runtime DLL directories on `PATH` before CTest.
- Release packaging accepts the current libavif allocation API and uses the correct `libjpeg-turbo` vcpkg port on Windows runners.
- Release documentation and direct download links now point to the synchronized `v0.2.5` artifacts.

### Verified

- Windows full-engine build and 11/11 Qt tests passed locally.
- Ubuntu 24.04 full-engine build and 11/11 Qt tests passed locally.
- Real Windows GUI validation covered the full requested format matrix, OCR, annotations, Knowledge center, TTS controls, PDF export/print, lookup actions, settings export, and database backup integrity.
- The Linux executable launched through WSLg and rendered the native dark Lumina interface; detailed Linux control automation remains separately recorded in the final checklist where the WSLg wrapper limits accessibility inspection.

## [0.2.3] - 2026-11-03

This patch release completes the PDF validation pass in the Windows GUI and restores the in-document search selection after page navigation. Windows, Linux, and macOS packages are published from the same tag by GitHub Actions.

### Fixed

- PDF search now restores the first matching occurrence after moving between pages, so the result counter remains consistent instead of showing `0/N` for an active match.

### Verified

- A two-page PDF fixture was imported, opened, rendered, searched, navigated, zoomed, and inspected through the native Windows GUI; the corrected flow reports `1/1`, `No matches`, and `1/1` when returning to the matching page.

## [0.2.2] - 2026-11-03

This patch release keeps the validated Windows and Linux package flow and repairs the macOS DMG workflow for the Qt 6.8.3 `macdeployqt` command-line options.

### Fixed

- Removed the unsupported `-no-translations` flag from the macOS packaging workflow so the DMG job can complete on the configured Qt runner.

## [0.2.1] - 2026-11-03

This release completes the responsive dark Lumina design pass, repairs saved Knowledge center refresh and OCR command construction, and records the GUI-based cross-format audit. Windows, Linux, and macOS packages are published from the same tag by GitHub Actions.

### Fixed

- Settings sections now use the available page width and expose a clear way back to the library at compact window sizes.
- The reader toolbar wraps its controls instead of clipping them on narrow windows.
- Saved Knowledge center items refresh after navigation and an empty-term view lists saved items correctly.
- Tesseract receives its standard language option (`-l`) and reports a clear local-dependency message when it is not installed.
- Installer progress copy now describes the product operation instead of exposing an internal file name.

### Added

- Responsive dark/purple UI components and the empty-library artwork used by the validated Windows UI.
- Audit fixtures and a complete requirement-by-requirement validation checklist.

## [0.2.0] - 2026-11-03

This release includes the complete local-library restore flow, scoped reader preferences, offline reuse of saved lookups, performance benchmarks, and the repaired graphical Windows installer. Windows, Linux, and macOS packages are published from the same tag by GitHub Actions.

## [Unreleased]

### Added

- Initial Qt 6/CMake desktop application foundation.
- Qt Quick base theme and functional application window.
- SQLite connection handling with versioned migrations and an FTS5 search foundation.
- Typed domain errors and categorized diagnostic logging.
- Replaceable `PdfEngine` abstraction.
- Initial unit tests, cross-platform CI configuration, and open-source project documentation.
- Local document library import with duplicate detection, folder import, smart views, favorites, ratings, metadata, collections, tags, and progress history.
- Text, RTF, FB2, image, EPUB, CBZ, and ZIP reader paths, with optional Qt PDF and Qt Speech capabilities.
- Persistent highlights, annotations, bookmarks, global Knowledge center search, contextual browser lookup actions, and library/knowledge exports.
- Metadata restoration from a library JSON export, archive resource limits, Zip Slip checks, and a documented format-parity matrix.
- Unencrypted PalmDOC-compatible MOBI/AZW/AZW3 decoding, keyboard navigation shortcuts, focus/full-screen reading controls, and manual region annotations for image-based pages.
- Optional asynchronous Tesseract OCR for image/PDF/comic pages, with per-page persistence and Knowledge center indexing.
- A format-independent `DocumentEngine` contract with adapters for text, images, archives, EPUB/CBZ/ZIP, and optional Qt PDF rendering.
- Automatic and user-selected library covers, plus persisted page rotation for visual documents and backup restoration.
- Accumulated local reading time is persisted per document alongside progress and reading history.
- Optional libarchive-backed, read-only comic archive engines for CBR, CB7, CBT, RAR, and 7Z when the dependency is available at configure time.
- macOS x86_64 DMG packaging with Qt deployment and a self-contained application bundle.
- Integrated optional dictionary and translation dialogs using explicit, bounded online requests; results can be copied and lookup history remains local.
- Dictionary and translation history can now be reviewed, reopened, cleared, and marked as favorites from the lookup dialog.
- Editable document genres with case-insensitive library filters and backup/restore support.
- Optional DjVuLibre-backed DJVU/DJV reading with page rendering and hidden-text extraction.
- Scoped local search history for the library, open document, and Knowledge center, with reopening and clearing.
- Continuous text reading, chapter-aware Contents navigation, selectable extracted page text, OCR language selection, selected-passage notes, and Knowledge undo/redo.
- Plain-text export for the opened document when selectable or OCR text is available, with protection against overwriting the source file.
- One `project-version.txt` file now controls the application, installer, and cross-platform release version; packaging workflows reject mismatched Git tags.
- Document-wide OCR now processes image pages sequentially with progress, cancellation, and local persistence.
- Basic PDF export and system printing now render the opened document without modifying the original file.
- Visual documents now support single-page and two-page reading layouts with matching navigation steps.
- System read-aloud can now highlight the spoken word when the installed Qt Speech backend reports word progress.
- Saved image and PDF region annotations are now redrawn over their original page coordinates after reopening.
- PDF export and system printing now carry saved text highlights and visual region annotations into the output.
- OCR now persists Tesseract word coordinates so saved OCR highlights can be restored over the source image.
- Linux release packaging now publishes both an x86_64 AppImage and a Debian/Ubuntu-compatible `.deb` from the same versioned build.
- HTML and HTM files can now be imported, searched, and read as safe local text without executing page content.
- Reader appearance preferences can now be saved globally or for a format, content type, or individual document, with settings export and restore preserving those layers.
- The Windows installer now keeps the selected desktop-shortcut option, renders its progress page safely, creates a verifiable Start Menu shortcut, and opens the installed application from the completion page.
- Qt benchmarks now cover library refresh/search and representative text/image reading paths without requiring network services.

### Changed

- The file picker now advertises only readers enabled in the current Qt build instead of treating every planned extension as supported.
- The README and architecture/building documents now describe the real current behavior and optional dependencies.

## [0.1.6] - 2026-11-02

### Fixed

- The Windows installer now launches Lumina Reader directly after installation and reports a clear error when the installed executable cannot be started.

### Added

- The Lumina Reader application window, Windows executable, installer, and shortcuts now use the project logo.

### Changed

- The application and Windows installer metadata now use version `0.1.6`.

## [0.1.5] - 2026-11-02

### Added

- Linux x86_64 AppImage published as a direct executable download.
- Automated Linux packaging and GitHub Release publication when a version tag is pushed.

### Fixed

- Linux packaging now excludes only an optional Qt SQL driver that depends on an unavailable proprietary library; SQLite remains available to the application.
- GitHub Actions now uses Qt-compatible Linux, Windows, and macOS runner/toolchain combinations.

### Changed

- The application and release metadata now use version `0.1.5`.

## [0.1.4] - 2026-11-02

### Changed

- Redesigned the Windows installer with a branded visual layout, clearer hierarchy, polished cards, rounded controls, and a dedicated progress experience.
- Preserved localized user-facing copy, installation-folder selection, Start menu integration, and optional desktop shortcut creation.
- The application and release metadata now use version `0.1.4`.

## [0.1.3] - 2026-11-02

### Added

- Lumina Reader logo asset used by the application and Windows installer.
- User-facing installation folder selection and optional desktop shortcut creation.

### Changed

- The first installer screen now presents Lumina Reader as a product without technical implementation terms.
- The application and release metadata now use version `0.1.3`.

## [0.1.2] - 2026-11-02

### Added

- System-language detection for the Windows installer, with Portuguese and English user interfaces.

### Changed

- The installer now selects Portuguese for Portuguese Windows installations and uses English as the fallback language.
- The application and release metadata now use version `0.1.2`.

## [0.1.1] - 2026-11-02

### Added

- Graphical Windows installer with an English welcome screen, installation confirmation, progress feedback, and Start menu shortcut creation.
- Windows x64 installer download published alongside the portable package.

### Changed

- The application and release metadata now use version `0.1.1`.
