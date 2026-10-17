# 0012 — Live reload

Status: In progress (native check pending)
Authorization: Owner request to proceed to the next roadmap action item (milestone 1, live reload), 2026-10-17.

## Problem and outcome

Editing the open file in an external editor leaves the viewer stale. After this change saved edits, including atomic
replace-by-rename saves, appear without reopening and without a disruptive jump to the top.

## Scope and non-goals

- The opened local file and its parent directory are watched; events are debounced (150 ms) before a reload.
- A reload reads the file through the normal asynchronous path (UTF-8, 16 MiB cap, regular files only).
- Unchanged content (touch, unrelated sibling changes) does not bump the revision or re-render.
- A failed reload (file briefly missing, unreadable, now oversize) keeps the last good content, shows the error in the
  footer, and a later successful save clears it. The watch stays active so a recreated file is picked up.
- Reading position is preserved proportionally (scroll offset / scrollable height) across a reload. Opening another
  document still scrolls to the top.
- Non-goals: semantic (heading) restoration, selection preservation, watching linked/relative files, manual refresh action.

## Design and decisions

- `DocumentSession` owns a `QFileSystemWatcher` and a single-shot debounce timer. After each load attempt it re-adds
  the file and directory paths that the watcher dropped (atomic replace removes the file watch).
- `reloaded` is true when the current revision came from a reload, so QML can choose restore over reset.
- `ReaderPane` records the scroll fraction when a load starts (`savePosition`) and reapplies it as the content height
  settles (`restorePosition`); a short timer ends the restoring window. No renderer internals are used.

## Acceptance criteria

- [x] Ordinary write and atomic rename saves both update `markdown` and bump the revision.
- [x] Rewriting identical content leaves the revision unchanged.
- [x] Deleting the file shows an error and keeps content; recreating it recovers.
- [ ] A reload keeps the proportional reading position; opening a new file resets to the top.

## Tasks

- [x] Watcher, debounce and reload in `DocumentSession` with tests.
- [x] Position save/restore in `ReaderPane`, wiring in `Main.qml`, application-level test.
- [x] `task check`.
- [x] Update ROADMAP and ARCHITECTURE.
- [ ] Native check with a real editor (pending).

## Verification

2026-10-17, Arch Linux, Qt 6.12.0, offscreen: `task check` passed, including five new DocumentSession tests (ordinary write,
repeated atomic rename, identical content, delete and recover, switching files stops the old watch).

Not run: the proportional position restore (QML, no automated test) and "opening a new file resets to the top" are
unverified, as is a native check with a real editor (acceptance item 4 stays open).

## Open decisions

None.
