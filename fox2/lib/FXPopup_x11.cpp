/********************************************************************************
*                                                                               *
*                               Popup   ( X 1 1 )                               *
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
#include "FXPopup.h"
#include "fxpriv.h"

#if !defined(WIN32)

/*
  Notes:

  - X11 popup: placement within the root window.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Popup the menu at some location
void FXPopup::popup(FXWindow* grabto,FXint x,FXint y,FXint w,FXint h){
  FXint rx,ry,rw,rh;
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();
  FXTRACE(TOPIC_DETAIL,"%s::popup %p\n",getClassName(),this);
  grabowner=grabto;
  if((options&POPUP_SHRINKWRAP) || w<=1) w=getDefaultWidth();
  if((options&POPUP_SHRINKWRAP) || h<=1) h=getDefaultHeight();
  if(x+w>rx+rw) x=rx+rw-w;
  if(y+h>ry+rh) y=ry+rh-h;
  if(x<rx) x=rx;
  if(y<ry) y=ry;
  position(x,y,w,h);
  show();
  raise();
  setFocus();
  if(!grabowner) grab();
  }



// X11 has no window classes
const void* FXPopup::GetClass() const {
  return nullptr;
  }

}

#endif
