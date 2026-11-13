/********************************************************************************
*                                                                               *
*        T o p   L e v e l   W i n d o w   F u n c t i o n s   ( X 1 1 )        *
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

#if defined(FOX_X11)

/*
  Notes:

  - Platform implementation of the FXTopWindow members which create, show, decorate, stack and size the native top level window; the layout and the common accessors stay in FXTopWindow.cpp.
*/

#define TOPIC_DETAIL    1002

// Definitions for Motif-style WM Hints.
#define MWM_HINTS_FUNCTIONS	(1L << 0)       // Definitions for FXMotifHints.flags
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)
#define MWM_HINTS_ALL           (MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE)

#define MWM_FUNC_ALL		(1L << 0)       // Definitions for FXMotifHints.functions
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

#define MWM_DECOR_ALL		(1L << 0)       // Definitions for FXMotifHints.decorations
#define MWM_DECOR_BORDER	(1L << 1)
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

#define MWM_INPUT_MODELESS		    0   // Values for FXMotifHints.inputmode
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL		    2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define DISPLAY(app) ((Display*)((app)->display))

using namespace FX;

/*******************************************************************************/

namespace FX {

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


      // Set WM protocols
      Atom protocols[4];
      protocols[0]=getApp()->wmDeleteWindow;
      protocols[1]=getApp()->wmTakeFocus;
      protocols[2]=getApp()->wmNetPing;
      protocols[3]=getApp()->wmSaveYourself;
      XSetWMProtocols(DISPLAY(getApp()),xid,protocols,4);       // Catch delete window

      // Set resource and class name for toplevel windows.
      // In a perfect world this would be set in FXTopWindow, but for some strange reasons
      // some window-managers (e.g. fvwm) this will be too late and they will not recognize them.
      XClassHint hint;
      hint.res_name=const_cast<char*>(getApp()->getAppName().text());           // "FoxApp"
      hint.res_class=const_cast<char*>(getApp()->getVendorName().text());       // "FoxWindow"
      XSetClassHint((Display*)getApp()->getDisplay(),xid,&hint);

      // Set client machine name and application pid
      FXString hostname=FXSystem::getHostName();
      XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->wmClientMachine,XA_STRING,8,PropModeReplace,(unsigned char*)hostname.text(),hostname.length());
      FXint proccessid=FXProcess::current();
      XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->wmNetProcessId,XA_CARDINAL,32,PropModeReplace,(unsigned char*)&proccessid,1);

//#ifdef HAVE_XFIXES_H
//      int evb,erb;
//      if(XFixesQueryExtension(DISPLAY(getApp()),&evb,&erb)){
//        XFixesSelectSelectionInput(DISPLAY(getApp()),xid,getApp()->xcbSelection,XFixesSetSelectionOwnerNotifyMask|XFixesSelectionWindowDestroyNotifyMask|XFixesSelectionClientCloseNotifyMask);
//        }
//#endif
      }
    }
  }


// Destroy window
void FXTopWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      }
    }
  FXShell::destroy();
  }


// Focus to this toplevel window
void FXTopWindow::setFocus(){
  FXShell::setFocus();
  if(xid){
    XSetInputFocus(DISPLAY(getApp()),xid,RevertToPointerRoot,CurrentTime);
    }
  }


// Focus away from this toplevel window
void FXTopWindow::killFocus(){
  FXShell::killFocus();
  if(xid){
    Window win;
    int    dum;
    XGetInputFocus(DISPLAY(getApp()),&win,&dum);
    if(win==xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE(TOPIC_DETAIL,"focus back to owner\n");
        XSetInputFocus(DISPLAY(getApp()),getOwner()->id(),RevertToPointerRoot,CurrentTime);
        }
      else{
        FXTRACE(TOPIC_DETAIL,"focus back to NULL\n");
        XSetInputFocus(DISPLAY(getApp()),PointerRoot,RevertToPointerRoot,CurrentTime);
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
      XWithdrawWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())));
      }
    }
  }


// Raise and make foreground window
void FXTopWindow::raise(){
  FXShell::raise();
  if(xid){
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetRestack;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=None;
    se.xclient.data.l[2]=Above;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
    }
  }


// Lower
void FXTopWindow::lower(){
  FXShell::lower();
  if(xid){
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetRestack;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=None;
    se.xclient.data.l[2]=Below;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
    }
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
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();

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
  FXWindow *own=this;
  XWMHints  wmhints;
  wmhints.flags=InputHint|StateHint;
  wmhints.input=true;       // True, but ICCCM says it should be false....
  wmhints.initial_state=NormalState;
  if(icon){
    if(!icon->xid || !icon->shape){ fxerror("%s::setIcon: illegal icon specified.\n",getClassName()); }
    wmhints.flags|=IconPixmapHint|IconMaskHint;
    wmhints.icon_pixmap=icon->xid;
    wmhints.icon_mask=icon->shape;
    }
  else if(miniIcon){
    if(!miniIcon->xid || !miniIcon->shape){ fxerror("%s::setMiniIcon: illegal icon specified.\n",getClassName()); }
    wmhints.flags|=IconPixmapHint|IconMaskHint;
    wmhints.icon_pixmap=miniIcon->xid;
    wmhints.icon_mask=miniIcon->shape;
    }
  while(own->getOwner()){   // Find the ultimate owner of the whole chain
    own=own->getOwner();
    }
  if(own && own->id()){     // Set the window_group id; all windows in the group should be iconified together
    wmhints.flags|=WindowGroupHint;
    wmhints.window_group=own->id();
    }
  XSetWMHints(DISPLAY(getApp()),xid,&wmhints);
  }


// Set title
void FXTopWindow::settitle(){
  if(!title.empty()){
    FX88591Codec ascii;
    FXString string=ascii.utf2mb(title);
    XTextProperty t;
    if(XStringListToTextProperty((char**)&string,1,&t)){
      XSetWMIconName(DISPLAY(getApp()),xid,&t);
      XSetWMName(DISPLAY(getApp()),xid,&t);
      XFree(t.value);
      }

    // Extended window manager hint for true unicode name in title
    XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetIconName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
    XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetWindowName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
    }
  }


// Set decorations
void FXTopWindow::setdecorations(){
  struct {
    long flags;
    long functions;
    long decorations;
    long inputmode;
    } prop;
  prop.flags=MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE;
  prop.decorations=0;
  prop.functions=MWM_FUNC_MOVE;
  prop.inputmode=MWM_INPUT_MODELESS;
  if(options&DECOR_TITLE){
    prop.decorations|=MWM_DECOR_TITLE;
    }
  if(options&DECOR_MINIMIZE){
    prop.decorations|=MWM_DECOR_MINIMIZE;
    prop.functions|=MWM_FUNC_MINIMIZE;
    }
  if(options&DECOR_MAXIMIZE){
    prop.decorations|=MWM_DECOR_MAXIMIZE;
    prop.functions|=MWM_FUNC_MAXIMIZE;
    }
  if(options&DECOR_CLOSE){
    prop.functions|=MWM_FUNC_CLOSE;
    }
  if(options&DECOR_BORDER){
    prop.decorations|=MWM_DECOR_BORDER;
    }
  if(options&(DECOR_SHRINKABLE|DECOR_STRETCHABLE)){
    if(options&DECOR_BORDER) prop.decorations|=MWM_DECOR_RESIZEH;       // Only grips if border
    prop.functions|=MWM_FUNC_RESIZE;
    }
  if(options&DECOR_MENU){
    prop.decorations|=MWM_DECOR_MENU;
    prop.functions|=MWM_FUNC_RESIZE;
    }
  if(icon || miniIcon){         // Icons won't show unless we enable menu
    prop.decorations|=MWM_DECOR_MENU;
    }
  XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmMotifHints,getApp()->wmMotifHints,32,PropModeReplace,(unsigned char*)&prop,4);
  }


// Obtain border sizes added to our window by the window manager
FXbool FXTopWindow::getWMBorders(FXint& left,FXint& right,FXint& top,FXint& bottom){
  left=right=top=bottom=0;
  if(xid){
    unsigned int sx,sy,msx,msy,cn,border,depth;
    Window w,rw,pw,*cw;
    int ox,oy;
    w=xid;
    XGetGeometry(DISPLAY(getApp()),w,&rw,&ox,&oy,&msx,&msy,&border,&depth);
    do{
      XQueryTree(DISPLAY(getApp()),w,&rw,&pw,&cw,&cn);
      XFree(cw);
      XGetGeometry(DISPLAY(getApp()),w,&rw,&ox,&oy,&sx,&sy,&border,&depth);
      if(pw!=rw){
        left+=ox;
        top+=oy;
        }
      w=pw;
      }
    while(w!=rw);
    right=(sx-msx-left);
    bottom=(sy-msy-top);
    return true;
    }
  return false;
  }


// Flash the window to get user's attention
void FXTopWindow::flash(FXbool yes){
  if(xid){
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetState;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=yes;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
    se.xclient.data.l[1]=getApp()->wmNetNeedAttention;
    se.xclient.data.l[2]=0;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
    }
  }


// Restore window
FXbool FXTopWindow::restore(FXbool notify){
  if(isMinimized() || isMaximized() || isFullScreen()){
    if(xid){
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=0;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetHMaximized;
      se.xclient.data.l[2]=getApp()->wmNetVMaximized;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      se.xclient.data.l[1]=getApp()->wmNetFullScreen;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
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
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=2;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetHMaximized;
      se.xclient.data.l[2]=getApp()->wmNetVMaximized;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
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
      XIconifyWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())));
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
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=2;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetFullScreen;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Special stacking order level
FXbool FXTopWindow::stackingOrder(FXuint order){
  if(xid){
    XEvent se;
    switch(order){
      case STACK_BOTTOM:
        se.xclient.data.l[0]=1;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetBelowOthers;
        se.xclient.data.l[2]=0;
        break;
      case STACK_TOP:
        se.xclient.data.l[0]=1;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetAboveOthers;
        se.xclient.data.l[2]=0;
        break;
      default:
        se.xclient.data.l[0]=0;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetAboveOthers;
        se.xclient.data.l[2]=getApp()->wmNetBelowOthers;
        break;
      }
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetState;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
    return true;
    }
  return false;
  }


// Return true if window has been maximized
FXbool FXTopWindow::isMaximized() const {
  FXbool result=false;
  if(xid){
    // For Window Managers supporting the Extended Window Manager Hints
    // See http://www.freedesktop.org/ for the official documentation of EWMH
    unsigned long n,i; Atom type; unsigned char *prop; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmNetState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(type==XA_ATOM && format==32){
        for(i=0; i<n; i++){
          if(((FXID*)prop)[i]==getApp()->wmNetHMaximized) result=true;
          if(((FXID*)prop)[i]==getApp()->wmNetVMaximized) result=true;
          }
        }
      XFree(prop);
      }
    }
  return result;
  }


// Return true if window has been minimized
FXbool FXTopWindow::isMinimized() const {
  FXbool result=false;
  if(xid){
    // This is ICCCM compliant method to ask about WM_STATE
    unsigned long n,i; unsigned char *prop; Atom type; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(format==32){ result=(IconicState==*((FXuint*)prop)); }
      XFree(prop);
      }
    }
  return result;
  }


// Return true if full screen
FXbool FXTopWindow::isFullScreen() const {
  FXbool result=false;
  if(xid){
    // For Window Managers supporting the Extended Window Manager Hints
    // See http://www.freedesktop.org/ for the official documentation of EWMH
    unsigned long n,i; Atom type; unsigned char *prop; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmNetState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(type==XA_ATOM && format==32){
        for(i=0; i<n; i++){
          if(((FXID*)prop)[i]==getApp()->wmNetFullScreen) result=true;
          }
        }
      XFree(prop);
      }
    }
  return result;
  }


// Request for toplevel window move
void FXTopWindow::move(FXint x,FXint y){
  if((x!=xpos) || (y!=ypos)){
    xpos=x;
    ypos=y;
    if(xid){
      XWindowChanges cw;
      cw.x=xpos;
      cw.y=ypos;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWX|CWY,&cw);
      }
    }
  }


// Request for toplevel window resize
void FXTopWindow::resize(FXint w,FXint h){
  if((flags&FLAG_DIRTY) || (w!=width) || (h!=height)){
    width=FXMAX(w,1);
    height=FXMAX(h,1);
    if(xid){
      XWindowChanges changes;
      XSizeHints size;
      size.flags=USSize|PSize|PWinGravity|USPosition|PPosition;
      size.x=xpos;
      size.y=ypos;
      size.width=width;
      size.height=height;
      size.min_width=0;
      size.min_height=0;
      size.max_width=0;
      size.max_height=0;
      size.width_inc=0;
      size.height_inc=0;
      size.min_aspect.x=0;
      size.min_aspect.y=0;
      size.max_aspect.x=0;
      size.max_aspect.y=0;
      size.base_width=0;
      size.base_height=0;
      size.win_gravity=NorthWestGravity;                        // Tim Alexeevsky <realtim@mail.ru>
      size.win_gravity=StaticGravity;                           // Account for border (ICCCM)
      if(!(options&DECOR_SHRINKABLE)){
        if(!(options&DECOR_STRETCHABLE)){                       // Cannot change at all
          size.flags|=PMinSize|PMaxSize;
          size.min_width=size.max_width=width;
          size.min_height=size.max_height=height;
          }
        else{                                                   // Cannot get smaller than default
          size.flags|=PMinSize;
          size.min_width=getDefaultWidth();
          size.min_height=getDefaultHeight();
          }
        }
      else if(!(options&DECOR_STRETCHABLE)){                    // Cannot get larger than default
        size.flags|=PMaxSize;
        size.max_width=getDefaultWidth();
        size.max_height=getDefaultHeight();
        }
      XSetWMNormalHints(DISPLAY(getApp()),xid,&size);
      changes.x=0;
      changes.y=0;
      changes.width=width;
      changes.height=height;
      changes.border_width=0;
      changes.sibling=None;
      changes.stack_mode=Above;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWWidth|CWHeight,&changes);
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
      XWindowChanges changes;
      XSizeHints size;
      size.flags=USSize|PSize|PWinGravity|USPosition|PPosition;
      size.x=xpos;
      size.y=ypos;
      size.width=width;
      size.height=height;
      size.min_width=0;
      size.min_height=0;
      size.max_width=0;
      size.max_height=0;
      size.width_inc=0;
      size.height_inc=0;
      size.min_aspect.x=0;
      size.min_aspect.y=0;
      size.max_aspect.x=0;
      size.max_aspect.y=0;
      size.base_width=0;
      size.base_height=0;
      size.win_gravity=NorthWestGravity;                        // Tim Alexeevsky <realtim@mail.ru>
      size.win_gravity=StaticGravity;                           // Account for border (ICCCM)
      if(!(options&DECOR_SHRINKABLE)){
        if(!(options&DECOR_STRETCHABLE)){                       // Cannot change at all
          size.flags|=PMinSize|PMaxSize;
          size.min_width=size.max_width=width;
          size.min_height=size.max_height=height;
          }
        else{                                                   // Cannot get smaller than default
          size.flags|=PMinSize;
          size.min_width=getDefaultWidth();
          size.min_height=getDefaultHeight();
          }
        }
      else if(!(options&DECOR_STRETCHABLE)){                    // Cannot get larger than default
        size.flags|=PMaxSize;
        size.max_width=getDefaultWidth();
        size.max_height=getDefaultHeight();
        }
      XSetWMNormalHints(DISPLAY(getApp()),xid,&size);
      changes.x=xpos;
      changes.y=ypos;
      changes.width=width;
      changes.height=height;
      changes.border_width=0;
      changes.sibling=None;
      changes.stack_mode=Above;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWX|CWY|CWWidth|CWHeight,&changes);
      layout();
      }
    }
  }


// Change regular icon
void FXTopWindow::setIcon(FXIcon* ic){
  if(icon!=ic){
    icon=ic;
    if(xid){
      seticons();
      }
    }
  }


// Change mini icon
void FXTopWindow::setMiniIcon(FXIcon *ic){
  if(miniIcon!=ic){
    miniIcon=ic;
    if(xid){
      seticons();
      }
    }
  }


// Remove this one from toplevel window list
FXTopWindow::~FXTopWindow(){
  icon=(FXIcon*)-1L;
  miniIcon=(FXIcon*)-1L;
  }



// X11 has no window classes
const void* FXTopWindow::GetClass() const {
  return nullptr;
  }

}

#endif
