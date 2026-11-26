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

}

#endif
