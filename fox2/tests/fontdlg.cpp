/********************************************************************************
*                                                                               *
*                        F o n t   D i a l o g   T e s t                        *
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
#include <string.h>

/*
  Stage 6 smoke test of the calculator port: enumerate the fonts the font
  selector lists, put a font description into an FXFontDialog and open and
  close it.  Font enumeration goes through the per-DPI font cache, so this
  test also guards that path.
*/

int main(int argc,char** argv){
  fxtestargs(argc,argv);

  printf("font dialog test\n");

  FXApp application("FontTest","FoxTest");
  application.init(argc,argv);

  int errors=0;

  // The platform lists some fonts
  FXFontDesc *fonts=nullptr;
  FXuint numfonts=0;
  if(!FXFont::listFonts(fonts,numfonts,FXString::null,0,0,0,0,0)){
    printf("font enumeration failed\n");
    errors++;
    }
  printf("%u fonts listed\n",numfonts);
  FXFontDesc first;
  if(numfonts>0){
    memcpy(&first,fonts,sizeof(FXFontDesc));
    }
  if(numfonts<1){
    printf("no fonts listed\n");
    errors++;
    }
  for(FXuint i=0; i<numfonts && i<3; i++){
    printf("  %s\n",fonts[i].face);
    }
  freeElms(fonts);

  // The main window, and every widget below it, deletes itself when the window
  // is closed, so the whole tree lives on the heap
  FXMainWindow *main=new FXMainWindow(&application,"FontTest",nullptr,nullptr,DECOR_ALL);

  // The dialog carries a font description in and out
  FXFontDialog *dialog=new FXFontDialog(main,"Font Dialog",DECOR_TITLE|DECOR_BORDER);
  FXFontDesc desc=first;
  desc.size=120;
  dialog->setFontDesc(desc);
  FXFontDesc got=dialog->getFontDesc();
  if(FXString(got.face)!=FXString(desc.face) || got.size!=desc.size){
    printf("font desc is %s-%d, expected %s-%d\n",got.face,got.size,desc.face,desc.size);
    errors++;
    }
  printf("dialog font %s-%d\n",got.face,got.size);

  application.create();
  main->show();

  // Open the dialog and close it again; the selector lists the faces
  dialog->show(PLACEMENT_OWNER);
  if(!dialog->shown()){
    printf("dialog did not show\n");
    errors++;
    }
  dialog->hide();
  if(dialog->shown()){
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
