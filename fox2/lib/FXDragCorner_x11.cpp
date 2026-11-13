/********************************************************************************
*                                                                               *
*                            Drag corner   ( X 1 1 )                            *
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

#if !defined(WIN32)

/*
  Notes:

  - X11 drag corner: optional EWMH _NET_WM_MOVERESIZE, else an XOR rectangle.
*/

#define DISPLAY(app) ((Display*)((app)->getDisplay()))

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create drag corner
void FXDragCorner::create(){
  FXWindow::create();
  unsigned long n,i; Atom type; unsigned char *prop; int format;
  if(Success==XGetWindowProperty(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),getApp()->wmNetSupported,0,2048,False,XA_ATOM,&type,&format,&n,&i,&prop)){
    for(i=0; i<n; i++){
      if(((Atom*)prop)[i]==getApp()->wmNetMoveResize){ ewmh=true; break; }
      }
    XFree(prop);
    }
  }


// Pressed LEFT button
long FXDragCorner::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(ewmh){
    XClientMessageEvent ev;
    ev.type=ClientMessage;
    ev.display=DISPLAY(getApp());
    ev.window=getShell()->id();
    ev.message_type=getApp()->wmNetMoveResize;
    ev.format=32;
    ev.data.l[0]=event->root_x;
    ev.data.l[1]=event->root_y;
    ev.data.l[2]=4;                // Bottom right
    ev.data.l[3]=LEFTBUTTON;
    ev.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,(SubstructureRedirectMask|SubstructureNotifyMask),(XEvent*)&ev);
    ungrab();
    return 1;
    }
  FXDCWindow dc(getRoot());
  FXint xx,yy,wx,wy;
  grab();
  xoff=width-event->win_x;
  yoff=height-event->win_y;
  translateCoordinatesTo(wx,wy,getShell(),event->win_x,event->win_y);
  oldw=wx+xoff;
  oldh=wy+yoff;
  dc.clipChildren(false);
  dc.setFunction(BLT_SRC_XOR_DST);
  dc.setForeground(FXRGB(255,255,255));
  getShell()->translateCoordinatesTo(xx,yy,getRoot(),0,0);
  dc.drawRectangle(xx,yy,oldw,oldh);
  flags|=FLAG_PRESSED;
  return 1;
  }

}

#endif
