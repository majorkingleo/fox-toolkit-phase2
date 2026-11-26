/********************************************************************************
*                                                                               *
*                         T e r m i n a l   W i n d o w                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
* You should have received a copy of the GNU General Public License             *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "fx.h"
#include "TerminalTabBar.h"
#include "TerminalWindow.h"

using namespace FX;

/*******************************************************************************/

// Geometry helpers
static FXint settingInt(FXApp* app,const FXchar* key,FXint fallback){
  return app->reg().readIntEntry("WINDOW",key,fallback);
  }


// Window map
FXDEFMAP(TerminalWindow) TerminalWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_TITLE,TerminalWindow::onCmdTitle),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_BELL,TerminalWindow::onCmdBell),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_EXITED,TerminalWindow::onCmdExited),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLIPBOARD,TerminalWindow::onCmdClipboard),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEW,TerminalWindow::onCmdNewWindow),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_COPY,TerminalWindow::onCmdCopy),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_PASTE,TerminalWindow::onCmdPaste),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_ABOUT,TerminalWindow::onCmdAbout),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_QUIT,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL,TerminalWindow::ID_QUIT,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_CLOSE,0,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_CLOSE,TerminalWindow::onCmdQuit),
  };

FXIMPLEMENT(TerminalWindow,FXMainWindow,TerminalWindowMap,ARRAYNUMBER(TerminalWindowMap))


// Construct
TerminalWindow::TerminalWindow(FXApp* a,const FXTerminalLaunch& l):FXMainWindow(a,"FoxTerm",nullptr,nullptr,DECOR_ALL,settingInt(a,"x",80),settingInt(a,"y",80),settingInt(a,"w",900),settingInt(a,"h",620)),launch(l){
  contents=new FXVerticalFrame(this,FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_FILL_Y)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0);
  tabrow=new FXHorizontalFrame(contents,FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0,0,0);
  tabbar=new TerminalTabBar(tabrow,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,2,2,2,2);
  newbutton=new FXButton(tabrow,"+",nullptr,this,ID_NEW,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  profilemenu=new FXMenuPane(this);
  new FXMenuCommand(profilemenu,"Default profile",nullptr,this,ID_NEW);
  profilebutton=new FXMenuButton(tabrow,"Default",nullptr,profilemenu,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|MENUBUTTON_DOWN);
  mainmenu=new FXMenuPane(this);
  new FXMenuCommand(mainmenu,"New &Window",nullptr,this,ID_NEW);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&Copy\tCtl-Shift-C",nullptr,this,ID_COPY);
  new FXMenuCommand(mainmenu,"&Paste\tCtl-Shift-V",nullptr,this,ID_PASTE);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&About",nullptr,this,ID_ABOUT);
  new FXMenuCommand(mainmenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  menubutton=new FXMenuButton(tabrow,"Menu",nullptr,mainmenu,FRAME_RAISED|FRAME_THICK|LAYOUT_RIGHT|LAYOUT_CENTER_Y|MENUBUTTON_DOWN);
  terminal=new FXTerminal(contents,this,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0,2,2,2,2);
  status=new FXLabel(contents,"Starting...",nullptr,FXuint(JUSTIFY_LEFT)|FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,4,4,1,1);
  }


// Create and start
void TerminalWindow::create(){
  FXString error;
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  raise();
  if(!terminal->start(launch,error)){
    FXMessageBox::error(this,MBOX_OK,"Terminal Error","%s",error.text());
    status->setText("Could not start the shell");
    return;
    }
  tabbar->setTitle(terminal->getTitle().empty() ? "Terminal" : terminal->getTitle());
  setTitle(FXString("FoxTerm - ")+tabbar->getTitle());
  terminal->setFocus();
  status->setText(launch.shell);
  }


// Update the title
long TerminalWindow::onCmdTitle(FXObject*,FXSelector,void*){
  FXString tabtitle=terminal->getTitle();
  if(tabtitle.empty()) tabtitle="Terminal";
  tabbar->setTitle(tabtitle);
  setTitle(FXString("FoxTerm - ")+tabtitle);
  return 1;
  }


// Bell
long TerminalWindow::onCmdBell(FXObject*,FXSelector,void*){
  getApp()->beep();
  return 1;
  }


// Process exit
long TerminalWindow::onCmdExited(FXObject*,FXSelector,void*){
  FXString text;
  text.format("Process exited with code %d",terminal->getExitCode());
  status->setText(text);
  tabbar->setTitle(FXString(tabbar->getTitle()+" [exited]"));
  return 1;
  }


// Clipboard request from OSC 52
long TerminalWindow::onCmdClipboard(FXObject*,FXSelector,void*){
  if(!terminal->takeClipboardRequest().empty()) status->setText("Clipboard request blocked");
  return 1;
  }


// New window
long TerminalWindow::onCmdNewWindow(FXObject*,FXSelector,void*){
  TerminalWindow *window=new TerminalWindow(getApp(),launch);
  window->create();
  return 1;
  }


// Copy
long TerminalWindow::onCmdCopy(FXObject*,FXSelector,void*){
  terminal->copySelection();
  return 1;
  }


// Paste
long TerminalWindow::onCmdPaste(FXObject*,FXSelector,void*){
  terminal->pasteClipboard();
  return 1;
  }


// Save geometry
void TerminalWindow::saveGeometry(){
  FXRegistry& reg=getApp()->reg();
  if(!isMaximized()){
    reg.writeIntEntry("WINDOW","x",getX());
    reg.writeIntEntry("WINDOW","y",getY());
    reg.writeIntEntry("WINDOW","w",getWidth());
    reg.writeIntEntry("WINDOW","h",getHeight());
    reg.write();
    }
  }


// Quit
long TerminalWindow::onCmdQuit(FXObject*,FXSelector,void*){
  saveGeometry();
  terminal->terminate();
  getApp()->exit(0);
  return 1;
  }


// About
long TerminalWindow::onCmdAbout(FXObject*,FXSelector,void*){
  FXMessageBox::information(this,MBOX_OK,"About FoxTerm","FoxTerm\nA FOX terminal widget application.\nCopyright (C) 2026 Jeroen van der Zijp");
  return 1;
  }


// Destructor
TerminalWindow::~TerminalWindow(){
  delete profilemenu;
  delete mainmenu;
  }

