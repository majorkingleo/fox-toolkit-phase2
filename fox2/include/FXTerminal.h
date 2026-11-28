/********************************************************************************
*                                                                               *
*                         T e r m i n a l   W i d g e t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXTERMINAL_H
#define FXTERMINAL_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRING_H
#include "FXString.h"
#endif

namespace FX {
struct FXEvent;
}


#ifndef FXSCROLLAREA_H
#include "FXScrollArea.h"
#endif


namespace FX {


class FXFont;
class FXTerminalPrivate;


/// Terminal emulation kinds
enum {
  TERMINAL_VT100,
  TERMINAL_XTERM
  };


/// Description of the shell launched for a terminal session
struct FXAPI FXTerminalLaunch {
  FXString shell;                      // Executable path
  FXString arguments;                  // Platform-quoted arguments
  FXString directory;                  // Starting directory
  FXString term;                       // TERM value
  FXint    kind;                       // TERMINAL_VT100 or TERMINAL_XTERM
  FXint    columns;                    // Initial columns
  FXint    rows;                       // Initial rows
  };


/**
* A terminal view. The widget owns one pseudo terminal and displays the screen
* maintained by FXTerminalScreen. Keys and IME text go to the child; selection,
* copy, paste, scrolling, cursor blinking and mouse reporting are handled here.
* The widget does not display dialogs or decide process-close policy; it sends
* ID_EXITED, ID_BELL, ID_TITLE and ID_CLIPBOARD to its target as SEL_COMMAND,
* and ID_ENCODING_LOSS as SEL_CHANGED when a key or a paste had to reach the
* child as '?' because the encoding cannot express it.  A different selector
* type keeps the notification out of the id space the window uses for its own
* menu commands.  The effective encoding is the return value of setEncoding().
* A right press that does not belong to the child is forwarded to the target as
* SEL_RIGHTBUTTONPRESS, so the menu it posts can use the event's root position
* and no selector of the widget has to be invented for it.
*/
class FXAPI FXTerminal : public FXScrollArea {
  FXDECLARE(FXTerminal)
protected:
  FXTerminalPrivate *priv;             // Screen, parser and PTY state
protected:
  FXTerminal();
  virtual void moveContents(FXint x,FXint y) override;
  void updateNotifications();
  void removeWatches();
  void addWriteWatch();
public:
  enum {
    ID_TITLE=FXScrollArea::ID_LAST,
    ID_BELL,
    ID_EXITED,
    ID_CLIPBOARD,
    ID_TRANSPARENCY,
    ID_TERMINAL_SEARCH_CHANGED,
    ID_ENCODING_LOSS,
    ID_READ,
    ID_WRITE,
    ID_BLINK,
    ID_LAST
    };
public:

  /// Construct a terminal view; explicit margins replace the theme padding,
  /// all zero selects the theme padding on all four sides
  FXTerminal(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0);

  /// Create server-side resources
  virtual void create() override;

  /// Detach server-side resources
  virtual void detach() override;

  /// Layout the view and size the terminal grid
  virtual void layout() override;

  /// Return default width
  virtual FXint getDefaultWidth() override;

  /// Return default height
  virtual FXint getDefaultHeight() override;

  /// Return content width
  virtual FXint getContentWidth() override;

  /// Return content height
  virtual FXint getContentHeight() override;

  /// Return true because a terminal can receive focus
  virtual FXbool canFocus() const override;

  /// Re-apply the theme font, padding and colors
  virtual void applyTheme(FXTheme* theme) override;

  /// Change the padding on all four sides; negative restores the theme padding
  void setPadding(FXint pad);

  /// Return the padding; -1 when the constructor gave explicit margins
  FXint getPadding() const;

  /// Start a shell on a new pseudo terminal
  FXbool start(const FXTerminalLaunch& launch);

  /// Start a shell and report a startup error
  FXbool start(const FXTerminalLaunch& launch,FXString& error);

  /// Signal the child
  void terminate(FXbool force=false);

  /// Return true while the child runs
  FXbool isRunning();

  /// Return the child exit code, or -1
  FXint getExitCode();

  /// Return the child process id
  FXint getProcessID() const;

  /// Send raw bytes to the child
  FXbool send(const FXString& bytes);

  /// Send UTF-8 text to the child, encoded for the session; characters the
  /// encoding cannot express are sent as '?' and reported as ID_ENCODING_LOSS
  FXbool sendText(const FXString& text);

  /// Parse bytes as if they came from the child
  void feed(const FXString& bytes);

  /// Return the window title
  FXString getTitle() const;

  /// Return the reported working directory
  FXString getWorkingDirectory() const;

  /// Return the TERM value
  FXString getTerm() const;

  /// Select the character encoding of this session by name or alias; returns
  /// the canonical name that is in effect, which is UTF-8 for an unknown name
  FXString setEncoding(const FXString& name);

  /// Return the character encoding of this session
  FXString getEncoding() const;

  /// Return the number of supported character encodings; index zero is UTF-8
  static FXint getSupportedEncodingCount();

  /// Return the canonical name of a supported character encoding
  static FXString getSupportedEncodingName(FXint index);

  /// Return the index of an encoding name or alias; -1 when unsupported
  static FXint findEncoding(const FXString& name);

  /// Return the terminal kind
  FXint getTerminalKind() const;

  /// Return the number of columns
  FXint getColumns() const;

  /// Return the number of rows
  FXint getRows() const;

  /// Return the cursor row
  FXint getCursorRow() const;

  /// Return the cursor column
  FXint getCursorColumn() const;

  /// Return retained scrollback lines
  FXint getScrollbackLines() const;

  /// Return the number of retained scrollback rows
  FXint getScrollbackCount() const;

  /// Change retained scrollback lines
  void setScrollbackLines(FXint lines);

  /// Change the cursor style
  void setCursorStyle(FXint style);

  /// Follow new output when the view is at the bottom
  void setFollowOutput(FXbool on);

  /// Return true when new output is followed
  FXbool isFollowOutput() const;

  /// Scroll to the bottom and follow new output
  void scrollToEnd();

  /// Change the terminal font
  void setFont(FXFont* fnt);

  /// Return the terminal font
  FXFont* getFont() const;

  /// Return selected text
  FXString getSelectedText() const;

  /// Select all retained text
  void selectAll();

  /// Copy the selection to the clipboard
  FXbool copySelection();

  /// Paste clipboard text to the child
  FXbool pasteClipboard();

  /// Return true when a selection exists, so a menu can enable Copy
  FXbool hasSelection() const;

  /// Drop the retained scrollback rows, keeping the screen
  void clearScrollback();

  /// Reset the emulator: clear screen and scrollback, reset modes and cursor
  void reset();

  /// Take the pending OSC 52 clipboard request
  FXString takeClipboardRequest();

  /// Find text in the retained screen and scrollback
  FXint findText(const FXString& pattern,FXuint flags);

  /// Select and reveal the next match
  FXbool findNext();

  /// Select and reveal the previous match
  FXbool findPrev();

  /// Clear find state
  void clearFind();

  /// Return find match count
  FXint getFindCount() const;

  /// Handle repaint
  long onPaint(FXObject*,FXSelector,void*);

  /// Handle key press
  long onKeyPress(FXObject*,FXSelector,void*);

  /// Handle left mouse press
  long onLeftBtnPress(FXObject*,FXSelector,void*);

  /// Handle left mouse release
  long onLeftBtnRelease(FXObject*,FXSelector,void*);

  /// Handle right mouse press; reports to the child or forwards the event
  long onRightBtnPress(FXObject*,FXSelector,void*);

  /// Handle right mouse release
  long onRightBtnRelease(FXObject*,FXSelector,void*);

  /// Handle mouse motion
  long onMotion(FXObject*,FXSelector,void*);

  /// Handle mouse wheel
  long onMouseWheel(FXObject*,FXSelector,void*);

  /// Handle vertical scrollbar changes
  long onVScrollerChanged(FXObject*,FXSelector,void*);

  /// Handle vertical scrollbar drags
  long onVScrollerDragged(FXObject*,FXSelector,void*);

  /// Handle focus in
  long onFocusIn(FXObject*,FXSelector,void*);

  /// Handle focus out
  long onFocusOut(FXObject*,FXSelector,void*);

  /// Position the input method spot
  long onIMEStart(FXObject*,FXSelector,void*);

  /// Handle clipboard requests
  long onClipboardRequest(FXObject*,FXSelector,void*);

  /// Handle clipboard ownership loss
  long onClipboardLost(FXObject*,FXSelector,void*);

  /// Handle primary selection requests
  long onSelectionRequest(FXObject*,FXSelector,void*);

  /// Handle primary selection loss
  long onSelectionLost(FXObject*,FXSelector,void*);

  /// Read from the PTY
  long onRead(FXObject*,FXSelector,void*);

  /// Flush queued input
  long onWrite(FXObject*,FXSelector,void*);

  /// Cursor blink timeout
  long onBlink(FXObject*,FXSelector,void*);

  /// Destructor
  virtual ~FXTerminal();
  };

}

#endif
