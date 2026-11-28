/********************************************************************************
*                                                                               *
*                     T e r m i n a l   M o d e l   T e s t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************/
#include "fx.h"
#include "fxterminalscreen.h"
#include "fxterminalcharset.h"
#include "fxterminalparser.h"
#include <stdio.h>
#include <string.h>

using namespace FX;

/*******************************************************************************/

static FXint failures=0;

// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    failures++;
    printf("FAIL: %s\n",what);
    }
  }

// Check one row of text
static void checkrow(FXTerminalScreen& screen,FXint row,const FXchar* expected,const FXchar* what){
  FXString actual=screen.getRowText(row,true);
  if(actual!=FXString(expected)){
    failures++;
    printf("FAIL: %s: row %d is [%s], expected [%s]\n",what,row,actual.text(),expected);
    }
  }

// Fresh screen and parser
static void fresh(FXTerminalScreen& screen,FXTerminalParser& parser,FXint cols=80,FXint rows=24,FXint kind=TERMINAL_XTERM){
  screen.reset(kind);
  screen.resize(cols,rows);
  parser.reset();
  }

// Feed a C string
static void feed(FXTerminalParser& parser,const FXchar* text){
  parser.feed(text,(FXint)strlen(text));
  }

// Take and check the next response
static void checkresponse(FXTerminalScreen& screen,const FXchar* expected,const FXchar* what){
  FXString reply;
  if(!screen.takeResponse(reply)){
    failures++;
    printf("FAIL: %s: no response\n",what);
    return;
    }
  if(reply!=FXString(expected)){
    failures++;
    printf("FAIL: %s: response [%s], expected [%s]\n",what,reply.text(),expected);
    }
  }

/*******************************************************************************/

// Text, cursor motion, wrapping and tabs
static void testtext(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,4);
  feed(parser,"abc\r\ndef");
  checkrow(screen,0,"abc","text");
  checkrow(screen,1,"def","text");
  check(screen.getCursorRow()==1 && screen.getCursorColumn()==3,"text cursor");

  fresh(screen,parser,5,3);
  feed(parser,"abcdef");
  checkrow(screen,0,"abcde","wrap");
  checkrow(screen,1,"f","wrap");
  check(screen.getCursorRow()==1 && screen.getCursorColumn()==1,"wrap cursor");

  fresh(screen,parser,20,3);
  feed(parser,"a\tb");
  checkrow(screen,0,"a       b","tab");
  check(screen.getCursorColumn()==9,"tab cursor");
  feed(parser,"\x1b[3g\r\t");
  check(screen.getCursorColumn()==19,"cleared tabs");

  fresh(screen,parser,10,3);
  feed(parser,"\x1b[1;5H\x1bH\r\t");
  check(screen.getCursorColumn()==4,"tab set");

  fresh(screen,parser,5,3);
  feed(parser,"\x1b[?7labcdefg");
  checkrow(screen,0,"abcdg","no wrap");
  check(screen.getWrapPending(),"no wrap pending");
  }

// Erase, insert and delete
static void testedit(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"abcdef\x1b[1;3H\x1b[K");
  checkrow(screen,0,"ab","erase line");
  feed(parser,"XYZ");
  checkrow(screen,0,"abXYZ","erase then print");

  fresh(screen,parser,10,3);
  feed(parser,"abcdef\x1b[1;1H\x1b[2@");
  checkrow(screen,0,"  abcdef","insert chars");
  feed(parser,"\x1b[1;1H\x1b[2P");
  checkrow(screen,0,"abcdef","delete chars");
  feed(parser,"\x1b[1;2H\x1b[3X");
  checkrow(screen,0,"a   ef","erase chars");

  fresh(screen,parser,10,3);
  feed(parser,"line0\r\nline1\r\nline2");
  feed(parser,"\x1b[2;1H\x1b[L");
  checkrow(screen,0,"line0","insert line");
  checkrow(screen,1,"","insert line");
  checkrow(screen,2,"line1","insert line");
  feed(parser,"\x1b[2;1H\x1b[M");
  checkrow(screen,1,"line1","delete line");
  feed(parser,"\x1b[1;1H\x1b[2J");
  checkrow(screen,0,"","clear display");
  }

// Margins, scrolling and origin mode
static void testscroll(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,5);
  feed(parser,"\x1b[2;4r");
  check(screen.getMarginTop()==1 && screen.getMarginBottom()==3,"margins");
  feed(parser,"\x1b[4;1H\nX");
  checkrow(screen,3,"X","scroll region");
  check(screen.getRowText(1,true).empty(),"scroll region blank");
  feed(parser,"\x1b[2;1H\x1bMX");
  checkrow(screen,1,"X","reverse index");
  feed(parser,"\x1b[r\x1b[?6h\x1b[1;1HX");
  checkrow(screen,0,"X","origin mode");
  check(screen.getCursorRow()==0,"origin cursor");

  fresh(screen,parser,10,5);
  feed(parser,"\x1b[2;4r\x1b[?6h\x1b[1;1HX");
  checkrow(screen,1,"X","origin home");
  feed(parser,"\x1b[AY");
  checkrow(screen,1,"XY","origin cursor clamp");

  fresh(screen,parser,5,3);
  feed(parser,"\x1b[1;1H\x1b[3S");
  check(screen.getCursorRow()==0,"scroll keeps cursor");
  }

// SGR attributes and colors
static void testrendition(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"\x1b[1;31;48;5;196mX");
  check((screen.getCell(0,0).attr&TERMATTR_BOLD)!=0,"bold");
  check(screen.getCell(0,0).fg==1,"basic foreground");
  check(screen.getCell(0,0).bg==196,"256 background");
  feed(parser,"\x1b[0;38;2;1;2;3mY");
  check(screen.getCell(0,1).fg==(TERMCOLOR_RGB|0x010203),"truecolor");
  feed(parser,"\x1b[4:3;21mZ");
  check((screen.getCell(0,2).attr&TERMATTR_UNDERLINE)!=0,"underline");
  check((screen.getCell(0,2).attr&TERMATTR_UNDERLINE_MASK)==(2<<9),"underline style");
  feed(parser,"\x1b[22;24;39;49mQ");
  check((screen.getCell(0,3).attr&(TERMATTR_BOLD|TERMATTR_UNDERLINE))==0,"attribute reset");
  check(screen.getCell(0,3).fg==TERMCOLOR_DEFAULT_FG,"default foreground");
  check(screen.getCell(0,3).bg==TERMCOLOR_DEFAULT_BG,"default background");
  }

// Alternate screen
static void testaltscreen(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"main");
  feed(parser,"\x1b[?1049h");
  check(screen.isAlternateScreen(),"alternate enter");
  checkrow(screen,0,"","alternate clear");
  feed(parser,"alt");
  checkrow(screen,0,"alt","alternate text");
  feed(parser,"\x1b[?1049l");
  check(!screen.isAlternateScreen(),"alternate leave");
  checkrow(screen,0,"main","primary restored");
  }

// Wide, combining, ZWJ and malformed UTF-8
static void testunicode(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"A");
  parser.feed("\xE4\xB8\xAD",3);
  feed(parser,"B");
  checkrow(screen,0,"A\xE4\xB8\xAD" "B","wide character");
  check(screen.getCell(0,1).width==2 && screen.getCell(0,2).width==0,"wide cells");
  check(screen.getCell(0,3).ch=='B',"wide cursor");

  fresh(screen,parser,10,3);
  feed(parser,"A");
  parser.feed("\xCC\x81",2);
  check(screen.getCell(0,0).cluster!=0,"combining cluster");
  checkrow(screen,0,"A\xCC\x81","combining text");

  fresh(screen,parser,10,3);
  parser.feed("\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB",12);
  checkrow(screen,0,"\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB","ZWJ sequence");

  fresh(screen,parser,10,3);
  parser.feed("\xC3\x28",2);
  checkrow(screen,0,"\xEF\xBF\xBD(","malformed UTF-8");
  }

// OSC title, directory, hyperlink, clipboard and limits
static void testosc(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  FXString text;
  fresh(screen,parser,10,3);
  feed(parser,"\x1b]0;My\x01Title\x07");
  check(screen.getTitle()==FXString("My Title"),"sanitized title");
  feed(parser,"\x1b]7;/tmp/work\x07");
  check(screen.getWorkingDirectory()==FXString("/tmp/work"),"working directory");
  feed(parser,"\x1b]8;;https://example.test/a\x07");
  check(screen.getHyperlink()==FXString("https://example.test/a"),"hyperlink");
  feed(parser,"\x1b]52;c;YWJj\x07");
  check(screen.takeClipboardRequest()==FXString("c;YWJj"),"clipboard request");
  feed(parser,"\x1b]4;1;rgb:ff/00/00\x07");
  check(screen.getPaletteColor(1)==(FXColor)FXRGB(255,0,0),"palette change");

  fresh(screen,parser,10,3);
  text="\x1b]0;";
  for(FXint i=0; i<9000; i++) text+='A';
  text+='\x07';
  feed(parser,text.text());
  check(screen.getTitle().empty(),"oversized OSC ignored");
  }

// Modes and mouse tracking
static void testmodes(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"\x1b[?1h\x1b[?7l\x1b[?25l");
  check(screen.getPrivateMode(1) && !screen.getPrivateMode(7) && !screen.getPrivateMode(25),"private modes");
  feed(parser,"\x1b[?1000h");
  check(screen.getMouseMode()==1,"x10 mouse");
  feed(parser,"\x1b[?1000l\x1b[?1002h\x1b[?1006h");
  check(screen.getMouseMode()==2 && screen.getPrivateMode(1006),"button mouse");
  feed(parser,"\x1b[5 q");
  check(screen.getCursorStyle()==TERMCURSOR_BAR,"cursor style");
  feed(parser,"\x1b[?2004h\x1b[?1004h");
  check(screen.getPrivateMode(2004) && screen.getPrivateMode(1004),"paste and focus");
  feed(parser,"\x1b[?2004l\x1b[?1004l");
  check(!screen.getPrivateMode(2004) && !screen.getPrivateMode(1004),"paste and focus off");
  feed(parser,"\x1b[4h\x1b[20h");
  check(screen.getMode(TERMMODE_INSERT) && screen.getMode(TERMMODE_NEWLINE),"ansi modes");
  feed(parser,"\x1b=");
  check(screen.getMode(TERMMODE_APPKEYPAD),"application keypad");
  feed(parser,"\x1b>");
  check(!screen.getMode(TERMMODE_APPKEYPAD),"numeric keypad");

  fresh(screen,parser,10,3);
  feed(parser,"abc\x1b[1m\x1b[2;3r\x1b[!p");
  checkrow(screen,0,"abc","soft reset keeps text");
  check(screen.getAttributes()==0,"soft reset rendition");
  check(screen.getMarginTop()==0 && screen.getMarginBottom()==2,"soft reset margins");
  }

// Device replies
static void testreplies(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,20,5);
  feed(parser,"\x1b[3;4H\x1b[6n");
  checkresponse(screen,"\x1b[3;4R","cursor position report");
  feed(parser,"\x1b[?6n");
  checkresponse(screen,"\x1b[?3;4R","private cursor position report");
  feed(parser,"\x1b[5n");
  checkresponse(screen,"\x1b[0n","device status report");
  feed(parser,"\x1b[c");
  checkresponse(screen,"\x1b[?1;2c","primary device attributes");
  feed(parser,"\x1b[>c");
  checkresponse(screen,"\x1b[>0;276;0c","secondary device attributes");
  feed(parser,"\x1b[>0q");
  checkresponse(screen,"\x1bP>|foxterm(2.0)\x1b\\","terminal version report");
  feed(parser,"\x1b[?u");
  checkresponse(screen,"\x1b[?0u","keyboard protocol report");
  feed(parser,"\x1b]10;?\x07");
  checkresponse(screen,"\x1b]10;rgb:cccc/cccc/cccc\x1b\\","foreground color report");
  feed(parser,"\x1b]11;?\x07");
  checkresponse(screen,"\x1b]11;rgb:0c0c/0c0c/0c0c\x1b\\","background color report");
  feed(parser,"\x1bP+q696e646e\x1b\\");
  checkresponse(screen,"\x1bP0+r696e646e\x1b\\","terminal capability report");

  fresh(screen,parser,20,5,TERMINAL_VT100);
  feed(parser,"\x1b[c");
  checkresponse(screen,"\x1b[?1;0c","vt100 device attributes");
  }

// Scrollback capacity and text
static void testscrollback(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  screen.setScrollbackLines(2);
  feed(parser,"1\r\n2\r\n3\r\n4\r\n5");
  check(screen.getScrollbackCount()==2,"scrollback capacity");
  check(screen.getScrollbackText(0,true)==FXString("1"),"scrollback first");
  check(screen.getScrollbackText(1,true)==FXString("2"),"scrollback last");
  checkrow(screen,0,"3","scrollback screen");
  checkrow(screen,2,"5","scrollback screen last");
  feed(parser,"\x1b[3J");
  check(screen.getScrollbackCount()==0,"erase scrollback");
  }

// Resize and soft-wrap reflow
static void testresize(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"abcdefghij");
  screen.resize(5,3);
  checkrow(screen,0,"abcde","reflow first");
  checkrow(screen,1,"fghij","reflow second");
  screen.resize(10,3);
  checkrow(screen,0,"abcdefghij","reflow back");

  fresh(screen,parser,5,3);
  feed(parser,"abcde\r\nfghij");
  screen.resize(10,3);
  checkrow(screen,0,"abcde","hard wrap first");
  checkrow(screen,1,"fghij","hard wrap second");
  }

// VT100 profile gating
static void testvt100(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3,TERMINAL_VT100);
  feed(parser,"\x1b]0;Title\x07");
  check(screen.getTitle().empty(),"vt100 title ignored");
  feed(parser,"\x1b[?1049h");
  check(!screen.isAlternateScreen(),"vt100 alternate ignored");
  feed(parser,"\x1b[?1000h");
  check(screen.getMouseMode()==0,"vt100 mouse ignored");
  feed(parser,"\x1b[38;5;196mX");
  check(screen.getCell(0,0).fg==4,"vt100 256 color reduced");
  feed(parser,"\x1b[0;38;2;255;0;0mY");
  check(screen.getCell(0,1).fg==1,"vt100 truecolor reduced");
  }

// Character sets, unknown sequences and limits
static void testcharset(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  fresh(screen,parser,10,3);
  feed(parser,"\x1b(0lqk\x1b(B");
  checkrow(screen,0,"\xE2\x94\x8C\xE2\x94\x80\xE2\x94\x90","line drawing charset");
  fresh(screen,parser,10,3);
  feed(parser,"\x1b[?9999hX");
  checkrow(screen,0,"X","unknown private mode");
  feed(parser,"\x1b[123;456;789m");
  checkrow(screen,0,"X","unknown SGR ignored");
  feed(parser,"\x1b[1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20m");
  checkrow(screen,0,"X","long SGR ignored");
  feed(parser,"\x1b[999;999H");
  check(screen.getCursorRow()==2 && screen.getCursorColumn()==9,"cursor clamped");
  }

// Character encodings: names, decoding, input mapping and the live switch
static void testencoding(){
  FXTerminalScreen screen;
  FXTerminalCharset charset;
  FXTerminalParser parser(screen,&charset);
  FXTerminalCharset input;
  FXString text;
  FXbool lossy;
  FXchar byte;
  FXwchar wc;
  FXint index;

  // Names and aliases
  check(FXTerminalCharset::count()>=10,"encoding count");
  check(FXString(FXTerminalCharset::name(0))==FXString("UTF-8"),"encoding order");
  check(FXTerminalCharset::find("utf-8")==0,"alias utf8");
  check(FXTerminalCharset::find("ISO_8859_1")==1,"alias latin-1");
  check(FXTerminalCharset::find("latin9")==3,"alias latin-9");
  check(FXTerminalCharset::find("cp1252")==6,"alias cp1252");
  check(FXTerminalCharset::find("koi8r")==7,"alias koi8-r");
  check(FXTerminalCharset::find("ibm850")==9,"alias cp850");
  check(FXTerminalCharset::find("no-such-set")==-1,"unknown encoding");
  check(FXTerminalCharset::find("")==-1,"empty encoding name");
  check(charset.setName("cp437")==FXString("CP437"),"select cp437");
  check(!charset.isUTF8(),"cp437 is single byte");
  check(charset.setName("nonsense")==FXString("UTF-8"),"unsupported name falls back");
  check(charset.isUTF8(),"fallback is utf-8");

  // One sample per encoding, decoded through the parser
  charset.setName("ISO-8859-1");
  fresh(screen,parser,10,3);
  parser.setCharset(&charset);
  parser.feed("\xE4\xA4",2);
  check(screen.getCell(0,0).ch==0xE4 && screen.getCell(0,1).ch==0xA4,"latin-1 decode");
  checkrow(screen,0,"\xC3\xA4\xC2\xA4","latin-1 text");
  charset.setName("ISO-8859-2");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\xE4\xA1",2);
  check(screen.getCell(0,0).ch==0xE4 && screen.getCell(0,1).ch==0x104,"iso-8859-2 decode");
  charset.setName("ISO-8859-15");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\xA4\xBD",2);
  check(screen.getCell(0,0).ch==0x20AC && screen.getCell(0,1).ch==0x153,"iso-8859-15 decode");
  charset.setName("Windows-1250");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\x8A\x9F",2);
  check(screen.getCell(0,0).ch==0x160 && screen.getCell(0,1).ch==0x17A,"windows-1250 decode");
  charset.setName("Windows-1251");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\xC0\xB8",2);
  check(screen.getCell(0,0).ch==0x410 && screen.getCell(0,1).ch==0x451,"windows-1251 decode");
  charset.setName("Windows-1252");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\x80\x9C",2);
  check(screen.getCell(0,0).ch==0x20AC && screen.getCell(0,1).ch==0x153,"windows-1252 decode");
  charset.setName("KOI8-R");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\xC1\xE4",2);
  check(screen.getCell(0,0).ch==0x430 && screen.getCell(0,1).ch==0x414,"koi8-r decode");
  charset.setName("CP437");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\x80\xB0\xE4",3);
  check(screen.getCell(0,0).ch==0xC7 && screen.getCell(0,1).ch==0x2591 && screen.getCell(0,2).ch==0x3A3,"cp437 decode");
  charset.setName("CP850");
  parser.setCharset(&charset);
  fresh(screen,parser,10,3);
  parser.feed("\x80\xB0\xE4",3);
  check(screen.getCell(0,0).ch==0xC7 && screen.getCell(0,1).ch==0x2591 && screen.getCell(0,2).ch==0xF5,"cp850 decode");

  // Every graphic high byte takes exactly one cell; 0x80..0x9F are C1 controls
  // in the ISO-8859 family and print nothing there, so the count per set is the
  // number of bytes that decode to a graphic codepoint
  for(index=1; index<FXTerminalCharset::count(); index++){
    FXint expected=0;
    FXchar raw;
    charset.setName(FXTerminalCharset::name(index));
    parser.setCharset(&charset);
    fresh(screen,parser,200,3);
    for(FXint b=0x80; b<0x100; b++){
      wc=charset.decode((FXuchar)b);
      if(wc>=0xA0) expected++;
      raw=(FXchar)(FXuchar)b;
      parser.feed(&raw,1);
      }
    check(screen.getCursorColumn()==expected,"high bytes are one cell");
    for(FXint c=0; c<expected; c++) check(screen.getCell(0,c).width==1,"high byte cell width");
    }

  // Round trip: every graphic codepoint encodes back to its own byte
  for(index=1; index<FXTerminalCharset::count(); index++){
    charset.setName(FXTerminalCharset::name(index));
    FXint bad=0;
    for(FXint b=0x80; b<0x100; b++){
      wc=charset.decode((FXuchar)b);
      if(wc==0xFFFD || wc<0xA0) continue;
      byte=0;
      if(!charset.encode(wc,byte) || (FXuchar)byte!=(FXuchar)b){ bad++; printf("  %s: U+%04X does not encode back to 0x%02X\n",charset.getName(),(unsigned)wc,(unsigned)b); }
      }
    check(bad==0,"encoding round trip");
    }

  // The soft hyphen is zero width in the Unicode width table, but a single
  // byte session draws every character in one cell and never combines it
  charset.setName("ISO-8859-1");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  parser.feed("a\xAD" "b",3);
  check(screen.getCell(0,1).ch==0xAD && screen.getCell(0,1).width==1,"soft hyphen takes one cell");
  checkrow(screen,0,"a\xC2\xAD" "b","soft hyphen is not a cluster");

  // Input mapping: a character becomes one byte of the session encoding
  input.setName("CP437");
  text=input.encode(FXString("h\xC3\xA4llo"),lossy);
  check(!lossy && text.length()==5 && text[1]==(FXchar)0x84,"cp437 encodes an umlaut");
  text=input.encode(FXString("\xE4\xB8\xAD"),lossy);
  check(lossy && text==FXString("?"),"unmappable input becomes ?");
  text=input.encode(FXString("a\xE2\x82\xAC b"),lossy);
  check(lossy && text==FXString("a? b"),"only the unmappable character is replaced");
  input.setName("UTF-8");
  text=input.encode(FXString("\xE4\xB8\xAD"),lossy);
  check(!lossy && text==FXString("\xE4\xB8\xAD"),"utf-8 input is a pass-through");

  // A live switch keeps the screen and only changes later output; a byte that
  // starts an incomplete UTF-8 sequence reports a replacement for the sequence
  charset.setName("UTF-8");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  feed(parser,"abc");
  parser.feed("\xE4z",2);
  checkrow(screen,0,"abc\xEF\xBF\xBDz","utf-8 byte above ascii is a replacement");
  charset.setName("ISO-8859-1");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  feed(parser,"abc");
  parser.feed("\xE4",1);
  checkrow(screen,0,"abc\xC3\xA4","latin-1 byte above ascii is a character");
  check(screen.getCell(0,0).ch=='a',"switch keeps drawn cells");
  charset.setName("UTF-8");
  parser.setCharset(&charset);
  parser.feed("\xE4z",2);
  checkrow(screen,0,"abc\xC3\xA4\xEF\xBF\xBDz","switch back decodes the next byte as utf-8");
  check(screen.getCell(0,3).ch==0xE4,"switch back keeps the decoded cell");

  // Switching drops a partial escape sequence instead of printing it
  charset.setName("UTF-8");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  feed(parser,"\x1b[");
  charset.setName("ISO-8859-1");
  parser.setCharset(&charset);
  feed(parser,"1m");
  checkrow(screen,0,"1m","switch drops a partial sequence");

  // Eight bit C1 escape forms belong to UTF-8 and are characters in a single
  // byte encoding
  charset.setName("UTF-8");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  parser.feed("\x9B" "1m",3);
  checkrow(screen,0,"","eight bit CSI in utf-8");
  charset.setName("ISO-8859-1");
  parser.setCharset(&charset);
  fresh(screen,parser,20,3);
  parser.feed("\x9B",1);
  checkrow(screen,0,"","latin-1 C1 control is not a CSI");
  check(screen.getCursorColumn()==0,"C1 control does not print");
  }

// Bounded parser stress and limits
static void testlimits(){
  FXTerminalScreen screen;
  FXTerminalParser parser(screen);
  FXchar buffer[1024];
  FXString title;
  FXuint seed=0x12345678;
  FXint i;
  FXint j;
  fresh(screen,parser,80,24);
  screen.setScrollbackLines(100);
  for(i=0; i<1000; i++){
    for(j=0; j<1024; j++){
      seed=seed*1664525u+1013904223u;
      buffer[j]=(FXchar)(seed>>24);
      }
    parser.feed(buffer,1024);
    }
  check(screen.getColumns()==80 && screen.getRows()==24,"fuzz keeps grid size");
  check(screen.getScrollbackCount()<=100,"fuzz keeps scrollback bounded");

  fresh(screen,parser,80,24);
  feed(parser,"\x1b[");
  for(i=0; i<10000; i++) feed(parser,"1;");
  feed(parser,"m");
  checkrow(screen,0,"","long CSI ignored");

  fresh(screen,parser,80,24);
  feed(parser,"\x1b]0;");
  for(i=0; i<4096; i++) feed(parser,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  feed(parser,"\x07");
  check(screen.getTitle().empty(),"oversized OSC discarded");
  }

// Main
int main(){
  testtext();
  testedit();
  testscroll();
  testrendition();
  testaltscreen();
  testunicode();
  testosc();
  testmodes();
  testreplies();
  testscrollback();
  testresize();
  testvt100();
  testcharset();
  testencoding();
  testlimits();
  if(failures){
    printf("%d terminal model failure(s)\n",failures);
    return 1;
    }
  printf("terminal model tests passed\n");
  return 0;
  }
