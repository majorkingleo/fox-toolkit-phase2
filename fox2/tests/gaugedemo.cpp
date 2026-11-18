/********************************************************************************
*                                                                               *
*                      G a u g e   T e s t                                      *
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
#include "FXProgressBar.h"
#include "FXGauge.h"
#include "FX7Segment.h"

/*
  Shows the progress bar in its three shapes, the gauge and the seven segment
  display, so a theme can be looked over on the trough, the filled part, the
  dial face and the segments at once.  The themed frame gate runs this program.
*/

/*******************************************************************************/

class Demo : public FXMainWindow {
  FXDECLARE(Demo)
public:
  Demo(){}
  Demo(FXApp* a):FXMainWindow(a,"Gauge Test",nullptr,nullptr,DECOR_ALL,0,0,520,360){
    FXVerticalFrame *f=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,8,8,8,8);
    hbar=new FXProgressBar(f,nullptr,0,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK|PROGRESSBAR_PERCENTAGE,0,0,0,26);
    hbar->setTotal(100); hbar->setProgress(37);
    FXHorizontalFrame *row=new FXHorizontalFrame(f,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
    vbar=new FXProgressBar(row,nullptr,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|FRAME_SUNKEN|FRAME_THICK|PROGRESSBAR_VERTICAL|PROGRESSBAR_PERCENTAGE,0,0,40,0);
    vbar->setTotal(100); vbar->setProgress(60);
    dial=new FXProgressBar(row,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK|PROGRESSBAR_DIAL|PROGRESSBAR_PERCENTAGE,0,0,120,0);
    dial->setTotal(100); dial->setProgress(45);
    gauge=new FXGauge(f,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK,180,-180,0,0,0,80);
    gauge->setRange(0,100); gauge->setValue(65);
    seg=new FX7Segment(f,"1234",LAYOUT_LEFT|LAYOUT_FIX_HEIGHT|FRAME_SUNKEN|FRAME_THICK|SEVENSEGMENT_SHADOW,0,0,0,48);
    }
  virtual void create(){ FXMainWindow::create(); show(FX::PLACEMENT_SCREEN); }
public:
  FXProgressBar *hbar,*vbar,*dial;
  FXGauge *gauge;
  FX7Segment *seg;
  };
FXDEFMAP(Demo) DemoMap[]={};
FXIMPLEMENT(Demo,FXMainWindow,DemoMap,ARRAYNUMBER(DemoMap))

int main(int argc,char**argv){
  FXApp app("GaugeTest","FoxTest");
  app.init(argc,argv);
  // A main window deletes itself when it is closed, so it must live on the
  // heap; a stack window would be deleted there and again when main returns
  new Demo(&app);
  app.create();
  return app.run();
  }
