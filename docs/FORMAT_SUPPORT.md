# Format support and feature parity

This document describes the behavior of the code that is currently shipped. A file extension is not treated as support by itself: Lumina Reader validates the file signature and only exposes a format when the corresponding reader is available in the build.

## Status legend

- **Available**: the format can be imported and opened in the tested build.
- **Conditional**: the feature is available when the optional Qt module or platform provider is included.
- **Partial**: the document opens, but one or more format-specific capabilities are not available yet.
- **Unavailable**: the extension is intentionally not advertised because there is no safe reader in this build.

## Current format matrix

| Format | Import | Open and navigate | Text/search | Highlights, notes, bookmarks, progress | Current limitation or alternative |
| --- | --- | --- | --- | --- | --- |
| EPUB | Available with Qt archive support | Available, following the EPUB spine | Available for XHTML chapters | Available for text locations and the global Knowledge center | CSS layout, embedded fonts, media, and a visual table of contents are not yet reproduced by the native text view |
| PDF | Conditional on `Qt6::Pdf` | Available when the module is present | Page text is indexed and in-document search can find it; visual text selection/highlight overlays are not yet wired to the rendered page | Progress and bookmarks are available; text highlights require a visual selection bridge | Release builds request Qt PDF. A build without it reports a clear capability message |
| MOBI | Available for unencrypted PalmDOC files | Available as paginated text | Available for decoded text | Available for text locations | PalmDOC compression is decoded; DRM and unsupported Huffman/KF8 variants are rejected with a normal reader error |
| AZW | Available for unencrypted PalmDOC-compatible files | Available as paginated text | Available for decoded text | Available for text locations | DRM-protected content is not supported and is never unlocked |
| AZW3 | Available for unencrypted PalmDOC-compatible files | Available as paginated text | Available for decoded text | Available for text locations | Complex KF8 layout/resources and DRM-protected content are not supported |
| FB2 | Available | Available as paginated text | Available after XML text extraction | Available for text locations | Rich FB2 styling and embedded images are not represented in the current reflow view |
| TXT | Available | Available as paginated text | Available | Available for text locations | Encoding detection is UTF-8 first |
| RTF | Available | Available as paginated text | Available after safe plain-text conversion | Available for text locations | Advanced layout, fields, embedded objects, and tracked changes are intentionally not executed |
| CBZ | Available with Qt archive support | Available as ordered image pages | Conditional on Tesseract OCR | Progress, page bookmarks, OCR text, and manual page annotations are available | OCR requires a local Tesseract installation; visual text selection overlays are not included |
| ZIP comics | Available with Qt archive support | Available when the archive contains supported images | Conditional on Tesseract OCR | Progress, page bookmarks, OCR text, and manual page annotations are available | ZIP archives are read-only and never extracted to user-selected paths; OCR requires Tesseract |
| CBR | Unavailable | — | — | — | Requires a RAR decoder; no external extractor is invoked automatically |
| CB7 | Unavailable | — | — | — | Requires a 7z decoder |
| CBT | Unavailable | — | — | — | Requires a tar reader integration |
| RAR | Unavailable | — | — | — | Requires a RAR decoder |
| 7Z | Unavailable | — | — | — | Requires a 7z decoder |
| DJVU/DJV | Unavailable | — | — | — | Requires DjVuLibre or another compatible decoder and OCR integration |
| JPG/JPEG | Available | Available as a single image | Conditional on Tesseract OCR | Progress, OCR text, and manual region annotations are available | OCR requires a local Tesseract installation; text selection uses the recognized text view |
| PNG | Available | Available as a single image | Conditional on Tesseract OCR | Progress, OCR text, and manual region annotations are available | OCR requires a local Tesseract installation; text selection uses the recognized text view |
| WEBP | Available when the Qt image plugin is present | Available as a single image | Not available without OCR | Progress and page bookmarks are available | Depends on the Qt image plugin shipped with the package |
| AVIF | Available when the Qt image plugin is present | Available as a single image | Not available without OCR | Progress and page bookmarks are available | Depends on the Qt image plugin shipped with the package |
| GIF | Available when the Qt image plugin is present | Available as a single image | Not available without OCR | Progress and page bookmarks are available | Animated content is shown through the Qt image pipeline |

## Shared capabilities

The following capabilities are format-independent when the underlying content provides text or a stable page location:

- local library import, duplicate detection by SHA-256, folder import, drag and drop, status, progress, favorites, ratings, descriptions, language, collections, and tags;
- SQLite migrations and FTS5 search for library metadata and saved knowledge;
- persistent highlights, annotations, bookmarks, reading history, and the global Knowledge center;
- Markdown, JSON, and plain-text knowledge export;
- library metadata export and metadata restoration by file hash or saved path;
- local themes, font size, line spacing, high contrast, keyboard-friendly navigation, and optional system text-to-speech;
- explicit browser actions for Google, dictionary lookup, and translation. The document itself is never uploaded automatically.

## Optional capabilities and honest limitations

Qt PDF, Qt Speech, and Qt's private ZIP helper are optional build capabilities. The CI and release workflows request the public Qt PDF and Speech modules. The ZIP helper is used only for in-process, read-only archive access; it is not a general-purpose extraction service.

OCR is provided through a local Tesseract executable when the user installs it; no document is uploaded and the UI falls back to manual region annotations when it is unavailable. Local dictionaries and dedicated RAR/7z and DJVU engines are not silently emulated. PDF text extraction is available through the optional Qt PDF module, while MOBI/AZW support is limited to unencrypted PalmDOC-compatible text. The application keeps unsupported formats out of the picker and explains the limitation instead of claiming support.
