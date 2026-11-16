/********************************************************************************
*                                                                               *
*                      I d l e   I n h i b i t   T e s t                       *
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
#include <cstdio>
#if defined(WIN32)
#include <windows.h>
#endif

/*

  A window that asks the system to stay awake, and gives it up again after a
  moment, so that a gate can watch what the platform does:  the compositor gets
  an inhibitor on Wayland, xdg-screensaver is run on X11, and on Windows the
  thread execution state is looked at here.  The program ends by itself.

*/

#define HOLD_TIME 1000000000            // How long it holds off, in ns


// Whether the display is being held on for this thread; only Windows can tell
static const FXchar* heldState(){
#if defined(WIN32)
  EXECUTION_STATE state=SetThreadExecutionState(ES_CONTINUOUS);      // Reads the state, and clears it
  SetThreadExecutionState(state|ES_CONTINUOUS);                      // Puts it back
  return (state&ES_DISPLAY_REQUIRED) ? "display held" : "display free";
#else
  return "not observable here";
#endif
  }


class IdleWindow : public FXMainWindow {
  FXDECLARE(IdleWindow)
public:
  enum { ID_RELEASE = FXMainWindow::ID_LAST, ID_LAST };
public:
  IdleWindow(){}
  IdleWindow(FXApp* a);
  virtual void create();
  long onRelease(FXObject*,FXSelector,void*);
  };


FXDEFMAP(IdleWindow) IdleWindowMap[]={
  FXMAPFUNC(SEL_TIMEOUT,IdleWindow::ID_RELEASE,IdleWindow::onRelease)
  };

FXIMPLEMENT(IdleWindow,FXMainWindow,IdleWindowMap,ARRAYNUMBER(IdleWindowMap))


// Make the window
IdleWindow::IdleWindow(FXApp* a):FXMainWindow(a,"Idle",nullptr,nullptr,DECOR_ALL,0,0,240,80){
  new FXLabel(this,"Keeping the screen on for a moment");
  setIdleInhibit(true);
  }


// Create it; the request made before is carried out now
void IdleWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  printf("idle inhibit %s, %s\n",getIdleInhibit()?"requested":"not requested",heldState());
  fflush(stdout);
  getApp()->addTimeout(this,ID_RELEASE,HOLD_TIME);
  }


// Give it up, and leave a moment later
long IdleWindow::onRelease(FXObject*,FXSelector,void*){
  setIdleInhibit(false);
  printf("idle inhibit %s, %s\n",getIdleInhibit()?"requested":"released",heldState());
  fflush(stdout);
  getApp()->addTimeout(getApp(),FXApp::ID_QUIT,HOLD_TIME/2);
  return 1;
  }


int main(int argc,char** argv){
  fxtestargs(argc,argv);
  FXApp application("IdleTest","FoxTest");
  application.init(argc,argv);
  new IdleWindow(&application);
  application.create();
  return application.run();
  }
