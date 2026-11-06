# Localization

Lumina Reader keeps UI localization local to the application. The first launch
uses the Windows or Linux UI locale reported by Qt (`QLocale`) and the choice
can be changed later in **Settings > General > Application language**. The
choice is stored in the local Lumina database and is independent from named
reading profiles.

The graphical Windows installer exposes the same locale selector on its welcome
screen. It preselects the detected Windows UI language, lets the user choose a
different locale before continuing, and carries that choice through every
installation page. Its offline catalog is bundled into the installer, so the
selection is applied before any files are copied and no network connection is
needed during installation.

The selector contains the Windows desktop language-pack and Language Interface
Pack (LIP) locale catalog, including the current regional and script variants
and the Windows compatibility aliases. The same selector and catalog rules are
used by the Windows and Linux builds.

Current catalog policy:

- English remains the source language and Portuguese keeps its reviewed core
  wording.
- The Windows catalog contains 91 internal language/region entries,
  including LIP entries and Windows compatibility aliases. The selector shows
  89 unique user-facing labels: the two Serbian compatibility aliases remain
  available internally for persisted settings, but are not duplicated in the
  menu and are never labelled “legacy”.
- The application and Windows installer share the same compressed, bundled
  offline catalog. Every one of the 91 internal locale entries has its own
  catalog key. Regional and script variants carry explicit regional wording
  overrides where the standard differs (for example Canadian/French,
  Mexican/European Spanish, Catalan/Valencian, Bokmål/Nynorsk, and Serbian
  Latin/Cyrillic/region variants) while preserving the selected locale in
  settings.
- Changing the language calls Qt's QML retranslation path immediately, so a
  restart is not required.
- User-facing quantities use the selected locale for digit shapes, grouping,
  decimal separators, and percent signs. Internal identifiers, file paths,
  database values, and application versions remain invariant for portability.
- Numeric formatting is centralized in `LanguageController` and exposed to QML,
  so the library, reader, search, bookmarks, zoom, reading preferences, and
  speech controls use the same rules across all 91 locale entries.
- Placeholder tokens (`%1`, `%2`, `{0}`, and similar) are protected while the
  catalogs are generated and are checked again during validation.
- Arabic, Persian, Hebrew, Uyghur, and Urdu update the Qt and WinForms layout
  direction together. The application mirrors inherited QML layouts and the
  installer mirrors its shell, controls, text alignment, and language popup.
- Cherokee (`chr-CHER-US`) has a dedicated offline catalog with Cherokee
  syllabary UI wording and installer messages; file-format names, brand names,
  keyboard shortcuts, and placeholders remain unchanged by design.

The compressed resource is regenerated with
`tools/update_localization_catalog.py`. The script normalizes installer
markers before writing the artifact and reports exact duplicate app catalogs,
Cherokee/English equality, and missing `{0}` markers so regional regressions
cannot be hidden by a runtime fallback.
