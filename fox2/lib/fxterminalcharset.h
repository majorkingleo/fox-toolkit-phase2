/********************************************************************************
*                                                                               *
*                    T e r m i n a l   C h a r s e t                            *
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
#ifndef FXTERMINALCHARSET_H
#define FXTERMINALCHARSET_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRING_H
#include "FXString.h"
#endif

namespace FX {

class FXTextCodec;


/**
* The character encoding of one terminal session.  UTF-8 is the default and the
* only multi-byte encoding: every other supported encoding is single byte, so
* each byte 0x80..0xFF decodes to exactly one codepoint and occupies exactly
* one cell.  The single byte tables are taken from the FXTextCodec classes the
* library already has (ISO-8859-1, ISO-8859-2, Windows-1252, KOI8-R) or from a
* small built-in table (ISO-8859-15, Windows-1250, Windows-1251, CP437, CP850).
*
* The charset never touches the screen; FXTerminalParser decodes output bytes
* through it and FXTerminal maps keys, pasted text and IME text back through
* encode().  A byte or codepoint the encoding cannot express becomes '?'.
*/
class FXAPI FXTerminalCharset {
protected:
  FXint current;                // Index of the active encoding
protected:
  static FXTextCodec* codec(FXint index);
  static const FXushort* table(FXint index);
private:
  FXTerminalCharset(const FXTerminalCharset&);
  FXTerminalCharset& operator=(const FXTerminalCharset&);
public:

  /// Construct a UTF-8 charset
  FXTerminalCharset();

  /// Return the number of supported encodings; index zero is UTF-8
  static FXint count();

  /// Return the canonical name of a supported encoding
  static const FXchar* name(FXint index);

  /// Return the index of an encoding name or alias; -1 when it is unsupported
  static FXint find(const FXString& name);

  /// Select an encoding by name or alias; returns the canonical name selected
  FXString setName(const FXString& name);

  /// Return the canonical name of the active encoding
  const FXchar* getName() const;

  /// Return the index of the active encoding
  FXint getIndex() const;

  /// Return true when the active encoding is UTF-8
  FXbool isUTF8() const;

  /// Decode one byte 0x80..0xFF of a single byte encoding to a codepoint
  FXwchar decode(FXuchar byte) const;

  /// Encode one codepoint; returns false when the encoding has no byte for it
  FXbool encode(FXwchar ch,FXchar& out) const;

  /// Encode UTF-8 text; an unmappable character becomes '?' and sets lossy
  FXString encode(const FXString& utf8,FXbool& lossy) const;
  };

}

#endif
