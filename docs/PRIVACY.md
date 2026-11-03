# Privacy model

Lumina Reader follows a local-first privacy model.

## Defaults

- PDFs remain on the user's device.
- Reading progress, annotations, settings, and search indexes are local.
- There is no telemetry in the foundation.
- No online dictionary, OCR, synchronization, or sharing provider is required.
- Original PDFs are not modified by the application unless an explicit export flow is implemented and confirmed.

## Optional online features

Future online providers must be opt-in, clearly identified, independently disableable, and documented with the exact data sent. A dictionary provider may receive only the selected word or expression when the user explicitly chooses an online provider; it must not receive the PDF or unrelated reading history.

## Diagnostics

Diagnostic logs are categorized and intended for local troubleshooting. They must not contain document contents, passwords, access tokens, or unnecessary personal paths. Any future crash-reporting feature must be opt-in and documented separately.
