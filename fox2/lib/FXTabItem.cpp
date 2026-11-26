/********************************************************************************
*                                                                               *
*                           T a b   I t e m    W i d g e t                      *
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
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXTabBar.h"
#include "FXTabBook.h"
#include "FXTabItem.h"

/*
  Notes:
  - Should focus go to tab items?
  - Should callbacks come from tab items?
  - Should redesign this stuff a little.
  - Tab items should observe various border styles.
  - TAB/TABTAB should go into content, arrow keys navigate between tabs.
  - FXTabBook: pane's hints make no sense to observe
  - We hide the panes in FXTabBook.  This way, we don't have to change
    the position of each pane when the FXTabBook itself changes.
    Only the active pane needs to be moved.
  - Maybe honor frame styles.
  - Perhaps make a bit larger by default?
*/


#define TAB_ORIENT_MASK    (TAB_TOP|TAB_LEFT|TAB_RIGHT|TAB_BOTTOM)
#define TABBOOK_MASK       (TABBOOK_SIDEWAYS|TABBOOK_BOTTOMTABS)

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


// Map
FXDEFMAP(FXTabItem) FXTabItemMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXTabItem::onPaint),
  FXMAPFUNC(SEL_ENTER,0,FXTabItem::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXTabItem::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,FXTabItem::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXTabItem::onFocusOut),
  FXMAPFUNC(SEL_UNGRABBED,0,FXTabItem::onUngrabbed),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXTabItem::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXTabItem::onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS,0,FXTabItem::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXTabItem::onKeyRelease),
  FXMAPFUNC(SEL_KEYPRESS,FXWindow::ID_HOTKEY,FXTabItem::onHotKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,FXWindow::ID_HOTKEY,FXTabItem::onHotKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXTabItem,FXLabel,FXTabItemMap,ARRAYNUMBER(FXTabItemMap))


// Tab item
FXTabItem::FXTabItem(FXTabBar* p,const FXString& text,FXIcon* ic,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXLabel(p,text,ic,opts,x,y,w,h,pl,pr,pt,pb){
  border=2;
  }


// If window can have focus
FXbool FXTabItem::canFocus() const { return true; }


// Cursor entered the tab; repaint with the hover face.  The app does not
// update underCursor() until after a leave has been delivered, so the state
// is tracked here instead of being asked at paint time
long FXTabItem::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onEnter(sender,sel,ptr);
  if(!hovered){
    hovered=true;
    update();
    }
  return 1;
  }


// Cursor left the tab; repaint without the hover face
long FXTabItem::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onLeave(sender,sel,ptr);
  if(hovered){
    hovered=false;
    update();
    }
  return 1;
  }


// Gained focus
long FXTabItem::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusIn(sender,sel,ptr);
  update(border,border,width-(border<<1),height-(border<<1));
  return 1;
  }


// Lost focus
long FXTabItem::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onFocusOut(sender,sel,ptr);
  update(border,border,width-(border<<1),height-(border<<1));
  return 1;
  }


// Pressed mouse button
long FXTabItem::onLeftBtnPress(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXLabel::onLeftBtnPress(sender,sel,ptr)){
    if(isEnabled()){
      getParent()->handle(this,FXSEL(SEL_COMMAND,FXTabBar::ID_OPEN_ITEM),ptr);
      flags|=FLAG_PRESSED;
      flags&=~FLAG_UPDATE;
      return 1;
      }
    }
  return 0;
  }


// Released mouse button
long FXTabItem::onLeftBtnRelease(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXLabel::onLeftBtnRelease(sender,sel,ptr)){
    if(isEnabled()){
      flags|=FLAG_UPDATE;
      flags&=~FLAG_PRESSED;
      return 1;
      }
    }
  return 0;
  }


// Lost the grab for some reason
long FXTabItem::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXLabel::onUngrabbed(sender,sel,ptr);
  flags&=~FLAG_PRESSED;
  flags|=FLAG_UPDATE;
  return 1;
  }


// Key Press
long FXTabItem::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    if(event->code==KEY_space || event->code==KEY_KP_Space){
      getParent()->handle(this,FXSEL(SEL_COMMAND,FXTabBar::ID_OPEN_ITEM),ptr);
      return 1;
      }
    }
  return 0;
  }


// Key Release
long FXTabItem::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    if(event->code==KEY_space || event->code==KEY_KP_Space){
      return 1;
      }
    }
  return 0;
  }


// Hot key combination pressed
long FXTabItem::onHotKeyPress(FXObject*,FXSelector,void* ptr){
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  flags&=~FLAG_TIP;
  if(isEnabled()){
    getParent()->handle(this,FXSEL(SEL_COMMAND,FXTabBar::ID_OPEN_ITEM),ptr);
    }
  return 1;
  }


// Hot key combination released
long FXTabItem::onHotKeyRelease(FXObject*,FXSelector,void*){
  return 1;
  }


// Handle repaint
long FXTabItem::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  if(width<=0 || height<=0) return 1;

  // Compose the tab off-screen and copy only the exposed area to the window,
  // so the intermediate states (background, face, text) are never visible
  if(!paintBuffer){
    paintBuffer=new FXImage(getApp(),nullptr,0,width,height);
    paintBuffer->create();
    }
  else if(paintBuffer->getWidth()!=width || paintBuffer->getHeight()!=height){
    paintBuffer->resize(width,height);
    }
  {
    FXEvent full=*ev;
    full.rect.x=0; full.rect.y=0; full.rect.w=width; full.rect.h=height;
    FXDCWindow bdc(paintBuffer);
    paintTab(bdc,&full);
    }
  FXDCWindow dc(this,ev);
  dc.drawArea(paintBuffer,ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h,ev->rect.x,ev->rect.y);
  return 1;
  }


// Release off-screen buffer
FXTabItem::~FXTabItem(){
  delete paintBuffer;
  paintBuffer=nullptr;
  }


// Draw the tab into the given DC
void FXTabItem::paintTab(FXDCWindow& dc,FXEvent* ev){
  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  dc.setForeground(backColor);
//dc.setForeground(FXRGB(255,0,0));
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Modern themed tab face
  FXTheme *theme=getApp()->getTheme();
  FXint radius=theme->metric(METRIC_CORNER_RADIUS,getDisplayScale());
  FXbool themed=0<radius;
  if(themed){
    FXTabBar *bar=static_cast<FXTabBar*>(getParent());
    FXint index=bar?bar->indexOfChild(this):-1;
    FXbool selected=false;
    if(bar){
      if(bar->isMemberOf(FXMETACLASS(FXTabBook))) selected=(index>=0 && (index>>1)==bar->getCurrent());
      else selected=(index==bar->getCurrent());
      }
    FXColor face=selected?theme->color(ROLE_SURFACE):blendColor(theme->color(ROLE_SURFACE),theme->color(ROLE_TEXT),20);
    FXColor edge=selected?theme->color(ROLE_BORDER):blendColor(theme->color(ROLE_SURFACE),theme->color(ROLE_BORDER),35);
    FXColor accent=theme->color(ROLE_ACCENT);
    if(!isEnabled()){
      face=blendColor(face,theme->color(ROLE_SURFACE),45);
      edge=blendColor(edge,theme->color(ROLE_SURFACE),45);
      }
    else if(!selected && hovered){
      face=blendColor(theme->color(ROLE_SURFACE),accent,20);
      }
    if(hasFocus() && !selected) edge=theme->color(ROLE_FOCUS_RING);
    getApp()->getStyle()->drawRoundedRectangle(dc,0,0,width,height,radius,face,edge,1);

    // Square the side that joins the tab bar/page, so only the outer corners
    // of a top/bottom tab are rounded
    FXint rr=FXMIN(radius,FXMIN(width,height)/2);
    if(0<rr){
      dc.setForeground(face);
      switch(options&TAB_ORIENT_MASK){
        case TAB_BOTTOM:
          dc.fillRectangle(0,0,width,rr);
          dc.setForeground(edge);
          dc.fillRectangle(0,0,1,rr);
          dc.fillRectangle(width-1,0,1,rr);
          break;
        case TAB_LEFT:
          dc.fillRectangle(width-rr,0,rr,height);
          dc.setForeground(edge);
          dc.fillRectangle(width-rr,0,rr,1);
          dc.fillRectangle(width-rr,height-1,rr,1);
          break;
        case TAB_RIGHT:
          dc.fillRectangle(0,0,rr,height);
          dc.setForeground(edge);
          dc.fillRectangle(0,0,rr,1);
          dc.fillRectangle(0,height-1,rr,1);
          break;
        default:
          dc.fillRectangle(0,height-rr,width,rr);
          dc.setForeground(edge);
          dc.fillRectangle(0,height-rr,1,rr);
          dc.fillRectangle(width-1,height-rr,1,rr);
          break;
        }
      }
    if(selected){
      FXint thick=FXMAX(theme->metric(METRIC_BORDER_WIDTH,getDisplayScale())+1,2);
      dc.setForeground(accent);
      switch(options&TAB_ORIENT_MASK){
        case TAB_BOTTOM:  dc.fillRectangle(radius,height-thick,width-2*radius,thick); break;
        case TAB_LEFT:    dc.fillRectangle(0,radius,thick,height-2*radius); break;
        case TAB_RIGHT:   dc.fillRectangle(width-thick,radius,thick,height-2*radius); break;
        default:          dc.fillRectangle(radius,0,width-2*radius,thick); break;
        }
      }
    }
  if(!themed){
    switch(options&TAB_ORIENT_MASK){
    case TAB_LEFT:
      dc.setForeground(hiliteColor);
      dc.drawLine(2,0,width-1,0);
      dc.drawLine(0,2,1,1);
      dc.drawLine(0,height-4,0,2);
      dc.setForeground(shadowColor);
      dc.fillRectangle(1,height-3,1,1);
      dc.fillRectangle(2,height-2,width-3,1);
      dc.setForeground(borderColor);
      dc.drawLine(3,height-1,width-1,height-1);
      break;
    case TAB_RIGHT:
      dc.setForeground(hiliteColor);
      dc.drawLine(0,0,width-3,0);
      dc.drawLine(width-3,0,width-1,3);
      dc.setForeground(shadowColor);
      dc.drawLine(width-2,2,width-2,height-2);
      dc.drawLine(0,height-2,width-2,height-2);
      dc.setForeground(borderColor);
      dc.drawLine(0,height-1,width-3,height-1);
      dc.drawLine(width-1,3,width-1,height-4);
      dc.drawLine(width-3,height-1,width-1,height-4);
      break;
    case TAB_BOTTOM:
      dc.setForeground(hiliteColor);
      dc.drawLine(0,0,0,height-4);
      dc.drawLine(0,height-4,1,height-2);
      dc.setForeground(shadowColor);
      dc.fillRectangle(2,height-2,width-4,1);
      dc.drawLine(width-2,0,width-2,height-3);
      dc.fillRectangle(width-2,0,2,1);
      dc.setForeground(borderColor);
      dc.drawLine(3,height-1,width-4,height-1);
      dc.drawLine(width-4,height-1,width-1,height-4);
      dc.fillRectangle(width-1,1,1,height-4);
      break;
    case TAB_TOP:
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,2,1,height-2);
      dc.drawLine(0,2,2,0);
      dc.fillRectangle(2,0,width-4,1);
      dc.setForeground(shadowColor);
      dc.fillRectangle(width-2,1,1,height-1);
      dc.setForeground(borderColor);
      dc.drawLine(width-2,1,width-1,2);
      dc.fillRectangle(width-1,2,1,height-3);
      break;
      }
    }
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(icon){
    iw=getScaledIcon(icon)->getWidth();
    ih=getScaledIcon(icon)->getHeight();
    }
  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);
  if(icon){
    if(isEnabled())
      dc.drawIcon(getScaledIcon(icon),ix,iy);
    else
      dc.drawIconSunken(getScaledIcon(icon),ix,iy);
    }
  if(!label.empty()){
    dc.setFont(font);
    if(isEnabled()){
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      if(hasFocus() && !themed){
        dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
        }
      }
    else{
      if(themed){
        dc.setForeground(theme->color(ROLE_TEXT_DISABLED));
        drawLabel(dc,label,hotoff,tx,ty,tw,th);
        }
      else{
        dc.setForeground(hiliteColor);
        drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
        dc.setForeground(shadowColor);
        drawLabel(dc,label,hotoff,tx,ty,tw,th);
        }
      }
    }
  }


// Get tab style
FXuint FXTabItem::getTabOrientation() const {
  return (options&TAB_ORIENT_MASK);
  }


// Set tab style
void FXTabItem::setTabOrientation(FXuint style){
  FXuint opts=(options&~TAB_ORIENT_MASK) | (style&TAB_ORIENT_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }

}

