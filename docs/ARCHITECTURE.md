# Architecture

## Product boundary

A Linux-first standalone browser for local Markdown documentation. Qt/QML supplies the user interface;
qmarkdown supplies Markdown semantics, rendering and selection. There are no HoloNight libraries or services.
External editing is planned through a configured editor; embedded editing is outside scope.

## Implemented scaffold

```text
src/main.cpp -> QML engine -> Main.qml -> ReaderPane.qml -> QMarkdown 1.13
                                  |           |
                                  + Clipboard <- copy requests
```

The `mdv` executable owns its MdViewer QML module. Main composes the window and maps the effective Qt palette to
ReaderPane. ReaderPane owns its viewport, rendering style, selection enablement and code-copy controls.
Main also provides a header/footer frame; ReaderPane selects qmarkdown's Polished preset from the system theme,
keeps palette-bound color overrides and enables all GFM extensions.
`AppConfig` reads and atomically writes the TOML preferences (zoom, column width, theme, fonts, spacing, link underline); `SettingsDialog` edits them (SDD 0008). Clipboard owns the operating-system clipboard write. With no file argument the initial document is bundled text. `DocumentSession` loads one local file asynchronously (CLI path, `QtQuick.Dialogs` file dialog or drop; UTF-8 only, 16 MiB cap, last good content kept on failure); see SDD 0003. It also watches the file and its directory and reloads saves after a debounce (SDD 0012).
Images remain denied by qmarkdown's default resource policy. Link navigation is not implemented in the scaffold.
Standard runtime Controls allow user-selected installed styles without a direct dependency on any specific style.

The CMake project consumes installed shared QMarkdown. BUILD_TESTING adds GTest; Release needs no GTest.
The renderer import directory comes from the selected package, not a source checkout.
The installed app records that provider QML path and library directory (install RPATH); a package distributor must build against its final
provider prefix. Relocatable bundles and static-provider deployment are future packaging work.

## Planned application boundaries

| Component | Owns | Does not own |
| --- | --- | --- |
| DocumentSession | URL, source snapshot, load error/state, revision, watching | Markdown parsing or rendered geometry |
| NavigationController | Relative URL resolution, back/forward entries, restoration intent | Parsing headings independently |
| ReaderPane | Viewport, focus, palette/typography, search/outline presentation | File I/O and editor processes |
| Desktop integration | Clipboard, validated browser links, configured editor launching | Shell-interpreted commands from document content |
| FolderModel | Lazy Markdown file discovery and filename filtering | Persistent full-text index or Git state |

Keep these internal initially. Introduce types as features need them; do not create empty controller hierarchies.
Use asynchronous file I/O with revision/generation checks so obsolete work cannot replace a newer document.
Keep logical document position, selection state and viewport offset distinct.

## File and navigation contracts to specify

- Treat the document URL and fragment separately; resolve relative destinations against the document URL.
- Load local Markdown internally, open supported web URLs through the desktop, and explicitly define other schemes.
- History stores a restoration location as well as the document URL; Back must accompany link navigation.
- Watch the file and parent directory, debounce saves, and reattach after atomic replacement.
- Keep the last successfully loaded content during transient save/reload failures and present clear failure state.
- Prefer semantic heading restoration when supported; use a documented proportional fallback otherwise.
- Resolve local image permissions against the opened file/folder scope. Remote resources require a deliberate policy.
- Use one renderer-supported rendered-text coordinate system for search/selection; source offsets are distinct.
- Decide anchor slugging, duplicate-heading behavior and location lifetime with the upstream API.
- Preferences live in a TOML file (`AppConfig`, SDD 0007): `$XDG_CONFIG_HOME/md-viewer/config.toml`, else `~/.config/md-viewer.toml`.

## Renderer prerequisites

QMarkdown 1.13 provides native rendering, optional GFM, selection/plain-text copy and code-copy actions, headings,
fragment navigation, literal Find with rendered highlights, navigation rectangles and opt-in keyboard link traversal.
Application integration for outline, navigation and Find remains in later milestones.
Do not implement regex Markdown parsing or inspect renderer delegates to bridge the gap.

Use an application-local adapter only when needed to translate public qmarkdown APIs into application state.
Do not build a generic multi-renderer abstraction: qmarkdown is the intended renderer.

## Performance and sharing

Measure real documents before optimizing. Initially replace source snapshots on save and debounce updates.
Large-document performance and selection improvements are relevant upstream work; streaming is not required for
saved-file reading. Extension renderers can enable syntax highlighting or diagrams later without viewer-specific syntax.
Share qmarkdown with other applications. Extract reader/controller libraries only after concrete reuse emerges.

## Verification boundaries

GTest checks renderer integration and later application behavior. Offscreen startup catches QML/import failures;
qmllint checks QML statically. Native keyboard, selection, scrolling, light/dark and scaling checks remain manual.
Run clean-environment acceptance without HoloNight plugins/configuration and use public Qt APIs throughout.


Desktop installation uses relative GNUInstallDirs destinations for mdv, its desktop launcher and scalable icon.
Qt sets desktopFileName to mdv and embeds the same SVG as its window icon. System and local Release installation
are described in [SDD 0010](sdd/0010-desktop-installation.md); dependencies remain external.
