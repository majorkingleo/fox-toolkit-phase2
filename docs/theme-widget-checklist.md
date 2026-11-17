# Widget theming checklist

Companion to `docs/theme-plan.md`.  This is the remaining widget work needed
before a GTK/KDE theme can drive every common FOX control, not just the
sampler in `ControlPanel`.

Legend:

- **DONE** — reads theme colors, fonts and metrics; uses the style seam where
  its chrome needs one.
- **PARTIAL** — some theme data is applied, usually colors or fonts, but the
  custom drawing still uses hard-coded assumptions or the old palette.
- **OPEN** — no theme role is applied; the widget still paints from its own
  constructor-time colors or from `getApp()->get*Color()`.

Priority:

- **P0** — required before a normal Breeze/Adwaita dialog feels native.
- **P1** — common controls and list/editor widgets.
- **P2** — specialised controls, dialogs and window decorations.

## Already covered

| Widget | State | Notes |
| --- | --- | --- |
| `FXWindow` | DONE | Base background follows `ROLE_SURFACE`. |
| `FXFrame` | DONE | Base/hilite/shadow/border and theme padding. |
| `FXLabel` | DONE | Text role and UI font. |
| `FXButton` | DONE | `ROLE_BUTTON`, radius, hover/focus decoration, 1 px shadow. Still lacks a separate active/disabled text role. |
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Text/selection colors, `ROLE_WINDOW_BG` view background, disabled text, themed row height, focus handling and themed arrowless scrollbars. `FXList` and `FXTreeList` cut the corners of the viewport out of their own item background (`FXStyle::cutRoundedCorners()`) and draw its hairline, so a themed list fits a rounded frame; `FXIconList` still draws square corners because its content starts below the header. Hover rows and alternating rows remain. |
| `FXTextField` | DONE | Text, selection, cursor and background colors, rounded inset border, focus outline and disabled colors; the widget has no placeholder API. |
| `FXListBox`, `FXComboBox` | DONE | Single rounded shell with flat inset field and arrow area, themed popup/list colors and focused/hover shell border; popup shadow remains. |
| `FXToolTip` | PARTIAL | Themed background/text/border, theme padding and rounded frame; outer transparency and drop shadow remain. |
| `FXGroupBox` | DONE | Text color and font follow the theme, and the frame is a rounded hairline in `ROLE_BORDER` with the label cutting it open, the way GTK draws it; classic keeps the 3D frame. |
| `FXCheckButton`, `FXRadioButton` | DONE | Indicator face/border from the theme, themed check/radio mark, radius, hover/focus border and `METRIC_CHECK_SIZE`; Wayland check mark is antialiased. Classic path unchanged. |
| `ControlPanel` | DONE | Theme chooser, filters, footer source note and live preview. |

## P0: chrome needed for a native-looking dialog

| Widget | State | Needed |
| --- | --- | --- |
| `FXScrollBar` | PARTIAL | Themed trough, rounded pill thumb at two-thirds track width, full-track travel, hover/drag shading and no stepper arrows or arrow hit zones in rounded themes; thickness still comes from the app setting rather than a theme metric. |
| `FXScrollArea` | PARTIAL | Viewport background follows `ROLE_WINDOW_BG` for rounded themes; border, viewport clipping/rounding and scrollbar spacing remain. |
| `FXPopup`, `FXMenuPane` | PARTIAL | Flat themed surface and border in rounded themes, with `applyTheme()` on the popup shell. The corners are rounded and transparent through `FXWindow::setCornerRadius()` on all three backends (X11 shape, Win32 window region, Wayland alpha mask). On Wayland the popup is inflated by `METRIC_SHADOW_SIZE` and carries a translucent drop shadow; X11 and Win32 windows have no alpha channel, so they reserve no margin and draw none. Pane padding from `METRIC_PADDING` remains. |
| `FXToolTip` | PARTIAL | Rounded frame, theme padding, a transparent outside and the Wayland drop shadow are in: the tip shapes itself through `FXWindow::setCornerRadius()` and inflates by `METRIC_SHADOW_SIZE` like a popup. X11 and Win32 have no alpha surface, so no shadow there. |
| `FXTabItem`, `FXTabBar`, `FXTabBook` | PARTIAL | Themed tab face, accent edge, square page-side join and hairline themed frames; left/right/bottom orientations still need a visual pass. |
| `FXSwitcher` | OPEN | Themed page background/border; the pages themselves inherit from the children but the switcher chrome does not. |
| `FXStatusBar`, `FXStatusLine` | PARTIAL | Status bar background/border, status text and highlighted text roles, field separators. |
| `FXMenuButton`, `FXOptionMenu` | DONE | Button chrome through `FXStyle::drawButton()`, themed text/arrow/disabled colors, and themed option rows in the popup; classic rendering is unchanged for radius-0 themes. |
| `FXMenuCaption`, `FXMenuCommand`, `FXMenuCheck`, `FXMenuRadio`, `FXMenuSeparator` | PARTIAL | Themed text, active/selected rounded fill, disabled text, antialiased check/radio marks and hairline separators; menu row height/padding and explicit per-state colors remain. |

## P1: common input and data widgets

| Widget | State | Needed |
| --- | --- | --- |
| `FXText` | OPEN | Normal, selected, highlighted and active text/background roles; margins; focus frame; scrollbars. |
| `FXTable` | OPEN | Text/base/hilite/shadow/border/selection/grid/stipple/cell-border and alternating cell background colors; row/column metrics. |
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Row height and themed scrollbars are wired; still need alternating rows, hover decoration, tree expander/focus styling. Header text color is now themed. |
| `FXHeader` | PARTIAL | Text color and header height are themed, and a themed header drops the raised and sunken buttons: the items are flat and separated by a one pixel hairline in `ROLE_BORDER`, drawn after the items because an item background covers its whole rectangle. Hover and pressed decoration and the sort arrow colors remain. |
| `FXRuler`, `FXRulerView` | PARTIAL | Tick/number text role and tick colors; font is already retargeted. |
| `FXCalendarView`, `FXCalendar` | PARTIAL | Today/title/day/other-day/weekend colors and header/selection states; only the font is currently retargeted. |
| `FXConsole` | OPEN | Text, selection and cursor roles; background; frame/scrollbars. |
| `FXPathBox`, `FXTreeListBox` | PARTIAL | Their child text field/list controls need the same entry/list treatment. |
| `FXSpinner`, `FXRealSpinner` | PARTIAL | Field colors come from the inner text field; arrow buttons and disabled/active states need theme colors and metrics. |
| `FXSlider`, `FXRealSlider`, `FXRangeSlider` | PARTIAL | Frame colors follow `FXFrame`, but slot and thumb colors, thickness, hover/focus states do not. |
| `FXArrowButton` | OPEN | Arrow color, background, hover/pressed/focus chrome. |
| `FXColorWell` | PARTIAL | Frame colors follow `FXFrame`; focus ring and well border need theme roles, the swatch itself is the value. |
| `FXColorBar`, `FXColorList`, `FXColorRing`, `FXColorSelector`, `FXColorWheel` | OPEN | Mostly composed controls, but their labels, frames, focus rings and selection decorations still use constructor-time colors. |

## P1/P2: specialised and display widgets

| Widget | State | Needed |
| --- | --- | --- |
| `FXGauge` | PARTIAL | Number/caption fonts are retargeted; tick, face and arc colors are not. |
| `FXProgressBar` | PARTIAL | Bar background, filled bar, number/text colors, radius and height. |
| `FXDial`, `FXKnob`, `FX7Segment` | OPEN | Face, arc, pointer and segment colors. |
| `FXMDIChild`, `FXMDIClient`, `FXMDIButton` | PARTIAL/OPEN | MDI title background/text, active/inactive states, border and button decoration. |
| `FXSplitter`, `FX4Splitter`, `FXShutter` | PARTIAL | Divider/grip colors and spacing; the container frame follows `FXFrame`. `FXDragCorner` is themed: three flat grip lines in `ROLE_TEXT_DISABLED` instead of shaded ridges. |
| `FXToolBar`, `FXToolBarShell`, `FXToolBarTab`, `FXToolBarGrip`, `FXDockTitle` | OPEN/PARTIAL | Toolbar background/border, grip dots, tab chrome, drag handle, active/floating state. |
| `FXSeparator` | DONE | A themed separator is one hairline in `ROLE_BORDER` whatever the 3D style asks for, and it takes one pixel instead of two; classic keeps its bevel. Spacing still comes from the widget options. |

## P2: dialogs and window chrome

| Widget | State | Needed |
| --- | --- | --- |
| `FXTopWindow`, `FXMainWindow`, `FXDialogBox` | PARTIAL | Client background follows `FXWindow`; client-side title bar, title text, close/maximise/minimise buttons and shadow still depend on the platform/CSD implementation. |
| `FXMessageBox`, `FXInputDialog`, `FXChoiceBox` | PARTIAL | Composed from labels/buttons/text fields; verify dialog background, default button, button spacing and footer after the controls above are done. |
| `FXFileDialog`, `FXDirDialog`, `FXColorDialog`, `FXFontDialog`, `FXPrintDialog` | PARTIAL | Same: tabs, lists, fields, trees and buttons must all follow the theme. |
| `FXProgressDialog`, `FXWizard` | PARTIAL | Header image/background, labels, progress bar and navigation buttons. |
| `FXReplaceDialog` | PARTIAL | Text fields and buttons; it still reads palette colors directly. |
| `FXPopUp`/`FXMenuPane` shadow | OPEN | Requires inflating popup windows by the shadow margin and offsetting their children. |

## Theme model and importer gaps

These are not widgets, but they block “full GTK” coverage even after the widgets
use the seam.

### Missing or weak semantic roles

- Button text: normal, disabled, active/checked.
- Entry/field background and placeholder text.
- Menu background, menu text, menu border, menu hover/selected background.
- Popup background/border/shadow.
- Toolbar/header background, text and border.
- Scrollbar trough/thumb/hover.
- Slider slot/thumb.
- Check/radio indicator background/mark.
- Progress bar background/fill.
- Table/list alternating row, grid and hover.
- Titlebar/CSD background/text/button roles.
- Link, negative, warning and success roles if imported GTK states are to be
  represented faithfully.
- Shadow color, offset and alpha: only the blur of the GTK `box-shadow` is
  imported (`shadow_size`, bounded to 12), and the drawn shadow is 3/8 black
  without an offset.

### Missing metrics

- Control/entry/button height.
- Menu row height and tab height.
- Scrollbar thickness and slider thickness.
- Tab padding and notebook border width.
- Per-widget corner radius (entry, menu/popup, tab, scrollbar), not just one
  global `corner_radius`.
- Text margins inside entries, lists and menus.
- Shadow margin/blur for popup inflation.

### GTK CSS/KDE import

- GTK: expand state parsing beyond `@define-color` and simple declarations:
  `:hover`, `:active`, `:checked`, `:disabled`, `:focus`, `:selected`,
  `entry`, `menu`, `menubar`, `popover`, `headerbar`, `notebook`, `scrollbar`,
  `check`, `radio`, `switch`, `progressbar`, `scale`, `treeview`, `table`.
- Import `box-shadow`, `outline`, gradients and background images as far as
  the renderer supports them; report the rest as approximated.
- KDE: map the remaining `[Colors:Button]`, `[Colors:View]`,
  `[Colors:Window]`, `[Colors:Header]`, `[Colors:Tooltip]` and
  `[Colors:Selection]` foreground/alternate/decorations.
- Fonts: small/bold/mono/title roles and GTK font weight/features.
- Icons: GTK icon theme name and hint sizes.

## Backend/rendering gaps

| Area | State | Needed |
| --- | --- | --- |
| Wayland rounded corners | DONE | Software 4x4 coverage blending. |
| Wayland check marks | DONE | 4x4 supersampled segment coverage through `FXStyle::drawCheckMark()`. |
| X11 check marks | DONE | Cached A8 coverage pixmap composited through XRender. |
| Win32 check marks | PARTIAL | Still uses the GDI line primitives; no GDI+ antialiasing yet. |
| Wayland shadows | PARTIAL | Popups and tool tips carry a translucent shadow: a closed-form ring written with the surface alpha, no per-paint allocation. Buttons still use only the 1 px bottom shade, and X11/Win32 cannot composite a translucent shadow without an alpha window, so they reserve no margin. |
| X11 rounded corners | DONE with XRender | Bitmap/mask fallback is still hard-edged. |
| Win32 rounded corners | PARTIAL | Radii now match the other backends: the DC round-rect calls passed radii to GDI `RoundRect`, which wants the corner ellipse size, so every Win32 corner was half as round. `FXStyle_win32.cpp` still returns false from `drawRoundRect`, so the corners are hard-edged; no antialiasing. |
| Win32 shadows | OPEN | Same as Wayland/X11. |
| Popup inflation | OPEN | Needed before menus/tooltips can draw shadows outside their current window bounds. |
| Popup rounded corners | DONE | `FXWindow::setCornerRadius()`: X11 bounding shape, Win32 window region, Wayland premultiplied corner mask, so the corners show what is behind the popup. Used by `FXPopup` and `FXToolTip`. |
| Theme cache | DONE | Keyed by source path/size/mtime, importer version and schema; schema bumps invalidate old role layouts. |

## Suggested order

1. Finish the P0 chrome: `FXScrollArea` borders/rounding, `FXToolTip`,
   `FXPopup`/`FXMenuPane`, tabs, status bar, menu buttons, menu items.
2. Add the missing theme roles and metrics those widgets need (entry, menu,
   popup, scrollbar, tab, button text states).
3. Do the P1 input/data widgets: `FXText`, `FXTable`, lists/trees, sliders,
   spinners, check/radio indicators, headers and calendars.
4. Finish the specialised widgets and dialog chrome.
5. Fill in the GTK/KDE importer mappings and icon/font roles.
6. Add popup shadows and the Win32 rounded/AA path.
