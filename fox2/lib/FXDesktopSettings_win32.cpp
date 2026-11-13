/********************************************************************************
*                                                                               *
*                       Desktop settings   ( W i n 3 2 )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXSystem.h"
#include "FXDesktopSettings.h"
#include "fxpriv.h"

#if defined(WIN32)

/*
  Notes:

  - Windows desktop settings: no extra color scheme source.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// No Windows specific source of the color scheme is consulted yet
FXint fxdesktopcolorscheme(const FXchar*& name){
  name=nullptr;
  return SCHEME_UNKNOWN;
  }

}

#endif
