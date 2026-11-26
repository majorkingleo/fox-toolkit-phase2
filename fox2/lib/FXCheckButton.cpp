/********************************************************************************
*                                                                               *
*                    C h e c k   B u t t o n    O b j e c t                     *
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
#include "FXCheckButton.h"

/*
  Notes:
  - Works as intended.
*/

#define CHECKBUTTON_MASK  (CHECKBUTTON_AUTOGRAY|CHECKBUTTON_AUTOHIDE|CHECKBUTTON_PLUS)

using namespace FX;

/*******************************************************************************/

namespace FX {


// Blend color a toward color b by the given percentage of b
static FXColor blendColor(FXColor a,FXColor b,FXint percent){
  FXint tnecrep=100-percent;
  return FXRGB((FXREDVAL(a)*tnecrep+FXREDVAL(b)*percent+50)/100,
               (FXGREENVAL(a)*tnecrep+FXGREENVAL(b)*percent+50)/100,
               (FXBLUEVAL(a)*tnecrep+FXBLUEVAL(b)*percent+50)/100);
  }


// Size of the check indicator for the current theme and display scale
static FXint indicatorSize(FXWindow* win){
  return FXMAX(win->getApp()->getTheme()->metric(METRIC_CHECK_SIZE,win->getDisplayScale()),9);
  }


// Map
FXDEFMAP(FXCheckButton) FXCheckButtonMap[]={
  FXMAPFUNC(SEL_UPDATE,0,FXCheckButton::onUpdate),
  FXMAPFUNC(SEL_PAINT,0,FXCheckButton::onPaint),
  FXMAPFUNC(SEL_ENTER,0,FXCheckButton::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXCheckButton::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,FXCheckButton::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXCheckButton::onFocusOut),
  FXMAPFUNC(SEL_UNGRABBED,0,FXCheckButton::onUngrabbed),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXCheckButton::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXCheckButton::onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXCheckButton::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXCheckButton::onKeyRelease),
  FXMAPFUNC(SEL_KEYPRESS,FXWindow::ID_HOTKEY,FXCheckButton::onHotKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,FXWindow::ID_HOTKEY,FXCheckButton::onHotKeyRelease),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_CHECK,FXCheckButton::onCheck),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_UNCHECK,FXCheckButton::onUncheck),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_UNKNOWN,FXCheckButton::onUnknown),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXCheckButton::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,FXCheckButton::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,FXCheckButton::onCmdGetIntValue),
  };


// Object implementation
FXIMPLEMENT(FXCheckButton,FXLabel,FXCheckButtonMap,ARRAYNUMBER(FXCheckButtonMap))



// Deserialization
FXCheckButton::FXCheckButton(){
  checkColor=0;
  boxColor=0;
  check=false;
  oldcheck=false;
  }


// Make a check button
FXCheckButton::FXCheckButton(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXLabel(p,text,nullptr,opts,x,y,w,h,pl,pr,pt,pb){
  checkColor=getApp()->getForeColor();
  boxColor=getApp()->getBackColor();
  target=tgt;
  message=sel;
  check=false;
  oldcheck=false;
  }


// If window can have focus
FXbool FXCheckButton::canFocus() const { return true; }


// Get default width
FXint FXCheckButton::getDefaultWidth(){
  FXint tw=0,s=0,w=indicatorSize(this);
  if(!label.empty()){
    tw=labelWidth(label);
    s=spacing;
    }
  if(!(options&(ICON_AFTER_TEXT|ICON_BEFORE_TEXT))) w=FXMAX(tw,indicatorSize(this)); else w=tw+indicatorSize(this)+s;
  return padleft+padright+w+(border<<1);
  }


// Get default height
FXint FXCheckButton::getDefaultHeight(){
  FXint th=0,h;
  if(!label.empty()){
    th=labelHeight(label);
    }
  if(!(options&(ICON_ABOVE_TEXT|ICON_BELOW_TEXT))) h=FXMAX(th,indicatorSize(this)); else h=th+indicatorSize(this);
  return padtop+padbottom+h+(border<<1);
  }


// Re-apply theme colors and indicator metrics
void FXCheckButton::applyTheme(FXTheme* theme){
  FXLabel::applyTheme(theme);
  if(0<theme->metric(METRIC_CORNER_RADIUS)){
    if(!boxColorOverride) boxColor=theme->color(ROLE_BUTTON);
    if(!checkColorOverride) checkColor=theme->color(ROLE_TEXT);
    }
  else{
    if(!boxColorOverride) boxColor=getApp()->getBackColor();
    if(!checkColorOverride) checkColor=getApp()->getForeColor();
    }
  }


// Check button
void FXCheckButton::setCheck(FXuchar state,FXbool notify){
  if(check!=state){
    check=state;
    update();
    if(notify && target){target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check);}
    }
  }


// Change state to checked
long FXCheckButton::onCheck(FXObject*,FXSelector,void*){
  setCheck(true);
  return 1;
  }


// Change state to unchecked
long FXCheckButton::onUncheck(FXObject*,FXSelector,void*){
  setCheck(false);
  return 1;
  }


// Change state to indeterminate
long FXCheckButton::onUnknown(FXObject*,FXSelector,void*){
  setCheck(maybe);
  return 1;
  }


// Update value from a message
long FXCheckButton::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCheck((FXuchar)(FXuval)ptr);
  return 1;
  }


// Update value from a message
long FXCheckButton::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCheck((FXuchar)*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXCheckButton::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCheck();
  return 1;
  }


// Implement auto-hide or auto-gray modes
long FXCheckButton::onUpdate(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXLabel::onUpdate(sender,sel,ptr)){
    if(options&CHECKBUTTON_AUTOHIDE){if(shown()){hide();recalc();}}
    if(options&CHECKBUTTON_AUTOGRAY){disable();}
    }
  return 1;
  }


// Gained focus
long FXCheckButton::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusIn(sender,sel,ptr);
  update(border,border,width-(border<<1),height-(border<<1));
  return 1;
  }


// Lost focus
long FXCheckButton::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusOut(sender,sel,ptr);
  update(border,border,width-(border<<1),height-(border<<1));
  return 1;
  }


// Entered button
long FXCheckButton::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onEnter(sender,sel,ptr);
  if(isEnabled() && (flags&FLAG_PRESSED)) setCheck(!oldcheck);
  if(0<getApp()->getTheme()->metric(METRIC_CORNER_RADIUS,getDisplayScale())) update();
  return 1;
  }


// Left button
long FXCheckButton::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onLeave(sender,sel,ptr);
  if(isEnabled() && (flags&FLAG_PRESSED)) setCheck(oldcheck);
  if(0<getApp()->getTheme()->metric(METRIC_CORNER_RADIUS,getDisplayScale())) update();
  return 1;
  }


// Pressed mouse button
long FXCheckButton::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  flags&=~FLAG_TIP;
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    oldcheck=check;
    setCheck(!oldcheck);
    flags|=FLAG_PRESSED;
    flags&=~FLAG_UPDATE;
    return 1;
    }
  return 0;
  }


// Released mouse button
long FXCheckButton::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled() && (flags&FLAG_PRESSED)){
    ungrab();
    flags|=FLAG_UPDATE;
    flags&=~FLAG_PRESSED;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(check!=oldcheck && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check); }
    return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long FXCheckButton::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onUngrabbed(sender,sel,ptr);
  setCheck(oldcheck);
  flags&=~FLAG_PRESSED;
  flags|=FLAG_UPDATE;
  return 1;
  }


// Key Press
long FXCheckButton::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    if(!(flags&FLAG_PRESSED) && (event->code==KEY_space || event->code==KEY_KP_Space)){
      oldcheck=check;
      setCheck(!oldcheck);
      flags|=FLAG_PRESSED;
      flags&=~FLAG_UPDATE;
      return 1;
      }
    }
  return 0;
  }


// Key Release
long FXCheckButton::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    if((flags&FLAG_PRESSED) && (event->code==KEY_space || event->code==KEY_KP_Space)){
      flags|=FLAG_UPDATE;
      flags&=~FLAG_PRESSED;
      if(check!=oldcheck && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check); }
      return 1;
      }
    }
  return 0;
  }


// Hot key combination pressed
long FXCheckButton::onHotKeyPress(FXObject*,FXSelector,void* ptr){
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  flags&=~FLAG_TIP;
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    oldcheck=check;
    setCheck(!oldcheck);
    flags|=FLAG_PRESSED;
    flags&=~FLAG_UPDATE;
    }
  return 1;
  }


// Hot key combination released
long FXCheckButton::onHotKeyRelease(FXObject*,FXSelector,void*){
  if(isEnabled() && (flags&FLAG_PRESSED)){
    flags|=FLAG_UPDATE;
    flags&=~FLAG_PRESSED;
    if(check!=oldcheck && target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check);
    }
  return 1;
  }


// Handle repaint
long FXCheckButton::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXint tw=0,th=0,tx,ty,ix,iy;
  FXDCWindow dc(this,ev);

  // Figure text size
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }

  // Placement
  FXint isize=indicatorSize(this);
  just_x(tx,ix,tw,isize);
  just_y(ty,iy,th,isize);

  // Widget background
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Themed check indicator
  FXTheme *theme=getApp()->getTheme();
  if(0<theme->metric(METRIC_CORNER_RADIUS,getDisplayScale())){
    FXColor box=theme->color(ROLE_BUTTON);
    FXColor edge=theme->color(ROLE_BORDER);
    FXColor mark=theme->color(ROLE_TEXT);
    FXColor hov=theme->color(ROLE_HOVER_RING);
    FXColor foc=theme->color(ROLE_FOCUS_RING);
    FXColor surf=theme->color(ROLE_SURFACE);
    FXint radius=FXMIN(theme->metric(METRIC_CORNER_RADIUS,getDisplayScale()),FXMAX(2,isize/5));
    if(!isEnabled()){
      box=blendColor(box,surf,45);
      edge=blendColor(edge,surf,45);
      mark=theme->color(ROLE_TEXT_DISABLED);
      }
    else{
      if(check!=false){
        FXColor checked=blendColor(hov,FXRGB(0,0,0),40);
        box=blendColor(box,checked,28);
        edge=checked;
        }
      if(underCursor()) edge=hov;
      if(hasFocus()) edge=foc;
      }
    getApp()->getStyle()->drawRoundedRectangle(dc,ix,iy,isize,isize,radius,box,edge,1);
    if(options&CHECKBUTTON_PLUS){
      getApp()->getStyle()->drawRoundedRectangle(dc,ix+isize/4,iy+isize/2-1,isize/2,2,1,mark,mark,0);
      if(check!=true){
        getApp()->getStyle()->drawRoundedRectangle(dc,ix+isize/2-1,iy+isize/4,2,isize/2,1,mark,mark,0);
        }
      }
    else if(check==maybe){
      getApp()->getStyle()->drawRoundedRectangle(dc,ix+isize/4,iy+isize/2-1,isize/2,2,1,mark,mark,0);
      }
    else if(check!=false){
      getApp()->getStyle()->drawCheckMark(dc,ix,iy,isize,mark);
      }
    if(!label.empty()){
      dc.setFont(font);
      if(isEnabled()){
        dc.setForeground(textColor);
        drawLabel(dc,label,hotoff,tx,ty,tw,th);
        }
      else{
        dc.setForeground(theme->color(ROLE_TEXT_DISABLED));
        drawLabel(dc,label,hotoff,tx,ty,tw,th);
        }
      }
    drawFrame(dc,0,0,width,height);
    if(hasFocus()){
      getApp()->getStyle()->drawFocusRing(dc,0,0,width,height,getDisplayScale());
      }
    return 1;
    }

  // Check background
  if(check==maybe || !isEnabled())
    dc.setForeground(baseColor);
  else
    dc.setForeground(boxColor);
  dc.fillRectangle(ix+2,iy+2,9,9);

  // Check border
  if(options&CHECKBUTTON_PLUS){
    dc.setForeground(textColor);
    dc.drawRectangle(ix+2,iy+2,8,8);
    }
  else{
    dc.setForeground(shadowColor);
    dc.fillRectangle(ix,iy,12,1);
    dc.fillRectangle(ix,iy,1,12);
    dc.setForeground(borderColor);
    dc.fillRectangle(ix+1,iy+1,10,1);
    dc.fillRectangle(ix+1,iy+1,1,10);
    dc.setForeground(hiliteColor);
    dc.fillRectangle(ix,iy+12,13,1);
    dc.fillRectangle(ix+12,iy,1,13);
    dc.setForeground(baseColor);
    dc.fillRectangle(ix+1,iy+11,11,1);
    dc.fillRectangle(ix+11,iy+1,1,11);
    }

  // Check color
  if(check==maybe || !isEnabled())
    dc.setForeground(shadowColor);
  else
    dc.setForeground(checkColor);

  // Show as +
  if(options&CHECKBUTTON_PLUS){
    if(check!=true){
      dc.fillRectangle(ix+6,iy+4,1,5);
      }
    dc.fillRectangle(ix+4,iy+6,5,1);
    }

  // Show as v
  else{
    if(check!=false){
      drawCheckMark(dc,ix,iy);
      }
    }

  // Text
  if(!label.empty()){
    dc.setFont(font);
    if(isEnabled()){
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      if(hasFocus()){
        dc.drawFocusRectangle(tx-1,ty-1,tw+2,th+2);
        }
      }
    else{
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }

  // Frame
  drawFrame(dc,0,0,width,height);

  return 1;
  }


// Set check color
void FXCheckButton::setCheckColor(FXColor clr){
  if(clr!=checkColor){
    checkColor=clr;
    checkColorOverride=true;
    update();
    }
  }


// Set box color
void FXCheckButton::setBoxColor(FXColor clr){
  if(clr!=boxColor){
    boxColor=clr;
    boxColorOverride=true;
    update();
    }
  }


// Change check button style
void FXCheckButton::setCheckButtonStyle(FXuint style){
  FXuint opts=(options&~CHECKBUTTON_MASK) | (style&CHECKBUTTON_MASK);
  if(options!=opts){
    options=opts;
    update();
    }
  }


// Return current check button style
FXuint FXCheckButton::getCheckButtonStyle() const {
  return (options&CHECKBUTTON_MASK);
  }


// Save object to stream
void FXCheckButton::save(FXStream& store) const {
  FXLabel::save(store);
  store << checkColor;
  store << boxColor;
  }


// Load object from stream
void FXCheckButton::load(FXStream& store){
  FXLabel::load(store);
  store >> checkColor;
  store >> boxColor;
  }

}
