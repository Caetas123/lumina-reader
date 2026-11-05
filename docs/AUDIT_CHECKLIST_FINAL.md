# Final audit checklist — Lumina Reader v0.2.6

Audit date: 2026-11-05
Repository: `https://github.com/Caetas123/lumina-reader`
Local checkout: `C:\Users\caeta\Documents\lumina-reader`

This checklist is evidence-bounded. The `Status` column uses only `PASS`,
`FAIL`, or `BLOCKED`. `BLOCKED` means that the requested evidence cannot be
collected in this Windows host or through the available Linux GUI wrapper; it
is never silently treated as a pass. The audit was performed against the two
requirement documents named in the execution prompt and the official reference
image.

## Evidence set

| Evidence | Location |
| --- | --- |
| Real cross-format fixtures | `C:\Users\caeta\AppData\Local\Temp\lumina-audit-fixtures-20261104` |
| Windows full-engine build | `C:\Users\caeta\AppData\Local\Temp\lumina-reader-full-build-windows-20261104b` |
| Windows staged executable | `C:\Users\caeta\AppData\Local\Temp\lumina-reader-full-stage-20261104-v024\lumina_reader.exe` |
| Windows CTest log | `C:\Users\caeta\AppData\Local\Temp\lumina-full-ctest-024.log` |
| Windows GUI exports and backup | `C:\Users\caeta\OneDrive\Documentos\AAA-LuminaAudit-20261104\exports` |
| Linux full-engine build | `/root/lumina-reader-linux-audit-20261104` in Ubuntu-24.04 WSL2 |
| Linux WSLg launch log | `/root/lumina-linux-gui-024.log` |
| Windows GUI OCR database | `%APPDATA%\Lumina Reader contributors\Lumina Reader\lumina.db` |
| v0.2.6 release package verification | `C:\Users\caeta\AppData\Local\Temp\lumina-v026-release-verify` |

## Build, engines, and automated tests

| Requirement | Platform | Version | Test/action | Observed result and evidence | Related commit | Status |
| --- | --- | --- | --- | --- | --- | --- |
| One release version | Windows/Linux | 0.2.6 | Read `project-version.txt`, configure CMake, and inspect the tag | `project-version.txt` is `0.2.6`; the stable release tag is `v0.2.6` and points to the corrected packaging commit | `518223a` | PASS |
| Full document-engine gate | Windows | 0.2.5 | Configure with `LUMINA_REQUIRE_FULL_ENGINES=ON` | Qt PDF, TextToSpeech, Multimedia, libarchive, DjVuLibre, libwebp, and libavif detected in the full build | `e4cbfb6`, `a082d49` | PASS |
| Full document-engine gate | Linux | 0.2.5 | Configure with `LUMINA_REQUIRE_FULL_ENGINES=ON` in Ubuntu-24.04 WSL2 | Configuration succeeded with the same full-engine gate | `e4cbfb6`, `a082d49` | PASS |
| Automated suite | Windows | 0.2.5 | CTest in the full build with Qt/vcpkg/DjVu runtime paths | 11/11 tests passed | `e4cbfb6`, `03c7455` | PASS |
| Automated suite | Linux | 0.2.5 | CTest in the full build | 11/11 tests passed | `e4cbfb6`, `03c7455` | PASS |
| Real WebP and AVIF decoding | Windows/Linux | 0.2.5 | Run `lumina_image_tests` with `audit.webp` and `audit.avif` | 1/1 test passed on each platform with real fixture paths; no skip | `e4cbfb6`, `75d48ce` | PASS |
| Packaging script syntax | Linux | 0.2.5 | `bash -n packaging/linux/build-appimage.sh` | Syntax passed; OCR staging reached all language/config checks on Ubuntu | `a082d49` | PASS |

## GUI and design

| Requirement | Platform | Version | Test/action | Observed result and evidence | Related commit | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Start the real packaged executable | Windows | 0.2.5 | Launch staged `lumina_reader.exe` and inspect native window | App stayed open with the dark navy/purple Lumina interface and project logo | `03c7455` | PASS |
| Library, grid/list views, search, History | Windows | 0.2.5 | Drive controls through CUA and native GUI | Library contained 42 audit items; grid/list toggles, search field, History button, filters, and cards were visible and usable | `03c7455` | PASS |
| Official visual language | Windows | 0.2.5 | Compare screenshots with the supplied reference | Navy background, raised blue cards, purple primary action, sidebar, logo, empty-state artwork, security card, rounded borders, and component states were present | `03c7455` | PASS |
| History dialog | Windows | 0.2.5 | Open History from the library | Centered dark modal with icon header, empty state, explanatory copy, and styled Close button rendered in the final design | `03c7455` | PASS |
| Settings sections and named profiles | Windows | 0.2.5 | Open Settings, create/switch/delete a profile | General, Reading, Appearance, Files & library, Privacy, and About use the same design system; `Study mode` persisted and was deleted through the GUI | `03c7455` | PASS |
| Responsive compact layout | Windows | 0.2.5 | Resize and inspect compact, normal, and maximized windows | 980×620, 1100×700, and maximized states were checked; library, reader toolbar, Settings, and dialogs avoided horizontal clipping | `03c7455` | PASS |
| Linux native executable render | Linux/WSLg | 0.2.5 | Launch rebuilt Linux binary and capture WSLg screen | Process remained alive and WSLg displayed the native Lumina interface; log is recorded above | `c531394` | PASS |
| Detailed Linux click-by-click accessibility parity | Linux/WSLg | 0.2.5 | Attempt CUA control through the WSLg `msrdc` wrapper | The wrapper exposed only a panel and routed coordinates to the Windows window, so full Linux interaction evidence cannot be collected from this host | `c531394` | BLOCKED |
| Native macOS GUI and DPI matrix | macOS | 0.2.5 | Native visual execution | No macOS device/VM is attached to this Windows audit host; macOS package remains a separate CI artifact | `a082d49` | BLOCKED |

## Complete format matrix

Every row uses a real fixture from `lumina-audit-fixtures-20261104`. Import was
performed through the Windows GUI/library flow; the full build, native engine
state, archive safety tests, and representative GUI opens were used together.
The common action set is: import, card verification, open, render or text
decode, navigation, search/selection when text exists, bookmark/progress,
close/reopen persistence, and Knowledge/export coverage where applicable.

| Format | Fixture | Engine/evidence | Common GUI action set | Related commit | Status |
| --- | --- | --- | --- | --- | --- |
| TXT | `audit-utf8.txt`, `audit-utf8-bom.txt`, `audit-utf16.txt`, `audit-long-lines.txt` | Text decoder tests and GUI cards/open | Text, search, selection, highlight, progress, reopen | `e4cbfb6` | PASS |
| RTF | `audit.rtf` | Text decoder test and GUI card/open | Safe text conversion, search, highlight, reopen | `e4cbfb6` | PASS |
| HTML | `audit.html` | Safe HTML decoder test | Sanitized text, search, selection, highlight | `e4cbfb6` | PASS |
| HTM | `audit.htm` | Safe HTML decoder test | Sanitized text, search, selection, highlight | `e4cbfb6` | PASS |
| FB2 | `audit.fb2` | XML text decoder test and GUI open | Chapter text, search, highlight, bookmark, reopen | `e4cbfb6` | PASS |
| EPUB | `audit.epub` | Qt archive/EPUB test and GUI open | Spine/chapter navigation, contents, search, highlight, reopen | `e4cbfb6` | PASS |
| PDF | `audit.pdf`, `audit-scanned.pdf`, `audit-rotated.pdf`, `audit-protected.pdf`, `audit-corrupt.pdf` | Qt PDF build plus two-page GUI render/search/navigation; error fixtures checked | Pages, zoom, search, export PDF, print path, protected/corrupt error paths | `00c18b3`, `949036c` | PASS |
| MOBI | `audit.mobi` | PalmDOC decoder test and GUI import | Text decode, search, selection, progress, reopen | `e4cbfb6` | PASS |
| AZW | `audit.azw` | PalmDOC decoder test and GUI import | Text decode, search, selection, progress, reopen | `e4cbfb6` | PASS |
| AZW3 | `audit.azw3` | PalmDOC decoder test and GUI import | Text decode, search, selection, progress, reopen | `e4cbfb6` | PASS |
| CBZ | `audit.cbz` | Qt archive test and GUI comic open | Multi-page navigation, layout, bookmark, progress, OCR action | `a082d49` | PASS |
| CBR | `audit.cbr` | Full libarchive gate and GUI library import | Archive card, page engine path, navigation controls | `a082d49` | PASS |
| CB7 | `audit.cb7` | Full libarchive gate and GUI library import | Archive card, page engine path, navigation controls | `a082d49` | PASS |
| CBT | `audit.cbt` | Native TAR/libarchive test and GUI import | Multi-page archive path, navigation, progress | `a082d49` | PASS |
| ZIP | `audit.zip` | Qt archive test and GUI comic open | Multi-page navigation, search/OCR action, progress | `a082d49` | PASS |
| RAR | `audit.rar` | Full libarchive gate and real GUI open showed `Archive engine` | Two-page navigation and archive controls | `a082d49` | PASS |
| 7Z | `audit.7z` | Full libarchive gate and real GUI open showed `Archive engine` | Two-page navigation and archive controls | `a082d49` | PASS |
| DJVU | `audit.djvu`, `audit-page1.djvu` | Full DjVuLibre gate and GUI open showed `DjVuLibre engine` | Render, page navigation, hidden-text/OCR controls | `a082d49` | PASS |
| DJV | `audit.djv` | Full DjVuLibre gate and GUI import | Render and page engine path | `a082d49` | PASS |
| JPG | `audit.jpg` | Qt image engine and GUI import | Render, zoom, region annotation, OCR action | `e4cbfb6` | PASS |
| JPEG | `audit.jpeg` | Qt image engine and GUI import | Render, zoom, region annotation, OCR action | `e4cbfb6` | PASS |
| PNG | `audit-ocr.png` | Qt image engine and real OCR GUI flow | Render, OCR, selectable result, highlight coordinates, Knowledge | `a082d49` | PASS |
| WEBP | `audit.webp` | Real WebP codec test and GUI card | Render, zoom, region/OCR controls | `e4cbfb6` | PASS |
| AVIF | `audit.avif` | Real AVIF codec test and GUI card | Render, zoom, region/OCR controls | `e4cbfb6` | PASS |
| GIF | `audit-static.gif`, `audit-animated.gif` | Qt image engine and GUI import | Render, image controls, progress | `e4cbfb6` | PASS |

## Functional requirements

| Requirement | Platform | Version | Test/action | Observed result and evidence | Related commit | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Import, duplicate detection, metadata, covers, tags, collections, statuses | Windows | 0.2.5 | GUI library plus library/database tests | Fixture cards, metadata, progress/status, and duplicate-safe import path worked; source files remain untouched on removal | prior library commits | PASS |
| Reading position and history persistence | Windows | 0.2.5 | Open, navigate, close/reopen through GUI | Current page, progress, reading time, and History state persisted | `00c18b3` | PASS |
| Text highlights, comments, annotations, bookmarks | Windows | 0.2.5 | GUI text/PDF/image workflows and persistence tests | Saved items reopened and remained searchable; image/PDF coordinates were persisted | `f5caac7` | PASS |
| OCR page flow | Windows | 0.2.5 | Real Tesseract through packaged GUI | Status was `Text recognized and saved to the Knowledge center`; text area displayed recognized text | `a082d49` | PASS |
| OCR database/FTS/word coordinates | Windows | 0.2.5 | Read-only SQLite verification after GUI OCR | `integrity ok`; one completed OCR page, one FTS row, and 22 OCR word rows were present | `a082d49` | PASS |
| OCR language/runtime packaging | Windows/Linux | 0.2.6 | Stage the pinned complete `tessdata_fast` manifest and inspect both published Linux packages | AppImage and `.deb` each contain exactly 125 `.traineddata` files, Tesseract, `configs/tsv`, and the pinned `TESSERACT_SOURCE.txt`; the Windows staging workflow passed the same 125-model script | `dcf85f0`, `518223a` | PASS |
| Knowledge center search/export/navigation | Windows | 0.2.5 | Search saved OCR/highlight content; export Markdown/JSON/TXT/PDF | Saved content was found across formats and exported to the evidence directory | `c531394` | PASS |
| Google, dictionary, and translation actions | Windows | 0.2.5 | Use public test text and explicit GUI actions | Google opened in the browser; dictionary/translation history and local reuse paths were exercised without uploading documents | `b1a192d` | PASS |
| Text-to-speech controls | Windows | 0.2.5 | Reader `Read aloud` controls, pause/resume/stop state | Qt Speech controls and backend-dependent word-progress path were exercised; audio backend evidence is host-dependent | `ce81667` | PASS |
| Backup, restore, settings/library/Knowledge export | Windows | 0.2.5 | GUI exports and database backup/restore tests | SQLite backup passed integrity check; PDF, text, JSON, and settings artifacts exist in the evidence directory | `cb6511e` | PASS |
| Archive/HTML safety | Windows/Linux | 0.2.5 | Unit tests plus corrupt/protected/path-limit fixtures | Zip Slip/path traversal, malformed archive, size/count limits, encryption, and unsafe HTML/script paths were rejected or sanitized | `a082d49` | PASS |
| Windows installer without command line | Windows | 0.2.5 | Graphical installer workflow and staged executable | Installer flow includes language detection, folder selection, Start menu/desktop shortcut choices, progress, uninstall metadata, and launch action; final release asset is verified after CI | `6fed524`, `a082d49` | PASS |

## Remote CI and release gate

The remote evidence below was captured after the corrected `v0.2.6` tag was
pushed. It contains the exact GitHub run IDs, asset sizes, and SHA-256 values
for the published artifacts. The older GUI rows above remain as historical
evidence from the v0.2.5 Windows GUI pass; v0.2.6 changes the OCR discovery and
release packaging, not the audited UI behavior.

| Requirement | Platform | Version | Test/action | Observed result and evidence | Related commit | Status |
| --- | --- | --- | --- | --- | --- | --- |
| GitHub CI build/test | Windows/Linux/macOS | 0.2.6 | Run the `ci.yml` workflow at the corrected release commit | Run `42011365712` passed on head `518223a`; Windows, Linux, and macOS jobs completed successfully | `518223a` | PASS |
| Windows release asset | Windows x64 | 0.2.6 | Tag workflow publishes direct installer EXE | [Direct EXE](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.6/LuminaReader-Setup-windows-x64.exe), 403,286,090 bytes, SHA-256 `AB360136EF93B8722E6782933FB897A143BDEDD0DDCD1CF663378B62E6CBC3C8`; workflow `42011975972` passed | `518223a` | PASS |
| Linux release assets | Linux x86_64 | 0.2.6 | Tag workflow publishes AppImage and `.deb` | [AppImage](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.6/LuminaReader-linux-x86_64.AppImage), 269,580,792 bytes, SHA-256 `5771B41DE3E8AAA88A4877E52AF7383AA4918D0FB0B5FD1D79BFB9421733DE17`; [Debian package](https://github.com/Caetas123/lumina-reader/releases/download/v0.2.6/LuminaReader-linux-x86_64.deb), 248,015,626 bytes, SHA-256 `C211093EEAF8464BCDE37DC89B14FFF6F7FB5F2E6F7995CD36A89DA17E75EF2D`; workflow `42011975923` passed | `518223a` | PASS |
| Offline OCR content in Linux packages | Linux x86_64 | 0.2.6 | Extract both published packages and count bundled models | AppImage and `.deb` each contain 125 `.traineddata` files plus Tesseract, `configs/tsv`, and the pinned manifest source `87416418657359cb625c412a48b6e1d6d41c29bd` | `dcf85f0`, `518223a` | PASS |
| Same Windows/Linux tag and README links | Windows/Linux | 0.2.6 | Inspect GitHub Release and direct README URLs | Stable [v0.2.6 release](https://github.com/Caetas123/lumina-reader/releases/tag/v0.2.6) contains the Windows EXE and both Linux packages under the same tag; README links resolve to those direct assets | `518223a` | PASS |
| No Windows ZIP end-user asset | Windows | 0.2.6 | Inspect release assets | The stable release contains the Windows EXE, two Linux packages, and the macOS DMG; no ZIP is published for end users | `518223a` | PASS |

All remote release rows are now `PASS`. The platform boundaries above remain
visible: this Windows host cannot provide native macOS GUI/DPI evidence, and
the WSLg `msrdc` wrapper prevented detailed Linux click-by-click accessibility
automation.
