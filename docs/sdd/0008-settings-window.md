# 0008 — Settings window

Status: Complete
Authorization: Owner request to proceed to the next roadmap item ("Toolbar settings button and modal settings window").

## Problem and outcome

Zoom and column width could only be changed by shortcuts. The reader now has a toolbar Settings button that opens a
modal settings dialog bound to `AppConfig`, and it surfaces the configuration file path and any `AppConfig.error`
(deferred from [0007](0007-toml-config.md)).

## Scope and non-goals

- Toolbar "Settings" button, `Ctrl+,` (and `StandardKey.Preferences`) open `SettingsDialog`; `Esc`/Close dismisses it.
- Controls: zoom slider (50–300 %, 5 % steps) and column-width combo box. Both write through `AppConfig` immediately
  and the reader updates live; there is no Apply/Cancel step.
- Shows the config file path and, when present, the TOML syntax error (changes then apply for the run only).
- Non-goals: typography and light/dark options (next roadmap item), editor setting, a separate OS window.

## Design and decisions

- `qml/SettingsDialog.qml` is a `Controls.Dialog` (modal, centered on the overlay) taking the `AppConfig` instance and
  the width-preset names from `Main`; it owns no state beyond the config.
- A single in-window dialog keeps focus handling and Escape/Tab behavior to the Controls defaults.
- Slider and shortcut zoom values both live in `AppConfig`, so the stored value is the single source of truth.

## Acceptance criteria

- [x] `Ctrl+,` opens a modal dialog; changing width and zoom updates the reader; `Esc` closes it (`ApplicationSettings` test).
- [x] Native: button opens the dialog, Tab order/focus ring visible, a corrupt config file shows its error, light/dark legible.

## Tasks

- [x] `SettingsDialog.qml`, Main wiring, CMake resources, test.
- [x] Roadmap and architecture notes.

## Verification

2026-10-16, Qt 6.12.0, offscreen: `task check` passed (including `ApplicationSettings`). Native: owner confirmed manual verification passed (2026-10-17).
