# Third-party notices

Lumina Reader is designed to use third-party libraries without changing their licensing terms. This file is a starting point for the dependency inventory and must be updated whenever a dependency is added, removed, or redistributed.

## Qt 6

The application uses Qt 6 modules for the native UI, application services, SQL integration, and testing. Qt is available under licenses that may include the GNU Lesser General Public License, the GNU General Public License, or a commercial Qt license depending on the modules, version, and distribution context.

Consult the official [Qt licensing page](https://www.qt.io/licensing/) and the license files shipped with the exact Qt package used for a build. Do not assume that this project's MIT license changes Qt's license.

## SQLite

SQLite is used through Qt's SQL integration. SQLite is public domain; distribution and build notices must still follow the exact Qt and platform packaging requirements.

## Optional Qt modules

- `Qt6::Pdf` supplies PDF document loading and page rendering in CI/release builds.
- `Qt6::TextToSpeech` supplies the system text-to-speech adapter when the platform has a compatible voice backend.
- `Qt6::CorePrivate` supplies Qt's read-only `QZipReader` helper for EPUB, CBZ, and ZIP packages. It is a private Qt API and may change between Qt versions; builds without it disable those archive readers rather than silently substituting an extractor.

No RAR, 7z, DjVu, OCR, online dictionary, translation, or telemetry dependency is bundled by the current repository. Browser lookup actions are initiated by the user and do not create a project-owned network service.

## Inventory rule

Before adding a dependency:

1. record its name, version, source, and license;
2. verify compatibility with the MIT project license and the intended platform packages;
3. document whether it is build-time, runtime, optional, or bundled;
4. preserve the upstream license and notices.
