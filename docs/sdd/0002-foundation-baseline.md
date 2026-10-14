# 0002 — Foundation visual baseline

Status: Implemented; native review pending
Authorization: Owner requested on 2026-10-14 finishing Roadmap §0 with the Polished preset, all GFM features and a
basic single-document layout; hosted GitHub execution is explicitly deferred.

## Problem and outcome

The scaffold rendered with qmarkdown's Legacy style and a bare content area. The baseline now uses the Polished preset,
exercises every GFM extension and has the mockup's header/footer frame, so the native review judges a representative UI.

## Scope and non-goals

- Polished preset selected from the system theme (`PolishedDark` when the palette base is dark, else `PolishedLight`).
  It supplies structure (H1/H2 separators, code metrics, borders, spacing). Text, link and selection colors remain
  explicit overrides bound to Qt palette roles, so they follow the system theme.
- `gfmEnabled` enables tables, task lists, strikethrough, extended autolinks and tag filtering.
- Header (title, source label) and footer (source label, format) using `Controls.ToolBar`.
- Non-goals: right outline sidebar (needs Roadmap §2 navigation), Find/View/Editor controls, zoom, file opening,
  hosted GitHub execution (deferred by owner).

## Design and decisions

`Main.qml` derives `dark` from `palette.base.hslLightness` and passes it to `ReaderPane`, which selects the preset.
Only data-backed items appear in the frame; the mockup's actions arrive with the features that implement them.
Separator/border colors currently come from the preset's fixed palette; rebind to a palette role if review shows a mismatch.

## Acceptance criteria

- [x] Preset follows the system light/dark palette; color overrides are retained.
- [x] Bundled sample renders table, task list, strikethrough, autolink and fenced code.
- [x] Header and footer present; no sidebar.
- [x] Integration test: GFM source selects as rendered text without `|` or `~~` markers.
- [ ] Native visual/input review in light and dark themes.

## Tasks

- [x] `ReaderPane` preset, `Main` header/footer and GFM sample.
- [x] GTest for GFM rendering; `task check`.
- [x] Update roadmap, architecture and mockup notes.
- [ ] Owner native review (`task run`).

## Verification

2026-10-14: `task check` passed (Debug/Release builds, 3/3 CTest, format, tidy, qmllint, staged startup), offscreen.
Native visual/input review has not been fully performed. Owner's native run: GFM rendering OK; Polished separator/border/code
colors OK, now derived from the system text color (owner will tune later). Native mouse selection failed; the owner reports the
cause is already fixed in qmarkdown and will ship in 1.8, so re-verify after that upgrade. Offscreen synthesized-drag checks
could not reproduce it.
