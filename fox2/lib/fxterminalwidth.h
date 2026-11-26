/********************************************************************************
*                                                                               *
*                  T e r m i n a l   C h a r a c t e r   W i d t h              *
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
#ifndef FXTERMINALWIDTH_H
#define FXTERMINALWIDTH_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

namespace FX {


/// Return the number of terminal cells a codepoint occupies; zero means combining
extern FXAPI FXint fxterminalCharWidth(FXwchar ch);

/// Return true if the codepoint combines with the preceding cell
extern FXAPI FXbool fxterminalIsCombining(FXwchar ch);

}

#endif
