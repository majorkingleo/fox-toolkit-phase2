/********************************************************************************
*                                                                               *
*                     T o p   W i n d o w   ( W a y l a n d )                   *
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
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A top window is an xdg_toplevel: the title, the application id and the size
    limits it declares are the only things the compositor accepts.  Stacking,
    position on the screen, decoration style and the icon are the compositor's
    or the decoration protocol's business, so raise(), lower(), move() and
    flash() only keep the core and the virtual window table up to date.
  - Hiding a toplevel means detaching its buffer: a surface without a buffer
    shows nothing, and the next frame attaches the content again.
  - maximize(), minimize() and fullScreen() forward to xdg_toplevel; whether a
    window is maximized or full screen is answered from the state array of the
    last configure, so the core sees what the compositor did rather than what
    we asked for.  There is no minimized state in the protocol, so we remember
    our own request.
  - Client side decorations, the icon protocol and xdg_activation arrive in
    phase 5 and later; until then those members keep the bookkeeping the core
    needs.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// The surface of this toplevel
static FXWaylandSurface* topSurface(FXApp* app,FXID id){
  if(!id) return nullptr;
  return fxwaylandShellSurface(fxwayland(app->getDisplay()),id);
  }


// Declare the size limits that follow from the decorations of the window
static void sizeLimits(FXTopWindow* window,FXWaylandSurface* surf){
  FXuint options=window->getDecorations();
  FXint defw=window->getDefaultWidth();
  FXint defh=window->getDefaultHeight();
  if(!surf || !surf->toplevel) return;
  if(!(options&DECOR_SHRINKABLE) && !(options&DECOR_STRETCHABLE)){      // Fixed size
    xdg_toplevel_set_min_size(surf->toplevel,defw,defh);
    xdg_toplevel_set_max_size(surf->toplevel,defw,defh);
    }
  else if(!(options&DECOR_SHRINKABLE)){                        // May only grow
    xdg_toplevel_set_min_size(surf->toplevel,defw,defh);
    xdg_toplevel_set_max_size(surf->toplevel,0,0);
    }
  else if(!(options&DECOR_STRETCHABLE)){                       // May only shrink
    xdg_toplevel_set_min_size(surf->toplevel,0,0);
    xdg_toplevel_set_max_size(surf->toplevel,defw,defh);
    }
  else{                                                        // Free
    xdg_toplevel_set_min_size(surf->toplevel,0,0);
    xdg_toplevel_set_max_size(surf->toplevel,0,0);
    }
  }


/*******************************************************************************/

// Create window
void FXTopWindow::create(){
  FXWaylandSurface* surf;
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXShell::create();

  // Create icons
  if(icon) icon->create();
  if(miniIcon) miniIcon->create();

  if(xid && app && (surf=topSurface(getApp(),xid))!=nullptr){
    if(getApp()->isInitialized()){

      // Title and application id; the compositor uses the id to group windows
      settitle();
      xdg_toplevel_set_app_id(surf->toplevel,getApp()->getAppName().text());

      // Decorations tell the size limits
      setdecorations();

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

      // Icons, and the window is ready to be shown
      seticons();
      }
    }
  }


// Destroy window
void FXTopWindow::destroy(){
  FXShell::destroy();
  }


// Focus to this toplevel window
void FXTopWindow::setFocus(){
  FXShell::setFocus();
  }


// Focus away from this toplevel window
void FXTopWindow::killFocus(){
  FXShell::killFocus();
  }


// Hide window
void FXTopWindow::hide(){
  FXWaylandApp* app=fxwayland(getApp()->getDisplay());
  FXWaylandSurface* surf;
  if(flags&FLAG_SHOWN){
    killFocus();
    flags&=~FLAG_SHOWN;
    if(xid){
      if(app) app->childmap.setShown(xid,false);
      if((surf=topSurface(getApp(),xid))!=nullptr) fxwaylandHideSurface(surf);
      }
    }
  }


// Raise and make foreground window; the compositor stacks the toplevels
void FXTopWindow::raise(){
  FXShell::raise();
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
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();

  // Where's the mouse?
  x=0;
  y=0;
  getRoot()->getCursorPosition(x,y,state);

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
      ww=rw;
      wh=rh;
      break;

    // Default placement
    case PLACEMENT_DEFAULT:
    default:
      break;
    }

  // Place it
  position(wx,wy,ww,wh);
  }


// Set icons; the icon protocol is not bound yet, so nothing reaches the
// compositor
void FXTopWindow::seticons(){
  if(icon){
    if(!icon->id()){ fxerror("%s::setIcon: illegal icon specified.\n",getClassName()); }
    }
  else if(miniIcon){
    if(!miniIcon->id()){ fxerror("%s::setMiniIcon: illegal icon specified.\n",getClassName()); }
    }
  }


// Set title
void FXTopWindow::settitle(){
  FXWaylandSurface* surf;
  if(!title.empty() && (surf=topSurface(getApp(),xid))!=nullptr && surf->toplevel){
    xdg_toplevel_set_title(surf->toplevel,title.text());
    }
  }


// Set decorations; the decoration protocol arrives in phase 5, so only the
// size limits the decorations imply are declared here
void FXTopWindow::setdecorations(){
  sizeLimits(this,topSurface(getApp(),xid));
  }


// Obtain border sizes added to our window by the compositor; the client does
// not know them unless it draws the decorations itself
FXbool FXTopWindow::getWMBorders(FXint& left,FXint& right,FXint& top,FXint& bottom){
  left=0;
  right=0;
  top=0;
  bottom=0;
  return false;
  }


// Flash the window to get user's attention; xdg_activation arrives later
void FXTopWindow::flash(FXbool){
  }


// Restore window
FXbool FXTopWindow::restore(FXbool notify){
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  if(isMinimized() || isMaximized() || isFullScreen()){
    if(surf && surf->toplevel){
      xdg_toplevel_unset_fullscreen(surf->toplevel);
      xdg_toplevel_unset_maximized(surf->toplevel);
      surf->state&=~(FXXWL_STATE_MINIMIZED|FXXWL_STATE_FULLSCREEN|FXXWL_STATE_MAXIMIZED);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_RESTORE,message),nullptr);}
    return true;
    }
  return false;
  }


// Maximize window
FXbool FXTopWindow::maximize(FXbool notify){
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  if(!isMaximized()){
    if(surf && surf->toplevel){
      surf->state&=~FXXWL_STATE_MINIMIZED;
      xdg_toplevel_set_maximized(surf->toplevel);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Minimize or iconify window
FXbool FXTopWindow::minimize(FXbool notify){
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  if(!isMinimized()){
    if(surf && surf->toplevel){
      surf->state|=FXXWL_STATE_MINIMIZED;
      xdg_toplevel_set_minimized(surf->toplevel);
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Make window full screen
FXbool FXTopWindow::fullScreen(FXbool notify){
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  if(!isFullScreen()){
    if(surf && surf->toplevel){
      surf->state&=~FXXWL_STATE_MINIMIZED;
      xdg_toplevel_set_fullscreen(surf->toplevel,nullptr);       // Output of the compositor
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Special stacking order level; the compositor owns the stacking of toplevels
FXbool FXTopWindow::stackingOrder(FXuint){
  return false;
  }


// Return true if window has been maximized
FXbool FXTopWindow::isMaximized() const {
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  return surf?(surf->state&FXXWL_STATE_MAXIMIZED)!=0:false;
  }


// Return true if window has been minimized
FXbool FXTopWindow::isMinimized() const {
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  return surf?(surf->state&FXXWL_STATE_MINIMIZED)!=0:false;
  }


// Return true if full screen
FXbool FXTopWindow::isFullScreen() const {
  FXWaylandSurface* surf=topSurface(getApp(),xid);
  return surf?(surf->state&FXXWL_STATE_FULLSCREEN)!=0:false;
  }


// Request for toplevel window move
void FXTopWindow::move(FXint x,FXint y){
  FXWindow::move(x,y);
  }


// Request for toplevel window resize
void FXTopWindow::resize(FXint w,FXint h){
  FXWindow::resize(w,h);
  sizeLimits(this,topSurface(getApp(),xid));
  }


// Request for toplevel window reposition
void FXTopWindow::position(FXint x,FXint y,FXint w,FXint h){
  FXWindow::position(x,y,w,h);
  sizeLimits(this,topSurface(getApp(),xid));
  }


// Change regular icon
void FXTopWindow::setIcon(FXIcon* ic){
  if(icon!=ic){
    icon=ic;
    if(xid){ seticons(); }
    }
  }


// Change mini icon
void FXTopWindow::setMiniIcon(FXIcon *ic){
  if(miniIcon!=ic){
    miniIcon=ic;
    if(xid){ seticons(); }
    }
  }


// Remove this one from toplevel window list
FXTopWindow::~FXTopWindow(){
  icon=(FXIcon*)-1L;
  miniIcon=(FXIcon*)-1L;
  }


// Wayland has no window classes
const void* FXTopWindow::GetClass() const {
  return nullptr;
  }

}

#endif
