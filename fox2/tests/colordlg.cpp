/********************************************************************************
*                                                                               *
*                      C o l o r   D i a l o g   T e s t                        *
*                                                                               *
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
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>

/*
  Stage 5 smoke test of the calculator port: build an FXColorDialog, walk
  its panels, open and close it, and check that an FXColorWell reports the
  color it was given.
*/

int main(int argc,char** argv){
  fxtestargs(argc,argv);

  printf("color dialog test\n");

  FXApp application("ColorTest","FoxTest");
  application.init(argc,argv);

  FXMainWindow main(&application,"ColorTest",nullptr,nullptr,DECOR_ALL);
  FXVerticalFrame frame(&main,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  FXColorWell *well=new FXColorWell(&frame,FXRGB(255,0,0),nullptr,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,80,40);
  FXColorDialog dialog(&main,"Color Dialog",DECOR_TITLE|DECOR_BORDER);

  application.create();
  main.show();

  int errors=0;

  // The well keeps the color it was constructed with
  if(well->getRGBA()!=FXRGB(255,0,0)){
    printf("well color %08x\n",well->getRGBA());
    errors++;
    }
  well->setRGBA(FXRGB(1,2,3));
  if(well->getRGBA()!=FXRGB(1,2,3)){
    printf("well color after set %08x\n",well->getRGBA());
    errors++;
    }

  // The dialog starts on the ring panel and honors a color
  dialog.setRGBA(FXRGB(0,128,255));
  if(FXRGB(FXREDVAL(dialog.getRGBA()),FXGREENVAL(dialog.getRGBA()),FXBLUEVAL(dialog.getRGBA()))!=FXRGB(0,128,255)){
    printf("dialog color %08x\n",dialog.getRGBA());
    errors++;
    }

  // Every panel of the selector can be activated
  for(FXint panel=COLORTAB_COLOR_RING; panel<=COLORTAB_COLOR_LIST; panel++){
    dialog.setActivePanel(panel);
    if(dialog.getActivePanel()!=panel){
      printf("dialog panel %d, expected %d\n",dialog.getActivePanel(),panel);
      errors++;
      }
    }
  dialog.setActivePanel(COLORTAB_COLOR_RING);

  // Open the dialog and close it again
  dialog.show(PLACEMENT_OWNER);
  if(!dialog.shown()){
    printf("dialog did not show\n");
    errors++;
    }
  dialog.hide();
  if(dialog.shown()){
    printf("dialog did not hide\n");
    errors++;
    }

  if(errors){
    printf("%d errors\n",errors);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }
