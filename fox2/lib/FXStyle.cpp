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
#include "FXVisual.h"
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


// Blend color a toward color b by the given percentage of b
static FXColor blendColor(FXColor a,FXColor b,FXint percent){
  FXint tnecrep=100-percent;
  return FXRGB((FXREDVAL(a)*tnecrep+FXREDVAL(b)*percent+50)/100,
               (FXGREENVAL(a)*tnecrep+FXGREENVAL(b)*percent+50)/100,
               (FXBLUEVAL(a)*tnecrep+FXBLUEVAL(b)*percent+50)/100);
  }

// Rectangle helpers that take the colors of the widget instead of the theme;
// a widget may carry its own highlight, shadow and border colors
static void fillBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor border){
  dc.setForeground(border);
  dc.drawRectangle(x,y,w-1,h-1);
  }


static void fillRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow){
  if(0<w && 0<h){
    dc.setForeground(shadow);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(hilite);
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    }
  }


static void fillSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow){
  if(0<w && 0<h){
    dc.setForeground(shadow);
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(hilite);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    }
  }


static void fillDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow,FXColor border,FXColor surface){
  if(0<w && 0<h){
    dc.setForeground(border);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(hilite);
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(surface);
      dc.fillRectangle(x+1,y+1,w-2,1);
      dc.fillRectangle(x+1,y+1,1,h-2);
      dc.setForeground(shadow);
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }



static void fillRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow){
  if(0<w && 0<h){
    dc.setForeground(hilite);
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(shadow);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(hilite);
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(shadow);
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }


static void fillGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow){
  if(0<w && 0<h){
    dc.setForeground(shadow);
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(hilite);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(shadow);
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(hilite);
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }

static void fillDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXColor hilite,FXColor shadow,FXColor border,FXColor surface){
  if(0<w && 0<h){
    dc.setForeground(hilite);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(shadow);
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(border);
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      dc.setForeground(surface);
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }



using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct style for given theme
FXStyle::FXStyle(FXTheme* thm):theme(thm),xrender(false),xrenderChecked(false),dpy(nullptr),masks(nullptr){
  }


// Destructor
FXStyle::~FXStyle(){
  invalidate();
  dpy=nullptr;
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
void FXStyle::drawFocusRing(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXdouble scale){
  FXint radius=theme->metric(METRIC_CORNER_RADIUS,scale);
  FXint width=FXMAX(theme->metric(METRIC_FOCUS_WIDTH,scale),1);
  FXint r;

  // Rounded ring in the accent color for themed controls
  if(0<radius){
    r=FXMIN(radius,FXMIN(w,h)/2);
    dc.setForeground(theme->color(ROLE_FOCUS_RING));
    dc.setLineWidth(width);
    dc.drawRoundRectangle(x+width/2,y+width/2,w-width,h-width,r,r);
    dc.setLineWidth(1);
    return;
    }

  // Classic dashed rectangle
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


// Draw a rounded rectangle with a solid face and border.  Used by the small
// controls (check and radio indicators) that need the same antialiased chrome
// as a button without its shadow and state machinery.
FXbool FXStyle::drawRoundedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
  if(0<radius){
    if(drawRoundRect(dc,x,y,w,h,radius,face,border,borderWidth)) return true;
    FXint rr=FXMIN(radius,FXMIN(w,h)/2);
    dc.setForeground(face);
    dc.fillRoundRectangle(x,y,w,h,rr,rr);
    if(0<borderWidth){
      dc.setForeground(border);
      dc.drawRoundRectangle(x,y,w,h,rr,rr);
      }
    return true;
    }
  dc.setForeground(face);
  dc.fillRectangle(x,y,w,h);
  if(0<borderWidth){
    dc.setForeground(border);
    dc.drawRectangle(x,y,w-1,h-1);
    }
  return false;
  }


// Draw frame chrome for the given frame options
void FXStyle::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint options){
  drawFrame(dc,x,y,w,h,options,theme->color(ROLE_SURFACE_HILITE),theme->color(ROLE_SURFACE_SHADOW),theme->color(ROLE_BORDER),theme->color(ROLE_SURFACE));
  }


// Draw frame chrome with the colors of the widget
void FXStyle::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint options,FXColor hilite,FXColor shadow,FXColor border,FXColor surface){
  switch(options&FRAME_MASK){
    case FRAME_LINE: fillBorderRectangle(dc,x,y,w,h,border); break;
    case FRAME_SUNKEN: fillSunkenRectangle(dc,x,y,w,h,hilite,shadow); break;
    case FRAME_RAISED: fillRaisedRectangle(dc,x,y,w,h,hilite,shadow); break;
    case FRAME_GROOVE: fillGrooveRectangle(dc,x,y,w,h,hilite,shadow); break;
    case FRAME_RIDGE: fillRidgeRectangle(dc,x,y,w,h,hilite,shadow); break;
    case FRAME_SUNKEN|FRAME_THICK: fillDoubleSunkenRectangle(dc,x,y,w,h,hilite,shadow,border,surface); break;
    case FRAME_RAISED|FRAME_THICK: fillDoubleRaisedRectangle(dc,x,y,w,h,hilite,shadow,border,surface); break;
    }
  }


// Draw button chrome; returns rectangle available to icon and label
FXRectangle FXStyle::drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXButtonState& state){
  FXColor hilite=state.hilite;
  FXColor shadow=state.shadow;
  FXColor edge=state.edge;
  FXColor surface=state.surface;
  FXColor face=state.engaged?hilite:state.back;
  FXint borderWidth=theme->metric(METRIC_BORDER_WIDTH,state.scale);
  FXint radius=theme->metric(METRIC_CORNER_RADIUS,state.scale);
  FXint border=state.border;
  FXuint options=state.options;

  // Rounded chrome, when the theme asks for it
  if(0<radius){
    FXColor accent=theme->color(ROLE_ACCENT);

    // Breeze-like face: a light button on a light surface, a slightly lifted
    // one on a dark surface, with the same state shading as the GTK style
    face=theme->color(ROLE_BUTTON);
    if(!state.enabled){
      face=blendColor(face,surface,45);
      edge=blendColor(edge,surface,45);
      }
    else if(state.down){
      face=blendColor(face,accent,33);
      }
    else if(state.engaged){
      face=blendColor(face,state.text,12);
      }

    // Hover, keyboard focus and the default button get their theme decoration
    if(state.enabled){
      if(state.focused || state.defaulted) edge=theme->color(ROLE_FOCUS_RING);
      else if(state.hover) edge=theme->color(ROLE_HOVER_RING);
      }

    // Windows are created with a black background pixel, so the area outside
    // the rounded shape must be filled with the parent's color first
    dc.setForeground(state.parentBack);
    dc.fillRectangle(x,y,w,h);

    // A hairline shadow under the button, the way Breeze draws its box-shadow
    FXint faceh=h;
    if(state.enabled && 4<h){
      FXColor shade=blendColor(state.parentBack,FXRGB(0,0,0),12);
      FXint sr=FXMIN(radius,FXMIN(w,h-1)/2);
      faceh=h-1;
      if(!drawRoundRect(dc,x,y+1,w,faceh,radius,shade,shade,0)){
        dc.setForeground(shade);
        dc.fillRoundRectangle(x,y+1,w,faceh,sr,sr);
        }
      }

    if(drawRoundRect(dc,x,y,w,faceh,radius,face,edge,borderWidth)){
      return FXRectangle(x,y,w,h);
      }
    // Fallback: hard-edged rounded shape, corners stay transparent
    FXint rr=FXMIN(radius,FXMIN(w,faceh)/2);
    dc.setForeground(face);
    dc.fillRoundRectangle(x,y,w,faceh,rr,rr);
    if(0<borderWidth){
      dc.setForeground(edge);
      dc.drawRoundRectangle(x,y,w,faceh,rr,rr);
      }
    return FXRectangle(x,y,w,h);
    }

  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(state.toolbar){

      // Enabled, cursor inside, and up
      if(state.enabled && state.hover && !state.down && !state.engaged){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) fillDoubleRaisedRectangle(dc,x,y,w,h,hilite,shadow,edge,surface);
        else fillRaisedRectangle(dc,x,y,w,h,hilite,shadow);
        }

      // Enabled, cursor inside, and down
      else if(state.enabled && state.hover && state.down){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) fillDoubleSunkenRectangle(dc,x,y,w,h,hilite,shadow,edge,surface);
        else fillSunkenRectangle(dc,x,y,w,h,hilite,shadow);
        }

      // Enabled and checked
      else if(state.enabled && state.engaged){
        dc.setForeground(hilite);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) fillDoubleSunkenRectangle(dc,x,y,w,h,hilite,shadow,edge,surface);
        else fillSunkenRectangle(dc,x,y,w,h,hilite,shadow);
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
          if(options&FRAME_THICK) fillDoubleRaisedRectangle(dc,x+1,y+1,w-1,h-1,hilite,shadow,edge,surface);
          else fillRaisedRectangle(dc,x+1,y+1,w-1,h-1,hilite,shadow);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2-1,h-border*2-1);
          if(options&FRAME_THICK) fillDoubleSunkenRectangle(dc,x,y,w-1,h-1,hilite,shadow,edge,surface);
          else fillSunkenRectangle(dc,x,y,w-1,h-1,hilite,shadow);
          }

        // Black default border
        fillBorderRectangle(dc,x,y,w,h,edge);
        }

      // Non-default
      else{

        // Draw in up state if disabled or up
        if(!state.enabled || (!state.down && !state.engaged)){
          dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) fillDoubleRaisedRectangle(dc,x,y,w,h,hilite,shadow,edge,surface);
          else fillRaisedRectangle(dc,x,y,w,h,hilite,shadow);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) fillDoubleSunkenRectangle(dc,x,y,w,h,hilite,shadow,edge,surface);
          else fillSunkenRectangle(dc,x,y,w,h,hilite,shadow);
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


}
