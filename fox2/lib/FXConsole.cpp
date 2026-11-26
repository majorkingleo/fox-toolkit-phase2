/********************************************************************************
*                                                                               *
*                         C o n s o l e   W i d g e t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include <new>
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXArray.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXString.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXDCWindow.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXConsole.h"


/*
  To do:
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {


FXDEFMAP(FXConsole) FXConsoleMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXConsole::onPaint),
  FXMAPFUNC(SEL_COMMAND,FXConsole::ID_XXX,FXConsole::onXXX),
  FXMAPFUNC(SEL_COMMAND,FXScrollArea::ID_VSCROLLED,FXConsole::onVScrollerChanged),
  FXMAPFUNC(SEL_CHANGED,FXScrollArea::ID_VSCROLLED,FXConsole::onVScrollerDragged),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXConsole::onMouseWheel),
  };


// Object implementation
FXIMPLEMENT(FXConsole,FXScrollArea,FXConsoleMap,ARRAYNUMBER(FXConsoleMap))


// Construct empty bounded console storage
FXConsoleBuffer::FXConsoleBuffer():contents(10000),styles(10000),capacity(10000),first(0),count(0),lineopen(false){
  }


// Start a new line, evicting the oldest line when the buffer is full
FXint FXConsoleBuffer::beginLine(){
  FXint index;
  if(count<capacity){
    index=(first+count)%capacity;
    count++;
    }
  else{
    index=first;
    first=(first+1)%capacity;
    }
  contents[index].length(0);
  styles[index].length(0);
  lineopen=true;
  return index;
  }


// Change maximum number of retained lines, preserving the newest lines
void FXConsoleBuffer::setHistoryLines(FXint lines){
  if(lines<1) lines=1;
  if(capacity==lines) return;
  FXint keep=FXMIN(count,lines);
  FXStringBuffer newcontents(lines);
  FXStringBuffer newstyles(lines);
  FXint skip=count-keep;
  for(FXint i=0; i<keep; i++){
    FXint index=(first+skip+i)%capacity;
    newcontents[i]=contents[index];
    newstyles[i]=styles[index];
    }
  contents=newcontents;
  styles=newstyles;
  capacity=lines;
  first=0;
  count=keep;
  }


// Append text and split it into logical lines
void FXConsoleBuffer::appendText(const FXString& text,FXuint style){
  FXuint value=style&0xFF;
  for(FXint i=0; i<text.length(); i++){
    FXchar ch=text[i];
    if(ch=='\n'){
      if(!lineopen) beginLine();
      lineopen=false;
      continue;
      }
    FXint index=lineopen ? (first+count-1)%capacity : beginLine();
    contents[index]+=ch;
    styles[index]+=(FXchar)value;
    }
  }


// Append a line and close it without creating an empty trailing line
void FXConsoleBuffer::appendLine(const FXString& text,FXuint style){
  if(text.empty()){
    if(!lineopen) beginLine();
    lineopen=false;
    return;
    }
  appendText(text,style);
  lineopen=false;
  }


// Append text with a per-byte style index
void FXConsoleBuffer::appendStyled(const FXString& text,const FXuint* values){
  for(FXint i=0; i<text.length(); i++){
    FXchar ch=text[i];
    if(ch=='\n'){
      if(!lineopen) beginLine();
      lineopen=false;
      continue;
      }
    FXint index=lineopen ? (first+count-1)%capacity : beginLine();
    contents[index]+=ch;
    styles[index]+=(FXchar)(values ? values[i]&0xFF : 0);
    }
  }


// Clear all retained lines
void FXConsoleBuffer::clear(){
  for(FXint i=0; i<capacity; i++){
    contents[i].length(0);
    styles[i].length(0);
    }
  first=0;
  count=0;
  lineopen=false;
  }


// Return logical line by index
const FXString& FXConsoleBuffer::line(FXint lineindex) const {
  static const FXString empty;
  if(lineindex<0 || lineindex>=count) return empty;
  return contents[(first+lineindex)%capacity];
  }


// Return logical line by index
FXString FXConsoleBuffer::getLineText(FXint lineindex) const {
  return line(lineindex);
  }


// Return style for byte in logical line
FXuint FXConsoleBuffer::getStyle(FXint lineindex,FXint byte) const {
  if(lineindex<0 || lineindex>=count) return 0;
  const FXString& text=contents[(first+lineindex)%capacity];
  if(byte<0 || byte>=text.length()) return 0;
  return (FXuchar)styles[(first+lineindex)%capacity][byte];
  }


// Deserialization
FXConsole::FXConsole(){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  font=nullptr;
  margintop=0;
  marginbottom=0;
  marginleft=0;
  marginright=0;
  historylines=10000;
  visiblelines=0;
  topline=0;
  vrows=0;
  vcols=0;
  textColor=0;
  selbackColor=0;
  seltextColor=0;
  cursorColor=0;
  contentwidth=1;
  contentheight=0;
  layoutwidth=0;
  autoscroll=true;
  scrollpending=false;
  }


// Construct and init
FXConsole::FXConsole(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXScrollArea(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  defaultCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  font=getApp()->getRoleFont(FONT_MONO);
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  cursorColor=getApp()->getForeColor();
  target=tgt;
  message=sel;
  margintop=pt;
  marginbottom=pb;
  marginleft=pl;
  marginright=pr;
  historylines=10000;
  visiblelines=1;
  topline=0;
  contentwidth=marginleft+marginright+1;
  contentheight=margintop+marginbottom;
  layoutwidth=0;
  vrows=0;
  vcols=0;
  autoscroll=true;
  scrollpending=false;
  }


// Create window
void FXConsole::create(){
  FXScrollArea::create();
  font->create();
  recalc();
  }


// Detach window
void FXConsole::detach(){
  FXScrollArea::detach();
  font->detach();
  }


// If window can have focus
FXbool FXConsole::canFocus() const {
  return true;
  }


// Get default width
FXint FXConsole::getDefaultWidth(){
  return 0<vcols ? marginleft+marginright+vcols*font->getTextWidth("8",1) : FXScrollArea::getDefaultWidth();
  }


// Get default height
FXint FXConsole::getDefaultHeight(){
  return 0<vrows ? margintop+marginbottom+vrows*font->getFontHeight() : FXScrollArea::getDefaultHeight();
  }


// Move content
void FXConsole::moveContents(FXint x,FXint y){
  FXint dx=x-pos_x;
  FXint dy=y-pos_y;
  pos_x=x;
  pos_y=y;
  scroll(marginleft,margintop,getVisibleWidth()-marginleft-marginright,getVisibleHeight()-margintop-marginbottom,dx,dy);
  }


// Determine minimum content width of scroll area
FXint FXConsole::getContentWidth(){
  return contentwidth;
  }


// Determine minimum content height of scroll area
FXint FXConsole::getContentHeight(){
  return contentheight;
  }


// Recalculate layout
void FXConsole::layout(){
  FXint hh=font->getFontHeight();

  // Number of visible lines has changed
  visiblelines=(height-margintop-marginbottom+hh-1)/hh;
  if(visiblelines<1) visiblelines=1;

  FXTRACE(TOPIC_DETAIL,"visiblelines=%d historylines=%d lines=%d hh=%d space=%d\n",visiblelines,historylines,buffer.getNumLines(),hh,height-margintop-marginbottom);

  // Build row layout using the current viewport, then converge after scrollbar placement
  for(FXint pass=0; pass<3; pass++){
    FXint available=0;
    if(options&CONSOLE_WRAPLINES) available=FXMAX(1,getVisibleWidth()-marginleft-marginright);
    rebuildRows(available);
    placeScrollBars(width,height);
    if(!(options&CONSOLE_WRAPLINES)) break;
    FXint actual=FXMAX(1,getVisibleWidth()-marginleft-marginright);
    if(actual==layoutwidth) break;
    }

  // Keep the viewport at the end when following is enabled
  if(scrollpending || autoscroll){
    FXint bottom=FXMAX(0,getContentHeight()-getVisibleHeight());
    setPosition(pos_x,-bottom);
    }
  scrollpending=false;

  // No more dirty
  flags&=~FLAG_DIRTY;
  }


// Character width
FXint FXConsole::charWidth(FXwchar ch,FXint col) const {
  if(ch<' '){
    if(ch!='\t'){
      return font->getCharWidth('#');
      }
    return font->getCharWidth(' ')*(8-col%8);
    }
  return font->getCharWidth(ch);
  }


// Measure a complete logical line
FXint FXConsole::lineWidth(const FXString& text) const {
  FXint width=0;
  FXint col=0;
  for(FXint pos=0; pos<text.length(); pos=text.inc(pos),col++) width+=charWidth(text.wc(pos),col);
  return width;
  }


// Rebuild the visual row map and content dimensions
void FXConsole::rebuildRows(FXint availableWidth){
  rows.clear();
  contentwidth=marginleft+marginright+1;
  layoutwidth=availableWidth;
  FXint maxwidth=0;
  for(FXint lineindex=0; lineindex<buffer.getNumLines(); lineindex++){
    const FXString& text=buffer.line(lineindex);
    if(!(options&CONSOLE_WRAPLINES)){
      FXint w=lineWidth(text);
      rows.push_back({lineindex,0,text.length(),w});
      maxwidth=FXMAX(maxwidth,w);
      continue;
      }
    FXint start=0;
    FXint pos=0;
    FXint col=0;
    FXint rowwidth=0;
    if(text.empty()) rows.push_back({lineindex,0,0,0});
    while(pos<text.length()){
      FXint cw=charWidth(text.wc(pos),col);
      if(rowwidth>0 && rowwidth+cw>availableWidth){
        rows.push_back({lineindex,start,pos,rowwidth});
        maxwidth=FXMAX(maxwidth,rowwidth);
        start=pos;
        rowwidth=0;
        col=0;
        continue;
        }
      rowwidth+=cw;
      pos=text.inc(pos);
      col++;
      }
    if(start<text.length()){
      rows.push_back({lineindex,start,text.length(),rowwidth});
      maxwidth=FXMAX(maxwidth,rowwidth);
      }
    }
  if(!(options&CONSOLE_WRAPLINES)) contentwidth=marginleft+marginright+maxwidth;
  contentheight=margintop+marginbottom+font->getFontHeight()*FXMAX(visiblelines,(FXint)rows.size());
  }


// Re-evaluate auto-scroll after user scrolling
void FXConsole::updateAutoScrollFromPosition(){
  FXint bottom=FXMAX(0,getContentHeight()-getVisibleHeight());
  autoscroll=(-getContentY()>=bottom);
  }


// Determine style
FXuint FXConsole::styleOf(FXint line,FXint p) const {
  FXuint s=0;
  FXchar ch;

  // Selected part of text
//  if(selstartpos<=p && p<selendpos) s|=STYLE_SELECTED;

  // Highlighted part of text
 // if(hilitestartpos<=p && p<hiliteendpos) s|=STYLE_HILITE;

  // Current active line
//  if((row==cursorrow)&&(options&TEXT_SHOWACTIVE)) s|=STYLE_ACTIVE;

  // Blank part of line
  const FXString& text=buffer.line(line);
  if(p>=text.length()) return s;

  // Special style for control characters
  ch=text[p];

  // Get value from style buffer
  s|=buffer.getStyle(line,p);

  // Whitespace has no glyph but still fills a styled background
  if(ch=='\t' || ch==' ') return s|STYLE_TEXT;

  // Newlines are stored as line boundaries, not as characters
  if(ch=='\n') return s;

  // Get special style for control codes
  if((FXuchar)ch < ' ') return s|STYLE_CONTROL|STYLE_TEXT;

  return s|STYLE_TEXT;
  }


// Draw fragment of text in given style
void FXConsole::drawTextFragment(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXchar *text,FXint n,FXuint sty) const {
  FXuint index=sty&STYLE_MASK;
  FXuint usedstyle=sty;
  FXColor fore=0;
  FXColor back=0;
  if(index && index<=styletable.no()){
    const FXHiliteStyle& entry=styletable[index-1];
    usedstyle=entry.style;
    if(sty&STYLE_SELECTED){ back=entry.selectBackColor; fore=entry.selectForeColor; }
    else if(sty&STYLE_HILITE){ back=entry.hiliteBackColor; fore=entry.hiliteForeColor; }
    else if(sty&STYLE_ACTIVE) back=entry.activeBackColor;
    else back=entry.normalBackColor;
    if(!fore) fore=entry.normalForeColor;
    }
  if(!back){
    if(sty&STYLE_SELECTED) back=selbackColor;
    else back=backColor;
    }
  if(!fore){
    if(sty&STYLE_SELECTED) fore=seltextColor;
    if(!fore) fore=textColor;
    }
  dc.setForeground(back);
  if(0<w && 0<h) dc.fillRectangle(x,y,w,h);
  dc.setForeground(fore);
  y+=font->getFontAscent();
  dc.drawText(x,y,text,n);
  if(usedstyle&FXText::STYLE_BOLD) dc.drawText(x+1,y,text,n);
  if(usedstyle&FXText::STYLE_UNDERLINE) dc.fillRectangle(x,y+1,w,1);
  if(usedstyle&FXText::STYLE_STRIKEOUT) dc.fillRectangle(x,y-font->getFontAscent()/2,w,1);
  }


// Draw text line with correct style
void FXConsole::drawTextLine(FXDCWindow& dc,FXint line,FXint left,FXint right) const {
  if(line<0 || line>=(FXint)rows.size()) return;
  const VisualRow& row=rows[line];
  const FXString& text=buffer.line(row.line);
  FXint edge=pos_x+marginleft;
  FXint h=font->getFontHeight();
  FXint y=pos_y+margintop+line*h;
  FXint x=0;
  FXint w=0;
  FXint start=row.start;
  FXint end=row.end;
  FXuint curstyle;
  FXuint newstyle;
  FXint cw,sp,ep,sc,ec;

  // Skip columns left of the viewport when horizontal scrolling is enabled
  for(sp=sc=start; sp<end; sp=text.inc(sp),sc++){
    cw=charWidth(text.wc(sp),sc-start);
    if(x+edge+cw>=left) break;
    x+=cw;
    }

  // First style to display
  curstyle=styleOf(row.line,sp);

  // Draw until we hit the end or the right edge
  for(ep=sp,ec=sc; ep<end; ep=text.inc(ep),ec++){
    newstyle=styleOf(row.line,ep);
    if(newstyle!=curstyle){
      if(curstyle&STYLE_TEXT) drawTextFragment(dc,edge+x,y,w,h,&text[sp],ep-sp,curstyle);
      curstyle=newstyle;
      sp=ep;
      x+=w;
      w=0;
      }
    cw=charWidth(text.wc(ep),ec-start);
    if(x+edge+w>=right) break;
    w+=cw;
    }

  // Draw unfinished fragment
  if(curstyle&STYLE_TEXT) drawTextFragment(dc,edge+x,y,w,h,&text[sp],ep-sp,curstyle);
  x+=w;

  // Fill any left-overs outside of text
  if(x+edge<right){
    curstyle=styleOf(row.line,ep);
    }
  }


// Repaint lines of text
void FXConsole::drawContents(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h) const {
  FXint hh=font->getFontHeight();
  FXint yy=pos_y+margintop;
  FXint tl=(y-yy)/hh;
  FXint bl=(y+h-yy)/hh;
  FXint ln;
  if(tl<0) tl=0;
  if(bl>=(FXint)rows.size()) bl=(FXint)rows.size()-1;
  FXTRACE(TOPIC_DETAIL,"tl=%d bl=%d\n",tl,bl);
  for(ln=tl; ln<=bl; ln++){
    drawTextLine(dc,ln,x,x+w);
    }
  }


// Draw the text
long FXConsole::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  dc.setFont(font);

  // Clear the whole exposed area; the margins below cover only their strips
  dc.setForeground(backColor);
  dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);

  // Viewport
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();

  // Paint top margin
  if(event->rect.y<=margintop){
    dc.setForeground(backColor);
    dc.fillRectangle(0,0,vw,margintop);
    }

  // Paint bottom margin
  if(event->rect.y+event->rect.h>=vh-marginbottom){
    dc.setForeground(backColor);
    dc.fillRectangle(0,vh-marginbottom,vw,marginbottom);
    }

  // Paint left margin
  if(event->rect.x<marginleft){
    dc.setForeground(backColor);
    dc.fillRectangle(0,margintop,marginleft,vh-margintop-marginbottom);
    }

  // Paint right margin
  if(event->rect.x+event->rect.w>=vw-marginright){
    dc.setForeground(backColor);
    dc.fillRectangle(vw-marginright,margintop,marginright,vh-margintop-marginbottom);
    }

  // Paint text
  dc.setClipRectangle(marginleft,margintop,vw-marginright-marginleft,vh-margintop-marginbottom);
  drawContents(dc,event->rect.x,event->rect.y,event->rect.w,event->rect.h);

  return 1;
  }


// Draw the text
long FXConsole::onXXX(FXObject*,FXSelector,void*){
  return 1;
  }


// Vertical scrollbar moved
long FXConsole::onVScrollerChanged(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onVScrollerChanged(sender,sel,ptr);
  updateAutoScrollFromPosition();
  return 1;
  }


// Vertical scrollbar dragged
long FXConsole::onVScrollerDragged(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onVScrollerDragged(sender,sel,ptr);
  updateAutoScrollFromPosition();
  return 1;
  }


// Mouse wheel moved the viewport
long FXConsole::onMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  long result=FXScrollArea::onMouseWheel(sender,sel,ptr);
  updateAutoScrollFromPosition();
  return result;
  }


// Change top margin
void FXConsole::setMarginTop(FXint mt){
  if(margintop!=mt){
    margintop=mt;
    recalc();
    update();
    }
  }


// Change bottom margin
void FXConsole::setMarginBottom(FXint mb){
  if(marginbottom!=mb){
    marginbottom=mb;
    recalc();
    update();
    }
  }


// Change left margin
void FXConsole::setMarginLeft(FXint ml){
  if(marginleft!=ml){
    marginleft=ml;
    recalc();
    update();
    }
  }


// Change right margin
void FXConsole::setMarginRight(FXint mr){
  if(marginright!=mr){
    marginright=mr;
    recalc();
    update();
    }
  }


// Change history lines
void FXConsole::setHistoryLines(FXint hl){
  if(hl<1) hl=1;
  if(historylines!=hl){
    historylines=hl;
    buffer.setHistoryLines(hl);
    recalc();
    update();
    }
  }


// Append text
void FXConsole::appendText(const FXString& text,FXuint sty){
  buffer.appendText(text,sty);
  recalc();
  update();
  }


// Append line
void FXConsole::appendLine(const FXString& text,FXuint sty){
  buffer.appendLine(text,sty);
  recalc();
  update();
  }


// Append styled text
void FXConsole::appendStyled(const FXString& text,const FXuint* styles){
  buffer.appendStyled(text,styles);
  recalc();
  update();
  }


// Clear text
void FXConsole::clearText(){
  buffer.clear();
  setPosition(0,0);
  recalc();
  update();
  }


// Set style table; style id zero remains the default style
void FXConsole::setStyleTable(const FXHiliteStyle* styles,FXuint count){
  if(count>255) count=255;
  if(!styles) count=0;
  styletable.no(count);
  for(FXuint i=0; i<count; i++) styletable[i]=styles[i];
  update();
  }


// Enable or disable following new text
void FXConsole::setAutoScroll(FXbool on){
  if(on) scrollToEnd();
  else autoscroll=false;
  }


// Scroll to the end and resume following
void FXConsole::scrollToEnd(){
  autoscroll=true;
  scrollpending=true;
  recalc();
  update();
  }


// Change number of visible rows
void FXConsole::setVisibleRows(FXint rows){
  if(rows<0) rows=0;
  if(vrows!=rows){
    vrows=rows;
    recalc();
    }
  }


// Change number of visible columns
void FXConsole::setVisibleColumns(FXint cols){
  if(cols<0) cols=0;
  if(vcols!=cols){
    vcols=cols;
    recalc();
    }
  }


// Re-resolve the normal font for the window's monitor
void FXConsole::applyTheme(FXTheme* theme){
  FXScrollArea::applyTheme(theme);

  // The console is a view onto a log, so its page takes the window background
  if(!backColorOverride && 0<theme->metric(METRIC_CORNER_RADIUS)){
    backColor=theme->color(ROLE_WINDOW_BG);
    }
  textColor=theme->color(ROLE_TEXT);
  selbackColor=theme->color(ROLE_SELECTION_BG);
  seltextColor=theme->color(ROLE_SELECTION_TEXT);
  cursorColor=theme->color(ROLE_TEXT);
  setFont(getApp()->retargetRoleFont(font,FONT_MONO,getWindowDPI()));
  update();
  }


// Change the font
void FXConsole::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Save object to stream
void FXConsole::save(FXStream& store) const {
  FXScrollArea::save(store);
  store << margintop;
  store << marginbottom;
  store << marginleft;
  store << marginright;
  store << font;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << cursorColor;
  store << help;
  store << tip;
  }


// Load object from stream
void FXConsole::load(FXStream& store){
  FXScrollArea::load(store);
  store >> margintop;
  store >> marginbottom;
  store >> marginleft;
  store >> marginright;
  store >> font;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> cursorColor;
  store >> help;
  store >> tip;
  }


// Clean up
FXConsole::~FXConsole(){
  font=(FXFont*)-1L;
  }

}
