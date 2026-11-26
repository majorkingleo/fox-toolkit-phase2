/********************************************************************************
*                                                                               *
*                         T e r m i n a l   P a r s e r                         *
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
#ifndef FXTERMINALPARSER_H
#define FXTERMINALPARSER_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRING_H
#include "FXString.h"
#endif

#ifndef FXTERMINALSCREEN_H
#include "fxterminalscreen.h"
#endif


namespace FX {


/**
* The FXTerminalParser consumes UTF-8 bytes and ANSI/VT100/xterm escape
* sequences and dispatches the resulting operations to an FXTerminalScreen.
* It is incremental: a sequence may be split across any number of feed()
* calls. All buffers and parameter counts are bounded.
*/
class FXAPI FXTerminalParser {
protected:
  enum {
    PARSER_GROUND,
    PARSER_ESCAPE,
    PARSER_CSI,
    PARSER_STRING,
    PARSER_STRING_ESC,
    PARSER_CHARSET,
    PARSER_HASH,
    PARSER_UTF8
    };
protected:
  FXTerminalScreen& screen;       // Screen receiving the operations
  FXint             state;        // Parser state
  FXString          csi;          // Current CSI parameters
  FXString          string;       // Current OSC/DCS string
  FXString          intermediates; // CSI intermediate bytes
  FXchar            privateprefix; // Private CSI prefix, or zero
  FXchar            stringkind;   // ']', 'P', '^' or '_'
  FXbool            csiOverflow;  // CSI parameters were discarded
  FXbool            stringOverflow; // String payload was discarded
  FXint             charsetTarget; // G0..G3 selector
  FXchar            charsets[4];  // G0..G3 character sets
  FXint             gl;           // Active GL set
  FXint             utf8need;     // Remaining bytes in the UTF-8 sequence
  FXint             utf8have;     // Bytes collected so far
  FXwchar           utf8code;     // Codepoint being decoded
protected:
  FXint param(FXint index,FXint def) const;
  FXint paramCount() const;
  FXwchar mapCharset(FXwchar ch) const;
  void putCodepoint(FXwchar ch);
  void resetParser();
  void dispatchCsi(FXchar final);
  void dispatchOsc();
  void parseSgr(const FXString& text);
  FXColor parseColor(const FXString& text) const;
  void executeByte(FXuchar ch);
private:
  FXTerminalParser(const FXTerminalParser&);
  FXTerminalParser& operator=(const FXTerminalParser&);
public:

  /// Construct a parser writing to the given screen
  FXTerminalParser(FXTerminalScreen& screen);

  /// Reset the incremental parser state, not the screen
  void reset();

  /// Feed a block of bytes
  void feed(const FXchar* data,FXint length);

  /// Feed a string
  void feed(const FXString& text);

  /// Destructor
  virtual ~FXTerminalParser();
  };

}

#endif
