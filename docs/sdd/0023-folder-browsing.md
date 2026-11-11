# 0023 — Folder browsing

Status: Implemented; automated acceptance passed; native and hosted checks pending
Authorization: Owner accepted the first Milestone 3 slice, decided tree placement, ignore rules, symlinks and CLI
behavior, required recursive scanning with pruning, and requested implementation ("implement 0023") on 2026-11-11.

## Problem and outcome

The reader opened one file at a time through the CLI, the file dialog or a drop. Reading a documentation directory
meant reopening the dialog or relying on links between documents. Now `mdv <dir>` or Open Folder shows the folder's
Markdown files in a left sidebar tree, and a filename picker jumps to any Markdown file under that folder.
A project root usually has a README and one or two documentation folders among many unrelated ones, so the tree
lists only the folders that lead to Markdown.

## Scope and non-goals

In scope:

- One open folder per window. `mdv <dir>`, Open Folder (Ctrl+Alt+O), the Open prompt's folder button and dropping a
  directory all open it. `mdv <file>` behaves as before and opens no folder.
- The folder is scanned recursively in the background. The tree lists `.md`/`.markdown` files (case-insensitive
  suffix) and only the directories that contain one at any depth. Directories come first, then files, in natural
  case-insensitive order. Directories start collapsed.
- Activating a file opens it through NavigationController, so Back/Forward, restoration and live reload are unchanged.
  Keyboard focus then moves to the document, as it does after outline activation. The current document is
  highlighted and its ancestors expanded when it lies under the folder.
- Opening a folder also opens its root `README.md` (case-insensitive, `.md` before `.markdown`) unless the current
  document already lies under that folder. Without a README the current document, or the Open prompt, stays.
- Filename picker (Ctrl+P): case-insensitive substring match on the folder-relative path, file-name matches ranked
  before directory matches, then tree order, 50 results. Enter opens, Escape returns focus to where it was.
- Sidebar toggle: toolbar Files button and Ctrl+B. With no folder open, the button opens the folder dialog.
- The folder is rescanned when a listed directory changes (300 ms debounce), when the window becomes active and when
  the picker opens. Expansion state, scroll position and tree focus survive a rescan.
- No ignore rules: `.gitignore` and similar files are never consulted. Dot-prefixed files and directories are skipped.
- Symlinks are followed for both files and directories. Broken links are omitted.

Errors and edge cases:

- A missing, unreadable or non-directory folder reports a failure in the status area and keeps the previous folder.
- A directory link that resolves to one of its own ancestors is not followed, so cycles end.
- Unreadable subdirectories contribute nothing and are therefore not listed.
- The scan stops at 20 000 Markdown files or 200 000 visited directories; the sidebar and picker say that the
  listing is incomplete.
- A folder without Markdown opens with an explanatory note instead of a tree.
- A document outside the folder (link, dialog, drop) opens normally; the folder stays and nothing is highlighted.
- Only the first 1024 listed directories are watched. A Markdown file created in a directory that was not listed
  appears on the next rescan (window activation, picker, or any watched change), not immediately.

Non-goals: recent documents, remembered positions, tabs, session restoration and source view (later Milestone 3
specs); remembering the folder between runs; new configuration keys; full-text search across files; file operations
(create, rename, delete); Git state; multiple roots; registering `mdv` as a directory handler in the desktop file;
resizable sidebar; qmarkdown changes.

## Design and decisions

- `FolderModel` (C++, `QAbstractListModel`, QML element) owns the root, the scan, expansion state, the current-document
  row, directory watches and picker search. It exposes the visible tree rows as a flat list (name, URL, directory,
  depth, expanded), so the view needs no tree control and expansion survives model resets.
- `FolderModel::scan()` is a static synchronous function run through QtConcurrent with a cancellation flag and the
  generation check DocumentSession uses; a superseded scan cannot replace a newer one, and a failed scan leaves the
  adopted folder untouched. A rescan whose file list is unchanged does not reset the model, but every successful
  scan rebuilds directory watches so directory replacement with identical filenames remains observable.
- Reveal walks every ancestor to the folder root, including past already expanded descendants. Sorting retains
  the system collator except for the C locale, where an explicit English locale enables numeric ordering.
- Lazy per-directory loading was rejected: pruning directories without Markdown needs the recursive result anyway,
  and the same result feeds the picker. No persistent index.
- Paths stay lexical under the root as opened; canonical paths are used only for cycle detection. A file reached
  through a symlinked directory keeps the URL the tree shows, so relative links and the image root resolve as they
  do for any opened file.
- `qml/FolderSidebar.qml` presents the model in a `ListView` under a header with the folder name (full path as
  tooltip); only the current row is a Tab stop. `qml/FilePicker.qml` is a modal popup with a text field and results.
  Main owns `QtQuick.Dialogs.FolderDialog`, shortcuts, drop routing and layout through `openLocation()`.
- Layout: sidebar on the left, width `min(260, window width / 3)`; the reader and its right-hand outline share the
  rest under the existing SDD 0020 outline rule. The sidebar uses the reader paper, text and link colors.
- `main.cpp` renames the positional argument to `path`; Main asks FolderModel whether it is a directory.
- Local images stay authorized only under the opened document's directory, not the folder root.
- Keyboard: Up/Down move, Right expands or enters, Left collapses or moves to the parent, Enter/Space activate.

## Acceptance criteria

- [x] A directory location shows the tree and opens the root README when present; a file location is unchanged.
- [x] Tree lists only Markdown files and directories leading to them, in the specified order.
- [x] Activating a file navigates with working Back; the current document is highlighted and revealed.
- [x] Symlinked files and directories are followed; a self-referential link ends; broken links are absent.
- [x] Created, removed and renamed files appear after the debounce without collapsing expanded directories.
- [x] Picker filters, ranks, opens with Enter and restores focus on Escape; truncation is reported by the model.
- [x] Missing root keeps the previous folder and reports a failure; a folder without Markdown opens empty.
- [x] Sidebar toggle, shortcuts, keyboard traversal and the 480-pixel toolbar layout hold offscreen.

## Tasks

- [x] FolderModel: recursive pruned scan, ordering, symlinks, cycle guard, caps, async generation checks.
- [x] FolderModel rows, expansion, reveal, watches and picker search.
- [x] FolderSidebar, Files toolbar action, Ctrl+B, layout with the outline.
- [x] Folder opening: directory argument, FolderDialog, Open prompt button, directory drop, README selection.
- [x] FilePicker and Ctrl+P.
- [x] Model and application tests; `task check`.
- [x] Update ARCHITECTURE.md, ROADMAP.md and README.md.
- [ ] Native visual/input review and hosted CI.

## Verification

2026-11-11, installed Qt and QMarkdown 1.18.0, Fusion Controls, offscreen/software:

- `tests/foldermodel_test.cpp` (7 cases): pruning of directories without Markdown, hidden entries, natural ordering,
  followed file/directory links including one outside the root, cycle and broken-link handling, invalid roots,
  file cap and cancellation, expand/collapse with remembered nested expansion, reveal, parent rows, search ranking
  and limit, watched create/remove/rename with retained expansion, failed open keeping the previous folder.
- `tests/folder_application_test.cpp` (4 cases): directory location with README, sidebar geometry at default width
  and 480, non-overlapping toolbar actions including Files, Ctrl+B, tree keyboard traversal and activation with Back,
  picker filtering/Enter/Escape focus return, directory drop, README-less folder keeping the document, missing
  folder failure in the status area.
- `python3 scripts/check.py` (`task check`) passed: formatting, Debug build, 94/94 CTest cases, QML lint without
  warnings, clang-tidy, Release build, desktop validation, isolated install layouts and installed smoke tests.
- `mdv . --smoke-test` on this repository exits 0 offscreen.

Limitations: the `QtQuick.Dialogs.FolderDialog` path, the unreadable-root message, the 200 000-directory cap, scan time
on a very large real tree and the sidebar's appearance were not exercised. Native light/dark, scaling, focus,
tooltip and input review are pending; hosted CI was not run. The new tests use an asserting `waitFor` helper because
Qt's `QTRY_*` macros return silently when they time out outside a QTest function, which GTest reports as a pass;
existing tests still use those macros.

### Folder review fixes (2026-11-11)

Authorization: Owner requested implementation of the folder browsing review plan: ancestor reveal, watch
reattachment after unchanged scans, and numeric sorting under `C.UTF-8`; no dependencies or unrelated UI changes.

- [x] Walk all ancestors during reveal, even when a descendant already retains its expansion state.
- [x] Rebuild watches after every successful scan while keeping resets and revisions conditional on listing changes.
- [x] Use an English collator only when the default collator uses the C locale.
- [x] Add regressions for selecting another nested file after collapsing its parent and for automatic discovery
  after replacing a directory with identical filenames; the latter also checks that the unchanged scan causes no reset.
- All three scenarios failed before the fixes (including the existing ordering test under `C.UTF-8`).
- Focused folder CTest suite: 13/13 passed offscreen, including four application integration cases.
- Ordering test passed separately with `LC_ALL=C.UTF-8` and `LC_ALL=en_US.UTF-8`.
- The replacement regression allows 1500 ms for the debounce and unchanged background scan, which exposes no public
  completion signal, before adding the new file; discovery then uses a bounded condition wait.
- Full `task check` passed: formatting, Debug build, 96/96 CTest cases, QML lint, clang-tidy, Release build,
  desktop validation, isolated install layouts and installed smoke tests.
- Native visual/input checks and hosted CI were not run for these fixes.

## Open decisions

- Hidden entries: "no ignore rules" was read as "no `.gitignore`"; dot-prefixed files and directories are skipped.
  The owner has not confirmed this reading.
