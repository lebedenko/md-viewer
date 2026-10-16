# CI and early delivery

## Validation

`.github/workflows/ci.yml` runs on pull requests, pushes to main, and manual dispatch. One Ubuntu job uses Qt 6.8.3,
GTest, clang-format/clang-tidy 18 and the locked qmarkdown source. GitHub actions are pinned to commits.
Dependencies are prepared explicitly before `python3 scripts/check.py` (the same command as `task check`).

Checks: C++ formatting, Debug configure/build, CTest, qmllint, clang-tidy, Release build, application-only staging and
staged offscreen startup. Missing tools/dependencies fail rather than silently skipping checks.
Failed runs upload test/configure logs; successful runs upload the staged application for short-lived inspection.
Artifacts are development outputs tied to the selected Qt/qmarkdown stack, not portable release packages.

There is intentionally no Docker toolchain, large matrix, coverage gate, mandatory self-review, or separate deployment
system at this stage. The CI SDK checks the older supported Qt line; current host Qt provides an additional development
check. Add a second Qt job or sanitizers when failures or compatibility work justify them.
The GitHub repository is active; hosted run evidence is recorded in the
[Foundation SDD](sdd/0002-foundation-baseline.md). Hosted validation of local changes requires pushing them.

## Solo workflow

Use short branches for meaningful features and a PR as a reviewable record when useful. Make `validate` a required check
once hosted CI is proven. A second human approval is not required for a solo project. Existing user authorization is
sufficient for scoped implementation; routine changes need no approval ceremony.

The SDD records why and what; the diff and checks demonstrate the implementation. Small fixes can use an existing spec.
Use Conventional Commits. Keep main buildable. Dependency/security upkeep is configured in `.github/dependabot.yml`
for GitHub Actions; review proposed upgrades and run the same checks.

## Delivery

No automatic release publishing or repository creation is configured. Early delivery is a passing main branch plus
CI artifacts. Before the first public binary release: choose a license and app ID, implement desktop metadata, verify
package dependency ownership/runtime discovery, and exercise install/uninstall on a clean supported system.

Then add a manual release workflow that validates a selected commit, builds tested packages and creates a draft release.
Publishing should be an explicit final action. Avoid a tag-triggered publish job before the package format is settled.
