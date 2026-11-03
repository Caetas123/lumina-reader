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

The current concrete services include `Database`, `LibraryStore`, `ArchiveReader`, `TextDecoder`, `DocumentEngineFactory`, the optional libarchive adapter, `TesseractOcrProvider`, `BackupController`, `SettingsController`, and the optional `SpeechController`. `LibraryStore` owns the C++ model exposed to QML; it also owns import validation, duplicate detection, cover generation, FTS5 updates, collections, tags, favorites, annotations, bookmarks, OCR page text, page rotation, reading time, and reading history.

### `reader engines`

`domain/reader/DocumentEngine.h` is the central contract used by `ReaderController`. It exposes open/close, metadata identity, page count, page kind, text extraction, rendering, and safe errors without exposing Qt PDF, ZIP, or text-decoder implementation details to the application layer. `DocumentEngineFactory` selects the appropriate adapter for image, text, archive, EPUB/CBZ/ZIP, and optional PDF documents.

The current PDF adapter uses Qt PDF when `Qt6::Pdf` is available. The dependency remains optional so that the core, text, image, and database tests can still build on a Qt installation without the PDF add-on. The UI reports the missing capability instead of accepting a PDF that cannot be opened. The older `PdfEngine` header remains available for future backend-specific adapters.

### `platform`

Contains integrations that differ across Windows, macOS, and Linux, such as file associations, native dialogs, notifications, printing, clipboard, and system voices.

## Current foundation

- `AppController` is the initial QML-facing application coordinator.
- `Database` owns one SQLite connection and applies versioned migrations in transactions.
- Migration versions 1–8 create application settings, documents, reading state, annotations, bookmarks, OCR/provider history tables, FTS5 indexes, favorites, ratings, OCR knowledge entries, persisted page rotation, and accumulated reading time.
- `LibraryStore` is a `QAbstractListModel`; its view filter is applied in SQL, and its FTS5 queries have a safe LIKE fallback for unusual input.
- `ArchiveReader` reads EPUB and image archives in-process without extracting untrusted paths. It rejects parent/absolute paths, symbolic links, oversized entries, and archives over the configured entry/size limits.
- `TextDecoder` converts TXT, RTF, FB2, and unencrypted PalmDOC-compatible MOBI/AZW/AZW3 files into safe reader text without executing document content. DRM and unsupported KF8/Huffman variants are rejected.
- Image and rendered-PDF pages expose a manual region-annotation fallback when selectable text or OCR is unavailable; region coordinates are stored normalized to the displayed page.
- `OcrProvider` defines the provider boundary; `TesseractOcrProvider` runs the local Tesseract executable asynchronously, and `ReaderController` stores completed OCR by document and page.
- `ReaderController` owns the reader state and keeps page locations/progress in the library model. The QML layer never opens files or writes SQL directly.
- Image, PDF, and comic pages can be rotated through the reader; the normalized rotation is stored with the document and included in library backups.
- `Logging` configures categorized diagnostics without creating a persistent log file or recording document content.
- QML design tokens live in `src/ui/qml/Theme.qml`.

## Invariants

- Original PDFs are read-only unless a later, explicit export operation is confirmed by the user.
- Original documents are not copied, moved, or deleted by a normal library import or removal.
- No network provider is required for the local reading flow.
- Browser lookups are explicit user actions and receive only the selected text.
- New long-running work must not run on the GUI thread; the current import path remains synchronous and is intentionally bounded by format and archive-size limits.
- Database schema changes are additive migrations; historical migration files are not rewritten.
- Technical error details remain in diagnostics; UI messages stay actionable and safe.
