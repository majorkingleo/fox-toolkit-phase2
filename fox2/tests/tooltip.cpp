/********************************************************************************
*                                                                               *
*                       T o o l T i p   T e s t                                 *
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
  Tool tip test.

  One button that carries a tip, plus the tool tip window itself.  Resting the
  pointer on the button pops the tip; see scripts/gate-wayland-tooltip.sh.

  On Wayland the tip is an xdg_popup that must not take the pointer grab, which
  is what tells it apart from a menu.  The tip also asks the application where
  the pointer is, which a Wayland client can only answer from what the seat
  reported, so this test covers that as well.
*/

class TipWindow : public FXMainWindow {
  FXDECLARE(TipWindow)
protected:
  FXButton* button;
  FXToolTip* tip;
public:
  enum { ID_REPORT = FXMainWindow::ID_LAST, ID_TIP, ID_LAST };
  TipWindow(){ button=nullptr; tip=nullptr; }
  TipWindow(FXApp* a);
  virtual void create();
  void watch(FXToolTip* t){ tip=t; getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_REPORT),2500000000LL); }
  long onQueryTip(FXObject*,FXSelector,void*);
  long onReport(FXObject*,FXSelector,void*);
  };


FXDEFMAP(TipWindow) TipWindowMap[]={
  FXMAPFUNC(SEL_QUERY_TIP,TipWindow::ID_TIP,TipWindow::onQueryTip),
  FXMAPFUNC(SEL_TIMEOUT,TipWindow::ID_REPORT,TipWindow::onReport)
  };

FXIMPLEMENT(TipWindow,FXMainWindow,TipWindowMap,ARRAYNUMBER(TipWindowMap))


// Make the window
TipWindow::TipWindow(FXApp* a):FXMainWindow(a,"ToolTip",nullptr,nullptr,DECOR_ALL,0,0,320,160){
  button=new FXButton(this,"&Press me",nullptr,this,ID_TIP,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  button->setTipText("This is the tool tip");
  }


// Create it
void TipWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


// The tip query of the button; answering it is what makes the tool tip pop
long TipWindow::onQueryTip(FXObject*,FXSelector,void*){
  return 1;
  }


// Report where the application thinks the pointer is, for the gate
long TipWindow::onReport(FXObject*,FXSelector,void*){
  FXWindow* under=getApp()->getCursorWindow();
  FXint x=0,y=0;
  FXuint buttons=0;
  getRoot()->getCursorPosition(x,y,buttons);
  fxmessage("cursor window %s, tip %s, text \"%s\", pointer %d,%d\n",
            under?under->getClassName():"none",
            (tip&&tip->shown())?"shown":"not shown",
            tip?tip->getText().text():"",x,y);
  return 1;
  }


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("ToolTipTest","FoxTest");
  application.init(argc,argv);

  FXToolTip* tip=new FXToolTip(&application,0);
  TipWindow* window=new TipWindow(&application);
  window->watch(tip);

  application.create();

  return application.run();
  }
