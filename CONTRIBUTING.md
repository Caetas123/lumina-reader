# Contributing to Lumina Reader

Thank you for helping improve Lumina Reader. Contributions should preserve the project's local-first, private, native, accessible, and maintainable design.

## Before starting

1. Read [README.md](README.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), and [docs/BUILDING.md](docs/BUILDING.md).
2. Search existing issues and pull requests before opening a new one.
3. Keep user documents, generated builds, logs, credentials, and local database files out of the repository.
4. For security-sensitive issues, follow [SECURITY.md](SECURITY.md) instead of opening a public issue.

## Development workflow

1. Create a focused branch from the current default branch.
2. Make one coherent change at a time.
3. Keep business logic out of QML and database access out of views.
4. Add or update tests for behavior changes.
5. Update documentation when behavior, build requirements, or public interfaces change.
6. Run the relevant test suite and build locally.
7. Run `git diff --check` and review the complete diff before committing.
8. Open a pull request with a concise summary, testing notes, limitations, and follow-up work.

## Commit messages

Use English [Conventional Commits](https://www.conventionalcommits.org/):

```text
<type>(<scope>): short description
```

Allowed types are `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `build`, `ci`, `chore`, `style`, and `revert`. Keep commits small, reviewable, and compilable whenever practical. Do not squash or rewrite existing commits without agreement from the repository owners.

## Pull requests

Pull requests should explain:

- what changed and why;
- how the change was tested;
- which platforms were verified;
- known limitations or deferred work;
- any migration, privacy, accessibility, or compatibility impact.

Do not add placeholder controls, undocumented network calls, telemetry, or automatic changes to original PDF files.

## Code style

- Use C++20 and the Qt 6 APIs already established by the project.
- Prefer clear names and small classes over broad global state.
- Keep long-running work off the UI thread.
- Treat PDFs and imported paths as untrusted input.
- Return user-safe messages to the UI and retain technical details in diagnostics.
- Avoid adding dependencies without documenting their license and maintenance rationale.
