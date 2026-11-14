/********************************************************************************
*                                                                               *
*                         M e n u   T e s t                                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
********************************************************************************/
#include "fx.h"
#include "testargs.h"

/*
  Menu test.

  A menu with a submenu, opened from a button, so that a headless run can open
  both, pick an entry and see the application quit; see
  scripts/gate-wayland-menu.sh.

  On Wayland each menu is an xdg_popup with an explicit grab, and the submenu is
  a popup hanging off that popup, which is the case the protocol is strict
  about: a grabbing popup may only have a toplevel or another grabbing popup as
  its parent, and popups are destroyed from the top down.

  The entries report themselves on stdout, which tells the gate how far the
  click got.  A menu bar would need FXMenuBar, which inherits FXToolBar and with
  it the docking classes, which the library does not have yet.

  With MENU_SELFTEST set the program drives itself - it posts the menu, pops it
  down again and quits - so the same binary can be run without a pointer on any
  platform, which is how its teardown was compared with X11.
*/

class MenuWindow : public FXMainWindow {
  FXDECLARE(MenuWindow)
protected:
  FXButton*    opener;
  FXMenuPane*  filemenu;
  FXMenuPane*  recentmenu;
public:
  enum {
    ID_OPEN_MENU = FXMainWindow::ID_LAST,
    ID_NEW,
    ID_OPEN,
    ID_RECENT_ONE,
    ID_RECENT_TWO,
    ID_RECENT_THREE,
    ID_QUIT,
    ID_SELFTEST_POST,
    ID_SELFTEST_UNPOST,
    ID_SELFTEST_QUIT,
    ID_LAST
    };
public:
  MenuWindow(){ opener=nullptr; filemenu=nullptr; recentmenu=nullptr; }
  MenuWindow(FXApp* a);
  virtual void create();
  long onOpenMenu(FXObject*,FXSelector,void*);
  long onCommand(FXObject*,FXSelector,void*);
  long onSelfPost(FXObject*,FXSelector,void*);
  long onSelfUnpost(FXObject*,FXSelector,void*);
  long onSelfQuit(FXObject*,FXSelector,void*);
  };


FXDEFMAP(MenuWindow) MenuWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_OPEN_MENU,MenuWindow::onOpenMenu),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_NEW,MenuWindow::onCommand),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_OPEN,MenuWindow::onCommand),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_RECENT_ONE,MenuWindow::onCommand),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_RECENT_TWO,MenuWindow::onCommand),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_RECENT_THREE,MenuWindow::onCommand),
  FXMAPFUNC(SEL_COMMAND,MenuWindow::ID_QUIT,MenuWindow::onCommand),
  FXMAPFUNC(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_POST,MenuWindow::onSelfPost),
  FXMAPFUNC(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_UNPOST,MenuWindow::onSelfUnpost),
  FXMAPFUNC(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_QUIT,MenuWindow::onSelfQuit)
  };

FXIMPLEMENT(MenuWindow,FXMainWindow,MenuWindowMap,ARRAYNUMBER(MenuWindowMap))


// Make the window
MenuWindow::MenuWindow(FXApp* a):FXMainWindow(a,"Menus",nullptr,nullptr,DECOR_ALL,0,0,240,160){

  // A button in the top left corner opens the menu
  opener=new FXButton(this,"&File",nullptr,this,ID_OPEN_MENU,FRAME_RAISED|FRAME_THICK|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,8,8,80,26);

  // The menu, with a cascade that needs a popup below a popup
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&New",nullptr,this,ID_NEW);
  new FXMenuCommand(filemenu,"&Open",nullptr,this,ID_OPEN);
  new FXMenuSeparator(filemenu);
  recentmenu=new FXMenuPane(filemenu);
  new FXMenuCommand(recentmenu,"&One",nullptr,this,ID_RECENT_ONE);
  new FXMenuCommand(recentmenu,"&Two",nullptr,this,ID_RECENT_TWO);
  new FXMenuCommand(recentmenu,"&Three",nullptr,this,ID_RECENT_THREE);
  new FXMenuCascade(filemenu,"&Recent",nullptr,recentmenu);
  new FXMenuSeparator(filemenu);
  new FXMenuCommand(filemenu,"&Quit",nullptr,this,ID_QUIT);

  new FXLabel(this,"Menu test",nullptr,LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
  }


// Create it
void MenuWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


// The file button was pressed; drop the menu from it.  FXPopup::popup() wants
// the place in the coordinates of the root, not of the button.  With no grab
// owner the popup takes the grab itself, which is what makes a click outside it
// come back to the popup and dismiss it.
long MenuWindow::onOpenMenu(FXObject*,FXSelector,void*){
  FXint rx,ry;
  opener->translateCoordinatesTo(rx,ry,getRoot(),0,opener->getHeight());
  fxmessage("menu open at %d %d\n",rx,ry);
  filemenu->popup(nullptr,rx,ry);
  return 1;
  }


// An entry was picked
long MenuWindow::onCommand(FXObject* sender,FXSelector sel,void*){
  switch(FXSELID(sel)){
    case ID_NEW: fxmessage("command new\n"); break;
    case ID_OPEN: fxmessage("command open\n"); break;
    case ID_RECENT_ONE: fxmessage("command one\n"); break;
    case ID_RECENT_TWO: fxmessage("command two\n"); break;
    case ID_RECENT_THREE: fxmessage("command three\n"); break;
    case ID_QUIT: fxmessage("command quit\n"); break;
    }
  (void)sender;
  getApp()->exit(0);
  return 1;
  }


// Self test: post the menu
long MenuWindow::onSelfPost(FXObject*,FXSelector,void*){
  onOpenMenu(nullptr,0,nullptr);
  return 1;
  }


// Self test: pop it down again
long MenuWindow::onSelfUnpost(FXObject*,FXSelector,void*){
  fxmessage("self test unpost\n");
  filemenu->popdown();
  return 1;
  }


// Self test: quit
long MenuWindow::onSelfQuit(FXObject*,FXSelector,void*){
  fxmessage("self test quit\n");
  getApp()->exit(0);
  return 1;
  }


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("MenuTest","FoxTest");
  application.init(argc,argv);

  MenuWindow* window=new MenuWindow(&application);

  application.create();

  if(getenv("MENU_SELFTEST")){
    application.addTimeout(window,FXSEL(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_POST),700000000);
    application.addTimeout(window,FXSEL(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_UNPOST),1400000000);
    application.addTimeout(window,FXSEL(SEL_TIMEOUT,MenuWindow::ID_SELFTEST_QUIT),2000000000);
    }

  return application.run();
  }
