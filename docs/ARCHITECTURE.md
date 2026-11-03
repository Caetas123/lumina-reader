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

### `pdf`

Owns the `PdfEngine` and `PdfDocument` interfaces. The UI and application layers should depend on these abstractions rather than on a specific PDF backend. Qt PDF/PDFium integration will be added behind this boundary in a later phase.

### `platform`

Contains integrations that differ across Windows, macOS, and Linux, such as file associations, native dialogs, notifications, printing, clipboard, and system voices.

## Current foundation

- `AppController` is the initial QML-facing application coordinator.
- `Database` owns one SQLite connection and applies versioned migrations in transactions.
- Migration version 1 creates application settings storage and the FTS5 search foundation.
- `Logging` configures categorized diagnostics without creating a persistent log file or recording document content.
- QML design tokens live in `src/ui/qml/Theme.qml`.

## Invariants

- Original PDFs are read-only unless a later, explicit export operation is confirmed by the user.
- No network provider is required for the local reading flow.
- Long-running work must not run on the GUI thread.
- Database schema changes are additive migrations; historical migration files are not rewritten.
- Technical error details remain in diagnostics; UI messages stay actionable and safe.
