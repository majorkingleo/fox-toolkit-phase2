/********************************************************************************
*                                                                               *
*                          Root window   ( W i n 3 2 )                          *
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
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRootWindow.h"

#if defined(WIN32)

/*
  Notes:

  - Windows root window: the desktop window and the virtual screen size.
*/

#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Returns device context
FXID FXRootWindow::GetDC() const {
  LockWindowUpdate(GetDesktopWindow());
  return GetDCEx(GetDesktopWindow(),nullptr,DCX_CACHE|DCX_LOCKWINDOWUPDATE);
  }


// Release DC
int FXRootWindow::ReleaseDC(FXID hdc) const {
  int status=::ReleaseDC(GetDesktopWindow(),(HDC)hdc);
  LockWindowUpdate(nullptr);
  return status;
  }


/*

// EnumDisplayMonitors callback routine to determine biggest screen size
static BOOL maxScreenSizeCallback(HMONITOR hmon,HDC hdc,LPRECT rect,LPARAM args){
  MONITORINFO moninfo; FXint w,h;
  moninfo.cbSize=sizeof(moninfo);
  GetMonitorInfo(hmon,&moninfo);
  w=moninfo.rcWork.right-moninfo.rcWork.left;
  h=moninfo.rcWork.bottom-moninfo.rcWork.top;
  if(((FXint*)args)[0]<w) ((FXint*)args)[0]=w;
  if(((FXint*)args)[1]<h) ((FXint*)args)[1]=h;
  return true;
  }


// Obtain largest dimensions of all screens (bounding box)
static FXbool getMaxScreenSize(FXint& width,FXint& height){
  FXint args[2]={0,0};
  if(EnumDisplayMonitors(nullptr,nullptr,(MONITORENUMPROC)maxScreenSizeCallback,(LPARAM)args)){
    width=args[0];
    height=args[1];
    return true;
    }
  return false;
  }

*/


// When created, create subwindows ONLY
void FXRootWindow::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

      // Initialize visual
      visual->create();


      // Get handle of desktop window
      xid=GetDesktopWindow();

      // Obtain desktop window size
      width=GetSystemMetrics(SM_CXVIRTUALSCREEN);
      height=GetSystemMetrics(SM_CYVIRTUALSCREEN);

      //HDC hdc=::GetDC((HWND)xid);
      //width=GetDeviceCaps(hdc,HORZRES);
      //height=GetDeviceCaps(hdc,VERTRES);
      //::ReleaseDC((HWND)xid,hdc);

      // Size of primary display screen
      //width=GetSystemMetrics(SM_CXSCREEN);
      //height=GetSystemMetrics(SM_CYSCREEN);

      // Size bigger than all display screens
      //getMaxScreenSize(width,height);

      // Store for xid to C++ object mapping
      getApp()->hash.insert((void*)xid,this);


      // Normally create children
      for(FXWindow *c=getFirst(); c; c=c->getNext()) c->create();
      }
    }
  }


// Get default width
FXint FXRootWindow::getDefaultWidth(){
//  HDC hdc=::GetDC(GetDesktopWindow());
//  FXint w=GetDeviceCaps(hdc,HORZRES);
//  ::ReleaseDC(GetDesktopWindow(),hdc);
//  return w;
  return GetSystemMetrics(SM_CXVIRTUALSCREEN);
  }

// Get default height
FXint FXRootWindow::getDefaultHeight(){
//  HDC hdc=::GetDC(GetDesktopWindow());
//  FXint h=GetDeviceCaps(hdc,VERTRES);
//  ::ReleaseDC(GetDesktopWindow(),hdc);
//  return h;
  return GetSystemMetrics(SM_CYVIRTUALSCREEN);
  }

}

#endif
