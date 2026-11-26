/********************************************************************************
*                                                                               *
*                         T e r m i n a l   T a b   B a r                       *
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
#include "fx.h"
#include "TerminalTabBar.h"

using namespace FX;

/*******************************************************************************/

namespace FX {

FXDEFMAP(TerminalTabBar) TerminalTabBarMap[]={
  FXMAPFUNC(SEL_PAINT,0,TerminalTabBar::onPaint),
  };

FXIMPLEMENT(TerminalTabBar,FXPacker,TerminalTabBarMap,ARRAYNUMBER(TerminalTabBarMap))


// Construct
TerminalTabBar::TerminalTabBar(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb){
  font=getApp()->getRoleFont(FONT_UI);
  title="Terminal";
  active=true;
  }


// Create
void TerminalTabBar::create(){
  FXPacker::create();
  if(font) font->create();
  }


// Detach
void TerminalTabBar::detach(){
  if(font) font->detach();
  FXPacker::detach();
  }


// Re-apply theme
void TerminalTabBar::applyTheme(FXTheme* theme){
  FXPacker::applyTheme(theme);
  font=getApp()->retargetRoleFont(font,FONT_UI,getWindowDPI());
  recalc();
  update();
  }


// Default height
FXint TerminalTabBar::getDefaultHeight(){
  return font ? font->getFontHeight()+getPadTop()+getPadBottom() : 24;
  }


// Paint the strip
long TerminalTabBar::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  FXTheme* theme=getApp()->getTheme();
  FXint pad=6;
  FXint tabw=font ? font->getTextWidth(title)+pad*2 : 100;
  FXint tabh=height;
  tabw=FXMIN(tabw,width);
  dc.setForeground(theme->color(ROLE_WINDOW_BG));
  dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);
  dc.setForeground(theme->color(ROLE_SURFACE,active ? THEME_STATE_FOCUSED : THEME_STATE_NORMAL));
  dc.fillRectangle(0,1,tabw,tabh-2);
  dc.setForeground(theme->color(ROLE_ACCENT));
  dc.fillRectangle(0,0,tabw,2);
  if(font){
    dc.setFont(font);
    dc.setForeground(theme->color(active ? ROLE_TEXT : ROLE_TEXT_DISABLED));
    dc.drawText(pad,(tabh+font->getFontAscent())/2,title);
    }
  return 1;
  }


// Change title
void TerminalTabBar::setTitle(const FXString& text){
  if(title!=text){
    title=text;
    update();
    }
  }


// Set active
void TerminalTabBar::setActive(FXbool flag){
  if(active!=flag){
    active=flag;
    update();
    }
  }


// Destructor
TerminalTabBar::~TerminalTabBar(){
  font=(FXFont*)-1L;
  }

}
