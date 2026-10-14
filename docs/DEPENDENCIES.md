# External dependencies

## Policy

Use installed CMake packages and imported targets. Normal configuration, builds and tests are offline and fail clearly
when dependencies are missing. Never silently FetchContent from main, use sibling source trees, or vendor provider internals.
Dependency ownership does not change this rule: qmarkdown is a third-party package here.

| Dependency | Scope | Consumption | Version policy |
| --- | --- | --- | --- |
| Qt | Production | find_package(Qt6 ...); Qt6::* | Minimum 6.8; initial CI SDK 6.8.3 |
| qmarkdown | Production | find_package(QMarkdown 1.7 CONFIG REQUIRED); QMarkdown::QMarkdown | Compatible 1.x; reproducible preparation pins 1.7.0 commit + SHA-256 |
| GTest | Tests only | find_package(GTest 1.12 CONFIG REQUIRED); GTest::gtest | Distro/dev package; excluded with BUILD_TESTING=OFF |
| tomlplusplus | Not currently needed | Future find_package(tomlplusplus CONFIG REQUIRED); tomlplusplus::tomlplusplus | Add only with an actual TOML consumer |

Use distro packages for common development dependencies. For an unavailable provider, explicitly build/install a pinned
upstream release into a local prefix. CMAKE_PREFIX_PATH selects that prefix; never modify the global system as a build step.
A package manager such as vcpkg/Conan is unnecessary for this Linux-first scaffold; reconsider if platform coverage justifies it.

## Locked preparation

`dependencies.lock.json` records qmarkdown version, immutable commit, archive URL and SHA-256.
`python3 scripts/deps.py` downloads and verifies it, configures library-only shared mode, builds with Ninja, and installs
under `build/deps/prefix`. Upstream tests/examples are disabled; their qualification belongs upstream. Our tests verify
consumer behavior. The same command runs locally and in CI. Python >=3.12 supplies safe tar extraction filtering.

An offline archive can be supplied with `--archive`. Downloads do not occur if that verified archive is provided.
Source/build directories include the revision; the local prefix is dedicated to this project. After a provider major-version,
Qt or compiler change, use a fresh dependency prefix/build tree rather than mixing incompatible artifacts.
The script reruns the upstream configure/build/install so partial preparation can be resumed.

`CMAKE_PREFIX_PATH` locates CMake packages, while the QML engine also needs the installed QML import root. The scaffold
uses QMarkdown_QML_IMPORT_PATH exported by the provider for both linting and runtime. Shared native libraries must also
be discoverable by the dynamic loader. Build-tree RPATH handles local app builds; scripts/check.py supplies the local
provider library directory for the staged smoke test. A custom externally installed provider may need its own
LD_LIBRARY_PATH for staging. System packages should install into normal loader/QML paths.

## Upgrades

1. Choose an upstream release and inspect its API/behavior changes and supported Qt/toolchain.
2. Record its immutable revision and verify a new archive checksum; update the lock in the same change.
3. Prepare it explicitly, then run task check and feature-specific regressions.
4. Record the result and any native checks in the relevant SDD. Do not silently follow tags or branches.

System packages can satisfy compatible CMake requirements independently of the preparation lock. The lock controls the
bootstrap/CI baseline, not every accepted local package. CI logs tool and package versions. Ubuntu repositories and pip
transitive packages are not fully locked: this is a small repeatable baseline, not a hermetic build guarantee.
