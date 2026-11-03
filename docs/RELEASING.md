# Releasing Lumina Reader

Lumina Reader uses [Semantic Versioning](https://semver.org/):

- `MAJOR` for incompatible changes;
- `MINOR` for backwards-compatible features;
- `PATCH` for backwards-compatible fixes.

Every public release gets a new Git tag in the form `vMAJOR.MINOR.PATCH`. Existing releases are not overwritten.

## Release checklist

1. Choose the next version and update the `project(...)` version in `CMakeLists.txt`.
2. Update `CHANGELOG.md` with the release date and user-visible changes.
3. Build and test the application on the supported platforms.
4. Build the Windows installer when Windows artifacts are part of the release. The Linux AppImage is built by GitHub Actions from the release tag.
5. Update the platform-specific download links in `README.md` to the new tag.
6. Commit the versioned changes and push the release tag. GitHub Actions publishes the Linux AppImage and its workflow artifact; upload any additional platform artifacts to the same release.

## Linux package

Pushing a tag matching `v*` starts `.github/workflows/linux-package.yml`. The workflow installs the pinned Qt kit, builds and tests the application, creates `LuminaReader-linux-x86_64.AppImage`, validates that the package is a non-empty Linux executable, and publishes it to the matching GitHub Release.

The AppImage targets x86_64 Linux systems. Users can download it, run `chmod +x LuminaReader-linux-x86_64.AppImage`, and launch it without installing Qt separately.

The version shown in the application window comes from `LUMINA_VERSION`, which is generated from the CMake project version.

The Windows installer detects the operating system UI language. Portuguese Windows installations use the Portuguese installer interface; other languages use English until an additional translation is added.
