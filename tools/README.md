# Test tools

Small programs used to drive and capture the applications while testing the
X11, Win32 and Wayland backends.  They are developer tools, not part of the
FOX library; the gates in `scripts/gate-wayland-*.sh` build some of them.

Build everything the installed libraries allow:

```
make -C tools
```

The binaries land in `tools/build/`: `xinject`, `xgrab`, `xlist` (X11) and
`wlxinject`, `screencopy` (Wayland).  `make -C tools clean` removes them.

| Tool | What it does |
| --- | --- |
| `xinject` | Synthesizes or XTest-injects pointer, wheel and keyboard events into an X11 application. |
| `xgrab` | Saves one X11 window as a PPM image, through the Composite pixmap when the window is redirected. |
| `xlist` | Lists the X11 window tree with absolute geometry and map state; `-geom` prints `ID X Y W H` for a title. |
| `wlxinject` | Creates one wlr virtual pointer and moves, presses, releases and scrolls it on a wlroots compositor (sway). |
| `screencopy` | Saves the first output of a wlroots compositor as a PPM image (a minimal grim). |

The sources are documented at the top of each file; `.github/skills/test-tools`
describes how to use them together with headless compositors, OCR and image
comparison.  The protocol XML they need is in `scripts/protocols/`.

Quick examples:

```
# X11 window geometry, then a click 60 pixels right and 25 down from its origin
xlist -geom -title "FOX Calculator"
xinject -title "FOX Calculator" xclick 60 25

# Wayland: float the window, click its title button, capture the screen
swaymsg '[title="FOX Calculator"] floating enable'
wlxinject 372 215 move sleep:200 down sleep:200 up sleep:200
screencopy out.ppm && magick out.ppm out.png
```

`tools/build/` is ignored by git.
