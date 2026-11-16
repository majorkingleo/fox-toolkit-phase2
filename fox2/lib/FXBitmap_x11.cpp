/********************************************************************************
*                                                                               *
*                              Bitmap   ( X 1 1 )                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxendian.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXMetaClass.h"
#include "FXString.h"
#include "FXStream.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXBitmap.h"

#if defined(FOX_X11)

/*
  Notes:
  - Platform part of the bitmap: native bitmap creation, restore and render.
*/

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

// Changable bitmap options
#define BITMAP_MASK   (BITMAP_KEEP|BITMAP_SHMI|BITMAP_SHMP)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create bitmap
void FXBitmap::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Initialize visual
      visual->create();

      xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),1);

      // Were we successful?
      if(!xid){ throw FXImageException("unable to create bitmap"); }

      // Render pixels
      render();

      // If we're not keeping the pixel buffer, release it
      if(!(options&BITMAP_KEEP)) release();
      }
    }
  }


// Destroy bitmap
void FXBitmap::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      XFreePixmap((Display*)getApp()->getDisplay(),xid);
      }
    xid=0;
    }
  }


// Restore client-side pixel buffer from bitmap
void FXBitmap::restore(){
  if(xid){
    XImage *xim=nullptr;
    FXint size,x,y;

    FXTRACE(TOPIC_CREATION,"%s::restore bitmap %p\n",getClassName(),this);

    // Check for legal size
    if(width<1 || height<1){ fxerror("%s::restore: illegal bitmap size %dx%d.\n",getClassName(),width,height); }

    // Make array for data if needed
    if(!data){
      size=bytewidth*height;
      if(!callocElms(data,size)){ throw FXMemoryException("unable to restore bitmap"); }
      options|=BITMAP_OWNED;
      }

    // Got local buffer to receive into
    if(data){
      xim=XGetImage((Display*)getApp()->getDisplay(),xid,0,0,width,height,1,XYPixmap);
      if(!xim){ throw FXImageException("unable to restore image"); }

      // Should have succeeded
      FXASSERT(xim);

      FXTRACE(TOPIC_DETAIL,"bm width = %d\n",xim->width);
      FXTRACE(TOPIC_DETAIL,"bm height = %d\n",xim->height);
      FXTRACE(TOPIC_DETAIL,"bm format = %s\n",xim->format==XYBitmap?"XYBitmap":xim->format==XYPixmap?"XYPixmap":"ZPixmap");
      FXTRACE(TOPIC_DETAIL,"bm byte_order = %s\n",(xim->byte_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"bm bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"bm bitmap_bit_order = %s\n",(xim->bitmap_bit_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"bm bitmap_pad = %d\n",xim->bitmap_pad);
      FXTRACE(TOPIC_DETAIL,"bm bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"bm depth = %d\n",xim->depth);
      FXTRACE(TOPIC_DETAIL,"bm bytes_per_line = %d\n",xim->bytes_per_line);
      FXTRACE(TOPIC_DETAIL,"bm bits_per_pixel = %d\n",xim->bits_per_pixel);

      // Grab pixels from image
      for(y=0; y<height; y++){
        for(x=0; x<width; x++){
          if(XGetPixel(xim,x,y)) data[y*bytewidth+(x>>3)]|=1<<(x&7);
          }
        }

      // Destroy image
      XDestroyImage(xim);
      }
    }
  }


// Render into pixmap
void FXBitmap::render(){
  if(xid){
    XImage *xim=nullptr;
    int size;
    FXuchar *pix;
    int i;
    XGCValues values;
    GC gc;

    FXTRACE(TOPIC_CREATION,"%s::render bitmap %p\n",getClassName(),this);

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Make GC
      values.foreground=0xffffffff;
      values.background=0;
      gc=XCreateGC((Display*)getApp()->getDisplay(),xid,GCForeground|GCBackground,&values);

      // Create image to transfer pixels
      xim=XCreateImage((Display*)getApp()->getDisplay(),(Visual*)visual->visual,1,XYBitmap,0,nullptr,width,height,8,(width+7)>>3);
      if(!xim){ throw FXImageException("unable to render bitmap"); }

      // Try create temp pixel store
      if(!allocElms(xim->data,xim->bytes_per_line*height)){ throw FXMemoryException("unable to render bitmap"); }

      FXTRACE(TOPIC_DETAIL,"bm width = %d\n",xim->width);
      FXTRACE(TOPIC_DETAIL,"bm height = %d\n",xim->height);
      FXTRACE(TOPIC_DETAIL,"bm format = %s\n",xim->format==XYBitmap?"XYBitmap":xim->format==XYPixmap?"XYPixmap":"ZPixmap");
      FXTRACE(TOPIC_DETAIL,"bm byte_order = %s\n",(xim->byte_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"bm bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"bm bitmap_bit_order = %s\n",(xim->bitmap_bit_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"bm bitmap_pad = %d\n",xim->bitmap_pad);
      FXTRACE(TOPIC_DETAIL,"bm bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"bm depth = %d\n",xim->depth);
      FXTRACE(TOPIC_DETAIL,"bm bytes_per_line = %d\n",xim->bytes_per_line);
      FXTRACE(TOPIC_DETAIL,"bm bits_per_pixel = %d\n",xim->bits_per_pixel);

      // Render bits into server-formatted bitmap
      size=xim->bytes_per_line*height;
      pix=(FXuchar*)xim->data;

      // Most significant bit first
      if(xim->bitmap_bit_order==MSBFirst){
        for(i=0; i<size; i++) pix[i]=reverse8(data[i]);
        }

      // Least significant bit first
      else{
        memcpy(pix,data,size);
        }

      // Blast the image
      XPutImage((Display*)getApp()->getDisplay(),xid,gc,xim,0,0,0,0,width,height);
      freeElms(xim->data);
      XDestroyImage(xim);
      XFreeGC((Display*)getApp()->getDisplay(),gc);
      }
    }
  }


// Resize bitmap to the specified width and height; the contents become undefined
void FXBitmap::resize(FXint w,FXint h){
  FXint bw;
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::resize(%d,%d)\n",getClassName(),w,h);
  bw=(w+7)>>3;
  if(xid){

    // Free old pixmap
    XFreePixmap((Display*)getApp()->getDisplay(),xid);

    // Make new pixmap
    xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,1);
    if(!xid){ throw FXImageException("unable to resize bitmap"); }

    }

  // Resize data array; only do the work if the new
  // array is a different size as measured in bytes!
  if(data){
    if(!(options&BITMAP_OWNED)){        // Need to own array
      if(!allocElms(data,h*bw)){ throw FXMemoryException("unable to resize bitmap"); }
      options|=BITMAP_OWNED;
      }
    else if(h*bw!=height*bytewidth){
      if(!resizeElms(data,h*bw)){ throw FXMemoryException("unable to resize bitmap"); }
      }
    }

  // Remember new size
  bytewidth=bw;
  width=w;
  height=h;
  }


}

#endif
