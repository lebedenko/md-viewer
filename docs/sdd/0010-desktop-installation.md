# 0010 — Desktop installation

Status: Implemented; automated verification passed; native desktop review pending
Authorization: Owner request to implement the accepted rename and desktop installation plan, 2026-10-17.

## Problem and outcome

Provide an mdv executable and a discoverable Markdown Viewer desktop launcher, with explicit system and
per-user Release installation. Previously only application staging was available.

## Scope and non-goals

Rename the executable without an md-viewer alias; retain display name, MdViewer module and configuration paths.
Install the supplied SVG and desktop entry. Dependencies remain external at their selected locations.
No MIME default changes, dependency acquisition, bundling, uninstall, removal of old installations, commits or publishing.
Native desktop review and release packaging remain separate work.

## Design and decisions

CMake owns relative GNUInstallDirs destinations, resource embedding, selected QMarkdown install RPATH and cache refresh.
The existing public QML import root remains embedded. Qt public QGuiApplication APIs set the desktop filename to mdv,
display name to Markdown Viewer and window icon from the embedded SVG. The Qt SVG icon plugin is required at runtime.
Task builds Release without privilege before system installation with sudo; local installation uses ~/.local.
Cache tools are optional; available-tool failures propagate. Cache paths honor both --prefix and DESTDIR.
The desktop entry opens one local file (%f), advertises text/markdown and uses the installed themed icon.

References: [Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry/latest-single/) and
[Qt application identity](https://doc.qt.io/qt-6/qguiapplication.html).
The short mdv identity is the explicitly selected launcher name; a stable reverse-DNS release identity remains future work.

## Acceptance criteria

- [x] mdv executable, startup check and current run documentation; no compatibility alias.
- [x] Embedded window icon, desktop filename and relative launcher/icon installation destinations.
- [x] Explicit system/local Release tasks and build/install staging.
- [x] Desktop validation, both staged layouts, repeated installation, executable permission and payload checks.
- [x] Staged help/version/offscreen startup without provider library-path injection.
- [x] Available cache-tool failure propagates; task check passes.
- [ ] Native launcher visibility, window icon appearance and file-manager opening (manual, pending).

## Tasks

- [x] Rename target and add desktop/icon resources and install rules.
- [x] Add installation tasks and extend offline staging acceptance.
- [x] Update architecture, roadmap, dependency and delivery documentation.
- [x] Complete automated verification and record outcomes.

## Verification

2026-10-17, Linux, Qt 6.12.0, selected system QMarkdown library directory /usr/lib:

- Final `task check` passed: formatting, Debug build, all 37 CTest cases, qmllint, clang-tidy,
  Release build and extended installation acceptance.
- `desktop-file-validate` passed for source and installed launchers. It emits an informational category
  hint suggesting Graphics or Office alongside Viewer; the authorized Utility;Viewer; categories are retained.
- Both /usr and /home/test/.local prefixes were installed twice beneath temporary DESTDIR roots.
  Installed desktop/icon bytes, executable permissions and absence of an alias in the install manifest were checked.
- Installed `mdv --help`, `--version` (mdv 0.1.0) and offscreen/software `--smoke-test` passed in both
  temporary layouts and build/install, without injecting QMarkdown's library path. Host LD_LIBRARY_PATH was unset.
- Cache refresh ran successfully. Injecting /bin/false as the available desktop cache tool caused installation to fail,
  as required.
- `task stage` passed. Dry runs of install, install:local and stage confirmed Release configuration/build,
  intended prefixes and sudo only for system installation. This also caught and fixed preset forwarding from build
  to configure.
- `readelf -d build/install/bin/mdv` reported RUNPATH /usr/lib, matching the selected provider.
- Initial acceptance rejected a stale md-viewer from earlier staging. The check was corrected to inspect the install
  manifest rather than reject or remove pre-existing installations; the final full run passed.
- Local documentation links resolve and `git diff --check` passed.

Native launcher visibility, window icon appearance and file-manager opening remain pending.
Hosted CI was updated but not run; native desktop/input checks are not claimed.
No installation into /usr or the real ~/.local is performed during verification.

### Portal registration follow-up

The owner reported a native Qt portal warning after local installation. Read-only inspection found the local
executable and desktop entry present, but the systemd user manager PATH omitted ~/.local/bin.
A focused Python ctypes call to installed GLib's g_desktop_app_info_new("mdv.desktop") returned null with the
service PATH and a valid object with ~/.local/bin prepended, in separate processes.
This explains why terminal execution succeeds while portal application lookup fails.
README now documents the distinction, a current-session service environment refresh and the persistent
desktop login PATH requirement. No user-service environment or system settings were changed.
Native registration after refreshing the environment remains unverified; earlier automated evidence is unchanged.

## Open decisions

None within this scope. Release packaging, licensing and stable release identity remain roadmap work.
