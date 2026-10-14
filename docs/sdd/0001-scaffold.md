# 0001 — Independent project scaffold

Status: Complete
Authorization: Owner requested this scaffold on 2026-10-14, accepting standalone Qt/qmarkdown architecture.

## Outcome and scope

Create an independent project outside the HoloNight umbrella with a runnable native Markdown shell, external dependency
policy, development tooling, early GitHub validation files and a small SDD workflow. No full reader features are implied.
HoloNight libraries and embedded Neovim are excluded; Vim navigation is deferred until all other work stabilizes.

## Design

One C++ executable and its QML module; qmarkdown remains an installed shared external package. GTest is test-only.
The bootstrap pins the provider by commit and checksum. Configure/build/check never acquire dependencies.
C++ conventions come from HoloNight Viewer dotfiles without its runtime dependencies.
CI uses one Ubuntu/Qt job and the same check script as local development. Publishing is deferred.

## Acceptance and tasks

- [x] Requested source/build/tooling/doc structure exists.
- [x] Renderer is a third-party package, without HoloNight coupling or sibling source references.
- [x] Architecture, ordered roadmap, SDD template and dependency/CI guidance describe agreed scope.
- [x] Both mockups are copied with explicit reference-only notes.
- [x] Debug/Release builds, GTest, offscreen startup, format, tidy, QML lint and staged startup pass locally.
- [x] Workflow YAML parses and repository links resolve.

## Verification

Verified on 2026-10-14 in the final project directory:

- `python3 scripts/deps.py`: verified the locked archive and built/installed shared qmarkdown 1.7.0.
- `task check`: passed Debug and Release Ninja builds, 2/2 CTest tests, clang-format, clang-tidy,
  all_qmllint, local staging and installed executable startup.
- Environment: Linux amd64, Qt 6.12.0, GCC 16.2.1, GTest 1.18.0, clang-format/clang-tidy 23.1.1.
- Task dry runs checked Debug/Release selection and runtime argument routing.
- Workflow/Task YAML, CMake preset and dependency JSON, Python syntax and relative documentation links checked.
- No HoloNight package is required; automated runtime checks use Fusion and clear the platform-theme override.

Hosted Ubuntu/Qt 6.8.3 execution and native visual/input review have not been performed.
The staged executable is not a portable package. Release packaging and license selection remain future work.
