# Privacy model

Lumina Reader follows a local-first privacy model.

## Defaults

- PDFs remain on the user's device.
- Reading progress, annotations, settings, and search indexes are local.
- There is no telemetry in the foundation.
- No online dictionary, translation, OCR, synchronization, or sharing provider is required for local reading.
- Original PDFs are not modified by the application unless an explicit export flow is implemented and confirmed.

## Optional online features

Online dictionary and translation buttons are explicit user actions. The integrated providers are `dictionaryapi.dev` and `mymemory.translated.net`; they receive only the selected word or passage and the requested language pair. The result is displayed in a local dialog and the lookup history is stored locally. The original document, file path, and unrelated reading history are not sent. Browser fallback actions use only the selected text as well.

## Diagnostics

Diagnostic logs are categorized and intended for local troubleshooting. They must not contain document contents, passwords, access tokens, or unnecessary personal paths. Any future crash-reporting feature must be opt-in and documented separately.
