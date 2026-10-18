# 0015 — Reader rendering polish

Status: Implemented; automated acceptance passed; native review pending
Authorization: Owner request to implement the accepted reader rendering polish plan, 2026-10-18.

## Problem and outcome

Use the active Controls checkbox artwork for read-only tasks and compact icon-based code headers.
Prepare upstream requirements for task marker placement and spacing without changing the provider.

## Scope and non-goals

Unordered task checkboxes should replace bullets upstream; ordered tasks retain numbers. Local work only
supplies decorative artwork and code controls. No dependency upgrade, settings migration, editor or geometry workaround.

## Design and decisions

TaskIndicator follows QMarkdown's documented public Controls indicator adapter. Its hidden, enabled backing
checkbox has no focus or accessibility; QMarkdown owns semantics, geometry and its single read-only node.
Reader palette and body font style the artwork. CodeHeader places the optional language label on the left and
the 16-pixel copy SVG button at the right edge, with at least an eight-pixel gap, four-pixel button padding and
native keyboard focus. Long labels elide to fit the remaining width. requestCopy remains unchanged.
The public codeBlock.verticalPadding becomes six logical pixels at each end; horizontal padding is unchanged.
Upstream requirements target reviewed QMarkdown 1.13.0 revision 9e07d41da7ecef454a53e8d80a95abb9becc39e9.

## Acceptance criteria

- [x] Decorative task adapter tracks checked/style state with positive preferred size and no input semantics.
- [x] Copy icon is right aligned beside the optional left language label; tooltip, accessible name, keyboard activation and callback work.
- [x] Named MarkdownView exposes the configured adapter and six-pixel code padding through public APIs.
- [x] Upstream requirements delivered separately; no provider changes.

## Tasks

- [x] Implement and register adapter and icon in app/test resources.
- [x] Add focused component and public integration coverage; correct SDD 0014 artwork description.
- [x] Complete task check; native review remains pending.

## Verification

2026-10-18, installed QMarkdown 1.13.0, Qt 6.12.0, Fusion, offscreen/software:

- `task test`: passed 57/57 tests, including independent code-header keyboard/callback, icon resource,
  label order/visibility and accessible-name checks; decorative adapter state, dimensions, ignored accessibility,
  font updates and lack of pointer/keyboard toggle behavior; public MarkdownView adapter/padding configuration.
- `task qml-lint`: passed. `task check`: passed formatting, Debug build, 57/57 tests,
  QML lint, clang-tidy, Release build, desktop metadata, isolated system/local installation, staged installation
  and installed startup smoke checks. No dependency acquisition occurred.
- Local documentation links and `git diff --check` passed.
- Upstream requirements saved to `~/Documents/HoloNight/qmarkdown/qmarkdown-rq-task-list-presentation.md`,
  matching the prepared document. Provider source and dependency lock are unchanged.
- Native checkbox appearance, compact header, tooltip, focus, copy, light/dark, zoom and fractional-scaling review
  was not performed and remains pending. Offscreen input tests do not establish native visual/input acceptance.
- Task-row geometry and unordered marker replacement remain pending upstream.

Follow-up authorization: Owner corrected copy-button alignment to the right and explicitly requested focused tests
only. The independent component test now checks right-edge alignment at two widths and with an empty language,
while retaining keyboard activation and callback coverage. Rebuilt `reader-tests` and ran
`ReaderFeatures.ApplicationOwnedCodeComponentsHonorCallbacks` with offscreen/software Fusion: passed (1/1).
`git diff --check` also passed. Full checks were not rerun for this alignment correction, as requested.
Build log: `/tmp/mdv15-align-build.log`.

Logs: `/tmp/mdv15-{build,test,lint,check}.log`.

## Open decisions

None.
