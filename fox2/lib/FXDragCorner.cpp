/********************************************************************************
*                                                                               *
*                       D r a g   C o r n e r   W i d g e t                     *
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

/*
  Notes:
  - Need to grab server while dragging?
  - Need to use extended window manager hints so that minimum/maximum size
    and so on are properly observed.
*/


#define CORNERSIZE    17

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDragCorner) FXDragCornerMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXDragCorner::onPaint),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXDragCorner::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXDragCorner::onLeftBtnRelease),
  FXMAPFUNC(SEL_MOTION,0,FXDragCorner::onMotion),
  };


// Object implementation
FXIMPLEMENT(FXDragCorner,FXWindow,FXDragCornerMap,ARRAYNUMBER(FXDragCornerMap))


// Deserialization
FXDragCorner::FXDragCorner(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  hiliteColor=0;
  shadowColor=0;
  oldw=0;
  oldh=0;
  xoff=0;
  yoff=0;
  ewmh=false;
  }


// Construct and init
FXDragCorner::FXDragCorner(FXComposite* p):FXWindow(p,LAYOUT_RIGHT|LAYOUT_BOTTOM){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  defaultCursor=getApp()->getDefaultCursor(DEF_DRAGBR_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_DRAGBR_CURSOR);
  backColor=getApp()->getBaseColor();
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  oldw=0;
  oldh=0;
  xoff=0;
  yoff=0;
  ewmh=false;
  }


// Get default width
FXint FXDragCorner::getDefaultWidth(){
  return CORNERSIZE;
  }


// Get default height
FXint FXDragCorner::getDefaultHeight(){
  return CORNERSIZE;
  }


// Slightly different from Frame border
long FXDragCorner::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  dc.setForeground(shadowColor);
  dc.drawLine(width-2,height-1,width,height-3);
  dc.drawLine(width-8,height-1,width,height-9);
  dc.drawLine(width-14,height-1,width,height-15);
  dc.setForeground(hiliteColor);
  dc.drawLine(width-5,height-1,width,height-6);
  dc.drawLine(width-11,height-1,width,height-12);
  dc.drawLine(width-17,height-1,width,height-18);
  return 1;
  }


// Released LEFT button
long FXDragCorner::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(flags&FLAG_PRESSED){
    FXDCWindow dc(getRoot());
    FXint xx,yy,wx,wy;
    ungrab();
    getShell()->translateCoordinatesTo(xx,yy,getRoot(),0,0);
    translateCoordinatesTo(wx,wy,getShell(),event->win_x,event->win_y);
    dc.clipChildren(false);
    dc.setFunction(BLT_SRC_XOR_DST);
    dc.setForeground(FXRGB(255,255,255));
    dc.drawRectangle(xx,yy,oldw,oldh);
    getShell()->resize(wx+xoff,wy+yoff);
    flags&=~FLAG_PRESSED;
    }
  return 1;
  }


// Moved
long FXDragCorner::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(flags&FLAG_PRESSED){
    FXDCWindow dc(getRoot());
    FXint xx,yy,wx,wy;
    getShell()->translateCoordinatesTo(xx,yy,getRoot(),0,0);
    translateCoordinatesTo(wx,wy,getShell(),event->win_x,event->win_y);
    dc.clipChildren(false);
    dc.setFunction(BLT_SRC_XOR_DST);
    dc.setForeground(FXRGB(255,255,255));
    dc.drawRectangle(xx,yy,oldw,oldh);
    oldw=wx+xoff;
    oldh=wy+yoff;
    dc.drawRectangle(xx,yy,oldw,oldh);
    return 1;
    }
  return 0;
  }


// Set highlight color
void FXDragCorner::setHiliteColor(FXColor clr){
  if(hiliteColor!=clr){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXDragCorner::setShadowColor(FXColor clr){
  if(shadowColor!=clr){
    shadowColor=clr;
    update();
    }
  }


// Save data
void FXDragCorner::save(FXStream& store) const {
  FXWindow::save(store);
  store << hiliteColor;
  store << shadowColor;
  }


// Load data
void FXDragCorner::load(FXStream& store){
  FXWindow::load(store);
  store >> hiliteColor;
  store >> shadowColor;
  }

}

