# 0007 — TOML application configuration

Status: Complete
Authorization: Owner request to add tomlplusplus and application configuration in the XDG config folder (accepted plan).

Header configuration and live header-only watching are added by [SDD 0020](0020-reader-visual-design.md);
the original reader-preference loading and save paths remain unchanged.

## Problem and outcome

Preferences were in `QtCore.Settings`. The reader now has a user-editable TOML file that the upcoming settings window
and typography/theme options can share.

## Scope and non-goals

- File: `$XDG_CONFIG_HOME/md-viewer/config.toml` when `XDG_CONFIG_HOME` is absolute, else `~/.config/md-viewer.toml`.
  The first existing candidate is read and written back; with none, the first candidate is created on first change.
- Keys: `[reader] zoom` (float, clamped 0.5–3.0) and `[reader] column_width` (`"narrow"`, `"comfortable"`, `"wide"`).
- Non-goals: settings UI (next roadmap item), migration of old QSettings values, file watching, new preferences.

## Design and decisions

- `AppConfig` (QML element, `src/appconfig.*`) reads once at construction and updates preferences immediately and debounces persistence with a restarting 250 ms single-shot timer
  on the GUI thread and `QSaveFile` (maintenance update: [0009](0009-review-maintenance.md)).
  Explicit `flush()`, application shutdown and destruction persist pending changes. Failed writes remain pending
  and report an error without automatic retries. Existing unreadable or malformed files are never replaced. Wrong-typed or unknown values fall back per key; unknown keys and invalid values are logged as warnings on the console
  (`Configuration <path> - ...`).
- Saving edits the existing parsed table, so unknown keys survive; comments are lost.
- A syntax error yields defaults plus an `error` message; the file is never overwritten while invalid (changes apply
  for the run only).
- tomlplusplus 3 from the system package via `find_package`; no lock or download. CI installs `libtomlplusplus-dev`.
- Test startup isolates both `HOME` and `XDG_CONFIG_HOME`; application tests use a fresh config directory per harness.

## Acceptance criteria

- [x] Path candidate rules, parsing/clamping/fallbacks, create/fallback-file writes, unknown-key preservation and
  syntax-error protection are covered by `tests/appconfig_test.cpp`.
- [x] Zoom and width shortcuts work through `AppConfig` (existing shortcut tests).
- [x] Native: persistence across restart with XDG set and unset; hand edits are read; corrupt file keeps defaults and is untouched.

## Tasks

- [x] AppConfig, CMake and QML wiring, tests, documentation.

## Verification

2026-10-16, Qt 6.12.0, offscreen: `task check` passed (format, tidy, qmllint, debug and release builds, CTest incl. AppConfig and shortcut tests, install smoke test). Native: owner confirmed manual verification passed (2026-10-16).

## Open decisions

Resolved in [0008](0008-settings-window.md): the settings dialog shows `AppConfig.error`.
