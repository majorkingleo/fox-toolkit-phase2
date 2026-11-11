/********************************************************************************
*                                                                               *
*                           B u t t o n    O b j e c t s                        *
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
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXThread.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXTheme.h"
#include "FXStyle.h"
#include "FXIcon.h"
#include "FXShell.h"
#include "FXButton.h"


// Interval between two face-color animation steps, in nanoseconds
#define ANIM_TICK 15000000LL


/*
  Notes:
  - Use flags for button instead of a whole integer
  - Add ``flat'' toolbar style also
  - Need check-style also (stay in when pressed, pop out when unpressed).
  - Who owns the icon(s)?
  - Arrow buttons should auto-repeat with a timer of some kind
  - "&Label\tTooltip\tHelptext\thttp://server/application/helponitem.html"
  - CheckButton should send SEL_COMMAND.
  - Default button mode:- should somehow get focus.
  - Add button multiple-click translations elsewhere
  - Button should be able to behave like a check (radio) button.
  - Need to draw ``around'' the icon etc. So it doesn't flash to background.
*/

// Button styles
#define BUTTON_MASK (BUTTON_AUTOGRAY|BUTTON_AUTOHIDE|BUTTON_TOOLBAR|BUTTON_DEFAULT|BUTTON_INITIAL)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXButton) FXButtonMap[]={
  FXMAPFUNC(SEL_UPDATE,0,FXButton::onUpdate),
  FXMAPFUNC(SEL_PAINT,0,FXButton::onPaint),
  FXMAPFUNC(SEL_ENTER,0,FXButton::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXButton::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,FXButton::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXButton::onFocusOut),
  FXMAPFUNC(SEL_UNGRABBED,0,FXButton::onUngrabbed),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXButton::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXButton::onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXButton::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXButton::onKeyRelease),
  FXMAPFUNC(SEL_KEYPRESS,FXButton::ID_HOTKEY,FXButton::onHotKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,FXButton::ID_HOTKEY,FXButton::onHotKeyRelease),
  FXMAPFUNC(SEL_COMMAND,FXButton::ID_CHECK,FXButton::onCheck),
  FXMAPFUNC(SEL_COMMAND,FXButton::ID_UNCHECK,FXButton::onUncheck),
  FXMAPFUNC(SEL_COMMAND,FXButton::ID_SETVALUE,FXButton::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXButton::ID_SETINTVALUE,FXButton::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXButton::ID_GETINTVALUE,FXButton::onCmdGetIntValue),
  FXMAPFUNC(SEL_TIMEOUT,FXButton::ID_ANIMATE,FXButton::onAnimate),
  };


// Object implementation
FXIMPLEMENT(FXButton,FXLabel,FXButtonMap,ARRAYNUMBER(FXButtonMap))


// Deserialization
FXButton::FXButton(){
  state=STATE_UP;
  animColor=0;
  animFrom=0;
  animTarget=0;
  animStart=0;
  animating=false;
  }


// Construct and init
FXButton::FXButton(FXComposite* p,const FXString& text,FXIcon* ic,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXLabel(p,text,ic,opts,x,y,w,h,pl,pr,pt,pb){
  target=tgt;
  message=sel;
  state=STATE_UP;
  animColor=backColor;
  animFrom=backColor;
  animTarget=backColor;
  animStart=0;
  animating=false;
  if(options&BUTTON_INITIAL){
    setInitial(true);
    setDefault(true);
    }
  }


// If window can have focus
FXbool FXButton::canFocus() const { return true; }


// Set focus to this widget
void FXButton::setFocus(){
  FXLabel::setFocus();
  if(options&BUTTON_DEFAULT) setDefault(true);
  update();
  }


// Kill focus to this widget
void FXButton::killFocus(){
  FXLabel::killFocus();
  if(options&BUTTON_DEFAULT) setDefault(maybe);
  update();
  }


// Make widget drawn as default
void FXButton::setDefault(FXuchar flag){
  FXLabel::setDefault(flag);
  update();
  }


// Set button state
void FXButton::setState(FXuint s){
  if(state!=s){
    state=s;
    update();
    }
  }


// Update value from a message
long FXButton::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setState((FXuint)(FXuval)ptr);
  return 1;
  }


// Update value from a message
long FXButton::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setState(*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXButton::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getState();
  return 1;
  }


// Check the menu button
long FXButton::onCheck(FXObject*,FXSelector,void*){
  setState(STATE_ENGAGED);
  return 1;
  }


// Check the menu button
long FXButton::onUncheck(FXObject*,FXSelector,void*){
  setState(STATE_UP);
  return 1;
  }


// Implement auto-hide or auto-gray modes
long FXButton::onUpdate(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXLabel::onUpdate(sender,sel,ptr)){
    if(options&BUTTON_AUTOHIDE){if(shown()){hide();recalc();}}
    if(options&BUTTON_AUTOGRAY){disable();}
    }
  return 1;
  }


// Gained focus
long FXButton::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusIn(sender,sel,ptr);
  update();
  return 1;
  }


// Lost focus
long FXButton::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusOut(sender,sel,ptr);
  update();
  return 1;
  }


// Entered button
long FXButton::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onEnter(sender,sel,ptr);
  if(isEnabled()){
    if((flags&FLAG_PRESSED) && (state!=STATE_ENGAGED)) setState(STATE_DOWN);
    if(options&BUTTON_TOOLBAR) update();
    updateAnimatedColor(true);
    }
  return 1;
  }


// Left button
long FXButton::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onLeave(sender,sel,ptr);
  if(isEnabled()){
    if((flags&FLAG_PRESSED) && (state!=STATE_ENGAGED)) setState(STATE_UP);
    if(options&BUTTON_TOOLBAR) update();
    updateAnimatedColor(false);
    }
  return 1;
  }


// Pressed mouse button
long FXButton::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  flags&=~FLAG_TIP;
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    if(state!=STATE_ENGAGED) setState(STATE_DOWN);
    flags|=FLAG_PRESSED;
    flags&=~FLAG_UPDATE;
    updateAnimatedColor(underCursor());
    return 1;
    }
  return 0;
  }


// Released mouse button
long FXButton::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXbool click=(state==STATE_DOWN);
  if(isEnabled() && (flags&FLAG_PRESSED)){
    ungrab();
    flags|=FLAG_UPDATE;
    flags&=~FLAG_PRESSED;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(state!=STATE_ENGAGED) setState(STATE_UP);
    if(click && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)1); }
    updateAnimatedColor(underCursor());
    return 1;
    }
  return 0;
  }


// Lost the grab for some reason
long FXButton::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onUngrabbed(sender,sel,ptr);
  if(state!=STATE_ENGAGED) setState(STATE_UP);
  flags&=~FLAG_PRESSED;
  flags|=FLAG_UPDATE;
  return 1;
  }


// Key Press
long FXButton::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    if(!(flags&FLAG_PRESSED) && ((event->code==KEY_space || event->code==KEY_KP_Space) || (isDefault() && (event->code==KEY_Return || event->code==KEY_KP_Enter)))){
      if(state!=STATE_ENGAGED) setState(STATE_DOWN);
      flags|=FLAG_PRESSED;
      flags&=~FLAG_UPDATE;
      return 1;
      }
    }
  return 0;
  }


// Key Release
long FXButton::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXbool click=(state==STATE_DOWN);
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    if((flags&FLAG_PRESSED) && ((event->code==KEY_space || event->code==KEY_KP_Space) || (isDefault() && (event->code==KEY_Return || event->code==KEY_KP_Enter)))){
      if(state!=STATE_ENGAGED) setState(STATE_UP);
      flags|=FLAG_UPDATE;
      flags&=~FLAG_PRESSED;
      if(click && target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)1); }
      return 1;
      }
    }
  return 0;
  }


// Hot key combination pressed
long FXButton::onHotKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled() && !(flags&FLAG_PRESSED)){
    if(state!=STATE_ENGAGED) setState(STATE_DOWN);
    flags&=~FLAG_UPDATE;
    flags|=FLAG_PRESSED;
    }
  return 1;
  }


// Hot key combination released
long FXButton::onHotKeyRelease(FXObject*,FXSelector,void*){
  FXuint click=(state==STATE_DOWN);
  if(isEnabled() && (flags&FLAG_PRESSED)){
    if(state!=STATE_ENGAGED) setState(STATE_UP);
    flags|=FLAG_UPDATE;
    flags&=~FLAG_PRESSED;
    if(click && target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)1);
    }
  return 1;
  }


// Handle repaint
long FXButton::onPaint(FXObject*,FXSelector,void* ptr){
  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  FXEvent *ev=(FXEvent*)ptr;

  // Start drawing
  FXDCWindow dc(this,ev);

  // Collect the state the style needs
  FXButtonState st;
  st.enabled=isEnabled();
  st.hover=underCursor();
  st.down=(state==STATE_DOWN);
  st.engaged=(state==STATE_ENGAGED);
  st.focused=hasFocus();
  st.defaulted=isDefault();
  st.toolbar=(options&BUTTON_TOOLBAR)!=0;
  st.border=border;
  st.options=options;
  st.back=animating?animColor:backColor;
  st.text=textColor;
  st.parentBack=getParent()?getParent()->getBackColor():backColor;

  // Draw the chrome; the classic style leaves the content area unchanged
  getApp()->getStyle()->drawButton(dc,0,0,width,height,st);

  // Place text & icon
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }

  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);

  // Shift a bit when pressed
  if(state && (options&(FRAME_RAISED|FRAME_SUNKEN))){ ++tx; ++ty; ++ix; ++iy; }

  // Draw enabled state
  if(isEnabled()){
    if(icon){
      dc.drawIcon(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    if(hasFocus()){
      getApp()->getStyle()->drawFocusRing(dc,border+1,border+1,width-2*border-2,height-2*border-2);
      }
    }

  // Draw grayed-out state
  else{
    if(icon){
      dc.drawIconSunken(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(getApp()->getTheme()->color(ROLE_SURFACE_HILITE));
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(getApp()->getTheme()->color(ROLE_SURFACE_SHADOW));
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }
  return 1;
  }


// Aim the face-color animation at the given hover state
void FXButton::updateAnimatedColor(FXbool hover){
  FXTheme *theme=getApp()->getTheme();
  FXint msec=theme->metric(METRIC_ANIMATION_TIME);
  FXColor target=backColor;

  // Transition only when the theme asks for it
  if(0<msec && isEnabled() && hover){
    target=theme->color(ROLE_SURFACE,(state==STATE_DOWN)?THEME_STATE_ACTIVE:THEME_STATE_HOVER);
    }

  // Nothing to do if the target did not change
  if(target==animTarget) return;

  // Retarget from wherever the face color is right now
  animTarget=target;
  animFrom=animColor;
  animStart=FXThread::time();

  // Start the timer if the transition is not already running
  if(!animating){
    animating=true;
    getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_ANIMATE),ANIM_TICK);
    }
  }


// Step the face color toward its target
long FXButton::onAnimate(FXObject*,FXSelector,void*){
  FXint msec=getApp()->getTheme()->metric(METRIC_ANIMATION_TIME);
  FXdouble alpha=1.0;
  FXint r1,g1,b1,r2,g2,b2;

  // Progress of the transition, 0..1
  if(0<msec) alpha=FXdouble(FXThread::time()-animStart)/FXdouble(msec*1000000LL);

  // Complete: snap to the target color and stop the timer
  if(alpha>=1.0){
    animColor=animTarget;
    animating=false;
    update();
    return 0;
    }

  r1=FXREDVAL(animFrom);
  g1=FXGREENVAL(animFrom);
  b1=FXBLUEVAL(animFrom);
  r2=FXREDVAL(animTarget);
  g2=FXGREENVAL(animTarget);
  b2=FXBLUEVAL(animTarget);
  animColor=FXRGB(r1+FXint(alpha*(r2-r1)),g1+FXint(alpha*(g2-g1)),b1+FXint(alpha*(b2-b1)));
  update();

  // Timers are one-shot, so arm the next step
  getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_ANIMATE),ANIM_TICK);
  return 1;
  }


// Re-apply theme colors, stopping any running animation
void FXButton::applyTheme(FXTheme* theme){
  FXLabel::applyTheme(theme);
  if(animating){
    getApp()->removeTimeout(this,FXSEL(SEL_TIMEOUT,ID_ANIMATE));
    animating=false;
    }
  animColor=backColor;
  animFrom=backColor;
  animTarget=backColor;
  }


// Destructor
FXButton::~FXButton(){
  if(animating){
    getApp()->removeTimeout(this,FXSEL(SEL_TIMEOUT,ID_ANIMATE));
    animating=false;
    }
  }


// Set button style
void FXButton::setButtonStyle(FXuint style){
  FXuint opts=(options&~BUTTON_MASK) | (style&BUTTON_MASK);
  if(options!=opts){
    options=opts;
    update();
    }
  }


// Get button style
FXuint FXButton::getButtonStyle() const {
  return (options&BUTTON_MASK);
  }

}
