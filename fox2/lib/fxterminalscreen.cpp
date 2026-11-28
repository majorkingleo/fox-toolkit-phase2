/********************************************************************************
*                                                                               *
*                         T e r m i n a l   S c r e e n                         *
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
#include "FXString.h"
#include "fxunicode.h"
#include "fxterminalwidth.h"
#include "fxterminalscreen.h"

using namespace FX;

/*******************************************************************************/

namespace FX {

// Maximum number of retained grapheme clusters
static const FXint TERMCLUSTERLIMIT=4096;
// Maximum size of a sanitized title or working directory
static const FXint TERMTEXTLIMIT=4096;


// Append one codepoint to a UTF-8 string
static void appendCodepoint(FXString& text,FXwchar ch){
  FXwchar buf[2];
  buf[0]=ch;
  buf[1]=0;
  text.append(buf,1);
  }


// Make a blank row of the given width
static FXTerminalRow makeBlankRow(FXint width){
  FXTerminalCell cell;
  FXTerminalRow row;
  cell.fg=TERMCOLOR_DEFAULT_FG;
  cell.bg=TERMCOLOR_DEFAULT_BG;
  cell.ch=' ';
  cell.attr=0;
  cell.width=1;
  cell.cluster=0;
  row.wrapped=false;
  row.cells.assign(width,cell);
  return row;
  }


/*******************************************************************************/

// Construct the screen
FXTerminalScreen::FXTerminalScreen():kind(TERMINAL_XTERM),columns(80),rows(24),scrollcap(10000),cursorrow(0),cursorcol(0),saverow(0),savecol(0),saveattr(0),savefg(TERMCOLOR_DEFAULT_FG),savebg(TERMCOLOR_DEFAULT_BG),savewrap(false),top(0),bottom(23),cursorstyle(TERMCURSOR_DEFAULT),curattr(0),curfg(TERMCOLOR_DEFAULT_FG),curbg(TERMCOLOR_DEFAULT_BG),modes(TERMMODE_WRAP|TERMMODE_CURSORVISIBLE|TERMMODE_CURSORBLINK),wrapPending(false),altActive(false),hasSaved(false),lastValid(false),lastRow(0),lastCol(0),bellCount(0){
  reset(kind);
  }


// Destructor
FXTerminalScreen::~FXTerminalScreen(){
  }


// Reset all screen state while keeping the current size and scrollback capacity
void FXTerminalScreen::resetState(FXint terminalkind){
  FXint i;
  FXint r;
  FXint g;
  FXint b;
  FXint level[6]={0,95,135,175,215,255};
  FXColor basic[16]={
    FXRGB(12,12,12),FXRGB(197,15,31),FXRGB(19,161,14),FXRGB(193,156,0),
    FXRGB(0,55,218),FXRGB(136,23,152),FXRGB(58,150,221),FXRGB(204,204,204),
    FXRGB(118,118,118),FXRGB(231,72,86),FXRGB(22,198,12),FXRGB(249,241,165),
    FXRGB(59,120,255),FXRGB(180,0,158),FXRGB(97,214,214),FXRGB(242,242,242)
    };
  this->kind=terminalkind;
  if(columns<1) columns=80;
  if(rows<1) rows=24;
  if(scrollcap<0) scrollcap=0;
  lines.clear();
  scrollback.clear();
  primaryLines.clear();
  clusters.clear();
  responses.clear();
  title.clear();
  workingdir.clear();
  hyperlink.clear();
  clipboard.clear();
  curattr=0;
  curfg=TERMCOLOR_DEFAULT_FG;
  curbg=TERMCOLOR_DEFAULT_BG;
  cursorrow=0;
  cursorcol=0;
  saverow=0;
  savecol=0;
  saveattr=0;
  savefg=TERMCOLOR_DEFAULT_FG;
  savebg=TERMCOLOR_DEFAULT_BG;
  savewrap=false;
  top=0;
  bottom=rows-1;
  cursorstyle=TERMCURSOR_DEFAULT;
  modes=TERMMODE_WRAP|TERMMODE_CURSORVISIBLE|TERMMODE_CURSORBLINK;
  wrapPending=false;
  altActive=false;
  hasSaved=false;
  lastValid=false;
  lastRow=0;
  lastCol=0;
  bellCount=0;
  palette.assign(256,0);
  for(i=0; i<16; i++) palette[i]=basic[i];
  for(i=16; i<232; i++){
    r=(i-16)/36;
    g=((i-16)%36)/6;
    b=(i-16)%6;
    palette[i]=FXRGB(level[r],level[g],level[b]);
    }
  for(i=232; i<256; i++){
    r=8+(i-232)*10;
    palette[i]=FXRGB(r,r,r);
    }
  tabstops.assign(columns,0);
  resetTabStops();
  lines.assign(rows,makeBlankRow(columns));
  }


// Reset the screen
void FXTerminalScreen::reset(FXint terminalkind){
  resetState(terminalkind);
  }


// Change the terminal kind without resetting the screen
void FXTerminalScreen::setTerminalKind(FXint terminalkind){
  this->kind=terminalkind;
  }


// Return a blank row for the current width
FXTerminalRow FXTerminalScreen::blankRow() const {
  return makeBlankRow(columns);
  }


// Return a blank cell
FXTerminalCell FXTerminalScreen::blankCell(FXbool erase) const {
  FXTerminalCell cell;
  cell.fg=TERMCOLOR_DEFAULT_FG;
  cell.bg=erase ? curbg : (FXuint)TERMCOLOR_DEFAULT_BG;
  cell.ch=' ';
  cell.attr=0;
  cell.width=1;
  cell.cluster=0;
  return cell;
  }


// Make one cell
FXTerminalCell FXTerminalScreen::makeCell(FXwchar ch,FXuint fg,FXuint bg,FXushort attr,FXushort width) const {
  FXTerminalCell cell;
  cell.fg=fg;
  cell.bg=bg;
  cell.ch=ch;
  cell.attr=attr;
  cell.width=width;
  cell.cluster=0;
  return cell;
  }


// Reduce a color to the profile's supported palette
FXuint FXTerminalScreen::normalizeColor(FXuint color) const {
  FXuint rr;
  FXuint gg;
  FXuint bb;
  FXuint index;
  if(kind!=TERMINAL_VT100 || color==TERMCOLOR_DEFAULT_FG || color==TERMCOLOR_DEFAULT_BG) return color;
  if(color&TERMCOLOR_RGB){
    rr=(color>>16)&0xFF;
    gg=(color>>8)&0xFF;
    bb=color&0xFF;
    return (rr>=128 ? 1 : 0)|(gg>=128 ? 2 : 0)|(bb>=128 ? 4 : 0);
    }
  index=color&0xFF;
  if(index<16) return index;
  return index&7;
  }


// Return the last non-blank cell end in a row
FXint FXTerminalScreen::contentLength(const FXTerminalRow& row) const {
  FXint i;
  for(i=(FXint)row.cells.size()-1; i>=0; i--){
    if(row.cells[i].cluster || row.cells[i].ch!=' '){
      return FXMIN((FXint)row.cells.size(),i+(row.cells[i].width ? row.cells[i].width : 1));
      }
    }
  return 0;
  }


// Return the index of a grapheme cluster, creating it if needed
FXint FXTerminalScreen::clusterIndex(const FXString& cluster){
  FXint i;
  for(i=0; i<(FXint)clusters.size(); i++){
    if(clusters[i]==cluster) return i+1;
    }
  if((FXint)clusters.size()>=TERMCLUSTERLIMIT) return 0;
  clusters.push_back(cluster);
  return (FXint)clusters.size();
  }


// Sanitize a text and bound its length
FXString FXTerminalScreen::sanitize(const FXString& text,FXint maxlength) const {
  FXString result;
  FXint i;
  for(i=0; i<text.length() && i<maxlength; i++){
    FXchar c=text[i];
    if(c=='\n' || c=='\r' || c=='\t' || (FXuchar)c<0x20 || c==0x7F) result+=' ';
    else result+=c;
    }
  return result;
  }


// Reset tab stops to every eighth column
void FXTerminalScreen::resetTabStops(){
  FXint i;
  tabstops.assign(columns,0);
  for(i=8; i<columns; i+=8) tabstops[i]=1;
  }


// Clamp the cursor to the screen
void FXTerminalScreen::clampCursor(){
  cursorrow=FXCLAMP(0,cursorrow,rows-1);
  cursorcol=FXCLAMP(0,cursorcol,columns-1);
  }


// Push one row into scrollback
void FXTerminalScreen::dumpRowToHistory(const FXTerminalRow& row){
  if(scrollcap<=0) return;
  scrollback.push_back(row);
  while((FXint)scrollback.size()>scrollcap) scrollback.pop_front();
  }


// Reflow rows to a new width, returning a cursor position in the new grid
void FXTerminalScreen::reflowRows(const std::vector<FXTerminalRow>& source,std::vector<FXTerminalRow>& target,FXint width,FXint& cursorline,FXint& cursorcolumn){
  std::vector<FXTerminalCell> logical;
  FXint r;
  FXint i;
  FXint count;
  FXint flat=0;
  FXint cursorflat=-1;
  FXint offset;
  FXTerminalRow row;
  target.clear();
  for(r=0; r<(FXint)source.size(); r++){
    count=contentLength(source[r]);
    if(r==cursorline) cursorflat=flat+FXMIN(cursorcolumn,count);
    for(i=0; i<count; i++) logical.push_back(source[r].cells[i]);
    flat+=count;
    if(!source[r].wrapped){
      if(logical.empty()){
        target.push_back(makeBlankRow(width));
        }
      else{
        for(offset=0; offset<(FXint)logical.size(); offset+=width){
          row=makeBlankRow(width);
          row.wrapped=(offset+width)<(FXint)logical.size();
          for(i=0; i<width && offset+i<(FXint)logical.size(); i++) row.cells[i]=logical[offset+i];
          target.push_back(row);
          }
        }
      logical.clear();
      }
    }
  if(!logical.empty()){
    for(offset=0; offset<(FXint)logical.size(); offset+=width){
      row=makeBlankRow(width);
      row.wrapped=(offset+width)<(FXint)logical.size();
      for(i=0; i<width && offset+i<(FXint)logical.size(); i++) row.cells[i]=logical[offset+i];
      target.push_back(row);
      }
    }
  if(target.empty()) target.push_back(makeBlankRow(width));
  if(cursorflat>=0){
    cursorline=FXCLAMP(0,cursorflat/width,(FXint)target.size()-1);
    cursorcolumn=FXCLAMP(0,cursorflat%width,width-1);
    }
  else{
    cursorline=FXCLAMP(0,cursorline,(FXint)target.size()-1);
    cursorcolumn=FXCLAMP(0,cursorcolumn,width-1);
    }
  }


// Resize the screen and reflow soft-wrapped primary rows
void FXTerminalScreen::resize(FXint newcolumns,FXint newrows){
  std::vector<FXTerminalRow> target;
  std::vector<FXTerminalRow> primary;
  FXint newcursorline;
  FXint newcursorcol;
  FXint oldcolumns=columns;
  FXint i;
  if(newcolumns<1) newcolumns=1;
  if(newrows<1) newrows=1;
  if(newcolumns==columns && newrows==rows) return;
  newcursorline=cursorrow;
  newcursorcol=cursorcol;
  if(newcolumns!=columns){
    reflowRows(lines,target,newcolumns,newcursorline,newcursorcol);
    lines.swap(target);
    if(altActive && !primaryLines.empty()){
      FXint primaryline=0;
      FXint primarycol=0;
      reflowRows(primaryLines,primary,newcolumns,primaryline,primarycol);
      primaryLines.swap(primary);
      }
    }
  columns=newcolumns;
  rows=newrows;
  while((FXint)lines.size()>rows && contentLength(lines.back())==0){
    lines.pop_back();
    }
  if((FXint)lines.size()<rows){
    lines.resize(rows,makeBlankRow(columns));
    }
  while((FXint)lines.size()>rows){
    if(!altActive && contentLength(lines.front())>0){
      dumpRowToHistory(lines.front());
      }
    lines.erase(lines.begin());
    if(0<newcursorline) newcursorline--;
    }
  if(altActive && (FXint)primaryLines.size()!=rows){
    primaryLines.resize(rows,makeBlankRow(columns));
    while((FXint)primaryLines.size()>rows) primaryLines.erase(primaryLines.begin());
    }
  top=0;
  bottom=rows-1;
  cursorrow=FXCLAMP(0,newcursorline,rows-1);
  cursorcol=FXCLAMP(0,newcursorcol,columns-1);
  wrapPending=false;
  tabstops.resize(columns,0);
  for(i=0; i<columns; i++) if(i && (i%8)==0 && i>=oldcolumns) tabstops[i]=1;
  }


// Return a row as text
FXString FXTerminalScreen::getRowText(FXint row,FXbool trimmed) const {
  FXString text;
  FXint i;
  FXint n;
  if(row<0 || row>=(FXint)lines.size()) return text;
  n=(FXint)lines[row].cells.size();
  for(i=0; i<n; i++){
    const FXTerminalCell& cell=lines[row].cells[i];
    if(cell.width==0) continue;
    if(cell.cluster && cell.cluster<=(FXuint)clusters.size()) text+=clusters[cell.cluster-1];
    else if(cell.ch) appendCodepoint(text,cell.ch);
    else text+=' ';
    }
  if(trimmed){
    text.trimEnd();
    }
  return text;
  }


// Return a scrollback row as text
FXString FXTerminalScreen::getScrollbackText(FXint row,FXbool trimmed) const {
  FXString text;
  FXint i;
  FXint n;
  if(row<0 || row>=(FXint)scrollback.size()) return text;
  n=(FXint)scrollback[row].cells.size();
  for(i=0; i<n; i++){
    const FXTerminalCell& cell=scrollback[row].cells[i];
    if(cell.width==0) continue;
    if(cell.cluster && cell.cluster<=(FXuint)clusters.size()) text+=clusters[cell.cluster-1];
    else if(cell.ch) appendCodepoint(text,cell.ch);
    else text+=' ';
    }
  if(trimmed){
    text.trimEnd();
    }
  return text;
  }


// Return one cell
const FXTerminalCell& FXTerminalScreen::getCell(FXint row,FXint col) const {
  static FXTerminalCell empty={TERMCOLOR_DEFAULT_FG,TERMCOLOR_DEFAULT_BG,' ',0,1,0};
  if(row<0 || row>=(FXint)lines.size() || col<0 || col>=(FXint)lines[row].cells.size()) return empty;
  return lines[row].cells[col];
  }


// Return a screen row
const FXTerminalRow& FXTerminalScreen::getRow(FXint row) const {
  static const FXTerminalRow empty;
  if(row<0 || row>=(FXint)lines.size()) return empty;
  return lines[row];
  }


// Return a scrollback row
const FXTerminalRow& FXTerminalScreen::getScrollbackRow(FXint row) const {
  static const FXTerminalRow empty;
  if(row<0 || row>=(FXint)scrollback.size()) return empty;
  return scrollback[row];
  }


// Return one screen cell text
FXString FXTerminalScreen::getCellText(FXint row,FXint col) const {
  FXString text;
  FXwchar ch[2];
  const FXTerminalCell& cell=getCell(row,col);
  if(cell.cluster && cell.cluster<=(FXuint)clusters.size()) return clusters[cell.cluster-1];
  if(cell.ch){
    ch[0]=cell.ch;
    ch[1]=0;
    text.assign(ch,1);
    }
  return text;
  }


// Return one scrollback cell text
FXString FXTerminalScreen::getScrollbackCellText(FXint row,FXint col) const {
  FXString text;
  FXwchar ch[2];
  const FXTerminalRow& line=getScrollbackRow(row);
  if(col<0 || col>=(FXint)line.cells.size()) return text;
  const FXTerminalCell& cell=line.cells[col];
  if(cell.cluster && cell.cluster<=(FXuint)clusters.size()) return clusters[cell.cluster-1];
  if(cell.ch){
    ch[0]=cell.ch;
    ch[1]=0;
    text.assign(ch,1);
    }
  return text;
  }


// Return true if a cell is a wide continuation
FXbool FXTerminalScreen::isContinuation(FXint row,FXint col) const {
  return row>=0 && row<(FXint)lines.size() && col>=0 && col<(FXint)lines[row].cells.size() && lines[row].cells[col].width==0;
  }


// Set the cursor style
void FXTerminalScreen::setCursorStyle(FXint style){
  cursorstyle=FXCLAMP(TERMCURSOR_DEFAULT,style,TERMCURSOR_BARBLINK);
  }


// Set the current foreground color
void FXTerminalScreen::setForeground(FXuint color){
  curfg=normalizeColor(color);
  }


// Set the current background color
void FXTerminalScreen::setBackground(FXuint color){
  curbg=normalizeColor(color);
  }


// Set the current attributes
void FXTerminalScreen::setAttributes(FXushort attr){
  curattr=attr;
  }


// Turn one attribute on or off
void FXTerminalScreen::setAttribute(FXushort attr,FXbool on){
  if(on) curattr|=attr;
  else curattr&=~attr;
  }


// Reset the rendition
void FXTerminalScreen::resetAttributes(){
  curattr=0;
  curfg=TERMCOLOR_DEFAULT_FG;
  curbg=TERMCOLOR_DEFAULT_BG;
  }


// Queue a response for the child
void FXTerminalScreen::addResponse(const FXString& reply){
  if(responses.size()<256) responses.push_back(reply);
  }


// Take one response
FXbool FXTerminalScreen::takeResponse(FXString& reply){
  if(responses.empty()) return false;
  reply=responses.front();
  responses.erase(responses.begin());
  return true;
  }


// Print one codepoint
void FXTerminalScreen::putChar(FXwchar ch){
  putGlyph(ch,fxterminalCharWidth(ch));
  }


// Print one character of a single byte encoding
void FXTerminalScreen::putCell(FXwchar ch){
  putGlyph(ch,1);
  }


// Print one codepoint; width zero combines with the previous cell, one fills a
// cell and two fills a cell and its continuation
void FXTerminalScreen::putGlyph(FXwchar ch,FXint width){
  FXint idx;
  FXTerminalCell cell;
  FXString cluster;
  FXString mark;
  FXwchar pair[2];
  if(ch==0) return;
  if(ch<0x20 || (0x7F<=ch && ch<0xA0)){
    execute(ch);
    return;
    }
  if(width==2 && columns<2) width=1;
  if(width==0){
    if(lastValid && lastRow>=0 && lastRow<(FXint)lines.size() && lastCol>=0 && lastCol<(FXint)lines[lastRow].cells.size()){
      cell=lines[lastRow].cells[lastCol];
      while(cell.width==0 && 0<lastCol) cell=lines[lastRow].cells[--lastCol];
      if(cell.cluster && cell.cluster<=(FXuint)clusters.size()) cluster=clusters[cell.cluster-1];
      else{
        pair[0]=cell.ch;
        pair[1]=0;
        cluster.assign(pair,1);
        }
      pair[0]=ch;
      pair[1]=0;
      mark.assign(pair,1);
      cluster+=mark;
      idx=clusterIndex(cluster);
      if(idx>0) lines[lastRow].cells[lastCol].cluster=idx;
      }
    return;
    }
  if(wrapPending){
    if(getMode(TERMMODE_WRAP)){
      lineFeed();
      cursorcol=0;
      }
    else{
      wrapPending=false;
      }
    }
  if(width==2 && cursorcol>=columns-1){
    if(getMode(TERMMODE_WRAP)){
      lineFeed();
      cursorcol=0;
      }
    else{
      cursorcol=columns-2;
      }
    }
  if(getMode(TERMMODE_INSERT)) insertCharacters(width);
  cell=makeCell(ch,curfg,curbg,curattr,width);
  lines[cursorrow].cells[cursorcol]=cell;
  if(width==2 && cursorcol+1<columns){
    cell.ch=0;
    cell.width=0;
    cell.cluster=0;
    lines[cursorrow].cells[cursorcol+1]=cell;
    }
  lastValid=true;
  lastRow=cursorrow;
  lastCol=cursorcol;
  cursorcol+=width;
  if(cursorcol>=columns){
    cursorcol=columns-1;
    wrapPending=true;
    }
  else{
    wrapPending=false;
    }
  }


// Execute a C0 control
void FXTerminalScreen::execute(FXuint ch){
  switch(ch){
    case 0x07:
      bellCount++;
      break;
    case 0x08:
      backspace();
      break;
    case 0x09:
      tab();
      break;
    case 0x0A:
    case 0x0B:
    case 0x0C:
      lineFeed();
      break;
    case 0x0D:
      carriageReturn();
      break;
    default:
      break;
    }
  }


// Move to the next line
void FXTerminalScreen::lineFeed(){
  if(getMode(TERMMODE_NEWLINE)) cursorcol=0;
  if(cursorrow==bottom){
    scrollRegionUp(top,bottom,1,true);
    }
  else if(cursorrow<rows-1){
    cursorrow++;
    }
  wrapPending=false;
  lastValid=false;
  }


// Move to column zero
void FXTerminalScreen::carriageReturn(){
  cursorcol=0;
  wrapPending=false;
  }


// Move one column left
void FXTerminalScreen::backspace(){
  if(wrapPending){
    wrapPending=false;
    }
  else if(0<cursorcol){
    cursorcol--;
    }
  lastValid=false;
  }


// Move to the next tab stop
void FXTerminalScreen::tab(){
  FXint i=cursorcol+1;
  while(i<columns && !tabstops[i]) i++;
  cursorcol=(i<columns) ? i : columns-1;
  wrapPending=false;
  lastValid=false;
  }


// Move to the previous tab stop
void FXTerminalScreen::backTab(){
  FXint i=cursorcol-1;
  while(0<i && !tabstops[i]) i--;
  cursorcol=FXMAX(0,i);
  wrapPending=false;
  lastValid=false;
  }


// Set a tab stop at the cursor
void FXTerminalScreen::setTabStop(){
  if(0<=cursorcol && cursorcol<columns) tabstops[cursorcol]=1;
  }


// Clear a tab stop, or all tab stops
void FXTerminalScreen::clearTabStops(FXint mode){
  if(mode==3){
    tabstops.assign(columns,0);
    }
  else if(0<=cursorcol && cursorcol<columns){
    tabstops[cursorcol]=0;
    }
  }


// Soft reset without clearing the screen
void FXTerminalScreen::softReset(){
  if(altActive) leaveAltScreen();
  resetAttributes();
  modes=TERMMODE_WRAP|TERMMODE_CURSORVISIBLE|TERMMODE_CURSORBLINK;
  cursorstyle=TERMCURSOR_DEFAULT;
  top=0;
  bottom=rows-1;
  wrapPending=false;
  lastValid=false;
  }


// Move the cursor by a relative amount
void FXTerminalScreen::moveCursor(FXint drow,FXint dcol){
  if(getMode(TERMMODE_ORIGIN)) cursorrow=FXCLAMP(top,cursorrow+drow,bottom);
  else cursorrow=FXCLAMP(0,cursorrow+drow,rows-1);
  cursorcol=FXCLAMP(0,cursorcol+dcol,columns-1);
  wrapPending=false;
  lastValid=false;
  }


// Set the cursor position
void FXTerminalScreen::setCursor(FXint row,FXint col){
  if(getMode(TERMMODE_ORIGIN)){
    row=top+row;
    }
  cursorrow=FXCLAMP(0,row,rows-1);
  cursorcol=FXCLAMP(0,col,columns-1);
  wrapPending=false;
  lastValid=false;
  }


// Set the absolute cursor row
void FXTerminalScreen::setCursorRow(FXint row){
  cursorrow=FXCLAMP(0,row,rows-1);
  wrapPending=false;
  lastValid=false;
  }


// Set the absolute cursor column
void FXTerminalScreen::setCursorColumn(FXint col){
  cursorcol=FXCLAMP(0,col,columns-1);
  wrapPending=false;
  lastValid=false;
  }


// Set the scrolling margins
void FXTerminalScreen::setMargins(FXint newtop,FXint newbottom){
  if(newtop<0 || newbottom>=rows || newtop>=newbottom){
    newtop=0;
    newbottom=rows-1;
    }
  top=newtop;
  bottom=newbottom;
  cursorrow=top;
  cursorcol=0;
  wrapPending=false;
  lastValid=false;
  }


// Save the cursor and rendition
void FXTerminalScreen::saveCursor(){
  saverow=cursorrow;
  savecol=cursorcol;
  saveattr=curattr;
  savefg=curfg;
  savebg=curbg;
  savewrap=wrapPending;
  hasSaved=true;
  }


// Restore the cursor and rendition
void FXTerminalScreen::restoreCursor(){
  if(!hasSaved) return;
  cursorrow=FXCLAMP(0,saverow,rows-1);
  cursorcol=FXCLAMP(0,savecol,columns-1);
  curattr=saveattr;
  curfg=savefg;
  curbg=savebg;
  wrapPending=savewrap;
  lastValid=false;
  }


// Index: move down and scroll when at the bottom margin
void FXTerminalScreen::index(){
  if(cursorrow==bottom) scrollRegionUp(top,bottom,1,true);
  else if(cursorrow<rows-1) cursorrow++;
  wrapPending=false;
  lastValid=false;
  }


// Reverse index: move up and scroll when at the top margin
void FXTerminalScreen::reverseIndex(){
  if(cursorrow==top) scrollRegionDown(top,bottom,1);
  else if(0<cursorrow) cursorrow--;
  wrapPending=false;
  lastValid=false;
  }


// Scroll the current region up
void FXTerminalScreen::scrollUp(FXint count){
  scrollRegionUp(top,bottom,count,true);
  }


// Scroll the current region down
void FXTerminalScreen::scrollDown(FXint count){
  scrollRegionDown(top,bottom,count);
  }


// Scroll a region up, optionally retaining the removed rows
void FXTerminalScreen::scrollRegionUp(FXint first,FXint last,FXint count,FXbool history){
  FXint y;
  if(count<=0 || first<0 || last>=rows || last<first) return;
  count=FXMIN(count,last-first+1);
  if(history && !altActive && first==0 && last==rows-1){
    for(y=0; y<count; y++) dumpRowToHistory(lines[first+y]);
    }
  for(y=first; y<=last-count; y++) lines[y]=lines[y+count];
  for(y=last-count+1; y<=last; y++) lines[y]=blankRow();
  }


// Scroll a region down
void FXTerminalScreen::scrollRegionDown(FXint first,FXint last,FXint count){
  FXint y;
  if(count<=0 || first<0 || last>=rows || last<first) return;
  count=FXMIN(count,last-first+1);
  for(y=last; y>=first+count; y--) lines[y]=lines[y-count];
  for(y=first; y<first+count; y++) lines[y]=blankRow();
  }


// Erase the display
void FXTerminalScreen::eraseInDisplay(FXint mode){
  FXint i;
  FXint j;
  FXTerminalCell blank=blankCell(true);
  if(mode==3){
    scrollback.clear();
    return;
    }
  if(mode==0){
    for(j=cursorcol; j<columns; j++) lines[cursorrow].cells[j]=blank;
    for(i=cursorrow+1; i<rows; i++){
      lines[i].cells.assign(columns,blank);
      lines[i].wrapped=false;
      }
    }
  else if(mode==1){
    for(i=0; i<cursorrow; i++){
      lines[i].cells.assign(columns,blank);
      lines[i].wrapped=false;
      }
    for(j=0; j<=cursorcol && j<columns; j++) lines[cursorrow].cells[j]=blank;
    }
  else if(mode==2){
    for(i=0; i<rows; i++){
      lines[i].cells.assign(columns,blank);
      lines[i].wrapped=false;
      }
    }
  lastValid=false;
  }


// Erase the line
void FXTerminalScreen::eraseInLine(FXint mode){
  FXint j;
  FXTerminalCell blank=blankCell(true);
  if(mode==0){
    for(j=cursorcol; j<columns; j++) lines[cursorrow].cells[j]=blank;
    }
  else if(mode==1){
    for(j=0; j<=cursorcol && j<columns; j++) lines[cursorrow].cells[j]=blank;
    }
  else if(mode==2){
    for(j=0; j<columns; j++) lines[cursorrow].cells[j]=blank;
    }
  lastValid=false;
  }


// Erase characters
void FXTerminalScreen::eraseCharacters(FXint count){
  FXTerminalCell blank=blankCell(true);
  FXint j;
  count=FXCLAMP(0,count,columns-cursorcol);
  for(j=0; j<count; j++) lines[cursorrow].cells[cursorcol+j]=blank;
  lastValid=false;
  }


// Insert blank characters
void FXTerminalScreen::insertCharacters(FXint count){
  FXTerminalCell blank=blankCell(true);
  FXint j;
  count=FXCLAMP(0,count,columns-cursorcol);
  if(count<=0) return;
  for(j=columns-1; j>=cursorcol+count; j--) lines[cursorrow].cells[j]=lines[cursorrow].cells[j-count];
  for(j=0; j<count; j++) lines[cursorrow].cells[cursorcol+j]=blank;
  lastValid=false;
  }


// Delete characters
void FXTerminalScreen::deleteCharacters(FXint count){
  FXTerminalCell blank=blankCell(true);
  FXint j;
  count=FXCLAMP(0,count,columns-cursorcol);
  if(count<=0) return;
  for(j=cursorcol; j<columns-count; j++) lines[cursorrow].cells[j]=lines[cursorrow].cells[j+count];
  for(j=columns-count; j<columns; j++) lines[cursorrow].cells[j]=blank;
  lastValid=false;
  }


// Insert blank lines
void FXTerminalScreen::insertLines(FXint count){
  FXint y;
  if(cursorrow<top || cursorrow>bottom) return;
  count=FXCLAMP(0,count,bottom-cursorrow+1);
  if(count<=0) return;
  for(y=bottom; y>=cursorrow+count; y--) lines[y]=lines[y-count];
  for(y=cursorrow; y<cursorrow+count; y++) lines[y]=blankRow();
  lastValid=false;
  }


// Delete lines
void FXTerminalScreen::deleteLines(FXint count){
  FXint y;
  if(cursorrow<top || cursorrow>bottom) return;
  count=FXCLAMP(0,count,bottom-cursorrow+1);
  if(count<=0) return;
  for(y=cursorrow; y<=bottom-count; y++) lines[y]=lines[y+count];
  for(y=bottom-count+1; y<=bottom; y++) lines[y]=blankRow();
  lastValid=false;
  }


// Enter the alternate screen
void FXTerminalScreen::enterAltScreen(FXbool save){
  if(altActive) return;
  if(save) saveCursor(); else hasSaved=false;
  primaryLines=lines;
  lines.assign(rows,blankRow());
  cursorrow=0;
  cursorcol=0;
  top=0;
  bottom=rows-1;
  wrapPending=false;
  altActive=true;
  modes|=TERMMODE_ALTSCREEN;
  lastValid=false;
  }


// Leave the alternate screen
void FXTerminalScreen::leaveAltScreen(){
  if(!altActive) return;
  lines=primaryLines;
  primaryLines.clear();
  altActive=false;
  modes&=~TERMMODE_ALTSCREEN;
  if(hasSaved) restoreCursor();
  top=0;
  bottom=rows-1;
  wrapPending=false;
  lastValid=false;
  }


// Set or reset a DEC private mode
FXbool FXTerminalScreen::setPrivateMode(FXint mode,FXbool on){
  if(mode==1){
    if(on) modes|=TERMMODE_CURSORKEYS; else modes&=~TERMMODE_CURSORKEYS;
    return true;
    }
  if(mode==6){
    if(on) modes|=TERMMODE_ORIGIN; else modes&=~TERMMODE_ORIGIN;
    setCursor(0,0);
    return true;
    }
  if(mode==7){
    if(on) modes|=TERMMODE_WRAP; else modes&=~TERMMODE_WRAP;
    return true;
    }
  if(mode==12){
    if(on) modes|=TERMMODE_CURSORBLINK; else modes&=~TERMMODE_CURSORBLINK;
    return true;
    }
  if(mode==25){
    if(on) modes|=TERMMODE_CURSORVISIBLE; else modes&=~TERMMODE_CURSORVISIBLE;
    return true;
    }
  if(kind==TERMINAL_VT100) return false;
  if(mode==47 || mode==1047 || mode==1049){
    if(on) enterAltScreen(mode==1049);
    else leaveAltScreen();
    return true;
    }
  if(mode==1048){
    if(on) saveCursor(); else restoreCursor();
    return true;
    }
  if(mode==1000){
    if(on){ modes&=~(TERMMODE_MOUSEBUTTON|TERMMODE_MOUSEANY); modes|=TERMMODE_MOUSEX10; }
    else modes&=~TERMMODE_MOUSEX10;
    return true;
    }
  if(mode==1002){
    if(on){ modes&=~(TERMMODE_MOUSEX10|TERMMODE_MOUSEANY); modes|=TERMMODE_MOUSEBUTTON; }
    else modes&=~TERMMODE_MOUSEBUTTON;
    return true;
    }
  if(mode==1003){
    if(on){ modes&=~(TERMMODE_MOUSEX10|TERMMODE_MOUSEBUTTON); modes|=TERMMODE_MOUSEANY; }
    else modes&=~TERMMODE_MOUSEANY;
    return true;
    }
  if(mode==1006){
    if(on) modes|=TERMMODE_MOUSESGR; else modes&=~TERMMODE_MOUSESGR;
    return true;
    }
  if(mode==1004){
    if(on) modes|=TERMMODE_FOCUS; else modes&=~TERMMODE_FOCUS;
    return true;
    }
  if(mode==2004){
    if(on) modes|=TERMMODE_BRACKETEDPASTE; else modes&=~TERMMODE_BRACKETEDPASTE;
    return true;
    }
  if(mode==2026){
    if(on) modes|=TERMMODE_SYNCHRONIZED; else modes&=~TERMMODE_SYNCHRONIZED;
    return true;
    }
  return false;
  }


// Return a private mode
FXbool FXTerminalScreen::getPrivateMode(FXint mode) const {
  switch(mode){
    case 1: return getMode(TERMMODE_CURSORKEYS);
    case 6: return getMode(TERMMODE_ORIGIN);
    case 7: return getMode(TERMMODE_WRAP);
    case 12: return getMode(TERMMODE_CURSORBLINK);
    case 25: return getMode(TERMMODE_CURSORVISIBLE);
    case 47:
    case 1047:
    case 1049: return altActive;
    case 1000: return getMode(TERMMODE_MOUSEX10);
    case 1002: return getMode(TERMMODE_MOUSEBUTTON);
    case 1003: return getMode(TERMMODE_MOUSEANY);
    case 1006: return getMode(TERMMODE_MOUSESGR);
    case 1004: return getMode(TERMMODE_FOCUS);
    case 2004: return getMode(TERMMODE_BRACKETEDPASTE);
    case 2026: return getMode(TERMMODE_SYNCHRONIZED);
    default: return false;
    }
  }


// Set or reset an ANSI mode
FXbool FXTerminalScreen::setAnsiMode(FXint mode,FXbool on){
  if(mode==4){
    if(on) modes|=TERMMODE_INSERT; else modes&=~TERMMODE_INSERT;
    return true;
    }
  if(mode==20){
    if(on) modes|=TERMMODE_NEWLINE; else modes&=~TERMMODE_NEWLINE;
    return true;
    }
  return false;
  }


// Set application keypad mode
void FXTerminalScreen::setApplicationKeypad(FXbool on){
  if(on) modes|=TERMMODE_APPKEYPAD; else modes&=~TERMMODE_APPKEYPAD;
  }


// Return the mouse tracking mode
FXint FXTerminalScreen::getMouseMode() const {
  if(getMode(TERMMODE_MOUSEANY)) return 3;
  if(getMode(TERMMODE_MOUSEBUTTON)) return 2;
  if(getMode(TERMMODE_MOUSEX10)) return 1;
  return 0;
  }


// Set the window title
void FXTerminalScreen::setTitle(const FXString& text){
  title=sanitize(text,1024);
  }


// Set the working directory
void FXTerminalScreen::setWorkingDirectory(const FXString& path){
  workingdir=sanitize(path,TERMTEXTLIMIT);
  }


// Set the current hyperlink
void FXTerminalScreen::setHyperlink(const FXString& uri){
  hyperlink=sanitize(uri,TERMTEXTLIMIT);
  }


// Set a pending clipboard request
void FXTerminalScreen::setClipboardRequest(const FXString& request){
  clipboard=sanitize(request,TERMTEXTLIMIT);
  }


// Take the pending clipboard request
FXString FXTerminalScreen::takeClipboardRequest(){
  FXString request=clipboard;
  clipboard.clear();
  return request;
  }


// Set a palette entry
void FXTerminalScreen::setPaletteColor(FXint index,FXColor color){
  if(0<=index && index<256) palette[index]=color;
  }


// Return a palette entry
FXColor FXTerminalScreen::getPaletteColor(FXint index) const {
  if(0<=index && index<256) return palette[index];
  return 0;
  }


// Set the scrollback capacity
void FXTerminalScreen::setScrollbackLines(FXint capacity){
  scrollcap=FXMAX(0,capacity);
  while((FXint)scrollback.size()>scrollcap) scrollback.pop_front();
  }


// Drop the retained scrollback rows; the screen and the capacity stay
void FXTerminalScreen::clearScrollback(){
  scrollback.clear();
  }

}

/*******************************************************************************/
