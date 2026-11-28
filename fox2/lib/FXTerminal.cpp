/********************************************************************************
*                                                                               *
*                         T e r m i n a l   W i d g e t                         *
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
#include "fxkeys.h"
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
#include "FXComposeContext.h"
#include "FXTheme.h"
#include "FXTerminal.h"
#include "fxterminalscreen.h"
#include "fxterminalcharset.h"
#include "fxterminalparser.h"
#include "fxpty.h"

#include <vector>

using namespace FX;

/*******************************************************************************/

namespace FX {

// A find match in absolute row and cell coordinates
struct FXTerminalFind {
  FXint row;
  FXint col;
  FXint length;
  };


// Private terminal state
struct FXTerminalPrivate {
  FXTerminalScreen      screen;
  FXTerminalCharset     charset;
  FXTerminalParser      parser;
  FXPty                 pty;
  FXFont               *font;
  FXObject             *notify;
  FXSelector            notifyselector;
  FXint                 cellw;
  FXint                 cellh;
  FXint                 ascent;
  FXint                 padding;
  FXint                 marginleft;
  FXint                 marginright;
  FXint                 margintop;
  FXint                 marginbottom;
  FXint                 contentwidth;
  FXint                 contentheight;
  FXint                 selstartrow;
  FXint                 selstartcol;
  FXint                 selendrow;
  FXint                 selendcol;
  FXint                 anchorrow;
  FXint                 anchorcol;
  FXint                 mousemode;
  FXbool                mousereport;
  FXbool                rightreport;
  FXbool                follow;
  FXbool                blinkon;
  FXbool                fixedpitch;
  FXbool                readwatch;
  FXbool                writewatch;
  FXbool                exitnotified;
  FXint                 kind;
  FXString              term;
  FXString              clipped;
  FXString              clipboardrequest;
  FXString              findpattern;
  FXuint                findflags;
  std::vector<FXwchar>  oddadvance;
  FXint                 findindex;
  std::vector<FXTerminalFind> matches;
  FXString              lasttitle;
  FXString              lastdirectory;
  FXint                 lastbell;
  FXColor               defaultfg;
  FXColor               defaultbg;
  FXColor               cursorcolor;
  FXColor               selbackcolor;
  FXColor               seltextcolor;
  FXTerminalPrivate():parser(screen,&charset),font(nullptr),notify(nullptr),notifyselector(0),cellw(1),cellh(1),ascent(1),padding(-1),marginleft(0),marginright(0),margintop(0),marginbottom(0),contentwidth(1),contentheight(1),selstartrow(-1),selstartcol(-1),selendrow(-1),selendcol(-1),anchorrow(-1),anchorcol(-1),mousemode(0),mousereport(false),rightreport(false),follow(true),blinkon(true),fixedpitch(false),readwatch(false),writewatch(false),exitnotified(false),kind(TERMINAL_XTERM),findflags(0),findindex(-1),lastbell(0),defaultfg(FXRGB(204,204,204)),defaultbg(FXRGB(12,12,12)),cursorcolor(FXRGB(255,255,255)),selbackcolor(FXRGB(38,79,120)),seltextcolor(FXRGB(255,255,255)){ }
  };


// Object implementation
FXDEFMAP(FXTerminal) FXTerminalMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXTerminal::onPaint),
  FXMAPFUNC(SEL_KEYPRESS,0,FXTerminal::onKeyPress),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXTerminal::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXTerminal::onLeftBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXTerminal::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXTerminal::onRightBtnRelease),
  FXMAPFUNC(SEL_MOTION,0,FXTerminal::onMotion),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXTerminal::onMouseWheel),
  FXMAPFUNC(SEL_FOCUSIN,0,FXTerminal::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXTerminal::onFocusOut),
  FXMAPFUNC(SEL_IME_START,0,FXTerminal::onIMEStart),
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,FXTerminal::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,FXTerminal::onClipboardRequest),
  FXMAPFUNC(SEL_SELECTION_LOST,0,FXTerminal::onSelectionLost),
  FXMAPFUNC(SEL_SELECTION_REQUEST,0,FXTerminal::onSelectionRequest),
  FXMAPFUNC(SEL_IO_READ,FXTerminal::ID_READ,FXTerminal::onRead),
  FXMAPFUNC(SEL_IO_WRITE,FXTerminal::ID_WRITE,FXTerminal::onWrite),
  FXMAPFUNC(SEL_TIMEOUT,FXTerminal::ID_BLINK,FXTerminal::onBlink),
  FXMAPFUNC(SEL_COMMAND,FXScrollArea::ID_VSCROLLED,FXTerminal::onVScrollerChanged),
  FXMAPFUNC(SEL_CHANGED,FXScrollArea::ID_VSCROLLED,FXTerminal::onVScrollerDragged),
  };


FXIMPLEMENT(FXTerminal,FXScrollArea,FXTerminalMap,ARRAYNUMBER(FXTerminalMap))


/*******************************************************************************/

// Return the total number of rows, including scrollback
static FXint totalRows(FXTerminalPrivate *p){
  return p->screen.getScrollbackCount()+p->screen.getRows();
  }


// Return one row
static const FXTerminalRow& rowAt(FXTerminalPrivate *p,FXint row){
  if(row<p->screen.getScrollbackCount()) return p->screen.getScrollbackRow(row);
  return p->screen.getRow(row-p->screen.getScrollbackCount());
  }


// Return one cell text
static FXString cellTextAt(FXTerminalPrivate *p,FXint row,FXint col){
  if(row<p->screen.getScrollbackCount()) return p->screen.getScrollbackCellText(row,col);
  return p->screen.getCellText(row-p->screen.getScrollbackCount(),col);
  }


// Return the text of one absolute row
static FXString rowTextAt(FXTerminalPrivate *p,FXint row,FXbool trimmed){
  if(row<p->screen.getScrollbackCount()) return p->screen.getScrollbackText(row,trimmed);
  return p->screen.getRowText(row-p->screen.getScrollbackCount(),trimmed);
  }


// Resolve a terminal color
static FXColor resolveColor(FXTerminalPrivate *p,FXuint color,FXColor def){
  if(color==TERMCOLOR_DEFAULT_FG || color==TERMCOLOR_DEFAULT_BG) return def;
  if(color&TERMCOLOR_RGB) return FXRGB((color>>16)&0xFF,(color>>8)&0xFF,color&0xFF);
  return p->screen.getPaletteColor((FXint)(color&0xFF));
  }


// Normalize a selection
static void normalizeSelection(FXTerminalPrivate *p,FXint& r1,FXint& c1,FXint& r2,FXint& c2){
  if(p->selstartrow<0 || p->selendrow<0){
    r1=c1=r2=c2=-1;
    return;
    }
  r1=p->selstartrow;
  c1=p->selstartcol;
  r2=p->selendrow;
  c2=p->selendcol;
  if(r1>r2 || (r1==r2 && c1>c2)){
    FXint t;
    t=r1; r1=r2; r2=t;
    t=c1; c1=c2; c2=t;
    }
  }


// Test whether a cell is selected
static FXbool isSelected(FXTerminalPrivate *p,FXint row,FXint col){
  FXint r1,c1,r2,c2;
  normalizeSelection(p,r1,c1,r2,c2);
  if(r1<0 || row<r1 || row>r2) return false;
  if(r1==r2) return c1<=col && col<=c2;
  if(row==r1) return c1<=col;
  if(row==r2) return col<=c2;
  return true;
  }


// Select one cell or extend the selection
static void setSelection(FXTerminalPrivate *p,FXint row,FXint col,FXbool extend){
  FXint total=totalRows(p);
  if(total<=0) return;
  row=FXCLAMP(0,row,total-1);
  col=FXCLAMP(0,col,FXMAX(0,p->screen.getColumns()-1));
  if(!extend || p->selstartrow<0){
    p->selstartrow=row;
    p->selstartcol=col;
    }
  p->selendrow=row;
  p->selendcol=col;
  p->anchorrow=row;
  p->anchorcol=col;
  }


// Select a whole line
static void setLineSelection(FXTerminalPrivate *p,FXint row){
  FXint total=totalRows(p);
  if(total<=0) return;
  row=FXCLAMP(0,row,total-1);
  p->selstartrow=row;
  p->selstartcol=0;
  p->selendrow=row;
  p->selendcol=FXMAX(0,p->screen.getColumns()-1);
  p->anchorrow=row;
  p->anchorcol=0;
  }


// Select a word around a cell
static void setWordSelection(FXTerminalPrivate *p,FXint row,FXint col){
  FXString text=rowTextAt(p,row,false);
  FXint start=col;
  FXint end=col;
  FXbool word=false;
  while(start>0){
    FXString c=cellTextAt(p,row,start-1);
    if(c.empty() || c[0]==' ' || c[0]=='\t') break;
    word=true;
    start--;
    }
  while(end<p->screen.getColumns()){
    FXString c=cellTextAt(p,row,end);
    if(c.empty() || c[0]==' ' || c[0]=='\t') break;
    word=true;
    end++;
    }
  if(!word){ start=col; end=FXMIN(col+1,p->screen.getColumns()); }
  p->selstartrow=row;
  p->selstartcol=start;
  p->selendrow=row;
  p->selendcol=FXMAX(start,end-1);
  p->anchorrow=row;
  p->anchorcol=start;
  }


// Map a content row to the first visible row
static FXint firstVisibleRow(FXTerminal *term,FXTerminalPrivate *p){
  FXint first=(-term->getContentY()-p->margintop)/FXMAX(1,p->cellh);
  return FXMAX(0,first);
  }


// Map window coordinates to an absolute row and column
static void cellAtPoint(FXTerminal *term,FXTerminalPrivate *p,FXint x,FXint y,FXint& row,FXint& col){
  FXint first=firstVisibleRow(term,p);
  FXint total=totalRows(p);
  row=first+(y-p->margintop)/FXMAX(1,p->cellh);
  col=(x-p->marginleft)/FXMAX(1,p->cellw);
  row=FXCLAMP(0,row,FXMAX(0,total-1));
  col=FXCLAMP(0,col,FXMAX(0,p->screen.getColumns()-1));
  }


// Send one mouse report
static void sendMouseReport(FXTerminalPrivate *p,FXTerminal *term,FXint button,FXint row,FXint col,FXbool release){
  FXString report;
  FXint history=p->screen.getScrollbackCount();
  FXint screenrow=row-history;
  FXint screencol=col;
  if(screenrow<0 || screenrow>=p->screen.getRows()) return;
  if(release && !p->screen.getPrivateMode(1006)) button=3;
  if(p->screen.getPrivateMode(1006)){
    report.format("\x1b[<%d;%d;%d%c",button,screencol+1,screenrow+1,release?'m':'M');
    }
  else{
    report.format("\x1b[M%c%c%c",32+button,32+screencol+1,32+screenrow+1);
    }
  term->send(report);
  }


// Return the modifier parameter for a key sequence
static FXint keyModifier(FXuint state){
  return 1+((state&SHIFTMASK)?1:0)+((state&ALTMASK)?2:0)+((state&CONTROLMASK)?4:0);
  }


// Send a cursor/function key sequence
static FXbool sendKeySequence(FXTerminalPrivate *p,FXTerminal *term,FXEvent *event){
  FXString sequence;
  FXint modifier=keyModifier(event->state);
  FXint code=event->code;
  FXbool appcursor=p->screen.getPrivateMode(1);
  switch(code){
    case KEY_Up:
    case KEY_KP_Up:
      if(modifier==1) sequence=appcursor ? "\x1bOA" : "\x1b[A";
      else sequence.format("\x1b[1;%dA",modifier);
      break;
    case KEY_Down:
    case KEY_KP_Down:
      if(modifier==1) sequence=appcursor ? "\x1bOB" : "\x1b[B";
      else sequence.format("\x1b[1;%dB",modifier);
      break;
    case KEY_Right:
    case KEY_KP_Right:
      if(modifier==1) sequence=appcursor ? "\x1bOC" : "\x1b[C";
      else sequence.format("\x1b[1;%dC",modifier);
      break;
    case KEY_Left:
    case KEY_KP_Left:
      if(modifier==1) sequence=appcursor ? "\x1bOD" : "\x1b[D";
      else sequence.format("\x1b[1;%dD",modifier);
      break;
    case KEY_Home:
    case KEY_KP_Home:
      if(modifier==1) sequence=appcursor ? "\x1bOH" : "\x1b[H";
      else sequence.format("\x1b[1;%dH",modifier);
      break;
    case KEY_End:
    case KEY_KP_End:
      if(modifier==1) sequence=appcursor ? "\x1bOF" : "\x1b[F";
      else sequence.format("\x1b[1;%dF",modifier);
      break;
    case KEY_Insert:
    case KEY_KP_Insert:
      if(modifier==1) sequence="\x1b[2~";
      else sequence.format("\x1b[2;%d~",modifier);
      break;
    case KEY_Delete:
    case KEY_KP_Delete:
      if(modifier==1) sequence="\x1b[3~";
      else sequence.format("\x1b[3;%d~",modifier);
      break;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      if(modifier==1) sequence="\x1b[5~";
      else sequence.format("\x1b[5;%d~",modifier);
      break;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      if(modifier==1) sequence="\x1b[6~";
      else sequence.format("\x1b[6;%d~",modifier);
      break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
      if(modifier==1) sequence.format("\x1bO%c",'P'+(code-KEY_F1));
      else sequence.format("\x1b[1;%d%c",modifier,'P'+(code-KEY_F1));
      break;
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
      {
        static const FXint codevalue[8]={15,17,18,19,20,21,23,24};
        FXint value=codevalue[code-KEY_F5];
        if(modifier==1) sequence.format("\x1b[%d~",value);
        else sequence.format("\x1b[%d;%d~",value,modifier);
      }
      break;
    default:
      return false;
    }
  return term->send(sequence);
  }


// Update the terminal content metrics
static void updateMetrics(FXTerminalPrivate *p){
  FXchar digit;
  FXint advance;
  if(!p->font) return;

  // A cell holds the widest glyph of the grid; a fixed pitch face makes every
  // advance equal, so a run of cells cannot drift away from the column grid
  advance=FXMAX(1,p->font->getTextWidth("M",1));
  advance=FXMAX(advance,p->font->getTextWidth("W",1));
  for(digit='0'; digit<='9'; digit++) advance=FXMAX(advance,p->font->getTextWidth(&digit,1));
  p->cellw=advance;

  // No extra leading: the cell is the ascent plus the descent, so rows pack
  // the way the terminal targets do
  p->ascent=FXMAX(0,p->font->getFontAscent());
  p->cellh=FXMAX(1,p->ascent+FXMAX(0,p->font->getFontDescent()));

  // A run of equal attribute cells is drawn with one text call, which is only
  // exact when ten glyphs advance ten cells
  p->fixedpitch=p->font->isFontMono() && p->font->getTextWidth("MMMMMMMMMM",10)==10*p->cellw;

  // Which glyphs advance one cell depends on the face and the size
  p->oddadvance.clear();
  }


// Padding used when the constructor did not ask for explicit margins
static FXint defaultPadding(FXApp* app){
  return FXMAX(6,app->getTheme()->metric(METRIC_PADDING));
  }


// Return the text of a selected cell range
static FXString selectedText(FXTerminalPrivate *p){
  FXint r1,c1,r2,c2;
  FXString text;
  normalizeSelection(p,r1,c1,r2,c2);
  if(r1<0) return text;
  for(FXint row=r1; row<=r2; row++){
    FXint start=(row==r1)?c1:0;
    FXint end=(row==r2)?c2:p->screen.getColumns()-1;
    for(FXint col=start; col<=end; col++){
      FXString piece=cellTextAt(p,row,col);
      if(!piece.empty()) text+=piece;
      else if(col<p->screen.getColumns()) text+=' ';
      }
    if(row<r2) text+='\n';
    }
  return text;
  }


// Find the cell column for a byte offset in a row
static FXint columnForByte(FXTerminalPrivate *p,FXint row,FXint offset){
  FXint bytes=0;
  FXString part;
  for(FXint col=0; col<p->screen.getColumns(); col++){
    part=cellTextAt(p,row,col);
    if(part.empty()) part.assign(" ",1);
    if(bytes+part.length()>offset) return col;
    bytes+=part.length();
    }
  return FXMAX(0,p->screen.getColumns()-1);
  }


/*******************************************************************************/

// Construct
FXTerminal::FXTerminal():FXScrollArea(),priv(nullptr){
  }


// Construct and initialize
FXTerminal::FXTerminal(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXScrollArea(p,opts,x,y,w,h){
  priv=new FXTerminalPrivate();

  // Explicit margins replace the theme padding; otherwise every side gets the
  // theme padding, which already carries the display scale
  if(pl || pr || pt || pb){
    priv->marginleft=pl;
    priv->marginright=pr;
    priv->margintop=pt;
    priv->marginbottom=pb;
    }
  else{
    priv->padding=defaultPadding(getApp());
    priv->marginleft=priv->marginright=priv->margintop=priv->marginbottom=priv->padding;
    }
  priv->font=getApp()->getRoleFont(FONT_MONO);
  priv->term="xterm-256color";
  updateMetrics(priv);
  priv->notify=tgt;
  priv->notifyselector=sel;
  target=nullptr;
  message=0;
  flags|=FLAG_ENABLED;
  defaultCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  priv->contentwidth=priv->marginleft+priv->marginright+80*priv->cellw;
  priv->contentheight=priv->margintop+priv->marginbottom+24*priv->cellh;
  }


// Destructor
FXTerminal::~FXTerminal(){
  if(priv){
    removeWatches();
    getApp()->removeTimeout(this,ID_BLINK);
    priv->pty.close();
    delete priv;
    priv=nullptr;
    }
  }


// Create native resources
void FXTerminal::create(){
  FXScrollArea::create();
  if(priv && priv->font) priv->font->create();

  // The metrics taken in the constructor came from a font that was not
  // realized yet; take them again from the real face
  if(priv) updateMetrics(priv);
  createComposeContext();
  if(getComposeContext() && priv && priv->font) getComposeContext()->setFont(priv->font);
  recalc();
  }


// Detach native resources
void FXTerminal::detach(){
  getApp()->removeTimeout(this,ID_BLINK);
  destroyComposeContext();
  if(priv && priv->font) priv->font->detach();
  FXScrollArea::detach();
  }


// Can focus
FXbool FXTerminal::canFocus() const {
  return true;
  }


// Default width
FXint FXTerminal::getDefaultWidth(){
  if(priv) return priv->marginleft+priv->marginright+80*FXMAX(1,priv->cellw);
  return FXScrollArea::getDefaultWidth();
  }


// Default height
FXint FXTerminal::getDefaultHeight(){
  if(priv) return priv->margintop+priv->marginbottom+24*FXMAX(1,priv->cellh);
  return FXScrollArea::getDefaultHeight();
  }


// Content width
FXint FXTerminal::getContentWidth(){
  return priv ? priv->contentwidth : FXScrollArea::getContentWidth();
  }


// Content height
FXint FXTerminal::getContentHeight(){
  return priv ? priv->contentheight : FXScrollArea::getContentHeight();
  }


// Move the viewport
void FXTerminal::moveContents(FXint x,FXint y){
  if(!priv){ FXScrollArea::moveContents(x,y); return; }
  pos_x=x;
  pos_y=y;
  priv->follow=(y<=-FXMAX(0,priv->contentheight-getVisibleHeight()));
  update();
  }


// Layout the widget
void FXTerminal::layout(){
  FXint pass;
  FXint columns;
  FXint rows;
  FXint vw;
  FXint vh;
  if(!priv){ FXScrollArea::layout(); return; }
  updateMetrics(priv);
  vw=FXMAX(1,width-priv->marginleft-priv->marginright);
  vh=FXMAX(1,height-priv->margintop-priv->marginbottom);
  for(pass=0; pass<3; pass++){
    columns=FXMAX(1,vw/FXMAX(1,priv->cellw));
    rows=FXMAX(1,vh/FXMAX(1,priv->cellh));
    if(columns!=priv->screen.getColumns() || rows!=priv->screen.getRows()){
      priv->screen.resize(columns,rows);
      if(priv->pty.isRunning()) priv->pty.resize(columns,rows);
      }
    priv->contentwidth=priv->marginleft+priv->marginright+columns*priv->cellw;
    priv->contentheight=priv->margintop+priv->marginbottom+(priv->screen.getScrollbackCount()+rows)*priv->cellh;
    placeScrollBars(width,height);
    vw=FXMAX(1,getVisibleWidth()-priv->marginleft-priv->marginright);
    vh=FXMAX(1,getVisibleHeight()-priv->margintop-priv->marginbottom);
    }
  if(priv->follow){
    pos_x=0;
    pos_y=-FXMAX(0,priv->contentheight-getVisibleHeight());
    }
  }


// Start a shell on a new PTY
FXbool FXTerminal::start(const FXTerminalLaunch& launch){
  FXString error;
  return start(launch,error);
  }


// Start a shell and report an error
FXbool FXTerminal::start(const FXTerminalLaunch& launch,FXString& error){
  FXTerminalLaunch copy=launch;
  FXInputHandle handle;
  if(!priv){
    error="terminal is not initialized";
    return false;
    }
  removeWatches();
  priv->pty.close();
  priv->kind=launch.kind;
  priv->term=launch.term;
  if(priv->term.empty()) priv->term=(launch.kind==TERMINAL_VT100) ? "vt100" : "xterm-256color";
  priv->screen.reset(launch.kind);
  priv->parser.reset();
  if(launch.columns>0 || launch.rows>0){
    priv->screen.resize(launch.columns>0 ? launch.columns : priv->screen.getColumns(),launch.rows>0 ? launch.rows : priv->screen.getRows());
    }
  if(copy.columns<=0) copy.columns=priv->screen.getColumns();
  if(copy.rows<=0) copy.rows=priv->screen.getRows();
  if(copy.term.empty()) copy.term=priv->term;
  if(!priv->pty.start(copy,error)) return false;
  handle=priv->pty.getWakeHandle();
  if(!handle || !getApp()->addInput(this,ID_READ,handle,INPUT_READ)){
    error="could not watch the pseudo terminal";
    priv->pty.close();
    return false;
    }
  priv->readwatch=true;
  priv->writewatch=false;
  priv->exitnotified=false;
  priv->follow=true;
  priv->blinkon=true;
  priv->selstartrow=priv->selstartcol=priv->selendrow=priv->selendcol=-1;
  priv->lasttitle=priv->screen.getTitle();
  priv->lastdirectory=priv->screen.getWorkingDirectory();
  priv->lastbell=priv->screen.getBellCount();
  priv->clipboardrequest.clear();
  recalc();
  update();
  return true;
  }


// Stop watching the PTY
void FXTerminal::removeWatches(){
  FXInputHandle handle;
  if(!priv) return;
  handle=priv->pty.getWakeHandle();
  if(priv->readwatch && handle) getApp()->removeInput(handle,INPUT_READ);
  if(priv->writewatch && handle) getApp()->removeInput(handle,INPUT_WRITE);
  priv->readwatch=false;
  priv->writewatch=false;
  }


// Add a write watch when input is backlogged
void FXTerminal::addWriteWatch(){
  FXInputHandle handle;
  if(!priv || priv->writewatch) return;
  handle=priv->pty.getWakeHandle();
  if(handle && getApp()->addInput(this,ID_WRITE,handle,INPUT_WRITE)) priv->writewatch=true;
  }


// Signal the child
void FXTerminal::terminate(FXbool force){
  if(priv) priv->pty.terminate(force);
  }


// Return true while the child runs
FXbool FXTerminal::isRunning(){
  return priv && priv->pty.isRunning();
  }


// Return the child exit code
FXint FXTerminal::getExitCode(){
  return priv ? priv->pty.getExitCode() : -1;
  }


// Return the process id
FXint FXTerminal::getProcessID() const {
  return priv ? priv->pty.getProcessID() : -1;
  }


// Send raw bytes
FXbool FXTerminal::send(const FXString& bytes){
  FXival count;
  if(!priv || bytes.empty()) return false;
  count=priv->pty.write(bytes.text(),bytes.length());
  if(count<0) return false;
  if(priv->pty.hasPendingWrites()) addWriteWatch();
  return true;
  }


// Send text; keys, pasted text and IME text go through the encoding of the
// session, so a single byte session sends one byte per character and reports
// the characters it cannot express
FXbool FXTerminal::sendText(const FXString& text){
  FXbool lossy=false;
  FXString bytes;
  if(!priv) return false;
  bytes=priv->charset.encode(text,lossy);
  if(lossy && priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_CHANGED,ID_ENCODING_LOSS),nullptr);
  return send(bytes);
  }


// Select the character encoding of this session
FXString FXTerminal::setEncoding(const FXString& name){
  if(!priv) return FXString("UTF-8");
  FXString selected=priv->charset.setName(name);
  // Switching the encoding drops a partial escape or UTF-8 sequence but keeps
  // the screen, so already drawn cells keep their codepoints and the child is
  // not restarted
  priv->parser.setCharset(&priv->charset);
  return selected;
  }


// Return the character encoding of this session
FXString FXTerminal::getEncoding() const {
  return priv ? FXString(priv->charset.getName()) : FXString("UTF-8");
  }


// Number of supported character encodings
FXint FXTerminal::getSupportedEncodingCount(){
  return FXTerminalCharset::count();
  }


// Canonical name of a supported character encoding
FXString FXTerminal::getSupportedEncodingName(FXint index){
  return FXString(FXTerminalCharset::name(index));
  }


// Index of an encoding name or alias
FXint FXTerminal::findEncoding(const FXString& name){
  return FXTerminalCharset::find(name);
  }


// Parse child output
void FXTerminal::feed(const FXString& bytes){
  FXString reply;
  if(!priv) return;
  priv->parser.feed(bytes);
  while(priv->screen.takeResponse(reply)) send(reply);
  updateNotifications();
  recalc();
  update();
  }


// Return the title
FXString FXTerminal::getTitle() const {
  return priv ? priv->screen.getTitle() : FXString();
  }


// Return the working directory
FXString FXTerminal::getWorkingDirectory() const {
  return priv ? priv->screen.getWorkingDirectory() : FXString();
  }


// Return the TERM value
FXString FXTerminal::getTerm() const {
  return priv ? priv->term : FXString();
  }


// Return the terminal kind
FXint FXTerminal::getTerminalKind() const {
  return priv ? priv->kind : TERMINAL_XTERM;
  }


// Return columns
FXint FXTerminal::getColumns() const {
  return priv ? priv->screen.getColumns() : 0;
  }


// Return rows
FXint FXTerminal::getRows() const {
  return priv ? priv->screen.getRows() : 0;
  }


// Return cursor row
FXint FXTerminal::getCursorRow() const {
  return priv ? priv->screen.getCursorRow() : 0;
  }


// Return cursor column
FXint FXTerminal::getCursorColumn() const {
  return priv ? priv->screen.getCursorColumn() : 0;
  }


// Return scrollback capacity
FXint FXTerminal::getScrollbackLines() const {
  return priv ? priv->screen.getScrollbackLines() : 0;
  }


// Return the number of retained scrollback rows
FXint FXTerminal::getScrollbackCount() const {
  return priv ? priv->screen.getScrollbackCount() : 0;
  }


// Change scrollback capacity
void FXTerminal::setScrollbackLines(FXint lines){
  if(priv){
    priv->screen.setScrollbackLines(lines);
    recalc();
    update();
    }
  }


// Change cursor style
void FXTerminal::setCursorStyle(FXint style){
  if(priv){ priv->screen.setCursorStyle(style); update(); }
  }


// Change follow mode
void FXTerminal::setFollowOutput(FXbool on){
  if(!priv) return;
  if(on) scrollToEnd(); else priv->follow=false;
  }


// Return follow mode
FXbool FXTerminal::isFollowOutput() const {
  return priv ? priv->follow : false;
  }


// Scroll to the end
void FXTerminal::scrollToEnd(){
  if(!priv) return;
  priv->follow=true;
  recalc();
  layout();
  update();
  }


// Change font
void FXTerminal::setFont(FXFont* fnt){
  if(!priv || !fnt) return;
  if(priv->font!=fnt){
    priv->font=fnt;
    updateMetrics(priv);
    recalc();
    layout();
    update();
    }
  }


// Return font
FXFont* FXTerminal::getFont() const {
  return priv ? priv->font : nullptr;
  }


// Change the padding on all four sides; the background fills it and the grid
// starts inside it, so the padding never moves a cell
void FXTerminal::setPadding(FXint pad){
  if(!priv) return;
  priv->padding=(0<=pad) ? pad : -1;
  if(pad<0) pad=defaultPadding(getApp());
  if(priv->marginleft!=pad || priv->marginright!=pad || priv->margintop!=pad || priv->marginbottom!=pad){
    priv->marginleft=priv->marginright=priv->margintop=priv->marginbottom=pad;
    recalc();
    layout();
    update();
    }
  }


// Return the padding; -1 when the constructor gave explicit margins
FXint FXTerminal::getPadding() const {
  return priv ? priv->padding : 0;
  }


// Update title/bell/clipboard notifications
void FXTerminal::updateNotifications(){
  FXString request;
  if(!priv) return;
  if(priv->screen.getTitle()!=priv->lasttitle){
    priv->lasttitle=priv->screen.getTitle();
    if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_COMMAND,ID_TITLE),nullptr);
    }
  if(priv->screen.getWorkingDirectory()!=priv->lastdirectory){
    priv->lastdirectory=priv->screen.getWorkingDirectory();
    if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_COMMAND,ID_TITLE),nullptr);
    }
  if(priv->screen.getBellCount()!=priv->lastbell){
    priv->lastbell=priv->screen.getBellCount();
    if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_COMMAND,ID_BELL),nullptr);
    }
  request=priv->screen.takeClipboardRequest();
  if(!request.empty()){
    priv->clipboardrequest=request;
    if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_COMMAND,ID_CLIPBOARD),nullptr);
    }
  }


// Return selected text
FXString FXTerminal::getSelectedText() const {
  return priv ? selectedText(priv) : FXString();
  }


// Select all
void FXTerminal::selectAll(){
  FXTerminalPrivate *p=priv;
  FXint total;
  if(!p) return;
  total=totalRows(p);
  if(total<=0) return;
  p->selstartrow=0;
  p->selstartcol=0;
  p->selendrow=total-1;
  p->selendcol=FXMAX(0,p->screen.getColumns()-1);
  update();
  }


// Copy the selection
FXbool FXTerminal::copySelection(){
  FXString text;
  FXDragType types[4];
  if(!priv) return false;
  text=selectedText(priv);
  if(text.empty()) return false;
  types[0]=stringType;
  types[1]=textType;
  types[2]=utf8Type;
  types[3]=utf16Type;
  if(!acquireClipboard(types,ARRAYNUMBER(types))) return false;
  priv->clipped=text;
  return true;
  }


// Paste clipboard text
FXbool FXTerminal::pasteClipboard(){
  FXString text;
  if(!priv) return false;
  if(getDNDData(FROM_CLIPBOARD,utf8Type,text) || getDNDData(FROM_CLIPBOARD,utf16Type,text) || getDNDData(FROM_CLIPBOARD,stringType,text)){
    if(priv->screen.getPrivateMode(2004)){
      FXString paste="\x1b[200~";
      paste+=text;
      paste+="\x1b[201~";
      return sendText(paste);
      }
    return sendText(text);
    }
  return false;
  }


// Return true when a selection exists, so a menu can enable Copy
FXbool FXTerminal::hasSelection() const {
  return priv && priv->selstartrow>=0 && priv->selendrow>=0;
  }


// Drop the retained scrollback rows and keep the screen; a selection that sat
// in the dropped rows goes with them, one that reached the screen moves up
void FXTerminal::clearScrollback(){
  FXTerminalPrivate *p=priv;
  FXint count;
  if(!p) return;
  count=p->screen.getScrollbackCount();
  if(count<=0) return;
  p->screen.clearScrollback();
  if(p->selendrow>=0 && p->selendrow<count){
    p->selstartrow=p->selstartcol=p->selendrow=p->selendcol=-1;
    p->anchorrow=p->anchorcol=-1;
    }
  else{
    if(p->selstartrow>=0) p->selstartrow=FXMAX(0,p->selstartrow-count);
    if(p->selendrow>=0) p->selendrow=FXMAX(0,p->selendrow-count);
    if(p->anchorrow>=0) p->anchorrow=FXMAX(0,p->anchorrow-count);
    }
  for(FXTerminalFind& match : p->matches) match.row=FXMAX(0,match.row-count);
  if(p->findindex>=(FXint)p->matches.size()) p->findindex=-1;
  p->follow=true;
  recalc();
  layout();
  update();
  }


// Reset the emulator; this is the RIS the child can send as ESC c as well
void FXTerminal::reset(){
  FXTerminalPrivate *p=priv;
  if(!p) return;
  p->screen.reset(p->kind);
  p->parser.reset();
  p->matches.clear();
  p->findindex=-1;
  p->findpattern.clear();
  p->findflags=0;
  p->selstartrow=p->selstartcol=p->selendrow=p->selendcol=-1;
  p->anchorrow=p->anchorcol=-1;
  p->clipboardrequest.clear();
  p->follow=true;
  p->lasttitle=p->screen.getTitle();
  p->lastdirectory=p->screen.getWorkingDirectory();
  p->lastbell=p->screen.getBellCount();
  recalc();
  layout();
  update();
  }


// Take the pending clipboard request
FXString FXTerminal::takeClipboardRequest(){
  FXString request;
  if(!priv) return request;
  request=priv->clipboardrequest;
  priv->clipboardrequest.clear();
  return request;
  }


// Find text in the retained rows
FXint FXTerminal::findText(const FXString& pattern,FXuint searchflags){
  FXString hay;
  FXString needle=pattern;
  FXint row;
  FXint pos;
  FXint col;
  if(!priv) return 0;
  priv->findpattern=pattern;
  priv->findflags=searchflags;
  priv->matches.clear();
  priv->findindex=-1;
  if(pattern.empty()){ update(); return 0; }
  if(searchflags&SEARCH_IGNORECASE) needle.lower();
  for(row=0; row<totalRows(priv); row++){
    hay=rowTextAt(priv,row,false);
    if(searchflags&SEARCH_IGNORECASE) hay.lower();
    pos=0;
    while((pos=hay.find(needle,pos))>=0){
      col=columnForByte(priv,row,pos);
      priv->matches.push_back({row,col,(FXint)needle.length()});
      pos+=FXMAX(1,(FXint)needle.length());
      }
    }
  update();
  return (FXint)priv->matches.size();
  }


// Reveal an absolute row
static void revealRow(FXTerminal *term,FXTerminalPrivate *p,FXint row){
  FXint first=firstVisibleRow(term,p);
  FXint visible=FXMAX(1,(term->getVisibleHeight()-p->margintop-p->marginbottom+p->cellh-1)/FXMAX(1,p->cellh));
  FXint y;
  if(row>=first && row<first+visible) return;
  y=row*p->cellh;
  y=FXCLAMP(0,y,FXMAX(0,p->contentheight-term->getVisibleHeight()));
  term->setPosition(0,-y);
  }


// Select the next match
FXbool FXTerminal::findNext(){
  FXTerminalFind *match;
  if(!priv || priv->matches.empty()) return false;
  priv->findindex=(priv->findindex+1)%(FXint)priv->matches.size();
  match=&priv->matches[priv->findindex];
  setSelection(priv,match->row,match->col,false);
  priv->selendcol=FXMIN(priv->screen.getColumns()-1,match->col+FXMAX(0,match->length-1));
  revealRow(this,priv,match->row);
  update();
  return true;
  }


// Select the previous match
FXbool FXTerminal::findPrev(){
  FXTerminalFind *match;
  if(!priv || priv->matches.empty()) return false;
  priv->findindex=(priv->findindex<=0) ? (FXint)priv->matches.size()-1 : priv->findindex-1;
  match=&priv->matches[priv->findindex];
  setSelection(priv,match->row,match->col,false);
  priv->selendcol=FXMIN(priv->screen.getColumns()-1,match->col+FXMAX(0,match->length-1));
  revealRow(this,priv,match->row);
  update();
  return true;
  }


// Clear find state
void FXTerminal::clearFind(){
  if(priv){
    priv->matches.clear();
    priv->findindex=-1;
    priv->findpattern.clear();
    update();
    }
  }


// Return number of matches
FXint FXTerminal::getFindCount() const {
  return priv ? (FXint)priv->matches.size() : 0;
  }


// Re-apply theme
void FXTerminal::applyTheme(FXTheme* theme){
  FXScrollArea::applyTheme(theme);
  if(!priv) return;
  priv->font=getApp()->retargetRoleFont(priv->font,FONT_MONO,getWindowDPI());
  updateMetrics(priv);
  if(0<=priv->padding){
    priv->padding=defaultPadding(getApp());
    priv->marginleft=priv->marginright=priv->margintop=priv->marginbottom=priv->padding;
    }
  priv->selbackcolor=theme->color(ROLE_SELECTION_BG);
  priv->seltextcolor=theme->color(ROLE_SELECTION_TEXT);
  recalc();
  layout();
  update();
  }


// True if the main font lacks the character of a simple cell, so a fallback
// font draws it; such a glyph may have another advance than the cell
static FXbool usesFallback(FXTerminalPrivate *p,const FXTerminalCell& cell){
  return cell.cluster==0 && cell.ch>=0x80 && !p->font->hasChar(cell.ch);
  }


// True if a glyph the main font has advances exactly one cell.  A face is
// fixed pitch only for the characters it was designed for: U+FFFD advances
// fifteen pixels here against an eight pixel cell, and drawing such a glyph in
// a run of one text call would shift the text after it.
static FXbool advancesOneCell(FXTerminalPrivate *p,const FXTerminalCell& cell){
  if(cell.cluster!=0 || cell.ch<0x80) return true;
  for(size_t i=0; i<p->oddadvance.size(); i++){
    if(p->oddadvance[i]==(FXwchar)cell.ch) return false;
    }
  if(p->font->getCharWidth(cell.ch)!=p->cellw){
    if(p->oddadvance.size()<256) p->oddadvance.push_back(cell.ch);
    return false;
    }
  return true;
  }


// A glyph drawn after its row, so that the background of the next cell does not cover it
struct FXTerminalDeferred {
  FXint     x;
  FXint     width;
  FXColor   color;
  FXString  text;
  };


// Paint the terminal
long FXTerminal::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);
  FXint hist;
  FXint total;
  FXint firstrow;
  FXint visible;
  FXint r;
  FXint c;
  FXint x;
  FXint y;
  FXColor fg;
  FXColor bg;
  FXString text;
  std::vector<FXTerminalDeferred> deferred;
  if(!priv) return 0;
  dc.setFont(priv->font);
  dc.setForeground(priv->defaultbg);
  dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);
  hist=priv->screen.getScrollbackCount();
  total=hist+priv->screen.getRows();
  firstrow=firstVisibleRow(this,priv);
  visible=FXMAX(1,(getVisibleHeight()-priv->margintop-priv->marginbottom+priv->cellh-1)/FXMAX(1,priv->cellh));
  for(r=firstrow; r<total && r<firstrow+visible+1; r++){
    const FXTerminalRow& row=rowAt(priv,r);
    y=getContentY()+priv->margintop+(r-firstrow)*priv->cellh;
    if(y+priv->cellh<event->rect.y || y>event->rect.y+event->rect.h) continue;
    deferred.clear();
    for(c=0; c<priv->screen.getColumns(); ){
      const FXTerminalCell& cell=(c<(FXint)row.cells.size()) ? row.cells[c] : priv->screen.getCell(0,0);
      FXint span=cell.width;
      FXint cells=span;
      FXbool drawable=(cell.attr&TERMATTR_HIDDEN)==0;
      FXbool alone;
      if(span==0){ c++; continue; }
      x=priv->marginleft+c*priv->cellw;
      if(x+span*priv->cellw<event->rect.x){ c+=span; continue; }
      if(x>event->rect.x+event->rect.w) break;
      fg=resolveColor(priv,cell.fg,priv->defaultfg);
      bg=resolveColor(priv,cell.bg,priv->defaultbg);
      if(cell.attr&TERMATTR_REVERSE){ FXColor t=fg; fg=bg; bg=t; }
      if(isSelected(priv,r,c)){ fg=priv->seltextcolor; bg=priv->selbackcolor; }
      text=cellTextAt(priv,r,c);

      // Collect a run of cells with one rendition, so a word is a single text
      // call; only for a fixed pitch face, where the call advances exactly one
      // cell per character, and only over simple one cell glyphs.  A glyph the
      // face cannot draw, or draws with another advance than the cell, gets its
      // own call instead, so the rest of the row keeps its columns
      alone=usesFallback(priv,cell) || !advancesOneCell(priv,cell);
      if(priv->fixedpitch && span==1 && cell.cluster==0 && drawable && !alone){
        FXint c2=c+1;
        while(c2<priv->screen.getColumns()){
          const FXTerminalCell& neighbor=(c2<(FXint)row.cells.size()) ? row.cells[c2] : priv->screen.getCell(0,0);
          FXColor nfg;
          FXColor nbg;
          FXString piece;
          if(neighbor.width!=1 || neighbor.cluster!=0) break;
          if(neighbor.attr!=cell.attr) break;
          if(neighbor.attr&TERMATTR_HIDDEN) break;
          if(usesFallback(priv,neighbor) || !advancesOneCell(priv,neighbor)) break;
          nfg=resolveColor(priv,neighbor.fg,priv->defaultfg);
          nbg=resolveColor(priv,neighbor.bg,priv->defaultbg);
          if(neighbor.attr&TERMATTR_REVERSE){ FXColor t=nfg; nfg=nbg; nbg=t; }
          if(isSelected(priv,r,c2)){ nfg=priv->seltextcolor; nbg=priv->selbackcolor; }
          if(nfg!=fg || nbg!=bg) break;
          piece=cellTextAt(priv,r,c2);
          if(piece.empty()) text+=' ';
          else text+=piece;
          cells++;
          c2++;
          }
        }
      dc.setForeground(bg);
      dc.fillRectangle(x,y,cells*priv->cellw,priv->cellh);
      if(!text.empty() && drawable && alone){
        // Draw later, clipped to its cells; a blank cell after it may be used
        // too, as the icon fonts draw glyphs a little wider than the cell
        FXTerminalDeferred item;
        FXint nextc=c+span;
        FXbool blank=true;
        if(nextc<(FXint)row.cells.size()){
          const FXTerminalCell& next=row.cells[nextc];
          blank=(next.cluster==0 && (next.ch==0 || next.ch==' ') && !isSelected(priv,r,nextc));
          }
        item.x=x;
        item.width=span*priv->cellw+(blank ? priv->cellw : 0);
        item.color=fg;
        item.text=text;
        deferred.push_back(item);
        }
      else if(!text.empty() && drawable){
        dc.setForeground(fg);
        dc.drawText(x,y+priv->ascent,text);
        }
      if(drawable && (cell.attr&TERMATTR_UNDERLINE)){
        dc.setForeground(fg);
        dc.drawLine(x,y+priv->cellh-2,x+cells*priv->cellw-1,y+priv->cellh-2);
        }
      if(drawable && (cell.attr&TERMATTR_OVERLINE)){
        dc.setForeground(fg);
        dc.drawLine(x,y+1,x+cells*priv->cellw-1,y+1);
        }
      if(drawable && (cell.attr&TERMATTR_STRIKE)){
        dc.setForeground(fg);
        dc.drawLine(x,y+priv->cellh/2,x+cells*priv->cellw-1,y+priv->cellh/2);
        }
      c+=cells;
      }
    for(size_t i=0; i<deferred.size(); i++){
      dc.setClipRectangle(deferred[i].x,y,deferred[i].width,priv->cellh);
      dc.setForeground(deferred[i].color);
      dc.drawText(deferred[i].x,y+priv->ascent,deferred[i].text);
      dc.clearClipRectangle();
      }
    }
  if(hasFocus() && priv->screen.getPrivateMode(25)){
    FXint cursor=hist+priv->screen.getCursorRow();
    if(cursor>=firstrow && cursor<firstrow+visible){
      const FXTerminalRow& row=rowAt(priv,cursor);
      c=priv->screen.getCursorColumn();
      x=priv->marginleft+c*priv->cellw;
      y=getContentY()+priv->margintop+(cursor-firstrow)*priv->cellh;
      FXint style=priv->screen.getCursorStyle();
      FXbool blinkstyle=(style==TERMCURSOR_BLOCKBLINK || style==TERMCURSOR_UNDERLINEBLINK || style==TERMCURSOR_BARBLINK);
      if(!blinkstyle || priv->blinkon){
        dc.setForeground(priv->cursorcolor);
        if(style==TERMCURSOR_BAR || style==TERMCURSOR_BARBLINK){
          dc.fillRectangle(x,y,FXMAX(1,priv->cellw/8),priv->cellh);
          }
        else if(style==TERMCURSOR_UNDERLINE || style==TERMCURSOR_UNDERLINEBLINK){
          dc.fillRectangle(x,y+priv->cellh-2,priv->cellw,2);
          }
        else{
          dc.fillRectangle(x,y,priv->cellw,priv->cellh);
          if(c<(FXint)row.cells.size()){
            const FXTerminalCell& cell=row.cells[c];
            text=cellTextAt(priv,cursor,c);
            if(!text.empty()){
              bg=resolveColor(priv,cell.bg,priv->defaultbg);
              dc.setForeground(bg);
              dc.drawText(x,y+priv->ascent,text);
              }
            }
          }
        }
      }
    }
  return 1;
  }


// Handle a key press
long FXTerminal::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXWindow *shell;
  FXString text;
  FXchar control;
  FXint page;
  if(!priv) return 0;

  // Shortcuts of the owning window come first; otherwise Ctrl+Shift+W would
  // reach the child as 0x17 and the tab and quit accelerators never act.
  // A Ctrl+Shift+letter carries the unshifted keysym on Wayland while the
  // menu label parses to the shifted code, so both forms are tried.
  shell=getShell();
  if(shell && shell->getAccelTable()){
    FXEvent hotkey=*event;
    if(shell->getAccelTable()->handle(this,FXSEL(SEL_KEYPRESS,0),&hotkey)) return 1;
    if((hotkey.state&SHIFTMASK) && hotkey.code>='a' && hotkey.code<='z'){
      hotkey.code=hotkey.code-'a'+'A';
      if(shell->getAccelTable()->handle(this,FXSEL(SEL_KEYPRESS,0),&hotkey)) return 1;
      }
    }

  if((event->state&CONTROLMASK) && (event->state&SHIFTMASK)){
    if(event->code==KEY_c){ copySelection(); return 1; }
    if(event->code==KEY_v){ pasteClipboard(); return 1; }
    if(event->code==KEY_a){ selectAll(); return 1; }
    }
  if((event->code==KEY_Page_Up || event->code==KEY_Page_Down) && (event->state&SHIFTMASK)){
    page=FXMAX(1,(getVisibleHeight()-priv->margintop-priv->marginbottom)/FXMAX(1,priv->cellh));
    setPosition(pos_x,pos_y+(event->code==KEY_Page_Up ? page*priv->cellh : -page*priv->cellh));
    priv->follow=false;
    update();
    return 1;
    }
  if(event->state&CONTROLMASK){
    if(event->code>='a' && event->code<='z') control=(FXchar)(event->code-'a'+1);
    else if(event->code>='A' && event->code<='Z') control=(FXchar)(event->code-'A'+1);
    else if(event->code==KEY_space) control=0;
    else if(event->code==KEY_bracketleft) control=0x1B;
    else if(event->code==KEY_backslash) control=0x1C;
    else if(event->code==KEY_bracketright) control=0x1D;
    else control=0;
    if(control){
      return send(FXString(control,1)) ? 1 : 0;
      }
    }
  if(event->code==KEY_Return || event->code==KEY_KP_Enter){
    send(FXString("\r"));
    return 1;
    }
  if(event->code==KEY_Tab){
    return send(FXString((event->state&SHIFTMASK) ? "\x1b[Z" : "\t")) ? 1 : 0;
    }
  if(event->code==KEY_BackSpace){
    return send(FXString("\x7f")) ? 1 : 0;
    }
  if(event->code==KEY_Escape){
    return send(FXString("\x1b")) ? 1 : 0;
    }
  if(sendKeySequence(priv,this,event)) return 1;
  text=event->text;
  if(!text.empty()){
    if((event->state&ALTMASK) && !(event->state&CONTROLMASK)){
      FXString prefix="\x1b";
      text=prefix+text;
      }
    return sendText(text) ? 1 : 0;
    }
  return 0;
  }


// Handle left mouse press
long FXTerminal::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(!priv) return 0;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(priv->screen.getMouseMode() && !(event->state&SHIFTMASK)){
    cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
    sendMouseReport(priv,this,0,row,col,false);
    priv->mousereport=true;
    return 1;
    }
  cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
  grab();
  if(event->click_count>=3){
    priv->mousemode=3;
    setLineSelection(priv,row);
    }
  else if(event->click_count==2){
    priv->mousemode=2;
    setWordSelection(priv,row,col);
    }
  else{
    priv->mousemode=1;
    setSelection(priv,row,col,(event->state&SHIFTMASK)!=0);
    }
  update();
  return 1;
  }


// Handle left mouse release
long FXTerminal::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(!priv) return 0;
  if(priv->mousereport){
    cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
    sendMouseReport(priv,this,0,row,col,true);
    priv->mousereport=false;
    }
  else{
    ungrab();
    }
  priv->mousemode=0;
  return 1;
  }


// Handle right mouse press.  With mouse reporting active the child owns the
// button, and Shift is the escape hatch that still opens the menu; without it
// the press is forwarded to the target as SEL_RIGHTBUTTONPRESS, so the
// application posts the menu (with the event's root position) and the widget
// itself never displays UI.  A selector of the widget's own enum is not used
// here: FXScrollArea::ID_LAST and FXMainWindow::ID_LAST differ, so such an id
// collides with one of the window's own (ID_CONTEXTMENU landed on ID_EXITED).
long FXTerminal::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(!priv) return 0;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
  if(priv->screen.getMouseMode() && !(event->state&SHIFTMASK)){
    sendMouseReport(priv,this,2,row,col,false);
    priv->rightreport=true;
    return 1;
    }
  if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,0),ptr);
  return 1;
  }


// Handle right mouse release
long FXTerminal::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(!priv) return 0;
  if(priv->rightreport){
    cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
    sendMouseReport(priv,this,2,row,col,true);
    priv->rightreport=false;
    }
  return 1;
  }


// Handle mouse motion
long FXTerminal::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(!priv || priv->mousemode==0) return 0;
  cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
  if(priv->screen.getMouseMode() && priv->mousereport){
    sendMouseReport(priv,this,32,row,col,false);
    return 1;
    }
  if(priv->mousemode==1 || priv->mousemode==2) setSelection(priv,row,col,true);
  update();
  return 1;
  }


// Handle mouse wheel
long FXTerminal::onMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint row;
  FXint col;
  if(priv && priv->screen.getMouseMode() && !(event->state&SHIFTMASK)){
    cellAtPoint(this,priv,event->win_x,event->win_y,row,col);
    sendMouseReport(priv,this,event->code>0 ? 64 : 65,row,col,false);
    return 1;
    }
  long result=FXScrollArea::onMouseWheel(sender,sel,ptr);
  if(priv) priv->follow=false;
  return result;
  }


// Scrollbar changed
long FXTerminal::onVScrollerChanged(FXObject* sender,FXSelector sel,void* ptr){
  long result=FXScrollArea::onVScrollerChanged(sender,sel,ptr);
  if(priv) priv->follow=(pos_y<=-FXMAX(0,priv->contentheight-getVisibleHeight()));
  update();
  return result;
  }


// Scrollbar dragged
long FXTerminal::onVScrollerDragged(FXObject* sender,FXSelector sel,void* ptr){
  long result=FXScrollArea::onVScrollerDragged(sender,sel,ptr);
  if(priv) priv->follow=(pos_y<=-FXMAX(0,priv->contentheight-getVisibleHeight()));
  update();
  return result;
  }


// Focus in
long FXTerminal::onFocusIn(FXObject*,FXSelector,void*){
  if(priv){
    priv->blinkon=true;
    getApp()->addTimeout(this,ID_BLINK,getApp()->getBlinkSpeed());
    update();
    }
  if(priv && priv->screen.getPrivateMode(1004)) send(FXString("\x1b[I"));
  if(getComposeContext()) getComposeContext()->focusIn();
  return 1;
  }


// Focus out
long FXTerminal::onFocusOut(FXObject*,FXSelector,void*){
  if(priv){
    getApp()->removeTimeout(this,ID_BLINK);
    priv->blinkon=true;
    update();
    }
  if(priv && priv->screen.getPrivateMode(1004)) send(FXString("\x1b[O"));
  if(getComposeContext()) getComposeContext()->focusOut();
  return 1;
  }


// Cursor blink
long FXTerminal::onBlink(FXObject*,FXSelector,void*){
  if(priv){
    priv->blinkon=!priv->blinkon;
    getApp()->addTimeout(this,ID_BLINK,getApp()->getBlinkSpeed());
    update();
    }
  return 1;
  }


// Position the IME spot
long FXTerminal::onIMEStart(FXObject*,FXSelector,void*){
  FXint hist;
  FXint firstrow;
  FXint visible;
  FXint row;
  FXint x;
  FXint y;
  if(!priv || !getComposeContext()) return 0;
  hist=priv->screen.getScrollbackCount();
  firstrow=firstVisibleRow(this,priv);
  visible=FXMAX(1,(getVisibleHeight()-priv->margintop-priv->marginbottom)/FXMAX(1,priv->cellh));
  row=hist+priv->screen.getCursorRow();
  if(row>=firstrow && row<firstrow+visible){
    x=priv->marginleft+priv->screen.getCursorColumn()*priv->cellw;
    y=getContentY()+priv->margintop+(row-firstrow)*priv->cellh+priv->cellh;
    getComposeContext()->setSpot(x,y);
    }
  return 1;
  }


// Clipboard lost
long FXTerminal::onClipboardLost(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onClipboardLost(sender,sel,ptr);
  if(priv) priv->clipped.clear();
  return 1;
  }


// Selection lost
long FXTerminal::onSelectionLost(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onSelectionLost(sender,sel,ptr);
  if(priv){
    priv->selstartrow=priv->selstartcol=priv->selendrow=priv->selendcol=-1;
    update();
    }
  return 1;
  }


// Clipboard request
long FXTerminal::onClipboardRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(FXScrollArea::onClipboardRequest(sender,sel,ptr)) return 1;
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){
    setDNDData(FROM_CLIPBOARD,event->target,priv ? priv->clipped : FXString());
    return 1;
    }
  return 0;
  }


// Primary selection request
long FXTerminal::onSelectionRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(FXScrollArea::onSelectionRequest(sender,sel,ptr)) return 1;
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){
    setDNDData(FROM_SELECTION,event->target,getSelectedText());
    return 1;
    }
  return 0;
  }


// Read child output
long FXTerminal::onRead(FXObject*,FXSelector,void*){
  FXchar buffer[16384];
  FXival count;
  if(!priv) return 0;
  while((count=priv->pty.read(buffer,sizeof(buffer)))>0){
    FXString reply;
    priv->parser.feed(buffer,(FXint)count);
    while(priv->screen.takeResponse(reply)) send(reply);
    }
  if(count<0){
    FXInputHandle handle=priv->pty.getWakeHandle();
    if(priv->readwatch && handle){ getApp()->removeInput(handle,INPUT_READ); priv->readwatch=false; }
    }
  updateNotifications();
  if(!priv->exitnotified && !priv->pty.isRunning()){
    priv->exitnotified=true;
    if(priv->notify) priv->notify->tryHandle(this,FXSEL(SEL_COMMAND,ID_EXITED),nullptr);
    }
  recalc();
  update();
  return 1;
  }


// Flush queued input
long FXTerminal::onWrite(FXObject*,FXSelector,void*){
  if(!priv) return 0;
  priv->pty.flushWrites();
  if(!priv->pty.hasPendingWrites() && priv->writewatch){
    FXInputHandle handle=priv->pty.getWakeHandle();
    if(handle) getApp()->removeInput(handle,INPUT_WRITE);
    priv->writewatch=false;
    }
  return 1;
  }

}

/*******************************************************************************/
