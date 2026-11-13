/********************************************************************************
*                                                                               *
*                            W i n d o w   O b j e c t                          *
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
#include "FXRegion.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXCursor.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXRootWindow.h"
#include "FXShell.h"
#include "FXTopWindow.h"
#include "FXException.h"
#include "FXStatusLine.h"
#include "FXTranslator.h"
#include "FXComposeContext.h"
#include "FXTextField.h"
#include "FX88591Codec.h"
#include "FXUTF16Codec.h"
#include "fxplatformwindow.h"
#include "fxpriv.h"

/*
 Notes:
  - Major Contributions for Windows NT by Lyle Johnson.
  - When window is disabled, it should lose focus
  - When no subclass handles SEL_SELECTION_REQUEST, send back None property here
  - Update should only happen if widget is not in some sort of transaction.
  - Not every widget needs help and tip data; move this down to buttons etc.
  - Default constructors [for serialization] should initialize dynamic member
    variables same as regular constructor.  Dynamic variables are those that
    are not saved during serialization.
  - If FLAG_DIRTY gets reset at the END of layout(), it will be safe to have
    show() and hide() to call recalc(), in case they get called from the
    application code.
  - Use INCR mechanism if it gets large.
  - Perhaps should post CLIPBOARD type list in advance also, just like windows insists.
    We will need to keep this list some place [perhaps same place as XDND] and clean it
    after we're done [This would imply the CLIPBOARD type list would be the same as the
    XDND type list, which is probably OK].
  - Add URL jump text also.
  - Can we call parent->recalc() in the constructor?
  - Need other focus models:
      o click to focus (like we have now)
      o focus only by tabs and arrows (buttons should not move focus when clicking)
      o point to focus (useful for canvas type widgets)
    Mouse wheel should probably not cause focus changes...
  - Close v.s. delete messages are not consistent.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002
#define TOPIC_CONFIGURE 1003
#define TOPIC_KEYBOARD  1009
#define TOPIC_MOUSE     1010
#define TOPIC_EVENT     1011

// Side layout modes
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

// Layout modes
#define LAYOUT_MASK (LAYOUT_SIDE_MASK|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_DOCK_SAME|LAYOUT_DOCK_NEXT)


using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXWindow) FXWindowMap[]={
  FXMAPFUNC(SEL_UPDATE,0,FXWindow::onUpdate),
  FXMAPFUNC(SEL_PAINT,0,FXWindow::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXWindow::onMotion),
  FXMAPFUNC(SEL_CONFIGURE,0,FXWindow::onConfigure),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXWindow::onMouseWheel),
  FXMAPFUNC(SEL_MAP,0,FXWindow::onMap),
  FXMAPFUNC(SEL_UNMAP,0,FXWindow::onUnmap),
  FXMAPFUNC(SEL_DRAGGED,0,FXWindow::onDragged),
  FXMAPFUNC(SEL_ENTER,0,FXWindow::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXWindow::onLeave),
  FXMAPFUNC(SEL_DESTROY,0,FXWindow::onDestroy),
  FXMAPFUNC(SEL_FOCUSIN,0,FXWindow::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXWindow::onFocusOut),
  FXMAPFUNC(SEL_SELECTION_LOST,0,FXWindow::onSelectionLost),
  FXMAPFUNC(SEL_SELECTION_GAINED,0,FXWindow::onSelectionGained),
  FXMAPFUNC(SEL_SELECTION_REQUEST,0,FXWindow::onSelectionRequest),
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,FXWindow::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_GAINED,0,FXWindow::onClipboardGained),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,FXWindow::onClipboardRequest),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXWindow::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXWindow::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXWindow::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXWindow::onMiddleBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXWindow::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXWindow::onRightBtnRelease),
  FXMAPFUNC(SEL_SPACEBALLMOTION,0,FXWindow::onSpaceBallMotion),
  FXMAPFUNC(SEL_SPACEBALLBUTTONPRESS,0,FXWindow::onSpaceBallButtonPress),
  FXMAPFUNC(SEL_SPACEBALLBUTTONRELEASE,0,FXWindow::onSpaceBallButtonRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXWindow::onUngrabbed),
  FXMAPFUNC(SEL_KEYPRESS,0,FXWindow::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXWindow::onKeyRelease),
  FXMAPFUNC(SEL_DND_ENTER,0,FXWindow::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXWindow::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXWindow::onDNDDrop),
  FXMAPFUNC(SEL_DND_MOTION,0,FXWindow::onDNDMotion),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXWindow::onDNDRequest),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXWindow::onFocusSelf),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXWindow::onBeginDrag),
  FXMAPFUNC(SEL_ENDDRAG,0,FXWindow::onEndDrag),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXWindow::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXWindow::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_TOGGLESHOWN,FXWindow::onUpdToggleShown),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_TOGGLESHOWN,FXWindow::onCmdToggleShown),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_RAISE,FXWindow::onCmdRaise),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_LOWER,FXWindow::onCmdLower),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_ENABLE,FXWindow::onCmdEnable),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_DISABLE,FXWindow::onCmdDisable),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_TOGGLEENABLED,FXWindow::onUpdToggleEnabled),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_TOGGLEENABLED,FXWindow::onCmdToggleEnabled),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_UPDATE,FXWindow::onCmdUpdate),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_DELETE,FXWindow::onUpdYes),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  };


// Object implementation
FXIMPLEMENT(FXWindow,FXDrawable,FXWindowMap,ARRAYNUMBER(FXWindowMap))


/*******************************************************************************/


// Drag type names
const FXchar FXWindow::octetTypeName[]="application/octet-stream";
const FXchar FXWindow::deleteTypeName[]="DELETE";
const FXchar FXWindow::textTypeName[]="text/plain";
const FXchar FXWindow::colorTypeName[]="application/x-color";
const FXchar FXWindow::urilistTypeName[]="text/uri-list";
const FXchar FXWindow::utf8TypeName[]="UTF8_STRING";
const FXchar FXWindow::utf16TypeName[]="text/utf16";
const FXchar FXWindow::actionTypeName[]="application/x-kde-cutselection";


// Drag type atoms; first widget to need it should register the type
FXDragType FXWindow::octetType=0;
FXDragType FXWindow::deleteType=0;
FXDragType FXWindow::textType=0;
FXDragType FXWindow::colorType=0;
FXDragType FXWindow::urilistType=0;
FXDragType FXWindow::utf8Type=0;
FXDragType FXWindow::actionType=0;


/*******************************************************************************/


// For deserialization; note that the deserialized window only
// becomes part of the widget tree after its loaded with load().
FXWindow::FXWindow(){
  FXTRACE(TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this);
  parent=nullptr;
  owner=nullptr;
  first=nullptr;
  last=nullptr;
  next=nullptr;
  prev=nullptr;
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=nullptr;
  dragCursor=nullptr;
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=0;
  ypos=0;
  dpi=0;
  backColor=0;
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=0;
  wk=0;
  }


// Only used for the root window
FXWindow::FXWindow(FXApp* a,FXVisual *vis):FXDrawable(a,1,1){
  FXTRACE(TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this);
  getApp()->windowCount++;
  visual=vis;
  parent=nullptr;
  owner=nullptr;
  first=last=nullptr;
  next=prev=nullptr;
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=0;
  ypos=0;
  dpi=0;
  backColor=0;
  flags=FLAG_DIRTY|FLAG_SHOWN|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT;
  wk=1;
  }


// This constructor is used for shell windows
FXWindow::FXWindow(FXApp* a,FXWindow* own,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDrawable(a,w,h){
  FXTRACE(TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this);
  getApp()->windowCount++;
  parent=a->root;
  owner=own;
  visual=getApp()->getDefaultVisual();
  first=last=nullptr;
  prev=parent->last;
  next=nullptr;
  parent->last=this;
  if(prev){
    wk=prev->wk+1;
    prev->next=this;
    }
  else{
    wk=1;
    parent->first=this;
    }
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=x;
  ypos=y;
  dpi=0;
  backColor=getApp()->getBaseColor();
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_SHELL|FLAG_CURSOR;
  options=opts;
  }


// This constructor is used for all child windows
FXWindow::FXWindow(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDrawable(p->getApp(),w,h){
  FXTRACE(TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this);
  getApp()->windowCount++;
  parent=p;
  owner=parent;
  visual=parent->getVisual();
  first=last=nullptr;
  prev=parent->last;
  next=nullptr;
  parent->last=this;
  if(prev){
    wk=prev->wk+1;
    prev->next=this;
    }
  else{
    wk=1;
    parent->first=this;
    }
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=x;
  ypos=y;
  dpi=0;
  backColor=getApp()->getBaseColor();
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=opts;
  }


// Return the scale factor from 96-DPI logical units to device pixels
FXdouble FXWindow::getScale() const {
  return dpi?FXdouble(dpi)/96.0:1.0;
  }


/*******************************************************************************/

// The idea is to recurse down, but not sideways or upward.  Recursing upward
// through the parent or owner, or sideways through the left and right sibling,
// carries the risk of serializing the entire widget tree instead of just the
// one widget and its children.
// Each widget only saves its own data and resources, and loops over its child
// widgets saving each one in turn. A final NULL child pointer is saved to
// indicate the end of the sequence.
// When loading back, we deserialize a child pointer and hang it in the widget
// tree under this widget.  We continue to deserialize child pointers until
// we find the NULL child pointer.


// Save window to stream
void FXWindow::save(FXStream& store) const {
  FXWindow *child=getFirst();
  FXDrawable::save(store);
  while(child){
    store << child;
    child=child->getNext();
    }
  store << child;
  store << accelTable;
  store << defaultCursor;
  store << dragCursor;
  store << target;
  store << message;
  store << xpos;
  store << ypos;
  store << backColor;
  store << tag;
  store << options;
  store << flags;
  store << wk;
  }


// Load window from stream
void FXWindow::load(FXStream& store){
  FXWindow *child=nullptr;
  FXDrawable::load(store);
  getApp()->windowCount++;
  store >> child;
  while(child){
    child->parent=this;
    child->owner=this;
    child->prev=last;
    child->next=nullptr;
    if(last){
      last->next=child;
      }
    else{
      first=child;
      }
    last=child;
    store >> child;
    }
  store >> accelTable;
  store >> defaultCursor;
  store >> dragCursor;
  store >> target;
  store >> message;
  store >> xpos;
  store >> ypos;
  store >> backColor;
  store >> tag;
  store >> options;
  store >> flags;       // FIXME
  store >> wk;
  }


// Get window class; most windows have normal class
FXWindow::WindowClass FXWindow::getWindowClass() const {
  return ClassNormal;
  }


/*******************************************************************************/


// Return a pointer to the shell window
FXWindow* FXWindow::getShell() const {
  FXWindow* win=const_cast<FXWindow*>(this);
  while(win->getParent() && win->getParent()->getParent()){ win=win->getParent(); }
  return win;
  }


// Return a pointer to the root window
FXWindow* FXWindow::getRoot() const {
  FXWindow* win=const_cast<FXWindow*>(this);
  while(win->getParent()){ win=win->getParent(); }
  return win;
  }


// Test if logically inside
FXbool FXWindow::contains(FXint parentx,FXint parenty) const {
  return xpos<=parentx && parentx<xpos+width && ypos<=parenty && parenty<ypos+height;
  }


// Return child window with given window key
FXWindow* FXWindow::getChildWithKey(FXuint k) const {
  for(FXWindow *child=first; child; child=child->next){
    if(child->getKey()==k) return child;
    }
  return nullptr;
  }


// Return true if this window contains child in its subtree
FXbool FXWindow::containsChild(const FXWindow* child) const {
  while(child){
    if(child==this) return true;
    child=child->parent;
    }
  return false;
  }


// Return true if specified window is owned by this window
FXbool FXWindow::isOwnerOf(const FXWindow* window) const {
  while(window){
    if(window==this) return true;
    window=window->owner;
    }
  return false;
  }


// Return true if specified window is ancestor of this window
FXbool FXWindow::isChildOf(const FXWindow* window) const {
  const FXWindow* child=this;
  while(child){
    child=child->parent;
    if(child==window) return true;
    }
  return false;
  }


// Get child at x,y
FXWindow* FXWindow::getChildAt(FXint x,FXint y) const {
  FXWindow *child;
  if(0<=x && 0<=y && x<width && y<height){
    for(child=last; child; child=child->prev){
      if(child->shown() && child->xpos<=x && child->ypos<=y && x<child->xpos+child->width && y<child->ypos+child->height) return child;
      }
    }
  return nullptr;
  }


// Count number of children
FXint FXWindow::numChildren() const {
  const FXWindow *child=first;
  FXint num=0;
  while(child){
    child=child->next;
    num++;
    }
  return num;
  }


// Get index of child window
FXint FXWindow::indexOfChild(const FXWindow *window) const {
  FXint index=0;
  if(!window || window->parent!=this) return -1;
  while(window->prev){
    window=window->prev;
    index++;
    }
  return index;
  }


// Get child window at index
FXWindow* FXWindow::childAtIndex(FXint index) const {
  FXWindow* child=first;
  if(index<0) return nullptr;
  while(index && child){
    child=child->next;
    index--;
    }
  return child;
  }


// Find common ancestor between window a and b
FXWindow* FXWindow::commonAncestor(FXWindow* a,FXWindow* b){
  FXWindow *p1,*p2;
  if(a || b){
    if(!a) return b->getRoot();
    if(!b) return a->getRoot();
    p1=a;
    while(p1){
      p2=b;
      while(p2){
        if(p2==p1) return p1;
        p2=p2->parent;
        }
      p1=p1->parent;
      }
    }
  return nullptr;
  }


// Return true if sibling a <= sibling b in list
FXbool FXWindow::before(const FXWindow *a,const FXWindow* b){
  while(a!=b && a) a=a->next;
  return a==b;
  }


// Return true if sibling a >= sibling b in list
FXbool FXWindow::after(const FXWindow *a,const FXWindow* b){
  while(a!=b && b) b=b->next;
  return a==b;
  }


// Return true if window is a shell window
FXbool FXWindow::isShell() const {
  return (flags&FLAG_SHELL)!=0;
  }


/*******************************************************************************/


// Handle repaint
long FXWindow::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);
  return 1;
  }


// Window was mapped to screen
long FXWindow::onMap(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_CONFIGURE,"%s::onMap %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_MAP,message),ptr);
  }


// Window was unmapped; the grab is lost
long FXWindow::onUnmap(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_CONFIGURE,"%s::onUnmap %p\n",getClassName(),this);
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  return target && target->tryHandle(this,FXSEL(SEL_UNMAP,message),ptr);
  }


// Handle configure notify
long FXWindow::onConfigure(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_CONFIGURE,"%s::onConfigure %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_CONFIGURE,message),ptr);
  }


// The window was destroyed; the grab is lost
long FXWindow::onDestroy(FXObject*,FXSelector,void*){
  FXTRACE(TOPIC_CONFIGURE,"%s::onDestroy %p\n",getClassName(),this);
  getApp()->hash.remove((FXptr)xid);
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
  if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
  flags&=~FLAG_FOCUSED;
  xid=0;
  return 1;
  }


// Left button pressed
long FXWindow::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Left button released
long FXWindow::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Middle button released
long FXWindow::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Middle button pressed
long FXWindow::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Right button pressed
long FXWindow::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Right button released
long FXWindow::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Mouse motion
long FXWindow::onMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_MOTION,message),ptr);
  }


// Mouse wheel
long FXWindow::onMouseWheel(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_MOUSEWHEEL,message),ptr);
  }


// Keyboard press
long FXWindow::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_KEYBOARD,"%s::onKeyPress %p keysym=0x%04x state=%04x\n",getClassName(),this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state);
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr);
  }


// Keyboard release
long FXWindow::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_KEYBOARD,"%s::onKeyRelease %p keysym=0x%04x state=%04x\n",getClassName(),this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state);
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr);
  }


// Space ball motion
long FXWindow::onSpaceBallMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLMOTION,message),ptr);
  }


// Space ball button press
long FXWindow::onSpaceBallButtonPress(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLBUTTONPRESS,message),ptr);
  }


// Space ball button release
long FXWindow::onSpaceBallButtonRelease(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLBUTTONRELEASE,message),ptr);
  }


// Start a drag operation
long FXWindow::onBeginDrag(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_BEGINDRAG,message),ptr);
  }


// End drag operation
long FXWindow::onEndDrag(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_ENDDRAG,message),ptr);
  }


// Dragged stuff around
long FXWindow::onDragged(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DRAGGED,message),ptr);
  }


// Entering window
long FXWindow::onEnter(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onEnter %p (%s)\n",getClassName(),this, (((FXEvent*)ptr)->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (((FXEvent*)ptr)->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (((FXEvent*)ptr)->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?");
  if(((FXEvent*)ptr)->code!=CROSSINGGRAB){
    if(!(((FXEvent*)ptr)->state&(SHIFTMASK|CONTROLMASK|METAMASK|LEFTBUTTONMASK|MIDDLEBUTTONMASK|RIGHTBUTTONMASK))) flags|=FLAG_TIP;
    flags|=FLAG_HELP;
    }
  if(isEnabled() && target){ target->tryHandle(this,FXSEL(SEL_ENTER,message),ptr); }
  return 1;
  }


// Leaving window
long FXWindow::onLeave(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onLeave %p (%s)\n",getClassName(),this, (((FXEvent*)ptr)->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (((FXEvent*)ptr)->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (((FXEvent*)ptr)->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?");
  if(((FXEvent*)ptr)->code!=CROSSINGUNGRAB){
    flags&=~(FLAG_TIP|FLAG_HELP);
    }
  if(isEnabled() && target){ target->tryHandle(this,FXSEL(SEL_LEAVE,message),ptr); }
  return 1;
  }


// We were asked about tip text
long FXWindow::onQueryTip(FXObject* sender,FXSelector,void* ptr){
  return (flags&FLAG_TIP) && target && target->tryHandle(sender,FXSEL(SEL_QUERY_TIP,message),ptr);
  }


// We were asked about status text
long FXWindow::onQueryHelp(FXObject* sender,FXSelector,void* ptr){
  return (flags&FLAG_HELP) && target && target->tryHandle(sender,FXSEL(SEL_QUERY_HELP,message),ptr);
  }


// True if window under the cursor
FXbool FXWindow::underCursor() const {
  return (getApp()->cursorWindow==this);
  }


// Handle drag-and-drop enter
long FXWindow::onDNDEnter(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onDNDEnter %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_DND_ENTER,message),ptr);
  }


// Handle drag-and-drop leave
long FXWindow::onDNDLeave(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onDNDLeave %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_DND_LEAVE,message),ptr);
  }


// Handle drag-and-drop motion
long FXWindow::onDNDMotion(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onDNDMotion %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_DND_MOTION,message),ptr);
  }


// Handle drag-and-drop drop
long FXWindow::onDNDDrop(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onDNDDrop %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_DND_DROP,message),ptr);
  }


// Request for DND data
long FXWindow::onDNDRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onDNDRequest %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_DND_REQUEST,message),ptr);
  }


// Show window
long FXWindow::onCmdShow(FXObject*,FXSelector,void*){
  if(!shown()){ show(); recalc(); }
  return 1;
  }


// Hide window
long FXWindow::onCmdHide(FXObject*,FXSelector,void*){
  if(shown()){ hide(); recalc(); }
  return 1;
  }

// Hide or show window
long FXWindow::onCmdToggleShown(FXObject*,FXSelector,void*){
  shown() ? hide() : show();
  recalc();
  return 1;
  }


// Update hide or show window
long FXWindow::onUpdToggleShown(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETVALUE),(void*)(FXuval)shown());
  return 1;
  }


// Raise window
long FXWindow::onCmdRaise(FXObject*,FXSelector,void*){
  raise();
  return 1;
  }


// Lower window
long FXWindow::onCmdLower(FXObject*,FXSelector,void*){
  lower();
  return 1;
  }


// Delete window
long FXWindow::onCmdDelete(FXObject*,FXSelector,void*){
  delete this;
  return 1;
  }


// Enable window
long FXWindow::onCmdEnable(FXObject*,FXSelector,void*){
  enable();
  return 1;
  }


// Disable window
long FXWindow::onCmdDisable(FXObject*,FXSelector,void*){
  disable();
  return 1;
  }


// Disable or enable window
long FXWindow::onCmdToggleEnabled(FXObject*,FXSelector,void*){
  isEnabled() ? disable() : enable();
  return 1;
  }


// Update disable or enable window
long FXWindow::onUpdToggleEnabled(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETVALUE),(void*)(FXuval)isEnabled());
  return 1;
  }


// In combination with autograying/autohiding widgets,
// this could be used to ungray or show when you don't want
// to write a whole handler routine just to do this.
long FXWindow::onUpdYes(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  return 1;
  }


// Update (repaint) window
long FXWindow::onCmdUpdate(FXObject*,FXSelector,void*){
  update();
  return 1;
  }


/*******************************************************************************/

// Gained focus
long FXWindow::onFocusIn(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onFocusIn %p\n",getClassName(),this);
  flags|=FLAG_FOCUSED;
  if(target){ target->tryHandle(this,FXSEL(SEL_FOCUSIN,message),ptr); }
  if(composeContext){ composeContext->focusIn(); }
  if(focus){ focus->handle(focus,FXSEL(SEL_FOCUSIN,0),nullptr); }
  return 1;
  }


// Lost focus
long FXWindow::onFocusOut(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onFocusOut %p\n",getClassName(),this);
  if(focus){ focus->handle(focus,FXSEL(SEL_FOCUSOUT,0),nullptr); }
  if(composeContext){ composeContext->focusOut(); }
  if(target){ target->tryHandle(this,FXSEL(SEL_FOCUSOUT,message),ptr); }
  flags&=~FLAG_FOCUSED;
  return 1;
  }


// Focus on widget itself, if its enabled
long FXWindow::onFocusSelf(FXObject*,FXSelector,void*){
  FXTRACE(TOPIC_EVENT,"%s::onFocusSelf %p\n",getClassName(),this);
  if(isEnabled() && canFocus()){ setFocus(); return 1; }
  return 0;
  }


// Create compose context
void FXWindow::createComposeContext(){
  if(!composeContext){
    composeContext=new FXComposeContext(getApp(),this,0);
    composeContext->create();
    }
  }


// Destroy compose context
void FXWindow::destroyComposeContext(){
  if(composeContext){
    delete composeContext;
    composeContext=nullptr;
    }
  }


// If window can have focus
FXbool FXWindow::canFocus() const { return false; }


// Has window the focus
FXbool FXWindow::hasFocus() const {
  return (flags&FLAG_FOCUSED)!=0;
  }


// Window is in focus chain
FXbool FXWindow::inFocusChain() const {
  return parent->focus==this;
  }


// Set focus to this widget.
// The chain of focus from shell down to a control is changed.
// Widgets now in the chain may or may not gain real focus,
// depending on whether parent window already had a real focus!
// Setting the focus to a composite will cause descendants to loose it.
void FXWindow::setFocus(){
  FXTRACE(TOPIC_EVENT,"%s::setFocus %p\n",getClassName(),this);
  if(parent && parent->focus!=this){
    if(parent->focus) parent->focus->killFocus(); else parent->setFocus();
    parent->focus=this;
    if(parent->hasFocus()) handle(this,FXSEL(SEL_FOCUSIN,0),nullptr);
    }
  flags|=FLAG_HELP;
  }


// Kill focus to this widget.
void FXWindow::killFocus(){
  FXTRACE(TOPIC_EVENT,"%s::killFocus %p\n",getClassName(),this);
  if(parent && parent->focus==this){
    if(focus) focus->killFocus();
    if(hasFocus()) handle(this,FXSEL(SEL_FOCUSOUT,0),nullptr);
    parent->focus=nullptr;
    }
  flags&=~FLAG_HELP;
  }


// Return true if widget is drawn as default
FXbool FXWindow::isDefault() const {
  return (flags&FLAG_DEFAULT)!=0;
  }


// Make widget drawn as default
void FXWindow::setDefault(FXuchar flag){
  FXWindow *win;
  switch(flag){
    case false:
      flags&=~FLAG_DEFAULT;
      break;
    case true:
      if(!(flags&FLAG_DEFAULT)){
        win=getShell()->findDefault();
        if(win) win->setDefault(false);
        flags|=FLAG_DEFAULT;
        }
      break;
    case maybe:
      if(flags&FLAG_DEFAULT){
        flags&=~FLAG_DEFAULT;
        win=getShell()->findInitial();
        if(win) win->setDefault(true);
        }
      break;
    }
  }

// Find default window
FXWindow* FXWindow::findDefault() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  while(1){
    if(win->isDefault()) return win;
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  return nullptr;
  }



// Return true if this is the initial default window
FXbool FXWindow::isInitial() const {
  return (flags&FLAG_INITIAL)!=0;
  }


// Make this window the initial default window
void FXWindow::setInitial(FXbool flag){
  FXWindow *win;
  if((flags&FLAG_INITIAL) && !flag){
    flags&=~FLAG_INITIAL;
    }
  if(!(flags&FLAG_INITIAL) && flag){
    win=getShell()->findInitial();
    if(win) win->setInitial(false);
    flags|=FLAG_INITIAL;
    }
  }


// Find initial window
FXWindow* FXWindow::findInitial() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  while(1){
    if(win->isInitial()) return win;
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  return nullptr;
  }

/*******************************************************************************/

// Create X window
void FXWindow::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Gotta have a parent already created!
      if(!parent->id()){ fxerror("%s::create: trying to create window before creating parent window.\n",getClassName()); }

      // If window has owner, owner should have been created already
      if(owner && !owner->id()){ fxerror("%s::create: trying to create window before creating owner window.\n",getClassName()); }

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

      // Initialize visual
      visual->create();

      // Create default cursor
      if(defaultCursor) defaultCursor->create();

      // Create drag cursor
      if(dragCursor) dragCursor->create();

      // Describe the native window; the platform module owns the frame style
      FXWindowCreateInfo info;
      info.display=getApp()->getDisplay();
      info.instance=getApp()->getDisplay();
      info.classname=GetClass();
      info.userdata=this;
      info.parent=(FXID)parent->id();
      info.owner=owner?(FXID)owner->id():0;
      info.ownershell=owner?(FXID)owner->getShell()->id():0;
      info.shell=isShell()?0:(FXID)getShell()->id();
      info.colormap=(FXID)visual->colormap;
      info.cursor=defaultCursor?(FXID)defaultCursor->id():0;
      info.xdndaware=(FXID)getApp()->xdndAware;
      info.x=xpos;
      info.y=ypos;
      info.w=width;
      info.h=height;
      info.visual=visual;
      info.flags=0;
      if(isShell()) info.flags|=WINDOWINFO_SHELL;
      if(isMemberOf(FXMETACLASS(FXTopWindow))) info.flags|=WINDOWINFO_TOPWINDOW;
      if(doesOverrideRedirect()) info.flags|=WINDOWINFO_OVERRIDEREDIRECT;
      if(doesSaveUnder()) info.flags|=WINDOWINFO_SAVEUNDER;
      if(isEnabled()) info.flags|=WINDOWINFO_ENABLED;
      if(shown()) info.flags|=WINDOWINFO_SHOWN;

      // Create this window
      xid=fxplatformCreateWindow(info);

      // Uh-oh, we failed
      if(!xid){ throw FXWindowException("unable to create window."); }

      // Remember the DPI of the monitor this window is on; platforms without
      // per-window DPI report nothing, leaving it unknown.
      FXint wdpi=getApp()->getWindowDPI(xid);
      if(wdpi>0){
        dpi=wdpi;

        // Realize fonts for this window's monitor
        applyTheme(getApp()->getTheme());
        }

      // Store for xid to C++ object mapping
      getApp()->hash.insert((FXptr)xid,this);
      flags|=FLAG_OWNED;
      }
    }
  }


// Attach to a window belonging to another application
void FXWindow::attach(FXID w){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::attach %p\n",getClassName(),this);

      // Gotta have a parent already created!
      if(!parent->id()){ fxerror("%s::attach: trying to attach window before creating parent window.\n",getClassName()); }

      // If window has owner, owner should have been created already
      if(owner && !owner->id()){ fxerror("%s::attach: trying to attach window before creating owner window.\n",getClassName()); }

      // Got to have a visual
      if(!visual){ fxerror("%s::attach: trying to attach window without a visual.\n",getClassName()); }

      // Uh-oh, we failed
      if(!w){ throw FXWindowException("unable to attach window."); }

      // Initialize visual
      visual->create();

      // Create default cursor
      if(defaultCursor) defaultCursor->create();

      // Create drag cursor
      if(dragCursor) dragCursor->create();

      // Simply assign to xid
      xid=w;

      // Store for xid to C++ object mapping
      getApp()->hash.insert((FXptr)xid,this);

      // Take over the foreign window
      FXWindowCreateInfo info;
      info.display=getApp()->getDisplay();
      info.instance=getApp()->getDisplay();
      info.classname=GetClass();
      info.userdata=this;
      info.parent=(FXID)parent->id();
      info.owner=0;
      info.ownershell=0;
      info.shell=isShell()?0:(FXID)getShell()->id();
      info.colormap=(FXID)visual->colormap;
      info.cursor=defaultCursor?(FXID)defaultCursor->id():0;
      info.xdndaware=(FXID)getApp()->xdndAware;
      info.x=xpos;
      info.y=ypos;
      info.w=width;
      info.h=height;
      info.visual=visual;
      info.flags=0;
      if(isShell()) info.flags|=WINDOWINFO_SHELL;
      if(isEnabled()) info.flags|=WINDOWINFO_ENABLED;
      fxplatformAttachWindow(info,xid);

      flags&=~FLAG_OWNED;
      }
    }
  }


// Detach window
void FXWindow::detach(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((FXptr)xid);

      // Detach visual
      visual->detach();

      // Detach default cursor
      if(defaultCursor) defaultCursor->detach();

      // Detach drag cursor
      if(dragCursor) dragCursor->detach();
      }

    // No longer grabbed
    if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
    if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
    if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
    if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
    flags&=~FLAG_FOCUSED;
    flags&=~FLAG_OWNED;
    xid=0;
    }
  }


// Destroy; only destroy window if we own it
void FXWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((FXptr)xid);

      // Its our own window, so destroy it
      if(flags&FLAG_OWNED){
        fxplatformDestroyWindow(getApp()->getDisplay(),xid,(FXID)getShell()->id(),(FXID)visual->colormap,(FXID)getApp()->xdndAware);
        }
      }

    // No longer grabbed
    if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
    if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
    if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
    if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
    flags&=~FLAG_FOCUSED;
    flags&=~FLAG_OWNED;
    xid=0;
    }
  }


/*******************************************************************************/


// Test if active
FXbool FXWindow::isActive() const {
  return (flags&FLAG_ACTIVE)!=0;
  }


// Get default width
FXint FXWindow::getDefaultWidth(){
  return 1;
  }


// Get default height
FXint FXWindow::getDefaultHeight(){
  return 1;
  }


// Return width for given height
FXint FXWindow::getWidthForHeight(FXint){
  return getDefaultWidth();
  }


// Return height for given width
FXint FXWindow::getHeightForWidth(FXint){
  return getDefaultHeight();
  }


// Set X position
void FXWindow::setX(FXint x){
  xpos=x;
  recalc();
  }


// Set Y position
void FXWindow::setY(FXint y){
  ypos=y;
  recalc();
  }


// Set width
void FXWindow::setWidth(FXint w){
  if(w<0) w=0;
  width=w;
  recalc();
  }


// Set height
void FXWindow::setHeight(FXint h){
  if(h<0) h=0;
  height=h;
  recalc();
  }


// Change layout
void FXWindow::setLayoutHints(FXuint lout){
  FXuint opts=(options&~LAYOUT_MASK) | (lout&LAYOUT_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get layout hints
FXuint FXWindow::getLayoutHints() const {
  return (options&LAYOUT_MASK);
  }


// Is widget a composite
FXbool FXWindow::isComposite() const {
  return false;
  }


// Window does override-redirect
FXbool FXWindow::doesOverrideRedirect() const {
  return false;
  }


// Window does save-unders
FXbool FXWindow::doesSaveUnder() const {
  return false;
  }


/*******************************************************************************/


// Add hot key to closest ancestor's accelerator table
void FXWindow::addHotKey(FXHotKey code){
  if(code){
    for(FXWindow *win=this; win; win=win->parent){
      if(win->getAccelTable()){
        win->getAccelTable()->addAccel(code,this,FXSEL(SEL_KEYPRESS,ID_HOTKEY),FXSEL(SEL_KEYRELEASE,ID_HOTKEY));
        break;
        }
      }
    }
  }


// Remove hot key from closest ancestor's accelerator table
void FXWindow::remHotKey(FXHotKey code){
  if(code){
    for(FXWindow *win=this; win; win=win->parent){
      if(win->getAccelTable()){
        win->getAccelTable()->removeAccel(code);
        break;
        }
      }
    }
  }


/*******************************************************************************/


// Is cursor shown
FXbool FXWindow::cursorShown() const {
  return (flags&FLAG_CURSOR)!=0;
  }


// Set window background
// Re-apply theme colors to this window
void FXWindow::applyTheme(FXTheme*){
  }


void FXWindow::setBackColor(FXColor clr){
  if(clr!=backColor){
    backColor=clr;
    backColorOverride=true;
//    if(xid){
//#ifdef WIN32
//      // FIXME //
//#else
//      XSetWindowBackground((Display*)getApp()->getDisplay(),xid,visual->getPixel(backColor));
//#endif
//      }
    update();
    }
  }



/*******************************************************************************/


// Lost the selection
long FXWindow::onSelectionLost(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onSelectionLost %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_LOST,message),ptr);
  }


// Gained the selection
long FXWindow::onSelectionGained(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onSelectionGained %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_GAINED,message),ptr);
  }


// Somebody wants our the selection
long FXWindow::onSelectionRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onSelectionRequest %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_REQUEST,message),ptr);
  }


// Has this window the selection
FXbool FXWindow::hasSelection() const {
  return (getApp()->selectionWindow==this);
  }


/*******************************************************************************/


// Lost the selection
long FXWindow::onClipboardLost(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onClipboardLost %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_LOST,message),ptr);
  }


// Gained the selection
long FXWindow::onClipboardGained(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onClipboardGained %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_GAINED,message),ptr);
  }


// Somebody wants our the selection
long FXWindow::onClipboardRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_EVENT,"%s::onClipboardRequest %p\n",getClassName(),this);
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_REQUEST,message),ptr);
  }


// Has this window the selection
FXbool FXWindow::hasClipboard() const {
  return (getApp()->clipboardWindow==this);
  }


/*******************************************************************************/


// Update this widget by sending SEL_UPDATE to its target.
// If there is no target, onUpdate returns 0 on behalf of widgets
// which have the autogray feature enabled.  If there is a target
// but we're not updating because the user is manipulating the
// widget, then onUpdate returns 1 to prevent it from graying
// out during manipulation when the autogray feature is enabled.
// Otherwise, onUpdate returns the value returned by the SEL_UPDATE
// callback to the target.
long FXWindow::onUpdate(FXObject*,FXSelector,void*){
  return target && (!(flags&FLAG_UPDATE) || target->tryHandle(this,FXSEL(SEL_UPDATE,message),nullptr));
  }


// Perform layout immediately
void FXWindow::layout(){
  flags&=~FLAG_DIRTY;
  }


// Mark this window's layout as dirty for later layout
void FXWindow::recalc(){
  if(parent) parent->recalc();
  flags|=FLAG_DIRTY;
  }


// Force GUI refresh of this window and all of its children
void FXWindow::forceRefresh(){
  FXWindow *win=this;
  while(1){
    win->handle(win,FXSEL(SEL_UPDATE,0),nullptr);
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  }


// Update dirty window
void FXWindow::update() const {
  update(0,0,width,height);
  }


// If marked but not yet painted, paint the entire window
void FXWindow::repaint() const {
  repaint(0,0,width,height);
  }


// Move window
void FXWindow::move(FXint x,FXint y){
  FXTRACE(TOPIC_DETAIL,"%s::move: x=%d y=%d\n",getClassName(),x,y);
  if((flags&FLAG_DIRTY)||(x!=xpos)||(y!=ypos)){
    xpos=x;
    ypos=y;
    if(xid){

      // Similar as for position(), we have to generate protocol
      // here so as to make the display reflect reality...
      fxplatformPlaceWindow(getApp()->getDisplay(),xid,x,y,width,height,width,height,shown(),PLACEMENT_X|PLACEMENT_Y);
      if(flags&FLAG_DIRTY) layout();
      }
    }
  }


// Move and resize
void FXWindow::position(FXint x,FXint y,FXint w,FXint h){
  FXint ow=width;
  FXint oh=height;
  FXTRACE(TOPIC_DETAIL,"%s::position: x=%d y=%d w=%d h=%d\n",getClassName(),x,y,w,h);
  if(w<0) w=0;
  if(h<0) h=0;
  if((flags&FLAG_DIRTY)||(x!=xpos)||(y!=ypos)||(w!=ow)||(h!=oh)){
    xpos=x;
    ypos=y;
    width=w;
    height=h;
    if(xid){

      // Alas, we have to generate some protocol here even if the placement
      // as recorded in the widget hasn't actually changed.  This is because
      // there are ways to change the placement w/o going through position()!
      fxplatformPlaceWindow(getApp()->getDisplay(),xid,x,y,w,h,ow,oh,shown(),PLACEMENT_X|PLACEMENT_Y|PLACEMENT_W|PLACEMENT_H);

      // We don't have to layout the interior of this widget unless
      // the size has changed or it was marked as dirty:- this is
      // a very good optimization as it's applied recursively!
      if((flags&FLAG_DIRTY)||(w!=ow)||(h!=oh)) layout();
      }
    }
  }


// Resize
void FXWindow::resize(FXint w,FXint h){
  FXint ow=width;
  FXint oh=height;
  FXTRACE(TOPIC_DETAIL,"%s::resize: w=%d h=%d\n",getClassName(),w,h);
  if(w<0) w=0;
  if(h<0) h=0;
  if((flags&FLAG_DIRTY)||(w!=ow)||(h!=oh)){
    width=w;
    height=h;
    if(xid){

      // Similar as for position(), we have to generate protocol here..
      fxplatformPlaceWindow(getApp()->getDisplay(),xid,xpos,ypos,w,h,ow,oh,shown(),PLACEMENT_W|PLACEMENT_H);

      // And of course the size has changed so layout is needed
      layout();
      }
    }
  }


// Check if logically shown
FXbool FXWindow::shown() const {
  return (flags&FLAG_SHOWN)!=0;
  }


// Show window
void FXWindow::show(){
  FXTRACE(TOPIC_DETAIL,"%s::show %p\n",getClassName(),this);
  if(!shown()){
    flags|=FLAG_SHOWN;
    if(xid){
      fxplatformShowWindow(getApp()->getDisplay(),xid,width,height,true);
      }
    }
  }


// Hide window
void FXWindow::hide(){
  FXTRACE(TOPIC_DETAIL,"%s::hide %p\n",getClassName(),this);
  if(shown()){
    killFocus();
    flags&=~FLAG_SHOWN;
    if(xid){
      if(getApp()->mouseGrabWindow==this){
        fxplatformReleaseMouseGrab(getApp()->getDisplay(),defaultCursor?(FXID)defaultCursor->id():0);
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        fxplatformReleaseKeyboardGrab(getApp()->getDisplay(),getApp()->event.time);
        getApp()->keyboardGrabWindow=nullptr;
        }
      fxplatformShowWindow(getApp()->getDisplay(),xid,width,height,false);
      }
    }
  }


// Reparent window under a new parent
void FXWindow::reparent(FXWindow* father,FXWindow* other){
  FXbool hadfocus=inFocusChain();
  if(!father){ fxerror("%s::reparent: NULL parent specified.\n",getClassName()); }
  if(!parent){ fxerror("%s::reparent: cannot reparent root window.\n",getClassName()); }
  if(parent==getRoot() || father==getRoot()){ fxerror("%s::reparent: cannot reparent toplevel window.\n",getClassName()); }
  if(other && father!=other->getParent()){ fxerror("%s::reparent: other window has different parent.\n",getClassName()); }
  if(this!=other){

    // Check for funny cases
    if(containsChild(father)){ fxerror("%s::reparent: new parent is child of window.\n",getClassName()); }

    // Both windows created or both non-created
    if(xid && !father->id()){ fxerror("%s::reparent: new parent not created yet.\n",getClassName()); }
    if(!xid && father->id()){ fxerror("%s::reparent: window not created yet.\n",getClassName()); }

    // Kill focus chain through this window
    if(hadfocus) killFocus();

    // Recalc old path
    recalc();

    // Unlink from old parent
    if(prev) prev->next=next; else parent->first=next;
    if(next) next->prev=prev; else parent->last=prev;

    // Link to new parent
    if(other){
      next=other;
      prev=other->prev;
      other->prev=this;
      }
    else{
      next=nullptr;
      prev=father->last;
      father->last=this;
      }
    if(prev){
      prev->next=this;
      }
    else{
      father->first=this;
      }

    // Moved between parents
    if(parent!=father){

      // New parent and owner
      parent=father;
      owner=father;

      // Hook up to new window in server too
      if(xid && parent->id()){
        fxplatformReparentWindow(getApp()->getDisplay(),xid,(FXID)parent->id());
        }
      }

    // Set focus back if we had it
    if(hadfocus) setFocus();

    // Recalc new path
    recalc();
    }
  }


// Is window enabled
FXbool FXWindow::isEnabled() const {
  return (flags&FLAG_ENABLED)!=0;
  }


// Raise (but do not activate!)
void FXWindow::raise(){
  if(xid){
    fxplatformRaiseWindow(getApp()->getDisplay(),xid,true);
    }
  }


// Lower
void FXWindow::lower(){
  if(xid){
    fxplatformRaiseWindow(getApp()->getDisplay(),xid,false);
    }
  }


#if 0
// Get coordinates from another window (for symmetry)
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  if(!fromwindow){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  for(const FXWindow* w=fromwindow; w; w=w->getParent()){
    fromx+=w->getX();
    fromy+=w->getY();
    }
  for(const FXWindow* w=this; w; w=w->getParent()){
    fromx-=w->getX();
    fromy-=w->getY();
    }
  tox=fromx;
  toy=fromy;
  }


// Get coordinates to another window (for symmetry)
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  if(!towindow){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  for(const FXWindow* w=this; w; w=w->getParent()){
    fromx+=w->getX();
    fromy+=w->getY();
    }
  for(const FXWindow* w=towindow; w; w=w->getParent()){
    fromx-=w->getX();
    fromy-=w->getY();
    }
  tox=fromx;
  toy=fromy;
  }
#endif

/*******************************************************************************/


// Return true if active grab is in effect
FXbool FXWindow::grabbed() const {
  return getApp()->mouseGrabWindow==this;
  }


// Return true if active grab is in effect
FXbool FXWindow::grabbedKeyboard() const {
  return getApp()->keyboardGrabWindow==this;
  }


// The widget lost the grab for some reason [Windows].
// Subclasses should try to clean up the mess...
long FXWindow::onUngrabbed(FXObject*,FXSelector,void* ptr){
  FXTRACE(TOPIC_DETAIL,"%s::onUngrabbed\n",getClassName());
  if(target) target->tryHandle(this,FXSEL(SEL_UNGRABBED,message),ptr);
//#ifdef WIN32
//  SetCursor((HCURSOR)defaultCursor->id());    // FIXME Maybe should be done with WM_SETCURSOR?
//#endif
  return 1;
  }


/*******************************************************************************/


// Translate message
const FXchar* FXWindow::tr(const FXchar* text,const FXchar* hint,FXint count) const {
  FXTranslator *translator=getApp()->getTranslator();
  if(translator){
    return translator->tr(getClassName(),text,hint,count);
    }
  return text;
  }


/*******************************************************************************/


// Is window drop enabled
FXbool FXWindow::isDropEnabled() const {
  return (flags&FLAG_DROPTARGET)!=0;
  }


// Enable widget as drop target
void FXWindow::dropEnable(){
  flags|=FLAG_DROPTARGET;
  }


// Disable widget as drop target
void FXWindow::dropDisable(){
  flags&=~FLAG_DROPTARGET;
  }


// Get dropped data; called in response to DND enter or DND drop
FXbool FXWindow::getDNDData(FXDNDOrigin origin,FXDragType targettype,FXuchar*& ptr,FXuint& size) const {
  if(xid==0){ fxerror("%s::getDNDData: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropGetData(this,targettype,ptr,size);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardGetData(this,targettype,ptr,size);
      break;
    case FROM_SELECTION:
      getApp()->selectionGetData(this,targettype,ptr,size);
      break;
    }
  return ptr!=nullptr;
  }


// Get dropped string; called in response to DND enter or DND drop
FXbool FXWindow::getDNDData(FXDNDOrigin origin,FXDragType targettype,FXString& string) const {
  FXuchar *ptr; FXuint size;
  string=FXString::null;
  if(getDNDData(origin,targettype,ptr,size)){
    if(targettype==utf16Type){                                  // UTF16
      FXUTF16LECodec unicode;
      string.length(unicode.mb2utflen((FXchar*)ptr,size));
      unicode.mb2utf(string.text(),string.length(),(FXchar*)ptr,size);
      }
    else if(targettype==textType || targettype==stringType){    // ASCII
      FX88591Codec ascii;
      string.length(ascii.mb2utflen((FXchar*)ptr,size));
      ascii.mb2utf(string.text(),string.length(),(FXchar*)ptr,size);
      }
    else{                                                       // AS-IS
      string.assign((FXchar*)ptr,size);
      }
    freeElms(ptr);
    return true;
    }
  return false;
  }


// Set drop data; data array will be deleted by the system automatically!
FXbool FXWindow::setDNDData(FXDNDOrigin origin,FXDragType targettype,FXuchar* ptr,FXuint size) const {
  if(xid==0){ fxerror("%s::setDNDData: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropSetData(this,targettype,ptr,size);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardSetData(this,targettype,ptr,size);
      break;
    case FROM_SELECTION:
      getApp()->selectionSetData(this,targettype,ptr,size);
      break;
    }
  return true;
  }


// Set drop data; data array will be deleted by the system automatically!
FXbool FXWindow::setDNDData(FXDNDOrigin origin,FXDragType targettype,const FXString& string) const {
  FXuchar* ptr; FXuint size;
  if(targettype==utf16Type){                                    // UTF16
    FXUTF16LECodec unicode;
    size=unicode.utf2mblen(string.text(),string.length());
    if(callocElms(ptr,size+2)){
      unicode.utf2mb((FXchar*)ptr,size,string.text(),string.length());
      return setDNDData(origin,targettype,ptr,size);
      }
    }
  else if(targettype==textType || targettype==stringType){      // ASCII
    FX88591Codec ascii;
    size=ascii.utf2mblen(string.text(),string.length());
    if(callocElms(ptr,size+2)){
      ascii.utf2mb((FXchar*)ptr,size,string.text(),string.length());
      return setDNDData(origin,targettype,ptr,size);
      }
    }
  else{                                                         // AS-IS
    size=string.length();
    if(callocElms(ptr,size+2)){
      memcpy(ptr,string.text(),string.length());
      return setDNDData(origin,targettype,ptr,size);
      }
    }
  return false;
  }

// Inquire about types being dragged or available on the clipboard or selection
FXbool FXWindow::inquireDNDTypes(FXDNDOrigin origin,FXDragType*& types,FXuint& numtypes) const {
  if(xid==0){ fxerror("%s::inquireDNDTypes: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropGetTypes(this,types,numtypes);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardGetTypes(this,types,numtypes);
      break;
    case FROM_SELECTION:
      getApp()->selectionGetTypes(this,types,numtypes);
      break;
    }
  return types!=nullptr;
  }


// Is a certain type being offered via drag and drop, clipboard, or selection?
FXbool FXWindow::offeredDNDType(FXDNDOrigin origin,FXDragType type) const {
  if(xid==0){ fxerror("%s::offeredDNDType: window has not yet been created.\n",getClassName()); }
  FXbool offered=false;
  FXDragType *types;
  FXuint i,ntypes;
  if(inquireDNDTypes(origin,types,ntypes)){
    for(i=0; i<ntypes; i++){
      if(type==types[i]){ offered=true; break; }
      }
    freeElms(types);
    }
  return offered;
  }


// Target accepts or rejects drop, and may suggest different action
void FXWindow::acceptDrop(FXDragAction action) const {
  getApp()->ansAction=DRAG_REJECT;
  if(action!=DRAG_REJECT){
    getApp()->ansAction=getApp()->ddeAction;
    if(action!=DRAG_ACCEPT) getApp()->ansAction=action;
    }
  }


// Target wants to know drag action of source
FXDragAction FXWindow::inquireDNDAction() const {
  return getApp()->ddeAction;
  }


// Source wants to find out if target accepted
FXDragAction FXWindow::didAccept() const {
  return getApp()->ansAction;
  }


// True if we're in a drag operation
FXbool FXWindow::isDragging() const {
  return (getApp()->dragWindow==this);
  }


// True if this window is drop target
FXbool FXWindow::isDropTarget() const {
  return (getApp()->dropWindow==this);
  }


/*******************************************************************************/


// Delete window
FXWindow::~FXWindow(){
  FXTRACE(TOPIC_CONSTRUCT,"FXWindow::~FXWindow %p\n",this);
  getApp()->windowCount--;
  destroy();
  delete accelTable;
  delete composeContext;
  if(prev) prev->next=next; else if(parent) parent->first=next;
  if(next) next->prev=prev; else if(parent) parent->last=prev;
  if(parent && parent->focus==this) parent->focus=nullptr;
  if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
  if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  if(getApp()->keyWindow==this) getApp()->keyWindow=nullptr;
  if(getApp()->selectionWindow==this) getApp()->selectionWindow=nullptr;
  if(getApp()->clipboardWindow==this) getApp()->clipboardWindow=nullptr;
  if(getApp()->dragWindow==this) getApp()->dragWindow=nullptr;
  if(getApp()->dropWindow==this) getApp()->dropWindow=nullptr;
  if(getApp()->refresherstop==this) getApp()->refresherstop=parent;
  if(getApp()->refresher==this) getApp()->refresher=parent;
  if(parent) parent->recalc();
  parent=(FXWindow*)-1L;
  owner=(FXWindow*)-1L;
  first=last=(FXWindow*)-1L;
  next=prev=(FXWindow*)-1L;
  focus=(FXWindow*)-1L;
  composeContext=(FXComposeContext*)-1L;
  defaultCursor=(FXCursor*)-1L;
  dragCursor=(FXCursor*)-1L;
  accelTable=(FXAccelTable*)-1L;
  target=(FXObject*)-1L;
  }

}
