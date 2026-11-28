/********************************************************************************
*                                                                               *
*                         T e r m i n a l   P a r s e r                         *
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
#include "fxterminalcharset.h"
#include "fxterminalparser.h"

#include <vector>

using namespace FX;

/*******************************************************************************/

namespace FX {

// Limits for the incremental buffers
static const FXint TERMCSILIMIT=256;
static const FXint TERMSTRINGLIMIT=8192;


// Split a string on one delimiter, keeping empty fields
static void splitFields(const FXString& text,FXchar separator,std::vector<FXString>& fields){
  FXint start=0;
  FXint i;
  fields.clear();
  for(i=0; i<=text.length(); i++){
    if(i==text.length() || text[i]==separator){
      fields.push_back(text.mid(start,i-start));
      start=i+1;
      }
    }
  }


// Parse a hexadecimal color component, scaled to 0..255
static FXuint colorComponent(const FXString& text){
  FXint n=text.length();
  FXint value;
  if(n<=0) return 0;
  value=text.toInt(16);
  if(n==1) return (FXuint)(value*17);
  if(n==2) return (FXuint)value;
  if(n==3) return (FXuint)((value*255)/0x0FFF);
  return (FXuint)((value*255)/0xFFFF);
  }


// Make an RGB cell color
static FXuint rgbColor(FXint r,FXint g,FXint b){
  return TERMCOLOR_RGB|(((FXuint)FXCLAMP(0,r,255))<<16)|(((FXuint)FXCLAMP(0,g,255))<<8)|((FXuint)FXCLAMP(0,b,255));
  }


// Constructor
FXTerminalParser::FXTerminalParser(FXTerminalScreen& targetscreen,FXTerminalCharset *textcharset):screen(targetscreen),charset(textcharset),narrow(textcharset && !textcharset->isUTF8()){
  resetParser();
  }


// Destructor
FXTerminalParser::~FXTerminalParser(){
  }


// Drop a partial sequence, keeping the screen and the DEC character sets
void FXTerminalParser::flush(){
  state=PARSER_GROUND;
  csi.clear();
  string.clear();
  intermediates.clear();
  privateprefix=0;
  stringkind=0;
  csiOverflow=false;
  stringOverflow=false;
  charsetTarget=0;
  utf8need=0;
  utf8have=0;
  utf8code=0;
  }


// Reset the parser only
void FXTerminalParser::resetParser(){
  flush();
  charsets[0]='B';
  charsets[1]='B';
  charsets[2]='B';
  charsets[3]='B';
  gl=0;
  }


// Decode output through another character encoding
void FXTerminalParser::setCharset(FXTerminalCharset *textcharset){
  charset=textcharset;
  narrow=(textcharset && !textcharset->isUTF8());
  flush();
  }


// Reset the parser
void FXTerminalParser::reset(){
  resetParser();
  }


// Feed a string
void FXTerminalParser::feed(const FXString& text){
  feed(text.text(),text.length());
  }


// Return one CSI parameter, or the default when it is omitted
FXint FXTerminalParser::param(FXint index,FXint def) const {
  FXint pos=0;
  FXint current=0;
  FXint slot=0;
  FXbool have=false;
  while(pos<=csi.length()){
    if(pos==csi.length() || csi[pos]==';'){
      if(slot==index) return have ? current : def;
      slot++;
      current=0;
      have=false;
      if(pos==csi.length()) break;
      }
    else if('0'<=csi[pos] && csi[pos]<='9'){
      current=current*10+(csi[pos]-'0');
      if(current>1000000) current=1000000;
      have=true;
      }
    pos++;
    }
  return def;
  }


// Return the number of CSI parameter fields
FXint FXTerminalParser::paramCount() const {
  FXint i;
  FXint count=0;
  if(csi.empty()) return 0;
  count=1;
  for(i=0; i<csi.length(); i++) if(csi[i]==';') count++;
  return count;
  }


// Map a printable codepoint through the active DEC character set
FXwchar FXTerminalParser::mapCharset(FXwchar ch) const {
  if(ch<0x20) return ch;
  if(charsets[gl]=='A'){
    if(ch=='#') return 0x00A3;
    return ch;
    }
  if(charsets[gl]!='0') return ch;
  switch(ch){
    case '`': return 0x25C6;
    case 'a': return 0x2592;
    case 'b': return 0x2409;
    case 'c': return 0x240C;
    case 'd': return 0x240D;
    case 'e': return 0x240A;
    case 'f': return 0x00B0;
    case 'g': return 0x00B1;
    case 'h': return 0x2424;
    case 'i': return 0x240B;
    case 'j': return 0x2518;
    case 'k': return 0x2510;
    case 'l': return 0x250C;
    case 'm': return 0x2514;
    case 'n': return 0x253C;
    case 'o': return 0x23BA;
    case 'p': return 0x23BB;
    case 'q': return 0x2500;
    case 'r': return 0x23BC;
    case 's': return 0x23BD;
    case 't': return 0x251C;
    case 'u': return 0x2524;
    case 'v': return 0x2534;
    case 'w': return 0x252C;
    case 'x': return 0x2502;
    case 'y': return 0x2264;
    case 'z': return 0x2265;
    case '{': return 0x03C0;
    case '|': return 0x2260;
    case '}': return 0x00A3;
    case '~': return 0x00B7;
    default: return ch;
    }
  }


// Print a codepoint after charset mapping
void FXTerminalParser::putCodepoint(FXwchar ch){
  screen.putChar(mapCharset(ch));
  }


// Execute one byte that is not part of a sequence
void FXTerminalParser::executeByte(FXuchar ch){
  switch(ch){
    case 0x0E:
      gl=1;
      break;
    case 0x0F:
      gl=0;
      break;
    default:
      screen.execute(ch);
      break;
    }
  }


// Parse one complete OSC string
void FXTerminalParser::dispatchOsc(){
  FXint number;
  FXint separator;
  FXint separator2;
  FXString value;
  FXString spec;
  FXString uri;
  FXColor color;
  if(stringOverflow || stringkind!=']') return;
  separator=string.find(';');
  number=(separator<0 ? string : string.left(separator)).toInt();
  value=(separator<0 ? FXString() : string.mid(separator+1,string.length()-separator-1));
  if(screen.getTerminalKind()==TERMINAL_VT100) return;
  switch(number){
    case 0:
    case 1:
    case 2:
      screen.setTitle(value);
      break;
    case 7:
      screen.setWorkingDirectory(value);
      break;
    case 8:
      separator2=value.find(';');
      uri=(separator2<0 ? value : value.mid(separator2+1,value.length()-separator2-1));
      screen.setHyperlink(uri);
      break;
    case 4:
      separator2=value.find(';');
      if(separator2>=0){
        spec=value.mid(separator2+1,value.length()-separator2-1);
        color=parseColor(spec);
        if(color) screen.setPaletteColor(value.left(separator2).toInt(),color);
        }
      break;
    case 52:
      screen.setClipboardRequest(value);
      break;
    case 10:
      if(value=="?") screen.addResponse(FXString("\x1b]10;rgb:cccc/cccc/cccc\x1b\\"));
      break;
    case 11:
      if(value=="?") screen.addResponse(FXString("\x1b]11;rgb:0c0c/0c0c/0c0c\x1b\\"));
      break;
    default:
      break;
    }
  }


// Parse one DCS string
void FXTerminalParser::dispatchDcs(){
  FXString reply;
  if(stringOverflow || stringkind!='P') return;
  if(string.left(2)=="+q"){
    reply="\x1bP0+r";
    reply+=string.mid(2,string.length()-2);
    reply+="\x1b\\";
    screen.addResponse(reply);
    }
  }


// Parse an OSC color specification
FXColor FXTerminalParser::parseColor(const FXString& text) const {
  FXString spec=text;
  FXint i;
  FXint r;
  FXint g;
  FXint b;
  if(spec.length()>7 && spec.left(4)=="rgb:"){
    spec=spec.mid(4,spec.length()-4);
    i=spec.find('/');
    if(i<0) return 0;
    r=colorComponent(spec.left(i));
    spec=spec.mid(i+1,spec.length()-i-1);
    i=spec.find('/');
    if(i<0) return 0;
    g=colorComponent(spec.left(i));
    b=colorComponent(spec.mid(i+1,spec.length()-i-1));
    return (FXColor)FXRGB(r,g,b);
    }
  if(spec.length()==7 && spec[0]=='#'){
    r=spec.mid(1,2).toInt(16);
    g=spec.mid(3,2).toInt(16);
    b=spec.mid(5,2).toInt(16);
    return (FXColor)FXRGB(r,g,b);
    }
  return 0;
  }


// Parse one SGR parameter list
void FXTerminalParser::parseSgr(const FXString& text){
  std::vector<FXString> fields;
  std::vector<FXString> sub;
  FXint i;
  FXint code;
  FXint mode;
  FXint r;
  FXint g;
  FXint b;
  FXint style;
  FXuint color;
  splitFields(text,';',fields);
  if(fields.empty()) fields.push_back("0");
  for(i=0; i<(FXint)fields.size(); i++){
    if(fields[i].find(':')>=0){
      splitFields(fields[i],':',sub);
      code=sub[0].toInt();
      if(code==4){
        style=(sub.size()>1 ? sub[1].toInt() : 1);
        if(style<=0){
          screen.setAttribute(TERMATTR_UNDERLINE,false);
          screen.setAttributes((FXushort)(screen.getAttributes()&~TERMATTR_UNDERLINE_MASK));
          }
        else{
          screen.setAttribute(TERMATTR_UNDERLINE,true);
          screen.setAttributes((FXushort)((screen.getAttributes()&~TERMATTR_UNDERLINE_MASK)|((style&7)<<9)));
          }
        }
      else if(code==38 || code==48 || code==58){
        color=0;
        if(sub.size()>=2 && sub[1].toInt()==5 && sub.size()>=3){
          color=(FXuint)sub[2].toInt();
          }
        else if(sub.size()>=2 && sub[1].toInt()==2){
          if(sub.size()>=6){
            r=sub[3].toInt();
            g=sub[4].toInt();
            b=sub[5].toInt();
            }
          else if(sub.size()>=5){
            r=sub[2].toInt();
            g=sub[3].toInt();
            b=sub[4].toInt();
            }
          else{
            r=0;
            g=0;
            b=0;
            }
          color=rgbColor(r,g,b);
          }
        if(color && code==38) screen.setForeground(color);
        if(color && code==48) screen.setBackground(color);
        }
      }
    else{
      code=fields[i].toInt();
      if(code==0){
        screen.resetAttributes();
        }
      else if(code==1){
        screen.setAttribute(TERMATTR_BOLD,true);
        }
      else if(code==2){
        screen.setAttribute(TERMATTR_DIM,true);
        }
      else if(code==3){
        screen.setAttribute(TERMATTR_ITALIC,true);
        }
      else if(code==4){
        screen.setAttribute(TERMATTR_UNDERLINE,true);
        screen.setAttributes((FXushort)((screen.getAttributes()&~TERMATTR_UNDERLINE_MASK)|(1<<9)));
        }
      else if(code==5 || code==6){
        screen.setAttribute(TERMATTR_BLINK,true);
        }
      else if(code==7){
        screen.setAttribute(TERMATTR_REVERSE,true);
        }
      else if(code==8){
        screen.setAttribute(TERMATTR_HIDDEN,true);
        }
      else if(code==9){
        screen.setAttribute(TERMATTR_STRIKE,true);
        }
      else if(code==21){
        screen.setAttribute(TERMATTR_UNDERLINE,true);
        screen.setAttributes((FXushort)((screen.getAttributes()&~TERMATTR_UNDERLINE_MASK)|(2<<9)));
        }
      else if(code==22){
        screen.setAttribute(TERMATTR_BOLD,false);
        screen.setAttribute(TERMATTR_DIM,false);
        }
      else if(code==23){
        screen.setAttribute(TERMATTR_ITALIC,false);
        }
      else if(code==24){
        screen.setAttribute(TERMATTR_UNDERLINE,false);
        screen.setAttributes((FXushort)(screen.getAttributes()&~TERMATTR_UNDERLINE_MASK));
        }
      else if(code==25){
        screen.setAttribute(TERMATTR_BLINK,false);
        }
      else if(code==27){
        screen.setAttribute(TERMATTR_REVERSE,false);
        }
      else if(code==28){
        screen.setAttribute(TERMATTR_HIDDEN,false);
        }
      else if(code==29){
        screen.setAttribute(TERMATTR_STRIKE,false);
        }
      else if(code==39){
        screen.setForeground(TERMCOLOR_DEFAULT_FG);
        }
      else if(code==49){
        screen.setBackground(TERMCOLOR_DEFAULT_BG);
        }
      else if(code==53){
        screen.setAttribute(TERMATTR_OVERLINE,true);
        }
      else if(code==55){
        screen.setAttribute(TERMATTR_OVERLINE,false);
        }
      else if(30<=code && code<=37){
        screen.setForeground((FXuint)(code-30));
        }
      else if(40<=code && code<=47){
        screen.setBackground((FXuint)(code-40));
        }
      else if(90<=code && code<=97){
        screen.setForeground((FXuint)(code-90+8));
        }
      else if(100<=code && code<=107){
        screen.setBackground((FXuint)(code-100+8));
        }
      else if(code==38 || code==48 || code==58){
        color=0;
        if(i+1<(FXint)fields.size()){
          mode=fields[i+1].toInt();
          if(mode==5 && i+2<(FXint)fields.size()){
            color=(FXuint)fields[i+2].toInt();
            i+=2;
            }
          else if(mode==2 && i+4<(FXint)fields.size()){
            r=fields[i+2].toInt();
            g=fields[i+3].toInt();
            b=fields[i+4].toInt();
            color=rgbColor(r,g,b);
            i+=4;
            }
          }
        if(color && code==38) screen.setForeground(color);
        if(color && code==48) screen.setBackground(color);
        }
      }
    }
  }


// Dispatch one complete CSI sequence
void FXTerminalParser::dispatchCsi(FXchar final){
  FXint i;
  FXint n;
  FXint row;
  FXint col;
  FXTerminalScreen& t=screen;
  FXString reply;
  if(csiOverflow) return;
  if(privateprefix=='!' && final=='p'){
    t.softReset();
    return;
    }
  switch(final){
    case '@':
      t.insertCharacters(param(0,1));
      break;
    case 'A':
      t.moveCursor(-param(0,1),0);
      break;
    case 'B':
      t.moveCursor(param(0,1),0);
      break;
    case 'C':
      t.moveCursor(0,param(0,1));
      break;
    case 'D':
      t.moveCursor(0,-param(0,1));
      break;
    case 'E':
      t.setCursorRow(t.getCursorRow()+param(0,1));
      t.setCursorColumn(0);
      break;
    case 'F':
      t.setCursorRow(t.getCursorRow()-param(0,1));
      t.setCursorColumn(0);
      break;
    case 'G':
      t.setCursorColumn(param(0,1)-1);
      break;
    case 'H':
    case 'f':
      t.setCursor(param(0,1)-1,param(1,1)-1);
      break;
    case 'I':
      for(i=0; i<param(0,1); i++) t.tab();
      break;
    case 'J':
      t.eraseInDisplay(param(0,0));
      break;
    case 'K':
      t.eraseInLine(param(0,0));
      break;
    case 'L':
      t.insertLines(param(0,1));
      break;
    case 'M':
      t.deleteLines(param(0,1));
      break;
    case 'P':
      t.deleteCharacters(param(0,1));
      break;
    case 'S':
      t.scrollUp(param(0,1));
      break;
    case 'T':
      t.scrollDown(param(0,1));
      break;
    case 'X':
      t.eraseCharacters(param(0,1));
      break;
    case 'Z':
      for(i=0; i<param(0,1); i++) t.backTab();
      break;
    case '`':
      t.setCursorColumn(param(0,1)-1);
      break;
    case 'a':
      t.setCursorColumn(t.getCursorColumn()+param(0,1));
      break;
    case 'b':
      break;
    case 'c':
      if(privateprefix=='>'){
        if(t.getTerminalKind()!=TERMINAL_VT100) t.addResponse(FXString("\x1b[>0;276;0c"));
        }
      else if(t.getTerminalKind()==TERMINAL_VT100){
        t.addResponse(FXString("\x1b[?1;0c"));
        }
      else{
        t.addResponse(FXString("\x1b[?1;2c"));
        }
      break;
    case 'd':
      t.setCursorRow(param(0,1)-1);
      break;
    case 'e':
      t.setCursorRow(t.getCursorRow()+param(0,1));
      break;
    case 'g':
      t.clearTabStops(param(0,0));
      break;
    case 'h':
    case 'l':
      if(privateprefix=='?'){
        n=paramCount();
        if(n==0) t.setPrivateMode(0,final=='h');
        for(i=0; i<n; i++) t.setPrivateMode(param(i,0),final=='h');
        }
      else{
        n=paramCount();
        for(i=0; i<n; i++) t.setAnsiMode(param(i,0),final=='h');
        }
      break;
    case 'm':
      if(privateprefix==0) parseSgr(csi);
      break;
    case 'n':
      if(privateprefix=='?'){
        if(param(0,0)==6){
          reply.format("\x1b[?%d;%dR",t.getCursorRow()+1,t.getCursorColumn()+1);
          t.addResponse(reply);
          }
        else if(param(0,0)==15){
          t.addResponse(FXString("\x1b[?13n"));
          }
        }
      else if(param(0,0)==5){
        t.addResponse(FXString("\x1b[0n"));
        }
      else if(param(0,0)==6){
        reply.format("\x1b[%d;%dR",t.getCursorRow()+1,t.getCursorColumn()+1);
        t.addResponse(reply);
        }
      break;
    case 'q':
      if(privateprefix=='>'){
        t.addResponse(FXString("\x1bP>|foxterm(2.0)\x1b\\"));
        }
      else if(intermediates==" ") t.setCursorStyle(param(0,0));
      break;
    case 'r':
      row=param(0,1)-1;
      col=param(1,t.getRows())-1;
      t.setMargins(row,col);
      break;
    case 's':
      if(privateprefix==0) t.saveCursor();
      break;
    case 't':
      if(param(0,0)==18){
        reply.format("\x1b[8;%d;%dt",t.getRows(),t.getColumns());
        t.addResponse(reply);
        }
      break;
    case 'u':
      if(privateprefix=='?') t.addResponse(FXString("\x1b[?0u"));
      else if(privateprefix==0) t.restoreCursor();
      break;
    default:
      break;
    }
  }


// Feed bytes to the parser
void FXTerminalParser::feed(const FXchar* data,FXint length){
  FXint i;
  FXuchar ch;
  FXint need;
  FXwchar code;
  if(!data || length<=0) return;
  for(i=0; i<length; i++){
    ch=(FXuchar)data[i];
    if(state==PARSER_GROUND && (ch==0x18 || ch==0x1A)){
      resetParser();
      continue;
      }
    if(state==PARSER_STRING && (ch==0x18 || ch==0x1A)){
      resetParser();
      continue;
      }
    if(state==PARSER_CSI && (ch==0x18 || ch==0x1A)){
      resetParser();
      continue;
      }
    if(state==PARSER_GROUND){
      // A single byte encoding owns every byte above ASCII, so it is decoded
      // here, before the eight bit C1 forms below
      if(narrow && ch>=0x80){
        screen.putCell(charset->decode(ch));
        continue;
        }
      if(ch==0x1B){
        state=PARSER_ESCAPE;
        continue;
        }
      if(ch==0x9B){
        state=PARSER_CSI;
        csi.clear();
        intermediates.clear();
        privateprefix=0;
        csiOverflow=false;
        continue;
        }
      if(ch==0x9D){
        state=PARSER_STRING;
        string.clear();
        stringkind=']';
        stringOverflow=false;
        continue;
        }
      if(ch==0x90 || ch==0x98 || ch==0x9E || ch==0x9F){
        state=PARSER_STRING;
        string.clear();
        stringkind=(ch==0x90 ? 'P' : (ch==0x98 ? 'X' : (ch==0x9E ? '^' : '_')));
        stringOverflow=false;
        continue;
        }
      if(ch==0x7F) continue;
      if(ch<0x20){
        executeByte(ch);
        continue;
        }
      if(0x80<=ch && ch<0xA0) continue;
      if(ch<0x80){
        putCodepoint(ch);
        continue;
        }
      if(ch<0xC2){
        screen.putChar(0xFFFD);
        continue;
        }
      if(ch<0xE0){
        need=2;
        code=ch&0x1F;
        }
      else if(ch<0xF0){
        need=3;
        code=ch&0x0F;
        }
      else if(ch<0xF5){
        need=4;
        code=ch&0x07;
        }
      else{
        screen.putChar(0xFFFD);
        continue;
        }
      utf8need=need;
      utf8have=1;
      utf8code=code;
      state=PARSER_UTF8;
      continue;
      }
    if(state==PARSER_UTF8){
      if((ch&0xC0)!=0x80){
        screen.putChar(0xFFFD);
        state=PARSER_GROUND;
        i--;
        continue;
        }
      utf8code=(utf8code<<6)|(ch&0x3F);
      utf8have++;
      if(utf8have<utf8need) continue;
      code=utf8code;
      if((utf8need==2 && code<0x80) ||
         (utf8need==3 && code<0x800) ||
         (utf8need==4 && code<0x10000) ||
         (0xD800<=code && code<=0xDFFF) || 0x10FFFF<code){
        code=0xFFFD;
        }
      screen.putChar(code);
      state=PARSER_GROUND;
      utf8need=0;
      utf8have=0;
      utf8code=0;
      continue;
      }
    if(state==PARSER_ESCAPE){
      if(ch==0x1B){
        state=PARSER_ESCAPE;
        continue;
        }
      if(ch=='['){
        state=PARSER_CSI;
        csi.clear();
        intermediates.clear();
        privateprefix=0;
        csiOverflow=false;
        continue;
        }
      if(ch==']'){
        state=PARSER_STRING;
        string.clear();
        stringkind=']';
        stringOverflow=false;
        continue;
        }
      if(ch=='P' || ch=='X' || ch=='^' || ch=='_'){
        state=PARSER_STRING;
        string.clear();
        stringkind=ch;
        stringOverflow=false;
        continue;
        }
      if(ch=='(' || ch==')' || ch=='*' || ch=='+'){
        charsetTarget=(ch=='(' ? 0 : (ch==')' ? 1 : (ch=='*' ? 2 : 3)));
        state=PARSER_CHARSET;
        continue;
        }
      if(ch=='#'){
        state=PARSER_HASH;
        continue;
        }
      if(ch=='7'){
        screen.saveCursor();
        }
      else if(ch=='8'){
        screen.restoreCursor();
        }
      else if(ch=='D'){
        screen.index();
        }
      else if(ch=='E'){
        screen.carriageReturn();
        screen.lineFeed();
        }
      else if(ch=='H'){
        screen.setTabStop();
        }
      else if(ch=='M'){
        screen.reverseIndex();
        }
      else if(ch=='c'){
        screen.reset(screen.getTerminalKind());
        resetParser();
        continue;
        }
      else if(ch=='='){
        screen.setApplicationKeypad(true);
        }
      else if(ch=='>'){
        screen.setApplicationKeypad(false);
        }
      else if(ch=='N' || ch=='O'){
        }
      else if(ch=='n' || ch=='o'){
        }
      else if(ch=='\\'){
        }
      state=PARSER_GROUND;
      continue;
      }
    if(state==PARSER_HASH){
      state=PARSER_GROUND;
      continue;
      }
    if(state==PARSER_CHARSET){
      if(ch=='0' || ch=='B' || ch=='A') charsets[charsetTarget]=(FXchar)ch;
      else charsets[charsetTarget]='B';
      state=PARSER_GROUND;
      continue;
      }
    if(state==PARSER_CSI){
      if(ch==0x1B){
        state=PARSER_ESCAPE;
        continue;
        }
      if(ch<0x20){
        executeByte(ch);
        continue;
        }
      if(ch==0x7F) continue;
      if(0x20<=ch && ch<=0x2F){
        if(ch=='!' && csi.empty() && intermediates.empty()){
          privateprefix='!';
          }
        else if(intermediates.length()<8){
          intermediates+=(FXchar)ch;
          }
        continue;
        }
      if(0x30<=ch && ch<=0x3F){
        if(csi.empty() && (ch=='?' || ch=='>' || ch=='=' || ch=='!')){
          privateprefix=(FXchar)ch;
          }
        else if(csi.length()<TERMCSILIMIT){
          csi+=(FXchar)ch;
          }
        else{
          csiOverflow=true;
          }
        continue;
        }
      if(0x40<=ch && ch<=0x7E){
        dispatchCsi((FXchar)ch);
        state=PARSER_GROUND;
        csi.clear();
        intermediates.clear();
        privateprefix=0;
        csiOverflow=false;
        continue;
        }
      continue;
      }
    if(state==PARSER_STRING){
      if(ch==0x1B){
        state=PARSER_STRING_ESC;
        continue;
        }
      if(ch==0x9C){
        dispatchOsc();
        state=PARSER_GROUND;
        string.clear();
        stringkind=0;
        stringOverflow=false;
        continue;
        }
      if(ch==0x07 && stringkind==']'){
        dispatchOsc();
        state=PARSER_GROUND;
        string.clear();
        stringkind=0;
        stringOverflow=false;
        continue;
        }
      if(string.length()<TERMSTRINGLIMIT) string+=(FXchar)ch;
      else stringOverflow=true;
      continue;
      }
    if(state==PARSER_STRING_ESC){
      if(ch=='\\' || ch==0x9C){
        if(stringkind=='P') dispatchDcs();
        else dispatchOsc();
        state=PARSER_GROUND;
        string.clear();
        stringkind=0;
        stringOverflow=false;
        continue;
        }
      state=PARSER_STRING;
      if(string.length()<TERMSTRINGLIMIT){
        string+='\x1B';
        string+=(FXchar)ch;
        }
      else{
        stringOverflow=true;
        }
      continue;
      }
    }
  }

}
