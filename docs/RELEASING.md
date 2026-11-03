# Releasing Lumina Reader

Lumina Reader uses [Semantic Versioning](https://semver.org/):

- `MAJOR` for incompatible changes;
- `MINOR` for backwards-compatible features;
- `PATCH` for backwards-compatible fixes.

Every public release gets a new Git tag in the form `vMAJOR.MINOR.PATCH`. Existing releases are not overwritten.

## Release checklist

1. Choose the next version and update the `project(...)` version in `CMakeLists.txt` and the Windows installer metadata.
2. Update `CHANGELOG.md` with the release date and user-visible changes.
3. Build and test the application on the supported platforms.
4. Build the Windows installer with `packaging/windows/build-installer.ps1` when Windows artifacts are part of the release. The Linux AppImage and macOS DMG are built by GitHub Actions from the release tag.
5. Update the platform-specific download links in `README.md` to the new tag.
6. Commit the versioned changes and push the release tag. GitHub Actions publishes the Linux AppImage, macOS DMG, Windows installer, and their workflow artifacts to the matching release.

## Linux package

Pushing a tag matching `v*` starts `.github/workflows/linux-package.yml`, `.github/workflows/macos-package.yml`, and `.github/workflows/windows-package.yml`. The workflows install the pinned Qt kit, build and test the application, and publish direct executable assets to the matching GitHub Release:

- `LuminaReader-linux-x86_64.AppImage` for Linux;
- `LuminaReader-macos-x86_64.dmg` for macOS;
- `LuminaReader-Setup-windows-x64.exe` for Windows.

The Windows ZIP is only an internal input to the graphical installer and is not listed as an end-user download. The installer embeds the application, Qt runtime, and platform plugins, then launches the installed executable.

The AppImage targets x86_64 Linux systems. Users can download it, run `chmod +x LuminaReader-linux-x86_64.AppImage`, and launch it without installing Qt separately.

The macOS package targets Intel x86_64 Macs and contains a self-contained `.app` bundle inside the DMG. Apple Silicon users can run it through Rosetta 2; a native ARM64 package is not published yet.

The version shown in the application window comes from `LUMINA_VERSION`, which is generated from the CMake project version.

The Windows installer detects the operating system UI language. Portuguese Windows installations use the Portuguese installer interface; other languages use English until an additional translation is added. The installer embeds the packaged application and launches the installed `lumina_reader.exe`, so end users do not need a command prompt or a portable ZIP.
