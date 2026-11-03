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
