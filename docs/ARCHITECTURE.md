# Architecture

Lumina Reader is organized into layers so that the QML interface does not own business rules or database connections.

```text
ui (QML and C++ view models)
        |
application (use cases and orchestration)
        |
domain (entities, value types, reader contracts)
        |
infrastructure       platform
(SQLite, files,      (engines,            (OS-specific
 cache, OCR, TTS)     adapters)            integrations)
```

## Layer responsibilities

### `ui`

Contains QML screens, reusable visual components, design tokens, navigation, focus behavior, and accessibility presentation. QML receives state through C++ properties, models, and invokable commands. It must not issue SQL queries or manipulate filesystem paths directly.

### `application`

Coordinates user-facing use cases such as import, open, search, and reading-position persistence. It validates input, calls domain services, and exposes stable models/controllers to QML.

### `domain`

Contains product concepts and rules independent of Qt Quick, SQLite, and operating-system APIs. Domain errors distinguish user-safe messages from diagnostic messages.

### `infrastructure`

Provides persistence, migrations, filesystem access, caches, indexing, OCR, dictionary providers, text-to-speech adapters, backup, and export. Infrastructure implementations are replaceable behind interfaces where external behavior is involved.

The current concrete services include `Database`, `LibraryStore`, `ArchiveReader`, `TextDecoder`, `DocumentEngineFactory`, the optional libarchive and DjVuLibre adapters, `TesseractOcrProvider`, `BackupController`, `SettingsController`, `LookupController`, and the optional `SpeechController`. `LibraryStore` owns the C++ model exposed to QML; it also owns import validation, duplicate detection, cover generation, FTS5 updates, collections, tags, favorites, annotations, bookmarks, OCR page text, page rotation, reading time, reading history, and a bounded in-memory undo/redo stack for Knowledge edits. `LookupController` keeps online dictionary and translation requests explicit, parses bounded JSON responses, records only local lookup history, and exposes history reopening, clearing, and favorites to the lookup dialog. `ReaderController` renders the opened pages through `QPdfWriter` for PDF export or `QPrinter` for system printing, carrying text highlights and normalized visual-region annotations into the output while leaving the source document untouched. `SpeechController` tracks the optional Qt Speech backend's word-progress signal and exposes the current utterance range to QML when supported.

### `reader engines`

`domain/reader/DocumentEngine.h` is the central contract used by `ReaderController`. It exposes open/close, metadata identity, page count, page kind, text extraction, rendering, and safe errors without exposing Qt PDF, ZIP, DjVuLibre, or text-decoder implementation details to the application layer. `DocumentEngineFactory` selects the appropriate adapter for image, text, archive, EPUB/CBZ/ZIP, optional PDF, and optional DJVU/DJV documents.

The current PDF adapter uses Qt PDF when `Qt6::Pdf` is available. The dependency remains optional so that the core, text, image, and database tests can still build on a Qt installation without the PDF add-on. The UI reports the missing capability instead of accepting a PDF that cannot be opened. The older `PdfEngine` header remains available for future backend-specific adapters.

The DjVu adapter uses the read-only DjVuLibre `ddjvuapi` when that optional library is found at configure time. It waits for bounded document/page decode jobs, renders RGB pages through the shared engine contract, and extracts hidden page text without exposing the third-party API to the application layer. Builds without DjVuLibre omit DJVU/DJV from the picker.

### `platform`

Contains integrations that differ across Windows, macOS, and Linux, such as file associations, native dialogs, notifications, printing, clipboard, and system voices.

## Current foundation

- `AppController` is the initial QML-facing application coordinator.
- `Database` owns one SQLite connection and applies versioned migrations in transactions.
- Migration versions 1–10 create application settings, documents, reading state, annotations, bookmarks, OCR/provider history tables, FTS5 indexes, favorites, ratings, OCR knowledge entries, persisted page rotation, accumulated reading time, document genres, and dictionary/translation history favorites.
- `LibraryStore` is a `QAbstractListModel`; its view filter is applied in SQL, its FTS5 queries have a safe LIKE fallback for unusual input, and its three search scopes persist a bounded local history without recording document contents.
- `ArchiveReader` reads EPUB and image archives in-process without extracting untrusted paths. It rejects parent/absolute paths, symbolic links, oversized entries, and archives over the configured entry/size limits.
- `TextDecoder` converts TXT, RTF, FB2, and unencrypted PalmDOC-compatible MOBI/AZW/AZW3 files into safe reader text without executing document content. DRM and unsupported KF8/Huffman variants are rejected.
- Image and rendered-PDF pages expose a manual region-annotation fallback when selectable text or OCR is unavailable; region coordinates are stored normalized to the displayed page. The QML reader can display one or two visual pages and advances by the active spread size. Text highlights and visual regions are preserved when pages are rendered to PDF or a system print job.
- `OcrProvider` defines the provider boundary and returns recognized text plus normalized word boxes; `TesseractOcrProvider` runs the local Tesseract executable asynchronously from TSV output, while `ReaderController` coordinates page and document-wide passes and stores completed OCR and word coordinates by document and page.
- `ReaderController` owns the reader state and keeps page locations/progress in the library model. The QML layer never opens files or writes SQL directly.
- Image, PDF, and comic pages can be rotated through the reader; the normalized rotation is stored with the document and included in library backups.
- `Logging` configures categorized diagnostics without creating a persistent log file or recording document content.
- QML design tokens live in `src/ui/qml/Theme.qml`.

## Invariants

- Original PDFs are read-only unless a later, explicit export operation is confirmed by the user.
- Original documents are not copied, moved, or deleted by a normal library import or removal.
- No network provider is required for the local reading flow.
- Browser and integrated online lookups are explicit user actions and receive only the selected text.
- New long-running work must not run on the GUI thread; the current import path remains synchronous and is intentionally bounded by format and archive-size limits.
- Database schema changes are additive migrations; historical migration files are not rewritten.
- Technical error details remain in diagnostics; UI messages stay actionable and safe.
