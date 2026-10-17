# Roadmap

Milestones are ordered by useful behavior, not dates. The foundation and single-file opening, copy, zoom, width and settings are implemented;
new feature work gets a short SDD. Checkboxes mean implementation, with verification recorded in its SDD.
Native open-dialog and file-manager drop checks in SDD 0003 remain pending.

## 0 — Foundation

Complete; local, hosted-main and owner-confirmed native evidence is recorded in
[0002](sdd/0002-foundation-baseline.md). Hosted checks for the uncommitted 1.10 upgrade await a push.

- [x] Independent C++23 / Qt Quick / CMake / Ninja project.
- [x] External QMarkdown 1.10 package and explicit pinned preparation.
- [x] GTest integration and application startup checks.
- [x] clang-format, clang-tidy, qmllint and local acceptance entry point.
- [x] SDD template, architecture, mockup notes, GitHub workflow files.
- [x] Hosted GitHub execution (see [Foundation verification](sdd/0002-foundation-baseline.md)).
- [x] Polished preset, full GFM and header/footer baseline ([0002](sdd/0002-foundation-baseline.md)).
- [x] Owner-confirmed native visual/input review on QMarkdown 1.10.

## 1 — Useful single-file reader

- [x] Open local Markdown from CLI and file dialog; explicit encoding/error behavior.
- [x] Selection/copy and code-copy behavior exercised on real documents.
- [x] Keyboard zoom ([0005](sdd/0005-zoom.md)).
- [x] Column width presets ([0006](sdd/0006-column-width.md)).
- [x] TOML application configuration, XDG location ([0007](sdd/0007-toml-config.md)).
- [x] Toolbar settings button and modal settings window ([0008](sdd/0008-settings-window.md)).
- [ ] Small typography preferences; light/dark override.
- [ ] Live reload through ordinary and atomic saves, with proportional position preservation.
- [ ] Configured external editor action; safe argument handling and failure feedback.
- [ ] Conventional keyboard shortcuts and visible focus.

Exit: open a real document, read/copy comfortably, edit externally and see saved changes without disruptive jumps.

## 2 — Documentation navigation

- [ ] Relative links, fragment navigation, Back/Forward and reading-position restoration.
- [ ] Outline and rendered-text search with visible results.
- [ ] Define and adopt required qmarkdown public APIs; update the dependency lock and consumer checks together.
- [ ] Keyboard access to navigation controls and document links, subject to upstream support.

QMarkdown 1.10 supplies headings, fragment navigation, literal Find with rendered highlights, navigation rectangles
and opt-in keyboard link traversal. Integrate these public APIs and define application history/restoration and
update validity behavior in this milestone. No private API workaround is planned.
Exit: follow a cross-document heading link, find text, copy it, return to the previous reading position, and reload safely.

## 3 — Folder browsing and continuity

- [ ] Open a folder, lazy Markdown tree, filename picker and defined hidden/ignored/symlink behavior.
- [ ] Recent documents and remembered reading positions.
- [ ] Tabs/session restoration and read-only source view, after validating the simpler workflow.

No database or persistent content index initially. Folder browsing is not a project-management system.

## 4 — Stabilization and delivery

- [ ] Real-world large-document performance and reload benchmarks; use upstream improvements where appropriate.
- [ ] Accessibility/focus review and native scaling/light/dark checks.
- [ ] Adopt extension renderers only for justified document needs; no requirement to wait for all qmarkdown v2 features.
- [ ] Choose application license and stable application identity.
- [ ] Desktop/MIME metadata and a tested installation/package story.
- [ ] Document supported environment and missing capabilities before a first downloadable release.

Streaming is not a blocker for local-file reading. PDF export, graphs, AI, Git UI, sync and built-in editing are not planned.

## Separate future decision

Vim-style navigation/selection will be considered only after all other planned features are implemented and stabilized.
There is no current commitment to implement it. It would require a separate design and renderer API assessment.
`holonight-qt` and embedded Neovim are excluded, not optional build flavors.
