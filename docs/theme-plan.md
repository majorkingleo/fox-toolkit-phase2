# FOX theme realization plan

Companion to `docs/modernization-plan.md` (workstream 1). This is the detailed
plan for making `fox2` themeable, including importing colors, metrics, fonts,
and rendering style data (radius, borders, gradients, shadows) from existing
sources such as GTK, KDE, Qt, and the native desktop.

## Goals

- One semantic theme model resolved centrally by `FXApp`, not per widget.
- Byte-for-byte compatibility with today's look by default (the classic FOX
  palette stays the default theme).
- Light, dark, and custom themes, switchable at runtime.
- Import colors/metrics/fonts from existing desktop themes instead of asking
  users to hand-write a FOX theme.
- No new mandatory dependency for the native build.

## Examples used for validation

Two examples drive every phase, always in this order: `hello` first, `button`
second.

- **`tests/hello` (first example)** — one `FXMainWindow` with a single
  `FXButton` that quits on `FXApp::ID_QUIT`. It exercises `FXApp` init, a
  top-level window, a button, and the event loop. It is already inside the
  `fox2` closure and is the pixel-compare target for T1 and T2.
- **`tests/button` (second example)** — a button-tweaking dialog. Besides
  several `FXButton` instances with different styles and icon/text
  justification, it uses `FXToolTip`, `FXStatusBar`, `FXVerticalFrame`,
  `FXVerticalSeparator`, `FXHorizontalFrame`, `FXGroupBox`, `FXCheckButton`,
  `FXRadioButton`, and `FXGIFIcon`. It exercises hover/pressed/checked states,
  frames, and layout, so it becomes the visual target once the chrome seam
  (T2) and the modern rendering (R1-R3) land. It is not in the minimal closure
  yet: Phase 3 adds `tests/button.cpp` and its closure before T2 uses it.

## What exists today (verified in the sources)

FOX 1.7.95 has no CSS or theme engine, but it already has the two pieces this
plan builds on:

- **Twelve application palette colors** in `FXApp`
  (`include/FXApp.h:822-847`):
  `borderColor`, `baseColor`, `hiliteColor`, `shadowColor`, `backColor`,
  `foreColor`, `selforeColor`, `selbackColor`, `tipforeColor`, `tipbackColor`,
  `selMenuTextColor`, `selMenuBackColor`, each with a `get*Color()` and
  `set*Color(FXColor)` pair.
- **Defaults** (`lib/FXApp.cpp:755-766`): `border #000000`, `base #D4D0C8`,
  `back #FFFFFF`, `fore #000000`, `selfore #FFFFFF`, `selback #0A246A`,
  `tipback #FFFFE1`, `tipfore #000000`; `hilite`/`shadow` are derived from
  `base` via `makeHiliteColor()`/`makeShadowColor()`. The Windows path instead
  seeds the palette from `GetSysColor()` (`lib/FXApp.cpp:4402-4409`).
- **Persistence** through the registry, section `SETTINGS`, keys
  `bordercolor`, `basecolor`, `hilitecolor`, `shadowcolor`, `backcolor`,
  `forecolor`, `selforecolor`, `selbackcolor`, `tipforecolor`, `tipbackcolor`
  (`lib/FXApp.cpp:4459-4468`).
- **Parsers already in the toolkit**: `FXSettings`/`FXRegistry` (INI),
  `FXJSON*`, `FXXML*`, and 576 named constants in `FXColors.h`.
- **Drawing primitives** (`include/FXDC.h`, `lib/FXDCWindow.cpp`):
  `fillRectangle`, `fillRoundRectangle`/`drawRoundRectangle` (Windows
  `RoundRect`; X11 `XFillArcs`+`XFillRectangles`/`XDrawArcs`+`XDrawSegments`,
  **hard-edged, not antialiased**), `fillVerticalGradient`/
  `fillHorizontalGradient` (X11: software scanline interpolation capped at 128
  bands; Windows: `GradientFill`), `drawFocusRectangle`, `drawIconShaded`,
  `drawIconSunken`, plus arcs, ellipses, and polygon fills.
- **Alpha support**: `FXImage::hasAlpha()` and `blend(FXColor)`, the
  `IMAGE_ALPHA*` flags, and `xincs.h` already includes
  `X11/extensions/Xrender.h` (Xft text is antialiased). A
  `XRenderCreateLinearGradient` path exists in `FXDCWindow.cpp` but is disabled
  with `#if 0` in favor of the scanline loop.
- **Timing**: `FXApp::addTimeout()`/`removeTimeout()`/`addChore()` plus the
  `animSpeed`/`blinkSpeed` settings, enough for state transitions.

The widgets in the `fox2` subset already draw from the palette
(`getApp()->getBaseColor()`, `getHiliteColor()`, `getShadowColor()`,
`getSelbackColor()`, `getForeColor()`, `getBorderColor()`), so the migration
surface is small at the current subset size.

## Theme model

### Semantic roles

Add `include/FXTheme.h` with a role enum. Every role has a legacy counterpart,
so mapping is mechanical and reversible:

| Role | Legacy source |
|---|---|
| `WindowBg` | `backColor` |
| `Surface` | `baseColor` |
| `SurfaceHilite` | `hiliteColor` |
| `SurfaceShadow` | `shadowColor` |
| `Text` | `foreColor` |
| `TextDisabled` | derived: `foreColor` blended toward `baseColor` |
| `Border` | `borderColor` |
| `SelectionBg` | `selbackColor` |
| `SelectionText` | `selforeColor` |
| `MenuSelectionBg` / `MenuSelectionText` | `selMenuBackColor` / `selMenuTextColor` |
| `TooltipBg` / `TooltipText` | `tipbackColor` / `tipforeColor` |
| `Accent` | `selbackColor` (new role, same default) |
| `FocusRing` | `Accent` (new role) |
| `Button` | `Surface` (new role; Breeze/KDE import their real button face) |
| `HoverRing` | `Accent` (new role; Breeze/KDE import their hover decoration) |

State variants are resolved by `FXTheme` from the base roles rather than stored
individually: `Normal`, `Hover`, `Active`, `Disabled`, `Selected`, `Focused`.
A theme may override any state explicitly; otherwise the role is derived
(darken/lighten by a fixed amount, blend toward `Surface` for `Disabled`).

### Metrics

Also in `FXTheme`, in logical units (see the DPI workstream):

`padding`, `margin`, `borderWidth`, `focusWidth`, `spacing`, `iconSize`,
`smallIconSize`, `rowHeight`, `cornerRadius` (advisory; FOX draws rectangles),
`shadowSize` (the margin a popup carries for its drop shadow, 0 for none).

### Fonts, assets, and surface style

Font roles: `Normal`, `Bold`, `Small`, `Monospace`, plus `titleFont`. Resolved as
`FXFontDesc` strings and created through the existing `FXFont` machinery, so the
Xft/fontconfig cache keeps working. Assets: icon theme name and hint sizes;
FOX-drawn artwork stays untouched.

Colors alone do not produce a modern look, so the theme also carries renderable
style data per role:

- `borderWidth` and `borderColor` (a 1 px hairline is the modern default),
- `cornerRadius` (4-6 px is the useful range),
- `gradient` (two stops plus direction, or none for flat fills),
- `shadow` (offset, blur budget, color, alpha) for popups, menus, and dialogs,
- `focusRing` (width, color, optional halo alpha),
- `stateDelta` (how far `Hover`/`Active`/`Pressed` shift the base color),
- `animationTime` in milliseconds; 0 disables transitions.

Each of these has a concrete rendering path in the next section; none of them is
left as decorative data that FOX silently ignores.

### Ownership and API

```
FXTheme* theme = app.getTheme();          // never null; default theme
FXColor bg   = theme->color(FXTheme::WindowBg);
FXColor bgD  = theme->color(FXTheme::WindowBg, FXTheme::Disabled);
FXint pad    = theme->metric(FXTheme::Padding);
```

- `FXApp` owns one `FXTheme`; `getTheme()` returns the default theme when the
  application never sets one, so existing code and applications are unaffected.
- `setTheme()` copies values into the palette so legacy `get*Color()` callers
  keep working during migration.
- Widget code migrates from `getApp()->getBaseColor()` to
  `getApp()->getTheme()->color(Surface)`. Until a widget is migrated, the palette
  mirror keeps both paths consistent.

## Modern look and feel (rendering)

Where the toolkit stands today, and the path for each visual element:

| Element | Today | Plan |
|---|---|---|
| Flat fills, 1 px borders | supported (`fillRectangle`) | use directly |
| Vertical/horizontal gradients | supported: X11 scanline (≤128 bands), Windows `GradientFill` | use directly; enable the disabled XRender gradient path behind a capability check to remove banding |
| Rounded corners | supported but aliased (core X11 arcs) | antialias by compositing through an XRender mask, with a cached 9-slice corner-bitmap fallback |
| Soft shadows | no primitive | cached 9-slice RGBA shadow via XRender composite; degrade to 2-3 stepped rings without XRender |
| Focus ring | dashed `drawFocusRectangle` | rounded accent ring (stroke plus optional translucent halo) |
| Hover/pressed states | static colors only | color data plus timer-driven interpolation (`addTimeout`, `animationTime`) |
| Text rendering | antialiased via Xft | unchanged |
| Alpha widgets/overlays | `FXImage` alpha and `blend()` | add ARGB32 picture compositing for correct alpha over any background |

Implementation notes:

1. **Antialiased corners.** Rasterize the existing hard-edged arcs once into an
   alpha mask per `(radius,width,height)` bucket, then `XRenderComposite` it.
   Cache by radius and DPI scale; never regenerate per paint. Without XRender,
   blit the cached mask through `FXImage` alpha instead.
2. **Shadows.** Pre-render one RGBA shadow tile per `(radius,blur,alpha)` bucket
   (cheap box blur over a rounded rect) and draw it 9-sliced, so paint cost is
   constant regardless of window size.
3. **Gradients.** Keep the working scanline loop; optionally enable the existing
   `#if 0` `XRenderCreateLinearGradient` code so banding disappears on
   render-capable servers.
4. **Focus ring and states.** Stroke a rounded ring inset by `focusWidth`, or
   composite a translucent halo. `Hover`/`Active`/`Pressed` interpolate from the
   base color over `animationTime` with a repeating timeout; zero disables
   animation entirely.
5. **Cost discipline.** Every derived resource is cached and keyed by
   role/radius/scale; no per-paint allocation and no per-paint parsing.

Checklist to validate against Adwaita/Breeze expectations:

- 4-6 px corner radius on containers and controls, 1 px hairline borders.
- Surfaces flat or with a 4-8% lightness gradient; no 3D bevel.
- Focus ring visible at 2 px with a soft halo; keyboard focus always visible.
- Disabled content blended about 45% toward the surface, not grayed-out text.
- 4 px spacing grid, 24-28 px control height, 16 px default icon size.
- Hover/pressed feedback in 120-150 ms; no hard color jumps.
- Dark theme conveys elevation with slightly lighter surfaces, not borders alone.

## Widget rendering seam

Decision: a theme does **not** replace widget classes. One `FXButton` class keeps
its behavior and all 18 of its message handlers; only chrome drawing moves behind
a style object. `onPaint` stays where it is, collects state, and delegates.

### Why not the alternatives

- **Theme logic inline in `onPaint`** — every widget re-implements radius,
  gradient, shadow, and focus drawing; six widgets produce six dialects, and an
  imported theme cannot vary any of it.
- **`FXThemeButton : FXButton`, instantiated per theme** — the 18 handlers and
  the `STATE_UP`/`STATE_DOWN`/`STATE_ENGAGED` machine live in the base anyway.
  Applications construct `new FXButton(...)` everywhere, so either every call
  site becomes a theme-aware factory or applications stop being theme-neutral.
  Runtime theme switching would then require destroying and recreating widgets,
  losing focus, scroll position, and children. `FXDECLARE`/`FXIMPLEMENT` class
  names feed `getClassName()`, `isMemberOf()`, and serialized layouts, so widget
  identity must not change per theme. Each theme class also adds sources to the
  build and to the closure.
- **`FXButton` as a wrapper owning an inner themed widget** — breaks
  `setTarget()`/`setSelector()` wiring, focus and hotkey routing,
  `getFirst()`/`cast<FXButton>()` semantics, and doubles both widget count and
  event dispatches per control.

The subclass/wrapper approach becomes appropriate only where the interaction
model itself differs — a browser-presented control (workstream 4, deferred) or a
genuine native system control — not for a native theme.

### Contract

`FXApp` owns one `FXStyle` (`getStyle()`); `FXTheme` stays pure data. The style
holds the cached chrome resources. It draws the chrome and returns the content
rectangle, so rounded corners and borders never disturb text/icon layout:

```cpp
struct FXButtonState {
  FXbool enabled;    // Widget is enabled
  FXbool hover;      // Pointer is inside the widget
  FXbool down;       // Button is pressed
  FXbool engaged;    // Button is engaged or checked
  FXbool focused;    // Widget has keyboard focus
  FXbool defaulted;  // Widget is the default button
  FXbool toolbar;    // Toolbar style button
  FXint  border;     // Border width of the widget
  FXuint options;    // Frame options of the widget
  FXColor back;      // Background color of the widget
  FXColor text;      // Text color of the widget
  };

// Chrome: background/gradient, rounded border, shadow, focus ring.
// Returns the rectangle available for icon and label.
FXRectangle FXStyle::drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXButtonState& state);
```

The state struct carries no widget types, so `FXStyle` depends only on `FXDC`
and colors and cannot create dependency cycles with the widgets.

| Chrome (`FXStyle`) | Content (widget) |
|---|---|
| background fill or gradient | icon placement, shaded/sunken disabled icon |
| rounded border, hairline | label layout, alignment, margins |
| hover/pressed color deltas | hotkey underline via `drawLabel` |
| focus ring | press-shift offset |
| default-button ring | state derived from `SEL_UPDATE` |

Rules:

- Hit testing stays rectangular; visual corners do not change the clickable area.
- The classic style calls the existing `drawRaisedRectangle` /
  `drawSunkenRectangle` helpers, keeping T1/T2 pixel-identical.
- Chrome resources are cached in the style, keyed by role, radius, and DPI scale.
- `FXButton::onPaint` remains `virtual`, so applications can still subclass a
  widget and draw fully custom chrome.

The same seam covers `FXFrame` (panel chrome), `FXTextField` (rounded input with
inset border), and later `FXScrollBar`, `FXMenuPane`, and `FXToolTip` (the shadow
consumers).

## Theme file format

Two equivalent inputs, same schema:

- `.foxtheme` INI, read with `FXSettings` (already in the subset, no new code):
  ```
  [colors]
  window_bg=#FFFFFF
  surface=#D4D0C8
  selection_bg=#0A246A
  [metrics]
  padding=2
  border_width=1
  [fonts]
  normal=helvetica-100
  ```
- `.foxtheme.json` via `FXJSON` for generated themes with per-state overrides.

Loading rules: unknown keys warn but do not fail; missing roles fall back to the
default theme; `setTheme()` stores the source path in the registry so the choice
persists like the current palette keys.

## Importing existing themes

A converter tool (proposed `fox2/tools/fxthemeimport`, built on `FXSettings` +
a small tokenizer) reads an external theme and writes `.foxtheme`.

**Scope of the first importer: GTK only.** GTK 3/4 CSS is the most complete
color-and-style description available on Linux desktops and covers Adwaita,
Breeze, and most community themes, followed by GTK 2 `gtkrc` for older themes.
KDE `.colors`, Qt QSS, and Xresources importers are explicitly deferred until
the GTK path works end to end; they reuse the same parser and role mapping, so
they stay cheap to add later.

### How a theme reaches the application

Three delivery modes share one parser, so behavior stays identical:

1. **Direct import at startup** (default when no cache exists): parse the GTK
   theme into `FXTheme` and resolve roles in memory.
2. **Offline conversion**: the same code exposed as the `fxthemeimport` CLI, run
   by a packager, a developer, or the user ahead of time and writing a
   `.foxtheme`. This is the answer when startup parsing is undesirable or fails
   on a particular theme: convert once, ship or install the result, and the
   application only ever loads a simple INI file.
3. **Theme cache**: after any successful import the resolved theme is serialized
   to a cache file, so later startups skip the parser entirely.
   - Location: the per-user configuration directory already used by the
     registry, one cache file per source theme.
   - Key: source path, source size, source mtime, importer version, and schema
     version. Any mismatch means re-import and rewrite.
   - Load path: `stat()` the source, validate the cache header, deserialize with
     `FXStream`; on corrupt, stale, or unreadable cache, delete it and fall back
     to a fresh import.
   - The cache is untrusted input: bounded sizes, a validated magic/version
     header, and no unbounded allocations.
   - Failure policy: if cache and import both fail, fall back to `fox-classic`
     and warn once instead of refusing to start.

This machine already has useful corpora for the GTK path:

- primary: `/usr/share/themes/Breeze/gtk-3.0/gtk.css` and `gtk-4.0/gtk.css`
- primary: `/usr/share/themes/Breeze/gtk-2.0/gtkrc`, plus `Breeze-Dark`
- later: `/usr/share/color-schemes/*.colors` (KDE schemes: Emerald, EmeraldDark, …)
- later: `gsettings get org.gnome.desktop.interface color-scheme` (currently
  `prefer-light`) and, on Wayland/Flatpak systems, the settings portal

### Supported sources and what to read

| Priority | Source | Fields to import | Status |
|---|---|---|---|
| 1 (first) | GTK 3/4 CSS | `@define-color` names; `color`, `background-color`, `border-color`, `border-width`, `border-radius`, `padding`, `font`, `box-shadow` (as a shadow spec), `linear-gradient` (as a two-stop gradient) | **Done (T6)**; parse declarations only, ignore images, complex shadow stacks, animations, and layout |
| 2 (first) | GTK 2 `gtkrc` | `bg[NORMAL/SELECTED/INSENSITIVE]`, `fg[...]`, `base[...]`, `text[...]`, `font_name` | **Done (T6)**; named colors and the UI font are read, style blocks are skipped |
| 3 | KDE `.colors` | `[Colors:Window]` BackgroundNormal/ForegroundNormal, `[Colors:View]`, `[Colors:Button]`, `[Colors:Selection]`, `[Colors:Tooltip]`, `[General]` fonts | **Done**; section-keyed INI, `r,g,b` triplets, QFont strings |
| — | Qt QSS | same properties as CSS; `QPalette` roles if a palette dump is available | **Left open for later**: reuses the CSS scanner, but no Qt palette source is exercised on this machine yet |
| — | Xresources | `*background`, `*foreground`, `*highlightColor`, cursor theme | **Left open for later**: simple `key: value` scan, low value until requested |
| n/a | Windows | `GetSysColor()` roles, accent color, `SystemParametersInfo` fonts | Handled by the existing `GetSysColor` seed in `FXApp.cpp`, not the importer |
| n/a | GNOME/portal | `color-scheme` preference only (`prefer-light`/`prefer-dark`) | **Done (T8)**: selects the built-in light or dark preset, not a full theme |

### Role mapping for GTK-named colors

| GTK name | FOX role |
|---|---|
| `theme_bg_color` | `WindowBg` |
| `theme_fg_color` | `Text` |
| `theme_base_color` | `Surface` |
| `theme_text_color` | `Text` (widget text) |
| `theme_selected_bg_color` | `SelectionBg` |
| `theme_selected_fg_color` | `SelectionText` |
| `insensitive_bg_color` / `insensitive_fg_color` | `Surface`, `TextDisabled` |
| `borders` | `Border` |
| `theme_button_background_normal*` / `button_bg_color` | `Button` |
| `theme_button_decoration_hover*` / `hover_color` | `HoverRing` |
| `unfocused_bg_color` / `unfocused_fg_color` | `WindowBg`, `TextDisabled` |
| `theme_unfocused_selected_bg_color` | `SelectionBg` (inactive variant) |

Missing roles are derived from what the source does provide: `SurfaceHilite` and
`SurfaceShadow` from `Surface` via the existing `makeHiliteColor()` /
`makeShadowColor()`; `Tooltip*` from `Surface`/`Text` if unspecified; `Accent`
from `SelectionBg`.

### Import constraints

- Import **values, including style data**: radius, border widths, gradient
  stops, and shadow specifications are imported and rendered where FOX has a
  path for them. Only behavior with no rendering path (source-defined
  animations, image-based decorations, blur beyond the shadow budget) is
  reported as approximated or ignored, per declaration, in the import summary.
- Never overwrite the default theme file; write a new name derived from the
  source (`breeze-dark.foxtheme`).
- Report a summary (roles imported, roles derived, style data imported,
  unsupported declarations).
- Do not parse untrusted theme files without bounds; cap file size and token
  lengths.

## Built-in presets

Ship two presets generated from the current palette, so dark mode works without
any external file:

- `fox-classic` — today's exact defaults (regression baseline).
- `fox-dark` — dark counterparts derived by inverting lightness on the classic
  palette, keeping the same role structure.

## Phases

| Phase | Example | Work | Acceptance |
|---|---|---|---|
| T1 | `hello` | Add `FXTheme` with colors only; map roles to the existing palette; palette mirroring | `hello` renders identically to the Phase 3 build (pixel-compare) |
| T2 | `hello`, `button` | Migrate subset widgets (`FXFrame`, `FXLabel`, `FXButton`) to the `FXStyle` seam | Same pixel output; no `getApp()->get*Color()` left in migrated files; `onPaint` only collects state and delegates |
| T3 | `hello` | Add `.foxtheme` INI loading through `FXSettings`, plus `fox-dark` preset | Theme switch at startup changes colors; registry remembers the choice |
| T4 | `hello` | Runtime switching: repaint, cache invalidation, relayout on metric change | Switching on a live window updates all widgets |
| R1 | `hello` | Antialiased rounded corners in `FXStyle`: cached alpha masks + XRender composite, bitmap fallback, `hasXRender` capability check | Corners of a 6 px-radius button show no stepping at 100/150/200% |
| R2 | `button` | Shadow 9-slice cache and rounded focus ring | Popup/menu shadow and focus ring render without per-paint allocation |
| R3 | `button` | Hover/pressed interpolation via `addTimeout` and `animationTime` | Color transitions complete in the configured time and stop cleanly on leave |
| T5 | `hello` | Metrics and font roles; DPI-aware units | Padding/border from theme; text sizes follow font roles |
| T6 | `hello` | `fxthemeimport` for **GTK only**: GTK 3/4 CSS subset first, then GTK 2 `gtkrc`. One parser shared by the CLI tool and the runtime loader. Includes radius/border/gradient/shadow values | Importing Breeze and Adwaita GTK CSS produces valid theme files with a summary |
| T7 | `hello` | Theme cache: serialize resolved themes via `FXStream`, key on source path+size+mtime+importer version, re-import on mismatch or corruption | Second startup loads the theme without parsing CSS; stale or corrupt cache regenerates silently |
| T8 | `hello` | Portal/`gsettings` detection for light/dark selection | `prefer-dark` selects `fox-dark` without configuration |
| KDE | `hello` | `.colors` importer (`[Colors:*]` sections, `r,g,b` triplets, QFont strings) | Breeze Light/Dark and Emerald convert with no warnings and render correctly |
| — | — | **Left open: Qt QSS and Xresources importers** | Not started; see the status list below |

Order matters: T1–T4 deliver a working theme system with zero visual change;
R1–R3 then deliver the modern rendering; T5 depends on the DPI coordinate
policy; T6 is independent once T3 exists; T7 makes T6 cheap at startup; T8 is
independent of both. The KDE importer was picked up after T7 because the cache
and the GTK path were stable by then; Qt QSS and Xresources stay left open.

**Status summary (2026-11-17):** T1, T2, T3, T4, T5, T6, T7, T8, R1, R2
(focus ring only) and R3 are done and verified, as is the KDE `.colors`
importer. The P0 popup chrome started: popups are rounded off and show what is
behind their corners on X11, Win32 and Wayland. Still open: the **Qt QSS** and
**Xresources** importers, the same rounding for `FXToolTip`, and the **R2 popup
shadow**, which now has a consumer (`FXPopup` is in the `fox2` build). The order
in which the remainder is taken up is fixed below.

**Next steps after the core (agreed 2026-11-11).** The theme core is complete
enough to stop migrating widgets for its own sake. The remaining work is
ordered so that each item is runtime-verifiable on this machine and does not
collide with the DPI workstream:

1. **Wire the theme data that is parsed but unused.** `METRIC_SPACING`,
   `METRIC_ICON_SIZE`, `METRIC_SMALL_ICON_SIZE` and `METRIC_ROW_HEIGHT` are
   stored, imported and serialized, but no widget reads them; the same is true
   of the `FONT_SMALL`, `FONT_MONO` and `FONT_TITLE` roles. Give `FXLabel` and
   `FXButton` the small and mono roles, let packing consume `METRIC_SPACING`,
   and let row-shaped controls honour `METRIC_ROW_HEIGHT`. Acceptance: with
   Breeze imported, a control's height lands in the 24-28 px band the design
   checklist asks for, and a theme setting `[fonts] small=...` changes only the
   small text while `hello` stays pixel-identical under the classic theme.
2. **Finish the seam by migrating the last chrome consumers** —
   `FXTextField`, `FXScrollBar`, `FXToolTip`, `FXMenuPane`. `FXPopup` already
   draws through `FXStyle` and shapes its own corners; the tool tip needs the
   same call, and the popup shadow needs the popup inflated first. Acceptance:
   each widget draws through `FXStyle`, and classic output stays pixel-identical.
3. **R2 popup shadow** — the cached 9-slice tile, now with a real consumer
   (`FXPopup`/`FXMenuPane` inflated by the shadow margin, children offset).
   Acceptance: no per-paint allocation, a shadow under menus and tooltips, and
   themes that ask for no shadow are unchanged.

Deliberately after that, in no fixed order: gradients in `FXStyle` (the
`FXDC`/`FXImage` primitives already exist), per-state colour overrides in
`FXTheme`, `.foxtheme.json`, and the Qt QSS and Xresources importers. None of
them blocks the DPI or platform-seam workstreams.

**Why not migrate more widgets first:** `FXTextField` and `FXScrollBar` are
exactly the widgets the Windows DPI workstream will re-geometry
(`WM_DPICHANGED` relayout, DPI-keyed caches, hit testing). Migrating them once,
after the coordinate policy is fixed, avoids doing the same edit twice and keeps
the classic pixel-identity guarantee meaningful. That is why step 2 above stops
at the four remaining chrome consumers instead of sweeping the whole widget set.

**Progress:** T1 implemented and verified (2026-11-11). `include/FXTheme.h` and
`lib/FXTheme.cpp` added; `FXApp` owns a theme, mirrors it to and from the
palette, and keeps it in sync from the twelve palette setters. `hello` is
pixel-identical to the Phase 3 build (0 differing pixels) and the role/palette
consistency check passes.

**Progress:** T2 implemented and verified (2026-11-11). `tests/button` and its
closure (18 sources, 10 headers) are in `fox2`. `include/FXStyle.h` and
`lib/FXStyle.cpp` provide `drawButton`, `drawFrame`, `drawFocusRing`, and the
bevel helpers, all drawing from theme roles. `FXButton::onPaint` now collects
`FXButtonState` and delegates; `FXFrame::drawFrame` delegates to the style;
`FXLabel` and `FXFrame` take their default colors from theme roles. Both
`hello` and `button` remain pixel-identical (0 differing pixels) and no
`getApp()->get*Color()` calls remain in the migrated files.

**Progress:** T3 implemented and verified (2026-11-11). `FXTheme::setClassic()`,
`setDark()`, and `load(path)` (INI through `FXSettings`) added; `FXApp::init()`
reads the `SETTINGS`/`theme` registry entry and accepts `classic`, `dark`, or a
`.foxtheme` path. Verified: classic is still pixel-identical to the original
baseline, `dark` renders surface `#2D2D2D`, and a custom `.foxtheme` with
`surface=#336699` renders exactly `#336699`.

**Progress:** T4 implemented and verified (2026-11-11). `FXApp::setTheme()`
invalidates style caches, re-applies theme colors through a recursive
`FXWindow::applyTheme()` walk, repaints every window, and relayouts when metrics
change. Explicit widget color setters (`setBackColor`, `setBaseColor`,
`setHiliteColor`, `setShadowColor`, `setBorderColor`, `setTextColor`) mark the
value as overridden, so application customization survives a theme change.
Verified on a live window: classic `#D4D0C8` switches to dark `#2D2D2D` with
52% of pixels changing, while both examples stay pixel-identical at startup.

**Progress:** R1 implemented and verified (2026-11-11). `FXStyle` probes XRender
(depth >= 24 plus `XRenderQueryExtension`), builds antialiased corner masks once
per radius (4x4 supersampled coverage in an 8-bit A8 picture), and composites
the border and face colors through them; without XRender it falls back to the
existing hard-edged `fillRoundRectangle`/`drawRoundRectangle`. Because FOX
creates every window with `background_pixel=0`, the state carries the parent's
background color and the rounded shape fills under itself first, so corners show
the parent instead of black. Masks are freed by `invalidate()` and released in
`~FXApp` while the display is still open. Verified: classic rendering stays
pixel-identical (hello and button, 0 differing pixels); with `corner_radius=6`
the hello corner goes parent `#D4D0C8` -> `(120,117,113)` -> `(40,39,38)` ->
border `#000000`, and the 8x8 corner region contains 8 distinct colors.

**Progress:** T5 implemented and verified (2026-11-11). `FXTheme` now carries
font roles (`FONT_UI`, `FONT_SMALL`, `FONT_MONO`, `FONT_TITLE`) and a DPI
`scale`. Roles default to an empty `FXFontDesc`, which means "inherit the
application font"; a theme file sets them through `[fonts] ui=...`,
`small=...`, `mono=...`, `title=...` using the usual
`face,size,weight,slant,setwidth,encoding,flags` string. `FXApp` owns a cached
`FXFont` per role and creates it lazily through the existing `FXFont`
machinery; `getRoleFont()` returns the normal font for roles the theme left
open, so classic output is unaffected. `FXLabel` takes its font from
`FONT_UI` (marked with `fontOverride` when the application calls `setFont`),
and `setTheme()` drops the cached role fonts and relayouts when a role
description changed. `FXFrame` treats `DEFAULT_PAD` as "the theme's padding",
so `METRIC_PADDING` drives the interior of labels and buttons. `metric()`
multiplies length metrics by `scale` and leaves `METRIC_ANIMATION_TIME` alone,
so a scaled theme does not slow transitions down; the DPI workstream only has
to call `setScale()` (or write `[display] scale=<percent>`).

Verified on `hello`: classic is 83x28 and still pixel-identical to the
baseline; `padding=10` gives 99x42; `[fonts] ui=Sans,150` gives 134x38;
`scale=200` with `padding=4` gives 95x38. The transition demo run against a
`scale=200` theme still reports `animation_time` as 300 ms and still takes 21
repaints per leg, confirming that time is not scaled.

**Progress:** T6 implemented and verified (2026-11-11). `include/FXCSS.h` and
`lib/FXCSS.cpp` hold one reader for both jobs: it strips comments and quotes,
collects `@define-color` entries and resolves chains of `@name` indirections
(bounded to 8 hops), parses selector lists with pseudo classes into a
selector-keyed declaration table, and understands `#rgb`, `#rrggbb`,
`#rrggbbaa`, `rgb()`, `rgba()` (alpha ignored) and a small set of CSS color
names. Lengths accept `px`, `mm`, `cm`, `pt` and `em` and are converted at the
nominal 96 dpi, which keeps imports independent of the machine. GTK 2 `gtkrc`
files are read with the same storage: `gtk-color-scheme = "name:#rrggbb"` lines
become named colors and `font_name` becomes the `FONT_UI` role. At-rules with
no color content (`@keyframes`, `@import`, `@binding-set`, `@font-face`) are
skipped silently; anything else is reported as a warning.

Role mapping is GTK-name based: `theme_bg_color`/`bg_color` -> `SURFACE`,
`theme_base_color` -> `WINDOW_BG`, `theme_fg_color`/`theme_text_color` ->
`TEXT`, `theme_selected_*` -> `SELECTION_*` and `MENU_SEL_*`,
`insensitive_fg_color` -> `TEXT_DISABLED`, `borders`/`border_color` ->
`BORDER`, `tooltip_*` -> `TOOLTIP_*`, `accent_*` -> `ACCENT`. Raised and
sunken edges are derived from the imported surface with `makeHiliteColor()`
and `makeShadowColor()`. Metrics come from the rule that names `button`:
`border-radius`, `border-width`, `padding` and `outline-width`, with zero
values skipped because GTK resets properties in more specific rules.

`tools/fxthemeimport.cpp` (new `tools/` subdirectory) writes the resolved
theme as `.foxtheme` plus a human-readable summary, and exits with status 2
when warnings were seen. The runtime loader in `FXApp::init()` falls back to
the same reader when the registry `SETTINGS/theme` value points at a GTK theme
directory or style sheet instead of a `.foxtheme`.

Verified with the corpora on this machine: Breeze GTK 3 (837 rules, 0
warnings, padding 6, border 1, radius 5 from `1.25mm`), Breeze GTK 4 (781
rules, 0 warnings, padding 5, radius 5), Breeze-Dark GTK 3 and the Breeze
`gtkrc` (surface `#EFF0F1`, text `#232629`, selection `#3DAEE9`, border
`#BCBDBF`, tooltip `#F7F7F7`, 0 warnings). End to end, `hello` renders 91x34
with face `#EFF0F1` from the generated file, and pointing the registry straight
at `/usr/share/themes/Breeze` gives a pixel-identical result (0 differing
pixels), while `/usr/share/themes/Breeze-Dark` gives face `#202326`. Classic
stays pixel-identical.

**Progress:** T7 implemented and verified (2026-11-11). `include/FXThemeCache.h`
and `lib/FXThemeCache.cpp` serialize a resolved theme with `FXMemoryStream`
under a `FXTCMAGIC` header carrying the schema version, the importer version,
the source path, its size and its modification time. Cache files live in
`$XDG_CACHE_HOME/fox/themes` (falling back to `~/.cache`), one per source,
named from the source basename plus a hash of the full path. A theme
*directory* is resolved to the style sheet that will actually be read
(`gtk-3.0/gtk.css`, then `gtk-4.0`, then `gtk-2.0/gtkrc`) before stamping, so
editing that file invalidates the entry. `FXTheme` gained `save`/`load`
(FXStream, with the font strings length-checked to 1 KB) and `describe()`,
which produces the `.foxtheme` text from a resolved theme rather than from the
parser, so a cached theme serializes identically to an imported one.

Failure policy is "cache never blocks startup": the file is bounded to 64 KB
before reading, every mismatch (magic, schema, importer, path, size, mtime,
truncated or garbled body) deletes the cache and returns a miss, and the caller
re-imports; the buffer is allocated with `FXMALLOC` and released with
`FXFREE(&ptr)` (the macro takes the address of the pointer, not the pointer).
`fxthemeimport` grew `-c` to fill and use the cache, and
`FXThemeCache::getHits()`/`getMisses()` make the behaviour observable.

Verified by a dedicated check program (22 assertions, all passing): cold miss
and no cache file, store + hit, fetched theme equal to the imported one (colors,
metrics, fonts, scale), a hit that survives the source being replaced by
unparseable bytes of the same size and mtime — proving a hit does not parse —
then a newer mtime invalidating, a garbled cache and a truncated cache both
missing and being deleted with the theme falling back to classic, and a bumped
importer version forcing a re-import. End to end, `fxthemeimport -c` reports a
miss on the first Breeze run and a hit on the second with byte-identical output
(and no `rules`/`warnings` lines, since nothing was parsed), and `hello` pointed
at `/usr/share/themes/Breeze` writes the cache once, does not rewrite it on the
second run, and renders `#EFF0F1` at 91x34 both times with 0 differing pixels.
Classic stays pixel-identical.

**Progress:** T8 implemented and verified (2026-11-11). `include/FXDesktopSettings.h`
and `lib/FXDesktopSettings.cpp` read the desktop's light/dark preference without
linking a desktop library. The checks run cheapest first and every failure
falls through: `$GTK_THEME` (a name containing `dark`), then
`gtk-application-prefer-dark-theme` in `$XDG_CONFIG_HOME/gtk-4.0/settings.ini`
and `gtk-3.0/settings.ini`, then `[General] ColorScheme` in `kdeglobals`, and
finally `gsettings get org.gnome.desktop.interface color-scheme` through a
bounded `popen()` read on POSIX only. The result is `SCHEME_DARK`,
`SCHEME_LIGHT` or `SCHEME_UNKNOWN`, and `getColorSchemeName()` reports which
source decided it. `FXApp::init()` now treats an absent `SETTINGS/theme` entry,
and the explicit value `auto`, as "follow the desktop": a dark preference
selects the built-in dark preset, and an explicit `classic`, `dark` or theme
path still wins.

Verified with a check program (9 assertions, all passing) covering no
configuration, `GTK_THEME=Adwaita-dark` (dark), `GTK_THEME=Adwaita` (no hint,
unknown), the GTK settings file in both directions, GTK 4 taking precedence
over GTK 3, `kdeglobals` with `BreezeDark`/`BreezeLight`/`Oxygen`, and this
desktop (light, from the GTK 4 settings file). With an isolated `HOME`, `hello`
renders the dark surface `#2D2D2D` when the desktop prefers dark and the classic
`#D4D0C8` when it prefers light, an explicit `classic` overrides a dark
preference, and `auto` follows it. Classic stays pixel-identical.

**FOX quirk found while implementing T8:** `FXString::toInt()` reports failure
for the string `"0"`, because `__strtoll()` consumes the leading zero looking
for an `0x`/`0b` prefix and then finds no digits. `FXSettings::readIntEntry()`
therefore returns its default for a value of `0`. The desktop adapter reads the
GTK preference as text and compares strings instead of relying on `toInt`; other
code should avoid `toInt()` on values that may be exactly `0` until the number
parser is addressed.

**Progress:** R2 focus ring implemented and verified (2026-11-11).
`FXStyle::drawFocusRing()` draws a rounded ring in `ROLE_FOCUS_RING` using
`METRIC_FOCUS_WIDTH` when `corner_radius>0`, and keeps the classic dashed
`drawFocusRectangle` at radius 0. Verified on a live window: focusing a rounded
button changes 4.5% of its pixels and leaves ~1660 accent `#0A246A` pixels.

**Progress: rounded popups (2026-11-17).** The first P0 item is in: a popup is
rounded off and shows what is behind its corners instead of its own background.
`FXWindow` gained `setCornerRadius(radius)`, next to `setShape()`, and each
platform implements it the way its window system allows:

- X11 shapes the window to a rounded rectangle (`XShapeCombineRegion` with one
  rectangle per group of rows that has the same inset), so the corner pixels
  belong to no window and the children that draw inside it are clipped too.
- Win32 calls `SetWindowRgn` with `CreateRoundRectRgn`; the region clips the
  children exactly like the X11 shape.
- Wayland has no window shape, and every window of a shell paints into one
  shared buffer, so the corners are cut out of the finished frame instead: the
  surface keeps the radius and `fxwaylandFlushFrames()` scales the alpha of the
  four corner squares by a 4x4 supersampled quarter-disc coverage, un-premultiplying
  as it goes. Because that pass runs on the frame the compositor receives, a
  menu item that fills its whole row cannot paint over the corner.

`FXPopup::onPaint()` draws the themed path through
`FXStyle::drawRoundedRectangle()` (face `ROLE_SURFACE`, hairline
`ROLE_BORDER`, radius and width from `METRIC_CORNER_RADIUS` and
`METRIC_BORDER_WIDTH`), fills the face first so the style's antialiased edge
blends with the popup and not with an empty frame, and shapes the window when
the radius or the size changed. A theme that returns to radius 0 clears the
shape again, so classic popups are untouched.

Verified on all three backends with `tests/popup` and the Breeze import
(radius 5): X11 reports `bounding_shaped=1` for the 98x72 popup and
`bounding_shaped=0` for the classic 90x56 popup; under Wine the Win32 build
reports the same pair with 83x62 and 75x46; on Wayland the dumped frame of the
98x72 popup gives corner alphas `0, 0x50, 0x9f, 0xbf, 0xff` while the centre is
opaque. The classic frames stay pixel-identical (`hello` 0 differing pixels,
popup frames unchanged) and the Wayland popup, menu, tool tip, hello and
repaint gates all pass.

**Progress: Win32 theme import with an installed GTK theme (2026-11-17).**
`scripts/install-wine-gtk-themes.sh` copies the named themes from
`/usr/share/themes` into `<prefix>/drive_c/ProgramData/themes`, which is where a
GTK installation on Windows keeps them, and prints the value to put in
`SETTINGS/theme`.  With Breeze and Breeze-Dark installed there, the Win32 build
imports the same CSS the Linux build does and the rounded popup appears with
83x62 pixels and the Breeze face; `Breeze-Dark` imports as well and caches
separately.

Two defects were found while verifying, both fixed:

- **The theme cache never worked on Windows.** `FXThemeCache::directory()`
  appended `"/.cache/fox/themes"` to the home directory, which on Windows is
  `C:\users\<name>`; the mixed separators made `FXDir::createDirectories()`
  fail and the cache stayed unwritten, so every start re-imported the CSS.
  The path is now built with `PATHSEPSTRING`, and `location()` takes the source
  name after the last `/` or `\`, so a Windows path names its cache file
  correctly.  The cache file now lands in
  `<prefix>/drive_c/users/<name>/.cache/fox/themes/` and the second start reuses
  it.
- **The popup was shaped only when it painted.** `FXPopup::shapePopup()` now
  runs from `show()` as well, right after the shell is shown; `onPaint()` still
  calls it, which covers a theme or size change.  It has to run *after*
  `FXShell::show()`, not before it: on Wayland the surface of a popup comes into
  being during the show, and shaping a window without a surface is a no-op,
  which leaves a square-cornered popup with the cache believing it is shaped.
  Wine applies the region to its X window about 0.7 s after the popup appears,
  which is a Wine detail, not a FOX one.

**A theme is only visible in the Wine prefix the program runs in.** A plain
`wine .../ControlPanel.exe` uses `~/.wine`, while the build scripts pass
`WINEPREFIX=$ROOT/tmp/wineprefix`; installing into one and looking at the other
is why the chooser looked empty after a fresh prefix.  `ControlPanel` now takes
`--list-themes`, which prints the discovered entries as `name<TAB>source` and
exits, so the question is answered by one command:

```
$ WINEPREFIX=tmp/wineprefix wine tmp/build-fox2-cross-cmake/bin/ControlPanel.exe --list-themes
Breeze      C:\ProgramData\themes\Breeze
Breeze-Dark C:\ProgramData\themes\Breeze-Dark
```

`scripts/install-wine-gtk-themes.sh` now prints the prefix it writes to, and
picks `$WINEPREFIX` first, then `~/.wine` (what a plain `wine` uses), then
`tmp/wineprefix`, so the themes land in the prefix the next run will use.

**The theme chooser did not see them, third defect fixed.** `ControlPanel`
only searched `XDG_DATA_HOME`, `XDG_DATA_DIRS`, `~/.local/share` and
`~/.themes`, so a theme installed in the prefix was invisible on Windows: there
are no XDG variables there, and the POSIX default list points at directories
that do not exist.  `discoverThemes()` now adds `%ProgramData%`,
`%APPDATA%` and `%LOCALAPPDATA%` on Windows, where the program data folder takes
the place of `/usr/share`, and skips the POSIX default list there.  Verified by
running the Win32 `ControlPanel` under Wine: it searches
`C:\users\<name>\.local\share\themes`, `C:\ProgramData\themes`,
`C:\users\<name>\AppData\Roaming\themes`,
`C:\users\<name>\AppData\Local\themes` and `C:\users\<name>\.themes`,
finds `Breeze` and `Breeze-Dark` in `C:\ProgramData\themes`, and selects them
like any other installed theme.

A settings value written by hand needs care on Windows: `FXSettings` treats a
backslash as an escape, so `theme=C:\ProgramData\themes\Breeze` loses its
separators.  Forward slashes work as they are, and the value the ControlPanel
writes is quoted and escaped, so the round trip through the chooser is intact.

**Progress: Win32 corners were half as round (2026-11-17).** The imported
Breeze theme looked less rounded under Wine than on Linux, and the theme was
not to blame: `FXDCWindow::drawRoundRectangle()` and `fillRoundRectangle()` hand
`ew`/`eh` to GDI `RoundRect`, but FOX's `ew`/`eh` are radii while `RoundRect`
wants the size of the corner ellipse, so every Win32 corner came out at half the
requested radius (5 px drawn as 2.5 px).  Both calls now pass `ew<<1` and
`eh<<1`, which makes the GDI path agree with the X11 and Wayland ones; a radius
of 0 is unaffected, so classic output is unchanged.

Measured on the Breeze popup at scale 1.0, the border curve of the top-left
corner before the fix reached x=3 only at row 2, and after the fix at row 1,
which is where the X11 shape puts it.  The remaining difference is
antialiasing: X11 composites the corner through an XRender mask and Wayland
through 4x4 coverage, while Win32 still draws a hard GDI edge.

**Progress: rounded tool tips (2026-11-17).** `FXToolTip` draws its rounded
frame through `FXStyle` when the theme has a radius, but the window stayed
square, so the corner pixels were the tip background.  It now carries the same
`shapeRadius`/`shapeWidth`/`shapeHeight` cache and calls a `shapeTip()` helper
from `show()` and from `onPaint()`, which shapes it through
`FXWindow::setCornerRadius()`.  Verified at scale 1.0 with Breeze: the Wayland
tip frame is 50x26 with corner alphas `0, 0, 0x50, 0xbf, 0xff` down the
diagonal, the X11 tip reports `bounding_shaped=1`, and the Win32 tip under Wine
reports `bounding_shaped=1` at 42x21 (the Wine font is smaller).  The classic
gates are unaffected.

**Progress: drop shadows under popups and tool tips (2026-11-17).** The theme
gained `METRIC_SHADOW_SIZE` (`[metrics] shadow_size`), filled from the GTK
`box-shadow` of the `popover.background` rule: the blur radius is imported and
bounded to 12 px, which turns Breeze's `0px 0px 2mm` into 8 px.  KDE schemes and
the classic preset leave it 0, and the cache schema moved to 5 so older cache
files are rebuilt.

`FXStyle` asks the platform whether it can carry a translucent shadow
(`shadowMargin()`), draws one (`drawShadow()`), and only Wayland answers yes: its
window surfaces are premultiplied ARGB, so a translucent ring is composited by
the compositor with what is behind the popup.  X11 and Win32 windows have no
alpha channel, so they report a margin of 0 and their popups keep today's size
instead of reserving a margin nobody can see.  On Wayland the popup is inflated
by the margin, its children are laid out inside it, the surface is drawn at the
offset and shaped to the rounded outline through the new
`FXWindow::setCornerRadius(radius,x,y,w,h)`, so the mask rounds the surface and
not the window, and the margin keeps the shadow.

The shadow itself is a closed-form ring: the distance from the pixel centre to
the rounded rectangle gives a squared falloff to 3/8 black over the margin, and
the ring is written with `FXRaster::plotAlpha()` so the alpha survives into the
buffer.  That is a few thousand pixels per paint and no allocation, which meets
the no-per-paint-allocation rule; a cached 9-slice tile is only worth it if the
profile ever gets expensive.  The ring is written even where it fades to nothing,
because the tool tip paints its background over the whole expose before the
shadow goes down.

Verified at scale 1.0 with Breeze: the Wayland popup grows from 98x72 to 114x88
and its screen-edge profile reads alpha 0,3,9,18,30,45,63,84 with the surface at
114-16; the tool tip grows from 50x26 to 66x42 with the same profile and a
rounded surface at (8,8).  X11 keeps 98x72 (`bounding_shaped=1`) and Win32 83x62
(`bounding_shaped=1`), and classic stays at 90x56 unshaped on X11 with the
Wayland popup, menu, tool tip and hello gates unchanged.

**Progress: the last 3D bevels out of a themed dialog (2026-11-17).** A
themed `button` still carried classic bevels after the widget migration, and the
histogram of a Breeze screenshot showed them: 2856 pixels of
`ROLE_SURFACE_SHADOW` plus as many of `ROLE_SURFACE_HILITE`, in a one pixel
column and a box outline.  They came from three widgets that draw their own
3D chrome instead of going through `FXStyle`:

- `FXSeparator` drew a groove or ridge as two pixels of shadow and highlight.  A
  themed separator is now a single hairline in `ROLE_BORDER`, and its default
  size drops from two pixels to one so a dialog's layout tightens to match.
- `FXGroupBox` drew a classic raised, sunken, groove or ridge frame.  A themed
  group box now draws a rounded hairline in `ROLE_BORDER` with the label cutting
  it open, the way GTK draws a frame.
- `FXDragCorner` drew shaded ridges into the corner; a themed corner draws three
  flat diagonal lines in `ROLE_TEXT_DISABLED`.

Measured on the same 800x748 `button` window at scale 1.0 with Breeze: the
shadow and highlight colors are gone from the screenshot, the border color grows
from 3036 to 4769 pixels, and the classic build still shows its 4210/4194
pixels of highlight and shadow, so the classic path is untouched.  The Wayland
hello, menu, popup and tool tip gates still pass.

**Progress: several rounded rectangles per Wayland shell (2026-11-17).** The
Wayland surface kept one radius and one rectangle, which was enough for the
popup surface plus its shadow margin but nothing that nests.  It now keeps up to
eight, keyed by the window that asked for the rounding, with the rectangle
translated into store coordinates the way a window binds to its store (including
the client side title bar offset).  That is what a scroll area viewport inside a
rounded frame needs, and it costs nothing for the popup: verified that the
Breeze popup still frames as 114x88 with corner alphas 0,0,80,191,255 at the
surface and a transparent shadow ring, and that the tool tip and menu runs are
unchanged.

**Progress: rounded viewport corners in lists (2026-11-17).**
`FXStyle::cutRoundedCorners(dc,x,y,w,h,radius,color)` fills the part of a
rectangle that lies outside its rounded corners, which is the primitive a widget
needs when it paints its own content into a rounded frame: the widget draws its
items as usual and then cuts the corners back out.  Wayland fills it
antialiased, blending the colour by the inverse of the corner coverage;
`FXStyle::fillOutsideCorners()` is the hard edged version X11 and Win32 use,
built from one span per row.  `FXList` and `FXTreeList` call it with the parent's
background and then draw the viewport hairline in `ROLE_BORDER`, because they
paint into the scroll area window themselves and would otherwise cover the
chrome the scroll area drew.

Verified at scale 1.0 with Breeze on the header test: on Wayland the corner of a
list reads `#EFF0F1, #EFF0F1, #B7DBEE` along the first row — parent background,
parent background, a 50/50 blend of the accent of the selected item and the
parent background — with the hairline following the arc; on X11 the same corner
is hard edged, `#EFF0F1, #EFF0F1, #EFF0F1` on the first row and the border at the
arc.  The classic Wayland gates all still pass and the classic build draws no
cut at all.

Two things this turned up, both recorded in the checklist: `FXIconList` starts
its content below its header and still needs the offset, and `FXHeader` draws its
buttons with raised and sunken rectangles built from the theme's highlight and
shadow colours, which is the last 3D chrome left in a themed dialog.

**Progress: a flat header (2026-11-17).** `FXHeader` was the last widget that
drew its header buttons with raised and sunken rectangles built from the theme's
highlight and shadow colours, which is why a themed dialog still had a 3D strip
under the toolbar.  A themed header now skips those rectangles and separates its
items with a one pixel hairline in `ROLE_BORDER`, drawn after the items because
each item paints its whole rectangle and would cover a line drawn before it; the
classic header keeps its bevels.

Measured on the header test at scale 1.0 with Breeze: the header rows hold 68
pixels of highlight or shadow colour instead of 3481 (the drawn separators take
170 pixels of border colour, four hairlines times the header height plus its
top edge).

The same screenshot still showed three thin lines of `ROLE_SURFACE_SHADOW`
outside the header: the last row of the client side title bar, one full width
line above the header and a one pixel column at the left edge.  Chasing them
found the last two producers of old chrome in the toolkit:

- `FXPacker::drawFrame()` had a switch of its own and called the packer's own
  `drawSunkenRectangle()` and friends, so an `FXVerticalFrame` with
  `FRAME_SUNKEN|FRAME_THICK` around the client area, and the `FXHorizontalFrame`
  around the lists, drew a two pixel 3D border that no theme could reach.  It
  now hands the options and its colors to `FXStyle::drawFrame()` exactly as
  `FXFrame::drawFrame()` does, which is the same thing for a classic theme: the
  style's classic fills are line for line the code the packer used to run.
- the Wayland client side title bar drew its separator with the app shadow
  color, which a theme maps to `ROLE_SURFACE_SHADOW`; it now uses
  `ROLE_BORDER` when the theme has a corner radius, and the classic app keeps
  the shadow line.

With those in, the header test at scale 1.0 with Breeze holds no
`ROLE_SURFACE_SHADOW` pixel at all (`#A0A1A1` went from 3428 to 0), the
highlight colour is down to 35 pixels inside the title bar's own button glyphs
(`#F4F5F6` from 2607), and the border colour carries the new hairlines
(`#C6C8C9` from 9768 to 13148).  A classic run of the same test still shows the
`#8E8B86` and `#E2E0DA` bevels and the black inner border of the double sunken
frames.  The Wayland hello, menu, popup, tool tip and repaint gates pass.

**Progress: icon lists (2026-11-17).** `FXIconList` now cuts its viewport corners
the same way `FXList` and `FXTreeList` do.  In icon mode that is all four
corners.  In detail mode the column header covers the top of the viewport and
paints square corners of its own, so only the bottom two belong to the list: the
cut is built from a viewport rectangle that starts above the widget, which keeps
the top arcs outside the clip instead of needing a second primitive.

Checked on the icon list test at scale 1.0 with Breeze, where the list is in
detail mode and its item background is white: the bottom left corner reads
`#FBFBFB` then `#F4F5F5` as the antialiased cut, with the border colour
`#C6C8C9` on the arc, and the top of the viewport is square under the header as
intended.

**Progress: menu rows from the theme (2026-11-17).** Menu items were the last
row-shaped controls with a hard coded height: every `FXMenuCaption` and its
subclasses added five pixels around the text (four for a menu bar title, and a
floor of twenty for a check or radio row), so a themed menu kept the height of
the classic palette whatever the theme asked for.

`FXMenuCaption::menuRowHeight()` now owns that arithmetic for the whole family.
A themed row is the text or the icon, plus `METRIC_ROW_PADDING` and
`METRIC_BORDER_WIDTH` on each side, and never less than `METRIC_ROW_HEIGHT`;
the classic case keeps the old numbers exactly, which is why the class still
takes the classic padding as an argument.  The CSS importer fills the two new
metrics from the `menu menuitem` rule, so Breeze's `min-height: 4.5mm` and
`padding: 0.75mm 2mm` land as seventeen and three, while `METRIC_ROW_PADDING`
defaults to two for the importers that do not set it; the cache schema moved to
six because the metric table grew.

Measured on the menu test under sway at scale 1.0 with Breeze: the File menu
surface went from 99x144 to 99x120 and the row pitch from thirty pixels to
twenty four, which is what GTK draws for the same theme: eighteen for the text,
three of padding and one of border twice.  A classic run of the menu gate still
picks the same entries, and the Wayland hello, popup, tool tip and repaint gates
pass.

**Progress: scroll bar thickness (2026-11-17).** The thickness of a scroll bar
was the last piece of a bar that a theme could not reach: `FXScrollBar` took it
from the application setting, so a themed scroll area kept the toolkit's fifteen
pixels whatever the theme drew inside them.  `FXScrollBar::barSize()` now asks
`METRIC_SCROLLBAR_WIDTH` first and falls back on the application setting when the
theme has nothing to say, and an application that calls `setBarSize()` still wins
because the automatic size only applies while `barauto` is set.  The CSS
importer fills the metric from the width the theme gives its scroll bar buttons,
which is what the theme sizes its bar by, and from the trough when it has no
steppers; the cache schema moved to seven.

Measured on the icon list test at scale 1.0 with Breeze, whose scroll bar buttons
are fourteen pixels wide: the vertical bar went from fifteen pixels to fourteen
and the list gained a pixel of content, while a classic run of the same test
still shows the fifteen pixel bevel sided bar.

**Progress: arrow buttons and switchers (2026-11-17).** `FXArrowButton` painted
its own raised and sunken rectangles and coloured the triangle with the widget's
arrow colour, so the arrow of a combo box, a spinner or a scroll bar stepper was
the one piece of a themed dialog that still looked classic.  A themed arrow
button now hands the same `FXButtonState` to `FXStyle::drawButton()` that a
button hands it, so it gets the theme face, the hover ring, the pressed tint and
the rounded shape, and the triangle is drawn in `ROLE_TEXT`, or
`ROLE_TEXT_DISABLED` when the button is off.  A classic arrow button keeps its
bevels and its own arrow colour.

`FXSwitcher` needed nothing of its own: it fills its background with the theme
surface and calls `drawFrame()`, and that call reaches the style now that
packers go through it.  The checklist entry for it was left over from before
that change and is now marked done.

Checked in the control panel at scale 1.0 with Breeze: the combo box arrow is a
dark triangle on the white field inside the field's hairline, with the raised
arrow box gone, and the whole window holds no `#8E8B86` or `#E2E0DA` bevel.  A
classic run of the same window still paints 9790 pixels of `#8E8B86` and 11700
of `#E2E0DA`.

**Progress: the text editor (2026-11-17).** `FXText` only retargeted its font
when the theme changed, so a themed editor kept the palette colours it collected
in its constructor: black text, the navy selection and the application's
background.  Its `applyTheme()` now sets the document view to `ROLE_WINDOW_BG`,
unless the application set a background of its own, and only when the theme
rounds its corners, the same rule the lists follow.  Text and the caret are
`ROLE_TEXT`, the selection is `ROLE_SELECTION_BG` over `ROLE_SELECTION_TEXT`, the
line number gutter is `ROLE_SURFACE` with `ROLE_TEXT_DISABLED` numbers, and the
active and highlighted spans use `ROLE_WINDOW_BG`, `ROLE_ACCENT` and
`ROLE_SELECTION_TEXT`.

Checked with a small editor window on Wayland at scale 1.0, since none of the
test programs put text in a view and then stay up: with Breeze the window holds
98630 pixels of `#FFFFFF` for the page, 8568 of `#3DAEE9` for the selection and a
hairline border, where a classic run of the same window holds the same page,
8568 pixels of `#0A246A` for the selection and the `#8E8B86` and `#E2E0DA`
bevels.

**Progress: tables (2026-11-17).** `FXTable` took its cell colors from the
palette in its constructor, which the theme does set at startup, but kept them
across a theme switch; and its headers were created with a fixed size of twenty
pixels while the theme pads a header item by six on each side, so a themed table
drew its column and row labels cut in half.

The cell, grid and selection colors now come from a `themeColors()` helper that
runs both at construction and from `applyTheme()`: `ROLE_TEXT` for the text,
`ROLE_WINDOW_BG` for the cells, `ROLE_SELECTION_BG` over `ROLE_SELECTION_TEXT`
for the selection, `ROLE_BORDER` for the grid and the cell border and
`ROLE_ACCENT` for the stipple.

The header clipping took two fixes, because the two headers have different
freedoms.  The column header can grow — it is its own dimension — so `layout()`
now grows a fixed header height to what its items need, which is eighteen pixels
of text plus six of padding and two of border on each side.  The row header
cannot: its rows are the rows of the grid, so a themed table drops its vertical
padding and keeps the two pixels of border, which leaves the same room the
classic header has.

Measured on the table test at scale 1.0 with Breeze the header band went from
twenty pixels with half-height labels to thirty four with the labels complete,
and the themed frame carries 9835 pixels of the border color for the header and
cell hairlines.  A classic run of the same test is unchanged: the same 913x515
window, the same 8E8B86 and E2E0DA bevels and the same black header borders.

**Progress: sliders (2026-11-17).** `FXSlider` drew a sunken slot with a
stippled fill and a bevelled head, which is the one control every themed dialog
shows at least once and which still looked like the classic palette inside an
otherwise flat window.  A themed slider now draws a thin rounded trough in
`ROLE_SURFACE_SHADOW`, fills the travel before the head with `ROLE_ACCENT`, puts
one pixel hairlines where the ticks go, and sets a round head on top in
`ROLE_BUTTON` whose edge is `ROLE_BORDER`, `ROLE_HOVER_RING` while the cursor is
over it or `ROLE_FOCUS_RING` when the slider has focus.  An inside-bar slider,
whose head runs in the slot, uses its slot as the trough, and the arrow variants
draw a small triangle in `ROLE_TEXT` inside the head.  The classic painter is
untouched and still draws the stipple and the bevels.

Checked with a four slider window at scale 1.0 on Wayland, because no test
program leaves sliders up: with Breeze the horizontal slider reads as a grey
trough with an accent band up to a white round head and hairline ticks, and the
vertical ones fill from the bottom; a classic run of the same window still shows
the `#8E8B86` and `#E2E0DA` bevels with the black ticks and the stippled slot.

`FXRangeSlider` has a painter of its own and got the same treatment in the same
pass: the trough is the same thin rounded bar, the range between the two heads
is filled with the accent and the heads are two round buttons sharing the
edge colours.  Checked in the same window with a horizontal and a vertical range
slider at scale 1.0: the accent band sits between the heads and the trough
extends past them on both sides, and a classic run still paints the same
`#D4D0C8` slot with the `#8E8B86` and `#E2E0DA` bevels.

**Progress: console, spinner and colour well (2026-11-17).** Three small ones in
one pass, found while looking for the next open entry in the checklist.

`FXConsole` kept the palette colours it was constructed with, its `onPaint()`
cleared every expose with a red fill, and `drawTextFragment()` drew every glyph
in white whatever the theme said.  The red fill and the white text are both in
the 1.7.95 release, not in the port: the widget is unfinished there, with a
hundred `blabla` placeholder lines in its constructor, an `onXXX` stub and the
`fillBufferRect` calls commented out.  The expose is now cleared with the
background colour and the text comes from `textColor`, or `seltextColor` over
`selbackColor` for a selected fragment, so the page and the log have contrast in
both palettes; the placeholder lines are left alone because they are what the
release ships.  The page takes `ROLE_WINDOW_BG` unless the application set
a background of its own and the theme rounds its corners, the text is
`ROLE_TEXT`, the selection `ROLE_SELECTION_BG` over `ROLE_SELECTION_TEXT` and the
caret `ROLE_TEXT`.  On the console test at scale 1.0 with Breeze the page is
588240 pixels of `#FFFFFF` inside a hairline frame with a themed scroll bar.

`FXSpinner` turned out to need nothing: it is a text field with two arrow
buttons, and both were already themed, so a spinner in a test window draws a
rounded field with two rounded steppers and their dark triangles in `ROLE_TEXT`.

`FXColorWell` now asks for the shared focus ring instead of drawing a dashed
rectangle of its own; the swatch stays whatever value the application put in it,
so only the frame and the ring are the theme's business.

**Scroll area viewports inside a content window are not rounded yet.** Shaping the
content window of a scroll area works when the area has one: `FXScrollWindow`
and the text/table widgets put their content in a child window, so the existing
`setCornerRadius(radius,x,y,w,h)` would clip it.  The attempt failed on two
points, both worth knowing before the next try:

- The child list is not reliable: `FXIconList` creates its header before the
  scroll area creates its scroll bars, so "the child that is not a scroll bar"
  is the header.  A content child has to be identified by covering the viewport,
  and the subclass sizes it *after* `placeScrollBars()`, so the shaping has to be
  deferred to a chore; the chore then ran before the size settled and shaped
  nothing.
- `FXList`, `FXIconList` and `FXTreeList` have no content window at all: their
  items paint into the scroll area window itself, over the chrome the area
  painted.  They need the opposite primitive, a fill of the area *outside* a
  rounded rectangle with the parent's background, so the item background is cut
  at the corners; that is a small addition next to `drawRoundRect` in each
  platform file.

**R2 shadow still deferred.** A drop shadow must be drawn *outside* the widget
rect, so it needs popup windows inflated by the shadow margin with their
children offset. The rounded popup above is the consumer that work was waiting
for, and with the shape in place the 9-slice shadow tile is the next step; the
`FXPopup.cpp` closure remark that used to stand here was wrong (the file is in
`lib/Makefile.am` and in the CMake build).

**Progress:** R3 implemented and verified (2026-11-11). `FXTheme` gained
`METRIC_ANIMATION_TIME` (classic default 0, parsed from `[metrics]`
`animation_time`). `FXButton` keeps `animColor`/`animFrom`/`animTarget`/
`animStart`/`animating`; `updateAnimatedColor(hover)` aims the face color at
`theme->color(ROLE_SURFACE,ACTIVE|HOVER)` while hovered and back at `backColor`
otherwise, and starts a 15 ms `ID_ANIMATE` timer. `onAnimate()` computes
`alpha=(now-animStart)/(animation_time*1e6)`, blends `animFrom`->`animTarget`
linearly, and snaps plus stops when `alpha>=1`.

Two defects were found and fixed while verifying:

- **The timer never repeated.** FOX timers are one-shot; `onAnimate()` returned
  1 without re-arming, so exactly one step ran and then nothing. It now calls
  `addTimeout()` again for the next step and only returns 0 when the transition
  completes.
- **`underCursor()` is stale in `onLeave`.** `FXApp::enterWindow()` sets
  `cursorWindow` *before* dispatching `SEL_ENTER`, but `FXApp::leaveWindow()`
  dispatches `SEL_LEAVE` *before* clearing it, so `underCursor()` is still true
  inside `onLeave()`. The hover state is therefore passed in explicitly as
  `updateAnimatedColor(FXbool hover)` instead of being read from the app.

Verified with a paint-counting `FXButton` subclass driven by synthetic crossing
events (Xwayland cannot warp the pointer): with `animation_time=300` the face
color ramps `#D4D0C8`->`#D7D3CC` over ~21 repaints and stops at ~295 ms, the
fade back ramps to `#D4D0C8` and stops the same way, and the 500 ms quiet window
afterwards sees 0 repaints; with `animation_time=0` and with the classic preset
there is no timer and a single repaint. Classic rendering stays pixel-identical
(hello and button, 0 differing pixels).

### Step details

Each step ends with: build, run the named example, and check the acceptance
column. Visual steps (`T1`, `T2`, `R1`, `R2`, `R3`) require a screenshot or
pixel comparison against the pre-change build.

Two constraints apply to the verification scripts in `tmp/`:

- The development session runs on KDE Wayland with Xwayland, so `XWarpPointer`
  cannot move the real pointer and `import -window <title>` is unreliable. Steps
  that need hover must synthesize the crossing event (`XSendEvent` with
  `EnterNotify`, `mode=NotifyNormal`, `detail=NotifyAncestor`) and steps that
  need a click must synthesize `ButtonPress`/`ButtonRelease`.
- `FXWindow::update()` queues a client-side repaint, so X `Expose` events are
  not a usable repaint signal. Count paints in-process by subclassing the widget
  and hooking `SEL_PAINT`; a subclass may also read the protected animation
  fields to check the blend.

#### T1 — theme skeleton, colors only

1. Add `include/FXTheme.h`: role enum and state enum as plain enums inside
   `namespace FX` (no `enum class`); twelve color fields initialized from the
   classic defaults; `FXColor color(FXint role,FXint state=Normal) const;` and a
   `FXint metric(FXint m) const;` stub returning the current hard-coded values.
2. Add `lib/FXTheme.cpp`: default constructor fills the classic palette; derive
   `Hilite`/`Shadow` from `Surface` with the existing `makeHiliteColor()` /
   `makeShadowColor()`; `Disabled` blends `Text` toward `Surface` by ~45%.
3. In `FXApp`: add a `FXTheme theme;` member, `FXTheme* getTheme()`, and
   initialize it after the palette is set up; `setTheme()` copies the theme
   colors into the existing palette fields so legacy `get*Color()` callers stay
   correct.
4. Add both files to `lib/Makefile.am`, rebuild, run `hello`, and pixel-compare
   with the Phase 3 build.

#### T2 — the FXStyle seam

1. Add `include/FXStyle.h` and `lib/FXStyle.cpp`: a `FXStyle` that references
   its `FXTheme` and provides `drawButton`, `drawFrame`, and `drawFocusRing`
   with the `FXButtonState` contract from the rendering-seam section.
2. Implement the classic style by calling the existing
   `drawRaisedRectangle`/`drawSunkenRectangle`/`drawDoubleRaisedRectangle`
   helpers, so output stays byte-identical.
3. Refactor `FXButton::onPaint` to collect state, call `style->drawButton`,
   then lay out icon and label inside the returned content rectangle; keep
   `drawLabel`, hotkey underline, and press-shift in the widget.
4. Refactor `FXFrame::onPaint` to call `style->drawFrame`; leave `FXLabel`
   chrome-free but switch its colors to `theme->color(Text)` /
   `theme->color(TextDisabled)`.
5. Add `tests/button.cpp` and its closure first (Phase 3), then verify both
   examples are pixel-identical.

#### T3 — INI themes and presets

1. Implement `FXTheme::load(const FXString& path)` with `FXSettings`: map
   `[colors]` keys to roles, warn on unknown keys, leave missing roles at their
   defaults. `[metrics]`/`[fonts]` are parsed but ignored until T5.
2. Add built-in `fox-classic` (today's defaults) and `fox-dark` (lightness
   inverted per role).
3. In `FXApp`, read a `theme` registry entry at startup: a path loads a
   `.foxtheme`, `dark` selects `fox-dark`, anything else falls back to
   `fox-classic`.
4. Run `hello` with `fox-dark` and confirm the registry remembers the choice.

#### T4 — runtime switching

1. `setTheme()` copies roles into the palette mirror, invalidates all `FXStyle`
   caches, and marks every top-level window dirty for a coalesced repaint.
2. If metrics changed, relayout; if font roles changed, recreate the affected
   `FXFont`s; use one `addTimeout` to coalesce the repaint.
3. Verify switching on a live `hello` updates every widget, leaks no resources,
   and does not relayout in a loop.

#### R1 — antialiased rounded corners

1. Add a capability check (`hasXRender`) and a per-`(radius,scale)` alpha-mask
   cache in `FXStyle`; rasterize the hard arcs once, then `XRenderComposite`
   with the cached mask, falling back to `FXImage` alpha blit.
2. Use the mask in `drawButton`/`drawFrame` only when `cornerRadius>0`.
3. Verify a 6 px-radius button shows no stepping at 100/150/200%, and the
   non-XRender fallback still renders.

#### R2 — shadows and rounded focus ring

1. Focus ring (done): rounded ring in `ROLE_FOCUS_RING` at `METRIC_FOCUS_WIDTH`
   when `corner_radius>0`; classic dashed rectangle otherwise.
2. Popup corners (done, 2026-11-17): a themed popup is shaped to a rounded
   rectangle (`FXWindow::setCornerRadius()`), so the corners show what is
   behind the popup instead of its background, and the shape clips the menu
   items that fill their rows. The Wayland backend masks the finished frame,
   because its windows share one buffer and nothing clips a child.
3. Shadow (deferred): cached 9-slice shadow tile under popups/menus. A shadow
   lies outside the widget rect, so popup windows must first be inflated by the
   shadow margin and their children offset; the rounded popup above is the
   consumer that work was waiting for.
4. Verify on `button` (frames, tooltip, status bar) with no per-paint
   allocation.

#### R3 — hover/pressed transitions

1. Store the widget's current and target colors; on `SEL_ENTER`/`SEL_LEAVE`
   drive interpolation with a repeating `addTimeout` over `animationTime`;
   finalize and free the timer on leave.
2. `animationTime=0` disables transitions entirely.
3. Verify transitions complete within the budget and stop cleanly when the
   pointer leaves.

#### T5 — metrics and font roles

1. Implement `FXTheme::metric()` with `[metrics]` keys (padding, margin,
   borderWidth, focusWidth, spacing, iconSize, smallIconSize, rowHeight,
   cornerRadius).
2. Resolve font roles to `FXFontDesc` and create the fonts through the existing
   `FXFont` machinery.
3. Multiply logical metrics by the DPI scale helper at the single conversion
   point from the DPI workstream.
4. Verify padding/border and font role changes appear in `hello`.

#### T6 — GTK-only importer

1. Add a shared parser in the library (e.g. `lib/fxcss.cpp`): comments, strings,
   `@define-color`, a declaration subset, and pseudo-class selectors mapped to
   states; add the same tokenizer as the core of `tools/fxthemeimport.cpp`.
2. Add the GTK 2 `gtkrc` scanner (`bg[]`, `fg[]`, `base[]`, `text[]`,
   `font_name`).
3. Apply the GTK-named-color role table and the derivation rules.
4. The CLI writes `.foxtheme` plus a summary; the runtime loader uses the same
   parser when no cache exists.
5. Verify Breeze GTK 3/4 CSS and the Breeze `gtkrc` convert cleanly with no
   warnings.

#### T7 — theme cache

1. Serialize a resolved `FXTheme` with `FXStream` into a cache file carrying a
   magic/version header.
2. Key on source path, size, mtime, importer version, and schema version;
   validate all of them before use, with bounded sizes.
3. On cache miss/corruption/staleness, delete the cache and re-import; if both
   fail, fall back to `fox-classic` with a single warning.
4. Verify the second startup loads without invoking the parser, and that a
   touched source file or bumped importer version triggers regeneration.

#### T8 — light/dark preference

1. Read `org.gnome.desktop.interface color-scheme` (or the equivalent portal
   setting) once at startup.
2. Map `prefer-dark` to `fox-dark`; `prefer-light` and absent values keep the
   current theme.
3. Verify `hello` honors a `prefer-dark` preference without a config file.

#### Remaining importers — Qt QSS and Xresources (left open)

Reuse the T6 parser and role mapping; the KDE `.colors` importer below landed
first and is the reference for how a new format plugs in.

**Left open for later (not started):**

- **Qt QSS.** Same declaration syntax as CSS, so `FXCSS::loadCSS()` is already
  most of the reader; what is missing is a color source, since QSS files rarely
  carry a palette. Two options when this is picked up: parse `*.qss` for
  `background-color`/`color`/`border-radius` exactly like GTK CSS, or read the
  palette from a Qt settings dump (`QPalette` names such as
  `Window`/`WindowText`/`Base`/`Highlight`) and map those onto the roles. Ship
  it only with a corpus that can be verified the way the GTK and KDE paths
  were.
- **Xresources.** A `key: value` scan for `*background`, `*foreground`,
  `*highlightColor`, `*cursorTheme`, `*font`, plus `Xft.dpi` as a scale hint.
  Low value on its own because it describes one application's palette rather
  than a theme, so it is worth doing only when a user asks for it.

Both formats are already dispatching-friendly: `FXCSS::load()` picks the reader
by suffix and `FXApp::init()` treats any non-`.foxtheme` path as an external
source to import and cache, so adding a loader plus an `apply*()` mapping is
enough — no changes to the CLI, the cache or the runtime.

**Progress — KDE `.colors` done (2026-11-11).** The first "later" importer
landed early because the cache and the GTK path are now stable.
`FXCSS::loadKdeColors()` reads a KDE color scheme into the same declaration
store, using the bracket-quoted section names (`[Colors:Window]`, ...) as
selectors, and `applyKde()` maps them onto the roles:
`[Colors:Window] BackgroundNormal` -> `SURFACE`, `[Colors:View]
BackgroundNormal` -> `WINDOW_BG`, `[Colors:Window] ForegroundNormal` ->
`TEXT`, `ForegroundInactive` -> `TEXT_DISABLED`, `BackgroundAlternate` ->
`BORDER` (a hairline one step off the surface), `[Colors:Selection]` ->
`SELECTION_*`/`MENU_SEL_*`/`ACCENT`, `[Colors:Button] DecorationFocus` ->
`FOCUS_RING`, `[Colors:Tooltip]` -> `TOOLTIP_*`, and `[General] font` ->
`FONT_UI`. KDE writes colors as `r,g,b` triplets, so `lookupTriplet()` was
added next to the CSS color parser, and the QFont string
(`family,pointsize,pixelsize,stylehint,weight,italic,...`) is converted to an
`FXFontDesc` (weight >= 600 becomes bold, italic flag becomes italic).
`FXCSS::load()` dispatches on the `.colors` suffix, so the CLI, the runtime
loader and the cache all accept KDE schemes without extra flags.

Verified on this machine: `BreezeLight.colors` gives surface `#EFF0F1`, text
`#232629`, selection `#3DAEE9`, tooltip `#F7F7F7`, border `#E3E5E7`;
`BreezeDark.colors` gives surface `#202326`, text `#FCFCFC`, border `#292C30`;
`Emerald.colors` gives surface `#FFFFFF`, text `#010101`, selection `#0A246A`.
All three import with 0 warnings. Rendering `hello` with the imported light and
dark schemes gives face `#EFF0F1` and `#202326`, and pointing the registry
straight at `/usr/share/color-schemes/BreezeLight.colors` renders `#EFF0F1`
through the runtime import path.

One runtime bug was found and fixed here: `FXTheme::load()` (the INI reader)
succeeds on a `.colors` file because it *is* an INI file, so the old
"try INI first, import on failure" order silently produced a classic theme.
`FXApp::init()` now dispatches on the `.foxtheme` suffix first and treats every
other path as an external source to import and cache. The KDE border choice
(`BackgroundAlternate`, `#E3E5E7`) is lighter than what the GTK stylesheet
reports (`#C6C8C9`); both are valid hairlines for the same surface, and the
difference is expected because KDE themes draw their frames, not their
backgrounds.

**Progress — ControlPanel theme chooser (2026-11-16).** The Themes tab no
longer stops at the compiled-in `ColorThemes` table. It now builds its list from
those presets plus the themes found under `$XDG_DATA_HOME/themes`,
`$XDG_DATA_DIRS/themes`, `~/.themes` and the `color-schemes` directories, and
it accepts a `gtk.css`, `gtkrc`, `.colors` or `.foxtheme` chosen through the new
`Import...` button. Selecting a source loads it through the cache and the same
`FXCSS`/`FXTheme` path as `FXApp::init()`, maps the semantic roles onto the
color wells for the sampler, and remembers the source in `SETTINGS/theme` so the
choice survives a restart.  The sampler is not merely recolored: the resolved
`FXTheme` is applied through `FXApp::setTheme()`, so the preview uses the real
style, including rounded control corners and the theme's metrics and fonts.
Editing a color well clears the source and falls back to explicit palette
colors, the `Show` filter narrows the list to GNOME/GTK, KDE, FOX presets,
system or user themes, and `Rescan` rebuilds the list after a theme is
installed.  The source line is a footer note instead of a line below the
buttons.  Verified with `Breeze` and `Breeze-Dark`: the chooser lists them,
selecting one renders surface `#EFF0F1` / `#202326` with corner radius 5, the
GTK filter shows 4 installed themes and the system filter 16, and the written
settings contain the source path plus the mapped palette; selecting `FOX`
restores radius 0 and writes `theme=classic` again.

**Progress — Breeze button chrome (2026-11-16).** KCalc shows the target:
the push button face is `#FCFCFC` on the `#EFF0F1` Breeze surface, with a
`#C6C8C9` one-pixel border, radius 5 and an accent border on hover, focus or
for the default button.  The theme model gained `ROLE_BUTTON`, filled from
`theme_button_background_normal_breeze` in GTK CSS and from
`[Colors:Button] BackgroundNormal` in KDE schemes, with a surface-derived
fallback (`#FCFCFC` light, `#292C30` dark).  `FXStyle::drawButton()` uses that
role for rounded buttons, shades checked/pressed/disabled states, and gives
hovered/focused/default buttons the accent border.  Rounded buttons no longer
draw a second focus ring on top of the accent border.  Classic buttons, which
have radius 0, remain pixel-identical because their button role equals the
surface color.  The Wayland fallback had passed the radius doubled to
`fillRoundRectangle()`/`drawRoundRectangle()` (those take the radius, not the
arc diameter), which made the corners twice as round as GTK's; the fallback and
the focus ring now pass the radius once.  Wayland also lacks XRender, so the
style now rasterizes the rounded rectangle itself: the straight bands are
filled, the four corners are blended with 4x4 supersampled coverage through
`FXRaster::blend()`, and a 1 px Breeze-style shadow is drawn under the face.
The hover outline is now its own `ROLE_HOVER_RING`, imported from
`theme_button_decoration_hover*` in GTK CSS and from
`[Colors:Button] DecorationHover` in KDE schemes, falling back on the accent;
focus and default buttons use `ROLE_FOCUS_RING`.  Entering and leaving a
rounded button now marks it dirty even when animation is disabled, so the
hover outline repaints immediately.  The hover colour therefore lives in the
theme and is cached with it, instead of being hard-coded in the style.

**Progress — check and radio indicators (2026-11-16).** `METRIC_CHECK_SIZE`
(default 13, imported from the GTK `check`/`radio` `min-width`) drives the
indicator size.  `FXCheckButton` and `FXRadioButton` now have `applyTheme()`
overrides and draw the modern indicator through
`FXStyle::drawRoundedRectangle()`: face `ROLE_BUTTON`, border `ROLE_BORDER`,
hover/focus borders, checked fill blended from `ROLE_HOVER_RING`, mark
`ROLE_TEXT`, disabled `ROLE_TEXT_DISABLED`, rounded check box and circular
radio.  Enter/leave repaints the indicator even without animation.  The check mark and
the indeterminate dash are drawn with 4x4 supersampled coverage on Wayland
(`FXStyle::drawCheckMark()`), so the glyph is antialiased like the indicator
outline.  X11 builds the same 4x4 coverage into a cached A8 pixmap and
composites it through XRender, which is how GTK/cairo gets antialiased marks
without a server-side primitive.  The classic radius-0 path still uses the
original 13 px bevel, so the default FOX look is unchanged.

**Progress — list views (2026-11-16).** `FXList`, `FXIconList` and `FXTreeList` now
switch their view background to `ROLE_WINDOW_BG`, use `ROLE_TEXT_DISABLED` for
disabled items and skip the classic dashed focus rectangle when the theme has
rounded chrome.  Their row height is at least `METRIC_ROW_HEIGHT` for themed
themes, and `FXHeader` takes its text color from the theme.  Classic radius-0
lists keep their original background, row height and focus rectangles.  Hover
rows, alternating row colors and themed scrollbars are still open.

**Progress — text fields (2026-11-16).** `FXTextField::onPaint()` now draws a
rounded entry face and border through `FXStyle::drawRoundedRectangle()` when
the theme has a corner radius: `ROLE_WINDOW_BG`/`ROLE_SURFACE` for the face,
`ROLE_BORDER` normally and `ROLE_FOCUS_RING` while focused, with the theme's
border width.  Focus changes repaint the whole field so the outline updates.
The classic radius-0 frame and background are untouched.  The widget has no
placeholder API, so no placeholder role is needed yet.

**Progress — list boxes and combo boxes (2026-11-16).** `FXPacker` now reapplies
surface, edge and border colors from the theme, and `FXPopup` themes its shell
colors.  For rounded themes, `FXListBox` and `FXComboBox` drop the outer sunken
frame and the arrow button's classic raised frame, keep the theme background on
the field and arrow area, and retain the original frame styles when the theme
returns to radius 0.  The field, arrow and dropdown list all use the theme
colors.  The combo/list-box widget itself now paints the single rounded shell;
the field and arrow are flat children inset into it, so the shell border and
corners stay intact.  `getDefaultWidth()`/`getDefaultHeight()` include the
inset, and the field's vertical padding is reduced by the same amount, so the
combo keeps the normal 34 px control height it has as a text field or button
instead of shrinking its content.  The popup shadow is still open.

One Wayland repaint trap found here: a themed button used to update its parent
whenever it gained or lost focus, so the message box button-row frame queued a
repaint after its buttons and covered them until the pointer moved.  The
parent repaint is now requested only when the parent is actually the combo or
list-box shell; ordinary dialog buttons do not touch their container.

**Progress — tooltips (2026-11-16).** GTK tooltip import now checks the
Breeze names `tooltip_background_breeze`, `tooltip_text_breeze` and their
generic variants, so Breeze imports `#F7F7F7` on `#232629` instead of the
classic pale yellow.  `FXToolTip` paints a themed background, uses the theme's
border and rounded rectangle, and takes its padding from `METRIC_PADDING`;
classic tips keep the original fixed spacing and border.  When the tip text
changes while the popup is visible, the tip hides, resizes to the new default
size and is shown again, so the next tab's longer text is not clipped.  Rounded
outer corners and a drop shadow still need an alpha/shaped popup surface.

**Progress — tabs (2026-11-16).** `FXTabItem` now draws a themed rounded tab
face, with selected, hover and disabled colors, a focus border and the accent
edge of the selected tab; `FXTabBar`/`FXTabBook` replace the classic 3D frame
with a themed separator on the tab side.  Classic radius-0 tabs keep the
original bevel drawing.  The selected tab no longer gets a full accent
outline just because it has focus: only the top accent edge remains, and the
side that joins the page is squared so the tab does not look like a floating
pill.  `FXStyle::drawFrame()` also reduces themed raised/sunken frames to a
single hairline, so the tab-book page follows the same look.  Left/right/bottom
tab geometry still needs a visual pass.

**Progress — scroll bars and scroll areas (2026-11-16).** `FXScrollBar` now
paints a themed trough, a rounded pill thumb on the full track and hover/drag
shading, and suppresses the stepper arrows and arrow hit zones in rounded
themes.  `FXScrollArea` takes the view window background for rounded themes;
classic bars keep the old stipple/bevel drawing.  Scrollbar thickness still
follows the application setting rather than a theme metric, and scroll area
borders/rounding are open.

**Option 1 vs option 2 (2026-11-16).** Rendering only the preview pane with the
imported theme is not possible with the current contract: `FXStyle` and the
theme are application-global and widgets call `getApp()->getStyle()`.  Per-pane
styling would need a style override that resolves through the widget ancestry,
which is a larger API change than the preview is worth.  Option 2 (theme the
whole dialog) is therefore the chosen behaviour, and the remaining defects were
fixed in the theme core: `FXApp::setTheme()` now applies colors and metrics
without queuing intermediate repaints, lays the tree out top down, suppresses
Wayland exposures during that pass through the new
`fxplatformBeginLayout()`/`fxplatformEndLayout()` seam, grows a window that the
new metrics made too small, and repaints once afterwards.  `FXWindow::applyTheme()`
now updates the base background as well, and the text, selection, cursor and
list colors of `FXTextField`, `FXList`, `FXIconList`, `FXTreeList`, `FXGroupBox`,
`FXListBox`, `FXComboBox` and `FXToolTip` follow the theme, which makes a switch
to a dark import readable.  Verified with `Breeze` and `Breeze-Dark` switched at
runtime on both the X11 and Wayland builds: the dialog resizes from 659x513 to
699x663, all widgets repaint once at their final geometry, the footer note
remains at the bottom, and the preview shows rounded buttons with radius 5.


## Validation

- Pixel comparison of the classic theme against the Phase 3 build
  (screenshot diff, tolerance zero): `hello` for T1, `hello` and `button` for T2.
- Light/dark matrix on `hello` and `button`:
  normal, hover, active, disabled, selected, focused.
- Import round-trip: convert `Breeze` and `Breeze-Dark` GTK 3/4 CSS and the
  Breeze `gtkrc`; confirm all required roles resolve and loads emit no warnings.
- Cache: hit, miss, truncated/corrupt, and stale-source cases (touch the CSS or
  bump the importer version) each behave correctly; a cache hit must not invoke
  the CSS parser.
- Runtime switch on a live window: no stale colors, no leaked resources, no
  relayout loops.
- Rendering quality: corner antialiasing inspected at 100/150/200% scale, both
  with and without XRender; shadow tile reuse confirmed (no per-paint
  allocation); focus ring visible on every focusable control.
- Animation: hover/press transitions measured against `animationTime`, and
  verified to stop and free timers when the pointer leaves.

## Risks and open decisions

- **Antialiasing fidelity:** core X11 arcs are aliased. Smooth corners depend on
  an XRender-backed path (with a cached-bitmap fallback), so quality varies by
  server capability; report which path is active in diagnostics.
- **Cost of soft shadows:** blurring per paint would be too slow; the plan is
  strictly cached 9-slice tiles, and the shadow blur budget is part of the theme
  schema so imports cannot request pathological values.
- **Cache staleness and trust:** a cache keyed only by path goes stale. Key on
  path, size, mtime, importer version, and schema version, and treat the cache as
  untrusted input with a validated header and bounded allocations.
- **Startup cost:** GTK themes such as Breeze and Adwaita contain thousands of
  rules. Parsing at startup is the fallback path; once the cache exists, the
  common case must not touch the parser at all.
- **Importer scope:** GTK ships first because it is the richest available source;
  KDE and Qt follow only after the GTK path and cache are proven, so early
  effort is not spread across formats.
- **Fidelity of imported themes:** GTK themes describe widgets, not roles.
  Several roles will be derived rather than imported; document that per source.
- **Custom-drawn widgets:** artwork stays owner-drawn; the theme supplies
  defaults only.
- **Performance:** role lookups must stay inline and cheap; no per-paint
  allocation or string parsing.
- **API surface:** `FXTheme` becomes public; decide early whether it is exported
  (`FXAPI`) and whether role values are `enum` constants (house style: plain
  `enum` inside `namespace FX`, no `enum class`).
