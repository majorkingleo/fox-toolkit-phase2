/********************************************************************************
*                                                                               *
*                        R u l e r   T e s t                                    *
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
#include "FXRuler.h"
#include "FXRulerView.h"

/*
  Shows a horizontal and a vertical ruler and the ruler view that composes them,
  so a theme can be looked over on the tick, the numbers, the document well and
  the markers at once.  The themed frame gate runs this program.
*/

/*******************************************************************************/

class Demo : public FXMainWindow {
  FXDECLARE(Demo)
public:
  Demo(){}
  Demo(FXApp* a):FXMainWindow(a,"Ruler Test",nullptr,nullptr,DECOR_ALL,0,0,560,300){
    FXVerticalFrame *f=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,8,8,8,8);
    hrule=new FXRuler(f,nullptr,0,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK|RULER_TICKS_BOTTOM|RULER_NUMBERS,0,0,0,32);
    hrule->setDocumentSize(500,false);
    hrule->setPosition(60,false);
    FXHorizontalFrame *row=new FXHorizontalFrame(f,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
    vrule=new FXRuler(row,nullptr,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|FRAME_SUNKEN|FRAME_THICK|RULER_VERTICAL|RULER_TICKS_RIGHT|RULER_NUMBERS,0,0,32,0);
    vrule->setDocumentSize(200,false);
    vrule->setPosition(40,false);
    view=new FXRulerView(row,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    }
  virtual void create(){ FXMainWindow::create(); show(FX::PLACEMENT_SCREEN); }
public:
  FXRuler *hrule,*vrule;
  FXRulerView *view;
  };
FXDEFMAP(Demo) DemoMap[]={};
FXIMPLEMENT(Demo,FXMainWindow,DemoMap,ARRAYNUMBER(DemoMap))

int main(int argc,char**argv){
  FXApp app("RulerTest","FoxTest");
  app.init(argc,argv);
  Demo demo(&app);
  app.create();
  app.run();
  return 0;
  }
