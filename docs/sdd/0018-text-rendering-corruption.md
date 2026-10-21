# 0018 — Text rendering investigation handoff

Status: transferred to QMarkdown; upstream qualification pending; no viewer dependency adoption.
Authorization: the owner authorized the renderer investigation and Feature 064 transfer on 2026-10-21,
then requested repository cleanup and a commit of the necessary md-viewer documentation.

## Outcome and ownership

The reported text corruption was reproduced in a public QMarkdown host. Controlled experiments
isolated horizontal glyph breaks to nearest-filtered paragraph textures at fractional physical-pixel
vertical origins. Rendering and geometry corrections belong to QMarkdown; no application workaround
or renderer-private API integration was added to md-viewer.

The hybrid renderer candidate and tests now live in `/home/andrii/Projects/pet/qmarkdown`, on branch
`feat/hybrid-text-rendering`, based on `bbb0d0c1985567c4f1a7b71f82445c8fafd6ccf7` at transfer.
Continue with `specs/064-hybrid-text-rendering/handoff.md` in that repository and its existing workflow.
Feature 064 owns further implementation and qualification. This transfer does not adopt the candidate:
md-viewer retains its pinned QMarkdown dependency and public API boundary.

## Qualification boundary

Historical Qt 6.12 provider, consumer, native and fixed replay checks are recorded in Feature 064.
Exact Qt 6.8 qualification remains unverified, and syntax-enabled packaging gates fail on a
cross-thread highlighting warning. Host Syntax Highlighting 6.31 differs from pinned CI 6.10;
the warning's root cause remains unresolved. Replay performance, compatibility and perceived shimmer
remain separate conclusions; shimmer and owner visual acceptance remain unassessed.
The separate historical Qt Text glyph symptom also remains unresolved.

These are historical results from md-viewer diagnostic workspaces, not builds of the transferred
checkout. They do not establish readiness for upstream integration, release or viewer adoption.

## Evidence and preservation

QMarkdown's ignored `build/diagnostics/hybrid-064-handoff/` contains verified copies of the Feature 064,
V9 and V10 report archives, checksum companions and the extracted Feature 064 report. Its handoff
records source identities, checksums, successful/failed checks, exclusions and continuation boundaries.

Original checkouts, measurements and archives remain under this repository's ignored
`build/diagnostics/`. Exact pre-cleanup copies of the full SDD, runtime evidence, unsent report drafts
and roadmap are in `build/diagnostics/sdd0018-docs-before-cleanup/`, with `SHA256SUMS`.
Those local artifacts are not stored in Git; retain them before deleting diagnostics. The obsolete
report drafts and detailed upstream investigation log are no longer maintained as viewer documentation.

## Completion and verification

- [x] Transfer candidate implementation, tests and specifications to QMarkdown with a self-contained handoff.
- [x] Verify source equivalence, archive checksums, extracted manifest and documentation links at transfer.
- [x] Preserve original evidence and installed packages; leave viewer dependency files unchanged.
- [x] Consolidate viewer documentation and preserve exact pre-cleanup copies with checksums.

Cleanup verification checks the saved copies, local documentation links and Git whitespace.
No application code or tooling changed; runtime tests and `task check` are not rerun for this prose-only
cleanup. Native and hosted results remain historical. Any future viewer adoption requires a separate
reviewed dependency update after upstream qualification.
