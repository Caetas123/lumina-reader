# Building Lumina Reader

## Requirements

- C++20 compiler;
- CMake 3.24 or newer;
- Qt 6.5 or newer with `Core`, `Gui`, `Qml`, `Quick`, `QuickControls2`, `Sql`, and `Test`;
- Ninja for the checked-in default preset.

The project does not require a server, browser runtime, JavaScript engine, or network service to configure and test the foundation.

## Standard build

Run from the repository root:

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

The default preset creates a local `build/default` directory, which is ignored by Git.

## Qt discovery

If CMake cannot find Qt, point it at the Qt kit prefix. The exact path depends on the operating system and compiler kit:

```sh
cmake --preset default -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit>
```

For Windows, use a Developer Command Prompt that provides the MSVC environment before configuring. For macOS, select the intended Xcode command-line tools and Qt kit. For Linux, install the compiler, Ninja, OpenGL development packages required by Qt, and the Qt development modules.

## Tests

The current tests are Qt Test executables registered with CTest:

- `lumina_smoke_tests` checks that the application version is defined;
- `lumina_database_tests` checks SQLite migration, FTS5 table creation, idempotence, and invalid input handling.

Tests use an in-memory SQLite database and do not use the internet or user documents.

## Local development data

The running application stores its SQLite database under Qt's `AppDataLocation`. This data is local runtime state and must not be committed. Delete it only through an explicit user action or a platform-appropriate data-management workflow; do not add deletion commands to build scripts.
