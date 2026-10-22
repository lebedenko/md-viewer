# 0019 — QMarkdown 1.17.1 adoption

Status: Implemented; automated and guided local native acceptance passed; hosted CI pending
Authorization: Owner requested adoption of installed QMarkdown 1.17.1 on 2026-10-22.
Owner subsequently authorized guided local native acceptance using isolated preferences; no hosted runs or publication.

## Problem and outcome

Move the installed-package contract and reproducible preparation baseline from 1.14.0 to 1.17.1.
The provider now supplies hybrid text rendering and the built-in highlighting initialization fix.

## Scope and non-goals

Update CMake, QML imports, the dependency lock and current documentation. Keep compatible later 1.x
acceptance, explicit 2.x rejection, Qt 6.8 minimum, KDE pins and existing reader behavior.
No upstream edits, private APIs, new settings, inline math, footnotes or custom highlighting provider integration.
Acquisition remains explicit and separate from offline configure/build/test.

## Design and decisions

Require package 1.17.1 and QML module 1.17 (QML imports have no patch component).
Reviewed the published archive's release notes for 1.17.0 and 1.17.1: existing public contracts and
SOVERSION 1 remain compatible; new features are opt-in. Hybrid drawing is selected upstream automatically.
The immutable release revision is `da77e3dc00a1b8e16dd2c25e8b519eb34f61191a`, with archive SHA-256
`2c76aeba074e1a2362244ddf5b74eab7d5061b4f4606832971c58848a1201e8d`.
See [upstream notes](https://github.com/lebedenko/qmarkdown/blob/v1.17.1/docs/release-notes-1.17.1.md).
This adoption follows the historical investigation in [0018](0018-text-rendering-corruption.md);
it does not convert historical or upstream evidence into viewer native visual acceptance.

## Acceptance criteria

- [x] Package minimum, QML imports and immutable preparation lock agree on the new baseline.
- [x] Existing consumer tests, QML lint and full task check pass with installed 1.17.1.
- [x] Locked archive preparation and a fresh consumer build pass.
- [x] Native visual/input review, including explicit owner verdict on text integrity and scrolling appearance.
- [ ] Hosted CI run (separate from automated local and native acceptance).

## Tasks

- [x] Review release compatibility and update dependency files and active documentation.
- [x] Run automated acceptance and record actual outcomes.
- [x] Prepare the current Release binary, isolated configuration and native review fixtures.
- [x] Complete guided owner review and record observations for every required native check.

## Verification

The installed `/usr/lib/cmake/QMarkdown/QMarkdownConfigVersion.cmake` reports 1.17.1.
Published tag lookup resolved the revision above; explicitly downloaded its codeload archive and
computed the recorded SHA-256. Native visual/input review passed below; hosted CI has not been run.

2026-10-22, Linux, system Qt 6.12.0, Fusion Controls, offscreen/software:

- Configured Debug and Release with `-DQMarkdown_DIR=/usr/lib/cmake/QMarkdown`.
  `task check` passed: formatting, Debug build, 68/68 CTest cases, QML lint, clang-tidy,
  Release build, desktop validation, isolated system/local installation layouts and staged startup checks.
- `task deps -- --archive /tmp/mdv-qmarkdown-1.17.1.tar.gz` verified the checksum and built/installed
  the locked shared package into `build/deps/prefix`, with math and highlighting enabled and installed KDE.
- Fresh Ninja Debug consumer at `build/qmarkdown-1.17.1-consumer`, BUILD_TESTING=ON, with absolute
  CMAKE_PREFIX_PATH and QMarkdown_DIR selecting `build/deps/prefix`: build, 68/68 CTest cases,
  `all_qmllint` and offscreen `mdv --smoke-test` passed.
- `ldd` confirms `/usr/lib/libQMarkdown.so.1` for Debug and the local prefix library for the fresh consumer.
- Changed-document local links and `git diff --check` passed.
- Logs: `/tmp/mdv19-check.log`, `/tmp/mdv19-deps.log`, `/tmp/mdv19-consumer.log`.
  These local Qt 6.12 checks do not establish hosted Qt 6.8 or native visual/input acceptance.

## Guided local native acceptance — 2026-10-22

Review passed. The first pass had no visual issues; the later Light-theme code-header contrast failure
was fixed in application code and passed owner native retest. All required batches and the final
explicit owner verdict passed; hosted CI remains pending.
Application source revision: `5cf6cff135c4d34c7a6af1f484e1ff585cbdd011` (clean at preparation).
`cmake --build --preset release` succeeded with no work required. Release resolves installed
`/usr/lib/libQMarkdown.so.1.17.1`; system Qt is 6.12.0. Existing automated evidence above is reused:
no application source or dependency changes were needed for initial preparation. The later
application-owned code-header fix and its fresh automated/native verification are recorded below.

Local evidence: ignored `build/diagnostics/qmarkdown-1.17.1-local-acceptance/`, containing build/display
logs, preparation metadata, initial configuration, per-launch logs and supplemental fixtures.
The active `$XDG_CONFIG_HOME/md-viewer/config.toml` was copied to the evidence directory's
`config/md-viewer/config.toml`; native launches use that isolated absolute `XDG_CONFIG_HOME`.
Initial preferences are 100% zoom, wide column, system theme, default font families, with selection,
math, highlighting, height limiting and line numbers enabled; code wrapping is disabled.

Native environment: labwc/Wayland, Fusion Controls, standard Qt platform theme (the inherited
HoloNight platform theme is unset for the process). The Samsung U32J59x display reports 3840×2160
at 60 Hz and compositor scale 1.5. No desktop settings were changed. Baseline launches unset
`QT_SCALE_FACTOR` and `QT_SCREEN_SCALE_FACTORS`, retaining `QT_AUTO_SCREEN_SCALE_FACTOR=1`.
A public `MarkdownStyle.PolishedLight` probe, run offscreen with the standard platform theme,
reports 9 pt Sans Serif resolving to DejaVu Sans and 9 pt monospace resolving to DejaVu Sans Mono.
These are probe results, not a measurement of native font fallback or effective window DPR.

Continuation preparation confirmed the same source revision and original-document checksum; Release
again reported no work to do. The existing review process was reused for the layout batch.
A separate native public-Qt-API probe under the isolated configuration and standard platform theme
reported Wayland window DPR 1.5 (QScreen DPR 2), body font Inter 12 pt resolving to Inter, and code
fonts JetBrains Mono 12 pt resolving to DejaVu Sans Mono. These describe the probe window/style,
not direct instrumentation of the reader window or per-glyph fallback. Evidence is in
`native-probe.cpp` and `native-probe-stderr.log` in the local evidence directory.
Controlled uncategorized and categorized warnings were absent from the first probe's redirected log.
Both appeared with `QT_FORCE_STDERR_LOGGING=1` and
`QT_LOGGING_RULES='*.warning=true;*.critical=true'`; use these process-local settings for subsequent
launches. Launch 1's empty log cannot establish absence of initialization/threading warnings.
The owner reports no font-rendering issues in the layout review, including the explicit Light reader
override. Light code-header contrast failed: the copy icon and language label are nearly invisible,
and the hovered copy control retains dark appearance. System-theme following was not accepted:
the owner observed a dark reader while desktop settings showed light. This run excludes the
HoloNight platform theme, so desktop theme propagation is not established by that observation.

The code-header failure is application-owned: CodeHeader inherited the window palette instead of
the reader override. The authorized small-fix scope binds its standard Controls palette to reader
paper/text/link colors, retaining the window theme and public renderer API. Acceptance requires
visible language text and copy icon in both reader themes, including hover/focus, and preservation
of copy/keyboard behavior. Tasks: implement palette propagation, add a focused regression check,
run `task check`, then repeat affected native checks. All these tasks are complete.

Fix verification: both focused code-header tests passed, followed by full `task check` (69/69
CTest cases, formatting, QML lint, clang-tidy, Debug/Release builds and staged installation checks).
Log: `/tmp/mdv19-header-check.log`. The corrected Release window was opened with the isolated
Light/150% preferences and verified warning-capture settings; see `header-retest.json` and
`native-header-retest.log`. Its SHA-256 is
`58c151a5ef3a1c60ce9fa9be7999be63c43517700436d683ed5fb78c4b6dcc82`.
This is a targeted retest, not one of the initial-preference fresh launches. Launch 1's original
success remains historical evidence. Owner confirmed “yes, all good” for the corrected language label
and copy icon in Light/Dark, including hover and Tab focus: native code-header retest passed.
The completed code-header retest log contains a Qt Wayland text-input `disableSurface` focus warning;
no highlighting initialization/threading warning was observed in that log. Completed fresh-launch
logs 2–5 likewise contain only the same Wayland focus warning; this warning is not evidence of a renderer failure.
Fresh launch 2 passed: owner reports “No issues, works well” after the initial/later-text checklist.
Fresh launch 3 passed: owner reports “no issues” after the initial/later-text checklist.
Fresh launch 4 passed: owner reports “no issues” after the initial/later-text checklist.
Fresh launch 5 passed: owner reports “no issues” after the initial/later-text checklist.
All five original-document launches passed; launch 1 remains evidence from before the header-palette fix,
and launches 2–5 used the corrected binary with initial isolated preferences restored each time.
Owner screenshots are retained only in the ignored evidence directory.

The original `~/Documents/HoloNight/labwc/shell-impl.md` SHA-256 still matches the historical
reproduction: `321bd8673aaa6a765179020ffec1e3370e80a4d90d9c75e45d1518babb599452`.
The supplemental `review.md` selection/Find batch passed: owner reports “All checks passed” and used
Neovim as the scratch editor. This covers drag selection and exact sentinel copy, select-all/copy
including TeX, visible Find highlights, forward/backward cycling through two matches, and formula
restoration after closing Find. Code/task checks also passed: owner reports “all checks passed”.
This covers initial syntax colors,
exact copy of the 80-line C++ block against `fixtures/expected-code.cpp` in Neovim, nested vertical/
horizontal scrolling, wrapping, line numbers, height limiting, and unordered/nested/ordered task indicators.
Navigation/focus passed: owner reports “all checks passed”, covering outline headings, same-document
fragments, relative document/heading links, Back/Forward with restored reading positions, visible
Tab/Shift+Tab focus and keyboard activation, Page Up/Down and Ctrl+Home/End.
Supplemental fixtures include a copy of `tests/data/real-document.md`, block math, task lists,
an 80-line wide C++ block, and relative document/heading links. Original-document captures, if needed,
remain local and unscaled. Still images cannot establish absence of shimmer.

| Required check | Result / owner observations |
| --- | --- |
| Five fresh original-document launches; initial and later text intact | Launch 1 passed: owner reports “No issues observed works well”; launch 2 passed: “No issues, works well”; launch 3 passed: “no issues”; launch 4 passed: “no issues”; launch 5 passed: “no issues” |
| Slow/reverse/fast scrolling and stopped text; explicit shimmer verdict | Passed: final owner confirmation “all checks passed” explicitly covers text integrity and acceptable scrolling appearance, including shimmer. Wheel speed recorded separately |
| 100%, 125%, 150% reader zoom; resize, column widths, light/dark | Passed: layout and code-header retests; final owner confirmation covers normal-scale resizing and all three widths |
| Selection, scratch-surface copy, select all, Find cycling/close, math/TeX | Passed: “All checks passed”; scratch editor Neovim |
| Initial highlighting, code copy, nested scrolling, wrap/line numbers, tasks | Passed: “all checks passed”; exact code-copy comparison in Neovim |
| Outline, relative document/fragment links, history/positions, Tab, paging, Ctrl+Home/End | Passed: “all checks passed” |
| Rendering-only pass with process-local `QT_SCALE_FACTOR=1.25` | Passed on original and supplemental documents, including Light/Dark and 100%/125%/150%. Japanese/emoji retest passed after owner font installation. Probe window DPR 1.875; synthetic scaling only |
| Highlighting initialization/threading log review across completed runs | Passed for all runs with verified capture: no such warnings. Wayland text-input focus warnings remain. Launch 1 capture unverified; its empty log is not counted as warning-free evidence |

Owner observation after launch 1: mouse-wheel scrolling feels 2–3 times slower/less responsive across
their Qt/QML apps than browsers, editors and other Qt/GTK/Java apps. This is recorded separately
from glyph integrity and shimmer; the cause is not yet established. No scrolling behavior or desktop
settings were changed.

Run the full checklist at the native display scale; present short batches and wait for owner responses.
On failure, preserve the smallest reproduction and distinguish application behavior from renderer
geometry. Renderer reports stay local and use public APIs; no sibling provider edits. Application
fixes within this scope require focused checks, `task check`, and repetition of affected native checks.
Local acceptance requires every required check and the explicit owner verdict; hosted CI remains pending.

Synthetic-scale preparation: restored initial isolated preferences and launched the same corrected
Release binary with process-local `QT_SCALE_FACTOR=1.25`. Public Qt probe reports window DPR 1.875
and QScreen DPR 2.5; fonts resolve as in the native baseline probe. Both controlled warnings were
captured. Evidence: `synthetic-scale-probe.log`, `synthetic-scale-launch.json`,
`native-synthetic-scale.log`. These are probe measurements, not direct instrumentation of the reader.
No compositor settings changed; this does not establish compositor-scale or monitor-transfer acceptance.
Original-document synthetic-scale rendering passed: owner reports “no issues” for slow/reverse/fast
scrolling, stopped text, 100%/125%/150% reader zoom, Light/Dark, resizing and all column widths,
including the explicit question about broken glyphs, clipping and objectionable shimmer.
Supplemental synthetic-scale result: owner reports “all good except of japanese and emoji”.
The screenshot shows missing-glyph placeholders for Japanese and the rocket emoji. Fontconfig
reports no installed covering fonts for U+65E5 (日) or U+1F680 (🚀), supporting an environment
font-coverage limitation subsequently resolved by the successful retest below.
Evidence: `owner-missing-glyphs.png` and `missing-font-coverage.txt` in the ignored evidence directory.
The owner installed `noto-fonts-cjk 20240730-1` and `noto-fonts-emoji 1:2.057-1`. Fontconfig now reports coverage for U+65E5
and U+1F680 (`installed-font-coverage.txt`). The agent restarted the same Release binary on
`real-document.md`, preserving isolated reader settings and process-local scale 1.25
(`font-retest-launch.json`, `native-font-retest.log`). Owner confirmed “all good now”; the screenshot
shows intact Japanese text and a colored rocket
emoji (`owner-font-retest-passed.png`). Missing-font coverage is resolved on the unchanged binary.
No system packages or settings were changed by the agent.
Japanese/emoji retest initially passed in the shown Dark/150% configuration. The owner subsequently
confirmed “all checks passed” in response to the final checklist: new glyphs in Light/Dark at
100%/125%/150%, earlier normal-scale resizing and all three column widths, intact text, and acceptable
scrolling appearance including shimmer. This is the explicit final owner verdict for local acceptance.
The font change was checked by native retest; the unchanged application's automated evidence is retained.

The isolated review window is closed. All completed capture-enabled logs were reviewed; they contain
only the recorded Qt Wayland text-input focus warnings, with no highlighting initialization/threading
warnings. `final-log-review.json` records the per-log results. Launch 1 retains its warning-capture
limitation; it is not reported as a warning-free run. No active user preferences were overwritten.
Changed-document local links and `git diff --check` passed after final documentation updates.

Local acceptance is passed for this environment. Hosted CI, desktop-theme propagation through the
excluded HoloNight platform theme, compositor-scale changes and monitor-transfer acceptance are not
established. Slower wheel scrolling remains separate work with no established cause. Unrelated
historical acceptance entries remain unchanged; no publication was performed.

## Open decisions

None for this dependency upgrade.
