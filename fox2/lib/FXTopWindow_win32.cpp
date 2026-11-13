/********************************************************************************
*                                                                               *
*      T o p   L e v e l   W i n d o w   F u n c t i o n s   ( W i n 3 2 )      *
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
#include "fxchar.h"
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
#include "FXAccelTable.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXTopWindow.h"
#include "FXMainWindow.h"
#include "FXToolBar.h"
#include "FXToolBarGrip.h"
#include "FXProcess.h"
#include "FXSystem.h"
#include "FX88591Codec.h"
#include "fxpriv.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXTopWindow members which create, show, decorate, stack and size the native top level window; the layout and the common accessors stay in FXTopWindow.cpp.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

const void* FXTopWindow::GetClass() const { return TEXT("FXTopWindow"); }


// Create window
void FXTopWindow::create(){
  FXShell::create();

  // Create icons
  if(icon) icon->create();
  if(miniIcon) miniIcon->create();

  if(xid){
    if(getApp()->isInitialized()){

      // Set title
      settitle();

      // Set decorations
      setdecorations();

      // Set icon for X-Windows
      seticons();

      // Only shrinkable; size may not be above default size
      if((options&DECOR_SHRINKABLE) && !(options&DECOR_STRETCHABLE)){
        if(width>getDefaultWidth()) width=getDefaultWidth();
        if(height>getDefaultHeight()) height=getDefaultHeight();
        }

      // Only stretchable; size may not be below default size
      else if((options&DECOR_STRETCHABLE) && !(options&DECOR_SHRINKABLE)){
        if(width<getDefaultWidth()) width=getDefaultWidth();
        if(height<getDefaultHeight()) height=getDefaultHeight();
        }

      RECT rect;
      SetRect(&rect,xpos,ypos,xpos+width,ypos+height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOZORDER|SWP_NOOWNERZORDER);
      }
    }
  }


// Destroy window
void FXTopWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      HICON icold;
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,0))!=0){
        DestroyIcon(icold);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,0))!=0){
        DestroyIcon(icold);
        }
      }
    }
  FXShell::destroy();
  }


// Focus to this toplevel window
void FXTopWindow::setFocus(){
  FXShell::setFocus();
  if(xid){
    //SetFocus((HWND)xid);
    SetActiveWindow((HWND)xid);
    }
  }


// Focus away from this toplevel window
void FXTopWindow::killFocus(){
  FXShell::killFocus();
  if(xid){
/*
    if(GetFocus()==(HWND)xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE(TOPIC_DETAIL,"focus back to owner\n");
        SetFocus((HWND)getOwner()->id());
        }
      else{
        FXTRACE(TOPIC_DETAIL,"focus back to NULL\n");
        SetFocus((HWND)nullptr);
        }
      }
*/
    if(GetActiveWindow()==(HWND)xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE(TOPIC_DETAIL,"focus back to owner\n");
        SetActiveWindow((HWND)getOwner()->getShell()->id());        // Fix from Sander
        //SetForegroundWindow((HWND)getOwner()->getShell()->id());
        }
      }
    }
  }


// Hide window
void FXTopWindow::hide(){
  if(flags&FLAG_SHOWN){
    killFocus();
    flags&=~FLAG_SHOWN;
    if(xid){
//      if(getOwner() && getOwner()->id()){
//        SetActiveWindow((HWND)getOwner()->getShell()->id());  // Fix from Sander
//        }
      ShowWindow((HWND)xid,SW_HIDE);
      }
    }
  }


// Raise and make foreground window
void FXTopWindow::raise(){
  FXShell::raise();
  if(xid){
//    SetForegroundWindow((HWND)xid);
    SetActiveWindow((HWND)xid);
    }
  }


// Lower
void FXTopWindow::lower(){
  FXShell::lower();
  }


// Position the window based on placement
void FXTopWindow::place(FXuint placement){
  FXint rx,ry,rw,rh,ox,oy,ow,oh,wx,wy,ww,wh,x,y;
  FXuint state;
  FXWindow *over;

  // Default placement:- leave it where it was
  wx=getX();
  wy=getY();
  ww=getWidth();
  wh=getHeight();

  // Get root window size
  RECT rect;
  MONITORINFO minfo;
  HMONITOR monitor;

  // Use mouse position to select screen
  if(placement!=PLACEMENT_OWNER){
    getRoot()->getCursorPosition(x,y,state);
    rect.left=x;
    rect.right=x+1;
    rect.top=y;
    rect.bottom=y+1;
    }

  // Use owner to select screen
  else{
    over=getOwner()?getOwner():getRoot();
    over->translateCoordinatesTo(ox,oy,getRoot(),0,0);
    ow=over->getWidth();
    oh=over->getHeight();
    rect.left=ox;
    rect.right=ox+ow;
    rect.top=oy;
    rect.bottom=oy+oh;
    }

  // Get monitor info if we have this API
  monitor=MonitorFromRect(&rect,MONITOR_DEFAULTTOPRIMARY);
  if(monitor){
    clearElms(&minfo,1);
    minfo.cbSize=sizeof(minfo);
    GetMonitorInfo(monitor,&minfo);
    rx=minfo.rcWork.left;
    ry=minfo.rcWork.top;
    rw=minfo.rcWork.right-minfo.rcWork.left;
    rh=minfo.rcWork.bottom-minfo.rcWork.top;
    }

  // Otherwise use the work-area
  else{
    SystemParametersInfo(SPI_GETWORKAREA,sizeof(RECT),&rect,0);
    rx=rect.left;
    ry=rect.top;
    rw=rect.right-rect.left;
    rh=rect.bottom-rect.top;
    }

  // Placement policy
  switch(placement){

    // Place such that it contains the cursor
    case PLACEMENT_CURSOR:

      // Get the owner
      over=getOwner()?getOwner():getRoot();

      // Get owner window size
      ow=over->getWidth();
      oh=over->getHeight();

      // Owner's coordinates to root coordinates
      over->translateCoordinatesTo(ox,oy,getRoot(),0,0);

      // Adjust position over owner
      wx=ox+(ow-ww)/2;
      wy=oy+(oh-wh)/2;

      // Where's the mouse?
      getRoot()->getCursorPosition(x,y,state);

      // Move by the minimal amount
      if(x<wx) wx=x-20; else if(wx+ww<=x) wx=x-ww+20;
      if(y<wy) wy=y-20; else if(wy+wh<=y) wy=y-wh+20;

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place centered over the owner
    case PLACEMENT_OWNER:

      // Get the owner
      over=getOwner()?getOwner():getRoot();

      // Get owner window size
      ow=over->getWidth();
      oh=over->getHeight();

      // Owner's coordinates to root coordinates
      over->translateCoordinatesTo(ox,oy,getRoot(),0,0);

      // Adjust position
      wx=ox+(ow-ww)/2;
      wy=oy+(oh-wh)/2;

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place centered on the screen
    case PLACEMENT_SCREEN:

      // Adjust position
      wx=rx+(rw-ww)/2;
      wy=ry+(rh-wh)/2;
      break;

    // Place to make it fully visible
    case PLACEMENT_VISIBLE:

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place maximized
    case PLACEMENT_MAXIMIZED:
      wx=rx;
      wy=ry;
      ww=rw;                // Yes, I know:- we should subtract the borders;
      wh=rh;                // trouble is, no way to know how big those are....
      break;

    // Default placement
    case PLACEMENT_DEFAULT:
    default:
      break;
    }

  // Place it
  position(wx,wy,ww,wh);
  }


// Set large icon(s)
void FXTopWindow::seticons(){
  HICON icold,icnew;
  icnew=nullptr;
  if(icon){
    ICONINFO iconinfo;
    iconinfo.fIcon=true;
    iconinfo.xHotspot=0;
    iconinfo.yHotspot=0;
    iconinfo.hbmMask=(HBITMAP)icon->shape;
    iconinfo.hbmColor=(HBITMAP)icon->xid;
    icnew=CreateIconIndirect(&iconinfo);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,(LPARAM)icnew))!=0){
    DestroyIcon(icold);
    }
  icnew=nullptr;
  if(miniIcon){
    ICONINFO iconinfo;
    iconinfo.fIcon=true;
    iconinfo.xHotspot=0;
    iconinfo.yHotspot=0;
    iconinfo.hbmMask=(HBITMAP)miniIcon->shape;
    iconinfo.hbmColor=(HBITMAP)miniIcon->xid;
    icnew=CreateIconIndirect(&iconinfo);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,(LPARAM)icnew))!=0){
    DestroyIcon(icold);
    }
  }


// Set title
void FXTopWindow::settitle(){
  if(!title.empty()){
#ifdef UNICODE
    FXnchar unititle[1024];
    utf2ncs(unititle,title.text(),ARRAYNUMBER(unititle));
    SetWindowTextW((HWND)xid,unititle);
#else
    SetWindowTextA((HWND)xid,title.text());
#endif
    }
  }


// Set decorations
void FXTopWindow::setdecorations(){
  // Get old style
  DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
  RECT rect;

  // Moved here just in case the size changes behind our backs
  SetRect(&rect,0,0,width,height);

  // Change style setting; note, under Windows, if we want a minimize,
  // maximize, or close button, we also need a window menu style as well.
  // Also, if you want a title, you will need a border.
  if(options&DECOR_BORDER) dwStyle|=WS_BORDER; else dwStyle&=~WS_BORDER;
  if(options&DECOR_TITLE) dwStyle|=WS_CAPTION; else dwStyle&=~WS_DLGFRAME;
  if(options&DECOR_RESIZE) dwStyle|=WS_THICKFRAME; else dwStyle&=~WS_THICKFRAME;
  if(options&DECOR_MENU) dwStyle|=WS_SYSMENU; else dwStyle&=~WS_SYSMENU;
  if(options&DECOR_CLOSE) dwStyle|=WS_SYSMENU;
  if(options&DECOR_MINIMIZE) dwStyle|=(WS_MINIMIZEBOX|WS_SYSMENU); else dwStyle&=~WS_MINIMIZEBOX;
  if(options&DECOR_MAXIMIZE) dwStyle|=(WS_MAXIMIZEBOX|WS_SYSMENU); else dwStyle&=~WS_MAXIMIZEBOX;

  // Set new style
  SetWindowLong((HWND)xid,GWL_STYLE,dwStyle);

  // Patch from Stephane Ancelot <sancelot@wanadoo.fr> and Sander Jansen <sander@knology.net>
  HMENU sysmenu=GetSystemMenu((HWND)xid,false);
  if(sysmenu){
    if(options&DECOR_CLOSE)
      EnableMenuItem(sysmenu,SC_CLOSE,MF_ENABLED);
    else
      EnableMenuItem(sysmenu,SC_CLOSE,MF_GRAYED);
    }

  // Moved here just in case SetWindowLong GWL_STYLE has changed
  // the GWL_EXSTYLE behind the scenes...
  DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);

  // Adjust non-client area size based on new style
  AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);
  SetWindowPos((HWND)xid,nullptr,0,0,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER);
  RedrawWindow((HWND)xid,nullptr,nullptr,RDW_FRAME|RDW_INVALIDATE);
  }


// Obtain border sizes added to our window by the window manager
FXbool FXTopWindow::getWMBorders(FXint& left,FXint& right,FXint& top,FXint& bottom){
  left=right=top=bottom=0;
  if(xid){
#if(WINVER >= 0x0500)
    WINDOWINFO wi;
    GetWindowInfo((HWND)xid,&wi);
    left=wi.rcClient.left-wi.rcWindow.left;
    top=wi.rcClient.top-wi.rcWindow.top;
    right=wi.rcWindow.right-wi.rcClient.right;
    bottom=wi.rcWindow.bottom-wi.rcClient.bottom;
#endif
    return true;
    }
  return false;
  }


// Flash the window to get user's attention
void FXTopWindow::flash(FXbool yes){
  if(xid){
//    FLASHWINFO flashinfo;
//    flashinfo.cbSize=sizeof(flashinfo);
//    flashinfo.hwnd=xid;
//    flashinfo.dwFlags=FLASHW_ALL;
//    flashinfo.uCount=5;
//    flashinfo.dwTimeout=0;
//    FlashWindowEx(flashinfo);
    FlashWindow((HWND)xid,false);
    }
  }


// Restore window
FXbool FXTopWindow::restore(FXbool notify){
  if(isMinimized() || isMaximized() || isFullScreen()){
    if(xid){
      ShowWindow((HWND)xid,SW_RESTORE);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_RESTORE,message),nullptr);}
    return true;
    }
  return false;
  }


// Iconify window
FXbool FXTopWindow::maximize(FXbool notify){
  if(!isMaximized()){
    if(xid){
      ShowWindow((HWND)xid,SW_MAXIMIZE);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Miminize or iconify window
FXbool FXTopWindow::minimize(FXbool notify){
  if(!isMinimized()){
    if(xid){
      ShowWindow((HWND)xid,SW_MINIMIZE);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }

#if 0

    // Go to fullscreen
    DWORD dWStyle=GetWindowLong((HWND)xid,GWL_STYLE);
    WINDOWPLACEMENT windowplacement={sizeof(WINDOWPLACEMENT)};
    if(GetWindowPlacement((HWND)xid,&windowplacement)){
      HANDLE hmonitor=fxMonitorFromWindow((HWND)xid,MONITOR_DEFAULTTOPRIMARY);
      if(hmonitor){
        MYMONITORINFO monitorinfo={sizeof(MYMONITORINFO)};
        if(fxGetMonitorInfo(hmonitor,&monitorinfo)){
          SetWindowLong((HWND)xid,GWL_STYLE,dwStyle&~WS_OVERLAPPEDWINDOW);
          SetWindowPos((HWND)xid,HWND_TOP,monitorinfo.rcMonitor.left,monitorinfo.rcMonitor.top,monitorinfo.rcMonitor.right-monitorinfo.rcMonitor.left,monitorinfo.rcMonitor.bottom-monitorinfo.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
          }
        }
      }

    // Go back to normal
    DWORD dWStyle=GetWindowLong((HWND)xid,GWL_STYLE);
    SetWindowLong((HWND)xid,GWL_STYLE,dwStyle|WS_OVERLAPPEDWINDOW);
    SetWindowPlacement((HWND)xid,&windowplacement);
    SetWindowPos((HWND)xid,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);


#endif

// Make window full screen, return true if success
FXbool FXTopWindow::fullScreen(FXbool notify){
  if(!isFullScreen()){
    if(xid){
      ShowWindow((HWND)xid,SW_MAXIMIZE);        // FIXME
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Special stacking order level
FXbool FXTopWindow::stackingOrder(FXuint order){
  if(xid){
    switch(order){
      case STACK_BOTTOM:
        SetWindowPos((HWND)xid,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      case STACK_TOP:
        SetWindowPos((HWND)xid,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      default:
        SetWindowPos((HWND)xid,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      }
    return true;
    }
  return false;
  }


// Return true if window has been maximized
FXbool FXTopWindow::isMaximized() const {
  FXbool result=false;
  if(xid){
    result=IsZoomed((HWND)xid)!=0;
    }
  return result;
  }


// Return true if window has been minimized
FXbool FXTopWindow::isMinimized() const {
  FXbool result=false;
  if(xid){
    result=IsIconic((HWND)xid)!=0;
    }
  return result;
  }


// Return true if full screen
FXbool FXTopWindow::isFullScreen() const {
  FXbool result=false;
  if(xid){
    result=IsZoomed((HWND)xid)!=0;          // FIXME
    }
  return result;
  }


// Request for toplevel window move
void FXTopWindow::move(FXint x,FXint y){
  if((x!=xpos) || (y!=ypos)){
    xpos=x;
    ypos=y;
    if(xid){
      RECT rect;
      SetRect(&rect,xpos,ypos,0,0);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
      }
    }
  }


// Request for toplevel window resize
void FXTopWindow::resize(FXint w,FXint h){
  if((flags&FLAG_DIRTY) || (w!=width) || (h!=height)){
    width=FXMAX(w,1);
    height=FXMAX(h,1);
    if(xid){
      RECT rect;
      SetRect(&rect,0,0,width,height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,0,0,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER);
      layout();
      }
    }
  }


// Request for toplevel window reposition
void FXTopWindow::position(FXint x,FXint y,FXint w,FXint h){
  if((flags&FLAG_DIRTY) || (x!=xpos) || (y!=ypos) || (w!=width) || (h!=height)){
    xpos=x;
    ypos=y;
    width=FXMAX(w,1);
    height=FXMAX(h,1);
    if(xid){
      RECT rect;
      SetRect(&rect,xpos,ypos,xpos+width,ypos+height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOZORDER|SWP_NOOWNERZORDER);
      layout();
      }
    }
  }


// Change regular icon
void FXTopWindow::setIcon(FXIcon* ic){
  if(icon!=ic){
    icon=ic;
    if(xid){
      HICON icold=nullptr;
      HICON icnew=nullptr;
      if(icon){
        ICONINFO iconinfo;
        iconinfo.fIcon=true;
        iconinfo.xHotspot=0;
        iconinfo.yHotspot=0;
        iconinfo.hbmMask=(HBITMAP)icon->shape;
        iconinfo.hbmColor=(HBITMAP)icon->xid;
        icnew=CreateIconIndirect(&iconinfo);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,(LPARAM)icnew))!=0){
        DestroyIcon(icold);
        }
      }
    }
  }


// Change mini icon
void FXTopWindow::setMiniIcon(FXIcon *ic){
  if(miniIcon!=ic){
    miniIcon=ic;
    if(xid){
      HICON icold=nullptr;
      HICON icnew=nullptr;
      if(miniIcon){
        ICONINFO iconinfo;
        iconinfo.fIcon=true;
        iconinfo.xHotspot=0;
        iconinfo.yHotspot=0;
        iconinfo.hbmMask=(HBITMAP)miniIcon->shape;
        iconinfo.hbmColor=(HBITMAP)miniIcon->xid;
        icnew=CreateIconIndirect(&iconinfo);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,(LPARAM)icnew))!=0){
        DestroyIcon(icold);
        }
      }
    }
  }


// Remove this one from toplevel window list
FXTopWindow::~FXTopWindow(){
  HICON icold;
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,0))!=0){           // FIXME move to destroy()?
    DestroyIcon(icold);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,0))!=0){
    DestroyIcon(icold);
    }
  icon=(FXIcon*)-1L;
  miniIcon=(FXIcon*)-1L;
  }

}

#endif
