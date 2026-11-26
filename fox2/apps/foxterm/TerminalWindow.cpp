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

static FXint settingInt(FXApp* app,const FXchar* key,FXint fallback){
  return app->reg().readIntEntry("WINDOW",key,fallback);
  }


FXDEFMAP(TerminalWindow) TerminalWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_TITLE,TerminalWindow::onCmdTitle),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_BELL,TerminalWindow::onCmdBell),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_EXITED,TerminalWindow::onCmdExited),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLIPBOARD,TerminalWindow::onCmdClipboard),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEW_TAB,TerminalWindow::onCmdNewTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEW_WINDOW,TerminalWindow::onCmdNewWindow),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLOSE_CURRENT,TerminalWindow::onCmdCloseCurrent),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_COPY,TerminalWindow::onCmdCopy),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_PASTE,TerminalWindow::onCmdPaste),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_ABOUT,TerminalWindow::onCmdAbout),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_QUIT,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL,TerminalWindow::ID_QUIT,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_CLOSE,0,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_CLOSE,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,TerminalTabBar::ID_TAB,TerminalWindow::onCmdTab),
  FXMAPFUNC(SEL_COMMAND,TerminalTabBar::ID_CLOSE_TAB,TerminalWindow::onCmdCloseTab),
  };

FXIMPLEMENT(TerminalWindow,FXMainWindow,TerminalWindowMap,ARRAYNUMBER(TerminalWindowMap))


// Construct
TerminalWindow::TerminalWindow(FXApp* a,const FXTerminalLaunch& l):FXMainWindow(a,"FoxTerm",nullptr,nullptr,DECOR_ALL,settingInt(a,"x",80),settingInt(a,"y",80),settingInt(a,"w",900),settingInt(a,"h",620)),launch(l),current(-1){
  contents=new FXVerticalFrame(this,FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_FILL_Y)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0);
  tabrow=new FXHorizontalFrame(contents,FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0,0,0);
  tabbar=new TerminalTabBar(tabrow,this,0,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,2,2,2,2);
  newbutton=new FXButton(tabrow,"+",nullptr,this,ID_NEW_TAB,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  profilemenu=new FXMenuPane(this);
  new FXMenuCommand(profilemenu,"Default profile",nullptr,this,ID_NEW_TAB);
  profilebutton=new FXMenuButton(tabrow,"Default",nullptr,profilemenu,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|MENUBUTTON_DOWN);
  mainmenu=new FXMenuPane(this);
  new FXMenuCommand(mainmenu,"New &Tab\tCtl-Shift-T",nullptr,this,ID_NEW_TAB);
  new FXMenuCommand(mainmenu,"New &Window\tCtl-Shift-N",nullptr,this,ID_NEW_WINDOW);
  new FXMenuCommand(mainmenu,"&Close Tab\tCtl-Shift-W",nullptr,this,ID_CLOSE_CURRENT);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&Copy\tCtl-Shift-C",nullptr,this,ID_COPY);
  new FXMenuCommand(mainmenu,"&Paste\tCtl-Shift-V",nullptr,this,ID_PASTE);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&About",nullptr,this,ID_ABOUT);
  new FXMenuCommand(mainmenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  menubutton=new FXMenuButton(tabrow,"Menu",nullptr,mainmenu,FRAME_RAISED|FRAME_THICK|LAYOUT_RIGHT|LAYOUT_CENTER_Y|MENUBUTTON_DOWN);
  switcher=new FXSwitcher(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0,0,0,0,0);
  status=new FXLabel(contents,"Starting...",nullptr,FXuint(JUSTIFY_LEFT)|FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,4,4,1,1);
  }


// Create and add first tab
void TerminalWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  raise();
  if(tabs.empty()){
    if(!addTab(launch)){
      status->setText("Could not start the shell");
      return;
      }
    }
  selectTab(0);
  }


// Find a tab by view
FXint TerminalWindow::findTab(FXObject* view) const {
  for(FXint i=0; i<(FXint)tabs.size(); i++){
    if(tabs[i].view==view) return i;
    }
  return -1;
  }


// Add one tab and start it
FXbool TerminalWindow::addTab(const FXTerminalLaunch& l){
  FXString error;
  TerminalTab tab;
  FXTerminal *view=new FXTerminal(switcher,this,0,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,2,2,2,2);
  view->create();
  tab.view=view;
  tab.launch=l;
  tab.title="Terminal";
  tab.directory=l.directory;
  tab.exited=false;
  tab.exitcode=-1;
  tabs.push_back(tab);
  tabbar->addTitle(tab.title);
  selectTab((FXint)tabs.size()-1);
  switcher->layout();
  view->layout();
  if(!view->start(l,error)){
    FXMessageBox::error(this,MBOX_OK,"Terminal Error","%s",error.text());
    closeTab((FXint)tabs.size()-1);
    return false;
    }
  return true;
  }


// Select a tab
void TerminalWindow::selectTab(FXint index){
  if(index<0 || index>=(FXint)tabs.size()) return;
  current=index;
  switcher->setCurrent(index);
  tabbar->setCurrent(index);
  switcher->layout();
  tabs[index].view->layout();
  tabs[index].view->setFocus();
  setTitle(FXString("FoxTerm - ")+tabs[index].title);
  status->setText(tabs[index].launch.shell);
  }


// Close one tab
void TerminalWindow::closeTab(FXint index){
  if(index<0 || index>=(FXint)tabs.size()) return;
  TerminalTab tab=tabs[index];
  tabs.erase(tabs.begin()+index);
  tabbar->removeTitle(index);
  delete tab.view;
  if(tabs.empty()){
    getApp()->exit(0);
    return;
    }
  if(current>=index && current>0) current--;
  selectTab(FXCLAMP(0,current,(FXint)tabs.size()-1));
  }


// Update one tab title
void TerminalWindow::updateTabTitle(FXint index){
  if(index<0 || index>=(FXint)tabs.size()) return;
  tabbar->setTitle(index,tabs[index].title);
  if(index==current) setTitle(FXString("FoxTerm - ")+tabs[index].title);
  }


// Update a title
long TerminalWindow::onCmdTitle(FXObject* sender,FXSelector,void*){
  FXint index=findTab(sender);
  if(index>=0){
    FXString tabtitle=tabs[index].view->getTitle();
    tabs[index].title=tabtitle.empty() ? "Terminal" : tabtitle;
    updateTabTitle(index);
    }
  return 1;
  }


// Bell
long TerminalWindow::onCmdBell(FXObject*,FXSelector,void*){
  getApp()->beep();
  return 1;
  }


// Process exit
long TerminalWindow::onCmdExited(FXObject* sender,FXSelector,void*){
  FXint index=findTab(sender);
  if(index>=0){
    tabs[index].exited=true;
    tabs[index].exitcode=tabs[index].view->getExitCode();
    if(tabs[index].title.find(" [exited]")<0) tabs[index].title+=" [exited]";
    updateTabTitle(index);
    FXString text;
    text.format("Process exited with code %d",tabs[index].exitcode);
    status->setText(text);
    }
  return 1;
  }


// Clipboard request
long TerminalWindow::onCmdClipboard(FXObject* sender,FXSelector,void*){
  FXint index=findTab(sender);
  if(index>=0 && !tabs[index].view->takeClipboardRequest().empty()){
    status->setText("Clipboard request blocked");
    }
  return 1;
  }


// Select tab
long TerminalWindow::onCmdTab(FXObject*,FXSelector,void* ptr){
  selectTab((FXint)(FXival)ptr);
  return 1;
  }


// Close tab
long TerminalWindow::onCmdCloseTab(FXObject*,FXSelector,void* ptr){
  closeTab((FXint)(FXival)ptr);
  return 1;
  }


// New tab
long TerminalWindow::onCmdNewTab(FXObject*,FXSelector,void*){
  addTab(launch);
  return 1;
  }


// Close active tab
long TerminalWindow::onCmdCloseCurrent(FXObject*,FXSelector,void*){
  closeTab(current);
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
  if(0<=current && current<(FXint)tabs.size()) tabs[current].view->copySelection();
  return 1;
  }


// Paste
long TerminalWindow::onCmdPaste(FXObject*,FXSelector,void*){
  if(0<=current && current<(FXint)tabs.size()) tabs[current].view->pasteClipboard();
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
  for(FXint i=0; i<(FXint)tabs.size(); i++) tabs[i].view->terminate();
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

