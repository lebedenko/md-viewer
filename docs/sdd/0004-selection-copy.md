# 0004 — Selection and copy on real documents

Status: Complete
Authorization: Accepted plan 2026-10-16 for the next Milestone 1 roadmap item.

## Problem and outcome

Selection and code-copy were wired but only checked with tiny inline strings and no clipboard assertion. After this
change a realistic document is loaded through the application, selected and copied, and the system clipboard content is
verified; native interaction is checked separately.

## Scope and non-goals

- Copy semantics (plain text, markers removed, list bullets, tab-separated tables) belong to qmarkdown; the application
  owns only the clipboard write.
- Non-goals: custom copy formats, rich-text/HTML clipboard, primary-selection behavior beyond what Qt provides.

## Design and decisions

- Fixture `tests/data/real-document.md` (nested lists, fenced code with indentation and a tab, table, task list,
  blockquote, CJK/emoji, long wrapped line), exposed to tests as `MD_VIEWER_TEST_DATA`.
- The test loads the fixture through `Main.qml` and `DocumentSession`, calls the public `selectAll` and `requestCopy`
  on the named `MarkdownView`, and reads `QGuiApplication::clipboard()`.
- The code-block "Copy code" button is a delegate inside qmarkdown; reaching it would require child-tree access, which
  is not allowed, so it is verified manually.

## Acceptance criteria

- [x] Copying a whole real document yields rendered text without Markdown markers, preserving code indentation, tabs,
  Unicode, table cells and wrapped paragraphs (no hard breaks).
- [x] The text reaches the system clipboard through `Clipboard`.
- [x] Native: drag/double/triple-click selection, Ctrl+C paste into another application, "Copy code" button.

## Tasks

- [x] Fixture and GTest.
- [x] Native manual checks.
- [x] Tick ROADMAP once native checks pass.

## Verification

2026-10-16, Qt 6.12.0, offscreen: `ctest -R ReaderIntegration` passed (3 tests). Observed copy output: bullets as `•`,
nested bullets indented, table cells tab-separated, task markers `[x]`/`[ ]` retained, strikethrough/emphasis markers
removed. `task check` passed (format, tidy, qmllint, debug and release builds, all CTest cases, install smoke test).

Native: owner confirmed manual verification passed (2026-10-16): mouse/keyboard selection, Ctrl+C paste and the "Copy code" button.

## Open decisions

None.
