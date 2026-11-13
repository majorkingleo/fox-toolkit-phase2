/********************************************************************************
*                                                                               *
*                     D r a g   C o r n e r   ( W a y l a n d )                 *
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
#include "FXDragCorner.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Dragging the corner resizes the window the way it does on X11, but without
    the XOR outline: there is no overlay to draw it into.  The window follows
    the pointer when the button is released.
  - xdg_toplevel_resize() would hand the whole interaction to the compositor
    and is the better answer, but it needs the seat and the serial of the
    press, which arrive with the input phase; until then the corner keeps the
    core side of the drag.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create drag corner
void FXDragCorner::create(){
  FXWindow::create();
  ewmh=false;
  }


// Pressed LEFT button
long FXDragCorner::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint wx,wy;

  // The compositor drags the edge for us when it can; that keeps the layout of
  // the window and the pointer together and needs no outline
  if(fxwaylandStartResize(fxwayland(getApp()->getDisplay()),getShell()->id())) return 1;

  grab();
  xoff=width-event->win_x;
  yoff=height-event->win_y;
  translateCoordinatesTo(wx,wy,getShell(),event->win_x,event->win_y);
  oldw=wx+xoff;
  oldh=wy+yoff;
  flags|=FLAG_PRESSED;
  return 1;
  }

}

#endif
