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
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,TerminalTabBar::onLeftBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,TerminalTabBar::onRightBtnPress),
  FXMAPFUNC(SEL_MOTION,0,TerminalTabBar::onMotion),
  FXMAPFUNC(SEL_LEAVE,0,TerminalTabBar::onLeave),
  };

FXIMPLEMENT(TerminalTabBar,FXPacker,TerminalTabBarMap,ARRAYNUMBER(TerminalTabBarMap))


// Construct
TerminalTabBar::TerminalTabBar(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb){
  font=getApp()->getRoleFont(FONT_UI);
  current=0;
  hover=-1;
  target=tgt;
  message=sel;
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


// Width of one tab
FXint TerminalTabBar::tabWidth(FXint index) const {
  FXint w;
  if(index<0 || index>=(FXint)titles.size() || !font) return 80;
  w=font->getTextWidth(titles[index])+32;
  return FXCLAMP(70,w,220);
  }


// Tab under a point
FXint TerminalTabBar::tabAt(FXint x) const {
  FXint left=0;
  for(FXint i=0; i<(FXint)titles.size(); i++){
    FXint right=left+tabWidth(i);
    if(left<=x && x<right) return i;
    left=right;
    }
  return -1;
  }


// Close box under a point
FXbool TerminalTabBar::closeAt(FXint index,FXint x) const {
  FXint left=0;
  for(FXint i=0; i<index; i++) left+=tabWidth(i);
  return left+tabWidth(index)-18<=x && x<left+tabWidth(index)-4;
  }


// Paint the strip
long TerminalTabBar::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  FXTheme* theme=getApp()->getTheme();
  FXint left=0;
  FXint right;
  FXint tabheight=this->height;
  dc.setForeground(theme->color(ROLE_WINDOW_BG));
  dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);
  if(!font) return 1;
  dc.setFont(font);
  for(FXint i=0; i<(FXint)titles.size() && left<width; i++){
    right=left+tabWidth(i);
    if(right<event->rect.x || left>event->rect.x+event->rect.w){
      left=right;
      continue;
      }
    dc.setForeground(theme->color(ROLE_SURFACE,i==current ? THEME_STATE_FOCUSED : (i==hover ? THEME_STATE_HOVER : THEME_STATE_NORMAL)));
    dc.fillRectangle(left,1,right-left,tabheight-2);
    if(i==current){
      dc.setForeground(theme->color(ROLE_ACCENT));
      dc.fillRectangle(left,0,right-left,2);
      }
    dc.setForeground(theme->color(i==current ? ROLE_TEXT : ROLE_TEXT_DISABLED));
    dc.drawText(left+8,(tabheight+font->getFontAscent())/2,titles[i]);
    dc.setForeground(theme->color(i==current ? ROLE_TEXT : ROLE_TEXT_DISABLED));
    dc.drawLine(right-14,tabheight/2-3,right-8,tabheight/2+3);
    dc.drawLine(right-14,tabheight/2+3,right-8,tabheight/2-3);
    left=right;
    }
  return 1;
  }


// Select a tab or close it
long TerminalTabBar::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=tabAt(event->win_x);
  if(index>=0){
    if(closeAt(index,event->win_x)){
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,ID_CLOSE_TAB),(void*)(FXival)index);
      }
    else{
      setCurrent(index);
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,ID_TAB),(void*)(FXival)index);
      }
    }
  return 1;
  }


// Post the tab menu; a press on the empty part of the strip posts the same
// menu with an index of -1, so its entries are shown disabled, not hidden
long TerminalTabBar::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=tabAt(event->win_x);
  if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,ID_CONTEXT_TAB),(void*)(FXival)index);
  return 1;
  }


// Track hover
long TerminalTabBar::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=tabAt(event->win_x);
  if(hover!=index){
    hover=index;
    update();
    }
  return 1;
  }


// Leave
long TerminalTabBar::onLeave(FXObject*,FXSelector,void*){
  if(hover!=-1){
    hover=-1;
    update();
    }
  return 1;
  }


// Replace all titles
void TerminalTabBar::setTitles(const std::vector<FXString>& text){
  titles=text;
  if(current>=(FXint)titles.size()) current=FXMAX(0,(FXint)titles.size()-1);
  update();
  }


// Append a title
void TerminalTabBar::addTitle(const FXString& text){
  titles.push_back(text);
  update();
  }


// Remove a title
void TerminalTabBar::removeTitle(FXint index){
  if(index<0 || index>=(FXint)titles.size()) return;
  titles.erase(titles.begin()+index);
  if(current>=(FXint)titles.size()) current=FXMAX(0,(FXint)titles.size()-1);
  update();
  }


// Change a title
void TerminalTabBar::setTitle(FXint index,const FXString& text){
  if(index<0 || index>=(FXint)titles.size()) return;
  titles[index]=text;
  update();
  }


// Return a title
const FXString& TerminalTabBar::getTitle(FXint index) const {
  static const FXString empty;
  if(index<0 || index>=(FXint)titles.size()) return empty;
  return titles[index];
  }


// Set active tab
void TerminalTabBar::setCurrent(FXint index){
  if(!titles.empty()){
    index=FXCLAMP(0,index,(FXint)titles.size()-1);
    if(current!=index){
      current=index;
      update();
      }
    }
  }


// Destructor
TerminalTabBar::~TerminalTabBar(){
  font=(FXFont*)-1L;
  }

}

/*******************************************************************************/
