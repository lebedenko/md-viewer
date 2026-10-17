# 0009 — Review fixes and maintenance

Status: Complete
Authorization: Owner request to implement the accepted review and maintenance plan, 2026-10-17.

## Problem and outcome

Preference changes caused synchronous writes for every slider event, failed reads could discard unknown keys,
and repeated application test setup could access fallback user configuration or share state. Required clipboard
fixtures were ignored and current documentation described an obsolete scaffold.

## Scope and non-goals

Debounce persistence by 250 ms while retaining immediate setters/signals. Protect existing files against read,
permission and syntax failures. Isolate tests, restore the tracked fixture, name width enum values explicitly,
and reconcile implemented capabilities. No dependencies, renderer changes, commits or publishing.

## Design and decisions

AppConfig owns a restarting single-shot GUI-thread timer and pending flag. Public C++ flush() stops the timer;
success alone clears pending changes. Shutdown and destruction flush. Failures report through error without
retry loops. Re-read existing TOML before atomic replacement, preserve unknown keys, check complete writes and
directory creation. Only an absent path starts an empty table. Named WidthPreset values retain integers 0–2;
QML owns translated labels and logical widths and resolves them by explicit enum value.

Tests register QML types once after temporary HOME/XDG setup. A small shared harness owns a fresh config directory,
engine and root for clipboard, drop, zoom, width and settings tests. Test startup fails if isolation cannot be created.

## Acceptance criteria

- [x] Immediate live preferences, deferred latest-value persistence and explicit/shutdown/destruction flushes.
- [x] Missing file creation, unknown keys and failed/malformed/unreadable existing-file protection.
- [x] Independent application state and subprocess sentinel fallback protection.
- [x] Every named width round-trips and selects the expected UI width.
- [x] Required Markdown/Unicode fixture is tracked and documentation reflects implementation.

## Tasks

- [x] Persistence and explicit width contracts.
- [x] Shared isolated application harness and focused regressions.
- [x] Fixture and documentation corrections.
- [x] Focused tests and task check; record outcomes below.

## Verification

2026-10-17, Linux, Qt 6.12.0, offscreen/software backend:

- Debug configure/build passed.
- `build/debug/reader-tests --gtest_filter='AppConfig.*:Application*.*:ReaderIntegration.CopiesSelectionOfRealDocumentToClipboard'`
  passed all 21 tests in one process, including the permission test (not skipped).
- CTest passed all 37 individually discovered cases, including the application startup check.
- Local Markdown links in README, roadmap and SDD files resolve; `git diff --check` passed.
- First `task check` reached clang-tidy after passing tests and qmllint; it reported aggregate initializer,
  special-member and enum diagnostics. These were corrected (the intentional QML-compatible integer enum is
  documented with targeted suppressions). Final `task check` passed: formatting, Debug build, all 37 CTest cases, qmllint, clang-tidy,
  Release build, staged installation and installed application smoke test.

Native checks from SDD 0003 remain pending; no new native or hosted checks are claimed.

## Open decisions

None.
