/********************************************************************************
*                                                                               *
*                      T o p l e v e l   I c o n   T e s t                     *
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

/*

  A window with an icon of its own, so that a gate can watch what the platform
  does with it.  The icon is drawn here instead of loaded, so the test needs no
  file and the pixels are known.

*/

#define ICON_SIZE 16


// A small icon: a red square with a green border
static FXIcon* makeIcon(FXApp* app){
  FXColor pixels[ICON_SIZE*ICON_SIZE];
  FXint x,y;
  for(y=0; y<ICON_SIZE; y++){
    for(x=0; x<ICON_SIZE; x++){
      FXbool edge=(x==0 || y==0 || x==ICON_SIZE-1 || y==ICON_SIZE-1);
      pixels[y*ICON_SIZE+x]=edge?FXRGB(0,200,0):FXRGB(200,0,0);
      }
    }
  return new FXIcon(app,pixels,0,0,ICON_SIZE,ICON_SIZE);
  }


int main(int argc,char** argv){
  FXApp application("IconTest","FoxTest");
  FXMainWindow *main;
  FXIcon *icon;
  fxtestargs(argc,argv);
  application.init(argc,argv);
  main=new FXMainWindow(&application,"Icon",nullptr,nullptr,DECOR_ALL,0,0,200,80);
  application.create();
  icon=makeIcon(&application);
  icon->create();
  main->setIcon(icon);
  main->show(PLACEMENT_SCREEN);
  printf("icon set on the toplevel\n");
  fflush(stdout);
  return application.run();
  }
