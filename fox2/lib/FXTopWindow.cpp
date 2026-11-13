/********************************************************************************
*                                                                               *
*                         T o p   W i n d o w   O b j e c t                     *
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


/*
  Notes:
  - Handle zero width/height case similar to FXWindow.
  - Pass Size Hints to Window Manager as per ICCCM.
  - Add padding options, as this is convenient for FXDialogBox subclasses;
    for FXTopWindow/FXMainWindow, padding should default to 0, for FXDialogBox,
    default to something easthetically pleasing...
  - Now observes LAYOUT_FIX_X and LAYOUT_FIX_Y hints.
  - LAYOUT_FIX_WIDTH and LAYOUT_FIX_HEIGHT take precedence over PACK_UNIFORM_WIDTH and
    PACK_UNIFORM_HEIGHT!
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

// Side layout modes
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

// Layout modes
#define LAYOUT_MASK (LAYOUT_SIDE_MASK|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y)

#define DISPLAY(app) ((Display*)((app)->display))

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXTopWindow) FXTopWindowMap[]={
  FXMAPFUNC(SEL_CLOSE,0,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_FOCUS_UP,0,FXTopWindow::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXTopWindow::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FXTopWindow::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FXTopWindow::onFocusRight),
  FXMAPFUNC(SEL_RESTORE,0,FXTopWindow::onRestore),
  FXMAPFUNC(SEL_MAXIMIZE,0,FXTopWindow::onMaximize),
  FXMAPFUNC(SEL_MINIMIZE,0,FXTopWindow::onMinimize),
  FXMAPFUNC(SEL_SESSION_NOTIFY,0,FXTopWindow::onSessionNotify),
  FXMAPFUNC(SEL_SESSION_CLOSED,0,FXTopWindow::onSessionClosed),
  FXMAPFUNC(SEL_CHORE,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_SIGNAL,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_TIMEOUT,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_RESTORE,FXTopWindow::onCmdRestore),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_MAXIMIZE,FXTopWindow::onCmdMaximize),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_MINIMIZE,FXTopWindow::onCmdMinimize),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_FULLSCREEN,FXTopWindow::onCmdFullScreen),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_SETSTRINGVALUE,FXTopWindow::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_GETSTRINGVALUE,FXTopWindow::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_SETICONVALUE,FXTopWindow::onCmdSetIconValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_GETICONVALUE,FXTopWindow::onCmdGetIconValue),
  };


// Object implementation
FXIMPLEMENT_ABSTRACT(FXTopWindow,FXShell,FXTopWindowMap,ARRAYNUMBER(FXTopWindowMap))


// Deserialization
FXTopWindow::FXTopWindow(){
  icon=nullptr;
  miniIcon=nullptr;
  padtop=0;
  padbottom=0;
  padleft=0;
  padright=0;
  hspacing=0;
  vspacing=0;
  }


// Create toplevel window object & add to toplevel window list
FXTopWindow::FXTopWindow(FXApp* ap,const FXString& name,FXIcon *ic,FXIcon *mi,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXShell(ap,opts,x,y,w,h){
  title=name;
  icon=ic;
  miniIcon=mi;
  accelTable=new FXAccelTable;
  padtop=pt;
  padbottom=pb;
  padleft=pl;
  padright=pr;
  hspacing=hs;
  vspacing=vs;
  }


// Create toplevel window object & add to toplevel window list
FXTopWindow::FXTopWindow(FXWindow* ow,const FXString& name,FXIcon *ic,FXIcon *mi,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXShell(ow,opts,x,y,w,h){
  title=name;
  icon=ic;
  miniIcon=mi;
  accelTable=new FXAccelTable;
  padtop=pt;
  padbottom=pb;
  padleft=pl;
  padright=pr;
  hspacing=hs;
  vspacing=vs;
  }


// Detach window
void FXTopWindow::detach(){
  FXShell::detach();
  if(icon) icon->detach();
  if(miniIcon) miniIcon->detach();
  }


// Show and raise window
void FXTopWindow::show(){
  FXShell::show();
  raise();
  }


// Show and raise window, placed properly on the screen
void FXTopWindow::show(FXuint placement){
  place(placement);
  FXShell::show();
  raise();
  }


// The code below returns the size of the entire virtual
// screen area instead of just that of the primary display;
// thanks to "Steve Granja" <Steven.Granja@abaqus.com>.
// [Apparently does not work on Win95 and WinNT...]
//xpos=GetSystemMetrics(SM_XVIRTUALSCREEN);
//ypos=GetSystemMetrics(SM_YVIRTUALSCREEN);
//width=GetSystemMetrics(SM_CXVIRTUALSCREEN);
//height=GetSystemMetrics(SM_CYVIRTUALSCREEN);


// Change decorations
void FXTopWindow::setDecorations(FXuint decorations){
  FXuint opts=(options&~DECOR_ALL) | (decorations&DECOR_ALL);
  if(options!=opts){
    options=opts;
    if(xid) setdecorations();
    recalc();
    }
  }


// Get decorations
FXuint FXTopWindow::getDecorations() const {
  return options&DECOR_ALL;
  }


// Attempt to close the window, return true if actually closed
FXbool FXTopWindow::close(FXbool notify){
  FXWindow *window;

  // Ask target if desired
  if(!notify || !target || !target->tryHandle(this,FXSEL(SEL_CLOSE,message),nullptr)){

    // Target will receive no further messages from us
    setTarget(nullptr);
    setSelector(0);

    // If there was another main level window still visible, that's all we do
    for(window=getRoot()->getFirst(); window; window=window->getNext()){
      if(window!=this && window->isMemberOf(FXMETACLASS(FXMainWindow))){
        goto x;
        }
      }

    // We've just hidden the last remaining top level window:- quit the application
    getApp()->handle(this,FXSEL(SEL_COMMAND,FXApp::ID_QUIT),nullptr);

    // Self destruct
x:  delete this;

    // Was closed
    return true;
    }
  return false;
  }


#if 0
  XWindowAttributes watts;
  XGetWindowAttributes(DISPLAY(getApp()),xid,&watts);
  return (watts.map_state==IsViewable)
  return (watts.map_state==IsUnMapped)
  return (watts.map_state==IsUnviewable)

  Window       rootwindow;
  Window       parentwindow;
  Window*      childwindow;
  unsigned int nchildwindows;
  Window       w;

//  w=xid;
  w=XDefaultRootWindow(thedisplay);

  if(XQueryTree(thedisplay,w,&rootwindow,&parentwindow,&childwindow,&nchildwindows)){
    fxmessage("root   = 0x%lx\n",rootwindow);
    fxmessage("parent = 0x%lx\n",parentwindow);
    for(int c=0; c<nchildwindows; ++c){
      fxmessage("child[%d] = 0x%lx\n",c,childwindow[c]);
      }
    XFree(childwindow);
    }
  fxmessage("self   = 0x%lx\n",xid);
  fxmessage("\n");


Status XQueryTree(Display *display,Window w,Window* root_return,Window* parent_return,Window** children_return,unsigned int * nchildren_return)

The XQueryTree() function returns the root ID, the parent window ID, a pointer to the list of children windows (NULL when there are no children), and the number of children in the list for the specified window. The children are listed in current stacking order, from bottommost (first) to topmost (last). XQueryTree() returns zero if it fails and nonzero if it succeeds. To free a non-NULL children list when it is no longer needed, use XFree().
#endif


// Compute minimum width based on child layout hints
FXint FXTopWindow::getDefaultWidth(){
  FXint w,wcum,wmax,mw;
  FXWindow* child;
  FXuint hints;
  wmax=wcum=mw=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getLast(); child; child=child->getPrev()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X)){    // Fixed X
        w=child->getX()+w;
        if(w>wmax) wmax=w;
        }
      else if(hints&LAYOUT_SIDE_LEFT){                      // Left or right
        if(child->getNext()) wcum+=hspacing;
        wcum+=w;
        }
      else{
        if(w>wcum) wcum=w;
        }
      }
    }
  wcum+=padleft+padright;
  return FXMAX(wcum,wmax);
  }


// Compute minimum height based on child layout hints
FXint FXTopWindow::getDefaultHeight(){
  FXint h,hcum,hmax,mh;
  FXWindow* child;
  FXuint hints;
  hmax=hcum=mh=0;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getLast(); child; child=child->getPrev()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y)){   // Fixed Y
        h=child->getY()+h;
        if(h>hmax) hmax=h;
        }
      else if(!(hints&LAYOUT_SIDE_LEFT)){                   // Top or bottom
        if(child->getNext()) hcum+=vspacing;
        hcum+=h;
        }
      else{
        if(h>hcum) hcum=h;
        }
      }
    }
  hcum+=padtop+padbottom;
  return FXMAX(hcum,hmax);
  }


// Recalculate layout
void FXTopWindow::layout(){
  FXint left,right,top,bottom,x,y,w,h;
  FXint mw=0,mh=0;
  FXWindow* child;
  FXuint hints;

  // Placement rectangle; right/bottom non-inclusive
  left=padleft;
  right=width-padright;
  top=padtop;
  bottom=height-padbottom;

  // Get maximum child size
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Pack them in the cavity
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      x=child->getX();
      y=child->getY();

      // Vertical
      if(hints&LAYOUT_SIDE_LEFT){

        // Height
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=bottom-top;
        else h=child->getDefaultHeight();

        // Width
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=child->getWidthForHeight(h);             // Width is a function of height!

        // Y
        if(!((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y))){
          if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
          else if(hints&LAYOUT_BOTTOM) y=bottom-h;
          else y=top;
          }

        // X
        if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
          if(hints&LAYOUT_CENTER_X) x=left+(right-left-w)/2;
          else if(hints&LAYOUT_SIDE_BOTTOM){            // Right
            x=right-w;
            right-=(w+hspacing);
            }
          else{                                         // Left
            x=left;
            left+=(w+hspacing);
            }
          }
        }

      // Horizontal
      else{

        // Width
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=child->getDefaultWidth();

        // Height
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=bottom-top;
        else h=child->getHeightForWidth(w);             // Height is a function of width!

        // X
        if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
          if(hints&LAYOUT_CENTER_X) x=left+(right-left-w)/2;
          else if(hints&LAYOUT_RIGHT) x=right-w;
          else x=left;
          }

        // Y
        if(!((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y))){
          if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
          else if(hints&LAYOUT_SIDE_BOTTOM){            // Bottom
            y=bottom-h;
            bottom-=(h+vspacing);
            }
          else{                                         // Top
            y=top;
            top+=(h+vspacing);
            }
          }
        }
      child->position(x,y,w,h);
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Update value from a message
long FXTopWindow::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setTitle(*((FXString*)ptr));
  return 1;
  }


// Obtain value from text field
long FXTopWindow::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTitle();
  return 1;
  }


// Update icon from a message
long FXTopWindow::onCmdSetIconValue(FXObject*,FXSelector,void* ptr){
  setMiniIcon(*((FXIcon**)ptr));
  return 1;
  }


// Obtain icon from text field
long FXTopWindow::onCmdGetIconValue(FXObject*,FXSelector,void* ptr){
  *((FXIcon**)ptr)=getMiniIcon();
  return 1;
  }


// Restore the window
long FXTopWindow::onCmdRestore(FXObject*,FXSelector,void*){
  restore(true);
  return 1;
  }


// Maximize the window
long FXTopWindow::onCmdMaximize(FXObject*,FXSelector,void*){
  maximize(true);
  return 1;
  }


// Minimize or iconify the window
long FXTopWindow::onCmdMinimize(FXObject*,FXSelector,void*){
  minimize(true);
  return 1;
  }


// Full screen the window
long FXTopWindow::onCmdFullScreen(FXObject*,FXSelector,void*){
  fullScreen(true);
  return 1;
  }


// Close window; ask target before doing close
long FXTopWindow::onCmdClose(FXObject*,FXSelector,void*){
  close(true);
  return 1;
  }


// Session is about to close, give opportunity to save data
long FXTopWindow::onSessionNotify(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onSessionNotify %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_SESSION_NOTIFY,message),ptr);
  }


// Session has closed, close the window with prejudice
long FXTopWindow::onSessionClosed(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onSessionClosed %p\n",getClassName(),this);
  if(target) target->tryHandle(this,FXSEL(SEL_SESSION_CLOSED,message),ptr);
  close(false);
  return 1;
  }


// Focus moved up
long FXTopWindow::onFocusUp(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint cury,childy;
  if(getFocus()){
    cury=getFocus()->getY();
    while(1){
      child=nullptr;
      childy=-10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && c->getY()<cury && childy<c->getY()){ childy=c->getY(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
      cury=childy;
      }
    }
  else{
    child=getLast();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
        }
      child=child->getPrev();
      }
    }
  return 0;
  }


// Focus moved down
long FXTopWindow::onFocusDown(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint cury,childy;
  if(getFocus()){
    cury=getFocus()->getY();
    while(1){
      child=nullptr;
      childy=10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && cury<c->getY() && c->getY()<childy){ childy=c->getY(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
      cury=childy;
      }
    }
  else{
    child=getFirst();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
        }
      child=child->getNext();
      }
    }
  return 0;
  }


// Focus moved to left
long FXTopWindow::onFocusLeft(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint curx,childx;
  if(getFocus()){
    curx=getFocus()->getX();
    while(1){
      child=nullptr;
      childx=-10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && c->getX()<curx && childx<c->getX()){ childx=c->getX(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
      curx=childx;
      }
    }
  else{
    child=getLast();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
        }
      child=child->getPrev();
      }
    }
  return 0;
  }


// Focus moved to right
long FXTopWindow::onFocusRight(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint curx,childx;
  if(getFocus()){
    curx=getFocus()->getX();
    while(1){
      child=nullptr;
      childx=10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && curx<c->getX() && c->getX()<childx){ childx=c->getX(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
      curx=childx;
      }
    }
  else{
    child=getFirst();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
        }
      child=child->getNext();
      }
    }
  return 0;
  }


// Handle restore notify
long FXTopWindow::onRestore(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onRestore %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_RESTORE,message),ptr);
  }


// Handle maximize notify
long FXTopWindow::onMaximize(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onMaximize %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),ptr);
  }


// Handle minimize notify
long FXTopWindow::onMinimize(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onMinimize %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),ptr);
  }


// Set new window title
void FXTopWindow::setTitle(const FXString& name){
  if(title!=name){
    title=name;
    if(xid) settitle();
    }
  }


// Change packing hints
void FXTopWindow::setPackingHints(FXuint ph){
  FXuint opts=(options&~(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH)) | (ph&(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH));
  if(opts!=options){
    options=opts;
    recalc();
    update();
    }
  }


// Get packing hints
FXuint FXTopWindow::getPackingHints() const {
  return (options&(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH));
  }


// Change top padding
void FXTopWindow::setPadTop(FXint pt){
  if(padtop!=pt){
    padtop=pt;
    recalc();
    update();
    }
  }


// Change bottom padding
void FXTopWindow::setPadBottom(FXint pb){
  if(padbottom!=pb){
    padbottom=pb;
    recalc();
    update();
    }
  }


// Change left padding
void FXTopWindow::setPadLeft(FXint pl){
  if(padleft!=pl){
    padleft=pl;
    recalc();
    update();
    }
  }


// Change right padding
void FXTopWindow::setPadRight(FXint pr){
  if(padright!=pr){
    padright=pr;
    recalc();
    update();
    }
  }


// Change horizontal spacing
void FXTopWindow::setHSpacing(FXint hs){
  if(hspacing!=hs){
    hspacing=hs;
    recalc();
    update();
    }
  }


// Change vertical spacing
void FXTopWindow::setVSpacing(FXint vs){
  if(vspacing!=vs){
    vspacing=vs;
    recalc();
    update();
    }
  }


// Save object to stream
void FXTopWindow::save(FXStream& store) const {
  FXShell::save(store);
  store << title;
  store << icon;
  store << miniIcon;
  store << padtop;
  store << padbottom;
  store << padleft;
  store << padright;
  store << hspacing;
  store << vspacing;
  }


// Load object from stream
void FXTopWindow::load(FXStream& store){
  FXShell::load(store);
  store >> title;
  store >> icon;
  store >> miniIcon;
  store >> padtop;
  store >> padbottom;
  store >> padleft;
  store >> padright;
  store >> hspacing;
  store >> vspacing;
  }

}
