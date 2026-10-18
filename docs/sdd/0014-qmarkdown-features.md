# 0014 — QMarkdown reader features

Status: Implemented; automated acceptance passed; full native input and hosted checks pending
Authorization: Owner request to implement the accepted QMarkdown 1.13 reader feature plan, 2026-10-18.

## Problem and outcome

Expose the installed renderer's public math, highlighting, code controls, Find, outline, link navigation and
local-image capabilities in the standalone reader.

## Scope and non-goals

Selection defaults on; selection and active Find display math as TeX source. Persist six reader booleans:
selection/math/highlighting/line numbers/height limiting default true; code wrapping defaults false.
Code height is half the viewport clamped to 8–30 lines. Find retains query and case when closed.
Outline starts hidden. Conventional shortcuts, selection edge scrolling and focused links remain public API clients.
History is session-only, positions proportional. Markdown links open internally, HTTP(S) through the desktop;
unsupported destinations and missing headings report errors. Failed loads retain content/history.
Authorize only the loaded document's directory for local images; remote images remain denied.
No upstream edits, private APIs, custom fenced renderer, embedded editor, Vim controls or inline math.

## Design and decisions

NavigationController owns DocumentSession, URL resolution, session history and restoration requests.
ReaderPane owns Find/outline presentation, viewport clamping, code headers and Controls scrollbar adapter.
AppConfig persists preferences. Settings remains scrollable and describes missing backends.
Retain pinned 1.13.0 and compatible 1.x acceptance; explicit dependency preparation enables math and highlighting.
Normal configure/build/test stay offline. CI's Qt 6.8 SDK requires compatible pinned KDE 6.10 preparation.
KDE archives are independently checksum pinned; offline archives are accepted. Preparation skips translations and
examples and uses the project prefix. Newer Qt installations use their installed compatible KDE backend.

A load generation belongs to DocumentSession; a separate QML restoration intent invalidates deferred layout work
on another load/navigation. Manual scrolling stops position restoration and suppresses refreshed navigation
rectangles until another Find, outline or keyboard navigation action. History commits successful loads only;
current-document fragments are validated synchronously before closing Find or committing a history entry.
Back/Forward restores the saved fraction rather than re-resolving the entry's original fragment.
QMarkdown receives encoded fragments with a leading `#` and owns slug resolution/decoding.
The reader is a Tab focus stop, and Qt's all-controls Tab focus policy includes document links.
CodeHeader and CodeScrollBar are application-owned adapters tested independently; no renderer child traversal.

## Acceptance criteria

- [x] Preferences validate and persist with specified defaults; Settings and toolbar control the renderer.
- [x] Find, outline, keyboard links, selection edge scrolling and clipboard use public APIs.
- [x] History, failures, fragments, competing loads and reloads retain valid positions and state.
- [x] Local images are directory scoped; escapes and remote images remain denied.
- [x] Fully enabled system and fresh pinned provider acceptance passes.
- [x] Native visual/input and hosted results recorded separately (pending checks explicitly listed below).

## Tasks

- [x] Implement preference plumbing and reader controls.
- [x] Introduce internal navigation controller and viewport integration.
- [x] Add focused acceptance coverage and backend preparation.
- [x] Update architecture, configuration, dependency instructions and roadmap.
- [x] Run focused checks and task check; document limitations.

## Verification

2026-10-18, Arch Linux, Qt 6.12.0, Fusion Controls, offscreen/software:

- Focused automated tests cover preference defaults/type validation/persistence, settings and selection/Find
  transitions, actual literal Find results, public headings/fragments, keyboard link activation through a stubbed
  launcher, proportional history restoration, failed/competing loads, missing targets and selection-scroll clamping.
- Temporary PNG fixtures exercise admitted local images, traversal/symlink escapes, remote denial and a changed
  document directory through the named MarkdownView's public size and resource-policy contract.
- Code headers and scrollbar callbacks are exercised as independent application components; clipboard checks compare
  the complete selected payload. Both backend availability flags are required by acceptance.
- Explicit pinned QMarkdown preparation with math and highlighting succeeded against installed compatible KDE.
  A separate fresh provider build/install also succeeded into `build/deps/fresh-prefix`.
- KDE archive downloads matched the locked SHA-256 values; ECM prepared successfully. Supplied offline archive
  preparation reached the syntax backend build. KDE 6.10 itself does not build against local Qt 6.12 due to its
  upstream `_sv` literal conflict; this path is reserved for CI's Qt 6.8.3 SDK. No provider sources were edited.

Native Wayland visual review: launched the system-provider build with a temporary document and isolated settings
(selection disabled). The captured native window shows rendered math, highlighted C++, language/Copy header,
line numbers and QMarkdown fallback task artwork (not native Controls indicators). Interactive nested scrolling, code-copy input, light/dark and Settings
scrolling remain pending. A compositor focus command failed, so no native keyboard-input success is claimed.
Native screenshot and temporary fixtures remain under `/tmp/mdv14-native*`; they are not repository artifacts.
Hosted CI, including the Qt 6.8.3 KDE preparation path, has not been run.

System-provider acceptance: `task check` passed, including formatting, Debug build, 55/55 CTest cases,
QML lint, clang-tidy, Release build, desktop metadata validation, isolated system/local installation checks,
staged installation and installed startup smoke tests. After the last outline restoration guard, a focused rebuild,
55/55 CTest rerun and QML lint also passed. `ldd` confirms `/usr/lib/libQMarkdown.so.1` and installed compatible KDE.

The separate fresh pinned consumer (`build/features-pinned-consumer`) also passes build, 55/55 CTest cases and
QML lint. A complete independent `task check` also passed from an identical application source snapshot under
`/tmp/mdv14-pinned-check-hpa61ib3`; its dependency prefix points only to the fresh pinned provider. Both backend
CMake options are ON. `ldd` confirms the fresh provider, with installed compatible KDE.

Local documentation links, Python syntax compilation and `git diff --check` pass. Temporary logs are
`/tmp/mdv14-{system-acceptance,system-focused-final,pinned-focused,pinned-acceptance,fresh-provider,kde-offline}.log`.
Independent pinned acceptance passed the same complete sequence: formatting, Debug/Release builds, 55/55 tests,
QML lint, clang-tidy, metadata, isolated installation layouts, staging and installed smoke tests. The checked
application source snapshot was hash-compared with the workspace and is identical. No dependency downloads occurred
during either acceptance run. Full native input/appearance review and hosted Qt 6.8.3 CI remain pending.

## Open decisions

None; accepted plan defines the scope.
