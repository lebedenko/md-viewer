# 0002 — Foundation visual baseline

Status: Complete
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
- [x] Native visual/input review in light and dark themes (owner-confirmed on 1.10).

## Tasks

- [x] `ReaderPane` preset, `Main` header/footer and GFM sample.
- [x] GTest for GFM rendering; `task check`.
- [x] Update roadmap, architecture and mockup notes.
- [x] Owner native review (`task run`), confirmed on 1.10.

## Verification

2026-10-14: `task check` passed (Debug/Release builds, 3/3 CTest, format, tidy, qmllint, staged startup), offscreen.
Native visual/input review has not been fully performed. Owner's native run: GFM rendering OK; Polished separator/border/code
colors OK, now derived from the system text color (owner will tune later). Native mouse selection failed; the owner reports the
cause is already fixed in qmarkdown and will ship in 1.8, so re-verify after that upgrade. Offscreen synthesized-drag checks
could not reproduce it.

## Foundation finalization — 2026-10-16

Authorization: Owner accepted the finalization plan: preserve the uncommitted 1.10 upgrade, fast-forward merged main,
correct the pinned archive checksum and staged loader selection, resolve qmllint, and close Foundation's records.
The earlier hosted/native deferrals above describe the original scope; this authorization supersedes them.
No reader features, public application API changes, commits, publishing or repository settings changes are included.

Design: retain QMarkdown 1.10 minimum/import and compatible 1.x policy, rejecting 2.x. Pin revision
`efb2d35fbbbffbe7b45c7d11031072f753b4706d` with SHA-256
`4da054b0e071d9508897eae9d1c156d3c208a4ed4b62fed069d597271449faac`.
CMake generates the selected provider library directory with `$<TARGET_FILE_DIR:QMarkdown::QMarkdown>`;
staged validation prepends it to existing loader paths. The header subtitle uses the window's public font.
QMarkdown supplies headings, fragments, literal Find/highlights, navigation rectangles and opt-in keyboard link traversal;
application integration remains in later milestones.

Acceptance and tasks:

- [x] Preserve owner changes and fast-forward to combined main `b83b9641c0f5e22d1012957fe83c679ec9a44663`.
- [x] Correct lock, selected-provider staging, subtitle binding and dependency/capability guidance.
- [x] Record reviewed and merged [PR #1](https://github.com/lebedenko/md-viewer/pull/1) and
  [PR #2](https://github.com/lebedenko/md-viewer/pull/2); GitHub confirms both merged on 2026-10-16.
- [x] Record owner-confirmed native selection/copy, scrolling, keyboard input and light/dark checks on 1.10.
- [x] Explicit `task deps`, then `task check`, including all three CTest cases and warning-free qmllint.
- [x] Verify selected-provider staging with an older local prefix present; check documentation and diff whitespace.
- [x] Record combined-main hosted outcome separately from pending hosted validation of these uncommitted changes.

Verification on 2026-10-16:

- `task deps` passed: downloaded archive matched the corrected SHA-256 and built/installed pinned QMarkdown 1.10.0.
- `task check` passed: Debug/Release builds, 3/3 CTest, clang-format, clang-tidy, warning-free qmllint and staged
  offscreen startup. Both build caches selected the installed system QMarkdown 1.10 (`/usr/lib/cmake/QMarkdown`).
- Before dependency installation replaced the local 1.7 symlink, Release configure/build/install and staged startup
  passed with generated provider `/usr/lib` prepended to the older local prefix in `LD_LIBRARY_PATH`.
  `ldd` confirmed `libQMarkdown.so.1 => /usr/lib/libQMarkdown.so.1`; the local prefix still contained 1.7.0.
- Relative documentation links, active version/capability references and `git diff --check` passed.
- [Combined-main CI](https://github.com/lebedenko/md-viewer/actions/runs/38883979466) completed successfully at
  `b83b9641c0f5e22d1012957fe83c679ec9a44663`. This verifies the merged Dependabot changes with the committed
  dependency baseline. Hosted verification of these uncommitted 1.10 finalization changes remains pending until pushed.
- Native selection/copy, scrolling, keyboard input and light/dark evidence is owner confirmation on 1.10,
  not an automated or agent-performed native check. Historical 2026-10-14 results remain unchanged.

Foundation is complete. Finalization changes remain uncommitted as requested; no publishing or repository settings changed.
