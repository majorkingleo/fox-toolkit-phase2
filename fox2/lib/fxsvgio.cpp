/********************************************************************************
*                                                                               *
*                      S V G   I m a g e   F o r m a t                          *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSVGImage.h"

/*
  Notes:

  - NanoSVG parses and rasterizes the vector image; it is header-only and
    vendored under lib/ (MIT license, see nanosvg-LICENSE.txt).
  - The rasterizer always draws into a canvas of the requested size; a target
    that does not have the aspect ratio of the image is letterboxed and the
    margins stay transparent.
  - NanoSVG writes non-premultiplied RGBA, which is what FXIcon and FXRaster
    expect, so the bytes go into FXColor without further scaling.
*/

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

using namespace FX;

/*******************************************************************************/

namespace FX {


// Check if stream contains an SVG: the XML header, comments or a DOCTYPE may
// come before the root element, so look for <svg> in the first few kilobytes
FXbool fxcheckSVG(FXStream& store){
  if(store.direction()==FXStreamLoad){
    FXlong save=store.position();
    FXlong total,avail;
    FXchar head[4097];
    FXint n;
    store.position(0,FXFromEnd);
    total=store.position();
    store.position(save);
    avail=total-save;
    if(avail<=0) return false;
    n=(FXint)FXMIN(avail,(FXlong)4096);
    store.load(head,n);
    store.position(save);
    head[n]='\0';
    return strstr(head,"<svg")!=nullptr || strstr(head,"<SVG")!=nullptr;
    }
  return false;
  }


// Load an SVG and rasterize it at the requested size
FXbool fxloadSVG(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint w,FXint h){
  NSVGimage* image=nullptr;
  NSVGrasterizer* rast=nullptr;
  FXuchar* rgba=nullptr;
  FXColor* pixels=nullptr;
  FXchar* text=nullptr;
  FXlong save,total,len;
  FXint nw,nh,tw,th,x,y;
  FXdouble scale,tx,ty;
  if(store.direction()!=FXStreamLoad) return false;

  // Read the whole document; the parser wants a mutable C string
  save=store.position();
  store.position(0,FXFromEnd);
  total=store.position();
  len=total-save;
  store.position(save);
  if(len<=0) return false;
  if(!allocElms(text,(FXuval)len+1)) return false;
  store.load(text,len);
  text[len]='\0';

  // Parse; the natural size is in pixels
  image=nsvgParse(text,"px",96.0f);
  freeElms(text);
  if(!image) return false;
  nw=(FXint)(image->width+0.5f);
  nh=(FXint)(image->height+0.5f);
  if(nw<=0 || nh<=0){
    nsvgDelete(image);
    return false;
    }

  // Target size; a missing dimension follows the aspect ratio
  tw=w;
  th=h;
  if(tw<=0 && th<=0){
    tw=nw;
    th=nh;
    }
  else if(tw<=0){
    tw=FXMAX((FXint)((FXdouble)nw*th/nh+0.5),1);
    }
  else if(th<=0){
    th=FXMAX((FXint)((FXdouble)nh*tw/nw+0.5),1);
    }

  // Uniform scale, centered so that the aspect ratio is kept
  scale=FXMIN((FXdouble)tw/nw,(FXdouble)th/nh);
  tx=((FXdouble)tw-(FXdouble)nw*scale)*0.5;
  ty=((FXdouble)th-(FXdouble)nh*scale)*0.5;

  if(!allocElms(rgba,(FXuval)tw*th*4) || !allocElms(pixels,(FXuval)tw*th)){
    freeElms(rgba);
    freeElms(pixels);
    nsvgDelete(image);
    return false;
    }

  rast=nsvgCreateRasterizer();
  if(!rast){
    freeElms(rgba);
    freeElms(pixels);
    nsvgDelete(image);
    return false;
    }
  nsvgRasterize(rast,image,(float)tx,(float)ty,(float)scale,rgba,tw,th,tw*4);
  nsvgDeleteRasterizer(rast);
  nsvgDelete(image);

  // RGBA, non-premultiplied, to FXColor 0xAARRGGBB
  for(y=0; y<th; y++){
    for(x=0; x<tw; x++){
      FXuchar* p=rgba+((FXuval)y*tw+x)*4;
      pixels[(FXuval)y*tw+x]=FXRGB(p[0],p[1],p[2])|((FXColor)p[3]<<24);
      }
    }
  freeElms(rgba);
  data=pixels;
  width=tw;
  height=th;
  return true;
  }

}
