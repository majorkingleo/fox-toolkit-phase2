/********************************************************************************
*                                                                               *
*                     T e r m i n a l   M o d e l   T e s t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************/
#include "fx.h"
#include "fxterminalscreen.h"
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
  testlimits();
  if(failures){
    printf("%d terminal model failure(s)\n",failures);
    return 1;
    }
  printf("terminal model tests passed\n");
  return 0;
  }
