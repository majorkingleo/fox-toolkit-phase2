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
  FXint value=app->reg().readIntEntry("WINDOW",key,fallback);
  if(value<-10000 || value>100000) return fallback;
  if(strcmp(key,"w")==0 && value<200) return fallback;
  if(strcmp(key,"h")==0 && value<100) return fallback;
  return value;
  }


// Live terminal windows of this process, so quit and the last-window check
// never depend on the order in which the windows are created or destroyed
static std::vector<TerminalWindow*> terminalwindows;


FXDEFMAP(TerminalWindow) TerminalWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_TITLE,TerminalWindow::onCmdTitle),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_BELL,TerminalWindow::onCmdBell),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_EXITED,TerminalWindow::onCmdExited),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLIPBOARD,TerminalWindow::onCmdClipboard),
  FXMAPFUNC(SEL_COMMAND,FXTerminal::ID_TITLE,TerminalWindow::onCmdTitle),
  FXMAPFUNC(SEL_COMMAND,FXTerminal::ID_BELL,TerminalWindow::onCmdBell),
  FXMAPFUNC(SEL_COMMAND,FXTerminal::ID_EXITED,TerminalWindow::onCmdExited),
  FXMAPFUNC(SEL_COMMAND,FXTerminal::ID_CLIPBOARD,TerminalWindow::onCmdClipboard),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEW_TAB,TerminalWindow::onCmdNewTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEW_WINDOW,TerminalWindow::onCmdNewWindow),
  FXMAPFUNC(SEL_CHORE,TerminalWindow::ID_CLOSE_ON_EXIT,TerminalWindow::onChoreCloseTab),
  FXMAPFUNC(SEL_PAINT,TerminalWindow::ID_RULE,TerminalWindow::onPaintRule),
  FXMAPFUNC(SEL_TIMEOUT,TerminalWindow::ID_STATUSHIDE,TerminalWindow::onStatusTimeout),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLOSE_CURRENT,TerminalWindow::onCmdCloseCurrent),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_DETACH,TerminalWindow::onCmdDetach),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_FIND,TerminalWindow::onCmdFind),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_FIND_NEXT,TerminalWindow::onCmdFindNext),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_FIND_PREV,TerminalWindow::onCmdFindPrev),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_FIND_CLOSE,TerminalWindow::onCmdFindClose),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_NEXT_TAB,TerminalWindow::onCmdNextTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_PREV_TAB,TerminalWindow::onCmdPrevTab),
  FXMAPFUNCS(SEL_COMMAND,TerminalWindow::ID_PROFILE0,TerminalWindow::ID_PROFILE_LAST,TerminalWindow::onCmdProfile),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_SETTINGS,TerminalWindow::onCmdSettings),
  FXMAPFUNCS(SEL_COMMAND,TerminalWindow::ID_OPACITY0,TerminalWindow::ID_OPACITY_LAST,TerminalWindow::onCmdOpacity),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_COPY,TerminalWindow::onCmdCopy),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_PASTE,TerminalWindow::onCmdPaste),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_SELECT_ALL,TerminalWindow::onCmdSelectAll),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLEAR_SCROLLBACK,TerminalWindow::onCmdClearScrollback),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_RESET_TERMINAL,TerminalWindow::onCmdResetTerminal),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,TerminalWindow::onCmdContextMenu),
  FXMAPFUNC(SEL_COMMAND,TerminalTabBar::ID_CONTEXT_TAB,TerminalWindow::onCmdTabMenu),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_DUP_TAB,TerminalWindow::onCmdDupTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_RENAME_TAB,TerminalWindow::onCmdRenameTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_TABMENU_DETACH,TerminalWindow::onCmdDetachTabMenu),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_TABMENU_CLOSE,TerminalWindow::onCmdCloseMenuTab),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_CLOSE_OTHERS,TerminalWindow::onCmdCloseOthers),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_COPY,TerminalWindow::onUpdCopy),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_PASTE,TerminalWindow::onUpdPaste),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_SELECT_ALL,TerminalWindow::onUpdHasTab),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_FIND,TerminalWindow::onUpdHasTab),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_DETACH,TerminalWindow::onUpdHasTab),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_CLOSE_CURRENT,TerminalWindow::onUpdHasTab),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_RESET_TERMINAL,TerminalWindow::onUpdHasTab),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_CLEAR_SCROLLBACK,TerminalWindow::onUpdClearScrollback),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_DUP_TAB,TerminalWindow::onUpdTabMenu),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_RENAME_TAB,TerminalWindow::onUpdTabMenu),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_TABMENU_DETACH,TerminalWindow::onUpdTabMenu),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_TABMENU_CLOSE,TerminalWindow::onUpdTabMenu),
  FXMAPFUNC(SEL_UPDATE,TerminalWindow::ID_CLOSE_OTHERS,TerminalWindow::onUpdCloseOthers),
  FXMAPFUNCS(SEL_UPDATE,TerminalWindow::ID_ENCODING0,TerminalWindow::ID_ENCODING_LAST,TerminalWindow::onUpdEncoding),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_ABOUT,TerminalWindow::onCmdAbout),
  FXMAPFUNC(SEL_COMMAND,TerminalWindow::ID_QUIT,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL,TerminalWindow::ID_QUIT,TerminalWindow::onSignalQuit),
  FXMAPFUNC(SEL_CLOSE,0,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_CLOSE,TerminalWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,TerminalTabBar::ID_TAB,TerminalWindow::onCmdTab),
  FXMAPFUNC(SEL_COMMAND,TerminalTabBar::ID_CLOSE_TAB,TerminalWindow::onCmdCloseTab),
  };

FXIMPLEMENT(TerminalWindow,FXMainWindow,TerminalWindowMap,ARRAYNUMBER(TerminalWindowMap))


// Construct
TerminalWindow::TerminalWindow(FXApp* a,const FXTerminalLaunch& l,const FXString& active,FXbool withtab):FXMainWindow(a,"FoxTerm",nullptr,nullptr,DECOR_ALL,settingInt(a,"x",80),settingInt(a,"y",80),settingInt(a,"w",900),settingInt(a,"h",620)),launch(l),activeprofile(active),current(-1),contexttab(-1),autostart(withtab){
  contents=new FXVerticalFrame(this,FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_FILL_Y)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0,0,0);
  tabrow=new FXHorizontalFrame(contents,FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,0,0,0,0,0,0);
  tabbar=new TerminalTabBar(tabrow,this,0,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,2,2,2,2);
  newbutton=new FXButton(tabrow,"+",nullptr,this,ID_NEW_TAB,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  profiles.load(a->reg());
  if(activeprofile.empty() || profiles.find(activeprofile)<0) activeprofile=profiles.getActive();
  profilemenu=nullptr;
  // ICON_BEFORE_TEXT puts the drop-down arrow at the left of the label; without
  // it the arrow and the text are both centred and overlap
  profilebutton=new FXMenuButton(tabrow,"Default",nullptr,nullptr,JUSTIFY_NORMAL|ICON_BEFORE_TEXT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|MENUBUTTON_DOWN);
  rebuildProfileMenu();
  profilebutton->setText(profiles.at(profiles.find(activeprofile)).name);
  mainmenu=new FXMenuPane(this);
  new FXMenuCommand(mainmenu,"New &Tab\tCtl-Shift-T",nullptr,this,ID_NEW_TAB);
  new FXMenuCommand(mainmenu,"New &Window\tCtl-Shift-N",nullptr,this,ID_NEW_WINDOW);
  new FXMenuCommand(mainmenu,"&Close Tab\tCtl-Shift-W",nullptr,this,ID_CLOSE_CURRENT);
  new FXMenuCommand(mainmenu,"&Detach Tab\tCtl-Shift-M",nullptr,this,ID_DETACH);
  new FXMenuCommand(mainmenu,"&Find...\tCtl-Shift-F",nullptr,this,ID_FIND);
  new FXMenuCommand(mainmenu,"&Settings...",nullptr,this,ID_SETTINGS);
  {
    opacitymenu=new FXMenuPane(this);
    new FXMenuCommand(opacitymenu,"100%",nullptr,this,ID_OPACITY0);
    new FXMenuCommand(opacitymenu,"85%",nullptr,this,ID_OPACITY0+1);
    new FXMenuCommand(opacitymenu,"70%",nullptr,this,ID_OPACITY0+2);
    new FXMenuCommand(opacitymenu,"50%",nullptr,this,ID_OPACITY0+3);
    new FXMenuCommand(opacitymenu,"0%",nullptr,this,ID_OPACITY0+4);
    new FXMenuCascade(mainmenu,"Opacity",nullptr,opacitymenu);
    }
  new FXMenuCommand(mainmenu,"Next Tab\tCtl-Tab",nullptr,this,ID_NEXT_TAB);
  new FXMenuCommand(mainmenu,"Previous Tab\tCtl-Shift-Tab",nullptr,this,ID_PREV_TAB);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&Copy\tCtl-Shift-C",nullptr,this,ID_COPY);
  new FXMenuCommand(mainmenu,"&Paste\tCtl-Shift-V",nullptr,this,ID_PASTE);
  new FXMenuSeparator(mainmenu);
  new FXMenuCommand(mainmenu,"&About",nullptr,this,ID_ABOUT);
  new FXMenuCommand(mainmenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  menubutton=new FXMenuButton(tabrow,"Menu",nullptr,mainmenu,FXuint(JUSTIFY_NORMAL)|FXuint(ICON_BEFORE_TEXT)|FXuint(FRAME_RAISED)|FXuint(FRAME_THICK)|FXuint(LAYOUT_RIGHT)|FXuint(LAYOUT_CENTER_Y)|FXuint(MENUBUTTON_DOWN));

  // Context menu of the terminal view (requirements R19).  The entries that
  // cannot act are disabled through SEL_UPDATE handlers, never hidden.
  contextmenu=new FXMenuPane(this);
  new FXMenuCommand(contextmenu,"&Copy\tCtl-Shift-C",nullptr,this,ID_COPY);
  new FXMenuCommand(contextmenu,"&Paste\tCtl-Shift-V",nullptr,this,ID_PASTE);
  new FXMenuCommand(contextmenu,"Select &All\tCtl-Shift-A",nullptr,this,ID_SELECT_ALL);
  new FXMenuCommand(contextmenu,"&Find...\tCtl-Shift-F",nullptr,this,ID_FIND);
  new FXMenuSeparator(contextmenu);
  new FXMenuCommand(contextmenu,"Clear Scroll&back",nullptr,this,ID_CLEAR_SCROLLBACK);
  new FXMenuCommand(contextmenu,"&Reset Terminal",nullptr,this,ID_RESET_TERMINAL);
  {
    FXMenuRadio *utf8;
    encodingmenu=new FXMenuPane(this);
    utf8=new FXMenuRadio(encodingmenu,"UTF-8",this,ID_ENCODING0);
    utf8->setCheck(true);
    new FXMenuCascade(contextmenu,"Character &Encoding",nullptr,encodingmenu);
    }
  new FXMenuSeparator(contextmenu);
  new FXMenuCommand(contextmenu,"New &Tab\tCtl-Shift-T",nullptr,this,ID_NEW_TAB);
  new FXMenuCommand(contextmenu,"&Detach Tab\tCtl-Shift-M",nullptr,this,ID_DETACH);
  new FXMenuCommand(contextmenu,"&Close Tab\tCtl-Shift-W",nullptr,this,ID_CLOSE_CURRENT);

  // Context menu of the tab strip (requirements R19)
  tabmenu=new FXMenuPane(this);
  new FXMenuCommand(tabmenu,"&Duplicate",nullptr,this,ID_DUP_TAB);
  new FXMenuCommand(tabmenu,"&Rename...",nullptr,this,ID_RENAME_TAB);
  new FXMenuCommand(tabmenu,"D&etach",nullptr,this,ID_TABMENU_DETACH);
  new FXMenuSeparator(tabmenu);
  new FXMenuCommand(tabmenu,"&Close",nullptr,this,ID_TABMENU_CLOSE);
  new FXMenuCommand(tabmenu,"Close &Others",nullptr,this,ID_CLOSE_OTHERS);
  rule=new FXCanvas(contents,this,ID_RULE,LAYOUT_FILL_X|FRAME_NONE,0,0,0,1);
  switcher=new FXSwitcher(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE,0,0,0,0,0,0,0,0);
  findbar=new FXHorizontalFrame(contents,FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,2,2,1,1);
  new FXLabel(findbar,"Find:",nullptr,LAYOUT_CENTER_Y);
  findfield=new FXTextField(findbar,20,this,ID_FIND,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  new FXButton(findbar,"Prev",nullptr,this,ID_FIND_PREV,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(findbar,"Next",nullptr,this,ID_FIND_NEXT,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  new FXButton(findbar,"Close",nullptr,this,ID_FIND_CLOSE,FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y);
  findstatus=new FXLabel(findbar,"",nullptr,LAYOUT_CENTER_Y);
  findbar->hide();
  status=new FXLabel(contents,"",nullptr,FXuint(JUSTIFY_LEFT)|FXuint(LAYOUT_FILL_X)|FXuint(FRAME_NONE),0,0,0,0,6,6,3,3);
  status->hide();
  opacity=FXCLAMP(0,a->reg().readIntEntry("APPEARANCE","opacity",100),100);
  terminalwindows.push_back(this);
  }


// Create and add first tab
void TerminalWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  raise();
  if(isOpacitySupported()) setOpacity(opacity);
  if(autostart && tabs.empty()){
    if(!addTab(launch)){
      showStatus("Could not start the shell");
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


// Add one tab and start it.  The launch comes by value: the vector of tabs
// reallocates in the middle of this function and a reference into it would
// dangle, which is what a duplicate of a tab used to crash on.
FXbool TerminalWindow::addTab(FXTerminalLaunch l,FXString profileid){
  FXString error;
  TerminalTab tab;
  FXTerminal *view=new FXTerminal(switcher,this,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  view->create();
  tab.view=view;
  tab.launch=l;
  tab.profileid=profileid.empty() ? activeprofile : profileid;
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
    showStatus(FXString("Could not start the shell: ")+error);
    FXMessageBox::error(this,MBOX_OK,"Terminal Error","%s",error.text());
    closeTab((FXint)tabs.size()-1,true);
    return false;
    }
  return true;
  }


// Return the profile a tab was created from
const TerminalProfile& TerminalWindow::tabProfile(FXint index) const {
  FXint p=profiles.find(tabs[index].profileid);
  if(p<0) p=profiles.find(activeprofile);
  if(p<0) p=profiles.find(profiles.getActive());
  if(p<0) p=0;
  return profiles.at(p);
  }


// Ask before closing a tab whose child is still running
FXbool TerminalWindow::confirmCloseTab(FXint index){
  FXString prompt;
  if(!tabProfile(index).confirmClose) return true;
  if(!tabs[index].view->isRunning()) return true;
  prompt.format("The session \"%s\" is still running.\nClose the tab anyway?",tabs[index].title.text());
  return FXMessageBox::question(this,MBOX_OK_CANCEL,"Close Tab","%s",prompt.text())==MBOX_CLICKED_OK;
  }


// Select a tab
void TerminalWindow::selectTab(FXint index){
  if(index<0 || index>=(FXint)tabs.size()) return;
  current=index;
  switcher->setCurrent(index);
  tabbar->setCurrent(index);
  switcher->layout();
  tabs[index].view->layout();
  for(FXint i=0; i<(FXint)tabs.size(); i++) tabs[i].view->setDefault(i==index);
  tabs[index].view->setFocus();
  setTitle(FXString("FoxTerm - ")+tabs[index].title);
  }


// Close one tab
void TerminalWindow::closeTab(FXint index,FXbool force){
  FXbool islast;
  TerminalTab tab;
  if(index<0 || index>=(FXint)tabs.size()) return;
  if(!force && !confirmCloseTab(index)) return;
  tab=tabs[index];
  tabs.erase(tabs.begin()+index);
  tabbar->removeTitle(index);
  delete tab.view;
  if(tabs.empty()){
    saveGeometry();
    islast=true;
    for(FXint i=0; i<(FXint)terminalwindows.size(); i++){
      if(terminalwindows[i]!=this && !terminalwindows[i]->tabs.empty()) islast=false;
      }
    if(islast) getApp()->exit(0);
    else getApp()->addChore(this,FXWindow::ID_DELETE);
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
  if(index<0) return 1;
  tabs[index].exited=true;
  tabs[index].exitcode=tabs[index].view->getExitCode();

  // Close-on-exit policy: close the tab, or keep the final screen with an
  // [exited] suffix.  The widget is still inside its own read callback here,
  // so the tab must not be destroyed before the callback returns.
  if(tabProfile(index).closeOnExit){
    getApp()->addChore(this,FXSEL(SEL_CHORE,ID_CLOSE_ON_EXIT),tabs[index].view);
    return 1;
    }
  if(tabs[index].title.find(" [exited]")<0) tabs[index].title+=" [exited]";
  updateTabTitle(index);
  FXString text;
  text.format("Process exited with code %d",tabs[index].exitcode);
  showStatus(text);
  return 1;
  }


// Close a tab whose child exited, once the widget callback has returned
long TerminalWindow::onChoreCloseTab(FXObject*,FXSelector,void* ptr){
  FXint index=findTab((FXObject*)ptr);
  if(index>=0) closeTab(index,true);
  return 1;
  }


// Clipboard request
long TerminalWindow::onCmdClipboard(FXObject* sender,FXSelector,void*){
  FXint index=findTab(sender);
  if(index>=0 && !tabs[index].view->takeClipboardRequest().empty()){
    showStatus("Clipboard request blocked");
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
  TerminalWindow *window=new TerminalWindow(getApp(),launch,activeprofile);
  window->create();
  return 1;
  }


// Detach active tab into a new window
void TerminalWindow::detachTab(FXint index){
  TerminalTab tab;
  TerminalWindow *window;
  if(index<0 || index>=(FXint)tabs.size()) return;
  tab=tabs[index];
  tabs.erase(tabs.begin()+index);
  tabbar->removeTitle(index);
  window=new TerminalWindow(getApp(),tab.launch,activeprofile,false);
  window->create();
  window->move(FXMAX(0,getX()+24),FXMAX(0,getY()+24));
  tab.view->reparent(window->switcher);
  window->tabs.push_back(tab);
  window->tabbar->addTitle(tab.title);
  window->selectTab((FXint)window->tabs.size()-1);
  window->raise();
  if(tabs.empty()){
    getApp()->addChore(this,FXWindow::ID_DELETE);
    }
  else{
    if(current>=index && current>0) current--;
    selectTab(FXCLAMP(0,current,(FXint)tabs.size()-1));
    }
  }


// Detach active tab
long TerminalWindow::onCmdDetach(FXObject*,FXSelector,void*){
  detachTab(current);
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


// Select the whole retained screen
long TerminalWindow::onCmdSelectAll(FXObject*,FXSelector,void*){
  if(0<=current && current<(FXint)tabs.size()) tabs[current].view->selectAll();
  return 1;
  }


// Drop the scrollback of the active tab
long TerminalWindow::onCmdClearScrollback(FXObject*,FXSelector,void*){
  FXint lines;
  if(0>current || current>=(FXint)tabs.size()) return 1;
  lines=tabs[current].view->getScrollbackCount();
  tabs[current].view->clearScrollback();
  FXString text;
  text.format("Cleared %d scrollback %s",lines,lines==1 ? "line" : "lines");
  showStatus(text);
  return 1;
  }


// Reset the emulator of the active tab; RIS clears the scrollback as well
long TerminalWindow::onCmdResetTerminal(FXObject*,FXSelector,void*){
  if(0>current || current>=(FXint)tabs.size()) return 1;
  tabs[current].view->reset();
  showStatus("Terminal reset");
  return 1;
  }


// The widget forwarded a right press that does not belong to the child; the
// popup opens where the press happened, in root coordinates
long TerminalWindow::onCmdContextMenu(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  contextmenu->popup(nullptr,event->root_x,event->root_y);
  return 1;
  }


// The strip posted its menu; the entries act on the tab it was posted for
long TerminalWindow::onCmdTabMenu(FXObject*,FXSelector,void* ptr){
  FXint x=0;
  FXint y=0;
  FXuint buttons=0;
  contexttab=(FXint)(FXival)ptr;
  getRoot()->getCursorPosition(x,y,buttons);
  tabmenu->popup(nullptr,x,y);
  return 1;
  }


// Duplicate the tab the strip menu was posted for
long TerminalWindow::onCmdDupTab(FXObject*,FXSelector,void*){
  if(contexttab<0 || contexttab>=(FXint)tabs.size()) return 1;
  addTab(tabs[contexttab].launch,tabs[contexttab].profileid);
  return 1;
  }


// Rename the tab the strip menu was posted for
long TerminalWindow::onCmdRenameTab(FXObject*,FXSelector,void*){
  FXString name;
  if(contexttab<0 || contexttab>=(FXint)tabs.size()) return 1;
  if(!FXInputDialog::getString(name,this,"Rename Tab","Tab title:")) return 1;
  if(name.empty()) return 1;
  tabs[contexttab].title=name;
  updateTabTitle(contexttab);
  return 1;
  }


// Detach the tab the strip menu was posted for
long TerminalWindow::onCmdDetachTabMenu(FXObject*,FXSelector,void*){
  if(contexttab<0 || contexttab>=(FXint)tabs.size()) return 1;
  detachTab(contexttab);
  return 1;
  }


// Close the tab the strip menu was posted for
long TerminalWindow::onCmdCloseMenuTab(FXObject*,FXSelector,void*){
  if(contexttab<0 || contexttab>=(FXint)tabs.size()) return 1;
  closeTab(contexttab);
  return 1;
  }


// Close every other tab of this window
long TerminalWindow::onCmdCloseOthers(FXObject*,FXSelector,void*){
  FXint keep=contexttab;
  if(keep<0 || keep>=(FXint)tabs.size()) return 1;
  for(FXint i=(FXint)tabs.size()-1; i>keep; i--) closeTab(i);
  for(FXint i=keep-1; i>=0; i--) closeTab(i);
  if(!tabs.empty()) selectTab(0);
  return 1;
  }


// Enable or disable the widget that sent the update.  A SEL_UPDATE handler has
// to do this itself: the return value only says the update was handled, and
// a menu entry without MENU_AUTOGRAY never grays on it.
static long setEnabled(FXObject* sender,FXObject* self,FXbool enabled){
  sender->handle(self,FXSEL(SEL_COMMAND,enabled ? FXWindow::ID_ENABLE : FXWindow::ID_DISABLE),nullptr);
  return 1;
  }


// Copy is only possible with a selection
long TerminalWindow::onUpdCopy(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=current && current<(FXint)tabs.size() && tabs[current].view->hasSelection());
  }


// Paste is only possible while the child accepts input
long TerminalWindow::onUpdPaste(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=current && current<(FXint)tabs.size() && tabs[current].view->isRunning());
  }


// Entries that need a live tab
long TerminalWindow::onUpdHasTab(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=current && current<(FXint)tabs.size());
  }


// Clear Scrollback is only possible with retained rows
long TerminalWindow::onUpdClearScrollback(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=current && current<(FXint)tabs.size() && tabs[current].view->getScrollbackCount()>0);
  }


// The tab entries need the tab the strip menu was posted for
long TerminalWindow::onUpdTabMenu(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=contexttab && contexttab<(FXint)tabs.size());
  }


// Close Others needs a second tab
long TerminalWindow::onUpdCloseOthers(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,0<=contexttab && contexttab<(FXint)tabs.size() && tabs.size()>1);
  }


// Encoding switching is N2; the submenu shows the active encoding, disabled
long TerminalWindow::onUpdEncoding(FXObject* sender,FXSelector,void*){
  return setEnabled(sender,this,false);
  }


// Save geometry
void TerminalWindow::saveGeometry(){
  FXRegistry& reg=getApp()->reg();
  if(!isMaximized() && !isMinimized()){
    reg.writeIntEntry("WINDOW","x",getX());
    reg.writeIntEntry("WINDOW","y",getY());
    reg.writeIntEntry("WINDOW","w",getWidth());
    reg.writeIntEntry("WINDOW","h",getHeight());
    reg.writeIntEntry("APPEARANCE","opacity",getOpacity());
    profiles.setActive(activeprofile);
    profiles.save(reg);
    reg.write();
    }
  }


// Show the status line; a transient text hides itself again after a while
void TerminalWindow::showStatus(const FXString& text,FXbool transient){
  status->setText(text);
  if(!status->shown()){
    status->show();
    contents->recalc();
    layout();
    }
  getApp()->removeTimeout(this,ID_STATUSHIDE);
  if(transient) getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_STATUSHIDE),FXTime(4000000000LL));
  }


// Hide the status line
void TerminalWindow::hideStatus(){
  getApp()->removeTimeout(this,ID_STATUSHIDE);
  if(status->shown()){
    status->hide();
    contents->recalc();
    layout();
    }
  }


// Hide the transient status line
long TerminalWindow::onStatusTimeout(FXObject*,FXSelector,void*){
  hideStatus();
  return 1;
  }


// Paint the hairline between the tab strip and the grid; the theme decides
// the color, so a live theme change needs no extra bookkeeping
long TerminalWindow::onPaintRule(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(rule,event);
  dc.setForeground(getApp()->getTheme()->color(ROLE_BORDER));
  dc.fillRectangle(event->rect.x,event->rect.y,event->rect.w,event->rect.h);
  return 1;
  }


// Ask once for every running session of the process
FXbool TerminalWindow::confirmQuit(){
  FXint running=0;
  FXint asked=0;
  for(FXint w=0; w<(FXint)terminalwindows.size(); w++){
    TerminalWindow *window=terminalwindows[w];
    for(FXint i=0; i<(FXint)window->tabs.size(); i++){
      if(window->tabs[i].view->isRunning()){
        running++;
        if(window->tabProfile(i).confirmClose) asked++;
        }
      }
    }
  if(asked==0) return true;
  FXString prompt;
  prompt.format("%d session%s still running.\nQuit FoxTerm anyway?",running,running==1 ? " is" : "s are");
  return FXMessageBox::question(this,MBOX_QUIT_CANCEL,"Quit FoxTerm","%s",prompt.text())==MBOX_CLICKED_QUIT;
  }


// Terminate every child this window owns
void TerminalWindow::terminateTabs(){
  for(FXint i=0; i<(FXint)tabs.size(); i++) tabs[i].view->terminate();
  }


// Quit
long TerminalWindow::onCmdQuit(FXObject*,FXSelector,void*){
  if(!confirmQuit()) return 1;
  for(FXint w=0; w<(FXint)terminalwindows.size(); w++){
    terminalwindows[w]->saveGeometry();
    terminalwindows[w]->terminateTabs();
    }
  getApp()->exit(0);
  return 1;
  }


// Quit on a termination signal without a dialog
long TerminalWindow::onSignalQuit(FXObject*,FXSelector,void*){
  for(FXint w=0; w<(FXint)terminalwindows.size(); w++){
    terminalwindows[w]->saveGeometry();
    terminalwindows[w]->terminateTabs();
    }
  getApp()->exit(0);
  return 1;
  }


// Show find bar
long TerminalWindow::onCmdFind(FXObject*,FXSelector,void*){
  findbar->show();
  showStatus("Find: type a pattern, Enter for the next match",false);
  recalc();
  findfield->setFocus();
  if(!findfield->getText().empty()) onCmdFindNext(nullptr,0,nullptr);
  return 1;
  }


// Find next
long TerminalWindow::onCmdFindNext(FXObject*,FXSelector,void*){
  FXString pattern;
  FXint count;
  if(0>current || current>=(FXint)tabs.size()) return 1;
  pattern=findfield->getText();
  if(pattern.empty()) return 1;
  if(tabs[current].view->findText(pattern,SEARCH_IGNORECASE)==0){
    findstatus->setText("0 matches");
    return 1;
    }
  tabs[current].view->findNext();
  count=tabs[current].view->getFindCount();
  FXString text;
  text.format("%d matches",count);
  findstatus->setText(text);
  if(findbar->shown()) showStatus(text,false);
  return 1;
  }


// Find previous
long TerminalWindow::onCmdFindPrev(FXObject*,FXSelector,void*){
  FXString pattern;
  FXint count;
  if(0>current || current>=(FXint)tabs.size()) return 1;
  pattern=findfield->getText();
  if(pattern.empty()) return 1;
  if(tabs[current].view->findText(pattern,SEARCH_IGNORECASE)==0){
    findstatus->setText("0 matches");
    return 1;
    }
  tabs[current].view->findPrev();
  count=tabs[current].view->getFindCount();
  FXString text;
  text.format("%d matches",count);
  findstatus->setText(text);
  if(findbar->shown()) showStatus(text,false);
  return 1;
  }


// Hide find bar
long TerminalWindow::onCmdFindClose(FXObject*,FXSelector,void*){
  findbar->hide();
  hideStatus();
  recalc();
  if(0<=current && current<(FXint)tabs.size()) tabs[current].view->setFocus();
  return 1;
  }


// Next tab
long TerminalWindow::onCmdNextTab(FXObject*,FXSelector,void*){
  if(!tabs.empty()) selectTab((current+1)%(FXint)tabs.size());
  return 1;
  }


// Previous tab
long TerminalWindow::onCmdPrevTab(FXObject*,FXSelector,void*){
  if(!tabs.empty()) selectTab((current+(FXint)tabs.size()-1)%(FXint)tabs.size());
  return 1;
  }


// Rebuild the profile drop-down
void TerminalWindow::rebuildProfileMenu(){
  FXint index=profiles.find(activeprofile);
  if(index<0) index=0;
  if(profilemenu) delete profilemenu;
  profilemenu=new FXMenuPane(this);
  for(FXint i=0; i<profiles.count() && i<16; i++){
    new FXMenuCommand(profilemenu,profiles.at(i).name,nullptr,this,ID_PROFILE0+i);
    }
  if(profilebutton) profilebutton->setMenu(profilemenu);
  }


// Create a tab from a profile
long TerminalWindow::onCmdProfile(FXObject*,FXSelector sel,void*){
  FXint index=(FXint)FXSELID(sel)-ID_PROFILE0;
  if(index<0 || index>=profiles.count()) return 1;
  profiles.setActive(profiles.at(index).id);
  activeprofile=profiles.at(index).id;
  launch=profiles.launch(profiles.at(index));
  profilebutton->setText(profiles.at(index).name);
  addTab(launch);
  return 1;
  }


// Edit the active profile
long TerminalWindow::onCmdSettings(FXObject*,FXSelector,void*){
  FXint index=profiles.find(activeprofile);
  TerminalProfile *profile;
  FXDialogBox dialog(this,"Terminal Profile");
  FXVerticalFrame *body;
  FXHorizontalFrame *row;
  FXHorizontalFrame *buttons;
  FXTextField *namefield;
  FXTextField *shellfield;
  FXTextField *argsfield;
  FXTextField *dirfield;
  FXTextField *termfield;
  FXSpinner *scrollback;
  FXComboBox *kindcombo;
  if(index<0) index=0;
  profile=&profiles.at(index);
  body=new FXVerticalFrame(&dialog,FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_FILL_Y),0,0,0,0,8,8,8,8,6,6);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"Name:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  namefield=new FXTextField(row,30,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  namefield->setText(profile->name);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"Shell:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  shellfield=new FXTextField(row,40,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  shellfield->setText(profile->shell);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"Arguments:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  argsfield=new FXTextField(row,40,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  argsfield->setText(profile->arguments);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"Directory:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  dirfield=new FXTextField(row,40,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  dirfield->setText(profile->directory);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"TERM:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  termfield=new FXTextField(row,20,nullptr,0,FXuint(TEXTFIELD_NORMAL)|FXuint(FRAME_SUNKEN)|FXuint(FRAME_THICK)|FXuint(LAYOUT_FILL_X)|FXuint(LAYOUT_CENTER_Y));
  termfield->setText(profile->term);
  kindcombo=new FXComboBox(row,10,nullptr,0,FXuint(COMBOBOX_NORMAL)|FXuint(LAYOUT_CENTER_Y));
  kindcombo->appendItem("xterm");
  kindcombo->appendItem("vt100");
  kindcombo->setCurrentItem(profile->kind==TERMINAL_VT100 ? 1 : 0);
  row=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXLabel(row,"Scrollback:",nullptr,FXuint(LAYOUT_CENTER_Y)|FXuint(JUSTIFY_RIGHT),0,0,110,0);
  scrollback=new FXSpinner(row,7,nullptr,0,FXuint(SPIN_NORMAL)|FXuint(LAYOUT_CENTER_Y));
  scrollback->setRange(0,100000);
  scrollback->setValue(profile->scrollback);
  buttons=new FXHorizontalFrame(body,LAYOUT_FILL_X);
  new FXButton(buttons,"Cancel",nullptr,&dialog,FXDialogBox::ID_CANCEL,FXuint(FRAME_RAISED)|FXuint(FRAME_THICK)|FXuint(LAYOUT_RIGHT));
  new FXButton(buttons,"OK",nullptr,&dialog,FXDialogBox::ID_ACCEPT,FXuint(FRAME_RAISED)|FXuint(FRAME_THICK)|FXuint(LAYOUT_RIGHT));
  if(dialog.execute(PLACEMENT_OWNER)){
    profile->name=namefield->getText();
    profile->shell=shellfield->getText();
    profile->arguments=argsfield->getText();
    profile->directory=dirfield->getText();
    profile->term=termfield->getText();
    profile->kind=kindcombo->getCurrentItem()==1 ? TERMINAL_VT100 : TERMINAL_XTERM;
    if(profile->kind==TERMINAL_VT100) profile->term="vt100";
    else if(profile->term.empty()) profile->term="xterm-256color";
    profile->scrollback=scrollback->getValue();
    profiles.save(getApp()->reg());
    getApp()->reg().write();
    launch=profiles.launch(*profile);
    profilebutton->setText(profile->name);
    rebuildProfileMenu();
    }
  return 1;
  }


// Apply whole-window opacity
long TerminalWindow::onCmdOpacity(FXObject*,FXSelector sel,void*){
  static const FXint values[5]={100,85,70,50,0};
  FXint index=(FXint)FXSELID(sel)-ID_OPACITY0;
  if(index<0 || index>4) return 1;
  if(!isOpacitySupported()){
    showStatus("Opacity is not supported by this display");
    return 1;
    }
  setOpacity(values[index]);
  FXString text;
  text.format("Opacity: %d%%",values[index]);
  showStatus(text);
  return 1;
  }


// About
long TerminalWindow::onCmdAbout(FXObject*,FXSelector,void*){
  FXMessageBox::information(this,MBOX_OK,"About FoxTerm","FoxTerm\nA FOX terminal widget application.\nCopyright (C) 2026 Jeroen van der Zijp");
  return 1;
  }


// Destructor
TerminalWindow::~TerminalWindow(){
  for(FXint i=0; i<(FXint)terminalwindows.size(); i++){
    if(terminalwindows[i]==this){ terminalwindows.erase(terminalwindows.begin()+i); break; }
    }

  // A queued chore or timer must not outlive its target
  getApp()->removeChore(this);
  getApp()->removeTimeout(this);

  // Menu panes are owned, not children; delete every one before the window
  // goes, or their commands keep a target that no longer exists
  delete tabmenu;
  delete encodingmenu;
  delete contextmenu;
  delete opacitymenu;
  delete profilemenu;
  delete mainmenu;
  }

