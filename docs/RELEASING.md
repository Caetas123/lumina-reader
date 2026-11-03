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
4. Build the Windows installer and portable package when Windows artifacts are part of the release.
5. Update the download links in `README.md` to the new tag.
6. Commit the versioned changes, create the tag, and publish the GitHub Release with checksums for every artifact.

The version shown in the application window comes from `LUMINA_VERSION`, which is generated from the CMake project version.

The Windows installer detects the operating system UI language. Portuguese Windows installations use the Portuguese installer interface; other languages use English until an additional translation is added.
