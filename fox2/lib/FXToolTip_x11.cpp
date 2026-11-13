/********************************************************************************
*                                                                               *
*                             Tool tip   ( X 1 1 )                              *
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
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXToolTip.h"
#include "fxpriv.h"

#if !defined(WIN32)

/*
  Notes:

  - X11 tool tip: placement within the root window.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Place the tool tip
void FXToolTip::place(FXint x,FXint y){
  FXint rx,ry,rw,rh,px,py,w,h;
  w=getDefaultWidth();
  h=getDefaultHeight();
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();
  px=x+16-w/3;
  py=y+20;
  if(px<rx) px=rx;
  if(py<ry) py=ry;
  if(px+w>rx+rw) px=rx+rw-w;
  if(py+h+50>ry+rh){ py=y-h-10; }
  position(px,py,w,h);
  }

}

#endif
