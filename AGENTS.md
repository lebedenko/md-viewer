# Markdown Viewer

## Scope and ownership

Build a standalone local Markdown reader with Qt Quick Controls and external QMarkdown.
Do not introduce holonight-qt, other HoloNight runtime dependencies, embedded Neovim, or a built-in editor.
Vim-style navigation/selection is a separate future decision after all other planned features stabilize;
it is not part of current implementation. Conventional keyboard access remains required.

Own application loading, navigation, viewport, clipboard/editor integration and settings here.
Markdown semantics and renderer geometry belong upstream in qmarkdown. Do not access QMarkdown.Private,
parser internals or child trees in production code. Tests may identify the application's named MarkdownView.
Treat qmarkdown exactly like any third-party library: installed package, public API, reviewed pinned upgrades.
Never silently edit a sibling provider or download dependencies during CMake configure/build/test.

## SDD workflow

Read docs/ARCHITECTURE.md, docs/ROADMAP.md and the relevant docs/sdd/NNNN-name.md before meaningful work.
Use docs/sdd/TEMPLATE.md for features, architectural changes and cross-cutting fixes.
Record the intended behavior, acceptance criteria, boundaries, tasks and actual verification in one file.
An explicit implementation request or accepted plan authorizes that scope; record it once and proceed.
Routine fixes, tests and implementation details inside an authorized scope do not need repeated approval.
Small bug fixes and documentation/tooling corrections can update the existing spec without a new document.
Raise new product or architectural scope decisions with the owner; do independent authorized work meanwhile.
Keep planned, implemented and verified states distinct. Do not mark manual or hosted checks as passed unless run.

## Structure and conventions

- src/: C++ application entry point and host integration; future non-UI logic remains independently testable.
- qml/: presentation, runtime `QtQuick.Controls as Controls`; qualify control types, enums and attached properties.
- tests/: GTest and bounded integration tests. No placeholder tests asserting constants.
- scripts/: small explicit development/CI commands; prefer standard CMake/CTest tools to custom frameworks.
- docs/: repository documentation is authoritative; mockups are visual references.

C++23, two spaces, 120 columns, Google-based clang-format. CamelCase classes, camelBack functions,
lower_case_ private members. QML uses four spaces. Use public Qt APIs and palette roles, not HoloNight tokens.
Use Conventional Commits (`feat: ...`, `fix: ...`, `docs: ...`). Do not create commits or publish unless requested.
Avoid speculative interfaces, unused dependencies and premature public libraries.

## Verification

Run focused checks while developing and `task check` before handing off a code/tooling change.
For prose-only changes, check links and consistency rather than rebuilding unrelated code.
`task deps` is explicit acquisition, separate from offline `task check`.
Keep upstream sources, generated files and build trees out of formatting/lint scope.
Record native visual/input verification separately from offscreen automated tests.
Do not install system packages, modify system settings, or publish releases as an incidental build step.
