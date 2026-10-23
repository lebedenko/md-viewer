# 0020 — Reader visual design

Status: Implemented; automated acceptance passed; native review pending
Authorization: Owner request to implement the accepted reader visual design refresh plan, 2026-10-24.

## Problem and outcome

Replace startup demonstration content with a minimal Open prompt, compact accessible icon actions, desktop-specific
header preferences and a right-hand outline. Keep native window decorations and the standalone QMarkdown boundary.

## Scope and non-goals

Initial loading shows progress; empty files are valid documents; failures retain the last successfully loaded content.
Toolbar order is Back, Forward, Open, shrinking plain-text title/path, Find, Edit, Outline, Settings, optional Close.
Selection stays in Settings and Ctrl+A. Minimum window width is 480. Icons use theme names and bundled SVG fallbacks.
Header title and Close default true; hiding title hides filename and toolbar path. Native window title is independent.
Outline defaults enabled, appears only with headings and retains its toggle during the run. Width is at most 240 or
one third of available width. Indented plain-text rows scroll independently; the accent marks the last successfully
activated outline heading and clears when content changes. No scroll tracking, dependency upgrade, editor or Vim mode.

## Design and decisions

AppConfig captures colon-separated XDG_CURRENT_DESKTOP tokens at construction, ASCII lowercases and validates
letters/digits/underscore/hyphen. Merge defaults, selected base, first existing `<base-stem>-<desktop>.toml`, per
header key. Invalid/missing keys inherit; malformed/unreadable layers contribute nothing. A selected invalid override
never falls through. File and ancestor-directory watches debounce header-only reloads by 100 ms and reattach after
replacement. Reader settings retain existing save paths and preserve header tables; desktop overrides are never saved.
Main composes startup and toolbar; IconButton owns icon sizing/accessibility/focus. ReaderPane uses public headings
and navigation only. Hiding a focused Close transfers focus to Settings unless a modal dialog is visible.

## Acceptance criteria

- [x] Startup/loading/failure/empty-file states and last-good retention verified offscreen.
- [x] Icon resources, action availability, names and 480-pixel layout verified offscreen.
- [x] Header combinations, inheritance, priority, validation, recreation and settings preservation verified offscreen.
- [x] Right outline geometry, toggle retention, nested/long rows, keyboard and activation marker verified offscreen.
- [x] Full automated acceptance and native limitations recorded separately.

## Tasks

- [x] Implement startup, toolbar and right-hand outline.
- [x] Implement desktop header layering and watches.
- [x] Add focused regression coverage and run task check.
- [x] Update architecture, roadmap, configuration and mockup notes.

## Verification

2026-10-24, installed Qt 6.12.0 / QMarkdown 1.17.1, Fusion Controls, offscreen/software:

- Focused AppConfigHeader and ApplicationVisualDesign checks cover desktop normalization/invalid-token skipping,
  first-existing priority, per-key inheritance, malformed and unreadable selected overrides, reader-setting isolation,
  header preservation on save, ordinary and atomic edits, creation/deletion, legacy stem naming, desktop capture and
  directory replacement. UI checks cover startup/failure/empty documents, action availability, bundled icon presence,
  non-overlapping 36-pixel toolbar actions at width 480, all four header combinations, focused Close transfer and
  modal focus preservation, right-side outline sizing, nested/long titles, arrows/Enter/Space, marker reset and toggle retention.
- Existing Find/selection shortcut coverage now explicitly opens `tests/data/real-document.md`.
- `task check` passed: formatting, Debug build, 75/75 CTest cases, QML lint, clang-tidy, Release build,
  desktop metadata validation, isolated system/local install layouts, staging and installed startup smoke tests.
  No dependencies were acquired or upgraded. Full log: `/tmp/mdv20-check-final.log`.
- After the final test-only brace cleanup, Debug rebuild and CTest rerun passed 75/75;
  `/tmp/mdv20-final-{rebuild,ctest}.log` records the results.
- Local Markdown links, SVG XML parsing and `git diff --check` pass.

Native light/dark, scaling, icon appearance/fallback rendering, tooltip behavior, focus and input review remain pending.
Offscreen geometry/input assertions do not establish native usability. Hosted CI was not run.

## Open decisions

None.
