# Releasing Lumina Reader

Lumina Reader follows Semantic Versioning. Every public release gets a new tag
in the form `vMAJOR.MINOR.PATCH`; an existing release is never overwritten as a
different version. `project-version.txt` is the single version source for the
application, installer, packages, documentation links, and tag checks.

## Release checklist

1. Choose the next version and update `project-version.txt`.
2. Add the user-visible changes and verification summary to `CHANGELOG.md`.
3. Run the full-engine Windows and Linux configure/build/test gates. Run the
   image-codec tests with real WebP and AVIF fixtures, not only the default
   skipped configuration.
4. Exercise the Windows staged package through the native GUI, including the
   installer, representative format matrix, OCR save/search path, settings,
   history, backup, export, responsive layouts, and error states.
5. Run the Linux build and WSLg smoke check available on the Windows host. Keep
   native macOS evidence tied to the macOS GitHub Actions job; do not infer it.
6. Update the direct download links in `README.md` to the new tag.
7. Review `docs/AUDIT_CHECKLIST_FINAL.md`; every row must be explicitly
   `PASS`, `FAIL`, or `BLOCKED` with evidence. Never turn a blocked platform
   check into a pass by inference.
8. Commit related changes in small thematic Conventional Commits, push `main`,
   then push the matching `v<project-version>` tag.
9. Wait for all tag workflows to finish and verify the release assets, sizes,
   SHA-256 digests, and absence of a Windows ZIP end-user asset.

## Published assets

The tag starts the Windows, Linux, and macOS package workflows. A successful
release contains:

- `LuminaReader-Setup-windows-x64.exe` — graphical self-contained Windows
  installer;
- `LuminaReader-linux-x86_64.AppImage` — direct executable for x86_64 Linux;
- `LuminaReader-linux-x86_64.deb` — Debian/Ubuntu-compatible package;
- `LuminaReader-macos-x86_64.dmg` — Intel macOS DMG.

The Windows ZIP is created only as an internal input to the graphical installer
and must not be uploaded or documented as an end-user download. The Windows
installer embeds the Qt runtime, platform plugins, native document engines,
Tesseract, every model in `tools/tesseract-languages.txt`, and
`tessdata/configs/tsv`, then launches the installed application.

The Linux AppImage and `.deb` copy Tesseract and its non-glibc runtime
dependencies into the deployed application. The application first searches for
`usr/bin/ocr/tesseract` beside itself, so the package does not depend on the
user's `PATH` or a distribution-specific tessdata directory.

## Workflow gates

The Windows and Linux package workflows configure with
`-DLUMINA_REQUIRE_FULL_ENGINES=ON`. Configuration fails if Qt PDF, Qt Speech,
Qt Multimedia, libarchive, DjVuLibre, libwebp, or libavif is missing. The
Windows workflow also adds the vcpkg and DjVu runtime directories to `PATH`
before CTest so the tests exercise the same native DLL set used by packaging.

The Windows workflow stages Tesseract using
`tools/stage-tesseract-windows.ps1`, which validates the executable, every
model in the pinned `tessdata_fast` manifest, and the TSV configuration before
the installer is built. The Linux workflow stages and validates the same
manifest in `packaging/linux/build-appimage.sh`.

The macOS workflow publishes the existing x86_64 core Qt DMG. It uses the same
version/tag but does not claim the Windows/Linux full-engine gate; macOS-native
capabilities and visuals must be proven by its own runner.

## Manual commands

```sh
git diff --check
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit> \
  -DLUMINA_REQUIRE_FULL_ENGINES=ON
cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure
```

On Windows, use an MSVC developer environment and run the staging/installer
scripts documented in [BUILDING.md](BUILDING.md). On Linux, install the native
codec, speech, and Tesseract packages documented there before running the full
gate.

## Release verification

After GitHub Actions finishes, verify the exact tag and assets:

```sh
gh run list --repo Caetas123/lumina-reader --limit 10
gh release view v<version> --repo Caetas123/lumina-reader --json tagName,assets
gh release download v<version> --repo Caetas123/lumina-reader --dir /tmp/lumina-release
sha256sum /tmp/lumina-release/*
```

On Windows use `Get-FileHash -Algorithm SHA256` for the downloaded EXE. Record
the run IDs, asset names, sizes, and digests in the final audit checklist.

The installer detects the Windows UI language: Portuguese Windows uses the
Portuguese installer interface and other languages use English until another
translation is added. End users do not need a command prompt or a portable ZIP.
