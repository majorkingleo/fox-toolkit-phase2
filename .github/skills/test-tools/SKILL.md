---
name: test-tools
description: Drive and capture FOX test applications with the small tools in tools/ (xinject, xgrab, xlist, wlxinject, screencopy). Use when a test has to type, click, screenshot or find a window on X11/Xwayland or on a headless Wayland compositor, when a gate misbehaves because input does not arrive, or when a screenshot is black, stale or cannot be read.
---

# Test tools: how to drive and capture the applications

The programs in `tools/` exist so a script can act like a user and check what
the application drew.  They are not part of the library and not installed; the
gates in `scripts/gate-wayland-*.sh` build some of them.

```
make -C tools            # everything the installed libraries allow
ls tools/build           # xinject xgrab xlist wlxinject screencopy
```

The top of each source file is its usage text.  `tools/README.md` has the
one-line summary of each program.

## Choose the input path first

There is no single injection method that works everywhere:

| Situation | Pointer / keyboard | Capture |
| --- | --- | --- |
| X11 app on a real KWin/Wayland desktop | `xinject` default (`XSendEvent`) | `xgrab` on the window, or a compositor screenshot |
| X11 app on a nested Xwayland under sway | `xinject` `x...` commands (XTest) | `xgrab` (Composite) |
| Wayland app under sway | `wlxinject` + `wtype` | `screencopy` or `FOX_WAYLAND_DUMP` |
| Wayland app under weston | no seat, so no input | `FOX_WAYLAND_DUMP` |

On KWin/Wayland the real pointer belongs to the compositor, so `XTest`
(`XWarpPointer`, `xdotool`) does not move it and the events never reach the
client.  `xinject` therefore synthesizes events with `XSendEvent` and delivers
them to the deepest X window at the requested point; that is what FOX sees.
On a nested X server (Xwayland under sway, Xvfb) XTest is the right thing
because the application follows the real pointer.

## X11: xinject, xlist, xgrab

### Find the window and its place

```
xlist -title "FOX Calculator"          # tree, mapped windows only
xlist -all -title "FOX Calculator"     # include unmapped widgets
xlist -geom -title "FOX Calculator"    # "0x1200112 312 190 400 260"
```

`xlist` matches `WM_NAME`, which is what FOX sets from the window title.  Its
geometry is in root coordinates, and its map state tells you whether the
window is really mapped: `map=0` is unmapped and no `XGetImage` will work on
it.

### Click and type

Coordinates are relative to the titled toplevel; `-title` is required in
practice, otherwise the root window is used.

```
xinject -title "FOX Calculator" id               # shell X window id
xinject -title "FOX Calculator" tree             # widget windows
xinject -title "FOX Calculator" click 60 25      # synthesized click
xinject -title "FOX Calculator" move 300 20      # enter + motion
xinject -title "FOX Calculator" type "2+3="      # synthesized keys
xinject -title "Calculator Help" wheel 3         # wheel down, X11 Button5
xinject -title "FOX Calculator" key Escape
xinject -title "FOX Calculator" xclick 60 25     # XTest click
xinject -title "FOX Calculator" xtype "2+3="     # XTest keys
xinject -title "FOX Calculator" sleep 0.5
```

The synthesized `click` sends `EnterNotify`, `MotionNotify`, `ButtonPress` and
`ButtonRelease` to the deepest X window under the point.  Give the application
time to process the motion; the tool already sleeps a little between the
steps, but a `sleep 0.3` between unrelated actions in the script is cheap and
saves flakiness.

Keyboard facts worth knowing:

- A key goes to the focused widget.  A read-only text field such as the
  calculator's display swallows hot keys, so click a button (or the window
  background) before typing when the application handles the key in its
  accelerator table.
- `type` maps each character to the keysym of a level-0/level-1 key, so
  shifted characters need the shift level to resolve.  A synthetic `+` that
  carries no `ShiftMask` will not match an accelerator registered as
  `MKUINT(KEY_plus,SHIFTMASK)`; use the button or an unshifted key in the test.
- Wine's keyboard layout can also differ from the X keymap; the Wine checklist
  drives the calculator with clicks for exactly that reason.

### Capture a window

```
xgrab 0x1200112 out.ppm
magick out.ppm out.png
```

`xgrab` first tries `XCompositeNameWindowPixmap`, because a compositor
redirects the window and the window drawable itself cannot be read; it falls
back to `XGetImage` for an unredirected server.  Both fail with `BadMatch`
when the window is not mapped.  On a locked desktop session every X11 window is
unmapped, which is why the X11 checks run on a private Xwayland under sway
instead of on the real display.

## Wayland: wlxinject and screencopy

### A private headless compositor

The gates use this recipe; it also works by hand:

```sh
RUN=tmp/sway-manual
rm -rf "$RUN"; mkdir -p "$RUN/home"; chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"
export XDG_RUNTIME_DIR="$RUN" HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS XDG_CONFIG_HOME
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 \
       WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
export WAYLAND_DISPLAY=$(basename "$(ls "$RUN"/wayland-* | head -1)")
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock | head -1)
$BIN &                    # the application under test
swaymsg '[title="FOX Calculator"] floating enable'
```

- `chmod 700` matters: `XDG_RUNTIME_DIR` is required to be private.
- A private `HOME` plus no session bus keeps the desktop theme out of a pixel
  comparison.
- `default_border pixel 0` makes the toplevel exactly the client size.  A
  compositor that offers no server side decoration also means the frame is the
  client area, so no title bar rows have to be skipped.
- A window is tiled unless a `for_window [...] floating enable` rule or a
  `swaymsg` after it appears says otherwise.  Tiling resizes it, which changes
  the layout you are comparing; float it before capturing.

### Pointer and keyboard

```
wlxinject 612 210 move sleep:300 move:690,292 sleep:300 down sleep:300 up sleep:300
```

`wlxinject` creates **one** virtual pointer, keeps it alive for the whole
gesture and then moves, presses, releases and scrolls it.  Each invocation
creates a new pointer, which makes the seat change capabilities in the middle
of a test; put all steps of one gesture in one invocation.  Actions are `down`,
`up`, `move` (to the command-line point), `move:X,Y`, `sleep:MS`, `rightdown`,
`rightup`, `wheel:<notches>`; positive notches are a wheel down (the protocol's
positive axis, towards the end of the document), negative ones a wheel up.
Coordinates are layout coordinates of the output.

- A pointer needs a motion after it is created before the client sees
  `EnterNotify`; the first point should therefore be inside the window, and a
  later `move:` to the widget.
- The compositor drops a press when it arrives in the same frame as the
  pointer's entrance; leave `sleep:` between the steps, especially in sway.
- `wlxinject` has no keyboard.  Use `wtype` for keys and text:
  `wtype -s 120 -k 6`, `wtype -s 120 -k slash`, `wtype -s 120 "text"`.
  `wtype -k plus` builds a minimal keymap where `+` sits at level 0, so a
  `Shift+=` hot key will not fire; use unshifted keys for shifted
  accelerators, or press the button with the pointer.
- A headless sway has no keyboard seat until `wtype` creates the virtual
  keyboard.  That is why a frame captured before typing has no focus ring and
  one captured after it may have one; keep the two sides of a pixel gate in the
  same state.

### Capture the screen

```
screencopy out.ppm
magick out.ppm out.png
```

`screencopy` is a minimal grim: it binds `zwlr_screencopy_manager_v1`, asks
for the first output and writes the pixels.  It works on wlroots compositors
(sway) and not on KWin or weston; for a Wayland application there is also
`FOX_WAYLAND_DUMP=<dir>`, which writes the client's own committed frames as
`window-<id>-<w>x<h>[-bar<n>].ppm` and works on every compositor.  The dump is
usually better for pixel gates: it is the exact client buffer, without the
desktop background or decorations.

For an X11 application on a nested Xwayland under sway, the X server is
started by sway itself; a separately started `Xwayland` is not connected to
sway's XWM, so its windows are never mapped.  Find the display sway created by
looking for the new `/tmp/.X11-unix/X<n>` socket, and use `xlist`/`xgrab` on
that display.

## Checking what was drawn

- **OCR**: capture, crop tightly to the text, upscale 400–500%, then
  `tesseract crop.png - --psm 7`.  A single character at the edge of a field
  is often misread when the crop includes the border; crop inside it, and
  normalize common confusions (`S`→`5`, `O`→`0`, `|`→`1`) when asserting a
  number.
- **Image comparison**: `magick compare -metric AE a.png b.png null:` gives
  the number of differing pixels.  A pixel gate on a classically themed
  application should be 0; a budget (a few hundred pixels in a large dialog)
  covers the text antialiasing difference between compositors, while a real
  regression (a hidden page painted, a moved widget) is thousands of pixels.
- **Focus and hover change pixels.**  A window with the keyboard focus draws
  its focus ring, a hovered button is highlighted, and a headless compositor
  with no keyboard seat cannot draw the ring.  Capture both sides in the same
  state: click a label rather than a button when only the update pass is
  wanted, and keep the pointer away from buttons that change on hover.
- Prefer comparing a frame with a fresh repaint of the same state to prove
  that an initial paint is complete; `scripts/gate-wayland-prefs.sh` does that
  against the X11 reference.

## When a tool "does nothing"

| Symptom | Cause | Fix |
| --- | --- | --- |
| `xinject` events have no effect on KWin | XTest does not move the real pointer | use the default synthesized commands, or a nested Xwayland under sway |
| keys are ignored on X11 | the focused widget (a read-only field) swallows them | click a button first, or click the window background |
| the window exists but `xgrab` says `BadMatch` | the window is unmapped (locked session, hidden toplevel) | check `xlist -all` map state; capture through the compositor |
| `wlxinject` creates a seat but no enter | the pointer never moves into the surface | move to a point inside the window; the first move after creation may be dropped, send another |
| a Wayland client dies as soon as it appears | a protocol error closed the connection | `WAYLAND_DEBUG=1` and `journalctl --user` (KWin logs `error in client communication`) |
| a screenshot is black or stale | the client buffer was not repainted, or the compositor output is not the app | use `FOX_WAYLAND_DUMP`, or force a repaint by changing state |

The gate scripts are the worked examples: `gate-wayland-cursor.sh` and
`gate-wayland-prefs.sh` show a click plus a dump comparison,
`gate-wayland-keys.sh` shows `wtype` driving the accelerator table, and
`gate-wayland-click.sh` shows the one-pointer-per-gesture rule.
