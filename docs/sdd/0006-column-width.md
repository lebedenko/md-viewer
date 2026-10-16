# 0006 — Column width presets

Status: Complete
Authorization: Owner request to proceed with the next roadmap item; scope chosen by the owner: column width presets only.

## Problem and outcome

The reading column is fixed at 800 logical px. The reader can cycle Narrow (600), Comfortable (800, default) and Wide
(1000) from the keyboard; the choice persists between runs.

## Scope and non-goals

- Ctrl+Shift+W cycles presets. The column is still limited by window width minus 48 px.
- Non-goals (remain in the roadmap): theme override, line-height/font-family preferences, a preferences menu or
  toolbar control, a width indicator.

## Design and decisions

- `ReaderPane.maxColumnWidth` replaces the hard-coded 800. `Main.qml` stores the preset index in
  `QtCore.Settings` (`reader/widthPreset`), clamped when read.

## Acceptance criteria

- [x] The shortcut cycles through three distinct widths and wraps to the start.
- [x] Native: visible width change, persistence across restart.

## Tasks

- [x] ReaderPane property, Main shortcut and setting, test.

## Verification

2026-10-16, Qt 6.12.0, offscreen: `task check` passed (format, tidy, qmllint, debug and release builds, CTest incl. the width shortcut test, install smoke test). Native: owner confirmed manual verification passed (2026-10-16).
