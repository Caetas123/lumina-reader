# Architecture

Lumina Reader is organized into layers so that the QML interface does not own business rules or database connections.

```text
ui (QML and C++ view models)
        |
application (use cases and orchestration)
        |
domain (entities, value types, and rules)
        |
infrastructure       pdf                 platform
(SQLite, files,      (PdfEngine          (OS-specific
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

The current concrete services include `Database`, `LibraryStore`, `ArchiveReader`, `TextDecoder`, `BackupController`, `SettingsController`, and the optional `SpeechController`. `LibraryStore` owns the C++ model exposed to QML; it also owns import validation, duplicate detection, FTS5 updates, collections, tags, favorites, annotations, bookmarks, and reading history.

### `pdf`

Owns the `PdfEngine` and `PdfDocument` interfaces. The UI and application layers should depend on these abstractions rather than on a specific PDF backend.

The current reader uses Qt PDF when `Qt6::Pdf` is available. The dependency remains optional so that the core, text, image, and database tests can still build on a Qt installation without the PDF add-on. The UI reports the missing capability instead of accepting a PDF that cannot be opened.

### `platform`

Contains integrations that differ across Windows, macOS, and Linux, such as file associations, native dialogs, notifications, printing, clipboard, and system voices.

## Current foundation

- `AppController` is the initial QML-facing application coordinator.
- `Database` owns one SQLite connection and applies versioned migrations in transactions.
- Migration versions 1–5 create application settings, documents, reading state, annotations, bookmarks, OCR/provider history tables, FTS5 indexes, favorites, and ratings.
- `LibraryStore` is a `QAbstractListModel`; its view filter is applied in SQL, and its FTS5 queries have a safe LIKE fallback for unusual input.
- `ArchiveReader` reads EPUB and image archives in-process without extracting untrusted paths. It rejects parent/absolute paths, symbolic links, oversized entries, and archives over the configured entry/size limits.
- `TextDecoder` converts TXT, RTF, and FB2 into safe reader text without executing document content.
- `ReaderController` owns the reader state and keeps page locations/progress in the library model. The QML layer never opens files or writes SQL directly.
- `Logging` configures categorized diagnostics without creating a persistent log file or recording document content.
- QML design tokens live in `src/ui/qml/Theme.qml`.

## Invariants

- Original PDFs are read-only unless a later, explicit export operation is confirmed by the user.
- Original documents are not copied, moved, or deleted by a normal library import or removal.
- No network provider is required for the local reading flow.
- Browser lookups are explicit user actions and receive only the selected text.
- Long-running work must not run on the GUI thread.
- Database schema changes are additive migrations; historical migration files are not rewritten.
- Technical error details remain in diagnostics; UI messages stay actionable and safe.
