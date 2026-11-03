# Implementation status

Audit date: 2026-11-03  
Branch: `feature/complete-reader`

This report describes the behavior implemented in the repository, not only the controls visible in QML. Optional capabilities are marked as conditional instead of being presented as universally available.

## Verified in the current Windows build

- Native C++20, Qt 6, Qt Quick/QML, SQLite, FTS5, CMake, and Qt Test architecture.
- Local library import, duplicate detection, folder import, drag and drop, optional folder watching, metadata, covers, status, progress, reading time, favorites, ratings, genres, languages, formats, collections, tags, moved-file relinking, and safe removal.
- Text, RTF, HTML/HTM, FB2, unencrypted PalmDOC-compatible MOBI/AZW/AZW3, images, EPUB/CBZ/ZIP through the Qt archive capability, and conditional PDF/DJVU/libarchive engines.
- Paged reading, continuous text reading, page navigation, contents/spine navigation, thumbnails, zoom, rotation, focus mode, full screen, bookmarks, in-document search, and selectable extracted page text.
- Single-page and two-page visual layouts with page-step navigation.
- Persistent highlights with color/style/comment, selected-passage notes, image-region annotations with restored coordinate overlays, bookmarks, OCR page storage, document-wide OCR, global Knowledge center search, filters, ordering, editing, deletion, export, and Knowledge undo/redo.
- Scoped local search history for the library, the open document, and the Knowledge center, including deduplication, retention, reopening, and clearing.
- Explicit Google, dictionary, and translation actions; lookup history can be reopened, cleared, favorited, and copied.
- Optional local Tesseract OCR with selectable English, Portuguese, Spanish, Japanese, Korean, and Chinese language codes, persisted word coordinates, and selection-to-image mapping.
- Optional system text-to-speech with start, pause, resume, stop, rate, volume, selected-passage reading, continuous-document reading, and backend-dependent spoken-word highlighting.
- Themes, font size, line spacing, continuous reading, high contrast, keyboard navigation, accessible labels/tooltips, persisted font/layout/color reader preferences at global, format, content-type, and document scope with fallback precedence, settings export/restore, library export/restore, knowledge export, database backup, and integrity-checked database restoration with a recoverable previous copy.
- Plain-text export of the opened document without changing the source file.
- PDF export and system printing of the opened document, including text highlights and normalized visual-region annotations, without changing the source file.
- One cross-platform release version from `project-version.txt`, with tag checks shared by the Linux, Windows, and macOS packaging workflows.
- Archive safety checks for normalized paths, symbolic links, entry count, entry sizes, total size, unsupported encryption, and malformed readers.

## Automated validation

The clean Windows/MSVC validation build used Qt 6.8.3 and completed:

```text
10/10 tests passed
lumina_smoke_tests
lumina_database_tests
lumina_lookup_tests
lumina_library_tests
lumina_reader_tests
lumina_archive_tests
lumina_settings_tests
lumina_backup_tests
lumina_ocr_tests
lumina_performance_tests
```

The graphical executable also stayed alive for more than ten seconds with `QT_QPA_PLATFORM=offscreen` after each clean build used for the latest changes. The process was then stopped as a test cleanup operation.

The performance target uses Qt Test benchmarks for library refresh, Knowledge search, large-text decoding, and representative image-page decoding. These are repeatable local micro-benchmarks rather than claims about every machine or every optional engine.

## Platform and dependency matrix

| Capability | Windows validation | Linux/macOS repository support | Condition |
| --- | --- | --- | --- |
| Core reader, library, SQLite, FTS5 | Verified | Covered by CI configuration | Qt 6 base modules |
| PDF | Code path and CI request | Code path and CI request | `Qt6::Pdf` |
| EPUB/CBZ/ZIP | Code path | Code path | Qt `CorePrivate` archive helper |
| CBR/CB7/CBT/RAR/7Z | Conditional code path | Conditional code path | libarchive at configure time |
| DJVU/DJV | Conditional code path | Conditional code path | DjVuLibre at configure time |
| OCR | Provider tests; runtime availability is machine-dependent | Same | Tesseract executable and language data |
| Text-to-speech | Conditional | Conditional | Qt TextToSpeech and a system voice backend |
| Windows installer | Existing packaging workflow | Not applicable | Windows/.NET packaging toolchain |
| Linux AppImage and `.deb` | Existing packaging workflow | Existing packaging workflow | linuxdeployqt, dpkg-deb, and Linux runner |
| macOS DMG | Not run in this Windows workspace | Existing packaging workflow | macOS runner and Qt deployment |

## Intentional limitations

- EPUB is rendered as a safe native text view. CSS pagination, embedded fonts/media, links, and nested navigation levels are not reproduced; the Contents panel follows the EPUB spine and uses chapter labels when available.
- HTML/HTM is converted to safe local plain text. Scripts, styles, frames, objects, and embeds are not executed or shown, and browser layout/resources are not reproduced.
- PDF pages are rendered through Qt PDF when available. Extracted text is exposed in a selectable fallback view, visual pages support single-page/two-page layouts, saved manual regions are restored as coordinate overlays, and text highlights/visual regions are carried into exported or printed output; native PDF text-selection coordinate mapping remains limited because the fallback view does not expose the source PDF glyph geometry.
- OCR supports asynchronous page and document-wide passes, persists normalized Tesseract word boxes, and maps saved OCR text highlights back over the original image; manual region annotations remain the safe fallback when OCR is unavailable.
- Dictionary and translation providers are explicit online integrations. No bundled local dictionary database is shipped; previously fetched definitions and translations are cached in the local database and reused offline, and no external request occurs without a user action.
- Reader appearance preferences support global, format, content-type, and document layers with document-first fallback to type, format, and global values. Named user profiles are not implemented yet.
- Qt Speech word-level highlighting is enabled when the selected backend exposes the Qt 6.6+ word-progress capability; engines without that capability still support read-aloud without visual word tracking.
- The current Windows workspace did not provide the optional Qt PDF, CorePrivate, libarchive, DjVuLibre, or Tesseract dependencies, so those runtime paths were not claimed as locally executed in this audit.
- Native macOS and Linux execution was not performed in this Windows workspace; the workflows and packaging scripts remain the source of truth for those platforms.

## Recent thematic commits

- `f5caac7` — restore saved visual region annotations over page coordinates.
- `c114061` — normalize optional highlight notes and cover cross-format Knowledge search.
- `c8ee5bd` — cover visual page sources and annotated PDF export paths.
- `b4d946c` — persist OCR word coordinates and restore OCR highlights over image regions.
- `ce81667` — highlight the spoken word when the speech backend provides progress.
- `c6ce6ad` — add single-page and two-page visual reading layouts.
- `cb6511e` — add basic PDF export and system printing.
- `3d0acbb` — add document-wide OCR with progress and cancellation.
- `06acb2a` — unify cross-platform release versions and tag validation.
- `c5a1698` — export selectable document text safely.
- `db73f7d` — add the implementation audit.
- `6badc8f` — preserve text selection offsets in locators.
- `feae2be` — add Knowledge highlight/annotation undo and redo.
- `ba304f7` — add selected-passage notes.
- `b778abd` — expose selectable extracted page text.
- `b962f86` — add chapter/contents navigation.
- `42d49cd` — choose OCR language.
- `9fda3d6` — read continuous passages aloud.
- `a485f9a` — persist scoped search history.
- `fdeabcd` — add lookup history favorites.
- `b33707f` — add library metadata filters and sorting.
- `7e91a7f` — add optional DjVu engine.

No push or release publication was performed by this work.
