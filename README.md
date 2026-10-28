# Markdown Viewer

A standalone Qt Quick application for reading and navigating local Markdown documentation.
Built with C++23, Qt 6.8+, and the external QMarkdown 1.18.0 library. Linux is the initial supported platform.

**Current state: local documentation reader.** Open local files from the command line, file dialog or drag and drop;
select and copy rendered text or code, adjust keyboard zoom and column width, and edit preferences in Settings.
Preferences persist in TOML after a 250 ms pause in changes and flush on shutdown.
Live reload, session Back/Forward history, outline, Find, keyboard links and directory-scoped local images are available.
Folder browsing remains planned. Selection keeps eligible formulas rendered and copies TeX; active Find displays TeX source.
There is no HoloNight dependency, embedded editor, or Vim interaction layer.

## Start developing

Prerequisites: CMake >=3.25, Ninja, a C++23 compiler, Qt >=6.8 development packages
(Core, Gui, Qml, Quick, QuickControls2, Network), Python >=3.12, and GTest >=1.12.
Install clang-format, clang-tidy, desktop-file-utils (for desktop-file-validate) and optionally [Task](https://taskfile.dev/).
Qt QML development tools (`qmllint`) and runtime Controls modules must be available.

On Arch Linux, install prerequisites explicitly with your package manager:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative gtest tomlplusplus syntax-highlighting clang python go-task
```

Then prepare the external renderer and verify the application:

```sh
task deps       # Explicit network operation; locked qmarkdown -> build/deps/prefix
task check      # Configure, build, test, lint, Release staging and startup; offline
task run
```

Task is a convenience wrapper. Equivalent essentials:

```sh
python3 scripts/deps.py
python3 scripts/check.py
build/debug/mdv
```

`task deps -- --archive /path/to/archive.tar.gz` prepares from an offline archive and still verifies its checksum.
Select a non-system Qt SDK using the `CMAKE_PREFIX_PATH` environment variable before both preparation and builds.
Use compatible Qt/toolchains for qmarkdown and the application. Build directories are specific to a toolchain;
use fresh directories when switching Qt or compiler versions.

An installed shared QMarkdown 1.18.0 or compatible later 1.x package can replace `task deps`:

```sh
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/provider/prefix
cmake --build --preset debug
ctest --preset debug
```

See [dependency policy](docs/DEPENDENCIES.md) for runtime QML discovery and system-package use.
`task stage` installs only the app under `build/install`; this is not a self-contained distribution.
Application licensing and release packaging remain decisions for the first packaging milestone.

## Common commands

| Command | Purpose |
| --- | --- |
| `task build` | Debug build with tests |
| `task build PRESET=release` | Release build without GTest |
| `task test` | Build and run GTest integration checks and QML startup |
| `task format` / `task format-check` | Format/check application-owned C++ |
| `task tidy` | Analyze application-owned C++ using the debug compilation database |
| `task qml-lint` | Run Qt's generated QML lint target |
| `task check` | Local acceptance sequence, also used in CI |

`.clangd` uses `build/debug/compile_commands.json`. `.clang-format` and `.clang-tidy` were copied from
HoloNight Viewer; `.clangd`, test lint exceptions, ignores and Task naming follow its conventions.
No machine-specific paths or HoloNight runtime imports are carried over.

## Project guide

- [Architecture](docs/ARCHITECTURE.md): implemented structure and planned boundaries.
- [Roadmap](docs/ROADMAP.md): delivery order, renderer prerequisites and deferred scope.
- [SDD workflow](docs/sdd/README.md): one short specification per meaningful feature.
- [Scaffold specification](docs/sdd/0001-scaffold.md): scope and verification evidence.
- [CI and delivery](docs/CI.md): one validation job and manual early delivery policy.
- [Mockup notes](docs/mockups/README.md): visual references, not a feature contract.
- [Agent instructions](AGENTS.md): project-specific collaboration rules.


## Desktop installation

`task install` builds Release as the current user, then uses sudo only to install into `/usr`.
`task install:local` builds Release and installs into `~/.local` without sudo.
Equivalent commands:

```sh
cmake --preset release
cmake --build --preset release
sudo cmake --install build/release --prefix /usr
# Or, for the current user:
cmake --install build/release --prefix "$HOME/.local"
```

Each prefix receives `bin/mdv`, `share/applications/mdv.desktop` and
`share/icons/hicolor/scalable/apps/mdv.svg`. `task stage` uses `build/install`.
Desktop and icon caches are refreshed when the corresponding utilities are available; utility failures fail installation.
Ensure `~/.local/bin` is on the **desktop session's PATH** for local launcher/file-manager use.

Qt runtime libraries, platform and Quick Controls plugins, the Qt SVG icon plugin (normally supplied by Qt SVG,
for example Arch's `qt6-svg`), and the selected shared QMarkdown library/QML module must remain installed.
The executable records the selected QMarkdown library directory in its install RPATH and its QML import root.
This is an application-only install, not a relocatable bundle.
The display name stays “Markdown Viewer”; the QML module and existing configuration paths stay unchanged.
No default MIME associations are changed. No `md-viewer` alias is installed, and older installations are not removed.


If Qt reports `Could not register app ID: App info not found for 'mdv'` after a local install,
check the desktop service environment with
`systemctl --user show-environment | grep '^PATH='`. GLib rejects a desktop entry when its Exec executable
cannot be found in that process's PATH, even when the desktop file exists.
A terminal-only PATH addition does not update an already running portal.
For the current systemd user session, run from a terminal where `mdv` resolves:

```sh
systemctl --user import-environment PATH
systemctl --user restart xdg-desktop-portal.service
```

For subsequent logins, include `~/.local/bin` in your desktop session's startup environment, then log out and back in.
These commands change the running user-service environment; installation does not apply them automatically.

## Reader features

Ctrl+F opens Find; Enter/Shift+Enter and F3/Shift+F3 move between results. Escape closes Find and
retains query/case. Ctrl+Shift+O toggles the outline. Alt+Left/Right navigate session history.
Ctrl+A enables text selection and selects the document. Tab traverses document links when the reader has focus.
Selection keeps eligible formulas rendered and copies their TeX. Active Find displays TeX source.

The `[reader]` TOML booleans `selection_enabled`, `math_enabled`, `highlighting_enabled`, `line_numbers`, and
`height_limiting` default to `true`; `code_wrapping` defaults to `false`. Settings exposes all six.
Code blocks show language/Copy controls and occupy at most half the viewport within 8–30 line limits.
Missing optional provider backends retain source/plain-code fallback and are explained in Settings.
Local images are restricted to the loaded file's directory; remote images are denied.

Ctrl+E (toolbar Edit) opens the current local file in an editor. By default the desktop's handler is used; to choose one,
set an argument array without a shell, where `{file}` is replaced inside a single argument (the path is appended if absent):

```toml
[editor]
command = ["code", "--goto", "{file}"]
```

Only launch failures are reported (status bar); the editor's later exit status is not observed. Terminal editors need a
terminal in the command, e.g. `["kitty", "nvim", "{file}"]`.

## Header configuration

The selected base is `$XDG_CONFIG_HOME/md-viewer/config.toml` (absolute XDG only), falling back to
`~/.config/md-viewer.toml`. With no existing candidate the first candidate is used for future saves.

```toml
[header]
show_title = true
show_close_button = true
```

Title controls both filename and toolbar path; native window title/decorations remain independent. Both values
are read-only in Settings. A desktop override beside the base, such as `config-hyprland.toml` (or
`md-viewer-hyprland.toml` for the legacy base), can override either header key. Colon-separated
`XDG_CURRENT_DESKTOP` tokens are captured at launch, ASCII lowercased and tried in order. Empty tokens are skipped;
names outside letters, digits, `_` and `-` warn and are skipped. Only the first existing override is selected,
even if malformed or unreadable. Invalid/missing values inherit per key; malformed/unreadable files contribute nothing.
Header edits, atomic saves, creation/deletion and directory replacement reload after a 100 ms debounce without
changing reader preferences. Settings saves preserve `[header]` and never write desktop override files.

The outline starts enabled, appears on the right when headings exist and retains its toggle for the current run.
Arrow keys move between rows; Enter/Space activate. Its accent marks the last activated outline heading and clears
when document content changes. Ctrl+Shift+O toggles it; Ctrl+F finds and Ctrl+E opens the external editor.
