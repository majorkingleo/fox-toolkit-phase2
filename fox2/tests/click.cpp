/********************************************************************************
*                                                                               *
*                         C l i c k   T e s t                                   *
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
  Click test.

  One button fills the whole window and quits the application when it is
  clicked, so a headless run can press the pointer anywhere inside the window
  and expect the program to end; see scripts/gate-wayland-click.sh.

  The press and the release are reported on stdout, which tells a gate or a
  person whether the pointer reached the button at all, and whether the grab
  took the release back to the same widget.
*/

class ClickWindow : public FXMainWindow {
  FXDECLARE(ClickWindow)
protected:
  FXButton* button;
public:
  enum { ID_QUIT = FXMainWindow::ID_LAST, ID_LAST };
public:
  ClickWindow(){ button=nullptr; }
  ClickWindow(FXApp* a);
  virtual void create();
  long onPress(FXObject*,FXSelector,void*);
  long onRelease(FXObject*,FXSelector,void*);
  long onQuit(FXObject*,FXSelector,void*);
  };


FXDEFMAP(ClickWindow) ClickWindowMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,ClickWindow::ID_QUIT,ClickWindow::onPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,ClickWindow::ID_QUIT,ClickWindow::onRelease),
  FXMAPFUNC(SEL_COMMAND,ClickWindow::ID_QUIT,ClickWindow::onQuit)
  };

FXIMPLEMENT(ClickWindow,FXMainWindow,ClickWindowMap,ARRAYNUMBER(ClickWindowMap))


// Make the window
ClickWindow::ClickWindow(FXApp* a):FXMainWindow(a,"Click",nullptr,nullptr,DECOR_ALL,0,0,320,200){
  button=new FXButton(this,"&Press me",nullptr,this,ID_QUIT,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  }


// Create it
void ClickWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


// The pointer went down on the button; let the button see it as well
long ClickWindow::onPress(FXObject*,FXSelector,void*){
  fxmessage("press\n");
  return 0;
  }


// The pointer came back up on the button; the button turns this into a command
long ClickWindow::onRelease(FXObject*,FXSelector,void*){
  fxmessage("release\n");
  return 0;
  }


// Quit
long ClickWindow::onQuit(FXObject*,FXSelector,void*){
  fxmessage("command from the button\n");
  getApp()->exit(0);
  return 1;
  }


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("ClickTest","FoxTest");
  application.init(argc,argv);

  new ClickWindow(&application);

  application.create();

  return application.run();
  }
