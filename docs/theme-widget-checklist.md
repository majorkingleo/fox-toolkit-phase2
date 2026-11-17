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
| `FXFrame` | DONE | Base/hilite/shadow/border and theme padding. A themed frame draws a single hairline in `ROLE_BORDER` instead of a 3D bevel. |
| `FXPacker`, `FXHorizontalFrame`, `FXVerticalFrame` | DONE | Packers go through the style as well, so `FRAME_SUNKEN`, `FRAME_RAISED`, `FRAME_THICK`, `FRAME_GROOVE` and `FRAME_RIDGE` draw the same hairline a themed frame draws; the classic bevel code is still there for a theme without a corner radius. |
| `FXLabel` | DONE | Text role and UI font. |
| `FXButton` | DONE | `ROLE_BUTTON`, radius, hover/focus decoration, 1 px shadow. Still lacks a separate active/disabled text role. |
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Text/selection colors, `ROLE_WINDOW_BG` view background, disabled text, themed row height, focus handling and themed arrowless scrollbars. `FXList`, `FXIconList` and `FXTreeList` cut the corners of the viewport out of their own item background (`FXStyle::cutRoundedCorners()`) and draw its hairline, so a themed list fits a rounded frame. An icon list in icon mode gets all four corners; in detail mode only the bottom two, because the column header covers the top of the viewport and paints its own square corners there. Hover rows and alternating rows remain. |
| `FXTextField` | DONE | Text, selection, cursor and background colors, rounded inset border, focus outline and disabled colors; the widget has no placeholder API. |
| `FXListBox`, `FXComboBox` | DONE | Single rounded shell with flat inset field and arrow area, themed popup/list colors and focused/hover shell border; popup shadow remains. |
| `FXToolTip` | PARTIAL | Themed background/text/border, theme padding and rounded frame; outer transparency and drop shadow remain. | The font is `FONT_SMALL` while the theme gives that role a face.
| `FXGroupBox` | DONE | Text color and font follow the theme, and the frame is a rounded hairline in `ROLE_BORDER` with the label cutting it open, the way GTK draws it; classic keeps the 3D frame. |
| `FXCheckButton`, `FXRadioButton` | DONE | Indicator face/border from the theme, themed check/radio mark, radius, hover/focus border and `METRIC_CHECK_SIZE`; Wayland check mark is antialiased. Classic path unchanged. |
| `ControlPanel` | DONE | Theme chooser, filters, footer source note and live preview. |

## P0: chrome needed for a native-looking dialog

| Widget | State | Needed |
| --- | --- | --- |
| `FXScrollBar` | PARTIAL | Themed trough, rounded pill thumb at two-thirds track width, full-track travel, hover/drag shading and no stepper arrows or arrow hit zones in rounded themes. The thickness comes from `METRIC_SCROLLBAR_WIDTH`, which the CSS importer fills from the width of the theme's scroll bar buttons, falling back on its trough; an application that calls `setBarSize()` keeps the size it asked for and a theme without the metric leaves the toolkit default alone. |
| `FXScrollArea` | PARTIAL | Viewport background follows `ROLE_WINDOW_BG` for rounded themes; border, viewport clipping/rounding and scrollbar spacing remain. |
| `FXPopup`, `FXMenuPane` | PARTIAL | Flat themed surface and border in rounded themes, with `applyTheme()` on the popup shell. The corners are rounded and transparent through `FXWindow::setCornerRadius()` on all three backends (X11 shape, Win32 window region, Wayland alpha mask). On Wayland the popup is inflated by `METRIC_SHADOW_SIZE` and carries a translucent drop shadow; X11 and Win32 windows have no alpha channel, so they reserve no margin and draw none. Pane padding from `METRIC_PADDING` remains. |
| `FXToolTip` | PARTIAL | Rounded frame, theme padding, a transparent outside and the Wayland drop shadow are in: the tip shapes itself through `FXWindow::setCornerRadius()` and inflates by `METRIC_SHADOW_SIZE` like a popup. X11 and Win32 have no alpha surface, so no shadow there. | The font is `FONT_SMALL` while the theme gives that role a face.
| `FXTabItem`, `FXTabBar`, `FXTabBook` | PARTIAL | Themed tab face, accent edge, square page-side join and hairline themed frames; left/right/bottom orientations still need a visual pass. |
| `FXSwitcher` | DONE | The page background follows the theme surface and the frame is drawn through the style, so a themed switcher keeps a single hairline like every other packer; the pages themselves belong to their children. |
| `FXStatusBar`, `FXStatusLine` | PARTIAL | Status bar background/border, status text and highlighted text roles, field separators. | A status line picks its font from `FONT_SMALL` when the theme has one.
| `FXMenuButton`, `FXOptionMenu` | DONE | Button chrome through `FXStyle::drawButton()`, themed text/arrow/disabled colors, and themed option rows in the popup; classic rendering is unchanged for radius-0 themes. |
| `FXMenuCaption`, `FXMenuCommand`, `FXMenuCheck`, `FXMenuRadio`, `FXMenuSeparator` | PARTIAL | Themed text, active/selected rounded fill, disabled text, antialiased check/radio marks and hairline separators, and menu rows sized from the theme: the content plus `METRIC_ROW_PADDING` and `METRIC_BORDER_WIDTH` on each side, never less than `METRIC_ROW_HEIGHT`, which the CSS importer fills from the `menu menuitem` rule. Explicit per-state colors remain. |

## P1: common input and data widgets

| Widget | State | Needed |
| --- | --- | --- |
| `FXText` | DONE | The document view takes `ROLE_WINDOW_BG` unless the application set a background of its own and the theme rounds its corners, text is `ROLE_TEXT` and the cursor `ROLE_TEXT`, selection is `ROLE_SELECTION_BG` over `ROLE_SELECTION_TEXT`, the line number gutter is `ROLE_SURFACE` with `ROLE_TEXT_DISABLED` numbers, and the active and highlighted spans come from `ROLE_WINDOW_BG`, `ROLE_ACCENT` and `ROLE_SELECTION_TEXT`. Scroll bars are the themed ones. The font is `FONT_MONO` while the theme gives that role a face, and the application font otherwise. |
| `FXTable` | PARTIAL | Cell, grid and selection colors come from the theme roles (`ROLE_TEXT`, `ROLE_WINDOW_BG`, `ROLE_SELECTION_BG`/`ROLE_SELECTION_TEXT`, `ROLE_BORDER` for the grid and cell border, `ROLE_ACCENT` for the stipple), both at construction and on a theme switch. The headers are themed and their labels are no longer clipped: a themed table grows its fixed header height to what the padded items need and drops the vertical padding of the row header, which shares its height with the grid rows. Alternating row backgrounds and row/column metrics remain. A table clips its own painting to the cell area, so a scrolled row cannot paint over its headers on a platform that draws every window into one buffer, and the header separators sit on the same pixel as the column grid lines. |
| `FXList`, `FXIconList`, `FXTreeList` | PARTIAL | Row height and themed scrollbars are wired; still need alternating rows, hover decoration, tree expander/focus styling. Header text color is now themed. |
| `FXHeader` | PARTIAL | Text color and header height are themed, and a themed header drops the raised and sunken buttons: the items are flat and separated by a one pixel hairline in `ROLE_BORDER`, drawn after the items because an item background covers its whole rectangle. Hover and pressed decoration and the sort arrow colors remain. |
| `FXRuler`, `FXRulerView` | DONE | A themed ruler draws its document well as a page in `ROLE_WINDOW_BG` inside a `ROLE_BORDER` hairline instead of the grooved cartouche and the shaded well, the four markers are page-colored arrows with a hairline outline instead of bevelled ones, and the numbers are `ROLE_TEXT`. The ruler view puts the page on `ROLE_SURFACE` instead of the application shadow color, keeping the page itself on `ROLE_WINDOW_BG`. |
| `FXCalendarView`, `FXCalendar` | PARTIAL | Today/title/day/other-day/weekend colors and header/selection states; only the font is currently retargeted. |
| `FXConsole` | DONE | The page takes `ROLE_WINDOW_BG` unless the application set a background and the theme rounds corners, the text `ROLE_TEXT` (it was hard coded white), the selection `ROLE_SELECTION_BG` over `ROLE_SELECTION_TEXT` and the caret `ROLE_TEXT`; the expose is cleared with the background colour instead of the red fill the 1.7.95 release paints, which is why the white text used to be readable. Scroll bars are the themed ones and the font is `FONT_MONO` when the theme has one. The widget itself is unfinished in 1.7.95: its constructor pre-fills a hundred `blabla` placeholder lines, `onXXX` is a stub, the `fillBufferRect` calls are commented out and there is no public way to append text. |
| `FXPathBox`, `FXTreeListBox` | PARTIAL | Their child text field/list controls need the same entry/list treatment. |
| `FXSpinner`, `FXRealSpinner` | DONE | Composed of a themed text field and two themed arrow buttons, so the field, the arrows, their hover and pressed shading and the disabled colours all follow the theme. |
| `FXSlider`, `FXRealSlider` | DONE | A themed slider is a thin rounded trough in `ROLE_SURFACE_SHADOW` with the travel before the head filled with `ROLE_ACCENT`, hairlines for the ticks, and a round head on `ROLE_BUTTON` whose edge is `ROLE_BORDER`, `ROLE_HOVER_RING` under the cursor or `ROLE_FOCUS_RING` with focus. An inside-bar slider uses its slot as the trough, and the arrow variants draw their triangle in `ROLE_TEXT` inside the head. The frame comes from `FXFrame`. |
| `FXRangeSlider` | DONE | A themed range slider draws the same trough as a slider, fills the range between its two heads with `ROLE_ACCENT` and puts two round heads on it; the heads share the slider's edge colours and the arrow variants draw their triangle in `ROLE_TEXT`. |
| `FXArrowButton` | DONE | A themed arrow button draws the chrome a button draws (`FXStyle::drawButton()`), so the arrow of a combo box or a spinner sits on the theme surface with hover and pressed shading; the triangle itself is `ROLE_TEXT` and `ROLE_TEXT_DISABLED` when the button is disabled. Classic keeps its bevels and its own arrow colour. |
| `FXColorWell` | PARTIAL | The frame follows `FXFrame` and the focus ring is the shared themed ring instead of a dashed rectangle; the swatch itself is the value the application set, so only its frame and ring are themeable. |
| `FXColorBar`, `FXColorList`, `FXColorRing`, `FXColorSelector`, `FXColorWheel` | DONE | A themed color bar is the ramp inside a hairline with a rounded handle on it, drawn in `ROLE_WINDOW_BG` over `ROLE_BORDER`, or `ROLE_FOCUS_RING` when the bar has focus; the wheel and the ring replace their four-arc bevel with one hairline circle in `ROLE_BORDER`; the color list draws its swatches with a `ROLE_BORDER` frame and its focus rectangle in `ROLE_FOCUS_RING` on top of the themed list rows. The selector is composed of these, so it follows. |

## P1/P2: specialised and display widgets

| Widget | State | Needed |
| --- | --- | --- |
| `FXGauge` | PARTIAL | Number/caption fonts are retargeted; tick, face and arc colors are not. |
| `FXProgressBar` | PARTIAL | Bar background, filled bar, number/text colors, radius and height. |
| `FXDial`, `FXKnob`, `FX7Segment` | OPEN | Face, arc, pointer and segment colors. |
| `FXMDIChild`, `FXMDIClient`, `FXMDIButton` | PARTIAL/OPEN | MDI title background/text, active/inactive states, border and button decoration. |
| `FXSplitter`, `FX4Splitter`, `FXShutter` | DONE | Neither splitter paints chrome of its own: the divider is the gap between the panes, which shows the surface the packer paints, and the only drawing they do is the inverted rubber band while a pane is dragged. Their container frame goes through the style like any packer, `FXDragCorner` draws three flat grip lines in `ROLE_TEXT_DISABLED`, and a themed shutter no longer shades the content of its item with `ROLE_SURFACE_SHADOW`: it leaves the item on the surface, which is what the surrounding frame uses. |
| `FXToolBar`, `FXToolBarShell`, `FXToolBarTab`, `FXToolBarGrip`, `FXDockTitle` | OPEN/PARTIAL | Toolbar background/border, grip dots, tab chrome, drag handle, active/floating state. |
| `FXSeparator` | DONE | A themed separator is one hairline in `ROLE_BORDER` whatever the 3D style asks for, and it takes one pixel instead of two; classic keeps its bevel. Spacing still comes from the widget options. |

## P2: dialogs and window chrome

| Widget | State | Needed |
| --- | --- | --- |
| `FXTopWindow`, `FXMainWindow`, `FXDialogBox` | PARTIAL | Client background follows `FXWindow`. The Wayland client-side title bar takes its background from the base color and separates itself from the client area with a hairline in `ROLE_BORDER` when the theme has a corner radius; the title text and the close/maximise/minimise glyphs still use the classic app colors, and the X11 and Win32 decorations are the window manager's. |
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
