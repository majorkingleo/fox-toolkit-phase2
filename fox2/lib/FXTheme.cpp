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
#include "fxmath.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXStream.h"
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
  FXint i;
  colors[ROLE_WINDOW_BG]=FXRGB(255,255,255);
  colors[ROLE_SURFACE]=FXRGB(212,208,200);
  colors[ROLE_BUTTON]=colors[ROLE_SURFACE];
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
  colors[ROLE_HOVER_RING]=colors[ROLE_ACCENT];
  metrics[METRIC_PADDING]=2;          // Matches DEFAULT_PAD
  metrics[METRIC_MARGIN]=0;
  metrics[METRIC_BORDER_WIDTH]=1;
  metrics[METRIC_FOCUS_WIDTH]=1;
  metrics[METRIC_SPACING]=1;
  metrics[METRIC_ICON_SIZE]=16;
  metrics[METRIC_SMALL_ICON_SIZE]=12;
  metrics[METRIC_ROW_HEIGHT]=20;
  metrics[METRIC_CORNER_RADIUS]=0;
  metrics[METRIC_ANIMATION_TIME]=0;
  metrics[METRIC_CHECK_SIZE]=13;
  metrics[METRIC_ROW_PADDING]=2;
  metrics[METRIC_SCROLLBAR_WIDTH]=0;
  metrics[METRIC_SHADOW_SIZE]=0;
  metrics[METRIC_CONTROL_HEIGHT]=0;
  metrics[METRIC_MENUBAR_HEIGHT]=0;
  metrics[METRIC_MENUROW_HEIGHT]=0;
  scale=1.0;

  // Font roles start out as the application font; an empty description
  // inherits it, and a theme file may name a font per role
  for(i=0; i<FONT_LAST; i++){
    fonts[i].setFont(FXString::null);
    }
  }


// Switch to the dark preset
void FXTheme::setDark(){
  setClassic();
  colors[ROLE_WINDOW_BG]=FXRGB(32,32,32);
  colors[ROLE_SURFACE]=FXRGB(45,45,45);
  colors[ROLE_BUTTON]=makeHiliteColor(colors[ROLE_SURFACE],6);
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
  colors[ROLE_HOVER_RING]=colors[ROLE_ACCENT];
  }


// Switch to the built-in Breeze light preset, imported from Breeze GTK 6.8.3
void FXTheme::setBreezeLight(){
  setClassic();
  colors[ROLE_WINDOW_BG]=FXRGB(255,255,255);
  colors[ROLE_SURFACE]=FXRGB(239,240,241);
  colors[ROLE_SURFACE_HILITE]=FXRGB(244,245,246);
  colors[ROLE_SURFACE_SHADOW]=FXRGB(160,161,161);
  colors[ROLE_TEXT]=FXRGB(35,38,41);
  colors[ROLE_TEXT_DISABLED]=FXRGB(126,128,131);
  colors[ROLE_BORDER]=FXRGB(198,200,201);
  colors[ROLE_SELECTION_BG]=FXRGB(61,174,233);
  colors[ROLE_SELECTION_TEXT]=FXRGB(255,255,255);
  colors[ROLE_MENU_SEL_BG]=FXRGB(61,174,233);
  colors[ROLE_MENU_SEL_TEXT]=FXRGB(255,255,255);
  colors[ROLE_TOOLTIP_BG]=FXRGB(247,247,247);
  colors[ROLE_TOOLTIP_TEXT]=FXRGB(35,38,41);
  colors[ROLE_ACCENT]=FXRGB(61,174,233);
  colors[ROLE_FOCUS_RING]=FXRGB(61,174,233);
  colors[ROLE_BUTTON]=FXRGB(252,252,252);
  colors[ROLE_HOVER_RING]=FXRGB(61,174,233);
  metrics[METRIC_PADDING]=6;
  metrics[METRIC_ROW_HEIGHT]=15;
  metrics[METRIC_CORNER_RADIUS]=5;
  metrics[METRIC_CHECK_SIZE]=14;
  metrics[METRIC_SCROLLBAR_WIDTH]=14;
  metrics[METRIC_SHADOW_SIZE]=8;
  metrics[METRIC_CONTROL_HEIGHT]=30;
  metrics[METRIC_MENUBAR_HEIGHT]=30;
  metrics[METRIC_MENUROW_HEIGHT]=30;
  }


// Switch to the built-in Breeze dark preset, imported from Breeze GTK 6.8.3
void FXTheme::setBreezeDark(){
  setClassic();
  colors[ROLE_WINDOW_BG]=FXRGB(20,22,24);
  colors[ROLE_SURFACE]=FXRGB(32,35,38);
  colors[ROLE_SURFACE_HILITE]=FXRGB(106,108,110);
  colors[ROLE_SURFACE_SHADOW]=FXRGB(21,23,25);
  colors[ROLE_TEXT]=FXRGB(252,252,252);
  colors[ROLE_TEXT_DISABLED]=FXRGB(153,155,156);
  colors[ROLE_BORDER]=FXRGB(76,78,81);
  colors[ROLE_SELECTION_BG]=FXRGB(61,174,233);
  colors[ROLE_SELECTION_TEXT]=FXRGB(252,252,252);
  colors[ROLE_MENU_SEL_BG]=FXRGB(61,174,233);
  colors[ROLE_MENU_SEL_TEXT]=FXRGB(252,252,252);
  colors[ROLE_TOOLTIP_BG]=FXRGB(41,44,48);
  colors[ROLE_TOOLTIP_TEXT]=FXRGB(252,252,252);
  colors[ROLE_ACCENT]=FXRGB(61,174,233);
  colors[ROLE_FOCUS_RING]=FXRGB(61,174,233);
  colors[ROLE_BUTTON]=FXRGB(41,44,48);
  colors[ROLE_HOVER_RING]=FXRGB(61,174,233);
  metrics[METRIC_PADDING]=6;
  metrics[METRIC_ROW_HEIGHT]=15;
  metrics[METRIC_CORNER_RADIUS]=5;
  metrics[METRIC_CHECK_SIZE]=14;
  metrics[METRIC_SCROLLBAR_WIDTH]=14;
  metrics[METRIC_SHADOW_SIZE]=8;
  metrics[METRIC_CONTROL_HEIGHT]=30;
  metrics[METRIC_MENUBAR_HEIGHT]=30;
  metrics[METRIC_MENUROW_HEIGHT]=30;
  }


// Switch to Breeze dark chrome with the Layan dark color scheme
void FXTheme::setBreezeDarkLayan(){
  setBreezeDark();
  colors[ROLE_WINDOW_BG]=FXRGB(42,42,49);
  colors[ROLE_SURFACE]=FXRGB(51,51,59);
  colors[ROLE_SURFACE_HILITE]=FXRGB(117,117,123);
  colors[ROLE_SURFACE_SHADOW]=FXRGB(33,33,39);
  colors[ROLE_BUTTON]=FXRGB(68,69,75);
  colors[ROLE_TEXT]=FXRGB(234,234,234);
  colors[ROLE_BORDER]=FXRGB(87,87,94);
  colors[ROLE_SELECTION_BG]=FXRGB(86,87,245);
  colors[ROLE_SELECTION_TEXT]=FXRGB(255,255,255);
  colors[ROLE_MENU_SEL_BG]=FXRGB(86,87,245);
  colors[ROLE_MENU_SEL_TEXT]=FXRGB(255,255,255);
  colors[ROLE_TOOLTIP_BG]=FXRGB(33,35,41);
  colors[ROLE_TOOLTIP_TEXT]=FXRGB(221,221,221);
  updateDerived();
  colors[ROLE_TEXT_DISABLED]=FXRGB(151,151,155);
  colors[ROLE_ACCENT]=FXRGB(86,87,245);
  colors[ROLE_FOCUS_RING]=FXRGB(86,87,245);
  colors[ROLE_HOVER_RING]=FXRGB(86,87,245);
  colors[ROLE_SCROLLBAR_THUMB]=FXRGB(86,87,245);
  }


// Load theme from an INI file
FXbool FXTheme::load(const FXString& path){
  static const FXchar *colorkeys[ROLE_LAST]={
    "window_bg","surface","surface_hilite","surface_shadow","text","text_disabled","border",
    "selection_bg","selection_text","menu_selection_bg","menu_selection_text",
    "tooltip_bg","tooltip_text","accent","focus_ring","button","hover_ring","scrollbar_thumb"
    };
  static const FXchar *metrickeys[METRIC_LAST]={
    "padding","margin","border_width","focus_width","spacing",
    "icon_size","small_icon_size","row_height","corner_radius","animation_time","check_size",
    "row_padding","scrollbar_width","shadow_size","control_height","menubar_height","menu_row_height"
    };
  static const FXchar *fontkeys[FONT_LAST]={
    "ui","small","mono","title"
    };
  FXSettings settings;
  FXint i;
  FXint percent;
  FXString current;
  if(!settings.parseFile(path)) return false;
  for(i=0; i<ROLE_LAST; i++){
    colors[i]=settings.readColorEntry("colors",colorkeys[i],
                                      i==ROLE_BUTTON?colors[ROLE_SURFACE]:
                                      i==ROLE_HOVER_RING?colors[ROLE_ACCENT]:colors[i]);
    }
  for(i=0; i<METRIC_LAST; i++){
    metrics[i]=settings.readIntEntry("metrics",metrickeys[i],metrics[i]);
    }
  for(i=0; i<FONT_LAST; i++){
    current=fonts[i].getFont();
    fonts[i].setFont(FXString(settings.readStringEntry("fonts",fontkeys[i],current.text())));
    }
  percent=settings.readIntEntry("display","scale",FXint(scale*100.0));
  scale=FXdouble(percent)/100.0;
  return true;
  }


// Recompute colors derived from base roles
void FXTheme::updateDerived(){
  colors[ROLE_TEXT_DISABLED]=mixColor(colors[ROLE_TEXT],colors[ROLE_SURFACE],45);
  colors[ROLE_ACCENT]=colors[ROLE_SELECTION_BG];
  colors[ROLE_FOCUS_RING]=colors[ROLE_ACCENT];
  colors[ROLE_SCROLLBAR_THUMB]=mixColor(colors[ROLE_SURFACE],colors[ROLE_TEXT],20);
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


// Return value of given metric, scaled for the display DPI
FXint FXTheme::metric(FXint subject) const {
  if(subject<0 || METRIC_LAST<=subject) return 0;

  // Transitions are timed in milliseconds, not measured in pixels
  if(subject==METRIC_ANIMATION_TIME) return metrics[subject];
  return FXint(metrics[subject]*scale+0.5);
  }


// Return value of given metric at an explicit scale
FXint FXTheme::metric(FXint subject,FXdouble s) const {
  if(subject<0 || METRIC_LAST<=subject) return 0;
  if(subject==METRIC_ANIMATION_TIME) return metrics[subject];
  return FXint(metrics[subject]*(s>0.0?s:scale)+0.5);
  }


// Change value of given metric
void FXTheme::setMetric(FXint subject,FXint value){
  if(0<=subject && subject<METRIC_LAST){
    metrics[subject]=value;
    }
  }


// Return description of given font role
const FXFontDesc& FXTheme::font(FXint role) const {
  return fonts[(0<=role && role<FONT_LAST)?role:FONT_UI];
  }


// Change description of given font role
void FXTheme::setFont(FXint role,const FXFontDesc& desc){
  if(0<=role && role<FONT_LAST){
    fonts[role]=desc;
    }
  }


// Change DPI scale applied to length metrics
void FXTheme::setScale(FXdouble s){
  scale=(0.0<s)?s:1.0;
  }


// Return the theme as .foxtheme INI text
FXString FXTheme::describe(const FXString& name,const FXString& source) const {
  static const FXchar *colorkeys[ROLE_LAST]={
    "window_bg","surface","surface_hilite","surface_shadow","text","text_disabled","border",
    "selection_bg","selection_text","menu_selection_bg","menu_selection_text",
    "tooltip_bg","tooltip_text","accent","focus_ring","button","hover_ring","scrollbar_thumb"
    };
  static const FXchar *metrickeys[METRIC_LAST]={
    "padding","margin","border_width","focus_width","spacing",
    "icon_size","small_icon_size","row_height","corner_radius","animation_time","check_size",
    "row_padding","scrollbar_width","shadow_size","control_height","menubar_height","menu_row_height"
    };
  FXString text;
  FXint i;
  if(!source.empty()){
    text.append("# Generated by fxthemeimport from ");
    text.append(source);
    text.append("\n");
    }
  text.append("[name]\nname=");
  text.append(name.empty()?FXString("imported"):name);
  text.append("\n\n[colors]\n");
  for(i=0; i<ROLE_LAST; i++){
    text.append(FXString::value("%s=#%06x\n",colorkeys[i],color(i)&0xffffff));
    }
  text.append("\n[metrics]\n");
  for(i=0; i<METRIC_LAST; i++){
    text.append(FXString::value("%s=%d\n",metrickeys[i],metric(i)));
    }
  text.append("\n[fonts]\n");
  text.append(FXString::value("ui=%s\n",font(FONT_UI).getFont().text()));
  return text;
  }


// Save theme to a stream
void FXTheme::save(FXStream& store) const {
  FXString font;
  FXint i;
  FXint length;
  store << scale;
  for(i=0; i<ROLE_LAST; i++){
    store << (FXuint)colors[i];
    }
  for(i=0; i<METRIC_LAST; i++){
    store << metrics[i];
    }
  for(i=0; i<FONT_LAST; i++){
    font=fonts[i].getFont();
    length=font.length();
    store << length;
    store.save(font.text(),length);
    }
  }


// Load theme from a stream; strings are bounded because the cache is untrusted
void FXTheme::load(FXStream& store){
  FXString font;
  FXint i;
  FXint length;
  FXuint value;
  store >> scale;
  for(i=0; i<ROLE_LAST; i++){
    store >> value;
    colors[i]=value;
    }
  for(i=0; i<METRIC_LAST; i++){
    store >> metrics[i];
    }
  for(i=0; i<FONT_LAST; i++){
    store >> length;
    if(length<0 || 1024<length){ store.setError(FXStreamFormat); return; }
    font.length(length);
    store.load(font.text(),length);
    fonts[i].setFont(font);
    }
  }

}
