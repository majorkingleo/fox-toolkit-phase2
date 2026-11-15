# Calculator reference (FOX 1.7.95)

Stage 0 of `docs/calculator-port-plan.md`: the reference the ported application
must reproduce. The reference binary is built by `scripts/build-fox1-linux.sh`
into `tmp/build-fox1/calculator/calculator`.

The application keeps its settings in `$HOME/.config/Calculator.rc`. Run the
reference (and the port) with a private `HOME`; otherwise colours, fonts,
precision and window placement come from the desktop of whoever runs it:

```
rm -rf /tmp/fox1home && mkdir -p /tmp/fox1home
HOME=/tmp/fox1home tmp/build-fox1/calculator/calculator
```

The screenshots were captured with `import -window <id>` and a private `HOME`,
so they show the built-in defaults.

## Screenshots

`screenshots/tests/fox1/x11/`:

| File | Window |
| --- | --- |
| `calculator-main.png` | `FOX Calculator` |
| `calculator-prefs-calculator.png` | `Calculator Preferences`, "Calculator settings" page |
| `calculator-prefs-colors.png` | `Calculator Preferences`, "Color settings" page |
| `calculator-prefs-about.png` | `Calculator Preferences`, "About FOX Calculator" page |
| `calculator-help.png` | `Calculator Help` |

## Window titles and geometries

| Title | Size |
| --- | --- |
| `FOX Calculator` | 400x260 (default placement 50,50) |
| `Calculator Preferences` | 661x295 (placed at main window +80,+80) |
| `Calculator Help` | 652x964 (placed at main window +80,+80) |
| `Change Display Font` | 552x484 |
| `Change Mode Button Font` | 552x484 |
| `Change Operator Button Font` | 552x484 |

The preferences dialog has a button column on the left with three entries,
`Calculator`, `Colors` and `About`, and a `Close` button at the bottom right.
The settings page starts on `Calculator`; the switcher does not remember the
last page.

## Behaviour checklist

Each result is the text of the display field after the step. Buttons are
referred to by their label.

| Step | Result |
| --- | --- |
| fresh start | `0` |
| `2`, `+`, `3`, `=` | `5` |
| `AC`, `9`, `√` | `3` |
| `AC`, `4`, `1/x` | `0.25` |
| `AC`, `7`, `M+`, `AC`, `MR` | `7` |
| `Hex`, `a` | `A` |
| `Dec`, `Deg`, `180`, `sin` | `1.224646799E-16` |
| precision `10` → `11` in the preferences spinner, then `AC`, `1`, `÷`, `3`, `=` | `0.33333333333` |

Preferences:

- `Calculator settings`: exponent mode checkboxes, engineering mode, forced
  decimal point, beep, precision spinner (range 1..30) and the three font
  `Set...` buttons. The spinner shows `10` by default, the value
  `readRegistry()` uses.
- `Color settings`: thirteen colour wells: display, display number, digits,
  hex digits, operators, functions, memory, number base, angle mode, invert,
  hyper, clear all, clear. They display the calculator's current colours and
  send a colour command to the calculator; this application does not open an
  `FXColorDialog` from them.
- `About FOX Calculator`: the big calculator icon and the version text.

Other dialogs:

- Each font `Set...` button opens the matching font dialog, which enumerates
  the installed families (for example `Adwaita Mono [UKWN]`,
  `Adwaita Sans [RSMS]`, `Bitstream Vera Sans Mono [Bi...]`). `Escape`
  cancels it.
- The `?` button next to the title opens the help window with the read-only
  scrollable help text. `Escape` closes it.
- `q` (or the window close) exits the application cleanly and writes
  `$HOME/.config/Calculator.rc`.

## Driving the reference on KWin/Wayland

`XTestFakeMotionEvent`/`XTestFakeButtonEvent` do not reach the client on this
desktop; the real pointer belongs to the compositor. Synthesize the events with
`XSendEvent` instead: deliver `ButtonPress`/`ButtonRelease` to the deepest X
window at the point (FOX creates one X window per widget), preceded by an
`EnterNotify` for the crossing, and deliver `FocusIn` and the key events to the
shell window (`tmp/xinject.c` does this). Keyboard hotkeys only reach the main
window once a button has the focus, so click a button such as `AC` before
typing.
