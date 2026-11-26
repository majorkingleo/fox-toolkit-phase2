/********************************************************************************
*                                                                               *
*                                 Test Console Widget                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>




/*******************************************************************************/


// Console Window
class ConsoleWindow : public FXMainWindow {
  FXDECLARE(ConsoleWindow)
protected:
  FXToolTip       *tooltip;
  FXMenuBar       *menubar;
  FXMenuPane      *filemenu;
  FXMenuPane      *helpmenu;
  FXVerticalFrame *contents;
  FXVerticalFrame *frame;
  FXHorizontalFrame *controls;
  FXConsole       *console;
  FXTextField     *findfield;
  FXLabel         *status;
  FXuint           linenumber;
  FXbool           flooding;
protected:
  ConsoleWindow(){}
  void appendLines(FXint count);
  void startFlood();
  void stopFlood();
public:
  long onCmdAbout(FXObject*,FXSelector,void*);
  long onCmdAppend(FXObject*,FXSelector,void*);
  long onCmdAppendMany(FXObject*,FXSelector,void*);
  long onCmdFlood(FXObject*,FXSelector,void*);
  long onCmdStopFlood(FXObject*,FXSelector,void*);
  long onCmdClear(FXObject*,FXSelector,void*);
  long onCmdFind(FXObject*,FXSelector,void*);
  long onCmdFindNext(FXObject*,FXSelector,void*);
  long onCmdFindPrev(FXObject*,FXSelector,void*);
  long onTimeout(FXObject*,FXSelector,void*);
public:
  enum{
    ID_ABOUT=FXMainWindow::ID_LAST,
    ID_CONSOLE,
    ID_APPEND,
    ID_APPEND_MANY,
    ID_FLOOD,
    ID_STOP_FLOOD,
    ID_CLEAR,
    ID_FIND,
    ID_FIND_NEXT,
    ID_FIND_PREV,
    ID_FLOOD_TIMER,
    ID_LAST
    };
public:
  ConsoleWindow(FXApp* a);
  virtual void create();
  virtual ~ConsoleWindow();
  };


/*******************************************************************************/

// Map
FXDEFMAP(ConsoleWindow) ConsoleWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_ABOUT,ConsoleWindow::onCmdAbout),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_APPEND,ConsoleWindow::onCmdAppend),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_APPEND_MANY,ConsoleWindow::onCmdAppendMany),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_FLOOD,ConsoleWindow::onCmdFlood),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_STOP_FLOOD,ConsoleWindow::onCmdStopFlood),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_CLEAR,ConsoleWindow::onCmdClear),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_FIND,ConsoleWindow::onCmdFind),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_FIND_NEXT,ConsoleWindow::onCmdFindNext),
  FXMAPFUNC(SEL_COMMAND,ConsoleWindow::ID_FIND_PREV,ConsoleWindow::onCmdFindPrev),
  FXMAPFUNC(SEL_TIMEOUT,ConsoleWindow::ID_FLOOD_TIMER,ConsoleWindow::onTimeout),
  };


// Object implementation
FXIMPLEMENT(ConsoleWindow,FXMainWindow,ConsoleWindowMap,ARRAYNUMBER(ConsoleWindowMap))


/*******************************************************************************/

// Make some windows
ConsoleWindow::ConsoleWindow(FXApp* a):FXMainWindow(a,"Console",nullptr,nullptr,DECOR_ALL,0,0,800,800){
  linenumber=0;
  flooding=false;

  // Menubar
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

  // Contents
  contents=new FXVerticalFrame(this,FXuint(LAYOUT_SIDE_TOP)|FXuint(FRAME_NONE)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_FILL_Y));

  // Demo controls
  controls=new FXHorizontalFrame(contents,LAYOUT_FILL_X|LAYOUT_TOP|FRAME_NONE,0,0,0,0,4,4,3,3);
  new FXButton(controls,"Append line",nullptr,this,ID_APPEND,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Append 1000",nullptr,this,ID_APPEND_MANY,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Start flood",nullptr,this,ID_FLOOD,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Stop",nullptr,this,ID_STOP_FLOOD,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Clear",nullptr,this,ID_CLEAR,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  findfield=new FXTextField(controls,18,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_CENTER_Y));
  findfield->setText("line");
  new FXButton(controls,"Find",nullptr,this,ID_FIND,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Prev",nullptr,this,ID_FIND_PREV,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(controls,"Next",nullptr,this,ID_FIND_NEXT,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  status=new FXLabel(contents,"Ready",nullptr,FXuint(JUSTIFY_LEFT)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_TOP)|FXuint(FRAME_NONE),0,0,0,0,5,5,2,2);

  // Console frame
  frame=new FXVerticalFrame(contents,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);

  // Console widget
  console=new FXConsole(frame,this,ID_CONSOLE,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 2,2,2,2);
  console->setHistoryLines(10000);
  FXHiliteStyle styles[3];
  styles[0]={FXRGB(220,220,220),FXRGB(35,35,35),FXRGB(255,255,255),FXRGB(35,100,170),FXRGB(255,255,255),FXRGB(70,70,20),FXRGB(45,45,45),0};
  styles[1]={FXRGB(120,210,120),FXRGB(35,35,35),FXRGB(255,255,255),FXRGB(35,100,170),FXRGB(255,255,255),FXRGB(70,70,20),FXRGB(45,45,45),0};
  styles[2]={FXRGB(240,190,100),FXRGB(35,35,35),FXRGB(255,255,255),FXRGB(35,100,170),FXRGB(255,255,255),FXRGB(70,70,20),FXRGB(45,45,45),FXText::STYLE_BOLD};
  console->setStyleTable(styles,ARRAYNUMBER(styles));
  appendLines(1000);

  // File Menu
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",nullptr,getApp(),FXApp::ID_QUIT);
  new FXMenuTitle(menubar,"&File",nullptr,filemenu);

  helpmenu=new FXMenuPane(this);
  new FXMenuCommand(helpmenu,"About...",nullptr,this,ID_ABOUT);
  new FXMenuTitle(menubar,"&Help",nullptr,helpmenu,LAYOUT_RIGHT);

  // Tooltip
  tooltip=new FXToolTip(getApp());
  }


// Start
void ConsoleWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  startFlood();
  }


// About
long ConsoleWindow::onCmdAbout(FXObject*,FXSelector,void*){
  FXMessageBox::information(this,MBOX_OK,"About Console","Console Widget.\nCopyright (C) 2006 Jeroen van der Zijp");
  return 1;
  }


// Append a batch in one widget update; style ids cycle through the demo palette
void ConsoleWindow::appendLines(FXint count){
  FXString text;
  std::vector<FXuint> styles;
  for(FXint i=0; i<count; i++){
    FXString line;
    line.format("line-%08u  channel=%u  value=%0.3f  status=%s",linenumber,linenumber%8,(linenumber%10000)*0.125,(linenumber%7)==0 ? "warning" : "ready");
    FXuint style=(linenumber%7)==0 ? 2 : (linenumber%3)==0 ? 1 : 0;
    for(FXint j=0; j<line.length(); j++) styles.push_back(style);
    text+=line;
    text+='\n';
    styles.push_back(style);
    linenumber++;
    }
  console->appendStyled(text,styles.data());
  FXString statusmessage;
  statusmessage.format("Lines generated: %u   Retained: %d / %d   Matches: %d",linenumber,console->getNumLines(),console->getHistoryLines(),console->getFindCount());
  status->setText(statusmessage);
  }


// Append one representative line
long ConsoleWindow::onCmdAppend(FXObject*,FXSelector,void*){
  appendLines(1);
  return 1;
  }


// Append a thousand lines in one batch
long ConsoleWindow::onCmdAppendMany(FXObject*,FXSelector,void*){
  appendLines(1000);
  return 1;
  }


// Start the repeating 1000-lines-per-second timer
void ConsoleWindow::startFlood(){
  if(!flooding){
    flooding=true;
    getApp()->addTimeout(this,ID_FLOOD_TIMER,100000000);
    }
  }


// Stop the flood timer
void ConsoleWindow::stopFlood(){
  flooding=false;
  getApp()->removeTimeout(this,ID_FLOOD_TIMER);
  }


// Start the flood timer
long ConsoleWindow::onCmdFlood(FXObject*,FXSelector,void*){
  startFlood();
  return 1;
  }


// Stop the flood timer
long ConsoleWindow::onCmdStopFlood(FXObject*,FXSelector,void*){
  stopFlood();
  status->setText("Flood stopped");
  return 1;
  }


// Clear retained lines and search state
long ConsoleWindow::onCmdClear(FXObject*,FXSelector,void*){
  console->clearText();
  console->setFindPattern(FXString::null,0);
  status->setText("Console cleared");
  return 1;
  }


// Apply case-insensitive search
long ConsoleWindow::onCmdFind(FXObject*,FXSelector,void*){
  console->setFindPattern(findfield->getText(),SEARCH_IGNORECASE);
  FXString statusmessage;
  statusmessage.format("Matches: %d",console->getFindCount());
  status->setText(statusmessage);
  if(console->getFindCount()>0) console->findNext();
  return 1;
  }


// Navigate to next match
long ConsoleWindow::onCmdFindNext(FXObject*,FXSelector,void*){
  console->findNext();
  return 1;
  }


// Navigate to previous match
long ConsoleWindow::onCmdFindPrev(FXObject*,FXSelector,void*){
  console->findPrev();
  return 1;
  }


// Append one hundred lines every hundred milliseconds
long ConsoleWindow::onTimeout(FXObject*,FXSelector,void*){
  if(flooding){
    appendLines(100);
    getApp()->addTimeout(this,ID_FLOOD_TIMER,100000000);
    }
  return 1;
  }


// Delete it
ConsoleWindow::~ConsoleWindow(){
  stopFlood();
  delete filemenu;
  delete helpmenu;
  }


/*******************************************************************************/


// Start the whole thing
int main(int argc,char *argv[]){

  fxtestargs(argc,argv);

  // Make application
  FXApp application("Console","FoxTest");

  // Open display
  application.init(argc,argv);

  // Make window
  new ConsoleWindow(&application);

  // Create app
  application.create();

  // Run
  return application.run();
  }


