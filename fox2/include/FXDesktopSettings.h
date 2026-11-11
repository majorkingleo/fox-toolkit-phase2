/********************************************************************************
*                                                                              *
*              D e s k t o p   S e t t i n g s   (a d a p t e r)               *
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
#ifndef FXDESKTOPSETTINGS_H
#define FXDESKTOPSETTINGS_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

namespace FX {


class FXString;


/// Color scheme preferences found on the desktop
enum {
  SCHEME_UNKNOWN,         /// No preference was found
  SCHEME_LIGHT,           /// Light interface preferred
  SCHEME_DARK             /// Dark interface preferred
  };


/**
* Read the desktop's light/dark preference without depending on a desktop
* library.  The checks run from cheapest to most expensive and all of them are
* optional: an unknown result simply leaves the application's own theme alone.
*/
class FXAPI FXDesktopSettings {
public:

  /// Return SCHEME_DARK, SCHEME_LIGHT or SCHEME_UNKNOWN for the current desktop
  static FXint getColorScheme();

  /// Return the name of the color scheme setting that decided it, for diagnostics
  static FXString getColorSchemeName();
  };

}

#endif
