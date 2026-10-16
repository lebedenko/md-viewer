# Markdown Viewer

A standalone Qt Quick application for reading and navigating local Markdown documentation.
Built with C++23, Qt 6.8+, and the external QMarkdown 1.10 library. Linux is the initial supported platform.

**Current state: scaffold.** The application displays a bundled welcome document and supports selection and
copying. File opening, reload, history, outline, search, and folder browsing are planned, not implemented.
There is no HoloNight dependency, embedded editor, or Vim interaction layer.

## Start developing

Prerequisites: CMake >=3.25, Ninja, a C++23 compiler, Qt >=6.8 development packages
(Core, Gui, Qml, Quick, QuickControls2, Network), Python >=3.12, and GTest >=1.12.
Install clang-format, clang-tidy and optionally [Task](https://taskfile.dev/).
Qt QML development tools (`qmllint`) and runtime Controls modules must be available.

On Arch Linux, install prerequisites explicitly with your package manager:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative gtest clang python go-task
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
build/debug/md-viewer
```

`task deps -- --archive /path/to/archive.tar.gz` prepares from an offline archive and still verifies its checksum.
Select a non-system Qt SDK using the `CMAKE_PREFIX_PATH` environment variable before both preparation and builds.
Use compatible Qt/toolchains for qmarkdown and the application. Build directories are specific to a toolchain;
use fresh directories when switching Qt or compiler versions.

An installed shared QMarkdown 1.10 or compatible later 1.x package can replace `task deps`:

```sh
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/provider/prefix
cmake --build --preset debug
ctest --preset debug
```

See [dependency policy](docs/DEPENDENCIES.md) for runtime QML discovery and system-package use.
`task stage` installs only the app under `build/install`; this is not a self-contained distribution.
Application licensing and distribution metadata remain decisions for the first packaging milestone.

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
