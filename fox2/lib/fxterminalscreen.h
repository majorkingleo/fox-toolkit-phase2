/********************************************************************************
*                                                                               *
*                         T e r m i n a l   S c r e e n                         *
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
#ifndef FXTERMINALSCREEN_H
#define FXTERMINALSCREEN_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRING_H
#include "FXString.h"
#endif

#ifndef FXTERMINAL_H
#include "FXTerminal.h"
#endif

#include <vector>
#include <deque>


namespace FX {


/// Terminal cell attributes
enum {
  TERMATTR_BOLD            = 0x0001,   /// Bold or increased intensity
  TERMATTR_DIM             = 0x0002,   /// Decreased intensity
  TERMATTR_ITALIC          = 0x0004,   /// Italic
  TERMATTR_UNDERLINE       = 0x0008,   /// Underlined
  TERMATTR_BLINK           = 0x0010,   /// Blinking
  TERMATTR_REVERSE         = 0x0020,   /// Reverse video
  TERMATTR_HIDDEN          = 0x0040,   /// Concealed
  TERMATTR_STRIKE          = 0x0080,   /// Strike-through
  TERMATTR_OVERLINE        = 0x0100,   /// Overlined
  TERMATTR_UNDERLINE_MASK  = 0x0E00    /// Underline style bits
  };


/// Terminal modes
enum {
  TERMMODE_CURSORKEYS      = 0x0001,   /// Application cursor keys
  TERMMODE_ORIGIN          = 0x0002,   /// Origin mode
  TERMMODE_WRAP            = 0x0004,   /// Auto wrap
  TERMMODE_CURSORVISIBLE   = 0x0008,   /// Cursor visible
  TERMMODE_CURSORBLINK     = 0x0010,   /// Cursor blinks
  TERMMODE_INSERT          = 0x0020,   /// Insert mode
  TERMMODE_ALTSCREEN       = 0x0040,   /// Alternate screen active
  TERMMODE_APPKEYPAD       = 0x0080,   /// Application keypad
  TERMMODE_BRACKETEDPASTE  = 0x0100,   /// Bracketed paste
  TERMMODE_FOCUS           = 0x0200,   /// Focus reporting
  TERMMODE_MOUSEX10        = 0x0400,   /// X10 mouse reporting
  TERMMODE_MOUSEBUTTON     = 0x0800,   /// Button-event mouse reporting
  TERMMODE_MOUSEANY        = 0x1000,   /// Any-event mouse reporting
  TERMMODE_MOUSESGR        = 0x2000,   /// SGR mouse encoding
  TERMMODE_SYNCHRONIZED    = 0x4000,   /// Synchronized output
  TERMMODE_NEWLINE         = 0x8000    /// Newline mode (LNM)
  };


/// Cursor styles
enum {
  TERMCURSOR_DEFAULT       = 0,        /// Terminal default
  TERMCURSOR_BLOCK         = 1,        /// Filled block
  TERMCURSOR_BLOCKBLINK    = 2,        /// Blinking block
  TERMCURSOR_UNDERLINE     = 3,        /// Underline
  TERMCURSOR_UNDERLINEBLINK= 4,        /// Blinking underline
  TERMCURSOR_BAR           = 5,        /// Vertical bar
  TERMCURSOR_BARBLINK      = 6         /// Blinking bar
  };


/// Cell color encoding
enum {
  TERMCOLOR_DEFAULT_FG     = 0x00000100, /// Default foreground
  TERMCOLOR_DEFAULT_BG     = 0x00000101, /// Default background
  TERMCOLOR_RGB            = 0x01000000  /// Marker for a 24-bit RGB value
  };


/// A terminal screen cell
struct FXTerminalCell {
  FXuint  fg;                          // Palette index, default, or TERMCOLOR_RGB|rgb
  FXuint  bg;                          // Palette index, default, or TERMCOLOR_RGB|rgb
  FXuint  ch;                          // Base codepoint, or space for a blank
  FXushort attr;                       // TERMATTR_xxx
  FXushort width;                      // 0 continuation, 1 narrow, 2 wide
  FXuint  cluster;                     // 0, or 1-based index into the grapheme pool
  };


/// A terminal screen row
struct FXTerminalRow {
  std::vector<FXTerminalCell> cells;   // Cells in the row
  FXbool wrapped;                      // Row continues on the next row
  };


/**
* The FXTerminalScreen is the display-independent terminal state. It stores
* the primary and alternate screens, scrollback, cursor, margins, modes and
* the current rendition. The parser drives it through the public methods; the
* widget reads it for painting and selection.
*/
class FXAPI FXTerminalScreen {
protected:
  FXint                     kind;          // TERMINAL_VT100 or TERMINAL_XTERM
  FXint                     columns;       // Number of columns
  FXint                     rows;          // Number of screen rows
  std::vector<FXTerminalRow> lines;        // Current screen
  std::deque<FXTerminalRow>  scrollback;   // Retained primary screen rows
  std::vector<FXTerminalRow> primaryLines; // Saved primary screen
  FXint                     scrollcap;     // Maximum scrollback rows
  FXint                     cursorrow;     // Cursor row, zero based
  FXint                     cursorcol;     // Cursor column, zero based
  FXint                     saverow;       // Saved cursor row
  FXint                     savecol;       // Saved cursor column
  FXushort                  saveattr;      // Saved cursor attributes
  FXuint                    savefg;        // Saved cursor foreground
  FXuint                    savebg;        // Saved cursor background
  FXbool                    savewrap;      // Saved wrap-pending state
  FXint                     top;           // Top margin, inclusive
  FXint                     bottom;        // Bottom margin, inclusive
  FXint                     cursorstyle;   // TERMCURSOR_xxx
  FXushort                  curattr;       // Current rendition attributes
  FXuint                    curfg;         // Current foreground
  FXuint                    curbg;         // Current background
  FXuint                    modes;         // TERMMODE_xxx
  FXbool                    wrapPending;   // Cursor is at the right margin
  FXbool                    altActive;     // Alternate screen is active
  FXbool                    hasSaved;      // Saved cursor is valid
  FXbool                    lastValid;     // Last printed cell is valid
  FXint                     lastRow;       // Last printed row
  FXint                     lastCol;       // Last printed column
  FXint                     bellCount;     // Number of BEL events
  FXString                  title;         // Window title
  FXString                  workingdir;    // Reported working directory
  FXString                  hyperlink;     // Current OSC 8 hyperlink
  FXString                  clipboard;     // Pending OSC 52 request
  std::vector<FXString>      responses;    // Queued replies to the child
  std::vector<FXString>      clusters;     // Grapheme clusters
  std::vector<FXColor>       palette;      // 256 ANSI palette entries
  std::vector<FXuchar>       tabstops;     // Tab stops
  FXString                  sanitized;     // Reused sanitizer buffer
protected:
  FXTerminalRow blankRow() const;
  FXTerminalCell blankCell(FXbool erase) const;
  FXTerminalCell makeCell(FXwchar ch,FXuint fg,FXuint bg,FXushort attr,FXushort width=1) const;
  FXuint normalizeColor(FXuint color) const;
  FXint contentLength(const FXTerminalRow& row) const;
  FXint clusterIndex(const FXString& cluster);
  FXString sanitize(const FXString& text,FXint maxlength) const;
  void clearRows(std::vector<FXTerminalRow>& target) const;
  void scrollRegionUp(FXint first,FXint last,FXint count,FXbool history);
  void scrollRegionDown(FXint first,FXint last,FXint count);
  void resetTabStops();
  void clampCursor();
  void dumpRowToHistory(const FXTerminalRow& row);
  void reflowRows(const std::vector<FXTerminalRow>& source,std::vector<FXTerminalRow>& target,FXint width,FXint& cursorline,FXint& cursorcolumn);
  void resetState(FXint kind);
  void enterAltScreen(FXbool save);
  void leaveAltScreen();
private:
  FXTerminalScreen(const FXTerminalScreen&);
  FXTerminalScreen& operator=(const FXTerminalScreen&);
public:

  /// Construct an 80x24 terminal screen
  FXTerminalScreen();

  /// Reset the screen for the given terminal kind
  void reset(FXint kind);

  /// Reset the screen and choose the terminal kind
  void setTerminalKind(FXint kind);

  /// Return the terminal kind
  FXint getTerminalKind() const { return kind; }

  /// Resize the screen; soft-wrapped primary rows are reflowed
  void resize(FXint columns,FXint rows);

  /// Return the number of columns
  FXint getColumns() const { return columns; }

  /// Return the number of screen rows
  FXint getRows() const { return rows; }

  /// Return the cursor row
  FXint getCursorRow() const { return cursorrow; }

  /// Return the cursor column
  FXint getCursorColumn() const { return cursorcol; }

  /// Return true when the cursor is at the right margin and waiting to wrap
  FXbool getWrapPending() const { return wrapPending; }

  /// Return the cursor style
  FXint getCursorStyle() const { return cursorstyle; }

  /// Set the cursor style
  void setCursorStyle(FXint style);

  /// Return true if the alternate screen is active
  FXbool isAlternateScreen() const { return altActive; }

  /// Return the current attributes
  FXushort getAttributes() const { return curattr; }

  /// Return the current foreground color
  FXuint getForeground() const { return curfg; }

  /// Return the current background color
  FXuint getBackground() const { return curbg; }

  /// Set the current foreground color
  void setForeground(FXuint color);

  /// Set the current background color
  void setBackground(FXuint color);

  /// Set the current attributes
  void setAttributes(FXushort attr);

  /// Set one attribute on or off
  void setAttribute(FXushort attr,FXbool on);

  /// Reset the current rendition to the default
  void resetAttributes();

  /// Add a response for the child process
  void addResponse(const FXString& reply);

  /// Take one queued response; returns false when the queue is empty
  FXbool takeResponse(FXString& reply);

  /// Return the number of queued responses
  FXint getResponseCount() const { return (FXint)responses.size(); }

  /// Print one Unicode codepoint
  void putChar(FXwchar ch);

  /// Execute a C0 control
  void execute(FXuint ch);

  /// Move to the next line, scrolling when needed
  void lineFeed();

  /// Move to column zero
  void carriageReturn();

  /// Move one column left
  void backspace();

  /// Move to the next tab stop
  void tab();

  /// Move to the previous tab stop
  void backTab();

  /// Set a tab stop at the cursor column
  void setTabStop();

  /// Clear the tab stop at the cursor, or all tab stops when mode is 3
  void clearTabStops(FXint mode);

  /// Soft reset: rendition, modes and margins, without clearing the screen
  void softReset();

  /// Move the cursor by a relative amount
  void moveCursor(FXint drow,FXint dcol);

  /// Set the cursor position; origin mode and margins are honored
  void setCursor(FXint row,FXint col);

  /// Set the cursor row
  void setCursorRow(FXint row);

  /// Set the cursor column
  void setCursorColumn(FXint col);

  /// Set the scrolling region, zero-based and inclusive
  void setMargins(FXint top,FXint bottom);

  /// Return the top margin
  FXint getMarginTop() const { return top; }

  /// Return the bottom margin
  FXint getMarginBottom() const { return bottom; }

  /// Save the cursor and rendition
  void saveCursor();

  /// Restore the cursor and rendition
  void restoreCursor();

  /// Index: move down, scrolling if necessary
  void index();

  /// Reverse index: move up, scrolling if necessary
  void reverseIndex();

  /// Scroll the current region up
  void scrollUp(FXint count=1);

  /// Scroll the current region down
  void scrollDown(FXint count=1);

  /// Erase display: 0 cursor to end, 1 start to cursor, 2 all, 3 scrollback
  void eraseInDisplay(FXint mode);

  /// Erase line: 0 cursor to end, 1 start to cursor, 2 whole line
  void eraseInLine(FXint mode);

  /// Erase characters
  void eraseCharacters(FXint count);

  /// Insert blank characters
  void insertCharacters(FXint count);

  /// Delete characters
  void deleteCharacters(FXint count);

  /// Insert blank lines
  void insertLines(FXint count);

  /// Delete lines
  void deleteLines(FXint count);

  /// Set a private mode; returns true if the mode is recognized
  FXbool setPrivateMode(FXint mode,FXbool on);

  /// Get a private mode
  FXbool getPrivateMode(FXint mode) const;

  /// Set an ANSI mode; returns true if the mode is recognized
  FXbool setAnsiMode(FXint mode,FXbool on);

  /// Set application keypad mode
  void setApplicationKeypad(FXbool on);

  /// Return a TERMMODE_xxx bit
  FXbool getMode(FXuint mode) const { return (modes&mode)!=0; }

  /// Return the mouse tracking mode
  FXint getMouseMode() const;

  /// Set the window title, sanitized and bounded
  void setTitle(const FXString& text);

  /// Return the window title
  const FXString& getTitle() const { return title; }

  /// Set the reported working directory
  void setWorkingDirectory(const FXString& path);

  /// Return the reported working directory
  const FXString& getWorkingDirectory() const { return workingdir; }

  /// Set the current hyperlink
  void setHyperlink(const FXString& uri);

  /// Return the current hyperlink
  const FXString& getHyperlink() const { return hyperlink; }

  /// Set the pending OSC 52 request
  void setClipboardRequest(const FXString& request);

  /// Return and clear the pending OSC 52 request
  FXString takeClipboardRequest();

  /// Return the number of BEL events
  FXint getBellCount() const { return bellCount; }

  /// Clear the BEL counter
  void clearBell() { bellCount=0; }

  /// Set a palette entry
  void setPaletteColor(FXint index,FXColor color);

  /// Return a palette entry
  FXColor getPaletteColor(FXint index) const;

  /// Return a row text; trimmed removes trailing blanks
  FXString getRowText(FXint row,FXbool trimmed=true) const;

  /// Return a scrollback row text; trimmed removes trailing blanks
  FXString getScrollbackText(FXint row,FXbool trimmed=true) const;

  /// Return one cell; row and column are zero based
  const FXTerminalCell& getCell(FXint row,FXint col) const;

  /// Return the number of retained scrollback rows
  FXint getScrollbackCount() const { return (FXint)scrollback.size(); }

  /// Return the scrollback capacity
  FXint getScrollbackLines() const { return scrollcap; }

  /// Set the scrollback capacity, trimming the oldest rows
  void setScrollbackLines(FXint lines);

  /// Return true if the cell is a wide continuation
  FXbool isContinuation(FXint row,FXint col) const;

  /// Destructor
  virtual ~FXTerminalScreen();
  };

}

#endif
