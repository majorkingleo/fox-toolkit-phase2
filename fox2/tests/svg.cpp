/********************************************************************************
*                                                                               *
*                         S V G   I m a g e   T e s t                           *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "fx.h"
#include <stdio.h>


// A red square with a transparent margin, 32x32
static const char document[]=
  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\" viewBox=\"0 0 32 32\">"
  "<rect x=\"4\" y=\"4\" width=\"24\" height=\"24\" fill=\"#ff0000\"/>"
  "</svg>";


static int failures=0;

static void check(FXbool ok,const char* what){
  printf("%s: %s\n",ok?"ok":"FAIL",what);
  if(!ok) failures++;
  }


int main(int argc,char* argv[]){
  FXApp app("svgtest","FoxTest");
  app.init(argc,argv);

  FXSVGImage natural(&app,(const FXuchar*)document);
  check(natural.getWidth()==32 && natural.getHeight()==32,"natural size is 32x32");
  {
  FXColor c=natural.getData()?natural.getData()[16*32+16]:0;
  check(FXREDVAL(c)>240 && FXGREENVAL(c)<16 && FXBLUEVAL(c)<16 && FXALPHAVAL(c)>240,"center pixel is opaque red");
  }

  FXSVGImage scaled(&app,(const FXuchar*)document,0,16,16);
  check(scaled.getWidth()==16 && scaled.getHeight()==16,"requested raster size is 16x16");
  {
  FXColor c=scaled.getData()?scaled.getData()[8*16+8]:0;
  check(FXREDVAL(c)>240 && FXGREENVAL(c)<16 && FXBLUEVAL(c)<16,"scaled center pixel is red");
  }

  FXSVGIcon icon(&app,(const FXuchar*)document);
  check(icon.getWidth()==32 && icon.getHeight()==32,"icon size is 32x32");
  {
  FXColor c=icon.getData()?icon.getData()[16*32+16]:0;
  check(FXREDVAL(c)>240 && FXGREENVAL(c)<16,"icon center pixel is red");
  }

  FXIconSource source;
  {
  FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>((const FXuchar*)document),strlen(document)+1);
  FXImage* image=source.loadImageStream(&app,ms,FXString::null);
  check(image!=nullptr && image->getWidth()==32 && image->getHeight()==32,"FXIconSource recognizes SVG by content");
  delete image;
  }
  {
  FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>((const FXuchar*)document),strlen(document)+1);
  FXIcon* icn=source.loadIconStream(&app,ms,"svg");
  check(icn!=nullptr && icn->getWidth()==32 && icn->getHeight()==32,"FXIconSource loads SVG by extension");
  delete icn;
  }
  {
  FXImage* image=source.loadImageData(&app,(const FXuchar*)document,FXString::null);
  check(image!=nullptr && image->getWidth()==32 && image->getHeight()==32,"FXIconSource data array finds SVG by content");
  delete image;
  }

  printf("%s\n",failures==0?"PASS: SVG image and icon load":"FAIL: some SVG checks failed");
  return failures==0?0:1;
  }
