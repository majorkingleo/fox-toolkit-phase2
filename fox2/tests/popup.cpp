/********************************************************************************
*                                                                               *
*                         P o p u p   T e s t                                   *
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
#include "fx.h"
#include "testargs.h"

/*
  Popup test.

  Opens a menu pane over a main window on a timer and closes it again, so that
  a headless run commits a frame of the popup surface as well as one of the
  main window; see scripts/gate-wayland-popup.sh.

  - On X11 and Windows the pane is an override redirect window.
  - On Wayland it is an xdg_popup hanging off the surface of the main window,
    sized and placed by a positioner, and the compositor dismisses it.
*/

class PopupDriver : public FXObject {
  FXDECLARE(PopupDriver)
protected:
  FXApp* app;
  FXMainWindow* shell;
  FXMenuPane* pane;
public:
  enum { ID_OPEN = FXApp::ID_LAST, ID_STOP, ID_LAST };
public:
  PopupDriver(){
    app=nullptr;
    shell=nullptr;
    pane=nullptr;
    }
  void setup(FXApp* a,FXMainWindow* m,FXMenuPane* p){ app=a; shell=m; pane=p; }
  long onOpen(FXObject*,FXSelector,void*){
    fxmessage("opening the pane\n");
    pane->popup(shell,60,40);
    return 1;
    }
  long onStop(FXObject*,FXSelector,void*){
    fxmessage("closing the pane\n");
    if(pane->shown()) pane->hide();
    app->exit(0);
    return 1;
    }
  };


FXDEFMAP(PopupDriver) PopupDriverMap[]={
  FXMAPFUNC(SEL_TIMEOUT,PopupDriver::ID_OPEN,PopupDriver::onOpen),
  FXMAPFUNC(SEL_TIMEOUT,PopupDriver::ID_STOP,PopupDriver::onStop)
  };

FXIMPLEMENT(PopupDriver,FXObject,PopupDriverMap,ARRAYNUMBER(PopupDriverMap))


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("PopupTest","FoxTest");
  application.init(argc,argv);

  FXMainWindow *main=new FXMainWindow(&application,"Popup",nullptr,nullptr,DECOR_ALL);
  new FXButton(main,"&Open",nullptr,nullptr,0,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X);

  FXMenuPane *pane=new FXMenuPane(main);
  new FXButton(pane,"First entry",nullptr,nullptr,0,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X);
  new FXButton(pane,"Second entry",nullptr,nullptr,0,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X);

  PopupDriver driver;
  driver.setup(&application,main,pane);

  application.create();
  main->show(PLACEMENT_SCREEN);

  application.addTimeout(&driver,FXSEL(SEL_TIMEOUT,PopupDriver::ID_OPEN),700000000);
  application.addTimeout(&driver,FXSEL(SEL_TIMEOUT,PopupDriver::ID_STOP),2500000000LL);

  return application.run();
  }
