# 0011 — Typography preferences and reader theme

Status: Implemented; native visual check pending
Authorization: Owner asked to move to the next roadmap item and chose the scope below in planning (2026-10-17).

## Problem and outcome

Reading comfort depends on font and spacing, and the system palette is not always the wanted reader scheme.
The settings window edits body/code font family, block spacing, link underline and a reader theme.

## Scope and non-goals

- `[reader]` keys: `theme` (`system|light|dark`), `body_font`, `code_font` (empty = preset family),
  `block_spacing` (multiplier, clamped 0.5–2.0), `link_underline` (bool). Invalid values fall back per key with a warning.
- `theme` affects the reader only: the QMarkdown preset and the reader paper/text/link/selection colors.
  Toolbar, footer and dialogs keep the system palette. `system` follows the window palette as before.
- Body family also applies to headings; code family to code blocks and inline code. Sizes stay preset `pointSize` × zoom.
- Non-goals: line height (no public QMarkdown property), font size separate from zoom, whole-window theming.

## Design and decisions

`AppConfig` owns values and persistence (same setter/debounce pattern as zoom). `Main.qml` derives reader colors;
`ReaderPane.qml` applies families through `scaledFont`, `blockSpacing` (preset × scale) and `linkUnderline` on `MarkdownStyle`.
Only public QMarkdown properties are used.

## Acceptance criteria

- [x] Config parses, clamps, falls back with warnings, and round-trips all new keys.
- [x] Settings controls change the reader's `dark`, `blockSpacingScale` and `linkUnderline`.
- [ ] Native: fonts, theme override and spacing look right in light and dark system themes.

## Tasks

- [x] Config keys and setters; reader and settings wiring; tests.
- [x] Roadmap, architecture and SDD 0008 notes.

## Verification

`task check` passed locally (format, tidy, qmllint, offscreen GTest, install checks). Native visual check not run.

## Open decisions

- `link_underline` defaults to true; confirm against the preset default during the native check.
