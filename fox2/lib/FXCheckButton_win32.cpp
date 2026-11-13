/********************************************************************************
*                                                                               *
*                         Check Button   ( W i n 3 2 )                          *
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
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXCheckButton.h"

#if defined(WIN32)

/*
  Notes:

  - Win32 check mark geometry.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Draw check mark
void FXCheckButton::drawCheckMark(FXDCWindow& dc,FXint ix,FXint iy){
  FXSegment seg[6];
  seg[0].x1=3+ix; seg[0].y1=5+iy; seg[0].x2=5+ix; seg[0].y2=7+iy;
  seg[1].x1=3+ix; seg[1].y1=6+iy; seg[1].x2=5+ix; seg[1].y2=8+iy;
  seg[2].x1=3+ix; seg[2].y1=7+iy; seg[2].x2=5+ix; seg[2].y2=9+iy;
  seg[3].x1=5+ix; seg[3].y1=7+iy; seg[3].x2=10+ix; seg[3].y2=2+iy;
  seg[4].x1=5+ix; seg[4].y1=8+iy; seg[4].x2=10+ix; seg[4].y2=3+iy;
  seg[5].x1=5+ix; seg[5].y1=9+iy; seg[5].x2=10+ix; seg[5].y2=4+iy;
  dc.drawLineSegments(seg,6);
  }

}

#endif
