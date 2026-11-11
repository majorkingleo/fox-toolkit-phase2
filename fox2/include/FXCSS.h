/********************************************************************************
*                                                                              *
*                            C S S   P a r s e r                               *
*                                                                              *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                              *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXCSS_H
#define FXCSS_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRINGDICTIONARY_H
#include "FXStringDictionary.h"
#endif

namespace FX {


class FXString;
class FXTheme;


/**
* A small CSS reader for importing GTK themes.  It understands the subset of
* the format that GTK actually uses for colors and metrics: comments, quoted
* strings, @define-color with indirections, selector lists with pseudo
* classes, and declarations with px/mm/pt lengths.  The same reader serves
* the fxthemeimport tool and the runtime loader.
*/
class FXAPI FXCSS {
protected:
  FXStringDictionary colors;      // Named colors from @define-color and gtk-color-scheme
  FXStringDictionary rules;       // Declarations, keyed by selector
  FXString    source;           // File the theme was read from
  FXint       warnings;         // Number of unsupported constructs seen
  FXint       ruleno;           // Number of rules parsed
protected:
  FXColor lookupColor(const FXString& name,FXint depth) const;
  void parseDeclarations(const FXString& selector,const FXString& body);
  void parseBlock(const FXString& text,FXint& pos);
  void parseDefine(const FXString& text,FXint& pos);
  FXbool addColor(const FXString& name,const FXString& value);
  void warn(const FXchar* format,...) const;
public:

  /// Construct reader
  FXCSS();

  /// Read a GTK 3/4 style sheet
  FXbool loadCSS(const FXString& path);

  /// Read a GTK 2 gtkrc
  FXbool loadGtkrc(const FXString& path);

  /// Read either format, picked by the file name
  FXbool load(const FXString& path);

  /// Return true if the given color name was defined
  FXbool hasColor(const FXString& name) const;

  /// Return named color, or zero if it is unknown
  FXColor color(const FXString& name) const;

  /// Return the value of the first declaration of the given property in a rule whose selector contains match
  FXString property(const FXString& match,const FXString& prop) const;

  /// Return a length declaration converted to pixels, or def if it is missing
  FXint length(const FXString& match,const FXString& prop,FXint def) const;

  /// Map the GTK colors and metrics onto the given theme
  void apply(FXTheme& theme) const;

  /// Number of unsupported constructs encountered
  FXint getWarnings() const { return warnings; }

  /// Number of rules parsed
  FXint getRules() const { return ruleno; }

  /// File the theme was read from
  const FXString& getSource() const { return source; }

  /// Destructor
  ~FXCSS();
  };

}

#endif
