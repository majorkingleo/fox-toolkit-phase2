/********************************************************************************
*                                                                               *
*                       C o l o r   W i d g e t   T e s t                       *
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
#include "FXColorBar.h"
#include "FXColorWheel.h"
#include "FXColorRing.h"
#include "FXColorList.h"

/*
  Shows the color widgets that the color selector is made of, so a theme can be
  looked over on all of them at once: the ramp with its handle, the wheel, the
  ring and the color list.  The themed frame gate runs this program.
*/

/*******************************************************************************/

// Color widget test window
class ColorWindow : public FXMainWindow {
  FXDECLARE(ColorWindow)
protected:
  FXColorBar   *bar;
  FXColorWheel *wheel;
  FXColorRing  *ring;
  FXColorList  *list;
protected:
  ColorWindow(){}
public:
  ColorWindow(FXApp* a);
  virtual void create();
  };


/*******************************************************************************/

// Map
FXDEFMAP(ColorWindow) ColorWindowMap[]={
  };
FXIMPLEMENT(ColorWindow,FXMainWindow,ColorWindowMap,ARRAYNUMBER(ColorWindowMap))


/*******************************************************************************/

// Construct
ColorWindow::ColorWindow(FXApp* a):FXMainWindow(a,"Color Test",nullptr,nullptr,DECOR_ALL,0,0,560,420){
  FXVerticalFrame *frame=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,8,8,8,8);
  bar=new FXColorBar(frame,nullptr,0,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK,0,0,0,28);
  bar->setHue(200); bar->setSat(200); bar->setVal(200);
  FXHorizontalFrame *row=new FXHorizontalFrame(frame,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
  wheel=new FXColorWheel(row,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,180,180);
  wheel->setHue(200); wheel->setSat(200); wheel->setVal(200);
  ring=new FXColorRing(row,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,180,180);
  ring->setHue(200); ring->setSat(200); ring->setVal(200);
  list=new FXColorList(frame,nullptr,0,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK,0,0,0,110);
  list->appendItem("first colour",FXRGB(0xd0,0x50,0x50),nullptr);
  list->appendItem("second colour",FXRGB(0x50,0xd0,0x50),nullptr);
  list->appendItem("third colour",FXRGB(0x50,0x50,0xd0),nullptr);
  }


// Create and show
void ColorWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


/*******************************************************************************/

int main(int argc,char**argv){
  FXApp application("ColorTest","FoxTest");
  application.init(argc,argv);
  // The main window deletes itself when it is closed, so it lives on the heap
  new ColorWindow(&application);
  application.create();
  return application.run();
  }
