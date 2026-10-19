# External dependencies

## Policy

Use installed CMake packages and imported targets. Normal configuration, builds and tests are offline and fail clearly
when dependencies are missing. Never silently FetchContent from main, use sibling source trees, or vendor provider internals.
Dependency ownership does not change this rule: qmarkdown is a third-party package here.

| Dependency | Scope | Consumption | Version policy |
| --- | --- | --- | --- |
| Qt | Production | find_package(Qt6 ...); Qt6::* | Minimum 6.8; initial CI SDK 6.8.3 |
| qmarkdown | Production | find_package(QMarkdown 1.14 CONFIG REQUIRED); QMarkdown::QMarkdown | Compatible 1.x; reproducible preparation pins 1.14.0 commit + SHA-256 |
| GTest | Tests only | find_package(GTest 1.12 CONFIG REQUIRED); GTest::gtest | Distro/dev package; excluded with BUILD_TESTING=OFF |
| tomlplusplus | Production (application configuration) | find_package(tomlplusplus 3 CONFIG REQUIRED); tomlplusplus::tomlplusplus | Header-only distro package (`tomlplusplus`, `libtomlplusplus-dev`); no pinned preparation |

Use distro packages for common development dependencies. For an unavailable provider, explicitly build/install a pinned
upstream release into a local prefix. CMAKE_PREFIX_PATH selects that prefix; never modify the global system as a build step.
A package manager such as vcpkg/Conan is unnecessary for this Linux-first scaffold; reconsider if platform coverage justifies it.

## Locked preparation

`dependencies.lock.json` records qmarkdown version, immutable commit, archive URL and SHA-256.
`python3 scripts/deps.py` downloads and verifies it, configures library-only shared mode, builds with Ninja, and installs
under `build/deps/prefix`. Upstream tests/examples are disabled; their qualification belongs upstream. Our tests verify
consumer behavior. CI additionally prepares the pinned KDE dependencies described below. Python >=3.12 supplies safe tar extraction filtering.

Preparation explicitly enables both math and KSyntaxHighlighting. Locally install a compatible KF6SyntaxHighlighting
package (6.10 or newer, built with your Qt). Runtime fallback remains available, but acceptance requires both backends.
For the CI Qt 6.8.3 SDK, use `python3 scripts/deps.py --prepare-kde` to checksum-verify and prepare ECM and
KSyntaxHighlighting 6.10.0 into the dependency prefix before QMarkdown. Offline preparation accepts
`--archive`, `--ecm-archive` and `--syntax-archive` together with `--prepare-kde`.
All three archive hashes are recorded in the lock. KDE preparation skips translation processing and examples;
Qt Linguist tools are not needed. This KDE version is for the Qt 6.8 SDK; use installed compatible KDE with newer Qt. These operations are explicit; app builds remain offline.
See [SDD 0016](sdd/0016-qmarkdown-1.14.md) for the compatibility review and verification.

An offline archive can be supplied with `--archive`. Downloads do not occur if that verified archive is provided.
Source/build directories include the revision; the local prefix is dedicated to this project. After a provider major-version,
Qt or compiler change, use a fresh dependency prefix/build tree rather than mixing incompatible artifacts.
The script reruns the upstream configure/build/install so partial preparation can be resumed.

`CMAKE_PREFIX_PATH` locates CMake packages, while the QML engine also needs the installed QML import root. The scaffold
uses QMarkdown_QML_IMPORT_PATH exported by the provider for both linting and runtime. Shared native libraries must also
be discoverable by the dynamic loader. Build-tree RPATH handles local app builds; CMake generates the selected provider
library directory using `TARGET_FILE_DIR` and records it in the executable's install RPATH.
Staged startup does not inject the provider into `LD_LIBRARY_PATH`. Dependencies must remain at their selected locations.
System packages should install into normal loader/QML paths. The Qt SVG icon plugin is required to render the embedded
SVG window icon; it is a runtime dependency, not a bundled component.

## Upgrades

1. Choose an upstream release and inspect its API/behavior changes and supported Qt/toolchain.
2. Record its immutable revision and verify a new archive checksum; update the lock in the same change.
3. Prepare it explicitly, then run task check and feature-specific regressions.
4. Record the result and any native checks in the relevant SDD. Do not silently follow tags or branches.

System packages can satisfy compatible CMake requirements independently of the preparation lock. The lock controls the
bootstrap/CI baseline, not every accepted local package. CI logs tool and package versions. Ubuntu repositories and pip
transitive packages are not fully locked: this is a small repeatable baseline, not a hermetic build guarantee.
