# 0003 — Open file

Status: In progress
Authorization: Owner request to plan and implement the first roadmap feature (open a file from the CLI, a file dialog and
drag and drop); plan accepted 2026-10-16. Owner also authorized the three open-file review corrections in the accepted fix plan.

## Problem and outcome

The viewer shows only a bundled sample. After this change a user can open one local Markdown file from a command-line
path, from a parented modal file dialog, or by dropping a file onto the window, with explicit encoding and error behavior.

## Scope and non-goals

- One document at a time. A new open replaces the current document and scrolls to the top.
- CLI: `md-viewer [file]`. `~`, `~/…`, relative paths and `file://` URLs resolve; `~user` is rejected. More than one
  positional argument exits with status 2.
- Dialog: `QtQuick.Dialogs` `FileDialog` (Ctrl+O and an Open button), filters `*.md *.markdown` and all files.
- Drop: a nonempty URL list supporting Copy opens its first local file URL and explicitly accepts Copy, including
  when Move was proposed. Move-only, empty URL and plain-text drops are rejected. Non-local URLs show an error and
  remain unaccepted. Acceptance acknowledges the asynchronous request; later load errors appear in the footer.
- Encoding: UTF-8 only. A UTF-8 BOM is stripped. UTF-16/UTF-32 byte-order marks are rejected. Invalid UTF-8 bytes become
  U+FFFD and a warning is shown; the document still opens.
- Paths expand home and become absolute without lexical normalization or canonicalization. Preserve supplied `.`
  and `..` for filesystem resolution (including symlink parents), and retain that absolute spelling in metadata.
- Only regular files are opened; symlinks to regular files are allowed. Reject special files before opening.
- Errors (missing, directory, special file, unreadable, larger than 16 MiB, unsupported encoding, non-local URL) keep the last good
  document and show the message in the footer.
- Non-goals: live reload, recent files, history, relative links, images (`baseUrl`), multiple windows, session restore.

## Design and decisions

- `DocumentSession` (`src/documentsession.{h,cpp}`) owns URL, source snapshot, title, state, error, warning and revision.
  It reads on a worker thread (`QtConcurrent`) and ignores results whose generation is not the latest.
- `DocumentSession::resolvePath` and `DocumentSession::decode` are static and pure so they are unit-tested directly.
- `main.cpp` resolves nothing itself: it passes the positional argument to `Main.qml` as the `initialPath` initial
  property, and QML calls `session.openPath()`.
- The dialog uses the public Qt API. Qt uses the xdg-desktop-portal when the platform theme provides it
  (`QT_QPA_PLATFORMTHEME=xdgdesktopportal` forces it for diagnosis; the application does not set it). No QtDBus,
  KWindowSystem, Widgets or private API. A direct portal client is a separate decision if native testing shows a need.
- QtQuick.Dialogs is a runtime QML module; the CI Qt install (aqt, full qtdeclarative) already includes it.

## Acceptance criteria

- [ ] `md-viewer path.md`, `md-viewer ~/x.md`, `md-viewer ./rel.md` and `md-viewer file:///…` open the file.
- [ ] Missing, directory, oversize and UTF-16 inputs show a clear error and keep the previous content.
- [ ] UTF-8 BOM is stripped; invalid UTF-8 opens with a warning.
- [ ] Ctrl+O / Open shows a parented modal dialog; the chosen file loads; cancel changes nothing.
- [ ] Dropping a local file loads it; dropping text is refused.
- [ ] Rapid successive opens: the last request wins.

## Tasks

- [x] `DocumentSession` with tests.
- [x] CLI argument, dialog, drop area and error display in QML.
- [x] `task check`.
- [x] Review corrections: regular-file validation, filesystem path semantics and explicit Copy drop acceptance.
- [x] Named-pipe, symlink-path and actual application QML drag/drop regressions (15-second CTest bounds).
- [ ] Native manual checks.
- [x] Update ARCHITECTURE.
- [x] Record implementation in ROADMAP; native verification remains pending here.

## Verification

2026-10-16, Arch Linux, Qt 6.12.0, offscreen:

- `task check` passed (format, tidy, qmllint, debug and release builds, CTest incl. 10 new DocumentSession tests:
  path resolution, BOM/invalid UTF-8/UTF-16/32, metadata, failure keeps last good, oversize, last request wins).
- `md-viewer --smoke-test <existing file>` and `<missing file>` exit 0 (the error is shown in the window, not on stderr);
  two positional arguments exit 2.

Not run (planned only): native Ctrl+O/Open dialog on Hyprland, GNOME and KDE (default theme and
`QT_QPA_PLATFORMTHEME=xdgdesktopportal`, check parenting/modality), dragging a file from a file manager, and visual
check of the footer error and drop highlight. Native dialog and file-manager drop behavior remain unverified; automated drop verification is recorded below.

Review-fix verification, 2026-10-16, same offscreen environment:

- Focused CTest regressions passed for named-pipe rejection and symlink-parent resolution. The pipe fails promptly
  and preserves content, URL and revision; all discovered GTests have a 15-second CTest timeout.
- Symlink fixture `link → target/sub` with competing documents verifies relative paths, absolute paths, `open()`
  local URLs and `openPath()` file URLs select `target/doc.md` and retain the supplied absolute path spelling.
  Symlinks to regular files remain accepted.
- Actual `Main.qml` loaded in GTest with public Qt drag-enter/drop events: Copy loads content, a proposed Move
  resolves to supported Copy, and move-only, plain-text, empty-URL and remote-URL drops remain unaccepted.
  Remote URLs produce error feedback and preserve the previous document.
- Final `task check` passed: formatting, Debug build, all 16 CTest cases, qmllint, clang-tidy, Release build,
  staged installation and installed application smoke test. `git diff --check` passed.
- Native file-manager drops and dialog parenting/modality were not run and remain pending above.

## Open decisions

- Whether native testing shows a need for a direct portal client.
