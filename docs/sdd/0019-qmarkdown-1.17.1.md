# 0019 — QMarkdown 1.17.1 adoption

Status: Implemented; automated acceptance passed; native and hosted checks pending
Authorization: Owner requested adoption of installed QMarkdown 1.17.1 on 2026-10-22.

## Problem and outcome

Move the installed-package contract and reproducible preparation baseline from 1.14.0 to 1.17.1.
The provider now supplies hybrid text rendering and the built-in highlighting initialization fix.

## Scope and non-goals

Update CMake, QML imports, the dependency lock and current documentation. Keep compatible later 1.x
acceptance, explicit 2.x rejection, Qt 6.8 minimum, KDE pins and existing reader behavior.
No upstream edits, private APIs, new settings, inline math, footnotes or custom highlighting provider integration.
Acquisition remains explicit and separate from offline configure/build/test.

## Design and decisions

Require package 1.17.1 and QML module 1.17 (QML imports have no patch component).
Reviewed the published archive's release notes for 1.17.0 and 1.17.1: existing public contracts and
SOVERSION 1 remain compatible; new features are opt-in. Hybrid drawing is selected upstream automatically.
The immutable release revision is `da77e3dc00a1b8e16dd2c25e8b519eb34f61191a`, with archive SHA-256
`2c76aeba074e1a2362244ddf5b74eab7d5061b4f4606832971c58848a1201e8d`.
See [upstream notes](https://github.com/lebedenko/qmarkdown/blob/v1.17.1/docs/release-notes-1.17.1.md).
This adoption follows the historical investigation in [0018](0018-text-rendering-corruption.md);
it does not convert historical or upstream evidence into viewer native visual acceptance.

## Acceptance criteria

- [x] Package minimum, QML imports and immutable preparation lock agree on the new baseline.
- [x] Existing consumer tests, QML lint and full task check pass with installed 1.17.1.
- [x] Locked archive preparation and a fresh consumer build pass.
- [ ] Native visual/input review and hosted CI run (separate from automated local acceptance).

## Tasks

- [x] Review release compatibility and update dependency files and active documentation.
- [x] Run automated acceptance and record actual outcomes.

## Verification

The installed `/usr/lib/cmake/QMarkdown/QMarkdownConfigVersion.cmake` reports 1.17.1.
Published tag lookup resolved the revision above; explicitly downloaded its codeload archive and
computed the recorded SHA-256. Native visual/input review and hosted CI have not been run.

2026-10-22, Linux, system Qt 6.12.0, Fusion Controls, offscreen/software:

- Configured Debug and Release with `-DQMarkdown_DIR=/usr/lib/cmake/QMarkdown`.
  `task check` passed: formatting, Debug build, 68/68 CTest cases, QML lint, clang-tidy,
  Release build, desktop validation, isolated system/local installation layouts and staged startup checks.
- `task deps -- --archive /tmp/mdv-qmarkdown-1.17.1.tar.gz` verified the checksum and built/installed
  the locked shared package into `build/deps/prefix`, with math and highlighting enabled and installed KDE.
- Fresh Ninja Debug consumer at `build/qmarkdown-1.17.1-consumer`, BUILD_TESTING=ON, with absolute
  CMAKE_PREFIX_PATH and QMarkdown_DIR selecting `build/deps/prefix`: build, 68/68 CTest cases,
  `all_qmllint` and offscreen `mdv --smoke-test` passed.
- `ldd` confirms `/usr/lib/libQMarkdown.so.1` for Debug and the local prefix library for the fresh consumer.
- Changed-document local links and `git diff --check` passed.
- Logs: `/tmp/mdv19-check.log`, `/tmp/mdv19-deps.log`, `/tmp/mdv19-consumer.log`.
  These local Qt 6.12 checks do not establish hosted Qt 6.8 or native visual/input acceptance.

## Open decisions

None for this dependency upgrade.
