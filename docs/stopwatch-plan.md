# Desktop clock / stopwatch / timer: implementation plan

Plan for a low level agent. The application is new (there is no FOX 1.x
original to port), lives in `fox2/apps/stopwatch/`, and was first built and verified
**on the Windows MSYS2 (MinGW-w64) target only**.  Wayland was added later, see
section 13; X11 is still out of scope.  The application code stays platform
neutral (no `#if` blocks, see section 3), so a new target needs only a build and
a test run. Read
`.github/copilot-instructions.md` and the
`fox-toolkit` skill before writing any C++; the `app-port` skill describes the
build wiring (section 3 and 4 of that skill apply here, the origin comparison
does not).

## 1. Requirements

| # | Requirement |
| --- | --- |
| R1 | A small desktop widget that starts without a title bar or native border (`DECOR_NONE`). A right-click menu can temporarily enable a native title bar for moving and resizing; the choice is remembered. |
| R2 | The user chooses the display: digital digits (seven segment) or a classic round analog clock. The choice is remembered. |
| R3 | The default content is the current local time. |
| R4 | At the bottom a row of three switch buttons: **Clock** (current time), **Stopwatch**, **Timer**. The active one is shown pressed. |
| R5 | In borderless mode a middle-mouse drag remains the fast path. In title-bar mode the Win32 title bar handles left-button moving and the native frame handles resizing. No permanent drag button is required. |
| R6 | The last window position, size and title-bar state are remembered and restored. |
| R7 | The stopwatch remembers its last stop values (the value shown when stopped, and a history). |
| R8 | A right click opens a popup that shows an `FXTable` with the history: elapsed value and when it happened (date and time of the stop). |
| R9 | Build and verify with the MSYS2 target (Win32 only for now). |
| R10 | Switching the mode never stops the stopwatch or the timer: both keep running in the background and keep their state while the clock or the other mode is shown. |

Decisions taken in this plan (the agent must not re-open them):

- Binary and target name: `stopwatch` (lower case, like `calculator`).
- Registry application name: `Stopwatch`; sections described in 5.
- Timer ends with a beep and a blinking display until the user presses reset.
  If it ends while another mode is shown, it still beeps and the **Timer**
  button label blinks (`Timer!`); showing the timer mode shows the blinking
  display. Switching the mode does not acknowledge it.
- Stopwatch and timer are background states, not view states (R10). Quitting
  the application stops both: a running stopwatch folds its run into `accum`
  without a history entry, a running timer is saved as paused with its
  remaining time.
- Style (digital or analog) is chosen with a toggle button at the right end of
  the bottom row, so the right click stays free for the history popup.
- The history keeps the last 50 stops, newest first.

## 2. What the library already provides (do not rebuild it)

Verified in the tree at the time of writing:

- Borderless windows: commit `606a764` "Honor borderless windows and add a
  Wayland drag seam". `FXTopWindow::setdecorations()` handles `DECOR_NONE` on
  all three backends (Wayland: client side decorations, no title bar drawn;
  X11: Motif hints with no flags; Win32: caption, border, thick frame and
  system menu styles are cleared).
- `FXTopWindow::startDrag()` (`fox2/include/FXTopWindow.h`):
  - Wayland: calls `xdg_toplevel_move` with the last pointer serial, the
    compositor then moves the window.
  - X11 and Win32: **empty**, "the application moves the window itself".
  - `FXTopWindow::move(x,y)` really moves the window on X11 and Win32; on
    Wayland it only updates the stored position (compositors do not allow an
    absolute position), so calling it is harmless there.
- Event data for the manual drag: `FXEvent::root_x/root_y` (current pointer in
  root coordinates) and `rootclick_x/rootclick_y` (pointer at the press).
- Time: `FXThread::time()` (nanoseconds since the Unix epoch),
  `FXSystem::systemTimeFromTime()`, `FXSystem::localTime(FXTime,format)`,
  `FXSystem::localTimeZoneOffset()` and `daylightSavingsActive()` (all in
  `fox2/include/FXSystem.h`).
- Timers: `FXApp::addTimeout(tgt,sel,ns,ptr)` and `removeTimeout()`.
- Widgets: `FX7Segment`, `FXTable`, `FXPopup`, `FXSwitcher`, `FXButton`,
  `FXSpinner`, `FXCanvas`, `FXDCWindow` (`drawArc`, `fillArc`, `drawLine`,
  `fillPolygon`).
- Persistence: `getApp()->reg()` with `readIntEntry`, `writeIntEntry`,
  `readLongEntry`, `writeLongEntry`, `readStringEntry`, `writeStringEntry`
  (`fox2/include/FXSettings.h`). Calculator saves `x`, `y`, `w`, `h` the same
  way (`fox2/apps/calculator/Calculator.cpp`, around lines 735 and 838).
- A top level window must live on the heap (`new`), see commit `4a7dfaa`.

Known limits to document, not to fix in the application:

- Not verified now (Win32 only), but keep the code neutral so it works later:
  Wayland cannot restore a window position (compositor policy), so R6 only
  works on X11 and Win32; on Wayland `xdg_toplevel_move` takes the pointer away
  from the client, so the matching button release may never arrive and the
  drag state must also be cleared on the next press and on `SEL_LEAVE`.
- X11 and Win32 have no alpha window, so a rounded desktop widget would have
  hard-edged corners; do not try to round the main window.

## 3. Platform rule for the application

The application source must not contain `#if defined(WIN32)` or `#if
defined(X11)`. The middle-drag fast path is written once; the right-click
**Show title bar** fallback delegates movement and resizing to the native
Win32 window manager:

```
middle press   -> startDrag()           (Wayland: compositor moves it;
                                         X11/Win32: no-op)
                  remember dragx=event->root_x-getX(), dragy=event->root_y-getY()
motion         -> if(dragging) move(event->root_x-dragx,event->root_y-dragy)
                                        (X11/Win32: moves; Wayland: stores only)
middle release -> dragging=false, remember the position
```

If a library defect shows up (for example the Win32 `move()` jumps by the
border size), fix it in `fox2/lib/FXTopWindow_win32.cpp` in a separate commit.

## 4. Files

New directory `fox2/apps/stopwatch/`:

| File | Content |
| --- | --- |
| `main.cpp` | Version check, `FXApp application("Stopwatch")`, `init`, `new StopWatch(&application)`, `new FXToolTip`, `create`, `run` (copy the shape of `fox2/apps/calculator/main.cpp`). Also `application.addSignal(SIGINT,...,ID_QUIT)` so the position is saved. |
| `StopWatch.h`, `StopWatch.cpp` | `class StopWatch : public FXMainWindow`: layout, mode switching, timers, persistence, middle drag, right click popup. |
| `ClockView.h`, `ClockView.cpp` | `class ClockView : public FXCanvas`: draws the analog face from three values (hour, minute, second as fractions). Digital mode does not use it. |
| `History.h`, `History.cpp` | `class History`: plain data (no widgets), a list of `Entry{FXTime when; FXTime elapsed;}`, `add()`, `clear()`, `load(FXSettings&)`, `save(FXSettings&)`, `format()` helpers. Unit-testable without a display. |
| `CMakeLists.txt` | see 7. |
| `Makefile.am` | see 7. |
| `README` text goes into `docs/`, not here. |

Every file starts with the license header used by the other fox2 applications.
Follow the FOX formatting rules (2 spaces, brace on the statement line, closing
brace indented to the statement, no space after `if`, `for`, etc.).

No images are needed: the buttons use text labels (`Clock`, `Stopwatch`,
`Timer`, `Start`, `Stop`, `Reset`, `Digital`/`Round`), so there is no
`reswrap` step.

## 5. Design

### 5.1 State

```
enum { MODE_CLOCK, MODE_STOPWATCH, MODE_TIMER };
enum { STYLE_DIGITAL, STYLE_ANALOG };

FXuint  mode;          // persisted
FXuint  style;         // persisted
// stopwatch
FXbool  running;       // not persisted: a restart always shows it stopped
FXTime  accum;         // ns collected before the current run     (persisted)
FXTime  started;       // FXThread::time() at the last start
// timer
FXTime  duration;      // ns, set with the three spinners          (persisted)
FXTime  remaining;     // ns, valid while the timer is running or paused
FXTime  deadline;      // FXThread::time()+remaining at the start
FXbool  timerRunning, timerDone;   // both independent of 'mode' (R10)
// move
FXbool  dragging; FXint dragx,dragy;
History history;
```

Elapsed stopwatch value: `accum + (running ? FXThread::time()-started : 0)`.
Never integrate timer ticks; always recompute from the time stamp so a late
tick does not drift. Check whether the library has a monotonic clock
(`grep -n "clock_gettime\|QueryPerformanceCounter" fox2/lib/FXThread*.cpp`);
use it if there is one, otherwise `FXThread::time()` is acceptable and the
limitation (a wall clock jump during a run) is noted in the plan results.

### 5.2 Window layout

```
FXMainWindow(DECOR_NONE, no padding, no spacing)
  FXVerticalFrame (frame, LAYOUT_FILL, FRAME_LINE, pad 0)    <- the border the missing decoration no longer gives
    FXHorizontalFrame / FXSwitcher "face" (LAYOUT_FILL)
      page DIGITAL : FX7Segment            ("HH:MM:SS", "MM:SS.hh")
      page ANALOG  : ClockView (FXCanvas)  (+ small FXLabel with digital readout below it for stopwatch/timer)
    FXSwitcher "controls" (LAYOUT_FILL_X, only visible for stopwatch/timer)
      page 0 (clock)      : empty FXFrame of height 0
      page 1 (stopwatch)  : [Start/Stop] [Reset]
      page 2 (timer)      : [spin h][spin m][spin s] [Start/Pause] [Reset]
    FXHorizontalFrame "bottom" (LAYOUT_FILL_X|PACK_UNIFORM_WIDTH)
      [Clock] [Stopwatch] [Timer]  [Digital/Round style, clock only]
```

- A one pixel frame (`FRAME_LINE`) is needed: with `DECOR_NONE` the window has
  no edge at all and is invisible against a similar desktop.
- Mode buttons are `FXButton` with `BUTTON_NORMAL`; each has a `SEL_COMMAND`
  (`ID_MODE_CLOCK`, `ID_MODE_STOPWATCH`, `ID_MODE_TIMER`) and a `SEL_UPDATE`
  handler that calls `button->setState(mode==X ? STATE_DOWN : STATE_UP)`.
  Check `FXButton.h` in `fox2/include` for the exact `setState()` signature
  and for `STATE_*`; use `FXToggleButton` or `FXRadioButton` instead if that
  looks right in the Windows screenshot.
- Style button: `FXToggleButton` ("Digital"/"Round") or two entries; `ID_STYLE`.
- Sizes: default about 260x200 for digital, the analog face keeps a square
  (`ClockView::getDefaultWidth/Height` return the same value, face radius is
  `min(w,h)/2-margin`).
- Use theme colors (`getApp()->getForeColor()` etc.) in `ClockView`, never
  literal colors, so the face follows the GTK/KDE theme work in
  `docs/theme-plan.md`. A themed accent for the second hand is optional.

### 5.3 Clock

- One timer id `ID_TICK` drives everything. `onTick()` updates the model
  (timer countdown and finish check, stopwatch value), refreshes the display of
  the **visible** mode, then calls `rearm()`.
- `rearm()` does `removeTimeout(this,ID_TICK)` and arms a new one with the
  shortest delay anything needs right now:
  - visible stopwatch running, or analog face visible with a running
    stopwatch: 50 ms (`50000000`);
  - timer blinking (finished): 500 ms;
  - otherwise the next full second (`1000000000-(now%1000000000)`) so the
    digits change on the second;
  - nothing visible and nothing running (stopped stopwatch, timer idle, mode is
    stopwatch or timer): no tick.
  A background stopwatch or timer on a clock display only needs the one second
  rhythm, plus the model update; the 50 ms rate is for display only.
- Call `rearm()` after every state change (mode switch, start, stop, pause,
  reset, timer finish) and `removeTimeout` in the destructor. A mode switch
  must never reset or stop the model.
- Local time parts: `FXSystem::systemTimeFromTime(st,utc+offset)` where
  `offset=FXSystem::localTimeZoneOffset()` plus
  `FXSystem::daylightSavingsOffset()` when `daylightSavingsActive(utc)` is
  non-zero. **Verify the sign and the daylight handling** by comparing with
  `date` in the MSYS2 shell during verification; if it is wrong, take the hour,
  minute and second from `FXSystem::localTime(utc,"%H:%M:%S")` instead.
- Digital text: `HH:MM:SS`. Stopwatch: `HH:MM:SS.hh` when at least one hour,
  else `MM:SS.hh`. Timer: `HH:MM:SS`.

### 5.4 Analog face (`ClockView::onPaint`)

- Paint with `FXDCWindow dc(this,event)`; fill with `getBackColor()`.
- Face: `fillArc` in the base color, `drawArc` outline (64*360 angle units,
  FOX arcs are in 1/64 degree).
- 60 tick marks with `drawLine`; every fifth longer and thicker (`setLineWidth`).
- Hands as lines from the center: hour angle `=(h%12+m/60)*30`, minute angle
  `=(m+s/60)*6`, second angle `=s*6` (use fractional seconds for a stopwatch
  for a smooth hand). `x=cx+r*sin(a)`, `y=cy-r*cos(a)`; link `m` like the
  calculator does (`target_link_libraries(... m)`).
- For the stopwatch the face shows elapsed time: second hand = seconds,
  minute hand = minutes (so a full turn of the minute hand is an hour); hour
  hand is hidden. For the timer: the second hand shows the remaining seconds
  and a pie wedge (`fillArc`) shows `remaining/duration`.
- `ClockView` has `setTime(hour,min,sec,mode)`; it does not know about clocks,
  timers or the application. It calls `update()` only when the value changed.
- Add a hover tooltip on the face with the full date for the clock mode
  (`ID_QUERY_TIP` in `StopWatch`) - optional, last priority.

### 5.5 Stopwatch

- Controls: **Start/Stop** (one button, label follows the state through
  `SEL_UPDATE`) and **Reset**.
- Start: `started=FXThread::time(); running=true;` arm the 50 ms tick.
- Stop: `accum+=FXThread::time()-started; running=false;` remove the tick;
  **append to the history** `Entry{when=FXThread::time(), elapsed=accum}` and
  save immediately (R7: the last stop value survives a crash).
- Reset: only allowed when stopped; `accum=0`. It does not touch the history
  (use "Clear history" in the popup for that).
- Last stop value: `accum` is persisted on every stop and on exit; at startup
  in stopwatch mode the display shows the stored value, stopped.
- Switching to another mode while the stopwatch runs does **not** stop it; it
  keeps counting and keeps its value, because the value is computed from
  `started` and `accum` on demand. Only the display is swapped. While it runs
  and another mode is shown, the **Stopwatch** button label shows a running
  mark (`Stopwatch *`) so the user can see it. No history entry is written for
  a mode switch.

### 5.6 Timer

- Controls: three `FXSpinner`s (hours 0..99, minutes 0..59, seconds 0..59)
  editing `duration`, **Start/Pause**, **Reset**.
- Start: if `remaining==0` then `remaining=duration`; `deadline=now+remaining`.
- Tick: `remaining=deadline-now`; at `<=0` set `timerDone=true`, `remaining=0`,
  `timerRunning=false`, call `getApp()->beep()`, and blink at 500 ms until
  reset. The model update happens in `onTick()` whatever mode is displayed
  (R10), so a timer started and then hidden still finishes on time.
- While the timer runs and another mode is shown the **Timer** button label
  shows the remaining time or a running mark (`Timer *`); after finishing it
  blinks `Timer!`.
- Switching the mode does not pause or reset the timer; the spinners keep their
  values.
- Pause: `remaining=deadline-now`, remove the tick.
- Reset: `remaining=duration`, `timerDone=false`.
- The spinners are disabled while the timer runs (`SEL_UPDATE` +
  `ID_DISABLE`/`ID_ENABLE`).
- The timer does not write to the history; the history is a stopwatch feature
  (R7/R8).

### 5.7 Moving the window and the position (R5, R6)

A child widget swallows the mouse events that hit it, so the main window only
sees a middle press that lands on its own background. The drag must work over
every widget that covers the window. Message map entries on `StopWatch`:

```
FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,StopWatch::onMiddlePress)
FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,StopWatch::onMiddleRelease)
FXMAPFUNC(SEL_MOTION,0,StopWatch::onMotion)
```

FOX widgets send `SEL_MIDDLEBUTTONPRESS`, `SEL_MIDDLEBUTTONRELEASE`,
`SEL_RIGHTBUTTONPRESS` and `SEL_MOTION` to their target through
`FXWindow::onMiddleBtnPress()` and friends, so for each widget that covers the
window (`FX7Segment`, `ClockView`, the frames, the label of the analog
readout) call `setTarget(this)` with the selector `0` and check that
`StopWatch` receives the events. `FXButton`, `FXSpinner` and the table do not
need to forward them (middle and right click over a button may do nothing).
Test which widgets deliver the events (`-tracelevel`, or a temporary `fxmessage`)
and record the result in section 10. If a stock widget swallows the event,
subclass it in the application to forward the press; do not add an overlay
window.

**Native title-bar fallback (required).** Do not add `MoveGrip.h/.cpp`, a drag
button or a custom `FXDragCorner`. The Win32 native title bar already moves a
top-level window and its native resizable frame already resizes it reliably.
The right-click menu owns a checked **Show title bar** item:

- unchecked (default): decorations `DECOR_NONE`; middle-button drag remains
  available for quick movement;
- checked: call `setDecorations(DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_MENU|
  DECOR_MINIMIZE|DECOR_MAXIMIZE|DECOR_CLOSE)`; Windows supplies the title bar,
  minimize/maximize/close buttons and resizing frame;
- toggling back to borderless must not reset the model or change the current
  switcher page; preserve the client size and clamp the window back on screen;
- use `FXMenuCheck` with a `SEL_UPDATE` handler or `setCheck(true/false)` for
  the menu state; bind `ID_SHOW_TITLEBAR` to `onCmdShowTitleBar()";
- the decoration choice is persisted as `SETTINGS/titlebar` and restored before
  the window is shown. Restore `x`, `y`, `w`, `h`; width and height belong to
  the client area and must use the application's existing `FXTopWindow::resize`
  call (do not use Win32 APIs in the application);
- after a decorated resize, save the new `w` and `h` on `SEL_CONFIGURE`, but
  debounce the registry write: update in memory on each configure and write to
  disk on close, mode change menu popdown, or an idle chore. Do not write the
  registry on every mouse-move event;
- decorate mode is not a new layout or resize handler. Let
  `FXShell::onConfigure()` update geometry, then FOX's normal child layout and
  repaint must run. Remove any application-level resize workaround if it
  duplicates that logic.

Native title-bar mode is the supported answer to difficult drag/resize cases;
a custom drag corner would only duplicate Win32 and could conflict with the
frame resize border. Keep the existing middle-drag handlers as a borderless
convenience; they are not a reason to create a permanent visual control.

When changing decorations, remember the window's current client size, call
`setDecorations()`, call `resize()` to preserve that size, recalculate layout,
clamp position with `place(PLACEMENT_VISIBLE)` if needed, then update and save.
Verify both directions: borderless -> decorated and decorated -> borderless.

Position persistence:

- `loadSettings()` in the constructor or in `create()` before `show()`:
  `x=reg.readIntEntry("SETTINGS","x",-1)`, same for `y`. Call `move(x,y)` only
  when both are present and the point is on the desktop (compare against
  `getRoot()->getWidth()/getHeight()`; clamp so at least 32x32 pixels remain
  visible, otherwise a changed monitor setup strands the window).
- Without stored values place the window with `place(PLACEMENT_SCREEN)`.
- `saveSettings()` on `close()` (`StopWatch::close(FXbool notify)` override
  or the `ID_QUIT` handler as in `Calculator::close`) and at every middle
  release. Save `x`, `y`, `mode`, `style`, `duration`, `remaining`, `accum`.
- The window cannot be closed with a title bar button, so provide a way out:
  `Escape` and `Ctrl+Q` accelerators (`getAccelTable()->addAccel`, see
  `Calculator.cpp` around line 410) and a **Quit** entry in the right click
  popup (5.8). Quitting stops the stopwatch and the timer as described in
  section 1 and saves their state.

### 5.8 Right click popup with the history table (R8)

- Handler `SEL_RIGHTBUTTONPRESS` on the main window and the face widgets (same
  forwarding rule as 5.7). In stopwatch mode it opens the history popup; in
  the clock and timer modes it opens a small `FXMenuPane` with only **Quit**.
- The history popup is an `FXPopup` created once in the constructor with the
  main window as owner, containing
  - an `FXTable` (`TABLE_READONLY|TABLE_COL_SIZABLE|TABLE_NO_COLSELECT`,
    `LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT`, about 320x180),
  - a row of three buttons below the table: **Clear history**, **Quit** and
    **Close** (`FXPopup` is a packer; add an `FXHorizontalFrame` for the
    buttons).
- Table content:
  - Three columns: `#` (1 = newest), `When` (local date and time of the stop,
    `FXSystem::localTime(entry.when,"%Y-%m-%d %H:%M:%S")`), `Elapsed`
    (`HH:MM:SS.hh`).
  - `table->setTableSize(history.size(),3)`, `setColumnText()`,
    `setItemText(row,col,text)`, `fitColumnsToContents(0,3)`, row header off
    (`setRowHeaderMode(0)` or `LAYOUT_FIX_WIDTH` with width 0).
  - Rebuild the content every time the popup opens (`onRightPress`), not on
    every stop: the history is small and the table is hidden otherwise.
- Open with `popup->create(); popup->popup(nullptr,ev->root_x,ev->root_y);`
  check `fox2/include/FXPopup.h` (line 177 `popup(grabto,x,y,w,h)`) and keep
  the popup inside the desktop: when the widget is near the right or bottom
  edge pass an adjusted `x`/`y` (`root_x-width`, `root_y-height`). Close it on
  `ID_POPDOWN`/escape, as `FXPopup` does by itself.
- `FXTable` in a popup: the popup grabs the pointer; verify that scrolling and
  column resizing still work (a table scroll bar drag inside a popup was not
  exercised before). If the grab breaks the scroll bars on Win32, use a small
  borderless `FXDialogBox` (`DECOR_TITLE`-less, `DECOR_BORDER`) shown with
  `execute(PLACEMENT_CURSOR)` instead and record the reason in section 10.

### 5.9 History data and persistence

- `History::Entry{ FXTime when; FXTime elapsed; }`, max 50, newest first.
- Registry layout (written with `reg()`):

```
[SETTINGS]
x=...  y=...  w=...  h=...  titlebar=0|1  mode=0|1|2  style=0|1
[STOPWATCH]
elapsed=<ns, long>           ; last stop value
count=<n>
when0=<ns, long>  elapsed0=<ns, long>
when1=...         elapsed1=...
[TIMER]
duration=<ns, long>
remaining=<ns, long>         ; paused value saved on quit; 0 = idle
```

  Use `writeLongEntry` / `readLongEntry`; a nanosecond value does not fit the
  `FXint` entries. Build the key names with `FXString::value("when%d",i)`.
  `History::save()` first deletes the old `[STOPWATCH]` entries for indices
  `>= count` (`reg().deleteEntry`) so a shrinking history does not leave stale
  rows; check `FXSettings.h` for the exact deletion call.
- `History` has no FOX widget dependency; `FXSettings` is the only dependency,
  so it can be exercised in a small test (see 9).

## 6. Implementation order

Each step ends with a successful MSYS2 build (section 8) before the next.

1. **Skeleton**: `main.cpp`, `StopWatch` with `DECOR_NONE`, one `FXLabel`
   showing the local time, 1 s tick, `Escape`/`Ctrl+Q` quit. Wire the build
   (section 7). Run it; check that the window has no title bar and can be
   closed with `Escape`.
2. **Moving and position** (5.7): verify middle drag in borderless mode and
   persistence of `x`/`y`. If event forwarding from child controls proves
   unreliable, do not add a permanent `MoveGrip`; the supported fallback is the
   right-click **Show title bar** native drag/resize mode implemented in step 4.
3. **Digital clock and switcher**: three pages with equal-width SVG mode
   buttons at the bottom. The clock is digital for now; stopwatch and timer
   pages are digital placeholders.
4. **Window controls fallback**: implement the right-click **Show title bar**
   context command, native Win32 moving and resizing, save/restore `x`, `y`, `w`,
   `h`, `titlebar`, and verify both decoration transitions (5.7).
5. **Analog face** (`ClockView`) and the style toggle; persist `style` (clock
   only).
6. **Stopwatch**: Start/Stop/Reset, display with hundredths, persistence of
   `accum`.
7. **History**: `History` class, saving on stop, loading at start.
8. **Right click history**: add the `FXTable` and history commands to the same
   context menu as **Show title bar** (5.8), Clear history, Quit.
9. **Timer**: spinners, countdown, finish alert, persistence of `duration`
   and `remaining`. Then re-test R10: start the stopwatch and the timer, switch
   modes in every order, neither may stop, reset or lose time.
10. **Polish**: tooltips, disabled states, keyboard accelerators, screenshots
    and final plan results.

## 7. Build wiring

`fox2/apps/stopwatch/CMakeLists.txt`:

```cmake
add_executable(stopwatch
  main.cpp
  StopWatch.cpp
  ClockView.cpp
  History.cpp)
target_link_libraries(stopwatch PRIVATE FOX::FOX m)
```

`fox2/apps/stopwatch/Makefile.am`: copy `fox2/apps/calculator/Makefile.am`,
replace the program name and source list, drop `man_MANS` and the image
`EXTRA_DIST` lines (keep `LICENSE` only if the directory has one; otherwise
leave `EXTRA_DIST` out).

Register the application in the same four places as the `app-port` skill
lists:

| File | Change |
| --- | --- |
| `fox2/apps/CMakeLists.txt` | `add_subdirectory(stopwatch)` |
| `fox2/apps/Makefile.am` | add `stopwatch` to `SUBDIRS` |
| `fox2/configure.ac` | add `apps/stopwatch/Makefile` to `AC_CONFIG_FILES` |
| `fox2/.gitignore` | `/apps/stopwatch/Makefile.in` and `/apps/stopwatch/stopwatch` |

Do not regenerate or commit `Makefile.in` files by hand; the build scripts run
`autoreconf` when `Makefile.am` is newer than `configure`.

## 8. Verification on the MSYS2 target

MSYS2 is at `D:\msys64`. Build with the helper script, CMake first (about
20 s clean):

```powershell
$env:MSYS2_ROOT='D:\msys64'
.\scripts\build-fox2-mingw.ps1 -CMake            # incremental
.\scripts\build-fox2-mingw.ps1 -CMake -Clean     # after CMake file changes
.\scripts\build-fox2-mingw.ps1 -Fast             # Autotools build, the second proof
```

Binaries land in `tmp\build-fox2-win-cmake\bin\stopwatch.exe` (Autotools:
`tmp\build-fox2-win\`). The script copies the MinGW runtime DLLs next to the
binaries, so the program starts from PowerShell without MSYS2 on the PATH.

Run with a private registry location so tests never read or overwrite the
user's settings. FOX 2 on Windows keeps settings under `%APPDATA%` (see the
header comment of `fox2/lib/FXRegistry.cpp`), so for every test run:

```powershell
$env:APPDATA="$PWD\tmp\stopwatch-appdata"; $env:LOCALAPPDATA=$env:APPDATA; $env:USERPROFILE=$env:APPDATA
New-Item -ItemType Directory -Force $env:APPDATA | Out-Null
& .\tmp\build-fox2-win-cmake\bin\stopwatch.exe
```

Delete `tmp\stopwatch-appdata` to test the "no stored settings" path; keep it
to test restore. Check where the settings file really goes on the first run
and write that path into section 10.

Test matrix (every line is a pass/fail with a screenshot where it makes sense):

| # | Check | Expected |
| --- | --- | --- |
| V1 | Start without settings | Window with no title bar, thin frame, current time, **Clock** button shown pressed |
| V2 | Compare the displayed time with `Get-Date` | identical to the second, incl. daylight saving |
| V3 | Style toggle | Digital becomes round clock and back; the choice survives a restart |
| V4 | Middle drag in borderless mode | window follows the pointer without a jump over the shell; if a control swallows the event, use **Show title bar** for native dragging instead of adding a permanent grip |
| V4a | Right-click, enable **Show title bar** | native title bar and resize frame appear without changing the mode; menu item is checked |
| V4b | Drag the title bar, resize all four edges, close and restart | exact client `x`, `y`, `w`, `h` and decorated state are restored |
| V4c | Right-click in decorated mode and clear **Show title bar** | title bar and frame disappear; client content and dimensions remain valid and become borderless |
| V4d | Toggle title bar twice during a running stopwatch/timer | no reset, pause or mode change; no model history entry |
| V5 | Quit with `Escape`, restart | window reappears at the same position; delete the settings and it is centered |
| V6 | Stopwatch Start, wait 5 s, Stop | 5.0x s; History gets one entry |
| V7 | Start the stopwatch, switch to Clock, to Timer, back after 10 s | the stopwatch kept counting without a gap; the **Stopwatch** button showed the running mark while hidden |
| V8 | Quit with a stopped value, restart | the value is still shown; history intact (order, times) |
| V9 | Right click in stopwatch mode | table popup with `#`, `When`, `Elapsed` columns, one row with the stop time and value |
| V10 | Clear history in the popup | history table disappears, **Clear history** button is gone |
| V11 | Switch to timer, set 10 s, start, wait 5 s, switch to clock | timer keeps running, shows remaining time, **Timer** button has a running mark |
| V12 | Switch to stopwatch, stop, check history | new entry at the top, elapsed time correct, shows date and time |
| V13 | Right click in clock mode | small menu with **Quit** only |
| V14 | Resize the window, move it around, quit, restart | position, size and title bar state restored, also after a toggle of the title bar |
| V15 | Check the registry settings | `x`, `y`, `w`, `h`, `titlebar`, `mode`, `style`, `duration`, `remaining`, `accum` present and correct |
| V16 | Test every menu item and button | all must work, also in every combination (stopwatch running, timer running, etc.) |
| V17 | Build and run in Debug | no asserts, check the debug output for warnings or errors |
| V18 | Check the license and about box | correct and complete |

Automated input on Windows: the `capture-test-screenshots.ps1` helper can
start a program, wait, and capture it with `PrintWindow` (`-Programs`,
`-Delay`, `-Click`); extend it only if the application list needs a new
entry. Middle button and right button cannot be sent with `-Click`, so V4,
V9 and V10 need either manual testing or a small `SendMessage` helper
(`WM_MBUTTONDOWN`, `WM_MOUSEMOVE`, `WM_MBUTTONUP`, `WM_RBUTTONDOWN` to the
window handle, the same route `ShotWin.Click` uses for the left button);
add the helper under `tmp/` only, not under `tools/`, unless asked. Native
title-bar drag and resize are preferably checked manually once, because they
are handled by the Win32 non-client loop rather than FOX child-window input.

Screenshots go to `screenshots/tests/fox2/win32/`:

```
stopwatch-clock-digital.png
stopwatch-clock-analog.png
stopwatch-stopwatch.png
stopwatch-history.png
stopwatch-timer.png
stopwatch-titlebar.png
```

Other platforms are out of scope for now; do not build or test them. The
application code must stay free of platform conditionals so the later work is
only a build and a test run:

- X11: WSL as described in the project notes
  (`wsl -d Ubuntu-26.04 -- bash -c 'cd /mnt/d/progs/fox-toolkit-phase2 && FOX_BUILD_DIR=$HOME/build/fox2-cmake ./scripts/build-fox2-linux.sh --cmake --run'`;
  the build directory must be on the WSL file system; put multi-step commands
  in a script under `tmp/`). Expected later: V1 to V5 behave the same.
- Wayland: `./scripts/build-fox2-wayland.sh`. V4 would rely on `startDrag()`;
  V5 position restore is not expected to work.

## 9. Testing the non-visual parts

Add `fox2/tests/historytest.cpp` only if the test list in `fox2/tests` accepts
a console test without a display (check `fox2/tests/CMakeLists.txt` first).
It must cover `History`: add 60 entries and check `size()==50`, newest first,
save into an in-memory `FXSettings`, reload, compare. Skip this step if it
needs more than the existing test pattern.

## 10. Results to fill in by the agent

Leave this section in the plan and complete it as the work proceeds:

- Which widgets forwarded the middle and right button events (5.7):
- Whether the native title-bar fallback was needed during verification (5.7):
- Where the settings file was created on Windows (8):
- Sign/daylight handling used for local time (5.3):
- Popup vs dialog decision for the history table (5.8):
- Library fixes made (each in its own commit):
- Open defects:

## 11. Commits

1. Library fixes, if any (for example in `FXTopWindow_win32.cpp`), one commit.
2. The application: sources, `CMakeLists.txt`, `Makefile.am`, the four
   registration files, screenshots, and the filled-in section 10 of this plan.

Use the commit identity from the project notes:

```
git -c user.name="Martin Oberzalek" -c user.email="kingleo@gmx.at" commit --author="Martin Oberzalek <kingleo@gmx.at>"
```

Do not push. Do not modify anything under `fox-1.7.95/`.

## 12. Risks

| Risk | Mitigation |
| --- | --- |
| Middle press is swallowed by a child widget | Forward mouse events to `StopWatch`; if that remains unreliable, use the native **Show title bar** drag fallback (5.7), never a permanent grip |
| Toggling decorations changes client geometry | Save the client `w`/`h`, call `setDecorations()`, preserve size with `resize()`, layout and clamp with `place(PLACEMENT_VISIBLE)`; verify V4a-V4c |
| A mode switch resets or stops a running stopwatch or timer | The model (`accum`, `started`, `deadline`, `remaining`) is independent of `mode`; only `rearm()` and the display change (5.3); V7, V12a |
| Win32 `move()` jumps by the border width | `getX()`/`getY()` and `move()` use client coordinates through `AdjustWindowRectEx`; with `DECOR_NONE` the adjustment is 0, so check V4 and fix in the library if not |
| `FXTable` in a popup loses scroll bar input | Fall back to a borderless dialog (5.8) |
| Wayland drag leaves `dragging` set | Clear on `SEL_LEAVE` and the next press |
| Timer drift | Compute from time stamps, never count ticks |
| Position or title-bar state restores off screen | Persist client geometry, validate the point/size, and clamp against the desktop (5.7) |
| Local time offset wrong | Cross-check with `Get-Date`; fall back to `localTime(...,"%H:%M:%S")` |

## 13. Progress report: steps 5 to 9, Wayland and Wine

Steps 5 to 9 of section 6 are implemented: the analog face (`ClockView`), the
stopwatch with its history (`History`), the timer, and the right-click history
popup with an `FXTable`.  `main.cpp` is unchanged; the window is still created
on the heap and stays free of platform conditionals.

### 13.1 What was built

- `ClockView` draws the face from `setTime(hour,minute,second,millisecond,mode,
  progress)`: ticks, hour/minute/second hands, and for the timer a pie wedge
  for `remaining/duration`.  Colors come from `FXApp`, never from literals.
- `History` keeps the last 50 stops, newest first, and saves them under
  `[STOPWATCH]`; `formatElapsed()` prints `MM:SS.hh` and `HH:MM:SS.hh`.
- The stopwatch appends to the history on every stop and saves `elapsed`
  immediately, so a crash does not lose the value.
- The timer counts down from three spinners, beeps and blinks at the end, and
  keeps the model independent of the shown mode (R10).
- One tick timer (`ID_TICK`) is re-armed by `rearm()` with the shortest delay
  anything needs: 50 ms while a visible stopwatch or timer runs, 500 ms while
  the finished timer blinks, otherwise the next full second.
- The right-click popup in stopwatch mode shows `#`, `When`, `Elapsed` with
  **Clear history**, **Quit** and **Close**; the row header is hidden with
  `getRowHeader()->hide()` and `setRowHeaderWidth(0)`, the `#` column is 40 px
  wide.
- The analog face uses one readout label under the face; it is emptied in
  clock mode instead of hidden, because hiding it needs an extra layout pass.

### 13.2 Wayland

Built with `./scripts/build-fox2-wayland.sh` and driven by the new gate
`./scripts/gate-wayland-stopwatch.sh` on a headless sway (virtual pointer,
`FOX_WAYLAND_DUMP` frames).  The gate passes:

- 260x200 borderless window, digital clock, `Clock` button pressed;
- mode switch to the stopwatch over the bottom row icons;
- start, 2 s run, stop, value `00:03.xx`, one history row;
- right click on the face opens the history popup (second surface committed);
- style toggle to the analog face, in stopwatch mode and in clock mode;
- R10: start, switch to the clock, wait, switch back, stop.  The second run
  adds 7.6 s of wall clock time; the stopwatch was never paused.  (The gate
  sleeps 1 s and 2 s and each virtual click costs about a second, so the value
  is asserted to continue rather than to match the sleeps exactly.)
- Frames are stored under `screenshots/tests/fox2/wayland/stopwatch-*.png`.

### 13.3 MinGW-w64 and Wine

Cross build: `./scripts/build-fox2-mingw-cross.sh --cmake`, then
`tmp/build-fox2-cross-cmake/bin/stopwatch.exe` under Wine 10.  The driver is
`tmp/stopwatch-win-test.c` (Win32 `PostMessage` clicks, drags, keys, `rect`)
plus `tmp/run-stopwatch-wine.sh`, which keeps a private `WINEPREFIX`.  The run
passes: clock, stopwatch start/stop (3.4 s), history popup with the stopped
value, analog face, timer page, and after a restart the same position, size,
mode and style.  Frames are stored under
`screenshots/tests/fox2/win32/stopwatch-*.png`.

Two findings on the way:

- **The Win32 library was broken at HEAD.**  Commit `55ce6a4` (context menu)
  replaced the 17-entry `stipple_patterns` table in `fox2/lib/FXApp_win32.cpp`
  with a repeated table and swallowed about 280 lines of code, so the file did
  not compile.  The table and the code are restored; only the intended
  `WM_CONTEXTMENU` block remains.  Separate commit.
- **Every FOX widget owns a real HWND on Win32**, and the window procedure
  dispatches a mouse message to the window that owns the HWND.  A message
  posted to the top level window therefore reaches the shell, not the control
  under the point: a test helper must find the child with `WindowFromPoint()`
  and translate the coordinates into its client area.  This is why the first
  Wine runs showed a working right-click (the `WM_CONTEXTMENU` path uses
  `findWindowAt()`) but dead buttons.
- `PrintWindow` returns black for a Wine window, so the frames are grabbed
  with `spectacle` and cut to the window rectangle.  The window has to be
  raised (`SetWindowPos(HWND_TOPMOST)`) first, otherwise the terminal that
  drives the test can cover the point and `WindowFromPoint()` finds it instead.
- With Wine, `%APPDATA%` is redirected: the settings land in
  `<prefix>/drive_c/users/<user>/AppData/Roaming/Stopwatch.ini`, not in the
  directory given in the environment.

### 13.4 Section 10 answers (filled in)

- **Which widgets forwarded the middle and right button events:** `FXSwitcher`,
  `FX7Segment`, `ClockView`, the readout label and `FXSwitcher` of the controls
  are wired with `setTarget(this)` and selector `ID_RIGHT_CLICK`; the message
  map accepts `SEL_MIDDLEBUTTONPRESS`, `SEL_MIDDLEBUTTONRELEASE`, `SEL_MOTION`
  and `SEL_RIGHTBUTTONPRESS` both with id 0 and with `ID_RIGHT_CLICK`.
  `FXButton` and `FXSpinner` are not wired: a middle or right click over a
  control does nothing.
- **Whether the native title-bar fallback was needed:** not for the drag.  The
  middle-button drag works on Wayland through `startDrag()`; over the face
  widgets the forwarded events reach `StopWatch`.  The **Show title bar** item
  is implemented but only checked by hand, not in the automated gates.
- **Where the settings file was created on Windows:** Wine,
  `<prefix>/drive_c/users/<user>/AppData/Roaming/Stopwatch.ini`; native Windows
  uses `%APPDATA%\Stopwatch.ini` (the FXRegistry Win32 default `%APPDATA%`).
- **Sign/daylight handling for local time:** the hour, minute and second are
  taken from `FXSystem::localTime(now,"%H %M %S")`, so no offset arithmetic is
  repeated in the application and daylight saving is whatever the platform
  reports.  The milliseconds come from `now%1000000000`.
- **Popup vs dialog for the history table:** `FXPopup` with an `FXTable`; the
  table scrolls and the buttons work on Wayland and under Wine.  No dialog
  fallback was needed.
- **Library fixes:** the `FXApp_win32.cpp` repair described above.
- **Open defects:** the mode buttons do not show the running mark (`Stopwatch
  *`, `Timer!`) from R10/R5; the timer blink is computed but not drawn; the
  history popup in a decorated window is placed at the pointer, so it can run
  off the bottom edge; the application still lacks the `Stopwatch`/`Timer`
  labels required by the plan in 5.2 (icons only).
