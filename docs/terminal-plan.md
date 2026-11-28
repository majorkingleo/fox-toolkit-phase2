# Cross-platform terminal: implementation plan

Plan for a new terminal application built from the console example
(`fox2/tests/console.cpp`), with Windows Terminal style tabs, detachable tabs
and selectable `xterm` or `vt100` emulation.

The plan is written for the next agent to execute. The decisions in this
document are closed unless a build or test proves one of them impossible.

## 1. Requirements

| # | Requirement |
| --- | --- |
| R1 | Run real interactive programs through a pseudo terminal on Linux/X11, Linux/Wayland and Windows. |
| R2 | Display a complete VT100 screen model: cursor addressing, scrolling regions, erase/insert/delete, alternate screen, tab stops, character sets and standard SGR attributes. |
| R3 | Support an `xterm` profile for 16/256/truecolor, title reporting, alternate screen, bracketed paste, focus reporting and mouse reporting. |
| R4 | Support a `vt100` profile that advertises `TERM=vt100`, uses only VT100 behavior and ignores xterm-only extensions. |
| R5 | Keep bounded scrollback with selection, copy, paste, find, and follow-output behavior. |
| R6 | Show multiple sessions as tabs in one window, using the compact tab strip that Windows Terminal uses: active tab, close button, plus button, profile drop-down, reorder and overflow. |
| R7 | A tab can be detached into its own native window without interrupting the running process. The new window keeps the session, screen contents and scrollback. |
| R8 | Tabs can be created, closed, duplicated, renamed, reordered, switched and detached by mouse and keyboard. |
| R9 | The application includes profiles for shell, arguments, starting directory, terminal type, color scheme, font and scrollback, saved across runs. |
| R10 | Window geometry, active profile and appearance settings survive a restart. Running tabs are not restored across application restart. |
| R11 | The terminal is usable with keyboard, mouse, clipboard and an input method (IME). |
| R12 | The application source contains no platform conditionals. All platform code lives behind the library seams. |
| R13 | Build and run on Linux/X11, Linux/Wayland and Windows/MinGW-w64. The POSIX code must also compile on macOS, but FOX currently has no Cocoa backend; macOS support means an X11/XQuartz build. |
| R14 | The existing console test and `FXConsole` continue to work unchanged. The new application is not a replacement for the logging widget. |
| R15 | Escape sequences from a child process are treated as untrusted input: bounded memory, bounded parse work, no implicit clipboard writes and no implicit shell execution. |
| R16 | Background transparency is opt-in through `transparencyMode` (`off`, `window`, `background`) and `backgroundOpacity` (0..100). The default is opaque. |
| R17 | In `background` mode the terminal cell backgrounds are translucent while text, cursor and selection foregrounds stay opaque. The tab strip, menus and other chrome stay opaque. |
| R18 | Every backend advertises which transparency modes it supports. An unsupported mode downgrades to the next lower mode, is reported in the settings dialog and never produces corrupted output. |
| R19 | Right-click context menu in the terminal view: Copy, Paste, Select All, Find, Clear Scrollback, Reset Terminal, Character Encoding submenu, New Tab, Detach Tab, Close Tab. A right-click context menu on a tab offers Duplicate, Rename, Detach, Close, Close Others. When mouse reporting is active, Shift+right-click opens the menu. Items that cannot act are disabled, not hidden. |
| R20 | The character encoding of each session is selectable at run time, not only in the profile. Switching between UTF-8 and Latin-1 takes one menu choice and applies to output decoding, keyboard/paste encoding and cell width. It does not restart the child or clear the screen. The set includes at least UTF-8, ISO-8859-1 (Latin-1), ISO-8859-2, ISO-8859-15, Windows-1250, Windows-1251, Windows-1252, KOI8-R, CP437 and CP850. A profile sets the default encoding. The `LANG`/`LC_ALL` value given to the child follows the profile, not the live choice. |

## 2. Decisions

- **Application name and location.** New application `fox2/apps/foxterm/`, binary
  `foxterm`, registry application key `FoxTerm`.
- **The console example is the UI starting point, not the terminal engine.**
  Copy the shape of `tests/console.cpp` (main window, menus, frame, status) into
  `foxterm`, but use the new `FXTerminal` widget. Do not turn `FXConsole` into a
  terminal; its logical-line model cannot represent cursor addressing.
- **The terminal view is a library widget.** `FXTerminal` is public, like
  `FXConsole`; the ANSI parser and screen model stay private headers in
  `fox2/lib/`.
- **Tabs are application state.** `FXTabBook`/`FXTabBar` do not support close
  buttons, tab overflow, drag reordering or tear-off, so the application uses a
  custom `TerminalTabBar`. Do not modify `FXTabBook` for this application.
- **Native window decorations.** The tab strip sits directly below the native
  title bar. Embedding tabs into the operating-system title bar, as Windows
  Terminal does, is not implemented: it would require a custom title bar and
  client-side decorations on every backend, including compositor-controlled
  Wayland. The visual style of the tab strip still follows Windows Terminal.
- **One process per tab.** A tab owns one shell and one PTY. Split panes,
  sessions attached to an external server and cross-process tab transfer are
  out of scope.
- **Detaching is an in-process move.** The terminal widget is reparented into a
  new `TerminalWindow`; the PTY and parser are not restarted. Dragging a tab
  between two `foxterm` processes is out of scope.
- **POSIX uses the kernel PTY.** `posix_openpt()`/`grantpt()`/`unlockpt()` and
  `fork()`/`setsid()`/`ioctl(TIOCSCTTY)`; no `forkpty()` dependency.
- **Windows uses ConPTY.** Load `CreatePseudoConsole`,
  `ResizePseudoConsole` and `ClosePseudoConsole` dynamically from `kernel32`.
  Windows 10 1809 is the minimum. If the API is missing, the tab shows an error
  and no shell is started; the application does not silently fall back to pipes.
- **Settings use `FXRegistry`.** No JSON parser, no second settings format and
  no new dependency. The registry already has the correct per-platform paths
  and escaping.
- **No third-party terminal library.** The parser, screen model and PTY seam are
  part of the library. No `libvterm`, `libtsm`, `xterm.js`, `winpty` or
  `conpty.dll` is vendored.
- **No session restore across restart.** Only geometry, profiles and appearance
  are restored.
- **Encoding is a view setting.** The parser decodes bytes through a private
  `FXTerminalCharset` layer in front of the UTF-8 state machine. UTF-8 stays the
  default and the fast path. Single-byte encodings map each byte to a codepoint
  through the table of an `FXTextCodec` (`FX88591Codec`, `FX88592Codec`,
  `FXCP1252Codec`, `FXKOI8RCodec`) or a small built-in table for the codecs the
  library lacks (ISO-8859-15, Windows-1250, Windows-1251, CP437, CP850). Input
  (keys, paste, IME) goes through the inverse mapping; an unmappable character
  becomes `?` and the status line says so once. Multi-byte legacy encodings
  (Shift_JIS, EUC-JP, GBK, Big5) are out of scope for now. Changing the encoding
  never resets the screen model, so already drawn cells keep their codepoints.
- **Context menus are a view feature.** `FXTerminal` sends a context request to
  its target and the application builds the menu, so the library widget holds no
  menu policy.
- **Transparency is compositor-backed, not desktop capture.** The terminal never
  screenshots the desktop or wallpaper to fake transparency. The desktop shows
  through only where the window system supports alpha.
- **Three transparency modes.** `off` is the opaque baseline. `window` applies
  one alpha value to the whole shell, so text fades with it. `background`
  applies alpha only to terminal cell backgrounds, so text stays opaque.
- **Transparency is a global appearance setting.** It applies to every tab and
  every window and survives detach; profiles do not override it.
- **Capability first.** Wayland is the primary `background` backend. X11 and
  Windows must pass a capability check and a rendering gate before they
  advertise `background`; until then they advertise `window` only. A backend is
  never allowed to claim a mode it cannot draw correctly.

Explicit non-goals for this plan:

- Split panes and pane navigation.
- Sixel, Kitty graphics, iTerm2 image protocol and other inline graphics.
- SSH, serial ports, Telnet or a built-in multiplexer.
- Reattaching a tab to another process.
- Shell integration protocols beyond OSC 7 (working directory), OSC 0/1/2
  (title) and optional OSC 8 (hyperlink display).
- Mica, acrylic and blur as themed surfaces, rounded top-level windows and
  transparent chrome. On Windows, DWM blur-behind is allowed only as the
  mechanism for `background` alpha, not as a theme material. Terminal
  background transparency is in scope in section 8.4 and Phase 10/11.
- Pseudo-transparency by capturing the desktop, the root window or a wallpaper.
- Font ligatures and programming-font shaping. The terminal draws cell by cell.
- Screen-reader accessibility beyond standard keyboard focus and native window
  controls; FOX has no accessibility bridge.
- Restoring running sessions after the application exits.

## 3. What the tree already provides

Verified in `fox2/` at the time of writing:

- `FXConsole` (`include/FXConsole.h`, `lib/FXConsole.cpp`) provides a bounded
  logical-line buffer, per-byte styles, wrapping, selection, find and
  auto-scroll. It has no cursor addressing, no alternate screen, no escape
  parser and no PTY. Keep it for logs.
- `FXScrollArea` provides scrollbars, scrolling positions and mouse wheel
  handling. `FXConsole` already derives from it.
- `FXText` has keyboard, selection, clipboard and IME patterns that
  `FXTerminal` can copy without inheriting the editing model.
- `FXFont` provides `getFontHeight()`, `getFontAscent()`, `getTextWidth()` and
  scalable fixed-pitch faces. The theme provides `FONT_MONO`.
- `FXTheme` provides semantic colors and metrics through role and metric
  queries, and `FXWindow::applyTheme()` for live theme changes.
- `FXWindow::reparent()` can move a created child between created parents. Both
  windows must already exist and neither parent may be the root window. This is
  the mechanism for detaching a tab.
- `FXWindow::grab()`/`ungrab()` implement pointer capture on X11, Win32 and
  Wayland. The Wayland implementation records the grabbing surface.
- `FXApp::addInput()` watches a read handle. On X11/Wayland it takes a file
  descriptor; on Win32 it takes a waitable handle. The Win32 implementation is
  limited by `MAXIMUM_WAIT_OBJECTS` (64).
- `FXApp::addTimeout()` and `addDeadline()` provide timers.
- `FXComposeContext` and `SEL_IME_START` provide IME positioning and committed
  text. `FXText::onIMEStart()` is the worked example.
- `FXWindow` clipboard ownership and `FXText` clipboard handling provide a
  working copy/paste path.
- `FXString` stores UTF-8 and converts to/from wide characters.
- `fxunicode.h` provides character categories, combining classes and
  decomposition. It does **not** provide terminal cell width, so the terminal
  needs its own East Asian Width/emoji width table.
- `FXProcess` can start a child with pipes, but it has no API for attaching a
  PTY or for `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE`. Do not try to force ConPTY
  through `FXProcess::setInputStream()`.
- There is no PTY, ANSI parser or terminal screen anywhere in `fox2/`.

## 4. Architecture

```
 keyboard / IME / mouse / paste
          |
          v
   FXTerminal (view)  ---- input bytes ---->  FXPty  ---->  child process
          ^                                      |
          |                                      | output bytes
          |                                      v
   FXTerminalScreen  <--- parser dispatch ---  FXTerminalParser
          |
          +---- title / cwd / bell / clipboard / exit notifications
                              |
                              v
                    TerminalWindow / TerminalTabBar
```

- All screen and widget state is manipulated on the UI thread.
- PTY output may be produced on a helper thread on Windows. That thread only
  appends raw bytes to a bounded queue and signals a wake handle; parsing and
  screen mutation happen on the UI thread.
- Input bytes travel from `FXTerminal` to `FXPty`. The parser never writes to
  the child directly; replies such as cursor-position reports go through the
  same input queue.
- The tab bar and window controller never inspect terminal cells. They read
  title, working directory, running state and bell state through `FXTerminal`.

### 4.1 Files

Library:

| File | Contents |
| --- | --- |
| `fox2/include/FXTerminal.h` | Public `FXTerminal` widget, `FXTerminalKind`, launch description and notification selectors. |
| `fox2/lib/FXTerminal.cpp` | Layout, painting, scrolling, selection, search, input encoding, clipboard and PTY bridge. |
| `fox2/lib/fxterminalscreen.h/.cpp` | Private screen model: cells, rows, scrollback, cursor, modes, alternate screen, dirty state and reflow. |
| `fox2/lib/fxterminalparser.h/.cpp` | Private incremental UTF-8 and escape-sequence parser; dispatches to the screen. |
| `fox2/lib/fxterminalwidth.h/.cpp` | Private Unicode cell-width table and lookup. |
| `fox2/lib/fxpty.h` | Private neutral PTY interface. No OS types in this header. |
| `fox2/lib/fxpty_posix.cpp` | POSIX PTY, fork and process group implementation. |
| `fox2/lib/fxpty_win32.cpp` | ConPTY, process creation and reader/writer thread implementation. |
| `fox2/include/FXWindow.h` | Public whole-window opacity API for `window` mode. |
| `fox2/lib/FXWindow_x11.cpp`, `FXWindow_win32.cpp`, `FXWindow_wayland.cpp` | Platform implementations of whole-window opacity; X11 property, Win32 layered alpha, Wayland premultiplied surface alpha. |
| `fox2/lib/fxwayland.cpp` | Per-surface opacity applied to the finished ARGB frame. |
| `fox2/lib/fxraster.h`, `FXRaster.cpp` | Alpha-preserving fill, blending, image blit; current opaque behavior stays byte-identical. |
| `fox2/lib/FXVisual_x11.cpp` | ARGB visual capability for X11 `background` mode. |

Application:

| File | Contents |
| --- | --- |
| `fox2/apps/foxterm/main.cpp` | Version check, `FXApp`, command line, first window, event loop. |
| `fox2/apps/foxterm/TerminalWindow.h/.cpp` | Main window, tab ownership, switcher, menus, actions, close confirmation and detach. |
| `fox2/apps/foxterm/TerminalTabBar.h/.cpp` | Custom tab strip: paint, hit testing, close buttons, plus/drop-down, overflow, drag and reorder. |
| `fox2/apps/foxterm/TerminalTab.h/.cpp` | Tab state: profile, title, view, running/exit state, bell. No widgets are owned here beyond the view. |
| `fox2/apps/foxterm/Profiles.h/.cpp` | Profile and color-scheme model, defaults, discovery, registry load/save. |
| `fox2/apps/foxterm/PreferencesDialog.h/.cpp` | Minimal settings dialog for profiles and appearance. |
| `fox2/apps/foxterm/CMakeLists.txt` | CMake target. |
| `fox2/apps/foxterm/Makefile.am` | Autotools target. |
| `fox2/apps/foxterm/foxterm.1` | Manual page. |

Tests and documentation:

| File | Contents |
| --- | --- |
| `fox2/tests/terminalmodel.cpp` | Display-free model and parser tests. |
| `fox2/tests/pty.cpp` | PTY smoke test that starts the test binary itself as the child. |
| `docs/terminal.md` | User guide: keys, profiles, settings, limitations. |
| `screenshots/tests/fox2/x11/foxterm-tabs.png` etc. | Acceptance screenshots. |

### 4.2 Public widget API

`FXTerminal.h` defines:

```cpp
enum FXTerminalKind {
  TERMINAL_VT100,
  TERMINAL_XTERM
  };

enum FXTerminalTransparency {
  TRANSPARENCY_OFF,          // opaque baseline
  TRANSPARENCY_WINDOW,       // whole shell alpha
  TRANSPARENCY_BACKGROUND    // cell backgrounds alpha, text opaque
  };

struct FXTerminalLaunch {
  FXString shell;         // executable path
  FXString arguments;     // platform-quoted argument string, may be empty
  FXString directory;     // starting directory, may be empty
  FXString term;          // "vt100" or "xterm-256color"
  FXint    kind;          // TERMINAL_VT100 or TERMINAL_XTERM
  FXint    columns;       // initial columns, 0 means derive from the widget
  FXint    rows;          // initial rows, 0 means derive from the widget
  };
```

`FXTerminal` derives from `FXScrollArea` like `FXConsole`. If the launch
columns or rows are 0, start with 80x24 until `layout()` can derive the real
size from the visible area and font metrics. Its API must include:

- `start(const FXTerminalLaunch&)`, `terminate()`, `isRunning()`,
  `getExitCode()`.
- `send(const FXString& bytes)` for raw input, and `sendText()` for committed
  text. Both append to a bounded input queue.
- `feed(const FXString& bytes)` for tests and replay. It parses input as if it
  came from the child and never starts a process.
- `getTitle()`, `getWorkingDirectory()`, `getTermKind()`, `getTerm()`.
- `getColumns()`, `getRows()`, `getCursorRow()`, `getCursorColumn()`,
  `getScrollbackLines()`.
- `setScrollbackLines()`, `setColorScheme()`, `setFont()`, `setCursorStyle()`,
  `setFollowOutput()`.
- `setTransparencyMode(FXint mode)`, `getTransparencyMode()`,
  `setBackgroundOpacity(FXint percent)`, `getBackgroundOpacity()`.
- `getSupportedTransparencyModes()` returning a bit mask of the three modes.
  The mask is queried after the widget is created and may change when the
  window is moved to a different compositor or display.
- `setEncoding(const FXString& name)`, `getEncoding()`, the static
  `getSupportedEncodingCount()`, `getSupportedEncodingName(index)` and
  `findEncoding(name)` for the run-time character encoding (R20). `setEncoding`
  returns the canonical name that is in effect, so the caller learns about a
  fallback without a notification. The list is static because the context menu
  is built before the first session exists.
- `search()`, `findNext()`, `findPrev()`, `clearSearch()`.
- `selectAll()`, `copySelection()`, `pasteClipboard()`.
- `getSelectedText()`.
- `applyTheme(FXTheme*)` so a live theme change updates chrome and fallback
  colors.

Notification selectors from the widget to its target:

| Selector | Meaning |
| --- | --- |
| `ID_TITLE` | The child changed the title or working directory. The target calls `getTitle()`/`getWorkingDirectory()`. |
| `ID_BELL` | BEL was received. The tab bar flashes the tab; the application may also call the system bell. |
| `ID_EXITED` | The child exited. The target calls `getExitCode()` and applies the profile's close-on-exit policy. |
| `ID_CLIPBOARD` | The child requested a clipboard read or write through OSC 52. The application decides whether to allow it. |
| `ID_TERMINAL_SEARCH_CHANGED` | Search result count changed; optional for the search bar. |
| `ID_CONTEXT` | The user right-clicked, or pressed the menu key or Shift+F10. The data pointer holds the position and whether a selection exists; the target pops up the menu (R19). |
| `ID_ENCODING_LOSS` | A key or paste was sent as '?' because the session encoding cannot express it. Sent as `SEL_CHANGED`, not `SEL_COMMAND`, so it cannot collide with the ids the window uses for its own menu commands. The effective encoding itself comes from the return value of `setEncoding()`. |
| `ID_TRANSPARENCY` | The effective transparency mode changed, for example after a capability query or a move to another compositor. The target calls `getTransparencyMode()` and the supported-mask getter. |

`FXWindow` gains `setOpacity(FXint percent)`, `getOpacity()` and
`isOpacitySupported()` for `window` mode. `percent` is clamped to 0..100; 100
means fully opaque and removes the platform opacity state. The call is a no-op
on a backend that cannot provide it and on non-shell windows.
`FXTerminal` uses this API for `window` mode and uses its own alpha surface
path for `background` mode.

`FXTerminal` must not call `exit()`, display a message box, open a link or change
the clipboard by itself. It reports and the application decides.

## 5. Screen model

`FXTerminalScreen` is display-independent. It must be constructible without
`FXApp`, so `terminalmodel` runs as a console test.

### 5.1 Cell

Use one fixed-size cell:

```cpp
struct FXTerminalCell {
  FXuint fg;         // 0..255 palette index, or 0x01000000|0xRRGGBB
  FXuint bg;         // same encoding
  FXuint ch;         // base Unicode codepoint, 0 for continuation/empty
  FXushort attr;     // bold, dim, italic, underline, blink, reverse, hidden,
                     // strike, overline, underline style
  FXushort width;    // 0 continuation, 1 narrow, 2 wide
  FXuint cluster;    // 0 for a simple cell, else index into the grapheme pool
  };
```

- Colors use palette indices while possible. RGB colors are stored with the
  0x01000000 marker bit. This allows a color-scheme change to recolor existing
  text.
- The grapheme pool holds `FXString` values for combining sequences, ZWJ
  sequences and unsupported clusters. Simple Latin, CJK and box-drawing cells
  never touch the pool. The pool is bounded per screen (default 4096 entries);
  when it is full, a cluster is replaced by its base codepoint.
- A wide character occupies the first cell with `width=2`; the next cell has
  `width=0` and is skipped by painting and text extraction.
- Cell width comes from a checked-in range table based on Unicode 15.1 East
  Asian Width and emoji presentation data. `fxunicode.h` categories are used
  for combining marks, but `FXUnicode` has no terminal width function. A
  developer-only generator script produces the sorted ranges; the build does
  not run the script and has no Unicode data-file dependency.
- Width rules: combining marks, variation selectors and format controls are
  zero-width; East Asian Wide and Fullwidth characters and default emoji
  presentation are two cells; East Asian Ambiguous characters are one cell.
  A ZWJ emoji sequence is stored as one two-cell cluster when the pool has
  room, otherwise its first base character is used.
- The default scrollback is 10000 lines. A profile can set 0 to disable
  scrollback or up to 100000 lines. The screen also enforces a byte budget
  (default 64 MiB); when the budget is reached it drops the oldest scrollback
  rows. Alternate-screen rows never enter scrollback.

### 5.2 Rows and buffers

- Primary buffer: `rows` on screen plus a ring of scrollback rows.
- Alternate buffer: exactly `rows` on screen, no scrollback.
- Each row has a `wrapped` flag (soft wrap) and a dirty generation.
- Cursor state: row, column, saved row/column, origin mode, wrap-pending,
  visible, style and blink phase.
- Screen state: current rendition, scroll top/bottom margins, tab stops, active
  charsets (G0/G1), DEC line-drawing set, insert mode, auto-wrap, application
  cursor keys, application keypad, bracketed paste, focus reporting, mouse
  mode/encoding and the xterm window title.
- Save/restore cursor and the 1049 alternate-screen switch preserve and restore
  the primary screen, cursor, rendition and margins.

### 5.3 Scrolling and reflow

- Scrolling inside margins moves only the affected rows.
- Full-screen scroll moves the top primary row into scrollback and appends a
  blank row. If scrollback is disabled, the row is discarded.
- Resize first adjusts margins, cursor and the alternate buffer. In the primary
  buffer, rows marked `wrapped` are reflowed to the new width; hard-wrapped rows
  are not joined. Shrinking pushes the excess top rows into scrollback. Growing
  does not pull rows back out of scrollback.
- Resize is called from `FXTerminal::layout()` with the new column/row count.
  It calls `FXPty::resize()` after the model is consistent.

### 5.4 Memory and work limits

- Maximum parser parameter count: 32. Extra parameters are ignored.
- Maximum OSC/DCS string: 8 KiB. A longer string is consumed and discarded.
- Maximum title: 1 KiB after sanitizing control characters.
- Maximum hyperlink URI: 4 KiB.
- Maximum paste/input queue: 1 MiB. When full, further writes are refused and
  the bell is rung; never block the UI thread.
- Maximum unparsed output queue: 8 MiB. When full, reading from the PTY pauses
  (backpressure) until the UI catches up; output is not silently dropped.
- Parse budget per UI turn: 256 KiB. Remaining bytes stay queued and are parsed
  on the next chore/tick.
- Scrolling and painting operate on the visible rows plus a small overscan, not
  on the full scrollback.

## 6. Parser and emulation profiles

### 6.1 Parser

`FXTerminalParser` is an incremental byte state machine. It handles:

- UTF-8 decoding with replacement of malformed sequences by U+FFFD.
- C0 controls: BEL, BS, HT, LF, VT, FF, CR, SO, SI, CAN, SUB, ESC.
- ESC sequences: save/restore cursor, index, next line, reverse index, tab
  set, reset, line-drawing charset selection, keypad modes, 7/8-bit controls,
  string terminator and soft reset.
- CSI sequences: cursor movement, next/previous line, horizontal/vertical
  position, erase display/line/character, insert/delete line/character,
  scroll up/down, set/reset margins, tab clear, cursor style, device status,
  device attributes, window operations that can be answered safely, and
  private mode set/reset.
- DEC private modes used by VT100 and xterm: cursor keys, origin, auto-wrap,
  cursor blink, cursor visible, alternate screen (47/1047/1049), mouse
  tracking (1000/1002/1003), SGR mouse (1006), bracketed paste (2004), focus
  reporting (1004). Unknown private modes are recorded as ignored and never
  change cursor behavior.
- SGR: reset, bold, dim, italic, underline (single and styled), blink, reverse,
  hidden, strike, overline, 8/16 colors, 256-color `38;5`/`48;5` and truecolor
  `38;2`/`48;2`. Colon and semicolon parameter forms are both accepted.
- OSC: 0/1/2 title, 7 working directory, 8 hyperlink, and 52 clipboard. OSC 4
  palette changes are accepted only for palette indices 0..255 and only when
  the profile permits them.
- DCS, PM and APC strings are consumed and discarded with the same length limit.
  No DCS command executes anything.

The parser also produces replies for the child:

- Device attributes (`DA`), cursor position report (`CPR`), device status
  report (`DSR`) and terminal identity replies. Replies go into a small output
  queue consumed by `FXTerminal` and written to the PTY.
- Replies must respect the active profile. A `vt100` session must not announce
  xterm-only capabilities.

### 6.2 `xterm` profile

Advertised environment:

```
TERM=xterm-256color
COLORTERM=truecolor
TERM_PROGRAM=foxterm
TERM_PROGRAM_VERSION=<version>
```

Behavior:

- 16 ANSI colors, 256-color palette and 24-bit color.
- Alternate screen, bracketed paste, focus events, mouse reporting (SGR
  preferred; X10 and normal tracking accepted), application cursor keys and
  keypad.
- OSC 0/1/2 title, OSC 7 working directory, OSC 8 hyperlinks (displayed and
  copyable; this plan does not open URLs automatically).
- OSC 52 clipboard is disabled by default. If enabled in settings, the
  application confirms writes above 32 KiB and never returns clipboard content
  to the child unless the user explicitly enables clipboard reads.
- Cursor style changes through DECSCUSR are honored.

### 6.3 `vt100` profile

Advertised environment:

```
TERM=vt100
```

No `COLORTERM`. `TERM_PROGRAM` may still be set; it is informational.

Behavior:

- 80x24 default when no size is available; resize is still allowed.
- Standard VT100 cursor addressing, margins, insert/delete, erase, tab stops,
  save/restore, alternate screen (VT100 has no alternate screen, but the
  emulation may keep the 47/1047/1049 behavior; it must not advertise it),
  application cursor keys and keypad.
- 8 colors and the basic SGR attributes. 256-color and truecolor SGR are
  parsed but reduced to the nearest of the 8 basic colors.
- xterm-only private modes (mouse, bracketed paste, focus, OSC 8, OSC 52, OSC 4)
  are ignored. Mouse reporting stays off.
- OSC 0/1/2 title and OSC 7 are ignored so the application cannot change the
  window title or working directory in VT100 mode. This is deliberate: a
  VT100 has neither.
- DA replies identify a VT100 (`ESC[?1;0c`).

A profile's `kind` and `term` string must agree. Loading a profile with
`kind=TERMINAL_VT100` and `term=xterm-256color` is a configuration error and
falls back to the VT100 defaults.

### 6.4 Unknown and malformed input

- Unknown CSI/ESC sequences are consumed without printing their parameters.
- A malformed sequence is abandoned at the next C0 control or final byte.
- Invalid UTF-8 never aborts the parser.
- No sequence may allocate more than the limits in 5.4.
- The parser never calls back into the application from inside a screen update;
  requests are queued.

## 7. PTY seam

### 7.1 Neutral interface

`fxpty.h` exposes private types only. It must not include `windows.h`,
`unistd.h` or any platform typedef outside the implementation file.

```cpp
class FXPty {
public:
  FXPty();
  ~FXPty();

  FXbool start(const FXTerminalLaunch& launch,FXString& error);
  FXbool isRunning() const;
  FXint  getExitCode() const;

  // UI-thread notification handle. On POSIX this is the master fd.
  // On Windows this is a manual-reset event set by the reader thread.
  FXInputHandle wakeHandle() const;
  void clearWake();

  // Non-blocking. Returns bytes moved, 0 if no data is ready, -1 on error.
  FXival read(FXchar* buffer,FXival size);
  FXival write(const FXchar* buffer,FXival size);

  // Queue writes that could not be completed immediately.
  FXbool flushWrites();
  FXbool hasPendingWrites() const;

  void resize(FXint columns,FXint rows);
  void terminate(FXbool force=false);
  void close();
  };
```

`FXPty` does not own a `FXApp`, a `FXTerminal` or a window. The widget owns the
PTY and registers `wakeHandle()` with `FXApp::addInput(this,ID_READ,...)`.
`clearWake()` is called after the queue is drained. On POSIX, no wake handle is
needed beyond the master fd; `read()` returns `EAGAIN` as 0 and EOF as -1.
When `hasPendingWrites()` is true, the widget also registers `INPUT_WRITE` on
the same POSIX fd and calls `flushWrites()` from that callback. On Windows the
writer thread makes this unnecessary, but the interface stays the same.

### 7.2 POSIX implementation

- `posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK)`, `grantpt()`, `unlockpt()`,
  `ptsname()`.
- `ioctl(TIOCSWINSZ)` with the initial rows/columns before the child starts.
- `fork()`: child calls `setsid()`, opens the slave, `ioctl(TIOCSCTTY,0)`,
  duplicates the slave onto fd 0, 1 and 2, closes the master and slave copies,
  sets the environment and calls `execvp()`. An `exec` failure writes a short
  message to stderr and exits 127.
- Parent closes the slave and keeps the master non-blocking. Treat `EIO`
  and `EPIPE` as session end; do not install a global `SIGPIPE` handler from
  the library.
- Environment: inherit the parent, then set `TERM`, `COLORTERM` (xterm only),
  `TERM_PROGRAM=foxterm`, `TERM_PROGRAM_VERSION`, `COLUMNS`, `LINES` and
  `PWD`. Do not override the user's locale.
- Starting directory: `chdir()` in the child before `exec`; on failure the
  child reports the error and exits 126.
- Resize: `TIOCSWINSZ` on the master. A failed resize is logged; it must not
  kill the session.
- Exit: when the master read returns 0 or `EIO`, call `waitpid(WNOHANG)` until
  the child is reaped and remember its exit code. A child that dies without a
  readable PTY is also reaped by the `SIGCHLD` path if one is installed by the
  application.
- Terminate: send `SIGHUP` to the child's process group (`kill(-pgid,SIGHUP)`),
  after a short grace period send `SIGTERM`, then `SIGKILL`. Never block the UI
  thread waiting for the child.
- `posix_openpt` is the preferred path. Configure checks it; if it is absent,
  use `openpty()` and link `-lutil`. The platform file contains the fallback
  condition; the public header does not.
- macOS uses the same POSIX file. The application is built against X11/XQuartz
  because there is no native macOS backend in this tree.

### 7.3 Windows implementation

ConPTY requires Windows 10 1809 or newer.

- Resolve `CreatePseudoConsole`, `ResizePseudoConsole` and
  `ClosePseudoConsole` with `GetProcAddress(GetModuleHandleW(L"kernel32.dll"),...)`.
  Also resolve the `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE` constant if the SDK
  headers are older than the runtime.
- Create the pseudo console with the initial size, input pipe and output pipe.
- Build a `STARTUPINFOEXW`; set the pseudo-console attribute; call
  `CreateProcessW` with `EXTENDED_STARTUPINFO_PRESENT`, `CREATE_UNICODE_ENVIRONMENT`
  and a job object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`.
- Close the child-side pipe handles after `CreateProcessW`. The parent keeps
  the pseudo console handle, the input write pipe and the output read pipe.
- Use a reader thread blocked on `ReadFile` of the output pipe. It appends to a
  bounded mutex-protected byte queue and sets the manual-reset wake event.
  On EOF/error it marks the session ended and sets the event. On shutdown,
  close the output pipe and, if the reader has not stopped within 500 ms, call
  `CancelSynchronousIo()` on the reader thread before closing the remaining
  handles.
- Use a writer thread blocked on the input queue and `WriteFile` of the input
  pipe. `FXPty::write()` only enqueues. This avoids blocking the UI on a full
  ConPTY input buffer.
- `read()` drains the output queue on the UI thread. `clearWake()` resets the
  event only when the queue is empty, so a race cannot lose output.
- Resize calls `ResizePseudoConsole`.
- Terminate: close the pseudo console, terminate the job, wait for the reader
  and writer threads, then close the process and thread handles after the
  threads stop.
- The output pipe is UTF-8. Do not run `chcp`; ConPTY already decodes the
  console API output. Set `TERM`, `COLORTERM`, `TERM_PROGRAM`, `COLUMNS` and
  `LINES` in the child environment.
- Default shell: `%COMSPEC%` (normally `cmd.exe`). Discover PowerShell only at
  the standard `%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe`
  and `%ProgramFiles%\PowerShell\7\pwsh.exe` paths; do not search `PATH` for a
  shell at startup.
- Command line quoting follows the `CreateProcessW` rules already used by
  `FXProcess_win32.cpp`. The profile stores one argument string.
- If ConPTY is unavailable, `start()` returns false with a readable error and
  the application offers to open the profile settings. Do not fall back to
  anonymous pipes, because that loses terminal semantics and signals.

### 7.4 Wake-handle limit

On Windows, one wait handle per running tab consumes an `FXApp::addInput()`
slot. The application must keep at most 32 running sessions per process. If
`addInput()` fails, the window switches to a single 16 ms timer that polls all
its running sessions' `read()` calls; output is not dropped. On POSIX the
master fd is used directly and the normal fd limit applies.

## 8. Rendering

### 8.1 Font and cell metrics

- Use `getApp()->getRoleFont(FONT_MONO)` by default. A profile may override
  family, size and weight; build that font from an `FXFontDesc` initialised
  from `theme->font(FONT_MONO)`.
- Cell width is the advance of `M` in the normal font. For a true fixed-pitch
  font the width is constant; if the face is not fixed-pitch, log a warning and
  use the maximum of `M`, `W` and `0`.
- Cell height is `font->getFontHeight()` plus a leading of
  `theme->metric(METRIC_PADDING)/2`, minimum 1 pixel. Baseline is
  `font->getFontAscent()`.
- Create normal, bold, italic and bold-italic `FXFont` variants from the same
  `FXFontDesc`. Recreate them on DPI, theme or profile font change; update
  cell metrics after realization.
- Use `theme->metric(METRIC_BORDER_WIDTH)` for the frame and
  `METRIC_PADDING` for internal padding.
- Clear the full terminal background before drawing, then fill each run's
  background. Use the opaque path in `off` mode and the alpha paths in
  `window`/`background` mode described in 8.4. Do not rely on the window
  background matching the terminal background.
- Draw text in runs that share foreground, background and attributes. Draw
  underline, overline, strike-through and cursor with `FXDC::drawLine()` or
  `fillRectangle()`.
- The default palette is Windows Terminal's Campbell scheme: background
  `#0C0C0C`, foreground `#CCCCCC`, cursor `#FFFFFF`, selection background
  `#264F78`, selection foreground `#FFFFFF`. The 16 ANSI colors are:

  ```
  black          #0C0C0C    bright black   #767676
  red            #C50F1F    bright red     #E74856
  green          #13A10E    bright green   #16C60C
  yellow         #C19C00    bright yellow  #F9F1A5
  blue           #0037DA    bright blue    #3B78FF
  magenta        #881798    bright magenta #B4009E
  cyan           #3A96DD    bright cyan    #61D6D6
  white          #CCCCCC    bright white   #F2F2F2
  ```

  Built-in alternatives are `One Half Dark`, `Solarized Dark`,
  `Solarized Light` and `Tango`.

### 8.2 Painting and scrolling

- `FXTerminal` derives from `FXScrollArea`. The vertical scrollbar represents
  scrollback plus the visible rows.
- The scroll position is anchored at the bottom when `followOutput` is true.
  New output follows only while the view is at the bottom. If the user scrolls
  up, the position stays fixed until the user returns to the bottom, types, or
  chooses "Scroll to bottom".
- Alternate-screen sessions disable scrollback scrolling; the scrollbar is
  hidden or disabled while the alternate buffer is active.
- Paint only the visible rows plus one row of overscan. Track dirty rows and
  coalesce updates into one `refresh()` per output batch or timer tick.
- Use a back buffer (`FXImage` the size of the visible area) for repaint, as
  `FXTabItem` does for its paint buffer, so cursor blink and selection do not
  flicker. Rebuild the back buffer on resize, font change, theme change and
  full scroll; otherwise update the affected rows.
- The cursor is drawn on top of the cell: block (filled or hollow), bar or
  underline, with a blink timer driven by `FXApp::addTimeout()`. The cursor is
  hidden while the widget is not focused if the profile asks for that.
- Selection uses linear coordinates across rows, including scrollback. Wide
  cells are selected as a unit. Double-click selects a word, triple-click
  selects a line. Dragging beyond the viewport auto-scrolls.
- Search highlights are drawn as an overlay background; the current match has
  the accent color. Search covers scrollback and the primary screen.

### 8.3 Theme changes

- `applyTheme()` re-reads chrome colors, metrics and the mono font, rebuilds
  the font variants and refreshes the tab bar and all views.
- Terminal palette colors are independent of the UI theme. A theme change must
  not recolor the terminal content unless the profile explicitly maps the
  scheme to the UI theme.
- Do not add new `FXTheme` roles for the tab strip. Use `ROLE_WINDOW_BG`,
  `ROLE_SURFACE`, `ROLE_TEXT`, `ROLE_TEXT_DISABLED`, `ROLE_BORDER`,
  `ROLE_SELECTION_BG`, `ROLE_SELECTION_TEXT`, `ROLE_ACCENT` and
  `ROLE_FOCUS_RING` with the normal/hover/active/selected states.

### 8.4 Background transparency

#### Settings

```
transparencyMode   off | window | background   default: off
backgroundOpacity  0..100                      default: 100
```

In `background` mode the effective cell alpha is the product of
`backgroundOpacity` and the color scheme's background alpha
(`FXALPHAVAL(scheme.background)`). In `window` mode only `backgroundOpacity` is
used; the scheme alpha still affects the cell paint inside the opaque render.
A scheme with an opaque background and an opacity of 100 is the existing opaque
render. When `transparencyMode=off`, the opacity value is retained but not
applied.

`window` mode applies one alpha value to the whole terminal shell, including
the tab strip and terminal text. Menus and tooltips are separate shells and
stay opaque. It is the portable baseline. `background` mode applies alpha only
to the terminal grid and its margins; glyphs, cursor, selection foregrounds, the
tab strip, menus and the status line stay opaque.

The setting is global, not per profile. A detached tab keeps the current mode
and opacity. Changing it repaints all open terminal views and updates every
window's opacity immediately.

#### Rendering

`background` mode needs an alpha-aware drawing path; changing a color to an
ARGB value is not enough. The existing `FXRaster::fillRectangle()` forces
`0xff000000`, and glyph antialiasing blends against an opaque destination. The
work is:

1. Add an alpha-replacing rectangle fill:
   `FXRaster::clearRectangleAlpha(x,y,w,h,alpha)`. It writes premultiplied
   pixels, not a source-over blend over the parent background.
2. Make `FXRaster::blend()` and `blendPixel()` source-over correct for a
   transparent destination:
   - destination alpha 255: byte-identical to the current opaque result;
   - destination alpha 0: write the source color premultiplied by coverage and
     set alpha to coverage;
   - intermediate alpha: standard premultiplied source-over.
3. Add an alpha-preserving image blit so the terminal back buffer can be copied
   to the window without turning transparent background pixels opaque.
4. In `background` mode, clear the terminal viewport to the effective alpha,
   fill cell backgrounds with the same alpha (premultiplied), then draw glyphs,
   cursor and selection with the opaque paths.
5. In `window` mode, keep the terminal rendering opaque and apply the alpha to
   the completed shell surface. Do not also alpha-fill the cells; that would
   multiply the effect.

`FXTerminal` continues to use a back buffer for flicker-free painting. In
`background` mode that buffer must be a premultiplied ARGB image; in `off` and
`window` modes it may stay opaque. The terminal paints the back buffer over the
parent, so an alpha-aware blit is required.

Selection background is derived from the selection color with the effective
alpha; selection text stays opaque. The current search match is drawn with the
accent color and the same alpha rule. The cursor is always opaque, or fully
hidden when the profile hides it. The terminal's inner margins follow the cell
background alpha.

#### Platform matrix

| Backend | `window` | `background` | Notes |
| --- | --- | --- | --- |
| Wayland | Required | Required | Surface already is premultiplied `ARGB8888`; complete the alpha raster path and commit. |
| X11 | Required when a compositing manager is present | Capability-gated | `_NET_WM_WINDOW_OPACITY` for `window`; 32-bit ARGB visual plus XRender alpha fills for `background`. Without either, downgrade. |
| Win32 | Required with DWM composition | Capability-gated | `WS_EX_LAYERED` + `LWA_ALPHA` for `window`; `background` needs DWM composition and an unpainted terminal surface. If the GDI child-window path cannot keep per-pixel alpha, advertise `window` only. |
| macOS/X11 | Same as X11 | Same as X11 | There is no native Cocoa backend. |

Whole-window opacity does not claim to be per-pixel. On X11, the property is
honored only by a compositing manager; on Win32, DWM must be running. A backend
that cannot honor the request returns `isOpacitySupported()==false` and the
settings dialog shows the requested and effective mode.

`background` mode is not enabled by setting a color alpha alone. The backend
must pass a narrow capability gate:

- Wayland: the compositor accepts an `ARGB8888` buffer and the frame dump with
  `FOX_WAYLAND_DUMP_ALPHA` shows the expected alpha. Wayland qualifies by
  construction once the alpha raster path is in place.
- X11: the shell has a 32-bit TrueColor visual with an alpha mask and XRender is
  present. If the visual cannot be used for a top-level shell without changing
  the root visual or breaking existing widgets, `background` stays disabled and
  `window` is the supported mode.
- Win32: DWM composition is enabled and a test window can show a translucent
  client background with opaque child text. The candidate path is
  `DwmEnableBlurBehindWindow()` or `SetWindowCompositionAttribute()` with a
  transparent client area and an unpainted terminal background; both are
  resolved dynamically so an older system can refuse them. If the GDI
  child-window path cannot preserve that, `background` stays disabled.
  Color-key transparency and screenshot-based pseudo-transparency are not
  acceptable substitutes.

#### Interaction and performance

- The tab strip and menus are opaque in `background` mode. They remain readable
  even when the desktop behind is busy.
- Detaching a tab does not renegotiate transparency. The new window receives
  the same mode and opacity and is repainted once.
- Changing opacity is a one-time full repaint of the affected terminal views.
  `background` mode adds no per-frame full-surface pass; only the normal cell
  fills and glyph draws use alpha.
- On Wayland, `window` mode applies the opacity to the finished shell once per
  committed frame, as a premultiplied scale pass. On X11 and Win32 it sets a
  window property/attribute once and the compositor applies it; there is no
  per-frame client pass. Keep both paths out of the per-cell path.
- `off` mode must stay byte-identical to the current opaque render for the
  existing pixel gates.

#### Fallback

When the requested mode is not supported:

1. Try `background`; if unsupported, try `window`.
2. If `window` is unsupported, use `off`.
3. Keep the requested value in the registry, store the effective value in the
   settings dialog, and show one non-modal notice per session, not one per tab.
4. Never leave a window half-transparent, never change the visual on an
   existing shell without recreating it, and never enable a mode that would
   make text unreadable.

## 9. Input, selection and clipboard

### 9.1 Keyboard encoding

`FXTerminal::onKeyPress()` handles only events that should reach the child.
The application handles its own shortcuts first.

- Printable text comes from `event->text` (UTF-8). Insert it unchanged.
- Ctrl combinations map to ASCII control bytes: Ctrl+A..Z to 0x01..0x1A,
  Ctrl+Space to NUL, Ctrl+[ to ESC, Ctrl+\ to FS, Ctrl+] to GS, Ctrl+^ to RS,
  Ctrl+_ to US, Ctrl+? to DEL. Ctrl+Shift+letter uses the same control byte
  unless the application owns the shortcut.
- Alt is an ESC prefix when `altSendsEscape` is true (default), except while
  AltGr produces text. When both Ctrl and Alt are present and the event has
  text, prefer the text and do not add a second ESC.
- Cursor keys: `ESC[A/B/C/D` normally, `ESCOA/B/C/D` when DECCKM is set.
  Home/End follow the same mode.
- Function keys: VT100 F1..F4; xterm F1..F12 with modifier parameter
  `1;<mod><letter>`. Modifier value is `1 + shift + 2*alt + 4*ctrl`.
- Insert, Delete, PageUp, PageDown, keypad keys and application keypad mode
  follow the xterm table. VT100 mode uses the VT100 subset.
- Shift+PageUp/PageDown scroll the scrollback instead of sending keys.
- Typing any ordinary key returns the view to the bottom and re-enables
  follow-output.
- Unhandled keys are returned as 0 so the parent can use them for focus
  navigation.

### 9.2 Mouse

- When mouse reporting is off, mouse press/drag selects text and the wheel
  scrolls. Shift+wheel always scrolls even if the child requested mouse input.
- When mode 1000, 1002 or 1003 is enabled, send button press/release/motion
  reports. Prefer SGR encoding when 1006 is enabled; otherwise use X10/normal
  encoding.
- Report coordinates relative to the screen, 1-based, clamped to the grid.
  Do not report scrollback coordinates.
- Shift+click forces local selection even while mouse reporting is active.
- Wheel events send mouse wheel buttons when mouse reporting is active,
  otherwise they scroll the scrollback.

### 9.3 Clipboard and paste

- Selection is copied with Ctrl+Shift+C, the context menu, or the platform copy
  shortcut when the terminal has focus and `copyOnSelect` is off. Middle-click
  paste uses the primary selection on X11 where available.
- Ctrl+Shift+V pastes. If the child enabled bracketed paste (2004), wrap the
  text in `ESC[200~` and `ESC[201~`. Otherwise send the text unchanged except
  that `\n` becomes `\r`.
- Multi-line paste is refused with a confirmation dialog when
  `warnOnMultilinePaste` is true (default), because a paste can execute
  commands. The dialog offers Paste and Cancel. Do not strip or rewrite the
  user's text.
- OSC 52 is disabled by default. If enabled, writes are confirmed above
  32 KiB and reads never return data unless the user turns on clipboard reads.
- `FXTerminal` owns the clipboard acquisition; the application owns the
  confirmation dialogs.
- Right-click in the terminal opens a context menu: Copy, Paste, Select all,
  Find, Copy link (when the cell under the pointer has an OSC 8 link), Scroll
  to bottom, Close tab and Detach tab. Shift+right-click opens it while mouse
  reporting is active.

### 9.4 IME

- On focus, create an `FXComposeContext` for the terminal window with the
  terminal as the target, as `FXText` does.
- Handle `SEL_IME_START` by positioning the composition spot at the current
  cursor cell, converted to root coordinates.
- Commit text through the same `sendText()` path as typed text. Do not send
  composition previews to the child.
- If the IME commits while a selection exists, clear the selection first.
- Test on X11 with `fcitx`/`ibus` and on Windows with the Microsoft IME; on
  Wayland follow the existing `zwp_text_input_v3` path used by `FXTextField`.

## 10. Tabs, windows and detachment

### 10.1 Window layout

```
FXMainWindow (native title bar)
  FXVerticalFrame
    TerminalTabBar       fixed height, fill X
    FXSwitcher           fill X/Y, one FXTerminal child per tab
    FXLabel              optional exit/status line, hidden normally
```

- `TerminalTabBar` is a custom `FXPacker`. It paints tabs, close buttons, the
  plus button and the drop-down chevron, and hit-tests them itself.
- Do not use `FXTabBook`: its tabs cannot host close buttons or be moved out of
  the bar.
- The tab strip is one row. Active tab has a surface color, a one-pixel accent
  line on the active edge and a stronger text color. Inactive tabs are muted.
  The close button appears on hover or on the active tab.
- The plus button opens the default profile. The adjacent chevron opens a menu
  of profiles, open tabs and "New window".
- At the far right, a menu button opens the application menu: New tab, New
  window, Copy, Paste, Find, Settings, About and Quit. It is the only menu bar
  in the window; the terminal area keeps the full height.
- Overflow: when tabs do not fit, show left/right scroll buttons at the ends of
  the tab region and scroll the tab strip. The mouse wheel over the strip also
  scrolls it. The active tab is always scrolled into view.
- Tab width has a minimum and maximum based on the title; titles are elided in
  the middle with `...`. Tooltips show the full title, profile and working
  directory.

### 10.2 Tab actions

| Action | Mouse | Keyboard |
| --- | --- | --- |
| New tab, default profile | Plus button | Ctrl+Shift+T |
| New tab, chosen profile | Chevron menu | Ctrl+Shift+Space opens the menu |
| New window | Chevron menu, "New window" | Ctrl+Shift+N |
| Close tab | Close button, middle click | Ctrl+Shift+W |
| Next tab | Click | Ctrl+Tab |
| Previous tab | Click | Ctrl+Shift+Tab |
| Select tab 1..9 | Click | Alt+1..9 |
| Duplicate tab | Context menu | Ctrl+Shift+D |
| Detach tab | Drag out, context menu | Ctrl+Shift+M |
| Rename tab | Context menu | F2 |
| Reorder tab | Drag within strip | Ctrl+Shift+Left/Right |
| Find | Search bar | Ctrl+Shift+F |
| Copy / paste | Selection, context menu | Ctrl+Shift+C / Ctrl+Shift+V |
| Select all | Context menu | Ctrl+Shift+A |
| Font zoom in/out/reset | Ctrl+wheel | Ctrl+= / Ctrl+- / Ctrl+0 |
| Full screen | Hamburger menu, View | F11 |
| Quit | Hamburger menu, Quit | Ctrl+Q |

Shortcuts are consumed by `TerminalWindow` before `FXTerminal` sees the key, so
Ctrl+Shift+C is copy even when a child would interpret it. A future key-binding
editor is out of scope; the table lives in one `TerminalActions` helper.

### 10.3 Tab state

`TerminalTab` holds:

```cpp
struct TerminalTab {
  FXTerminal* view;        // owned by the window's switcher
  FXString    profileId;
  FXString    title;       // from OSC 0/1/2 or fallback
  FXString    workingDir;  // from OSC 7 or profile
  FXbool      running;
  FXbool      bell;
  FXbool      exited;
  FXint       exitCode;
  };
```

- The fallback title is the profile name. Once the child sets a title, show
  that. If the title is empty or only whitespace, revert to the profile name.
- OSC 7 updates `workingDir`; a new tab from the profile uses the active tab's
  `workingDir` when it is a valid existing directory, otherwise the profile's
  starting directory.
- BEL sets `bell`; the tab bar draws a small indicator and clears it when the
  tab is activated. The application may also call the system bell, but no more
  than once per second.
- Exit state: apply the profile's `closeOnExit` setting:
  - `graceful` (default): close the tab on exit code 0, keep it open on a
    non-zero code.
  - `always`: close the tab on any exit.
  - `never`: keep the tab open and show the final screen.
  When kept open, the tab title gets a `[exited]` suffix, the status label shows
  the exit code, and Enter starts a fresh shell in the same tab.

### 10.4 Detach algorithm

`TerminalWindow::detachTab(index,screenX,screenY)`:

1. Validate the tab and remember its `TerminalTab`, `FXTerminal` and switcher
   index.
2. Create a new `TerminalWindow` without any tab. Build its widgets, call
   `create()`, call `hide()`, and place it at `screenX,screenY`, clamped to the
   visible work area.
3. Remove the tab metadata from the old window's vector.
4. Call `view->reparent(newWindow->switcher)`. Both windows are already
   created, so the native reparent is legal.
5. Add the tab metadata to the new window, insert it in the new tab bar,
   `setCurrent(0)`, call `layout()` and `show()`.
6. Refresh the old tab bar. If it has no tabs left, close and delete the old
   window after the new one is visible. If it still has tabs, activate the
   neighbor.
7. Call `view->setFocus()` on the new window.

The PTY, parser, scrollback, selection and search state stay with the
`FXTerminal`; only the parent changes. Do not destroy and recreate the widget.

Drag tear-off:

- On left press on a tab, record the index, pointer origin and grab the pointer.
- Motion inside the tab strip beyond 4 pixels horizontally reorders the tab.
- Motion leaving the tab strip by more than 12 pixels vertically arms a detach.
- On release outside the strip while armed, call `detachTab()` at the pointer
  position.
- On release inside the strip while not armed, activate the tab.
- On `SEL_LEAVE` or lost grab, cancel the drag state.
- The context menu and Ctrl+Shift+M always provide detach even if a platform
  delivers an unusual drag sequence. This is the Wayland fallback.

Cross-window reattach is not implemented. A detached tab can be closed and a
new tab created in another window; the session itself is not transferred
between processes.

### 10.5 Window and close behavior

- `Ctrl+Shift+N` and the menu create a window with one default tab.
- New windows cascade from the active window by 24 pixels; clamp to the visible
  area.
- Closing a window closes its tabs. If more than one tab is running and
  `confirmClose` is on, show one dialog: "N sessions are still running. Close
  the window?" with Cancel/Close.
- Closing a tab with a running child follows the same confirmation policy.
- On application quit (Ctrl+Q, menu, window manager close), confirm once for
  all running tabs, terminate every PTY, save settings and exit.
- Window position and size are saved when the last window closes and on
  `SEL_CONFIGURE`. Do not save when maximized beyond the work area.
- Restore only the last window geometry and the active profile. Do not restore
  tabs or scrollback.

## 11. Profiles and settings

### 11.1 Profile model

```cpp
struct TerminalProfile {
  FXString id;                  // stable key, e.g. "default"
  FXString name;                // display name
  FXString shell;               // executable path
  FXString arguments;           // command line arguments
  FXString startingDirectory;   // may be empty
  FXint    termKind;            // TERMINAL_VT100 or TERMINAL_XTERM
  FXString term;                // TERM value
  FXString encoding;            // default character encoding of a new session
  FXString colorScheme;         // built-in or user scheme name
  FXString fontFace;            // empty means theme FONT_MONO
  FXint    fontSize;            // points, 0 means theme size
  FXint    scrollback;          // lines, default 10000
  FXint    cursorStyle;         // block, bar, underline
  FXbool   cursorBlink;
  FXbool   confirmClose;
  FXint    closeOnExit;         // graceful, always, never
  FXbool   altSendsEscape;
  FXbool   warnOnMultilinePaste;
  FXbool   copyOnSelect;
  };

struct TerminalColorScheme {
  FXString name;
  FXColor  foreground;
  FXColor  background;
  FXColor  cursor;
  FXColor  selectionBackground;
  FXColor  selectionForeground;
  FXColor  palette[16];
  };
```

Profile values override the global `[APPEARANCE]` values for color scheme,
font and size only. An empty profile font/family/scheme means "inherit the
appearance setting"; a zero size means "inherit the theme size". Transparency
mode and background opacity stay global.

Built-in profiles:

- `default` on POSIX: `$SHELL` or `/bin/sh`, no arguments, home directory,
  `TERMINAL_XTERM`, `xterm-256color`.
- `default` on Windows: `%COMSPEC%` or `cmd.exe`, no arguments, user profile,
  `TERMINAL_XTERM`, `xterm-256color`.
- Optional discovered profiles: `PowerShell` (7 if present, otherwise 5.1) on
  Windows; `bash`, `zsh`, `fish` only if the executable is found at its
  standard absolute path. Do not scan `PATH` for defaults.
- Built-in color schemes: `Campbell`, `One Half Dark`, `Solarized Dark`,
  `Solarized Light`, `Tango`.

### 11.2 Registry layout

Use the application's `FXRegistry` (`getApp()->reg()`). Sections:

```
[SETTINGS]
x, y, w, h, maximized, activeProfile, windowCount

[PROFILES]
count, default
id0, name0, shell0, args0, dir0, term0, encoding0, kind0, scheme0,
font0, size0, scrollback0, cursor0, confirm0, closeonexit0

[APPEARANCE]
scheme, font, size, cursor, scrollback, confirmClose,
warnOnMultilinePaste, copyOnSelect, transparencyMode, backgroundOpacity

[SCHEMES]
count
name0..n, foreground0..n, background0..n, cursor0..n,
selectionBg0..n, selectionFg0..n
color00..color15 per scheme
```

Keys are written with `writeStringEntry`, `writeIntEntry`, `writeBoolEntry`;
read with the matching `read*Entry` calls. Validate every loaded value:

- `kind` must be 0 or 1 and must agree with `term`.
- `encoding` must be a supported name or alias; anything else falls back to
  UTF-8.
- `scrollback` is clamped to 0..100000.
- `fontSize` is clamped to 6..72.
- `cursor` is clamped to the three supported styles.
- `transparencyMode` is clamped to 0..2 and defaults to `off`.
- `backgroundOpacity` is clamped to 0..100 and defaults to 100.
- Color values that fail to parse fall back to the scheme default.
- A missing or unreadable profile falls back to `default`; never fail to start
  because of settings.

### 11.3 Settings dialog

`PreferencesDialog` is an `FXDialogBox` with two tabs:

- **Profiles**: `FXTable` of profiles and fields for name, shell, arguments,
  starting directory, terminal type, color scheme, font, font size, scrollback,
  cursor, close-on-exit, confirm-close, paste warning. Buttons: New, Duplicate,
  Delete, Set Default.
- **Appearance**: color-scheme combo, font selector (`FXFontDialog`), font size,
  cursor style, scrollback, confirm-close, warn-on-multiline-paste,
  copy-on-select, transparency-mode combo (Off/Window/Background), a
  `backgroundOpacity` slider or spinner, a capability label showing the
  effective mode, and a preview strip with 16 ANSI colors on a checkerboard
  background.

Apply semantics:

- Appearance changes apply live to every open terminal and tab bar.
- Profile changes apply to new tabs. Existing tabs keep their profile values
  until the profile is explicitly re-applied from the tab context menu.
- Cancel discards changes; OK writes the registry immediately.
- The dialog must be usable with the keyboard and must not depend on a mouse.

## 12. Build and registration

### 12.1 Library

Add the new library files to `fox2/lib/Makefile.am`:

- `FXTerminal.cpp`, `fxterminalscreen.cpp`, `fxterminalparser.cpp`,
  `fxterminalwidth.cpp` to `libFOX_2_0_la_SOURCES`.
- `fxpty_posix.cpp` to the POSIX conditional block.
- `fxpty_win32.cpp` to the `COND_OSWINDOWS` block.

CMake reads `Makefile.am` for sources. Because `fox2/lib/CMakeLists.txt`
filters `_win32|_posix|_unix|_x11|_wayland` files, add `fxpty_win32.cpp` and
`fxpty_posix.cpp` explicitly in the same platform branches as the other seam
files.

Transparency adds no new source files. `FXWindow`, `FXRaster`, `fxwayland`,
`FXVisual` and the existing platform window files are modified in place. The
alpha code is compiled on every backend and selected at runtime by the
capability query; the opaque path must stay identical when the mode is `off`.

Add `FXTerminal.h` to the installed public headers:

- `fox2/include/Makefile.am`: add it to `include_HEADERS` next to
  `FXConsole.h`.
- `fox2/include/fx.h`: add `#include "FXTerminal.h"` next to the console
  include, so application code can keep including only `fx.h`.
- CMake installs the whole `include/` directory, so no CMake header list needs
  a change.

POSIX dependency:

- Prefer `posix_openpt`; check with `AC_CHECK_FUNCS([posix_openpt])` in
  `configure.ac` and a CMake symbol check.
- If the fallback `openpty` path is compiled, add `AC_SEARCH_LIBS([openpty],
  [util])` and the equivalent CMake check. Do not add `-lutil` unconditionally.
- Windows needs no extra library for dynamically loaded ConPTY. The job object
  APIs are in `kernel32`.

### 12.2 Application

Add the application to four places:

| File | Change |
| --- | --- |
| `fox2/apps/CMakeLists.txt` | `add_subdirectory(foxterm)` and add `foxterm` to the install target list. |
| `fox2/apps/Makefile.am` | Add `foxterm` to `SUBDIRS`. |
| `fox2/configure.ac` | Add `apps/foxterm/Makefile` to `AC_CONFIG_FILES`. |
| `fox2/.gitignore` | Add `/apps/foxterm/Makefile.in` and `/apps/foxterm/foxterm`. |

`fox2/CMakeLists.txt` has a Windows GUI executable list. Add `foxterm` there.
`fox2/apps/foxterm/CMakeLists.txt` links `FOX::FOX` and `m`; the Autotools
file uses `-mwindows` on Windows like the other applications. No platform
conditionals may appear in the application sources.

### 12.3 Tests

Add `terminalmodel` to `FOX_CONSOLE_TESTS` and `FOX_CTEST_PLAIN` in
`fox2/tests/CMakeLists.txt`; add it to `noinst_PROGRAMS` and
`terminalmodel_SOURCES` in `fox2/tests/Makefile.am`. Add `pty` to the same
console-test lists, but not to CTest initially if it proves flaky on a host
without a usable shell; the gate script covers it. Extend the existing
`tests/raster.cpp` with the alpha cases from section 14.5; no new test target is
needed.

`terminalmodel` includes `../lib/fxterminalparser.h` and
`../lib/fxterminalscreen.h`. Add the private library include directory exactly
as the `raster` test does:

```cmake
target_include_directories(terminalmodel PRIVATE ${PROJECT_SOURCE_DIR}/lib)
```

`pty` includes `../lib/fxpty.h`. It uses the same include directory. The
existing `AM_CPPFLAGS = -I$(top_srcdir)/lib` in `tests/Makefile.am` covers the
Autotools build.

### 12.4 Command line

`foxterm` accepts:

```
foxterm [--profile NAME] [--term vt100|xterm] [--working-directory DIR]
        [--execute COMMAND [ARG...]]
```

- `--execute` starts the given command directly, without a shell, and each
  remaining argument is one argument. It is intended for tests and for desktop
  launchers.
- `--term` overrides the profile's terminal type; the matching launch profile
  is selected, not mixed.
- A command-line launch always creates one window with one tab. Adding a tab
  to an already running `foxterm` process would need an IPC protocol and is not
  part of this plan.
- Unknown options are reported with a usage message and exit 2.
- `--help` and `--version` exit without opening a display.

## 13. Implementation phases

Every phase ends with a build and the named tests. Commit library changes and
application changes separately so a regression can be reverted without losing
the other half.

### Phase 1: screen model and parser

Build `fxterminalscreen`, `fxterminalparser` and `fxterminalwidth` as private
library modules. Implement text, CR/LF, cursor motion, erase, insert/delete,
scroll regions, SGR, alternate screen, tab stops, title/OSC handling, limits
and resize/reflow. Add `terminalmodel.cpp` with the cases in section 14.1.

Acceptance:

- `terminalmodel` passes on Linux and Windows console builds.
- The parser accepts a 1 MiB mixed escape stream without a crash and without
  exceeding the limits in 5.4.
- VT100 and xterm profiles differ in the asserted ways.

Phase 1 result: implemented `fxterminalscreen`, `fxterminalparser` and
`fxterminalwidth`, wired them into `lib/Makefile.am`, and added
`tests/terminalmodel.cpp` to the CMake and Autotools test lists. The test covers
text, wrapping, tabs, erase/insert/delete, margins and origin mode, SGR,
alternate screen, UTF-8/wide/combining text, OSC, modes, replies, scrollback,
reflow, VT100 gating, charset mapping, malformed input and a bounded 1 MiB
fuzz/limit pass.

Passed so far: CMake/X11 and CMake/Wayland builds, all 27 CTest cases, the
MinGW-w64 cross build, and `terminalmodel.exe` under Wine. The Autotools library
and `terminalmodel` target build and pass as well; the full Autotools tree still
stops in the unrelated `apps/chart` generated `icons.cpp` rule in the existing
build directory.

### Phase 2: PTY seam

Implement `fxpty_posix.cpp` and `fxpty_win32.cpp`. Add `pty.cpp` with a
self-child test. Do not add UI yet.

Acceptance:

- POSIX test starts the test binary with a PTY, reads and writes text, resizes
  it, and observes the exit code.
- Windows test does the same through ConPTY on Windows 10 1809+.
- Missing ConPTY produces a clear failure, not a hang.
- Closing a session with a running child does not leave a zombie on POSIX and
  does not leave a live child tree on Windows.

Phase 2 result: added `include/FXTerminal.h` with the terminal kind and launch
description, `lib/fxpty.h`, `lib/fxpty_posix.cpp` and `lib/fxpty_win32.cpp`.
The POSIX implementation uses `posix_openpt`/`fork`/`execvp`, sets the terminal
window size and child environment, queues partial writes, and kills the process
group on close. The Win32 implementation loads ConPTY dynamically, creates the
child with a kill-on-close job, uses a reader thread for output and a writer
thread for input, and keeps the UI thread non-blocking. `tests/pty.cpp` starts
the test binary itself as the child, checks the ready marker, writes a line,
resizes the PTY, and verifies exit code 7.

Passed: the POSIX test on the CMake/X11 and CMake/Wayland builds. The Win32
source cross-compiles and `pty.exe` starts under Wine, but Wine does not provide
a functional ConPTY; the test detects that and reports `SKIP`. A real Windows
10 1809+ runtime pass is still required. Autotools builds the library and the
`pty` target and the POSIX test passes there too.

### Phase 3: `FXTerminal` view

Implement the widget, rendering, scrolling, selection, clipboard, cursor blink
and IME. Add the public header and library build wiring. Use a temporary local
harness under `tmp/` that starts `/bin/sh`; do not modify the existing console
test.

Acceptance:

- A shell is usable: prompt, typed commands, output, Ctrl+C, resize, scrollback.
- ANSI colors, bold, underline, reverse and cursor addressing render correctly.
- The alternate screen works with `vim`/`less`; exiting restores the primary
  screen and scrollback.
- The view redraws without flicker while output floods.
- Theme and font changes apply live.
- X11 and Wayland both look correct; Windows build passes at least a manual
  smoke test.

Phase 3 result (partial): added the public `FXTerminal` widget and
`lib/FXTerminal.cpp`, using the screen, parser and PTY from Phases 1 and 2.
Implemented painting, scrollback viewport, selection, clipboard copy/paste,
mouse reporting, cursor blink, focus/IME positioning, key encoding, title/bell
notifications and basic find. The view was exercised with a temporary X11
harness that starts `/bin/sh`, displays the prompt, accepts a command, shows its
output, and can inject `feed()` text. The CMake/X11 library and the MinGW-w64
cross build compile the widget. The Wayland library target also compiles.
Still to verify before Phase 3 is complete: interactive resize/`stty size`,
Ctrl+C, `vim`/`less` alternate-screen round trip, selection drag/copy in the
window, and a Wayland runtime frame. These move to the Phase 3 follow-up and
the Phase 4 application work.

### Phase 4: single-window application

Create `fox2/apps/foxterm` from the console example. Add the tab strip with a
single tab, hamburger menu, plus button, profile drop-down, status line, command
line parsing and setting persistence. Add the build registration.

Acceptance:

- `foxterm` starts, runs the default shell, and closes cleanly.
- New window opens; quit and window-manager close behave correctly.
- Geometry and active profile survive a restart.
- The application contains no `#if` blocks.

Phase 4 result: added `fox2/apps/foxterm` with `main.cpp`,
`TerminalWindow.*`, a single-tab `TerminalTabBar`, menu/plus/profile controls,
status line, default shell detection through `COMSPEC`/`SHELL`, `--profile`,
`--term`, `--working-directory` and `--execute`, and geometry persistence in
`~/.config/FoxTerm.rc`. The shell receives the pseudo terminal and the widget
shows prompt, output, selection/copy/paste and title changes. The parser now
answers the terminal queries fish uses (XTVERSION, kitty keyboard, OSC 10/11 and
XTGETTCAP), so the default fish prompt appears.

Passed: CMake/X11 and CMake/Wayland application builds; a live X11 run with
`/bin/sh` and `/usr/bin/fish`; a headless Wayland run with a captured frame; and
SIGINT saving the window geometry. The MinGW-w64
cross build produces `foxterm.exe` and `--help` runs under Wine. The X11
screenshot is `screenshots/tests/fox2/x11/foxterm.png`; the Wayland frame is
`screenshots/tests/fox2/wayland/foxterm.png`. The Autotools
`apps/foxterm` target builds; the full Autotools tree still stops in the
unrelated chart icon rule. Multiple tabs, profiles and close confirmation remain
for Phases 5 and 9.

### Phase 5: tabs

Implement multiple tabs, new/close/duplicate/rename/switch/reorder, overflow,
close confirmation and tab titles. Ensure all PTYs keep running when hidden.

Acceptance:

- At least 20 tabs can be opened; hidden tabs continue to run.
- Tab title follows OSC 0/1/2 and falls back to the profile name.
- Overflow scrolling keeps the active tab visible.
- Closing a tab terminates only its child.
- Keyboard shortcuts in section 10.2 work.

Phase 5 result (partial): the app now owns a vector of `TerminalTab` objects and
a single `FXSwitcher`. New tabs are created by the plus button, the profile
menu and the hamburger menu; the tab bar paints each title and close box,
switches the active view, and sends `ID_TAB`/`ID_CLOSE_TAB` to the window.
Closing a tab deletes only that view; hidden views keep their PTY running.
OSC titles update the owning tab. The `New Tab`, `New Window`, `Close Tab`,
`Copy`, `Paste`, `About` and `Quit` commands have accelerators. A programmatic
three-tab X11 run verified the tab strip and prompt; the screenshot is
`screenshots/tests/fox2/x11/foxterm-tabs.png`.

Not yet done: tab overflow scrolling, drag reorder, duplicate/rename, close
confirmation, Ctrl+Tab cycling, and the 20-tab gate. These remain for the
Phase 5 follow-up.

### Phase 6: detach

Implement `detachTab`, drag tear-off, context menu, Ctrl+Shift+M, reparent,
window placement, old-window closing and focus transfer.

Acceptance:

- Detaching a running tab creates a window with the same session, screen,
  scrollback, selection and search state; the child is not restarted.
- The old window remains if it has other tabs and closes if it was the last.
- Detaching the last tab of the last window leaves the application with one
  window, not zero.
- Cancel/lost-grab paths leave the tab in place.
- The context menu detach works on Wayland where a drag may be unusual.

Phase 6 result (partial): `TerminalWindow::detachTab()` creates a new window
with autostart disabled, reparents the live `FXTerminal` into its switcher, moves
the `TerminalTab` state and selects it. The old window remains with its other
tabs and is deleted through a chore when it becomes empty. A programmatic
three-tab X11 run detached the active tab and produced two windows: the old one
with two tabs and the new one with one; the PTY was not restarted.

Not yet done: drag tear-off, the tab context menu, Ctrl+Shift+M, and the
`SEL_LEAVE`/lost-grab cancel paths. The menu command `Detach Tab` is the working
entry point for now.

### Phase 7: profile conformance and reflow

Finish xterm and VT100 behavior: DA/DSR replies, mouse encodings, bracketed
paste, focus reporting, color reduction, charset line drawing, cursor styles,
and soft-wrap reflow. Compare against `vttest` where available and against
recorded sessions from `xterm` and `tmux`.

Acceptance:

- `vttest` sections for cursor movement, screen features and colors produce no
  unexplained mismatch.
- `TERM=vt100` programs see a VT100; `TERM=xterm-256color` programs see 256
  colors and xterm modes.
- Resize reflows only soft-wrapped primary rows.
- Mouse reporting works in `vim`, `mc` and a small test program; Shift
  overrides selection.

Phase 7 result (partial): the screen already reflows soft-wrapped primary rows;
the parser now answers XTVERSION, kitty-keyboard, OSC 10/11 and XTGETTCAP
queries, which lets fish and other probes complete. Cursor styles from DECSCUSR
are rendered as block, bar and underline in `FXTerminal`; focus reporting and
bracketed-paste framing are wired. The model test covers cursor style and query
responses. Full `vttest` comparison, `vim`/`mc` mouse checks and the complete
profile matrix still need a manual gate.

### Phase 8: input, clipboard and search

Finish IME, paste protection, OSC 52 policy, search bar, double/triple click,
auto-scroll on selection drag, font zoom and wheel behavior.

Acceptance:

- Right-click opens the terminal context menu of R19 and a tab right-click
  opens the tab menu; Shift+right-click works while a child has mouse reporting
  on; the menu key and Shift+F10 open it at the cursor.
- IME commits correctly on X11 and Windows.
- Multi-line paste warning appears and Cancel sends nothing.
- Bracketed paste is used only when the child enabled it.
- Search finds text in scrollback, highlights all matches and wraps around.
- Copy/paste works across applications and platforms.

Phase 8 result (partial): `Ctrl+Shift+C/V/A` copy, paste and select-all are
handled; paste uses bracketed-paste framing when mode 2004 is active; focus
in/out emit `ESC[I`/`ESC[O` when mode 1004 is active; mouse reports use SGR when
1006 is active and X10 button 3 on release otherwise. `foxterm` has a find bar
(`Find...`, Next, Prev, Close) backed by `FXTerminal::findText/findNext/findPrev`,
plus Next/Previous Tab commands. Multi-line paste confirmation, OSC 52 policy and
the full 20-tab search gate remain for the follow-up.

### Phase 9: profiles and settings dialog

Add `Profiles`, `TerminalColorScheme`, registry load/save, defaults, discovery
and `PreferencesDialog`.

Acceptance:

- Switching a live tab from UTF-8 to Latin-1 and back through the context
  menu or the Settings dialog changes how bytes 0x80..0xFF display (`printf
  '\xe4'` shows `ä` in Latin-1 and U+FFFD in UTF-8) without restarting the
  child; the profile stores the default (`[PROFILE] encoding`).
- Built-in profiles and schemes are available on a fresh registry.
- Editing a profile and creating a tab uses the new values.
- Appearance changes apply live to open terminals.
- A corrupt setting never prevents startup.
- Settings are written only when OK/quit chooses to save.

Phase 9 result (partial): added `Profiles` with built-in `default` and `vt100`
profiles, registry load/save for name, shell, arguments, directory, TERM, kind
and scrollback, and active-profile persistence. `foxterm --profile` selects a
profile; the profile drop-down creates a tab from the chosen profile; the
Settings dialog edits the active profile and applies it to new tabs. The
registry is written only on OK or quit.

Not yet done: color schemes, font/cursor profile fields, profile create/delete,
and live appearance updates. The current color palette and font remain the
built-in defaults.

### Phase 10: transparency baseline (`window` mode)

Add the transparency settings, the `FXWindow` opacity API and the per-backend
whole-window opacity implementations. Add the settings controls and the
capability label. Do not yet add the per-pixel `background` mode.

Acceptance:

- `off` is the default and the existing opaque screenshots are unchanged.
- `window` at 50% works on Wayland, on X11 with a compositing manager and on
  Windows with DWM; on a host without the required compositor the effective
  mode is `off` and the settings dialog says so.
- Opacity applies to the whole shell, including the tab strip and all open
  terminals in that window.
- Changing opacity live repaints and does not steal focus, break tab dragging or
  change PTY sizing.
- Whole-window opacity survives detach and application restart.
- The X11 property, Win32 layered-alpha attribute and Wayland surface alpha
  are inspectable by the tests in section 14.5.

Phase 10 result (partial): added `FXWindow::setOpacity()`, `getOpacity()` and
`isOpacitySupported()` with an X11 `_NET_WM_WINDOW_OPACITY` property, Win32
layered-window alpha, and a Wayland per-surface premultiplied scale pass.
`foxterm` has an Opacity menu and `--opacity PERCENT`; the value is stored in
`[APPEARANCE] opacity` and restored on restart. A live X11 run at 50% reports
`_NET_WM_WINDOW_OPACITY` as `2147483647` and the composited window is visibly
translucent. A headless Wayland PAM dump at 50% has center alpha `0x80`. The
Win32 path cross-compiles; no real Windows compositor run yet.

Not yet done: the settings-dialog capability label, detach across opacity
changes, and the formal pixel gates from section 14.5.

### Phase 11: per-pixel `background` mode

Implement the alpha-aware raster path and the backend capability gates. Do the
work in this order: shared raster correctness, Wayland, X11, Win32.

Acceptance:

- Wayland `background` at 50% shows desktop pixels through the cell background;
  text, cursor, selection text and tab strip have alpha 255.
- X11 advertises `background` only when the shell has a usable 32-bit ARGB
  visual and XRender; the test records either a pass or an explicit downgrade
  to `window`.
- Win32 advertises `background` only when DWM composition can keep opaque child
  text over a translucent client background; otherwise it records `window` as
  the supported mode and the limitation is documented.
- `backgroundOpacity=0` makes the grid background fully transparent but leaves
  text readable and selectable.
- Alpha glyph edges do not show dark or light halos on a transparent
  background.
- Switching between `off`, `window` and `background` on an open window does not
  require restarting the shell or recreating the terminal.
- The opaque pixel gates remain at 0 differing pixels.
- The platform support matrix in section 8.4 is updated with the measured
  result, including any backend that only reaches `window`.

Phase 11 is not started. `FXTerminalTransparency`, `backgroundOpacity`, the
alpha-aware raster fill/blend path and the per-pixel background capability gates
remain unimplemented. `window` opacity is the only transparency mode available.

### Phase 12: hardening, verification and documentation

Finish the security limits, fuzzing, performance checks, screenshots, user
guide and man page. Run the full build matrix and the UI gates.

Acceptance:

- Section 14 passes, including the transparency gate.
- `docs/terminal.md` and `foxterm.1` describe keys, profiles, settings and
  limitations.
- The transparency support matrix and the fallback behavior are documented
  without overclaiming a backend.
- Screenshots exist for X11, Wayland and Windows, including one transparent
  background and one whole-window opacity shot per supported backend.
- No new compiler warnings in the new files under the project warning flags.

### Status review (Wayland build)

Reviewed against the tree on the `foxterm` Wayland target
(`FOX_WAYLAND_TARGET=foxterm ./scripts/build-fox2-wayland.sh`). The build is
clean for the terminal files. `terminalmodel` and `pty` pass. `foxterm --help`
runs, and `foxterm --execute /bin/sh -c ...` starts on the live Wayland session.

Done: Phase 1, Phase 2 (POSIX), most of Phase 3, Phase 4, the core of Phase 5,
the in-process detach of Phase 6, the core of Phases 7 to 9, `window` mode
of Phase 10, N1 (the right-click context menus) and N2 (the character encoding
switch, including the profile field). Phase 11 and Phase 12 are not started.

Bugs and gaps found in review:

- **B1 `closeOnExit` is never applied. FIXED.** `TerminalWindow::onCmdExited()`
  now applies the policy of the tab's profile: with `closeonexit=true` the tab
  closes through a chore (the widget is still inside its own read callback, so
  it must not be destroyed there), the last tab of the last window ends the
  application, and the last tab of a window that has siblings deletes only that
  window. With `closeonexit=false` the tab keeps the final screen and gains the
  `[exited]` suffix. `TerminalTab` records its `profileid` so the policy is the
  one the tab was created from, also after a detach.
- **B2 `confirmClose` is never applied. FIXED.** `closeTab()` asks through
  `FXMessageBox::question(MBOX_OK_CANCEL)` when the child of the tab is still
  running and its profile sets `confirm`; Cancel leaves the tab alone. Quit,
  window-manager close and Ctrl+Q ask once for every running session of the
  process (`MBOX_QUIT_CANCEL`), and a termination signal takes the new
  `onSignalQuit` path so a gate or a logout never waits on a dialog.
- **B2a Window accelerators never acted. FIXED.** `FXMenuCommand` registers its
  accelerator on the owning window, but `FXComposite::onKeyPress` bounces the
  key to the focus widget first, so `FXTerminal` turned Ctrl+Shift+W into 0x17
  for the child and no shortcut of section 10.2 worked. `FXTerminal::onKeyPress`
  now consults the accel table of its shell before encoding the key, and also
  tries the shifted form of a letter because a Ctrl+Shift+letter carries the
  unshifted keysym on Wayland. `Detach Tab` gained its `Ctl-Shift-M` label.
- **B2b Deleting a window crashed on the next update pass. FIXED.** The opacity
  submenu was created as a local `FXMenuPane`; it leaked, its commands kept the
  deleted window as their target, and the update walk dereferenced it
  (SIGSEGV in `FXWindow::onUpdate` under `FXMenuCaption::onUpdate`). Every menu
  pane is now a member and is deleted in `~TerminalWindow`. This also repairs
  the detach path that deletes a window emptied by tear-off.
- **B3 No docs or screenshots for Wayland tabs and opacity.** `docs/terminal.md`
  does not exist. `foxterm.1` now matches the command line (`--opacity`) and
  states the tab, detach, close-on-exit and confirmation behavior; the user
  guide and the re-captured screenshots are still missing.
- **B4 Phase 3 gates are unverified:** `stty size` after resize, Ctrl+C,
  `vim`/`less` alternate-screen round trip, selection drag, and a real Wayland
  frame of the interactive shell. The keyboard path is now covered by
  `gate-wayland-foxterm.sh` (typing reaches the child, Ctrl+Shift shortcuts act).
- **Missing, Phase 5:** overflow scrolling, drag reorder, duplicate, rename,
  the 20-tab gate. Ctrl+Shift+T/W/M, Ctrl+Q and the Next/Previous Tab
  accelerators work again through B2a.
- **Missing, Phase 6:** drag tear-off, tab context menu, lost-grab cancel.
  `Ctrl+Shift+M` (B2a) is the working detach shortcut.
- **Missing, Phase 8:** multi-line paste confirmation, OSC 52 policy (requests
  are only blocked and reported in the status line), IME gate, font zoom.
- **Missing, Phase 9:** color schemes, font and cursor fields, profile
  create/delete, live appearance updates.
- **Missing, Phase 10:** capability label in the settings dialog, detach and
  restart opacity gates, pixel gates from section 14.5.
- **Missing, whole plan:** the Windows ConPTY run is still untested on real
  Windows. The new `scripts/gate-wayland-foxterm.sh` covers the tab and window
  lifetime on headless sway; see "Fix result (B1, B2)" below.

- **B5 Missing glyphs draw as `.notdef` boxes (no font fallback).** Seen in
  `tmp/foders.png`: `eza`/`ll` icons (Nerd Font private-use codepoints such as
  U+F07B folder) show as a box in `foxterm`; Konsole shows the icons. Cause:
  `FXGlyphs` (`lib/FXGlyphs.cpp`) uses one FreeType face and calls
  `FT_Load_Glyph(f,FT_Get_Char_Index(f,ch),...)` (line 258). When the face has
  no glyph, the index is 0 and the face's `.notdef` box is drawn. The default
  mono face (`Noto Sans Mono`) has no private-use icons, although the system has
  85 fonts that do (the Meslo Nerd Fonts). Konsole asks fontconfig for a
  fallback font per missing codepoint. Fix is Step S9.

Visual review (screenshots, not re-captured in this review):
`screenshots/tests/fox2/wayland/foxterm.png` and
`x11/foxterm-tabs.png` date from 26 Nov, before the later theme work. They were
compared with the two targets, Windows Terminal on Windows 11 and Konsole on KDE
Plasma:

- **V1 Cell spacing is wrong.** The prompt `sh-5.3$` is drawn with a wide gap
  between glyphs; the cell width is larger than the glyph advance, so text looks
  letter-spaced. Windows Terminal and Konsole pack cells tightly. Check
  `getTextWidth()` rounding and the font choice (`FONT_MONO` must be a true
  fixed-pitch face).
- **V2 Tab strip is not Windows Terminal style.** The X11 strip has a plain white
  background and a hard-edged tab; Windows Terminal uses a darker strip, a
  rounded-top active tab that merges into the terminal background, a subtle
  hover state and a close button visible only on hover or on the active tab.
  Konsole uses a flat bar with a thin accent line. Use the theme roles (the
  Wayland frame already uses a dark strip with a blue accent line).
- **V3 Buttons look like 1990s FOX.** `+`, `Default`, `Menu` are bordered raised
  buttons with caret glyphs overlapping the text. Windows Terminal shows
  flat `+` and chevron buttons that highlight on hover; use flat toolbar
  buttons with separate drop-down arrows. (The overlap itself is fixed: the
  two menu buttons now carry `JUSTIFY_NORMAL|ICON_BEFORE_TEXT`, so the caret
  sits left of the label instead of being centred over it.)
- **V4 Chrome carries too much.** The Wayland frame has no padding around the
  grid (text touches the left edge) and a tall status line. Windows Terminal and
  Konsole pad the grid by 4 to 8 px and Konsole hides the status bar. Add a
  grid padding metric and make the status line optional (hidden by default,
  shown on exit or error).
- **V5 No scrollbar, cursor or selection shown** in any screenshot. Konsole and
  Windows Terminal both show a thin overlay scrollbar; capture a frame with a
  visible cursor, a selection and a scrollbar.
- **V6 Native title bar is intentional** (section 2). Windows 11 shows tabs in
  the title bar; this stays a documented difference.
- **V7 Re-capture** all screenshots on the current tree and add `foxterm-menu`
  (context menu) and `foxterm-latin1` shots. The method is in Step S8 below.

Steps S1 to S9 in "Visual plan" below fix V1 to V7 and B5.

New requests, now R19 and R20 in section 1 and in the Phase 8 and Phase 9
acceptance lists:

- **N1 Context menu. DONE.** `FXTerminal` forwards a right press it does not
  report to the child as `SEL_RIGHTBUTTONPRESS`, `TerminalWindow` posts the view
  menu and the strip menu, and the entries are enabled or disabled through
  `SEL_UPDATE` handlers. The Encoding submenu shows UTF-8 and stays disabled
  until N2. Details and the platform fix in "Fix result (N1)" below.
- **N2 Encoding switch. DONE.** `FXTerminalCharset` in `lib/` decodes output
  and encodes keys, paste and IME text; UTF-8 stays the default and the fast
  path. The context menu carries a checked radio per encoding, `[PROFILE]
  encoding` sets the default of a new tab, and `terminalmodel` covers every
  encoding. See "Fix result (N2)" below. Not done: the child's `LANG`/`LC_ALL`
  is still inherited (section 7.2 forbids overriding the user's locale), so a
  legacy profile changes how the view decodes bytes and not what the shell
  believes its locale is; a run-time switch of a *multibyte* legacy encoding
  (Shift_JIS, GBK) is out of scope by the same decision.

### Visual plan: reaching the Windows Terminal / Konsole look

Goal: on Linux (X11 and Wayland) and Windows, `foxterm` looks like Windows
Terminal in layout and like Konsole in density. No new widget set; all changes
use theme roles (`ROLE_*`) and metrics (`METRIC_*`) so every theme works. Do
the steps in order; each step ends with a re-captured screenshot compared with
the target.

**Step S1: fix cell metrics (`FXTerminal.cpp`, `cellw` near line 403).**
Cause: `cellw` comes from `getTextWidth("M")` of whatever face `FONT_MONO`
resolves to. If that face is proportional, or hinting rounds `M` wider than the
advance, every cell is too wide. Work:
- Verify that `FONT_MONO` resolves to a fixed-pitch face (`FXFont` pitch flag);
  if not, search the fallback list (`Cascadia Mono`, `Consolas`, `DejaVu Sans
  Mono`, `Liberation Mono`, `Noto Sans Mono`) and use the first fixed-pitch hit.
- Take `cellw` as the maximum advance over `0`..`9` and `M` and `W`, and take
  `cellh` as `ascent+descent` without extra leading; no rounding up of both.
- Draw each cell run at `col*cellw`; a run of equal attributes is one
  `drawText` call, so kerning inside a run cannot widen it.
- Test: a `terminalmodel`-independent gate that feeds `printf '%*s' 80 |
  tr ' ' M` and checks the row is exactly `80*cellw` px wide, and a screenshot
  where `sh-5.3$` touches no gap wider than 1 px.

**Step S1 result (done).** The cause was one level below the widget: no preset
sets a font role, so `FXTheme::font(FONT_MONO)` was empty and
`FXApp::getRoleFont(FONT_MONO)` inherited the interface font. On this machine
that is `Noto Sans` with `i=3, M=11, W=12, 0=7`, while the terminal sized a cell
from `M`: 11 px cells for an 8 px grid, which is the letter spacing of V1.

- `FXApp::roleFontDesc()` resolves the fixed pitch role when the theme leaves it
  open: `Cascadia Mono`, `Consolas`, `DejaVu Sans Mono`, `Liberation Mono`,
  `Noto Sans Mono`, `monospace` are tried in order, and a candidate is only
  accepted when `FXFont::isFontMono()` reports fixed pitch. The result is cached
  and re-resolved by `releaseRoleFonts()` on a theme change. `FXText`,
  `FXConsole` and `FXTerminal` all asked for the mono role and all silently got
  the proportional face; they now get `Noto Sans Mono` with an 8 px advance on
  both backends.
- `FXTerminal` metrics: `cellw` is the widest advance of `0`..`9`, `M` and `W`
  (equal for a fixed pitch face), `cellh` is `ascent+descent` with no leading,
  metrics are taken again in `create()` once the font is realized, and
  `fixedpitch` records whether ten glyphs really advance ten cells.
- `FXTerminal::onPaint()` draws a run of cells that share foreground,
  background and attributes with one `drawText()`, plus one underline,
  overline or strike line per run. Runs are only joined for a fixed pitch face
  and only over one-cell glyphs with no cluster, so a wide or clustered cell
  keeps its own call. This also paints the second half of a wide cell.

Gate: `scripts/gate-wayland-foxterm-metrics.sh` starts a shell that prints two
rows of 80 `M` and measures the committed frame: 80 separate blobs, one pitch,
largest gap 1 px, ink 639 px against 80 pitches of 640 px, row pitch 18 px
(= ascent 14 + descent 4). With the old resolution the same gate fails
(proportional face, 222 blobs). The `gate-wayland-foxterm.sh` phases,
`terminalmodel`, `pty` and the X11 CMake target still pass.

Not in S1: CJK still renders as boxes on this backend through a mono face
without CJK coverage (the same happens with the proportional face, so it is a
font fallback topic, not a metric defect); the committed screenshots are
recaptured by V7.

**Step S2: grid padding and chrome density.**
- Add a `padding` property to `FXTerminal` (default `METRIC_PADDING`, at least
  6 px, scaled by DPI) applied to all four sides; the background fills the
  padding, selection does not.
- Remove `FRAME_SUNKEN|FRAME_THICK` from the switcher; the terminal has no
  border. A 1 px `ROLE_BORDER` line separates strip and grid only.
- The status line becomes a transient bar: hidden by default, shown on child
  exit, on error and while the find bar is open. Konsole has no status bar and
  Windows Terminal has none.

**Step S2 result (done).**

- `FXTerminal` has `setPadding()` and `getPadding()`. The padding is
  `FXMAX(6, theme->metric(METRIC_PADDING))` (so the display scale carries), it
  applies to all four sides, and it is recomputed by `applyTheme()`. Explicit
  margins passed to the constructor still win (`getPadding()` returns -1), and
  all-zero margins select the theme padding; the background fills it while
  selection stays inside the cells.
- The switcher lost `FRAME_SUNKEN|FRAME_THICK` and the vertical frame's
  `DEFAULT_SPACING` of 4, which left a light 4 px band above and below the
  hairline. The separation is one `FXCanvas` row painted at paint time with
  `ROLE_BORDER`, so a live theme change needs no bookkeeping.
- The status line starts hidden. `showStatus()` shows it and arms a 4 s timeout,
  `hideStatus()` clears that timeout and hides it, and the find bar keeps it up
  (find count included) until the bar closes. Child exit, a startup error, a
  blocked clipboard request and an opacity change use the transient form.
  Showing or hiding it re-lays the frame out explicitly, because a Wayland
  tree is only laid out on a configure event.

Gate: `scripts/gate-wayland-foxterm-chrome.sh` measures committed frames:
strip 26 rows, exactly one separator row, first grid row 27; first glyph ink at
x=15 and 10 rows below the hairline (6 px padding plus bearings); the bottom
row is terminal background (no reserved status bar); select-all keeps the
padding columns at the terminal background while the cell under test is
selected; with `closeonexit=false` a child that exits brings the status line in
and it is gone 4.5 s later. Restoring the old 4 px frame spacing makes the gate
fail.

**Step S3: Windows Terminal style tab strip (`TerminalTabBar.cpp`).**
Paint everything in `onPaint` from theme roles; no child widgets per tab.
- Strip background: `ROLE_WINDOW_BG` darkened 8 percent (the Windows Terminal
  title strip is darker than the content). Strip height `METRIC_CONTROL_HEIGHT`.
- Active tab: filled with the terminal default background so it merges with the
  grid, top corners rounded by `METRIC_CORNER_RADIUS`, text `ROLE_TEXT`. No raised
  border, no underline box.
- Inactive tab: no fill; `ROLE_SURFACE_HILITE` at 40 percent alpha on hover;
  text `ROLE_TEXT_DISABLED` blended toward `ROLE_TEXT`.
- A 2 px `ROLE_ACCENT` line on top of the active tab, the Konsole alternative,
  selectable by a theme flag `tabAccentLine` (off by default).
- Tab width: 120 px target, shrinking to 48 px as tabs are added, then overflow
  arrows; title elided with `...`.
- Close button: a 12 px cross, drawn only on the active tab and on the hovered
  tab, with a round hover background; middle click closes the tab.
- Bell and exit state: a small dot before the title (`ROLE_ACCENT` for bell,
  `ROLE_TEXT_DISABLED` for exited).
- Separator: a 1 px vertical line between two inactive tabs, hidden next to the
  active or hovered tab.
- Animation: reorder and open/close use `METRIC_ANIMATION_TIME` when the theme
  enables animations; otherwise immediate.

**Step S4: flat strip buttons.**
- Replace `newbutton`, `profilebutton` and `menubutton` by tab-bar painted
  hit areas (drawn in the same `TerminalTabBar`): `+`, a chevron drop-down, and
  a hamburger glyph at the right end. They are flat, 28 px wide, get the hover
  rounded rectangle of S3 and open the same `FXMenuPane`s through `popup()`.
- No text labels `Default` or `Menu`; tooltips carry the names and the active
  profile name appears in the `+` tooltip.
- Use `FXMenuButton` only as the popup host (hidden); do not use
  `FRAME_RAISED|FRAME_THICK`.

**Step S5: scrollbar, cursor, selection.**
- Overlay scrollbar of `METRIC_SCROLLBAR_WIDTH`/2 on the right: invisible until
  scroll or mouse-over, fades after 1.5 s, thumb colour `ROLE_SCROLLBAR_THUMB`.
  It is drawn inside the padding and never changes the column count.
- Cursor: block is a solid cell with the glyph drawn in the background colour;
  an unfocused terminal draws a 1 px hollow block (Konsole and Windows Terminal
  both do this). Bar and underline are 2 px (scaled).
- Selection colours from the colour scheme, not fixed `selbackcolor`; the
  selection starts at the cell edge nearest the pointer (half-cell rule).
- Search matches: all matches `ROLE_ACCENT` at 35 percent alpha, the current
  one 70 percent.

**Step S6: colour scheme and font defaults.**
- Ship the Windows Terminal default scheme (`Campbell`: background `#0C0C0C`,
  foreground `#CCCCCC`, the 16 ANSI colours of Campbell) as the default and
  `One Half Dark`, `Solarized Dark` and `Konsole Breeze` as presets; the
  current `FXRGB(12,12,12)`/`(204,204,204)` already match Campbell.
- Default font: `Cascadia Mono` 12 pt if installed, else the S1 fallback list.
  Bold uses the bold face, not smeared double-strike; "bright bold" is a scheme
  option.
- Text rendering uses the grayscale antialiasing of the raster backend on
  Wayland; no subpixel colour fringes on transparent windows (Phase 11).

**Step S7: dialogs and menus.**
- The context menu (R19) and the hamburger menu use the theme menu style with
  shortcut text right-aligned, separators between groups, and the Encoding
  submenu with radio marks.
- The Settings dialog uses a left list (`Profiles`, `Appearance`, `Encoding`)
  and a right pane in Windows Terminal style; no nested tab books.

**Step S9: per-glyph font fallback (fixes B5).**
Status: done on Wayland, X11 and Win32 (Win32 only cross-compiled, not run on
real Windows). `FXGlyphs` (Wayland) opens up to 8 fallback faces through
`FcFontSort`; the Xft path (`fxXftDrawUtf8` in `FXFont_x11.cpp`) does the same
per character run and also measures with the fallback font; the GDI path
(`fxWin32Text` in `FXFont_win32.cpp`) enumerates the installed families, ranks
icon fonts first for private-use codepoints, and `FXFont::hasChar` now asks
`GetGlyphIndicesW`. Nerd Font and Symbols faces (Mono first) are preferred for
private-use codepoints on all three; a codepoint nobody covers is remembered.
`tests/glyphs` checks the Wayland cache; X11 was checked by drawing U+F07B into
an `FXImage`. `FXTerminal` draws a fallback glyph on its own, after its row,
clipped to its cells plus a following blank cell, and keeps it out of text runs.
Screenshot: `screenshots/tests/fox2/wayland/foxterm-icons.png`. Not done: glyphs
are not scaled down to the cell (they are clipped), box-drawing characters come
from the font (the terminal does not draw them itself yet), a codepoint nobody
covers still shows the main face's `.notdef` box, and the Win32 path needs a run
on Windows 10/11 with a Nerd Font installed.
Library change in `FXGlyphs`/`FXFont_wayland.cpp` (and the matching X11 and
Win32 font code), not in `FXTerminal`, so every widget benefits.
- When `FT_Get_Char_Index()` returns 0 for a codepoint, ask fontconfig for a
  font that covers it: build a pattern with the primary family, `FC_CHARSET`
  containing the codepoint, `FC_SPACING=FC_MONO` when the primary face is mono,
  then `FcFontSort`/`FcFontMatch` and take the first font whose charset has it.
  Win32 uses `IDWriteFontFallback`/`GDI` font linking; X11 uses Xft with the same
  fontconfig query.
- Keep a small list of fallback faces per `FXGlyphs` (maximum 8), opened lazily
  at the same pixel size, and a per-codepoint miss cache so a codepoint with no
  provider is searched once. Glyphs from a fallback face are cached under the
  codepoint like normal glyphs.
- For `FXTerminal` a fallback glyph is scaled to the cell: advance is forced to
  `cellw` (two cells for width 2), the glyph is clipped to its cells, and a glyph
  wider than the cell is shrunk to fit. Nerd Font icons are single-width in
  `... Nerd Font Mono` faces and slightly wider in the plain faces, so the shrink
  rule matters.
- Search order for private-use codepoints (U+E000..U+F8FF, U+F0000..): fonts
  whose family name contains `Nerd Font` or `Symbols`, then fontconfig order.
  Add `Symbols Nerd Font Mono` to the recommended packages in `docs/terminal.md`.
- When no font covers a codepoint, draw the replacement box once per cell, not
  the primary face's `.notdef` (which may be blank or ugly).
- Do not fall back for box-drawing, block and Braille characters
  (U+2500..U+259F, U+2800..U+28FF): `FXTerminal` draws them itself so lines join
  without gaps in every font. This is also what Konsole and Windows Terminal do.
- Test: `terminalmodel` stays display-free; add a display test that feeds
  U+F07B, U+E5FF, U+1F4C1 and `ä`, and checks that the first three differ from
  the `.notdef` bitmap when a covering font is installed, and that the cell
  pitch does not change. Add the folder icon row to the `foxterm-ls`
  screenshot in Step S8.

**Step S8: verification (answers V7).**
- Add `scripts/gate-wayland-foxterm.sh` on the model of
  `gate-wayland-stopwatch.sh` (headless sway, virtual pointer): starts
  `foxterm --execute` with a fixed script that prints colours, `ls` output and a
  `less` page; saves `foxterm.png`, `foxterm-tabs.png` (5 tabs, one hovered, one
  with bell), `foxterm-menu.png`, `foxterm-latin1.png`, `foxterm-selection.png`
  and `foxterm-opacity.png`.
- Pixel assertions, not only pictures: grid left edge is `padding` px from the
  window edge; the active tab fill equals the terminal background; the cursor
  cell colour equals the scheme cursor colour; the cell pitch of a
  `MMMM` row is `cellw`.
- Review each screenshot side by side with a Windows Terminal 1.x screenshot
  (dark, Campbell, Cascadia Mono 12) and a Konsole Breeze Dark screenshot of
  the same content, and record the remaining differences in this section.
- Windows and X11 get the same shots through
  `scripts/capture-test-screenshots.ps1` and the X11 harness.

Acceptance for the visual goals:

- The prompt row is a tight monospace row (S1); the grid has padding (S2).
- The tab strip, active tab, hover state, close button and `+`/chevron/hamburger
  controls match the description in S3 and S4 in all built-in themes.
- No raised or thick frames remain in the foxterm window.
- Screenshots of all three platforms exist and the recorded differences from
  the targets are limited to: native title bar (V6 above) and font
  availability.

Next steps in order: visual Steps S3 to S5, B4 gate script, Phase 5 and 6
follow-ups, Phase 8 follow-ups, Phase 9 color schemes, Phase 11, Phase 12.
B1, B2, N1, N2, S1 and S2 are done; see the fix results below.

### Fix result (B1, B2)

Fixed on the Wayland target (`FOX_WAYLAND_TARGET=foxterm
./scripts/build-fox2-wayland.sh`), clean build, no new warnings in the
terminal files.

Changes:

- `fox2/apps/foxterm/TerminalWindow.h/.cpp`: `TerminalTab` records its
  `profileid`; `tabProfile()`, `confirmCloseTab()` and `confirmQuit()` read the
  policy; `closeTab(index,force)`, `onChoreCloseTab()` (SEL_CHORE),
  `onSignalQuit()` (SEL_SIGNAL, no dialog), `terminateTabs()` and a static list
  of live windows back the close, quit and last-window decisions; every menu
  pane is a member and deleted in the destructor.
- `fox2/lib/FXTerminal.cpp`: the shell accelerator table is consulted before a
  key is encoded for the child, including the shifted form of a letter.
- `fox2/apps/foxterm/foxterm.1`: tabs, detach, close-on-exit, confirmation and
  `--opacity` documented.
- `scripts/gate-wayland-foxterm-chrome.sh`: visual step S2 gate; strip,
  hairline, padding, padding under selection and the transient status line.
- `scripts/gate-wayland-foxterm-metrics.sh`: visual step S1 gate; two rows of
  80 `M` in a committed frame must show 80 glyph blobs on one 8 px pitch with
  gaps of at most 2 px, ink spanning 80 pitches, and the cell height as the row
  pitch.
- `scripts/gate-wayland-foxterm.sh`: new headless-sway gate with six phases:
  default close-on-exit closes the process; `closeonexit=false` keeps the tab
  with `[exited]`; SIGINT quits without a dialog; Ctrl+Shift+W on a live child
  asks and Escape keeps the session, then OK closes the last tab and the
  process; typed text still reaches the child; Ctrl+Shift+T adds a tab,
  Ctrl+Shift+M detaches it, closing the detached tab leaves the other window
  and Ctrl+Q quits cleanly.

Result: `terminalmodel` and `pty` pass, the gate passes all six phases, and the
`windows/Ctrl+Q` runs no longer crash. Not run: the X11 gate for this change
(the library change is backend neutral) and a real Windows run. Existing
failures in this checkout are unrelated to the change: `raster` segfaults in
`FXRaster::fillPolyspan`, and `half`, `math` and `variant` fail their numeric
or platform comparisons; neither touches the terminal files.

### Fix result (N1)

Fixed on the Wayland target (`FOX_WAYLAND_TARGET=foxterm
./scripts/build-fox2-wayland.sh`), clean build, no new warnings in the terminal
files.

Requirements R19. What is there now:

- `FXTerminal` has `onRightBtnPress`/`onRightBtnRelease`. With mouse reporting
  on, the press goes to the child as button two (xterm numbering) and `Shift`
  is the escape hatch that still opens the menu, the same rule the left button
  has for selection. Otherwise the event is forwarded to the target as
  `SEL_RIGHTBUTTONPRESS`, so the application posts the menu and the widget
  stays free of UI.
- `FXTerminal` gained `hasSelection()`, `getScrollbackCount()`,
  `clearScrollback()` and `reset()` (RIS, which also clears the scrollback, as
  the reset sequence does). `FXTerminalScreen` gained `clearScrollback()`.
- `TerminalWindow` has a view menu (Copy, Paste, Select All, Find, Clear
  Scrollback, Reset Terminal, Character Encoding, New Tab, Detach Tab, Close
  Tab) and a strip menu (Duplicate, Rename, Detach, Close, Close Others). Both
  are members, deleted in the destructor (the B2b lesson). Every entry is
  enabled or disabled by a `SEL_UPDATE` handler: Copy needs a selection, Paste
  a running child, Clear Scrollback retained rows, the tab entries a tab under
  the press, Close Others a second tab, Detach Tab and Close Tab a live tab.
  Nothing is hidden. The handlers send `ID_ENABLE` or `ID_DISABLE` to the
  sender themselves (`setEnabled()` in `TerminalWindow.cpp`): the return value
  of a `SEL_UPDATE` handler only says the update was handled, and a menu entry
  created without `MENU_AUTOGRAY` never grays on it. The first version returned
  0 or 1 only, so every entry was drawn enabled; phase 6 of the gate caught it. The Encoding submenu is one disabled, checked
  UTF-8 entry until N2.
- `TerminalTabBar` has `onRightBtnPress` and posts `ID_CONTEXT_TAB` with the
  index, or -1 for the empty part of the strip (the entries then disable).
- `TerminalWindow::addTab()` takes the launch by value: the vector of tabs
  reallocates while a duplicate is being started, and a reference into it
  dangled - duplicating a tab crashed the application before this.

The platform side had to be fixed as well, and that is the part worth reading
before touching popups again:

- A menu posted at the pointer is a grabbing popup that appears under the
  cursor. `FXPopup::onMap`/`onEnter`/`onMotion` release the FOX-level pointer
  grab in exactly that case ("window appeared under the cursor"), which is
  harmless on X11 because the server-side grab still routes the press to the
  popup, and fatal on Wayland where that grab is all there is: the press that
  should dismiss the menu went to the widget below it and the menu stayed up.
  The fix is in `docs/wayland-plan.md`, section "A press inside a popup and the
  press that dismisses it": the backend routes a grabbed press that lands
  inside a grabbing popup to the widget under the pointer (as the X11 server
  does) and unposts the topmost popup when the press lands outside it, using up
  the press as X11 does. `FXPopup` itself is unchanged.
- A selector of `FXTerminal`'s own enum cannot be used for a notification:
  `FXScrollArea::ID_LAST` is 56 and `FXMainWindow::ID_LAST` is 60, so
  `FXTerminal::ID_CONTEXTMENU` (56+6) is `TerminalWindow::ID_EXITED` (60+2) and
  the right press closed the tab. The notification uses
  `SEL_RIGHTBUTTONPRESS` with an id of 0 instead. Note that
  `FXTerminal::ID_TRANSPARENCY` (60) and `ID_TERMINAL_SEARCH_CHANGED` (61)
  already collide with `TerminalWindow::ID_TITLE` and `ID_BELL`; neither is
  sent today, but a sender would have to be added with care.

Gate: `scripts/gate-wayland-foxterm-context.sh`, six phases on headless sway
with `tools/wlxinject.c` (the injector needed a `stty -icanon` in the child:
without it a mouse report waits in the line discipline for a newline):

1. A right click in the grid posts a menu that becomes a grabbing popup.
2. A click outside dismisses it and the application keeps running.
3. Select All from the keyboard plus Copy from the menu, read back with
   `scripts/wlclip.c`: the peer gets the marked line, so the entry really acts.
4. With mouse reporting on, the child receives `ESC [ M 34 ...` (button two)
   and no menu appears; `Shift`+right click still opens the menu (the shift is
   held by `wtype -M shift` while the injector clicks).
5. A right click on a tab posts the strip menu, Duplicate really adds a tab
   (detaching it then opens a second window, which detaching one tab cannot
   do), and a right click on the empty part of the strip posts the same menu.
6. Entries that cannot act are drawn disabled and come back when they can,
   read from a `grim` screenshot of the popup (menu text is black, a disabled
   entry gray): Copy without and with a selection, Clear Scrollback without
   rows, with rows and after it emptied them, every tab entry on the empty
   strip, Close Others on a single tab.

Result: the gate passes all six phases. The surrounding gates still pass on
the same build: `gate-wayland-menu`, `gate-wayland-tooltip`,
`gate-wayland-clipboard`, `gate-wayland-dnd`, `gate-wayland-repaint`,
`gate-wayland-mdi-drag`, `gate-wayland-foxterm`, `-metrics` and `-chrome`, plus
`terminalmodel` and `pty`. The X11 build (`scripts/build-fox2-linux.sh`) is
clean; its runtime behaviour is unchanged, `FXPopup` was not modified. Not run:
an X11 runtime check of the menu (no X server in this checkout) and a Windows
run.

Still missing from R19: the tab menu's Duplicate starts the same profile
instead of the same *session* (it does carry the live encoding of the session
it was made from, see N2), and Rename asks for a whole new title (FOX has no
input dialog with a preset value) and is overwritten by the next OSC title from
the child.

### Fix result (N2)

Fixed on the Wayland target (`FOX_WAYLAND_TARGET=foxterm
./scripts/build-fox2-wayland.sh`), clean build, no new warnings in the
terminal files. `terminalmodel` and `pty` pass, and the new gate passes all
four phases (see below).

What is there now:

- `fox2/lib/fxterminalcharset.{h,cpp}`: `FXTerminalCharset` holds the encoding
  of one session and answers three questions: which encodings exist (UTF-8,
  ISO-8859-1, ISO-8859-2, ISO-8859-15, Windows-1250, Windows-1251,
  Windows-1252, KOI8-R, CP437, CP850, with aliases such as `latin1`, `cp850`,
  `885915`; names are matched case and separator blind and an unknown name
  falls back to UTF-8), what a high byte decodes to, and which byte (if any) a
  codepoint encodes to. ISO-8859-1, ISO-8859-2, Windows-1252 and KOI8-R
  delegate to the `FXTextCodec` classes the library already had (their tables
  were checked against the reference mappings and round-trip exactly);
  ISO-8859-15, Windows-1250, Windows-1251, CP437 and CP850 carry a 128 entry
  table for 0x80..0xFF, with 0xFFFF for a byte the set leaves empty. Bytes
  below 0x80 are ASCII in every supported set and never reach the charset.
- `FXTerminalParser` takes an optional charset and has `setCharset()` plus a
  `flush()` that drops a partial escape or UTF-8 sequence. In a single byte
  encoding every byte above ASCII is decoded in the GROUND state, *before* the
  eight bit C1 forms, so 0x9B is a CSI sequence in UTF-8 and a character in
  CP437. `resetParser()` is now `flush()` plus the DEC character set reset.
- `FXTerminalScreen::putCell()`: a character of a single byte encoding is one
  cell, never wide and never combining, whatever the Unicode width table says
  about it (U+00AD, the soft hyphen, is zero width there and still takes a
  cell here). `putChar()` and `putCell()` share `putGlyph()`. This is the width
  rule of the plan: all 128 high bytes of every single byte encoding produce
  one cell each, and `terminalmodel` checks it for every set.
- `FXTerminal`: `setEncoding()`, `getEncoding()`, `getSupportedEncodingCount()`,
  `getSupportedEncodingName()`, `findEncoding()`; `sendText()` encodes keys,
  paste and IME text through the session charset and notifies
  `ID_ENCODING_LOSS` (as `SEL_CHANGED`) when a character had to become '?';
  `send()` still writes raw bytes for terminal responses. The parser and the
  screen are not reset by a switch, so drawn cells keep their codepoints and
  the child keeps running. A switch drops only a partial sequence.
- `foxterm`: the view menu has one `FXMenuRadio` per supported encoding,
  built from the static list; the update handler puts the check on the encoding
  of the active tab (a `SEL_UPDATE` handler must check and uncheck its own
  sender, a return value does not do it) and disables the entries when there is
  no tab. `onCmdEncoding()` switches and reports `Encoding: <name>` in the
  status line; `onCmdEncodingLoss()` reports the '?' substitution. `addTab()`
  applies the profile encoding before the child starts, and Duplicate keeps the
  live encoding of the session it copies. The profile dialog has an Encoding
  combo, `[PROFILE] encoding` is loaded, validated and saved, and both
  built-in profiles default to UTF-8.
- The run of one `drawText` call per equal-attribute span (visual step S1) was
  unsafe for any glyph whose advance is not the cell: `Cascadia Mono` advances
  U+FFFD fifteen pixels against an eight pixel cell, so a malformed byte
  shifted the rest of the line by one column. `FXTerminal::onPaint()` now draws
  such a cell on its own, clipped to its cells, like a fallback glyph, and
  caches the codepoints that need it (`oddadvance`, cleared when the font
  metrics change).

Gate: `scripts/gate-wayland-foxterm-encoding.sh`, four phases on headless sway
with `tools/wlxinject.c` and `scripts/wlclip.c`:

1. a child prints the byte 0xE4 on every line it reads; the byte is a
   replacement character in UTF-8, and one menu choice later the same byte is
   'ä' in ISO-8859-1 while the rows that were already on the screen stay
   pixel-identical; the difference between the two byte lines is confined to
   the eight pixel wide cell of the byte; switching back to UTF-8 draws the
   next line like the first one;
2. typing "zä" sends `7a c3 a4` in UTF-8 and `7a e4` in Latin-1, read back from
   the child with `od`, so the inverse mapping is checked on the wire and not
   on the screen;
3. pasting a CJK character into a Latin-1 session reaches the child as `3f`
   ('?') and the status line says so, while the same paste in UTF-8 sends
   `e4 b8 ad` and says nothing;
4. a profile with `encoding=ISO-8859-1` renders the byte exactly like the live
   switch did.

The gate reads the popup position of the encoding submenu from the Wayland
trace, so no menu coordinate is guessed, and it uses the status line as the
witness that a menu entry really ran.

Result: the gate passes, and the surrounding gates still pass on the same
build: `gate-wayland-foxterm`, `-metrics`, `-chrome` and `-context`, plus
`terminalmodel` and `pty`. The X11 build (`scripts/build-fox2-linux.sh`) is
clean and the new and changed library sources compile for Win32 with
`x86_64-w64-mingw32-g++`. Not run: an X11 runtime check of the encoding menu
(no X server in this checkout) and a real Windows or ConPTY run; on Windows the
PTY output is UTF-8 by construction (section 7.3), so a single byte encoding
there only affects how already decoded text is redrawn, which is why the
Windows profile default stays UTF-8.

Still missing from R20: a grapheme the set cannot express is reported once per
event, not once per character, and the status line of a lossy paste can be
covered by a later message. The child's locale is inherited by design (section
7.2), so a legacy profile does not tell the shell which charset to write.

## 14. Tests and verification

### 14.1 Model and parser tests (`terminalmodel`)

Cover at least:

1. Plain text, CR, LF, CRLF, backspace, tab stops and tab clearing.
2. Auto-wrap at the right margin; wrap-pending behavior; DECAWM off.
3. Cursor movement with and without margins; origin mode; save/restore.
4. Erase in display/line; insert/delete characters and lines; scroll up/down.
5. Scroll regions and reverse index.
6. SGR reset, bold, dim, italic, underline, blink, reverse, hidden, strike,
   overline; 8/16/256/truecolor; colon parameters.
7. Alternate screen entry/exit preserves the primary screen and scrollback.
8. Wide characters, combining marks, ZWJ clusters, malformed UTF-8 and C1
   bytes.
9. OSC title sanitizing, OSC 7 directory, OSC 8 hyperlink storage, OSC 52
   policy requests, oversized OSC/DCS strings.
10. Mouse mode changes, bracketed paste, focus reporting and DECCKM input
    mappings.
11. DA/CPR/DSR replies for both profiles.
12. Scrollback limit and memory trimming.
13. Resize reflow: soft wrap joins and splits, hard wrap stays.
14. VT100 profile ignores xterm-only modes and colors are reduced.
15. Invalid and unknown sequences never print their parameters.
16. The character encoding of a session (R20, N2): every supported name and
    alias resolves, an unknown name falls back to UTF-8, one sample byte per
    encoding decodes to its reference codepoint, every graphic high byte of
    every single byte encoding takes exactly one cell and encodes back to its
    own byte, an unmappable character becomes '?' on input, a live switch keeps
    drawn cells and decodes the next byte with the new set, and a switch drops
    a partial escape sequence.

Use table-driven cases in the test source. Keep a few recorded escape streams in
`fox2/tests/data/terminal/` with `.ans` input and expected text/attribute
snapshots. Do not require an external terminal emulator to run the test.

### 14.2 PTY smoke test (`pty`)

- The test binary with `--child` writes a known prompt/marker and reads from
  stdin.
- The parent starts the same binary through `FXPty`, waits for the marker,
  writes a line, checks the echo/output and resizes the PTY.
- The parent verifies the exit code and that the child is reaped.
- On Windows the same test runs through ConPTY; skip with a clear message when
  the OS is older than Windows 10 1809.

### 14.3 Fuzz and limits

- Add a mode to `terminalmodel` that feeds deterministic pseudorandom bytes
  (fixed seed) for a bounded number of iterations and asserts no crash, no
  unbounded allocation and no undefined state.
- Feed repeated CSI with 10000 parameters; verify the parameter cap.
- Feed a 1 MiB OSC/DCS string; verify it is discarded.
- Feed a 20 MiB output stream in a loop; verify scrollback and queue limits and
  that the process stays responsive.
- Run the fuzz mode under ASan/UBSan on Linux when available.

### 14.4 UI gates

Add gate scripts following the existing pattern. Each gate uses a fresh
temporary HOME (and APPDATA on Windows) so the FOX registry starts empty and
the test cannot overwrite a developer profile.

- `scripts/gate-terminal-x11.sh`: start `foxterm` with a deterministic test
  profile, run `printf` commands, type into the terminal, open two tabs, detach
  one, close it, capture screenshots under `screenshots/tests/fox2/x11/`.
- `scripts/gate-terminal-wayland.sh`: same under headless sway, with
  `FOX_WAYLAND_DUMP` frame capture. Verify tab strip, text rendering, resize,
  detach, and the old-window behavior after detach. Note that Wayland window
  placement is compositor-controlled and cross-window reattach is not part of
  this plan.
- Windows: a PowerShell capture/run script that starts `foxterm.exe`, uses the
  test profile, captures the window and checks that a detached window exists.
  If the host cannot run it, record that in the plan result and do not claim a
  runtime pass.

The gates assert:

- A known output string appears in the terminal.
- The character encoding menu decodes a byte differently, and encodes a typed
  or pasted character differently, without restarting the child
  (`scripts/gate-wayland-foxterm-encoding.sh` covers this).
- The tab title changes after an OSC title sequence.
- `Ctrl+Shift+T` creates a tab; `Ctrl+Shift+W` closes it.
- Detaching keeps the same process ID and the same screen contents.
- Resizing changes the reported `COLUMNS`/`LINES` in the child.
- Colors and font are stable across a theme change.

### 14.5 Transparency tests

Add these cases to `tests/raster` (no display required):

1. Opaque fill remains byte-identical: an opaque destination and an opaque
   source produce the same pixels as before.
2. `clearRectangleAlpha` writes premultiplied pixels for alpha 0, 128 and 255.
3. `blend()` over destination alpha 0, 128 and 255 produces the expected
   source-over result; over alpha 255 the result is byte-identical to the old
   blend.
4. An alpha image blit preserves transparent and semi-transparent pixels.
5. Opaque output after the raster change has 0 differing pixels from the old
   build in the existing `raster` comparison.

Add display gates:

- **Wayland**: set `transparencyMode=background` and `backgroundOpacity=50`.
  Dump the frame with `FOX_WAYLAND_DUMP_ALPHA`; assert the terminal cell
  background has alpha near 128, the tab strip and glyph body have alpha 255,
  and a fully transparent `backgroundOpacity=0` grid still has opaque text.
  For `window` mode, assert the whole surface alpha is near 128. Detach a tab
  and assert the new window has the same effective setting.
- **X11**: with a compositing manager, read `_NET_WM_WINDOW_OPACITY` for
  `window` mode and check it matches the requested percentage. Run
  `background` mode only when the capability query returns a 32-bit ARGB visual
  with XRender; otherwise assert the explicit downgrade to `window` and record
  the skip. Capture a screenshot over a known-color window to verify the
  background is translucent and the text is opaque.
- **Win32**: for `window` mode, call `GetLayeredWindowAttributes` and check the
  alpha value. For `background` mode, run the DWM capability test; if it passes,
  capture the window over a known-color window and check the grid alpha and
  opaque text; if it fails, assert the mode reports `window` and document the
  downgrade.
- **All backends**: start in `off` mode and compare against the pre-change
  screenshot or the opaque frame. The classic pixel gates must stay at 0
  differing pixels. Confirm that changing mode at runtime does not restart the
  PTY or change the child process ID.

The gate scripts may force a result with `FOX_TERMINAL_TRANSPARENCY` for test
repeatability, but production capability detection must still run.

### 14.6 Build matrix

| Target | Build | Runtime |
| --- | --- | --- |
| Linux/X11 | `./scripts/build-fox2-linux.sh` and CMake | Required |
| Linux/Wayland | `./scripts/build-fox2-wayland.sh` | Required |
| Windows/MinGW-w64 | `./scripts/build-fox2-mingw.sh` and the PowerShell script | Required on a Windows host; otherwise record as cross-build only |
| macOS/X11 | CMake or Autotools with XQuartz | Optional; compile check only on this machine |

The transparency gates require a compositor and, on Windows, DWM. A host that
cannot provide them records a skip with the reason; it does not count as a pass.
The shared `raster` alpha tests run everywhere, including console-only builds.

Run the existing `hello`, `button`, `console`, `tabbook`, `process`,
`consolemodel` and `raster` tests after the library change to catch regressions.

### 14.7 Performance and memory

- Target: 10 MiB/s of plain text output parsed and rendered without the UI
  blocking for more than 100 ms per turn.
- Target: typing latency under 16 ms on a 100x40 view.
- Target: default 10000-line scrollback with an 80-column screen stays below
  40 MiB of cell storage.
- Target: `background` transparency at 80x40 adds no more than 10% to terminal
  paint time compared with `off`; opacity changes are a one-time repaint.
- Measure with a release build and record the results in `docs/terminal.md`.
- Do not optimize before the correctness tests pass.

## 15. Security and robustness

The child process is untrusted. A terminal emulator is a parser for attacker
controlled bytes.

- All escape strings have hard length limits and are consumed, not printed.
- No OSC/DCS sequence may write a file, run a command, open a URL or write the
  clipboard without an explicit application policy.
- OSC 52 is off by default; clipboard reads are off by default.
- OSC 8 links are stored and displayed but are never opened automatically. The
  terminal context menu offers Copy link; opening URLs is not part of this
  plan. No OSC/DCS sequence may invoke a browser or the shell.
- Titles and working directories are sanitized for control characters and
  length. A title cannot contain a newline.
- Paste is treated as user input, not trusted text: multi-line paste warns,
  bracketed paste is used only on request, and no command is executed by the
  terminal itself.
- The PTY is a real process boundary. Do not use `system()`, `popen()` or a shell
  to launch a profile that specifies an executable and arguments.
- POSIX child gets its own session and controlling terminal. Close the master
  and slave descriptors in the right process; a leaked master keeps the child
  alive.
- Windows child is placed in a kill-on-close job. Closing a tab terminates the
  whole tree, not only the shell.
- Write queues are bounded. Backpressure is preferable to unbounded memory.
- No UI thread waits indefinitely for a child, a pipe or a thread. Every wait
  has a timeout and a fallback.
- Transparency reads no pixels from behind the window and captures no desktop.
  It is implemented only with the compositor or DWM, never with a root-window
  screenshot, color key or wallpaper copy.
- The parser must be safe with `-fsanitize=address,undefined` under the fuzz
  mode.

## 16. Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| ConPTY is missing on older Windows. | Detect the API, show a clear error, require Windows 10 1809+. Do not fall back to pipes. |
| Windows wait-handle limit with many tabs. | Cap 32 running sessions per process; fall back to a 16 ms poll timer when `addInput` fails. |
| Wayland pointer grab makes drag-out unreliable. | Always provide the context menu and Ctrl+Shift+M; cancel drag on lost grab. Placement is compositor-controlled. |
| macOS has no native FOX backend. | POSIX PTY code is portable; the application builds against X11/XQuartz. Do not claim Cocoa support. |
| Reflow is complex and can corrupt hard-wrapped output. | Reflow only rows marked soft-wrapped; add golden tests; keep the old primary screen untouched on alternate-screen resize. |
| Emoji and grapheme clusters vary by font and platform. | Use a width table plus grapheme pool; fall back to base codepoint when the pool is full; test common cases, not every Unicode sequence. |
| Color scheme changes should recolor existing text. | Store palette indices in cells; resolve to RGB at paint time. Truecolor cells stay RGB by definition. |
| Tabs are not restored after restart. | Document it. Restoring running sessions requires a server and is out of scope. |
| Detach reparenting can lose focus or leave an empty window. | Create the new window before reparenting, show it after the move, close the old window only after the new one has the tab. Test all empty/non-empty cases. |
| A profile command line can be misquoted. | Keep profile arguments as one string and use the platform's own quoting rules; `--execute` passes arguments separately and is the tested path. |
| X11 or Windows has no compositor, so `window`/`background` has no visible effect. | Query the capability, report the effective mode, fall back to `off`; never claim success from a screenshot that did not include a compositor. |
| X11 background mode needs a second visual and can break existing widgets. | Gate on a 32-bit ARGB visual plus XRender, recreate the shell before enabling, and fall back to `window` if the gate fails. |
| Win32 GDI child windows cannot preserve per-pixel alpha. | Treat `background` as capability-gated; if the DWM test cannot keep opaque child text, advertise `window` only and document it. |
| Antialiased glyph edges show halos over a transparent background. | Make `blend()` source-over correct for a transparent destination; test alpha 0, 128 and 255; keep opaque output byte-identical. |
| Wayland uses premultiplied ARGB and a straight-alpha mistake produces dark fringes. | Write premultiplied pixels in `clearRectangleAlpha` and the blit; verify with `FOX_WAYLAND_DUMP_ALPHA` and a checkerboard background. |
| Whole-window opacity makes text hard to read. | Default is `off`; `background` mode is the preferred readable mode; the settings dialog warns that `window` fades text. |
| Transparency changes invalidate the existing pixel gates. | Run the gates in `off` mode; assert 0 differing pixels against the pre-change build before enabling any alpha path. |

## 17. Documentation deliverables

- `docs/terminal.md`: user guide, keyboard table, profile/settings format,
  supported escape sequences, transparency modes and platform support,
  platform limitations, minimum Windows version, macOS/XQuartz note, test and
  performance results.
- `fox2/apps/foxterm/foxterm.1`: short manual page, installed with the other
  application man pages.
- Update `README.md` only with the build target and a one-line description;
  keep the detailed material in `docs/terminal.md`.
- Screenshots:
  - `screenshots/tests/fox2/x11/foxterm-tabs.png`
  - `screenshots/tests/fox2/x11/foxterm-detached.png`
  - `screenshots/tests/fox2/wayland/foxterm-tabs.png`
  - `screenshots/tests/fox2/wayland/foxterm-detached.png`
  - `screenshots/tests/fox2/win32/foxterm-tabs.png`
  - `screenshots/tests/fox2/win32/foxterm-detached.png`
  - `screenshots/tests/fox2/wayland/foxterm-transparent-bg.png`
  - `screenshots/tests/fox2/wayland/foxterm-transparent-window.png`
  - `screenshots/tests/fox2/x11/foxterm-transparent-window.png` (only when a
    compositing manager is used; record the skip otherwise)
  - `screenshots/tests/fox2/win32/foxterm-transparent-window.png`
  - `screenshots/tests/fox2/win32/foxterm-transparent-bg.png` (only when the
    DWM capability gate passes; record the downgrade otherwise)
- Record the final build and test result in this plan under a short
  "Implementation result" section, as the other plans do.

## 18. Acceptance summary

The work is complete when:

1. `foxterm` runs an interactive shell on Linux/X11, Linux/Wayland and Windows.
2. VT100 and xterm profiles are selectable per tab and behave as described in
   section 6.
3. The screen model handles the test matrix in section 14.1 and survives fuzz
   input.
4. Multiple tabs, overflow, close buttons, plus/drop-down, reorder, switching
   and titles work as described in section 10.
5. A running tab can be detached into its own window and keeps its process,
   screen and scrollback.
6. Keyboard, mouse, IME, clipboard, paste warning and search work.
7. Profiles, color schemes, transparency mode, background opacity and geometry
   survive a restart.
8. `window` transparency works on every backend that has a compositor/DWM;
   `background` transparency works on every backend that passes its capability
   gate, with documented fallback where it does not.
9. The existing library tests and the console example still build and pass;
   opaque `off` output remains pixel-identical.
10. The application has no platform conditionals; the platform code is isolated
    in the PTY, window opacity and alpha raster seams.
11. Documentation and screenshots are present, and the known limitations are
    stated without overclaiming platform support.
