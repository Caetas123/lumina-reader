# Format and feature support

This is the user-facing capability matrix for Lumina Reader `v0.2.7`.
Windows and Linux release builds are configured with the full-engine gate and
ship the native engines and OCR runtime required by the table. Development
builds may intentionally omit those dependencies; the picker then hides the
unavailable formats instead of pretending that an extension is enough.

## Supported formats

| Format | Release reader | Search and text | Notes |
| --- | --- | --- | --- |
| TXT | Built in | Yes | UTF-8, BOM, UTF-16, and common fallback decoding are handled. |
| RTF | Built in | Yes | Safe plain-text conversion; advanced layout and embedded objects are not executed. |
| HTML / HTM | Built in | Yes | Safe local text conversion; scripts, styles, frames, objects, and embeds are removed. |
| FB2 | Built in | Yes | XML text is extracted into the native reading view. |
| EPUB | Qt archive engine | Yes for extracted chapter text | The Contents panel follows the EPUB spine. CSS pagination, embedded media, and complex layout are not reproduced. |
| PDF | Qt PDF | Extracted text or OCR | Scanned pages can use OCR. Visual pages support zoom, rotation, single/two-page layouts, export, and printing. |
| MOBI / AZW / AZW3 | PalmDOC decoder | Yes for decoded text | Unencrypted PalmDOC-compatible files are supported; DRM and complex KF8 resources are not unlocked. |
| CBZ / ZIP comic | Qt archive engine | OCR for image pages | Read-only archives containing supported image pages. ZIP files containing only arbitrary text are rejected as comic documents. |
| CBR / CB7 / CBT | libarchive | OCR for image pages | Read-only in-process archive access with path, count, size, and encryption safety limits. |
| RAR / 7Z | libarchive | OCR for image pages | Release builds include the reader; encrypted or malformed entries are rejected. |
| DJVU / DJV | DjVuLibre | Hidden text or OCR | Native page rendering and hidden-text extraction are available in the full release build. |
| JPG / JPEG | Qt image pipeline | OCR and image regions | Single visual page with saved regions and OCR coordinate overlays. |
| PNG | Qt image pipeline | OCR and image regions | Single visual page with saved regions and OCR coordinate overlays. |
| WEBP | libwebp fallback plus Qt image pipeline | OCR and image regions | The full release build verifies the native WebP decoder and ships its runtime. |
| AVIF | libavif fallback plus Qt image pipeline | OCR and image regions | The full release build verifies the native AVIF decoder and ships its runtime. |
| GIF | Qt image pipeline | OCR and image regions | Static and animated images are accepted; animation is presented through Qt's image pipeline. |

## Shared reading capabilities

When the source provides text or a stable page location, the application
supports:

- library import, duplicate detection, folder import, drag and drop, optional
  folder watching, status, progress, favorites, ratings, descriptions,
  language, genre, collections, tags, and reading history;
- persistent highlights, annotations, bookmarks, notes, OCR text, normalized
  OCR word coordinates, and the Knowledge center backed by SQLite FTS5;
- local search history for the library, document, and Knowledge center, with
  deduplication, reopening, favorites, and clearing;
- PDF and plain-text export, system printing, Markdown/JSON/plain-text
  Knowledge export, library metadata export, settings export, and validated
  database backup/restore;
- dark, light, sepia, paper, gray, low-light, and high-contrast themes;
  responsive layouts, keyboard focus, font/layout/color preferences, and
  named reading profiles;
- contextual Google, dictionary, and translation actions. Online requests are
  explicit user actions; saved results and lookup history remain local;
- Qt Speech read-aloud controls. Word-level highlighting is available when the
  selected operating-system voice backend exposes progress callbacks.

## OCR

Windows and Linux release packages include Tesseract and 125 official
`tessdata_fast` data files (123 selectable language/script models), pinned to the commit recorded in
`tools/tessdata-fast-ref.txt`, plus orientation detection and Tesseract's
`configs/tsv` file required for word-coordinate parsing. The UI discovers the
models installed beside the executable instead of exposing a hard-coded list.
OCR runs locally and asynchronously; no page image or OCR result is sent to an
online provider. Recognized text, word boxes, and page status are persisted in
the local database and indexed in Knowledge. The UI also supports cancellation
and a manual region annotation fallback. Google Translate's translation list
is not an OCR model list, so a language without an official local Tesseract
model cannot be promised as offline OCR support.

## Platform and build boundaries

- Windows x64 publishes a self-contained graphical installer EXE. The ZIP
  created by the workflow is only an internal installer input and is not an
  end-user release asset.
- Linux x86_64 publishes a self-contained AppImage and a Debian/Ubuntu `.deb`.
- macOS publishes the existing x86_64 DMG workflow. The macOS workflow does
  not run the Windows/Linux full-engine gate, so native macOS capability and
  visual evidence must be read from its own CI run rather than inferred from
  the Windows/Linux audit.
- All public packages use one tag and the version in `project-version.txt`.
- The original documents are never modified implicitly. Archives are read-only,
  normalized paths and resource limits are enforced, and unsupported or
  encrypted content produces a user-facing error.

See [BUILDING.md](BUILDING.md) for dependency installation and
[AUDIT_CHECKLIST_FINAL.md](AUDIT_CHECKLIST_FINAL.md) for evidence-backed
validation results.
