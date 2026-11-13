/********************************************************************************
*                                                                               *
*                            Slider   ( W i n 3 2 )                             *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXSlider.h"

#if defined(WIN32)

/*
  Notes:

  - Win32 slider head geometry.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Draw lower edges of right pointing slider head
void FXSlider::drawArrowRightLower(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint m){
  dc.setForeground(shadowColor);
  dc.drawLine(x+w-1,y+h-m-2,x+w-m-2,y+h-1);
  dc.drawLine(x+1,y+h-2,x+w-m-1,y+h-2);
  dc.setForeground(borderColor);
  dc.drawLine(x+w,y+h-m-2,x+w-m-1,y+h-1);
  dc.drawLine(x,y+h-1,x+w-m-1,y+h-1);
  }

}

#endif
