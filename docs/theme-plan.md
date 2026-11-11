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

State variants are resolved by `FXTheme` from the base roles rather than stored
individually: `Normal`, `Hover`, `Active`, `Disabled`, `Selected`, `Focused`.
A theme may override any state explicitly; otherwise the role is derived
(darken/lighten by a fixed amount, blend toward `Surface` for `Disabled`).

### Metrics

Also in `FXTheme`, in logical units (see the DPI workstream):

`padding`, `margin`, `borderWidth`, `focusWidth`, `spacing`, `iconSize`,
`smallIconSize`, `rowHeight`, `cornerRadius` (advisory; FOX draws rectangles).

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
model itself differs — a browser-presented control (workstream 4) or a genuine
native system control — not for a native theme.

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

| Priority | Source | Fields to import | Notes |
|---|---|---|---|
| 1 (first) | GTK 3/4 CSS | `@define-color` names; `color`, `background-color`, `border-color`, `border-width`, `border-radius`, `padding`, `font`, `box-shadow` (as a shadow spec), `linear-gradient` (as a two-stop gradient) | Primary target: Adwaita/Breeze CSS; parse declarations only, ignore images, complex shadow stacks, animations, and layout |
| 2 (first) | GTK 2 `gtkrc` | `bg[NORMAL/SELECTED/INSENSITIVE]`, `fg[...]`, `base[...]`, `text[...]`, `font_name` | Fallback for older GTK themes; same scanner family as the CSS path |
| later | KDE `.colors` | `[Colors:Window]` BackgroundNormal/ForegroundNormal, `[Colors:View]`, `[Colors:Button]`, `[Colors:Selection]`, `[Colors:Tooltip]`, `[General]` fonts | Plain INI, easy to add, but deferred until GTK import is stable |
| later | Qt QSS | same properties as CSS; `QPalette` roles if a palette dump is available | Reuses the CSS scanner |
| later | Xresources | `*background`, `*foreground`, `*highlightColor`, cursor theme | Simple `key: value` scan; low value until requested |
| n/a | Windows | `GetSysColor()` roles, accent color, `SystemParametersInfo` fonts | Handled by the existing `GetSysColor` seed in `FXApp.cpp`, not the importer |
| n/a | GNOME/portal | `color-scheme` preference only (`prefer-light`/`prefer-dark`) | Selects the built-in light or dark preset, not a full theme |

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
| later | — | KDE `.colors`, Qt QSS, Xresources importers | Deferred until the GTK import path is stable and the cache exists |

Order matters: T1–T4 deliver a working theme system with zero visual change;
R1–R3 then deliver the modern rendering; T5 depends on the DPI coordinate
policy; T6 is independent once T3 exists; T7 makes T6 cheap at startup; T8 is
independent of both. KDE, Qt, and Xresources importers wait until after T7.

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

**Progress:** R2 focus ring implemented and verified (2026-11-11).
`FXStyle::drawFocusRing()` draws a rounded ring in `ROLE_FOCUS_RING` using
`METRIC_FOCUS_WIDTH` when `corner_radius>0`, and keeps the classic dashed
`drawFocusRectangle` at radius 0. Verified on a live window: focusing a rounded
button changes 4.5% of its pixels and leaves ~1660 accent `#0A246A` pixels.

**R2 shadow deferred, with reason.** A drop shadow must be drawn *outside* the
widget rect, but a FOX popup/menu window is exactly widget-sized, so a shadow
requires inflating every popup window by the shadow margin and offsetting its
children. `FXPopup.cpp` is not in the `fox2` closure at all yet (only
`FXMenuPane.h`, `FXToolTip`, and `FXStatusBar` are), so there is no consumer to
validate against. The 9-slice shadow-tile cache will land together with popup
support rather than as an unexercised code path.

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
2. Shadow (deferred): cached 9-slice shadow tile under popups/menus. A shadow
   lies outside the widget rect, so popup windows must first be inflated by the
   shadow margin and their children offset; that work belongs with the popup/
   menu build-out, since `FXPopup.cpp` is not in the closure yet.
3. Verify on `button` (frames, tooltip, status bar) with no per-paint
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

#### Later — KDE, Qt, Xresources

Reuse the T6 parser and role mapping once GTK import and the cache are stable;
KDE `.colors` first (plain INI), then Qt QSS, then Xresources.


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
