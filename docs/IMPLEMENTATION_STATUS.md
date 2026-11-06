# Implementation status

Audit date: 2026-11-06
Branch: `main`
Release under audit: `0.2.7`

This report describes the current repository and separates local evidence from
platform evidence that must come from GitHub Actions. The final checklist uses
only `PASS`, `FAIL`, and `BLOCKED`; a blocked item is kept visible instead of
being converted into a guessed pass.

## Implemented product surface

- Native C++20, Qt 6, Qt Quick/QML, SQLite, FTS5, CMake, and Qt Test desktop
  application with no browser, Electron, WebView, or server dependency.
- Local library import, duplicate detection, folder import, drag and drop,
  optional folder watching, metadata, covers, statuses, progress, reading time,
  favorites, ratings, genres, languages, collections, tags, and reading history.
- TXT, RTF, HTML/HTM, FB2, unencrypted PalmDOC-compatible MOBI/AZW/AZW3,
  EPUB, PDF, comic archives, DJVU/DJV, and raster image reading through the
  format-independent document engine contract.
- Full Windows/Linux release engines: Qt PDF, Qt Speech, Qt Multimedia,
  libarchive, DjVuLibre, libwebp, libavif, and bundled Tesseract OCR.
- Fully local OCR packaging for the official Tesseract `tessdata_fast`
  manifest: 125 data files are bundled in the Windows/Linux release packages
  (123 selectable language/script models after excluding `equ` and `osd`),
  with runtime discovery of the models actually installed.
- Single-page and two-page visual layouts, zoom, rotation, thumbnails,
  contents navigation, in-document search, selectable text, bookmarks, focus
  mode, full screen, and keyboard navigation.
- Persistent highlights, notes, annotations, OCR text and word coordinates,
  Knowledge center search, filters, export, and undo/redo.
- Explicit Google, dictionary, and translation actions with local cache/history;
  no online request is made without a user action.
- Named reading profiles with persisted preferences, switching, creation, and
  deletion in Settings > General.
- Cross-platform language preference infrastructure: automatic system-locale
  detection on Windows/Linux, a persistent Settings > General selector covering
  the Windows desktop language-pack/LIP locale catalog, live QML retranslation,
  and a shared compressed offline catalog for the installer and application,
  covering all 91 selectable Windows language-pack/LIP entries and aliases.
- Plain-text and annotated PDF export, system printing, library/Knowledge/
  settings export, metadata restore, and integrity-checked database backup and
  restore with a recoverable previous copy.
- Responsive dark/purple design system, themes, high contrast, accessible
  labels/focus, and compact-layout handling for the audited Windows UI.
- English user-facing project documentation, direct release links, graphical
  Windows installer with Start menu/desktop-shortcut choices, and synchronized
  version checks from `project-version.txt`.

## Local automated evidence

The full Windows/MSVC build used Qt 6.8.3, libarchive, DjVuLibre, libwebp,
libavif, Qt PDF, Qt Speech, Qt Multimedia, and the release gate. It passed all
11 registered Qt tests. The Ubuntu 24.04 WSL build used the same source and
full-engine gate and passed all 11 registered Qt tests. The Linux packaging
script passes `bash -n` syntax validation.

The image-codec test is also run with the real WebP and AVIF fixtures in the
final evidence pass; it is not accepted merely because the test binary exists.

For release `v0.2.7`, the pinned Tesseract manifest and commit were validated
in the Windows and Linux packaging workflows. The downloaded AppImage and
Debian package each contain exactly 125 `.traineddata` files, the Tesseract
executable, `configs/tsv`, and `TESSERACT_SOURCE.txt` identifying the pinned
source commit.

## GUI evidence

The Windows release-stage executable was driven through the native GUI and the
Windows file picker. The audit covered library/list views, representative and
archive formats, PDF rendering/search/navigation, image OCR, OCR persistence
into SQLite/FTS5, highlights and Knowledge center, lookup actions, TTS controls,
settings and named profiles, history, export, backup integrity, compact window
layouts, and the final dark/purple visual language.

The packaged Windows OCR flow produced recognized text, saved an OCR page,
created word rows and a Knowledge search row, and returned the status
`Text recognized and saved to the Knowledge center.` The database was checked
read-only with SQLite integrity verification after the GUI action.

The Linux executable was built and launched through WSLg and rendered the
native interface. WSLg accessibility automation is limited in this Windows
host, so detailed Linux click-by-click parity is recorded as `BLOCKED` in the
final checklist rather than inferred from the Windows run. Native macOS GUI
validation is also `BLOCKED` in this host; its package remains covered by its
own GitHub Actions workflow.

## Intentional limitations

- EPUB and HTML use safe native text extraction rather than browser layout.
- MOBI/AZW/AZW3 support is limited to unencrypted PalmDOC-compatible text.
- TTS word highlighting depends on the installed operating-system voice
  backend exposing word-progress callbacks.
- Online lookup providers are explicit integrations and require network access;
  previously fetched results can be reused from local history.
- The macOS workflow publishes the existing core Qt DMG and is not configured
  with the Windows/Linux full-engine gate; its native capability set must not be
  claimed from this Windows-hosted audit.

See [AUDIT_CHECKLIST_FINAL.md](AUDIT_CHECKLIST_FINAL.md) for the requirement-
by-requirement result and evidence paths.
