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
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Text/selection colors, `ROLE_WINDOW_BG` view background, disabled text, themed row height and focus handling; hover rows, alternates and scrollbars remain. |
| `FXTextField` | DONE | Text, selection, cursor and background colors, rounded inset border, focus outline and disabled colors; the widget has no placeholder API. |
| `FXListBox`, `FXComboBox` | PARTIAL | Field/list colors, themed field chrome, flat arrow area and themed popup/list; an integrated single rounded shell and popup shadow remain. |
| `FXToolTip` | PARTIAL | Tooltip colors and font; no rounded frame or shadow. |
| `FXGroupBox` | PARTIAL | Text color/font and frame colors; no themed group-box border. |
| `FXCheckButton`, `FXRadioButton` | DONE | Indicator face/border from the theme, themed check/radio mark, radius, hover/focus border and `METRIC_CHECK_SIZE`; Wayland check mark is antialiased. Classic path unchanged. |
| `ControlPanel` | DONE | Theme chooser, filters, footer source note and live preview. |

## P0: chrome needed for a native-looking dialog

| Widget | State | Needed |
| --- | --- | --- |
| `FXScrollBar` | OPEN | Trough, thumb, hover/pressed/focus states, arrow buttons, radius, thickness from `METRIC_ROW_HEIGHT`/a scrollbar metric. |
| `FXScrollArea` | OPEN | Viewport background and border; clipping/rounding of the viewport; scrollbar spacing. |
| `FXPopup`, `FXMenuPane` | OPEN | Background, border, padding, radius and the cached 9-slice popup shadow; menu panes currently have no `applyTheme()`. |
| `FXToolTip` | PARTIAL | Rounded popup frame and shadow in addition to the colors/font. |
| `FXTabItem`, `FXTabBar`, `FXTabBook` | OPEN | Tab background, selected/hover/focus states, tab radius/height, underline/accent, padding. |
| `FXSwitcher` | OPEN | Themed page background/border; the pages themselves inherit from the children but the switcher chrome does not. |
| `FXStatusBar`, `FXStatusLine` | PARTIAL | Status bar background/border, status text and highlighted text roles, field separators. |
| `FXMenuButton`, `FXOptionMenu` | OPEN | These derive from `FXLabel`, not `FXButton`; they need button chrome and an arrow color/state. |
| `FXMenuCaption`, `FXMenuCommand`, `FXMenuCheck`, `FXMenuRadio`, `FXMenuSeparator` | OPEN | Menu background/text, selected/hover background/text, check/radio marks, separator color and menu item height. |

## P1: common input and data widgets

| Widget | State | Needed |
| --- | --- | --- |
| `FXText` | OPEN | Normal, selected, highlighted and active text/background roles; margins; focus frame; scrollbars. |
| `FXTable` | OPEN | Text/base/hilite/shadow/border/selection/grid/stipple/cell-border and alternating cell background colors; row/column metrics. |
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Row height is wired; still need alternating rows, hover decoration, tree expander/focus styling and themed scrollbars. Header text color is now themed. |
| `FXHeader` | PARTIAL | Header button background/hover/pressed, arrow colors, text color and header height. |
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
| `FXSplitter`, `FX4Splitter`, `FXShutter` | PARTIAL | Divider/grip colors and spacing; the container frame follows `FXFrame`. |
| `FXToolBar`, `FXToolBarShell`, `FXToolBarTab`, `FXToolBarGrip`, `FXDockTitle` | OPEN/PARTIAL | Toolbar background/border, grip dots, tab chrome, drag handle, active/floating state. |
| `FXSeparator` | PARTIAL | Frame colors follow `FXFrame`; line thickness/spacing should come from border metrics. |

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
- Shadow color/offset/blur/alpha and popup shadow budget.

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
| Wayland shadows | OPEN | No 9-slice cached shadow; buttons use only a 1 px bottom shade. |
| X11 rounded corners | DONE with XRender | Bitmap/mask fallback is still hard-edged. |
| Win32 rounded corners | OPEN | `FXStyle_win32.cpp` returns false from `drawRoundRect`; no antialiasing. |
| Win32 shadows | OPEN | Same as Wayland/X11. |
| Popup inflation | OPEN | Needed before menus/tooltips can draw shadows outside their current window bounds. |
| Theme cache | DONE | Keyed by source path/size/mtime, importer version and schema; schema bumps invalidate old role layouts. |

## Suggested order

1. Finish the P0 chrome: `FXScrollBar`, `FXToolTip`, `FXPopup`/`FXMenuPane`,
   tabs, status bar, menu buttons, menu items.
2. Add the missing theme roles and metrics those widgets need (entry, menu,
   popup, scrollbar, tab, button text states).
3. Do the P1 input/data widgets: `FXText`, `FXTable`, lists/trees, sliders,
   spinners, check/radio indicators, headers and calendars.
4. Finish the specialised widgets and dialog chrome.
5. Fill in the GTK/KDE importer mappings and icon/font roles.
6. Add popup shadows and the Win32 rounded/AA path.
