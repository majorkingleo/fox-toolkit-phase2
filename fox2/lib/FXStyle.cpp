/********************************************************************************
*                                                                               *
*                        S t y l e    O b j e c t s                              *
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
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXTheme.h"
#include "FXFrame.h"
#include "FXStyle.h"


/*
  Notes:
  - The classic style must reproduce the original FXButton and FXFrame drawing
    byte for byte; that is what makes the theme refactoring safe to land.
  - Colors come from the theme, layout stays in the widget.
*/


// Frame style mask, matching FXFrame
#define FRAME_MASK (FRAME_SUNKEN|FRAME_RAISED|FRAME_THICK)


using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct style for given theme
FXStyle::FXStyle(FXTheme* thm):theme(thm){
  }


// Drop cached chrome resources after a theme change
void FXStyle::invalidate(){
  }


// Draw border rectangle
void FXStyle::drawBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(theme->color(ROLE_BORDER));
  dc.drawRectangle(x,y,w-1,h-1);
  }


// Draw raised rectangle
void FXStyle::drawRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    }
  }


// Draw sunken rectangle
void FXStyle::drawSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    }
  }


// Draw double raised rectangle
void FXStyle::drawDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_BORDER));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE));
      dc.fillRectangle(x+1,y+1,w-2,1);
      dc.fillRectangle(x+1,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }


// Draw double sunken rectangle
void FXStyle::drawDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_BORDER));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      dc.setForeground(theme->color(ROLE_SURFACE));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }


// Draw keyboard focus ring
void FXStyle::drawFocusRing(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(theme->color(ROLE_TEXT));
  dc.drawFocusRectangle(x,y,w,h);
  }


// Draw ridge rectangle
void FXStyle::drawRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }


// Draw groove rectangle
void FXStyle::drawGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }


// Draw frame chrome for the given frame options
void FXStyle::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint options){
  switch(options&FRAME_MASK){
    case FRAME_LINE: drawBorderRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN: drawSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED: drawRaisedRectangle(dc,x,y,w,h); break;
    case FRAME_GROOVE: drawGrooveRectangle(dc,x,y,w,h); break;
    case FRAME_RIDGE: drawRidgeRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN|FRAME_THICK: drawDoubleSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED|FRAME_THICK: drawDoubleRaisedRectangle(dc,x,y,w,h); break;
    }
  }


// Draw button chrome; returns rectangle available to icon and label
FXRectangle FXStyle::drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXButtonState& state){
  FXColor hilite=theme->color(ROLE_SURFACE_HILITE);
  FXint border=state.border;
  FXuint options=state.options;

  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(state.toolbar){

      // Enabled, cursor inside, and up
      if(state.enabled && state.hover && !state.down && !state.engaged){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x,y,w,h);
        else drawRaisedRectangle(dc,x,y,w,h);
        }

      // Enabled, cursor inside, and down
      else if(state.enabled && state.hover && state.down){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
        else drawSunkenRectangle(dc,x,y,w,h);
        }

      // Enabled and checked
      else if(state.enabled && state.engaged){
        dc.setForeground(hilite);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
        else drawSunkenRectangle(dc,x,y,w,h);
        }

      // Disabled or unchecked or not under cursor
      else{
        dc.setForeground(state.back);
        dc.fillRectangle(x,y,w,h);
        }
      }

    // Normal style
    else{

      // Default
      if(state.defaulted){

        // Draw in up state if disabled or up
        if(!state.enabled || (!state.down && !state.engaged)){
          dc.setForeground(state.back);
          dc.fillRectangle(x+border+1,y+border+1,w-border*2-1,h-border*2-1);
          if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x+1,y+1,w-1,h-1);
          else drawRaisedRectangle(dc,x+1,y+1,w-1,h-1);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2-1,h-border*2-1);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w-1,h-1);
          else drawSunkenRectangle(dc,x,y,w-1,h-1);
          }

        // Black default border
        drawBorderRectangle(dc,x,y,w,h);
        }

      // Non-default
      else{

        // Draw in up state if disabled or up
        if(!state.enabled || (!state.down && !state.engaged)){
          dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x,y,w,h);
          else drawRaisedRectangle(dc,x,y,w,h);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
          else drawSunkenRectangle(dc,x,y,w,h);
          }
        }
      }
    }

  // No borders
  else{
    if(state.enabled && state.engaged){
      dc.setForeground(hilite);
      dc.fillRectangle(x,y,w,h);
      }
    else{
      dc.setForeground(state.back);
      dc.fillRectangle(x,y,w,h);
      }
    }
  return FXRectangle(x,y,w,h);
  }


// Destructor
FXStyle::~FXStyle(){
  }

}
