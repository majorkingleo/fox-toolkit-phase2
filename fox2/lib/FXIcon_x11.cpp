/********************************************************************************
*                                                                               *
*                               Icon   ( X 1 1 )                                *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXElement.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"

#if defined(FOX_X11)

/*
  Notes:

  - X11 icon: pixmaps and shape/etch masks, rendering and resizing.
*/

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002
#define DARKCOLOR(r,g,b) (((r)+(g)+(b))<thresh)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create icon
void FXIcon::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Initialize visual
      visual->create();


      // Make image pixmap
      xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),visual->depth);
      if(!xid){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make shape pixmap
      shape=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),1);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch pixmap
      etch=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),1);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Render pixels
      render();

      // If we're not keeping the pixel buffer, release it
      if(!(options&IMAGE_KEEP)) release();
      }
    }
  }


// Destroy icon
void FXIcon::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      XFreePixmap((Display*)getApp()->getDisplay(),shape);
      XFreePixmap((Display*)getApp()->getDisplay(),etch);
      XFreePixmap((Display*)getApp()->getDisplay(),xid);
      }
    shape=0;
    etch=0;
    xid=0;
    }
  }


// Render icon X Windows
void FXIcon::render(){
  if(xid){
    XImage *xim=nullptr;
    FXbool shmi=false;
    FXColor *img;
    FXint x,y;
    XGCValues values;
    GC gc;
#ifdef HAVE_XSHM_H
    XShmSegmentInfo shminfo;
#endif

    FXTRACE(TOPIC_CREATION,"%s::render shape %p\n",getClassName(),this);

    // Render the image pixels
    FXImage::render();

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Turn it on iff both supported and desired
#ifdef HAVE_XSHM_H
      if(options&IMAGE_SHMI) shmi=getApp()->shmi;
#endif

      // First try XShm
#ifdef HAVE_XSHM_H
      if(shmi){
        xim=XShmCreateImage((Display*)getApp()->getDisplay(),(Visual*)visual->visual,1,ZPixmap,nullptr,&shminfo,width,height);
        if(!xim){ shmi=0; }
        if(shmi){
          shminfo.shmid=shmget(IPC_PRIVATE,xim->bytes_per_line*xim->height,IPC_CREAT|0777);
          if(shminfo.shmid==-1){ xim->data=nullptr; XDestroyImage(xim); xim=nullptr; shmi=0; }
          if(shmi){
            shminfo.shmaddr=xim->data=(char*)shmat(shminfo.shmid,0,0);
            shminfo.readOnly=false;
            XShmAttach((Display*)getApp()->getDisplay(),&shminfo);
            FXTRACE(TOPIC_DETAIL,"Bitmap XSHM attached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
            }
          }
        }
#endif

      // Try the old fashioned way
      if(!shmi){

        // Try create image
        xim=XCreateImage((Display*)getApp()->getDisplay(),(Visual*)visual->visual,1,ZPixmap,0,nullptr,width,height,32,0);
        if(!xim){ fxerror("%s::render: unable to render icon.\n",getClassName()); }

        // Try create temp pixel store
        if(!callocElms(xim->data,xim->bytes_per_line*height)){ fxerror("%s::render: unable to allocate memory.\n",getClassName()); }
        }

      // Make GC
      values.foreground=0xffffffff;
      values.background=0xffffffff;
      gc=XCreateGC((Display*)getApp()->getDisplay(),shape,GCForeground|GCBackground,&values);

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

      // Fill shape mask
      if(options&IMAGE_OPAQUE){                                 // Opaque image
        FXTRACE(TOPIC_DETAIL,"Shape rectangle\n");
        memset(xim->data,0xff,xim->bytes_per_line*height);
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        FXTRACE(TOPIC_DETAIL,"Shape from alpha-color\n");
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(img[x]!=transp));
            }
          img+=width;
          }
        }
      else{                                                     // Transparency channel
        FXTRACE(TOPIC_DETAIL,"Shape from alpha-channel\n");
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(((FXuchar*)(img+x))[3]!=0));
            }
          img+=width;
          }
        }

      // Transfer image
#ifdef HAVE_XSHM_H
      if(shmi){
        XShmPutImage((Display*)getApp()->getDisplay(),shape,gc,xim,0,0,0,0,width,height,False);
        XSync((Display*)getApp()->getDisplay(),False);
        }
#endif
      if(!shmi){
        XPutImage((Display*)getApp()->getDisplay(),shape,gc,xim,0,0,0,0,width,height);
        }

      // Fill etch image
      if(options&IMAGE_OPAQUE){                                 // Opaque image
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(img[x]!=transp) && DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }
      else{                                                     // Transparency channel
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(((FXuchar*)(img+x))[3]!=0) && DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }

      // Transfer image
#ifdef HAVE_XSHM_H
      if(shmi){
        XShmPutImage((Display*)getApp()->getDisplay(),etch,gc,xim,0,0,0,0,width,height,False);
        XSync((Display*)getApp()->getDisplay(),False);
        }
#endif
      if(!shmi){
        XPutImage((Display*)getApp()->getDisplay(),etch,gc,xim,0,0,0,0,width,height);
        }

      // Clean up
#ifdef HAVE_XSHM_H
      if(shmi){
        FXTRACE(TOPIC_DETAIL,"Bitmap XSHM detached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
        XShmDetach((Display*)getApp()->getDisplay(),&shminfo);
        xim->data=nullptr;
        XDestroyImage(xim);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid,IPC_RMID,0);
        }
#endif
      if(!shmi){
        freeElms(xim->data);
        XDestroyImage(xim);
        }
      XFreeGC((Display*)getApp()->getDisplay(),gc);
      }
    }
  }


// Resize pixmap to the specified width and height; the contents become undefined
void FXIcon::resize(FXint w,FXint h){
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::resize(%d,%d) %p\n",getClassName(),w,h,this);
  if(width!=w || height!=h){

    // Resize device dependent pixmap
    if(xid){
      // Get depth (should use visual!!)
      int dd=visual->getDepth();

      // Free old pixmaps
      XFreePixmap((Display*)getApp()->getDisplay(),xid);
      XFreePixmap((Display*)getApp()->getDisplay(),etch);
      XFreePixmap((Display*)getApp()->getDisplay(),shape);

      // Make new pixmap
      xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,dd);
      if(!xid){ fxerror("%s::resize: unable to resize image.\n",getClassName()); }

      // Make shape pixmap
      shape=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,1);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch pixmap
      etch=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,1);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }
      }
    }

  // Resize data array
  if(data){
    if(!(options&IMAGE_OWNED)){       // Need to own array
      allocElms(data,w*h);
      options|=IMAGE_OWNED;
      }
    else if(w*h!=width*height){
      resizeElms(data,w*h);
      }
    }

  // Remember new size
  width=w;
  height=h;
  }

}

#endif
