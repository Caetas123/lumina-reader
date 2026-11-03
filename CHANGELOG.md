# Changelog

All notable changes to Lumina Reader will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows Semantic Versioning where a public release version exists.

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
