# 0022 — Tabbed settings redesign

Status: Implemented; automated acceptance passed; native and hosted checks pending
Authorization: Owner requested implementation of the accepted tabbed settings plan on 2026-11-02.
Owner requested the screenshot-based layout corrections below on 2026-11-03.

## Problem and outcome

Replace the long preference list with seven categorized pages and expose reader-facing QMarkdown 1.18
options through its public API. Text selection is always available.

## Scope and non-goals

Keep the modal in-window dialog, immediate changes, debounced atomic persistence, error feedback,
and conventional keyboard access. Target 760 × 680 logical pixels, constrained by 16-pixel window margins.
No streaming, injected renderers, resource permissions, editing, or Vim navigation preferences.

## Design and decisions

AppConfig owns an explicit allowlist of options, style groups, syntax roles and Find colors, with types,
limits and tab placement. New settings use nested reader.options, reader.style, reader.syntax and
reader.view tables. Existing keys keep their meanings; unrelated keys survive saves. Legacy
reader.selection_enabled is ignored without warnings and removed on successful save.

ReaderPane resolves preset/application defaults, general font/spacing preferences, detailed overrides,
then font zoom. Grouped properties appear once, avoiding flat aliases. Detailed overrides are shared
between light and dark themes. Automatic style preserves the application palette; five explicit presets
use upstream colors. Automatic syntax follows the reader theme. Reset removes an override.
GFM forces its individual extensions on upstream; disabled individual controls retain their saved values.
Line limits remain ordered, math scale and height ratio are positive. Invalid values warn per key;
malformed files are never overwritten. Header, errors and configuration path stay outside page scrolling.

## Acceptance criteria

- [x] Seven tabs, accessible close icon, Escape, focus return and modal live changes (offscreen).
- [x] Public style groups, syntax roles and reader options persist, validate and reset.
- [x] Preset/theme changes preserve shared overrides and scale resolved fonts.
- [x] Legacy selection false cannot disable selection or copy.
- [x] Malformed-file protection and unrelated-key preservation remain intact.
- [ ] Native scrolling, focus, small-window, dialog, theme and pointer-selection review.

## Tasks

- [x] Implement configuration, renderer bindings and tabbed dialog.
- [x] Add focused automated coverage and run QML lint and task check.
- [x] Update README, architecture and roadmap.

## Verification

2026-11-02, Linux, installed Qt 6.12.0 / QMarkdown 1.18.0, Fusion Controls,
offscreen/software rendering:

- Final focused AppConfig/ApplicationSettings suite passed: 25/25, followed by clean QML lint.
- Full Debug CTest suite passed: 81/81, including every canonical style field and syntax role,
  preset/theme changes, resolved-font zoom, tab keyboard navigation, modal live edits, close/Escape,
  GFM control disabling, live override reset, legacy-false selection/copy, persistence and validation.
- `all_qmllint` passed without warnings after adding an explicit public Flickable cast for focus scrolling.
- Changed-document local links and `git diff --check` passed.
- Full `task check` passed: formatting, Debug build, 81/81 CTest cases, clean QML lint,
  clang-tidy, Release build, desktop metadata validation, isolated system/local installation layouts,
  staged installation and installed startup/help/version checks. No dependency acquisition occurred.
- The final dialog title and test type-annotation changes were followed by a Debug rebuild,
  the 25/25 focused suite and clean QML lint; the full check's clang-tidy, Release and installation
  stages also used these final sources.
- Logs: `/tmp/mdv22-check-final.log` and `/tmp/mdv22-final-focused.log`.

The allowlist contains 14 options, 74 canonical style members, 12 syntax-role colors and two Find colors.
Font strings use Qt's serialized QFont format; colors persist as alpha-preserving #AARRGGBB strings.
Invalid or unknown nested fields warn and retain computed defaults. Reset removes the persisted field;
resetting line limits also clears a conflicting counterpart to preserve the ordered-limit invariant.

Native review remains separate: scrolling, keyboard focus, small windows, font/color dialogs,
light/dark appearance and pointer selection. Hosted checks are not implied by local tests.

## Layout correction — 2026-11-03

The owner screenshots showed inconsistent form spacing, stacked labels, edge-aligned swatches and
large blank areas before Headings and subsequent pages. Inactive legacy Loaders still occupied layout
slots. The correction uses a single form row model for general preferences and detailed overrides:
label, expanding editor and fixed-size reset icon, with 12-pixel row gaps and 16-pixel column gaps.
General preferences now also have reset icons, restoring their existing application defaults.

The dialog has a palette-based rounded background (12-pixel radius). After the owner clarified the
padding scope, the outer horizontal padding again follows the Controls style; only tab content has
24-pixel side padding. The header reserves 12 pixels above and to the right of the close button,
and four below it. The close button retains its accessible name but has no tooltip.
The color swatch sits 12 pixels inside its button;
text reserves 44 pixels on that side. Reset uses the existing IconButton's tooltip and accessible name,
with an application-owned reset SVG. Backend/GFM explanations follow the controls, keeping the first
row aligned across pages. Scrolling, live updates, persistence and focus handling remain in scope.

Verification:

- Focused `ApplicationSettings.*`: 6/6 passed, including a geometry regression checking aligned
  columns, uniform General row gaps, matching page starts, reset icons, swatch insets and containment
  at a 480 × 420 window size.
- Inspected offscreen Fusion captures at 960 × 720 (General and Headings) and 480 × 420 (Headings):
  `/tmp/mdv22-settings-{general,headings,small}.png`. Rounded corners, aligned form columns, inset
  swatches and the removed page-top gap are visible. This is offscreen visual review, not native review.
- Full `task check` passed for this correction: 82/82 tests, clean QML lint, clang-tidy,
  Debug/Release builds, desktop validation, isolated installation layouts and installed startup checks.
  The final geometry-test cleanup was also rebuilt and passed independently.
- Reset SVG XML, SDD links and `git diff --check` passed. Logs:
  `/tmp/mdv22-polish-check-final.log`, `/tmp/mdv22-polish-focused.log` and
  `/tmp/mdv22-polish-final-focused.log`.
- Native review of the owner's installed Controls style remains pending.

## Padding and disabled-hover follow-up — 2026-11-03

Authorization: Owner requested restoration of the original outer side padding, top clearance for the
close button, and no hover background or tooltip for disabled reset buttons.

The custom header now includes its own top inset; Dialog.topPadding alone applies below the header.
Disabled reset icons set hoverEnabled false, clear tooltip text, suppress tooltip visibility, and hide
the style background. Qt can retain its read-only hovered flag when disabling an item already under
the pointer, so background suppression also covers that transition without private APIs.

- Focused settings suite: 6/6 passed, including outer-versus-page padding, close-button top clearance,
  and an enabled/hovered reset becoming disabled while the pointer remains over it. Tooltip visibility
  and background suppression are checked through public properties.
- Inspected the updated offscreen General capture: restored outer margins and header top clearance.
- Full `task check` passed: 82/82 tests, clean QML lint, clang-tidy, Debug/Release builds,
  desktop validation, isolated installation layouts and installed startup checks. The final conditional
  background binding also passed a rebuilt six-test settings suite and clean QML lint, preserving the
  Controls style's own background behavior whenever a reset is enabled.
- Logs: `/tmp/mdv22-insets-check.log`, `/tmp/mdv22-insets-focused-final.log` and
  `/tmp/mdv22-insets-final-validation.log`. Native style review remains pending.

## Close-button follow-up — 2026-11-03

Authorization: Owner requested removing the close tooltip and adding right-edge clearance to match
its existing top clearance. The header alone now reserves 12 pixels on the right; the dialog's
Controls-style outer padding and tab-content padding are unchanged. The close icon keeps its
accessible name and hover feedback, with tooltip text empty and tooltip visibility forced off.

Verification: focused settings suite passed (6/6), and the offscreen General capture shows matching
top/right clearance. Full `task check` passed: 82/82 tests, clean QML lint, clang-tidy, Debug/Release
builds, desktop validation, isolated installation layouts and installed startup checks. Logs:
`/tmp/mdv22-close-focused.log` and `/tmp/mdv22-close-check.log`. `git diff --check` passed.
Native style review remains pending.

## Open decisions

None within the accepted scope.
