/********************************************************************************
*                                                                               *
*                          P o p u p   ( W a y l a n d )                        *
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

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A popup is placed within the screen area the way the X11 module does it:
    the compositor decides where the surface really ends up, so the placement
    here only feeds the virtual window table and the core.
  - The grab of a menu is implicit: the pointer stays with the popup surface
    that received the press.  Until the popup role arrives in phase 7 a popup
    is a plain toplevel, so it is placed like a small top window.
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


// Wayland has no window classes
const void* FXPopup::GetClass() const {
  return nullptr;
  }

}

#endif
