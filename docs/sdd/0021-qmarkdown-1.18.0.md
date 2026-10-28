# 0021 — QMarkdown 1.18.0 adoption

Status: Implemented; automated verification passed; native and hosted checks pending
Authorization: Owner requested implementation of the accepted adoption plan on 2026-10-28.

## Problem and outcome

Raise the installed-package contract and reproducible preparation baseline from 1.17.1 to 1.18.0.
Both existing presets select the installed system package, which reports 1.18.0.

## Scope and non-goals

Update CMake, QML imports, the immutable dependency lock and current documentation. Preserve compatible
later 1.x support, explicit 2.x rejection, Qt 6.8 minimum, KDE pins, settings and existing reader behavior.
No optional streaming integration, upstream edits, private APIs, system installation or publication.
Acquisition stays explicit and separate from offline application configure/build/test.

## Design and decisions

Require package 1.18.0 and QML import 1.18, including the test-created style component.
Reviewed the release archive's [1.18.0 notes](https://github.com/lebedenko/qmarkdown/blob/v1.18.0/docs/release-notes-1.18.0.md)
and public package/module declarations. Earlier imports and existing public contracts remain supported;
SOVERSION 1 and dependencies are unchanged. Streaming defaults to false; retained padded-paragraph
rendering is upstream behavior. Native rendering acceptance is separate from automated compatibility.
The release tag resolves to `b22d5196fa5ee94ce6f36dada45bc61c9fa6106d`.
The immutable codeload archive SHA-256 is
`f3aeda9f913c1dbe9220e2a0077c2167ee11af2efbb8b30a2bc750fb9ab0b21b`.

## Acceptance criteria

- [x] CMake minimum, QML imports and immutable lock agree on 1.18.0 / QML 1.18.
- [x] System Debug/Release configuration and full task check pass.
- [x] Explicit locked archive preparation and fresh consumer build, CTest and QML lint pass.
- [x] Library and QML resolution match each selected provider.

## Tasks

- [x] Review release compatibility, resolve tag and hash archive.
- [x] Update baseline and active documentation while preserving historical evidence.
- [x] Run automated checks and record actual results.

## Verification

2026-10-28, Linux, system Qt 6.12.0, Fusion Controls, offscreen/software:

- System version file reports 1.18.0; `/usr/lib/libQMarkdown.so.1` resolves to 1.18.0.
  Explicit upstream tag lookup and archive acquisition completed; SHA-256 computed locally.
- Configured Debug and Release with `cmake --preset debug` / `release` and
  `-DQMarkdown_DIR=/usr/lib/cmake/QMarkdown`. Full `task check` passed: formatting,
  Debug build, 75/75 CTest cases, QML lint, clang-tidy, Release build, desktop validation,
  isolated system/local installation layouts and staged startup checks.
- `task deps -- --archive /tmp/mdv-qmarkdown-1.18.0.tar.gz` passed checksum verification,
  built the locked shared package with math and highlighting enabled, and installed into
  `build/deps/prefix`, using compatible installed KDE. No system installation occurred.
- Fresh Ninja Debug consumer at `build/qmarkdown-1.18.0-consumer`, BUILD_TESTING=ON, with
  absolute CMAKE_PREFIX_PATH and QMarkdown_DIR selecting `build/deps/prefix`: build,
  75/75 CTest cases, `all_qmllint` and offscreen `mdv --smoke-test` passed.
- `ldd` confirms `/usr/lib/libQMarkdown.so.1` for system Debug/Release and the local prefix
  library for the fresh consumer. Runtime QML import tracing confirms system
  `/usr/lib/qt6/qml/QMarkdown/qmldir` and fresh-prefix
  `build/deps/prefix/lib/qt6/qml/QMarkdown/qmldir`, respectively. Both use import 1.18.
- Changed-document local links and `git diff --check` passed.
- Logs: `/tmp/mdv21-config-debug.log`, `/tmp/mdv21-config-release.log`, `/tmp/mdv21-check.log`,
  `/tmp/mdv21-deps.log`, `/tmp/mdv21-consumer.log`, `/tmp/mdv21-system-import.log`,
  `/tmp/mdv21-consumer-import.log` and corresponding `*-ldd.log` files.

Native visual/input and hosted CI remain pending. Local Qt 6.12 automated checks do not establish
hosted Qt 6.8 compatibility or native rendering acceptance. Historical verification is unchanged.

## Open decisions

None for this dependency adoption.
