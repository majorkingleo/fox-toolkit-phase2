# ChartDemo SVG icons

All icons use a 32×32 view box and a centered 28-pixel drawing area. The
set uses a monoline/outline style with 2-pixel strokes, round joins and
rounded caps. Small filled dots are used only where a point or handle needs
extra emphasis.

- `x-axis-label.svg`: horizontal axis and crossing series.
- `y-axis-label.svg`: vertical axis and Y-oriented series.
- `reset-zoom.svg`: return arrow around a reset/magnifier action.
- `zoom.svg`: magnifier with a centered plus sign.
- `data-series.svg`: axes with a multi-point series line.
- `crop.svg`: zoom/crop rectangle with the selection bracket.
- `drag.svg`: pan control with directional arrows and center handle.
- `detach.svg`: external-window arrow and docked frame.

The `dark/` copies use a light foreground for dark surfaces. The regular
copies use `#202124`, suitable for a light surface.

## Dark/light mode mapping

Use exactly one variant for the current theme foreground. Do not recolor
individual SVG paths at draw time: NanoSVG does not provide CSS
`currentColor` here.

For an icon inside a FOX `FXButton`, keep its SVG source and choose the
variant together with the button state:

- Light theme: load the icon from `icons/`.
- Dark theme: load the matching icon from `icons/dark/`.
- Hover/pressed/disabled: retain the same icon shape and use a dedicated
  hover/pressed icon only when the state needs a stronger surface treatment.

The recommended foreground colors are:

- Light theme: `#202124`.
- Dark theme: `#F1F3F4`.
- Disabled state: derive a 40–50% opacity foreground from the theme,
  rather than changing the icon geometry.

Chartdemo embeds both variants with `reswrap` and chooses the light or dark
resource from the current window-background luminance when its icons are
created. The toolbar uses the X/Y label, reset zoom, data series, crop, drag,
and detach icons. Crop selects the chart's rectangular zoom gesture; Drag
switches the plot to left-button panning.

A FOX implementation can select variants like this:

```cpp
const FXchar* resetZoomSvg = chartResetZoomIcon;
FXSVGIcon* icon = new FXSVGIcon(app,(const FXuchar*)resetZoomSvg,
                               FXRGB(192,192,192),0,32,32);
```

For runtime theme selection, expose a small `loadChartIcon(name,darkMode)`
helper that returns the SVG source for either directory. Rebuild or
replace the icon only when the theme changes; `FXSVGIcon` preserves the
vector source and rasterizes it sharply at DPI-scaled sizes.
