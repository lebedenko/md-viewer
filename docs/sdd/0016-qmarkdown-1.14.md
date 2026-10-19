# 0016 — QMarkdown 1.14 adoption

Status: Implemented; automated acceptance passed; native and hosted checks pending
Authorization: Owner request to implement the accepted QMarkdown 1.14 upgrade plan, 2026-10-19.

## Problem and outcome

Adopt QMarkdown 1.14.0 so eligible native formulas remain rendered during selection and copy their TeX body.
Active Find exposes literal TeX source and preserves exact selection endpoints when closed.
Unordered task indicators replace bullets, while ordered tasks retain numbers.

## Scope and non-goals

Upgrade the installed shared provider contract and immutable preparation lock. Keep Qt 6.8 minimum,
compatible later 1.x acceptance, explicit 2.x rejection, existing KDE pins and enabled backends.
No settings migration, table styling changes, upstream edits, private APIs, editor or Vim behavior.
Dependency acquisition remains explicit and separate from offline configure/build/test.

## Design and decisions

ReaderPane enables public `taskIndicatorsReplaceBullets` and retains the decorative Controls adapter from
[0015](0015-reader-rendering-polish.md). Upstream owns marker geometry and ordered numbering.
Its observing WheelHandler uses `blocking: false` and leaves events unaccepted, retaining manual-scroll tracking.
Toolbar, Settings, Find help and README describe the new math behavior; host-renderer fallbacks remain upstream.
CMake and QML require 1.14. The lock pins revision `20d5ba98b7b79648910248cbfc3ff6a8a35d81a1`
and SHA-256 `2724b2ec8e49d1923883f569c26fd1ee6a1adc412cba8b122c56e0c1c4b1a6e8`.
See [upstream release notes](https://github.com/lebedenko/qmarkdown/blob/v1.14.0/docs/release-notes-1.14.0.md).
This supersedes the selection-source description in historical SDD 0014 without changing its recorded evidence.

## Acceptance criteria

- [x] Require QMarkdown 1.14, retain compatible 1.x policy and pin the reviewed release.
- [x] Configure replacement markers and nonblocking wheel observation through public APIs.
- [x] Correct math guidance and preserve existing adapter/table/settings behavior.
- [x] Public consumer tests verify marker configuration, exact formula copy, Find and selection endpoints.
- [x] Installed-provider task check and fresh locked-provider build, CTest and QML lint pass.
- [ ] Native task appearance, math selection and nested scrolling reviewed.
- [ ] Hosted checks run.

## Tasks

- [x] Update dependency contract, lock, reader configuration and user guidance.
- [x] Add focused public consumer regression coverage and update active documentation.
- [x] Run automated acceptance and document actual outcomes.

## Verification

2026-10-19, Linux, Qt 6.12.0, Fusion Controls, offscreen/software:

- Focused formula/Find and presentation tests passed (2/2) against installed system QMarkdown 1.14.0.
  The formula test compares the complete surrounding-text/TeX payload and clipboard, locates exact TeX
  through Find, then checks both public selection endpoints and selected text after closing Find.
- `task deps -- --archive /tmp/mdv-qmarkdown-1.14.tar.gz` passed checksum verification and built/installed
  the locked shared provider into `build/deps/prefix`; math and KSyntaxHighlighting cache options are ON.
  Download was explicit; configure/build/test did not acquire dependencies. Installed compatible KDE was used.
- Changed-document local links and `git diff --check` passed.
- Native task appearance, math selection, nested scrolling and hosted checks have not been performed;
  they remain pending and are not established by offscreen automation.

Fresh consumer verification passed: configured a new Ninja Debug build at `build/qmarkdown-1.14-consumer`
with BUILD_TESTING=ON and absolute CMAKE_PREFIX_PATH/QMarkdown_DIR pointing to `build/deps/prefix`.
Build (`--parallel 2`), CTest (58/58) and `all_qmllint` passed. `ldd` and `LD_DEBUG=libs` startup runs
confirm `/usr/lib/libQMarkdown.so.1` for the system consumer and `build/deps/prefix/lib/libQMarkdown.so.1`
for the fresh consumer. Compile definitions select the corresponding installed QML import roots; both roots
contain the provider module, and both offscreen startup runs passed without a loader path override.

System-provider `task check` passed: formatting, Debug build, 58/58 CTest cases, QML lint, clang-tidy,
Release build, desktop metadata validation, isolated system/local installation layouts, staged installation and
installed startup smoke checks. Debug and Release caches explicitly select `/usr/lib/cmake/QMarkdown` (1.14.0). Temporary logs: `/tmp/mdv16-{focused,deps,check,consumer}.log`.

## Open decisions

None; accepted plan defines scope.
