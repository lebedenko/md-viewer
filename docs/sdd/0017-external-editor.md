# 0017 — External editor action

Status: Implemented and automated checks verified; native checks pending
Authorization: Owner request to plan and implement the next roadmap item; plan accepted (argv-array configuration, desktop-default fallback).

Correction authorization: Owner requested implementation of the accepted hidden-error feedback plan (2026-10-20).

## Problem and outcome

Live reload ([0012](0012-live-reload.md)) shows external saves, but the viewer could not launch an editor. A toolbar Edit
button and `Ctrl+E` now open the current local file in a configured editor, or the desktop's default handler.

## Scope and non-goals

- `[editor] command = ["prog", "arg", "{file}"]` in the TOML configuration. `{file}` is replaced inside each element;
  without it the path is appended. Invalid values (wrong type, empty array, empty or non-string element, unknown key)
  warn and fall back to the desktop default.
- No command: `QDesktopServices::openUrl`. Launch failures (missing program, openUrl refusal, non-local URL) appear in the status bar and clear when that component clears its error.
- The editor starts detached with the document's directory as working directory. The action is disabled for the bundled sample.
- Non-goals: shell strings, editor setting in the Settings window, line/column jump, embedded editor, Vim behavior,
  observing the editor's exit status, terminal-editor wrapping.

## Design and decisions

- `EditorLauncher` (src/editorlauncher.*) owns process launching; `AppConfig::editorCommand` is read-only (edited in TOML).
- `buildInvocation` is pure: arguments are never concatenated and re-split, so spaces or shell metacharacters in the path
  remain a single argument. Document content never reaches the command line; only the opened file's path does.
- Existing `save()` edits the parsed document, so `[editor]` survives preference saves.

## Acceptance criteria

- [x] Placeholder substitution, append, metacharacter and empty-command behavior (`EditorLauncher` tests).
- [x] A configured script receives the exact argv and document directory; a missing program sets an error that clears on success.
- [x] Invalid `[editor]` values warn and fall back; `[editor]` survives a settings save (`AppConfig` tests).
- [x] `Ctrl+E` and the button are inert for the bundled sample and launch the editor for an opened file (`ApplicationEditor`).
- [ ] Native: real editor from the button and `Ctrl+E`, bad command message, save and live reload (pending).

## Tasks

- [x] Config parsing, `EditorLauncher`, QML wiring, CMake, test registration.
- [x] Tests, README, roadmap and architecture notes.

## Verification

2026-10-19, Qt 6.12, offscreen: `task check` passed (format, build, tidy, qmllint, CTest including the new tests, install
smoke). Native and hosted checks were not run. The desktop-default branch (`openUrl`) is not exercised by automated tests.

## Hidden error feedback correction

The footer tracks the most recently reported failure and its originating component instead of prioritizing stale
navigation errors. NavigationController and EditorLauncher emit `errorOccurred` on every failure, including identical
repeats; DocumentSession reports load/reload failures through `stateChanged`. Clear notifications dismiss only the
originating component's displayed failure. Older failures never return. Unrelated successes and notifications leave
the latest failure visible. Warning and document-path fallbacks remain unchanged. Both Edit handlers qualify
`window.session.url`. Configuration, detached launching and desktop defaults are unchanged.

Acceptance and tasks:

- [x] Implement latest-failure tracking and per-operation signals.
- [x] Cover heading then editor failure, editor then failed open/reload, and identical repeated failures.
- [x] Cover originating recovery, unrelated notifications/success, and no revival of older errors.
- [x] Exercise toolbar and Ctrl+E activation in offscreen application tests.
- [x] Run focused checks and `task check`; record results below.
- [ ] Native visual/input check (separate from offscreen automation).

2026-10-20 correction verification, Qt 6.12, offscreen/software/Fusion:

- Focused CTest (`ApplicationEditor|EditorLauncher|Navigation`): 13/13 passed.
- `task check`: passed; formatting, Debug build, all 68 CTest cases, qmllint (no warnings), clang-tidy,
  Release build, desktop validation and staged installation smoke checks completed.
- `git diff --check`: passed. Existing worktree changes preserved.
- Native visual/input and hosted checks were not run. Toolbar coverage invokes its clicked signal; Ctrl+E uses
  offscreen key input. Desktop-default launching remains outside automated coverage.
