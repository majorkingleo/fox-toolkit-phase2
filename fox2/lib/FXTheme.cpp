/********************************************************************************
*                                                                               *
*                      T h e m e    O b j e c t s                                *
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
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXColors.h"
#include "FXTheme.h"


/*
  Notes:
  - Classic theme values must match the FXApp defaults exactly, so that
    introducing the theme changes nothing visually.
  - Derived colors are recomputed from base roles only; explicit palette
    overrides of the highlight and shadow colors are preserved.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Mix two colors; percent is the weight of the second color
static FXColor mixColor(FXColor col1,FXColor col2,FXint percent){
  FXint r1=FXREDVAL(col1);
  FXint g1=FXGREENVAL(col1);
  FXint b1=FXBLUEVAL(col1);
  FXint r2=FXREDVAL(col2);
  FXint g2=FXGREENVAL(col2);
  FXint b2=FXBLUEVAL(col2);
  return FXRGB(r1+(r2-r1)*percent/100,g1+(g2-g1)*percent/100,b1+(b2-b1)*percent/100);
  }


// Construct classic theme
FXTheme::FXTheme(){
  setClassic();
  }


// Reset to the classic theme
void FXTheme::setClassic(){
  colors[ROLE_WINDOW_BG]=FXRGB(255,255,255);
  colors[ROLE_SURFACE]=FXRGB(212,208,200);
  colors[ROLE_SURFACE_HILITE]=makeHiliteColor(colors[ROLE_SURFACE]);
  colors[ROLE_SURFACE_SHADOW]=makeShadowColor(colors[ROLE_SURFACE]);
  colors[ROLE_TEXT]=FXRGB(0,0,0);
  colors[ROLE_BORDER]=FXRGB(0,0,0);
  colors[ROLE_SELECTION_BG]=FXRGB(10,36,106);
  colors[ROLE_SELECTION_TEXT]=FXRGB(255,255,255);
  colors[ROLE_MENU_SEL_BG]=FXRGB(10,36,106);
  colors[ROLE_MENU_SEL_TEXT]=FXRGB(255,255,255);
  colors[ROLE_TOOLTIP_BG]=FXRGB(255,255,225);
  colors[ROLE_TOOLTIP_TEXT]=FXRGB(0,0,0);
  updateDerived();
  metrics[METRIC_PADDING]=2;          // Matches DEFAULT_PAD
  metrics[METRIC_MARGIN]=0;
  metrics[METRIC_BORDER_WIDTH]=1;
  metrics[METRIC_FOCUS_WIDTH]=1;
  metrics[METRIC_SPACING]=1;
  metrics[METRIC_ICON_SIZE]=16;
  metrics[METRIC_SMALL_ICON_SIZE]=12;
  metrics[METRIC_ROW_HEIGHT]=20;
  metrics[METRIC_CORNER_RADIUS]=0;
  }


// Switch to the dark preset
void FXTheme::setDark(){
  setClassic();
  colors[ROLE_WINDOW_BG]=FXRGB(32,32,32);
  colors[ROLE_SURFACE]=FXRGB(45,45,45);
  colors[ROLE_SURFACE_HILITE]=makeHiliteColor(colors[ROLE_SURFACE]);
  colors[ROLE_SURFACE_SHADOW]=makeShadowColor(colors[ROLE_SURFACE]);
  colors[ROLE_TEXT]=FXRGB(235,235,235);
  colors[ROLE_BORDER]=FXRGB(20,20,20);
  colors[ROLE_SELECTION_BG]=FXRGB(60,110,190);
  colors[ROLE_SELECTION_TEXT]=FXRGB(255,255,255);
  colors[ROLE_MENU_SEL_BG]=FXRGB(60,110,190);
  colors[ROLE_MENU_SEL_TEXT]=FXRGB(255,255,255);
  colors[ROLE_TOOLTIP_BG]=FXRGB(60,60,45);
  colors[ROLE_TOOLTIP_TEXT]=FXRGB(240,240,240);
  updateDerived();
  }


// Load theme from an INI file
FXbool FXTheme::load(const FXString& path){
  static const FXchar *colorkeys[ROLE_LAST]={
    "window_bg","surface","surface_hilite","surface_shadow","text","text_disabled","border",
    "selection_bg","selection_text","menu_selection_bg","menu_selection_text",
    "tooltip_bg","tooltip_text","accent","focus_ring"
    };
  static const FXchar *metrickeys[METRIC_LAST]={
    "padding","margin","border_width","focus_width","spacing",
    "icon_size","small_icon_size","row_height","corner_radius"
    };
  FXSettings settings;
  FXint i;
  if(!settings.parseFile(path)) return false;
  for(i=0; i<ROLE_LAST; i++){
    colors[i]=settings.readColorEntry("colors",colorkeys[i],colors[i]);
    }
  for(i=0; i<METRIC_LAST; i++){
    metrics[i]=settings.readIntEntry("metrics",metrickeys[i],metrics[i]);
    }
  return true;
  }


// Recompute colors derived from base roles
void FXTheme::updateDerived(){
  colors[ROLE_TEXT_DISABLED]=mixColor(colors[ROLE_TEXT],colors[ROLE_SURFACE],45);
  colors[ROLE_ACCENT]=colors[ROLE_SELECTION_BG];
  colors[ROLE_FOCUS_RING]=colors[ROLE_ACCENT];
  }


// Return color of given role in given state
FXColor FXTheme::color(FXint role,FXint state) const {
  if(role<0 || ROLE_LAST<=role) return 0;
  switch(state){
    case THEME_STATE_HOVER:
      return mixColor(colors[role],FXRGB(255,255,255),8);
    case THEME_STATE_ACTIVE:
      return mixColor(colors[role],FXRGB(0,0,0),12);
    case THEME_STATE_DISABLED:
      return mixColor(colors[role],colors[ROLE_SURFACE],45);
    default:
      return colors[role];
    }
  }


// Change color of given role
void FXTheme::setColor(FXint role,FXColor clr){
  if(0<=role && role<ROLE_LAST){
    colors[role]=clr;
    }
  }


// Return value of given metric
FXint FXTheme::metric(FXint subject) const {
  if(subject<0 || METRIC_LAST<=subject) return 0;
  return metrics[subject];
  }


// Change value of given metric
void FXTheme::setMetric(FXint subject,FXint value){
  if(0<=subject && subject<METRIC_LAST){
    metrics[subject]=value;
    }
  }

}
