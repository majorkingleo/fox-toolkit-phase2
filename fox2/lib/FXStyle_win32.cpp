/********************************************************************************
*                                                                               *
*                             Style   ( W i n 3 2 )                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXVisual.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXTheme.h"
#include "FXFrame.h"
#include "FXStyle.h"

#if defined(WIN32)

/*
  Notes:
  - Platform part of the style: the antialiased rounded rectangle cache.
*/

// Placeholder when antialiased compositing is unavailable
struct FXStyle::FXCornerMask {
  FXint         radius;
  FXCornerMask *next;
  };


using namespace FX;

/*******************************************************************************/

namespace FX {

// Drop cached chrome resources after a theme change
void FXStyle::invalidate(){
  masks=nullptr;
  }


// Probe whether the drawable supports antialiased compositing
FXbool FXStyle::hasXRender(FXDCWindow& dc){
  if(!xrenderChecked){
    xrenderChecked=true;
    }
  return xrender;
  }


// Return cached corner mask for the given radius
FXStyle::FXCornerMask* FXStyle::getMask(FXint radius){
  return nullptr;
  }


// Draw a check mark; GDI has no small AA primitive here, use the line primitives
void FXStyle::drawCheckMark(FXDCWindow& dc,FXint x,FXint y,FXint size,FXColor color){
  dc.setForeground(color);
  dc.setLineWidth(FXMAX(1,size/7));
  dc.drawLine(x+(size*22)/100,y+(size*50)/100,x+(size*43)/100,y+(size*72)/100);
  dc.drawLine(x+(size*43)/100,y+(size*72)/100,x+(size*78)/100,y+(size*28)/100);
  dc.setLineWidth(1);
  }


// Draw an antialiased rounded rectangle
// Margin a popup needs for its drop shadow.  Only a surface with an alpha
// channel can carry a translucent shadow, which the Win32 window does not have,
// so no margin is reserved and the popups keep their size
// Fill the outside of the rounded corners; these surfaces have no antialiased
// compositing for a corner shape, so the cut is hard edged
void FXStyle::cutRoundedCorners(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor color){
  fillOutsideCorners(dc,x,y,w,h,radius,color);
  }


FXint FXStyle::shadowMargin(FXdouble){
  return 0;
  }


// Draw the drop shadow of a popup; nothing to draw without an alpha surface
void FXStyle::drawShadow(FXDCWindow&,FXint,FXint,FXint,FXint,FXint){
  }


FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
  return false;
  }

}

#endif
