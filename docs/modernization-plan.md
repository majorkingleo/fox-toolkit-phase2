# FOX UI Modernization Plan

## Purpose

Plan an incremental modernization of FOX that preserves its main strengths:

- Small native binaries and a small core dependency footprint.
- Straightforward cross-compilation, especially for Windows.
- The existing C++ widget, target/message, layout, and application APIs wherever
  practical.
- Compile-time selection of platform/frontend code, so applications do not need
  to ship every backend or load a runtime plugin system.

The goals are:

1. Provide configurable, consistent themes.
2. Make Windows builds correctly DPI-aware, including DPI changes at runtime.
3. Create an architecture seam that can support additional native platforms
   and, as a separate experiment, a browser thin client.

This is a proposal, not a commitment to preserve every internal ABI. Public API
and serialization changes should be versioned and reviewed separately.

## Base phases (do these first)

The master plan in `docs/general-plan.md` defines two base phases that must
precede every workstream below:

1. **Rebuild the old library** — configure and build the unmodified
   `fox-1.7.95/` to capture a verified baseline (commands, options, warnings,
   result).
2. **Copy basic parts and start the basic implementation** — create `fox2/`,
   copy the minimal subset needed for a window with a button, make it compile,
   and get the `hello` example running. All changes happen in `fox2/`; the
   original tree stays read-only except for debugging/testing.

Workstreams 1–5 are later phases applied on top of the working `fox2/` base,
not on `fox-1.7.95/`.

## Current architecture and constraints

FOX already separates application code from operating-system window APIs at the
public API level, but much of the implementation is not separated into
independent backends:

- The public API is in `include/`; most widget behavior and platform operations
  are implemented together in `lib/`.
- Platform code is selected with compile-time conditionals such as `WIN32`.
  For example, `FXApp.cpp`, `FXWindow.cpp`, and `FXDCWindow.cpp` contain
  platform-specific paths. `include/xincs.h` also chooses platform headers.
- `lib/Makefile.am` builds one `libFOX-1.7` target from the shared source list.
  Windows has separate Visual Studio project files in `windows/`. This
  compile-time approach is compatible with FOX's small-library and
  cross-compilation goals.
- FOX already has application-wide palette colors and registry persistence in
  `FXApp`; widgets commonly consume those colors while drawing. This is a
  useful base for theming, but it is not yet a complete semantic theme system
  with shared metrics, assets, and reliable live refresh.
- Windows startup currently attempts `DPI_AWARENESS_CONTEXT_SYSTEM_AWARE`
  dynamically in `FXApp.cpp` when available. That is not per-monitor-v2
  awareness and does not by itself rescale existing widget geometry when a
  window moves between monitors.
- Widgets draw through FOX drawing contexts, but platform-specific drawing,
  resource creation, window management, and event translation are interleaved
  with toolkit code. A browser frontend cannot be added merely by swapping the
  native window-system calls.

The plan therefore avoids a large rewrite. It first delivers useful features
inside the current architecture, then extracts seams only where the features
need them.

## Architectural direction

Use three concepts, not one broad "frontend/backend" abstraction:

1. **FOX widget/application core**: widget tree, layout, focus, target/message
   dispatch, commands, timers, and application behavior.
2. **Platform adapter**: native windows, event acquisition/translation,
   clipboard and drag/drop integration, fonts, native resources, DPI/monitor
   queries, and scheduling integration. A build chooses one adapter.
3. **Presentation services**: drawing surface operations, theme tokens and
   metrics, image/font resource realization, and invalidation. Keep the existing
   `FXDC`-style API as the compatibility boundary initially; extract internal
   implementation seams behind it.

The platform adapter and presentation implementation should be selected at
**compile time**. Do not introduce mandatory dynamic loading, a large graphics
framework, or a dependency on a browser runtime for native FOX applications.
Use small interfaces and opaque platform handles; avoid leaking X11, Win32, or
browser-specific types into public cross-platform headers.

The browser is not simply another native window backend. A browser client has
its own event loop, input model, rendering, accessibility, and process/security
boundary. Keep browser transport and protocol in an optional server/frontend
target, not in the default `libFOX` dependency graph.

The browser is selected at **compile time**, not at run time: the application is
compiled for the browser target instead of a native backend. **A dual-mode X11 +
browser binary is explicitly not planned** and must not be designed for. Each
binary contains exactly one frontend.

## Workstreams

### 1. Theme system

The detailed realization plan, including importing colors, metrics, and fonts
from GTK, KDE, Qt, Xresources, and native desktop settings, is in
`docs/theme-plan.md`.

#### Proposed model

Add a small `FXTheme`/theme-provider concept owned by `FXApp` (or its existing
settings services). It should provide:

- **Semantic color roles**, such as window background, surface, text, disabled
  text, border, selection, focus, accent, and tooltip colors. Avoid requiring
  every widget to understand a named theme such as "dark".
- **Metrics** for spacing, control padding, borders, corner treatment, and
  standard icon sizes. Metrics must be expressed in logical units and feed into
  the DPI workstream.
- **Fonts and assets** where needed, resolved by FOX rather than by individual
  controls.
- **System, light, dark, and custom theme sources** as optional configuration.
  Preserve current colors and metrics as the compatibility/default theme until
  a separately reviewed visual change is chosen.

Widgets should request semantic roles and metrics rather than duplicating
palette assumptions. Keep custom drawing possible: a theme supplies defaults,
not a restriction on applications that intentionally draw their own interface.
Use the existing application palette as a compatibility mapping while controls
are migrated.

#### Implementation steps

1. Inventory widget drawing code for hard-coded colors, dimensions, and
   assumptions about fonts/assets. Classify cases that are true semantic roles
   versus intentional artwork.
2. Define stable theme roles, default values matching current FOX behavior,
   and an opt-in API. Resolve theme values centrally; do not make a theme
   subclass per widget.
3. Convert a representative set of common controls first: frames, labels,
   buttons, text fields, menus, and selection/focus states. Use that set to
   validate contrast and role names before broad migration.
4. Add a theme-change notification/invalidation path. A theme swap must repaint
   controls and recalculate layout when metrics or fonts change; recreate only
   resources that actually depend on the changed theme.
5. Add examples and screenshot/visual checks for the default and dark palettes,
   including disabled, focused, selected, hover, and high-contrast states.
6. Migrate remaining controls incrementally. Keep any legacy palette setters
   functioning through the compatibility mapping for the intended
   compatibility window.

#### Acceptance criteria

- Existing applications retain the current appearance unless they opt in or
  configure a different theme.
- Theme colors and metrics are consistent across common controls.
- Theme changes update existing widgets, not only widgets constructed later.
- Applications can override theme values without subclassing all controls.
- Native builds gain no mandatory dependency on a third-party theme engine.

### 2. Windows DPI awareness and scaling

#### Coordinate policy

Define a single coordinate policy before changing widget dimensions. Recommended
direction: public layout dimensions and style metrics are **logical units**;
the Windows adapter converts to device pixels at the native boundary. Keep
conversion in one place and avoid scaling both layout values and drawing values
for the same operation.

This is a potentially API-visible change because existing FOX dimensions have
historically behaved like pixel values. Decide whether to:

- Introduce an opt-in logical-DPI mode first, preserving legacy pixel mode; or
- Make logical units the default only in a major/API-compatibility transition.

Do not silently reinterpret serialized geometry or application-supplied pixel
coordinates.

#### Implementation steps

1. Add a Windows DPI policy setting and establish awareness before creating
   windows. Prefer a Per-Monitor-v2 manifest; retain a dynamically resolved
   API fallback for builds that cannot control the executable manifest. Make
   process-awareness selection explicit and check/report failures rather than
   assuming the request succeeded.
2. Store/query effective DPI per top-level window or monitor. Add an internal
   scale helper for logical-to-device conversion and its inverse. Avoid
   scattered `MulDiv`/percentage calculations in individual widgets.
3. Handle `WM_DPICHANGED`: apply the suggested top-level rectangle, update the
   window scale, recompute fonts and theme metrics, relayout descendants,
   rebuild DPI-dependent resources, and invalidate drawing.
4. Audit all input and geometry boundaries: window placement, client size,
   pointer coordinates, hit testing, drag/drop, scroll increments, text
   measurements, borders, cursors, and image/icon sizing. Normalize input to
   the same logical coordinate space as layout.
5. Make font and bitmap/icon caches DPI-aware. Key cached resources by the
   actual DPI/scale and avoid reusing a low-resolution resource on a high-DPI
   monitor.
6. Add a Windows example and diagnostic output that reports process awareness,
   per-window DPI, logical bounds, and device-pixel bounds.

#### Acceptance criteria

- New windows render crisply at 100%, 150%, and 200% scaling.
- Moving a live window between monitors with different scale factors updates
  size, fonts, metrics, and hit targets without restarting.
- Pointer hit testing remains aligned with drawn controls.
- Legacy mode remains available until the logical-unit compatibility decision
  is made.
- Windows builds remain cross-compilable; new APIs are guarded and have
  fallbacks appropriate to supported Windows SDK/runtime versions.

### 3. Compile-time platform and presentation seams

#### Extract incrementally

Do not begin by moving every function into abstract classes. First identify
platform responsibilities and split a few high-value seams:

- `FXPlatform`: display/session initialization, native top-level windows,
  event queue and event translation, monitor/DPI queries, clipboard, and
  platform-specific input integration.
- `FXSurface` or an internal drawing implementation behind the existing FOX
  drawing context: realization and drawing on a native drawable.
- Resource realization for fonts, images, cursors, and icons, keeping portable
  resource representations distinct from native handles.

Preserve the public FOX widget API and use compile-time-selected
implementations. Keep common behavior in the shared implementation. Make the
native window/drawing types opaque outside the selected platform module.

#### Migration steps

1. Document the current platform-specific responsibilities by source file and
   choose one narrow operation as a pilot.
2. Define internal headers/contracts with no platform types in shared/public
   declarations. Move the pilot's platform implementation into a backend
   source unit and preserve the current Windows/X11 behavior.
3. Build and run the existing examples on both backends. Only then move another
   seam. Avoid broad mechanical extraction that makes the shared control flow
   harder to follow.
4. Add a configure/build option to select the platform implementation. Update
   both Autotools and Visual Studio project definitions. Fail configuration
   clearly if a requested backend or dependency is unavailable.
5. Keep the default native build minimal: compile only the chosen backend and
   optional features selected by the user. Test cross compilation from a
   non-Windows host.
6. Once these seams are stable, evaluate a Wayland-native adapter as its own
   project, including compositor protocol support and graphics-buffer
   integration. Do not claim Wayland support merely because the abstraction
   exists.

#### Acceptance criteria

- Existing native behavior and examples pass without requiring runtime plugin
  loading.
- A normal build contains only its selected platform implementation.
- Public FOX headers remain free of operating-system handles.
- Linux-to-Windows cross-compilation remains a supported validation path.
- The internal contract is small enough that new backends do not force
  unrelated widget rewrites.

### 4. Browser thin-client prototype

Treat this as a separate, optional proof of concept after the platform and
presentation seams exist. The C++ FOX application remains authoritative for
application state and command handling; the browser is a presentation/input
client, not a second application implementation.

**Build model:** a browser build is its own compile-time configuration. The
application is compiled for the browser target, and one binary never serves both
X11 and a browser. Dual-mode X11 + browser builds are out of scope; the frontend
is selected at configure time like any other backend.

There are two plausible rendering strategies:

1. **Remote drawing surface**: serialize FOX drawing operations and assets to a
   browser canvas, and send normalized browser input events back to the FOX
   event loop. This better preserves custom FOX-painted widgets, but requires
   drawing command capture, text/font parity, resource transfer, and custom
   accessibility support.
2. **Semantic widget protocol**: send a widget tree and state deltas; map
   controls to browser DOM elements and translate input back to FOX selectors.
   This can provide native browser accessibility and text input, but FOX's
   current widgets do not expose a complete declarative semantic model, so
   mapping must be designed and maintained per widget.

Start with a bounded prototype (for example, labels, buttons, text fields, and
basic layout) and choose one strategy based on measurable fidelity and effort.
Do not promise that arbitrary custom-drawn widgets, OpenGL, drag/drop, clipboard,
and native dialogs will work in the first version.

#### Required boundaries

- Compile the browser transport/frontend only when explicitly selected; do not
  add it to native library dependencies.
- Run FOX application logic on the server and marshal client input onto the
  FOX event loop. Do not let network threads call widget methods directly.
- Define a versioned protocol for initial tree/state, incremental updates,
  input events, focus, resize, and disconnect/reconnect behavior.
- Bound message sizes and rates; validate all client-originated IDs, values,
  and event payloads before dispatch.
- Default to loopback-only binding. Any remote-access mode requires explicit
  configuration, authentication, origin/CSRF defenses, transport protection,
  and a documented threat model. A "thin client" is still an untrusted client.
- Avoid assuming browser CSS pixels equal FOX logical units; negotiate viewport
  size and device pixel ratio and convert through the same coordinate policy.

#### Prototype acceptance criteria

- A sample FOX app can be operated from a supported browser without duplicating
  its application logic.
- State changes made by the FOX application are reflected in the browser, and
  browser actions reach the same FOX target/message handlers.
- Disconnecting a browser does not corrupt or deadlock the application.
- Native builds and cross-compilation remain unchanged when the prototype is
  disabled.
- A browser build contains no X11 adapter code and an X11 build contains no
  browser transport; each configuration links exactly one frontend.
- Known limitations and security assumptions are explicit.

### 5. Wayland-native adapter (pre-scope)

This is the concrete starting point for the deferred Wayland work. It depends on
workstream 3's stable seams, is not part of the default `libFOX` build, and does
not claim Wayland support just because the seam exists. Its purpose is to give
the seam a known consumer and to surface the decisions that a Wayland backend
forces before implementation starts.

#### Protocol and stack

- Use `libwayland-client` and `wayland-scanner` for generated protocol bindings.
  The compositor protocol is the wire boundary; do not route Wayland through
  X11 or assume Xlib types.
- Core protocols: `wl_compositor`, `wl_surface`, `wl_shm`, `wl_seat`,
  `wl_output`, and `wl_data_device_manager`.
- Desktop shell: `xdg_wm_base` → `xdg_surface` → `xdg_toplevel` for top-level
  windows and `xdg_popup` for menus, combos, and tooltips. Negotiate window
  decorations via `xdg_decoration_manager_v1`.
- Optional scaling/cursor protocols: `wp_fractional_scale_v1`, `wp_viewporter`,
  and `wp_cursor_shape_v1`; each is optional and degrades to an integer output
  scale or a default cursor when absent.

#### Rendering model

Start with **software SHM buffers** as the `FXSurface` realization, mirroring
FOX's existing software drawing path: allocate a `wl_shm` buffer (ARGB8888 or
XRGB8888), draw into it with the existing `FXDC`-style code, then
`wl_surface.attach` + `wl_surface.damage` + `wl_surface.commit`. Wayland has no
server-side backing store, so the adapter must track dirty regions and repaint
from FOX's existing invalidation, replacing X expose events with buffer damage.

Accelerated rendering (`zwp_linux_dmabuf_v1` + EGL) is an optional later path
behind the same compile-time selection. Never leak GL or Wayland handles into
public headers.

#### Input model

`wl_seat` delivers pointer, keyboard, and touch events asynchronously, with no
server round-trip and no display-wide input grab. The adapter must absorb:

- No equivalent of `XGrabPointer`/`XGrabKeyboard`. FOX's grab-based behavior
  (menu tracking, splitter drags, window-move loops) must switch to `xdg_popup`
  grab chains or per-widget implicit grabs.
- Coordinates are relative to the focused surface. The workstream 2 coordinate
  policy (logical units, one conversion point) is the only place to map
  surface-local coordinates into FOX logical space.
- Keymaps arrive through `wl_keyboard`; the client owns modifier state. Key
  translation must not assume X11 keycodes.

#### Window management and geometry

Wayland clients cannot position toplevels absolutely, so `FXWindow` placement
and move APIs, and any code that reads global cursor or window coordinates,
must be adapted behind the `FXPlatform` seam rather than preserved verbatim.
Initial target: top-level show/hide, title, size constraints, maximize, and
fullscreen via `xdg_toplevel`; popups map to `xdg_popup` with an explicit
parent surface and grab chain.

#### DPI and scaling

The compositor reports an integer `scale` per `wl_output` and, where available,
a fractional scale per surface. Feed both into the same internal scale helper
used by Windows DPI (workstream 2) so fonts, metrics, and hit testing stay in
logical units. Prefer `wp_fractional_scale_v1` where supported; otherwise fall
back to the integer output scale and report which path is active.

#### Clipboard and drag/drop

Use `wl_data_device_manager` with `wl_data_source`/`wl_data_offer`. Clipboard is
selection-based and asynchronous: a source must answer `send` requests with file
descriptors after focus is lost. Drag/drop is enter/motion/drop on the client.
This replaces the X11 selection path behind the seam while preserving FOX's
existing clipboard and drag-and-drop message API.

#### Build and integration

Add a configure-time backend selection (for example `--enable-wayland`) so Linux
can build X11 (default), Wayland, or both. Keep the default native build
X11-only until the backend passes acceptance. Generate protocol code with
`wayland-scanner` at build time and keep generated files out of public headers.
Integrate the `wl_display` file descriptor and frame-callback round trips into
the FOX event loop, and document the compositor versions tested.

#### Pre-scope acceptance criteria

- A hello-world window (create, show, resize, close) works on at least Mutter,
  KWin, and one wlroots compositor.
- Software SHM rendering matches X11 output at 100%, 150%, and 200%, and at
  fractional scale where the compositor supports it.
- Pointer and keyboard input, focus, and key translation pass the existing
  event examples.
- Menus and tooltips open as `xdg_popup` and dismiss correctly on outside
  clicks.
- Clipboard and drag/drop pass the existing FOX examples.
- X11 remains the default Linux backend, and cross-compilation is unaffected
  when the backend is disabled.

## Recommended sequencing

0. **Base phases**: rebuild the unmodified `fox-1.7.95/` (baseline), then copy
   the basic subset into `fox2/` and get a minimal window-with-button example
   compiling and running. All later steps operate on `fox2/`.
1. **Baseline and compatibility policy**: inventory hard-coded drawing
   assumptions, test platforms, decide geometry compatibility and supported
   Windows versions.
2. **Theme tokens**: add semantic palette and metrics with current-look
   defaults; migrate common controls. Execute the phases in
   `docs/theme-plan.md`.
3. **Windows DPI**: enable and verify per-monitor awareness, then implement
   centralized scaling and runtime DPI transitions. Reuse theme metrics.
4. **Internal platform seam**: extract and validate narrow Windows/X11
   operations one at a time; update all build systems and cross-build checks.
5. **Browser prototype**: compare drawing-surface versus semantic-widget
   transport on a small demo before committing to a production protocol. Ship it
   as a separate build configuration, not as a run-time mode of the X11 build.
6. **Additional native backend**: execute the Wayland pre-scope (workstream 5)
   against the stable seam, then define production protocol, rendering, and
   packaging goals from its results.

Do not combine a geometry/API migration, broad renderer rewrite, theme rollout,
and browser protocol launch into one release. Each phase should be independently
usable and revertible.

## Validation matrix

| Area | Minimum validation |
|---|---|
| Theme | Default-look regression; light/dark states; focus, disabled, selected; runtime theme switch |
| Windows DPI | 100/150/200%; move between mixed-DPI monitors; resize; font/icon quality; mouse hit testing |
| Native backends | Build and run representative examples for each supported backend |
| Wayland (pre-scope) | SHM rendering parity at 100/150/200% + fractional; input/keymap; `xdg_popup` dismiss; clipboard/drag-drop on ≥2 compositors |
| Cross compile | Build Windows target from a non-Windows host; verify optional backends are excluded |
| Browser prototype | Browser input/output, reconnect, malformed/oversized messages, loopback default, native build unaffected |
| Compatibility | Existing examples, public API checks, serialized settings/geometry compatibility |

## Risks and open decisions

- **Geometry compatibility:** FOX code often uses explicit integer dimensions.
  A logical-DPI default could change application layouts; choose opt-in versus
  major-version transition before implementation.
- **Theme scope:** replacing every custom-drawn control is a large effort.
  Define a supported core widget set and leave intentional custom artwork
  alone.
- **Native look versus FOX look:** system controls, owner-drawn widgets, and a
  browser frontend will not have identical visual behavior. Decide whether
  themes are FOX-defined or track native system conventions.
- **ABI/build matrix:** internal extraction must keep Autotools and Visual
  Studio projects in sync. Public class layout changes may affect ABI even when
  source compatibility is preserved.
- **Browser fidelity:** text rendering, accessibility, IME, focus, modal
  dialogs, OpenGL, clipboard, and drag/drop are not automatically solved by
  serializing drawing commands.
- **Remote operation:** exposing a server changes FOX's threat model. Keep the
  initial prototype local-only and security review the protocol before any
  remote deployment.
- **Wayland compositor diversity:** Mutter, KWin, and wlroots differ in
  decorations, scaling, and popup behavior. Verify the pre-scope on at least
  one of each and keep X11/XWayland as the explicit fallback until coverage is
  proven.

## Code areas to revisit during implementation

- `include/FXApp.h`, `lib/FXApp.cpp`: application palette, settings, platform
  initialization, event loop, and current Windows DPI-awareness setup.
- `include/FXWindow.h`, `lib/FXWindow.cpp`: widget geometry, tree behavior,
  input/event handling, and layout invalidation.
- `include/FXDC.h`, `include/FXDCWindow.h`, `lib/FXDC.cpp`,
  `lib/FXDCWindow.cpp`: portable drawing API and platform drawing realization.
- `include/FXFont.h`, `lib/FXFont.cpp`, `include/FXVisual.h`,
  `lib/FXVisual.cpp`: font/visual resources and cache/realization behavior.
- `lib/Makefile.am`, `configure.ac`, `windows/`: compile-time source selection
  and build-system parity.
- `tests/` and `doc/`: examples, regression coverage, and user-facing
  compatibility guidance.
