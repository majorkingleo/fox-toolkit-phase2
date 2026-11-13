/********************************************************************************
*                                                                               *
*              C l i e n t   S i d e   D e c o r a t i o n   ( W a y l a n d )  *
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
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXVisual.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXRegion.h"
#include "FXDC.h"
#include "fxglyphs.h"
#include "fxraster.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A compositor that refuses to draw the decorations leaves the title bar to
    the client.  This is the minimal one: a strip the height of the font plus a
    small margin, the title on the left and three buttons on the right (close,
    maximize, minimize), drawn by the rasterizer into the store of the shell
    right before the frame is committed.
  - The bar is part of the surface, not a subsurface, so the frame is the window
    height plus the bar height and every window of the tree is pushed down by
    the bar.  The pointer path subtracts the bar again, and a press inside the
    bar is used here and never reaches a widget.
  - Dragging the bar asks the compositor to move the window, which keeps the
    window and the pointer together and needs no tracking on our side.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Title of a shell, as shown in a client side title bar
void fxwaylandSetTitle(FXWaylandSurface* surf,const FXString& title){
  if(surf && surf->btitle!=title){
    surf->btitle=title;
    surf->dirty=true;
    }
  }


// Height a client side title bar needs, from the font of the application
FXint fxwaylandBarHeight(FXWaylandApp* app){
  FXApp* foxapp=(FXApp*)app->foxapp;
  FXint height=0;
  if(foxapp && foxapp->getNormalFont()){
    height=foxapp->getNormalFont()->getFontHeight();
    }
  if(height<=0) height=12;
  return height+8;
  }


// The bar buttons, counted from the right edge inwards
enum { BAR_CLOSE=1, BAR_MAXIMIZE=2, BAR_MINIMIZE=3 };


// Which bar button sits at this place, 0 for the title itself
static FXint barButton(FXWaylandSurface* surf,FXint x){
  FXint size=FXMIN(surf->barh,18);
  FXint right=surf->storew;
  if(x>=right-size) return BAR_CLOSE;
  if(x>=right-2*size) return BAR_MAXIMIZE;
  if(x>=right-3*size) return BAR_MINIMIZE;
  return 0;
  }


// Paint the client side title bar into the store of its shell
void fxwaylandPaintBar(FXWaylandApp* app,FXWaylandSurface* surf){
  FXApp* foxapp=(FXApp*)app->foxapp;
  FXGlyphs* glyphs=nullptr;
  FXRaster raster;
  FXint w=surf->storew;
  FXint h=surf->barh;
  FXint size=FXMIN(h,18);
  FXint x;
  if(!foxapp || h<=0 || w<=0) return;
  raster.setTarget(surf->store,w,h,surf->storew);
  raster.setClipRectangle(0,0,w,h);
  raster.setForeground(foxapp->getBaseColor());
  raster.fillRectangle(0,0,w,h);
  raster.setForeground(foxapp->getShadowColor());
  raster.drawLine(0,h-1,w-1,h-1);

  // The buttons, from the right edge inwards
  raster.setLineWidth(1);
  for(x=1; x<=3; x++){
    FXint cx=w-x*size+size/2;
    FXint cy=h/2;
    FXint arm=FXMAX(size/5,2);
    raster.setForeground(x==BAR_CLOSE?foxapp->getSelbackColor():foxapp->getHiliteColor());
    if(x==BAR_CLOSE){
      raster.drawLine(cx-arm,cy-arm,cx+arm,cy+arm);
      raster.drawLine(cx-arm,cy+arm,cx+arm,cy-arm);
      }
    else if(x==BAR_MAXIMIZE){
      raster.drawRectangle(cx-arm,cy-arm,2*arm+1,2*arm+1);
      }
    else{
      raster.drawLine(cx-arm,cy,cx+arm,cy);
      }
    }

  // The title, clipped to the room left of the buttons
  if(foxapp->getNormalFont()){
    glyphs=(FXGlyphs*)foxapp->getNormalFont()->getGlyphCache();
    }
  if(glyphs && !surf->btitle.empty()){
    FXint base=(h+glyphs->getAscent()-glyphs->getDescent())/2;
    raster.setClipRectangle(4,0,w-3*size-6,h);
    raster.setGlyphs(glyphs);
    raster.setForeground(foxapp->getForeColor());
    raster.drawText(4,base,surf->btitle.text(),surf->btitle.length());
    }
  }


// Handle a press inside the client side title bar; true when it was used
FXbool fxwaylandBarPress(FXWaylandApp* app,FXWaylandSurface* surf,FXint x,FXint y){
  FXint which;
  if(!app || !surf || surf->barh<=0 || y>=surf->barh) return false;
  which=barButton(surf,x);
  if(which==BAR_CLOSE){
    FXWaylandEvent ev;
    clearElms(&ev,1);
    ev.code=FXXWL_CLOSE;
    ev.id=surf->window;
    fxwaylandPost(app,ev);
    return true;
    }
  if(which==BAR_MAXIMIZE && surf->toplevel){
    if(surf->state&FXXWL_STATE_MAXIMIZED){
      xdg_toplevel_unset_maximized(surf->toplevel);
      }
    else{
      xdg_toplevel_set_maximized(surf->toplevel);
      }
    return true;
    }
  if(which==BAR_MINIMIZE && surf->toplevel){
    surf->state|=FXXWL_STATE_MINIMIZED;
    xdg_toplevel_set_minimized(surf->toplevel);
    return true;
    }

  // Everywhere else the bar moves the window, which the compositor does
  if(surf->toplevel && app->seat && app->pointerserial){
    xdg_toplevel_move(surf->toplevel,app->seat,app->pointerserial);
    }
  return true;
  }

}

#endif
