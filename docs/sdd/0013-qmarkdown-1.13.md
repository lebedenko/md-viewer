# 0013 — QMarkdown 1.13 upgrade

Status: Complete
Authorization: Owner request to implement the accepted QMarkdown 1.13 upgrade plan, 2026-10-18.

## Problem and outcome

Adopt installed QMarkdown 1.13.0 and move the reproducible dependency preparation/CI baseline from 1.10.0
to the reviewed immutable 1.13.0 revision. Existing reader behavior remains unchanged.

## Scope and non-goals

Update the CMake minimum, QML import, dependency lock and active documentation. Compatible later 1.x packages
remain accepted and 2.x remains explicitly rejected. No application API, configuration format or UI changes.
Highlighting, math and additional code-block controls remain disabled; existing wrapping/copy behavior is retained.
No upstream edits, system installation, dependency acquisition during configure/build/test, commits or publishing.

## Design and decisions

Consume only the installed shared package and public QMarkdown APIs. The existing public style, selection,
GFM and code-copy interfaces remain compatible. Installed package metadata and the pinned archive retain
Qt 6.8 as the minimum. Upstream build defaults retain the optional syntax backend off and bundled math backend
on; these backends do not enable per-view rendering features. No new CI dependencies are required.

The lock pins version 1.13.0, revision `9e07d41da7ecef454a53e8d80a95abb9becc39e9`, and archive SHA-256
`934e0d3dd2dda1a565b10a416f1422ac4c56ed0f80496d672c5688021ca6028c`.
The existing preparation script verifies the checksum and explicitly builds/installs into the project prefix.
Runtime import paths and library RPATH continue to come from the selected installed provider.

## Acceptance criteria

- [x] CMake and QML require 1.13; compatible later 1.x support and explicit 2.x rejection remain.
- [x] Lock records the reviewed revision and independently verified archive checksum.
- [x] Full offline acceptance passes against the installed system package.
- [x] Explicit pinned preparation and fresh consumer build, tests and QML lint pass independently of /usr QMarkdown.
- [x] Runtime library/import selection and documentation consistency verified.
- [x] Native and hosted verification are recorded separately without claiming unperformed checks.

## Tasks

- [x] Review compatibility and update dependency requirements/lock.
- [x] Update active documentation; preserve historical verification records.
- [x] Run system acceptance and explicit offline dependency preparation.
- [x] Verify fresh pinned consumer and record actual results.

## Verification

2026-10-18: SHA-256 of `/tmp/mdv-qmarkdown-1.13-review.tar.gz` matches the lock.
System package metadata reports 1.13.0 and Qt 6.8 minimum; archive build options confirm the retained backend defaults.
Linux, Qt 6.12.0, offscreen/software backend, Fusion Controls:

- Selected the system package explicitly for Debug and Release with
  `cmake --preset debug -DQMarkdown_DIR=/usr/lib/cmake/QMarkdown` and the equivalent Release command.
- `task check` passed: formatting, Debug build, all 44 CTest cases, QML lint, clang-tidy, Release build,
  desktop metadata validation, isolated system/local installation checks, staged installation and installed smoke tests.
- `task deps -- --archive /tmp/mdv-qmarkdown-1.13-review.tar.gz` passed checksum verification and built/installed
  the locked provider into `build/deps/prefix`. Its cache confirms syntax backend OFF and math backend ON.
- Configured a fresh `build/qmarkdown-1.13-consumer` using Ninja, Debug, BUILD_TESTING=ON,
  `CMAKE_PREFIX_PATH` set to the absolute `build/deps/prefix` and `QMarkdown_DIR` set to its
  `lib/cmake/QMarkdown`. Build with `--parallel 2`, `ctest --test-dir build/qmarkdown-1.13-consumer
  --output-on-failure` (44/44), and its `all_qmllint` target all passed.
- `ldd` and `LD_DEBUG=libs` offscreen smoke runs confirm `/usr/lib/libQMarkdown.so.1` for the system build
  and `build/deps/prefix/lib/libQMarkdown.so.1` for the fresh consumer. Compile definitions confirm the corresponding
  `/usr/lib/qt6/qml` and prepared-prefix `lib/qt6/qml` import roots; both startup checks passed.
  QML import tracing produced no output; syscall tracing was unavailable under sandbox ptrace restrictions.
- Changed-document local links resolve and `git diff --check` passed. Active dependency references use 1.13;
  historical 1.10 verification evidence remains intact.

Logs from this run are in `/tmp/mdv-1.13-{check,deps,consumer}.log` (temporary local evidence).
Native visual/input and hosted checks are pending; offscreen automated checks do not establish native usability.

## Open decisions

None.
