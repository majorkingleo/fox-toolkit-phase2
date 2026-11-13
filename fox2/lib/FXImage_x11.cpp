/********************************************************************************
*                                                                               *
*                               Image   ( X 1 1 )                               *
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
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXImage.h"

#if defined(FOX_X11)

/*
  Notes:
  - Platform part of the image: native bitmap creation, restore and render.
*/

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002
#define DISPLAY(app) ((Display*)((app)->display))

// Maximum size of the colormap; for high-end graphics systems
// you may want to define HIGHENDGRAPHICS to allow for large colormaps
#ifdef HIGHENDGRAPHICS
#define MAX_MAPSIZE 4096
#else
#define MAX_MAPSIZE 256
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create image
void FXImage::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Initialize visual
      visual->create();


      FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(Pixmap));

      // Make pixmap
      xid=XCreatePixmap(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),FXMAX(width,1),FXMAX(height,1),visual->depth);


      // Were we successful?
      if(!xid){ throw FXImageException("unable to create image"); }

      // Render pixels
      render();

      // Release pixel buffer
      if(!(options&IMAGE_KEEP)) release();
      }
    }
  }


// Destroy image
void FXImage::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      XFreePixmap(DISPLAY(getApp()),xid);
      }
    xid=0;
    }
  }


// Find shift amount
static inline FXuint findshift(FXPixel mask){
  FXuint sh=0;
  while(!(mask&(1UL<<sh))) sh++;
  return sh;
  }


// Find low bit in mask
static inline FXPixel lowbit(FXPixel mask){
  return (~mask+1)&mask;
  }


// Restore client-side pixel buffer from image
void FXImage::restore(){
  if(xid){
    FXPixel red,green,blue;
    FXPixel red1,green1,blue1;
    FXPixel pixel;
    FXuint  redshift,greenshift,blueshift;
    FXPixel redmask,greenmask,bluemask;
    int size,i;
    FXbool shmi=false;
    XImage *xim=nullptr;
    Visual *vis;
    FXint x,y;
    FXuchar *img;
    FXuint r,g,b;
    FXuchar rtab[MAX_MAPSIZE];
    FXuchar gtab[MAX_MAPSIZE];
    FXuchar btab[MAX_MAPSIZE];
#ifdef HAVE_XSHM_H
    XShmSegmentInfo shminfo;
#endif

    FXTRACE(TOPIC_CREATION,"%s::restore image %p\n",getClassName(),this);

    // Check for legal size
    if(width<1 || height<1){ fxerror("%s::restore: illegal image size %dx%d.\n",getClassName(),width,height); }

    // Get Visual
    vis=(Visual*)visual->visual;

    // Just in case you're on a high-end system
    FXASSERT(vis->map_entries<=MAX_MAPSIZE);

    // Make array for data if needed
    if(!data){
      size=width*height;
      if(!allocElms(data,size)){ throw FXMemoryException("unable to restore image"); }
      options|=IMAGE_OWNED;
      }

    // Got local buffer to receive into
    if(data){

      // Turn it on iff both supported and desired
#ifdef HAVE_XSHM_H
      if(options&IMAGE_SHMI) shmi=getApp()->shmi;
#endif

      // First try XShm
#ifdef HAVE_XSHM_H
      if(shmi){
        xim=XShmCreateImage(DISPLAY(getApp()),vis,visual->depth,(visual->depth==1)?XYPixmap:ZPixmap,nullptr,&shminfo,width,height);
        if(!xim){ shmi=0; }
        if(shmi){
          shminfo.shmid=shmget(IPC_PRIVATE,xim->bytes_per_line*xim->height,IPC_CREAT|0777);
          if(shminfo.shmid==-1){ xim->data=nullptr; XDestroyImage(xim); xim=nullptr; shmi=0; }
          if(shmi){
            shminfo.shmaddr=xim->data=(char*)shmat(shminfo.shmid,0,0);
            shminfo.readOnly=false;
            XShmAttach(DISPLAY(getApp()),&shminfo);
            FXTRACE(TOPIC_DETAIL,"RGBPixmap XSHM attached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
            XShmGetImage(DISPLAY(getApp()),xid,xim,0,0,AllPlanes);
            XSync(DISPLAY(getApp()),False);
            }
          }
        }
#endif

      // Try the old fashioned way
      if(!shmi){
        xim=XGetImage(DISPLAY(getApp()),xid,0,0,width,height,AllPlanes,ZPixmap);
        if(!xim){ throw FXImageException("unable to restore image"); }
        }

      // Should have succeeded
      FXASSERT(xim);

      FXTRACE(TOPIC_DETAIL,"im width = %d\n",xim->width);
      FXTRACE(TOPIC_DETAIL,"im height = %d\n",xim->height);
      FXTRACE(TOPIC_DETAIL,"im format = %s\n",xim->format==XYBitmap?"XYBitmap":xim->format==XYPixmap?"XYPixmap":"ZPixmap");
      FXTRACE(TOPIC_DETAIL,"im byte_order = %s\n",(xim->byte_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"im bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"im bitmap_bit_order = %s\n",(xim->bitmap_bit_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"im bitmap_pad = %d\n",xim->bitmap_pad);
      FXTRACE(TOPIC_DETAIL,"im bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"im depth = %d\n",xim->depth);
      FXTRACE(TOPIC_DETAIL,"im bytes_per_line = %d\n",xim->bytes_per_line);
      FXTRACE(TOPIC_DETAIL,"im bits_per_pixel = %d\n",xim->bits_per_pixel);


      {
      XColor colors[MAX_MAPSIZE];

      // Get masks
      redmask=vis->red_mask;
      greenmask=vis->green_mask;
      bluemask=vis->blue_mask;

      // Read back the colormap and convert to more usable form
      if(vis->c_class!=TrueColor && vis->c_class!=DirectColor){
        for(i=0 ; i<vis->map_entries; i++){
          colors[i].pixel=i;
          colors[i].flags=DoRed|DoGreen|DoBlue;
          }
        }
      else{
        red=green=blue=0;
        red1=lowbit(redmask);
        green1=lowbit(greenmask);
        blue1=lowbit(bluemask);
        for(i=0; i<vis->map_entries; i++){
          colors[i].pixel=red|green|blue;
          colors[i].flags=DoRed|DoGreen|DoBlue;
          if(red<redmask) red+=red1;
          if(green<greenmask) green+=green1;
          if(blue<bluemask) blue+=blue1;
          }
        }
      XQueryColors(DISPLAY(getApp()),visual->colormap,colors,vis->map_entries);
      for(i=0; i<vis->map_entries; i++){
        rtab[i]=colors[i].red >> 8;
        gtab[i]=colors[i].green >> 8;
        btab[i]=colors[i].blue >> 8;
        }
      }

      // Now we convert the pixels back to color
      switch(xim->bits_per_pixel){
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          for(y=0,img=(FXuchar*)data; y<height; y++){
            for(x=0; x<width; x++){
              pixel=XGetPixel(xim,x,y);
              img[0]=btab[pixel];
              img[1]=gtab[pixel];
              img[2]=rtab[pixel];
              img[3]=255;
              img+=4;
              }
            }
          break;
        case 15:
        case 16:
        case 24:
        case 32:
        default:
          FXASSERT(vis->c_class==TrueColor || vis->c_class==DirectColor);
          redshift=findshift(redmask);
          greenshift=findshift(greenmask);
          blueshift=findshift(bluemask);
          for(y=0,img=(FXuchar*)data; y<height; y++){
            for(x=0; x<width; x++){
              pixel=XGetPixel(xim,x,y);
              r=(pixel&redmask)>>redshift;
              g=(pixel&greenmask)>>greenshift;
              b=(pixel&bluemask)>>blueshift;
              img[0]=btab[b];
              img[1]=gtab[g];
              img[2]=rtab[r];
              img[3]=255;
              img+=4;
              }
            }
          break;
        }

      // Destroy image
#ifdef HAVE_XSHM_H
      if(shmi){
        FXTRACE(TOPIC_DETAIL,"RGBPixmap XSHM detached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
        XShmDetach(DISPLAY(getApp()),&shminfo);
        XDestroyImage(xim);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid,IPC_RMID,0);
        }
#endif

      // Destroy image
      if(!shmi){
        XDestroyImage(xim);
        }
      }
    }
  }


// True generic mode
void FXImage::render_true_N_fast(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"True MSB/LSB N bpp render nearest\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// True generic mode
void FXImage::render_true_N_dither(void *xim,FXuchar *img){
  FXint x,y,d;
  FXTRACE(TOPIC_DETAIL,"True MSB/LSB N bpp render dither\n");
  y=0;
  do{
    x=0;
    do{
      d=((y&3)<<2)|(x&3);
      XPutPixel(((XImage*)xim),x,y,visual->rpix[d][img[2]] | visual->gpix[d][img[1]] | visual->bpix[d][img[0]]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// True 24 bit color
void FXImage::render_true_24(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-(width*3);
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXPixel val;
  FXint w,h;
  if(((XImage*)xim)->byte_order==MSBFirst){    // MSB
    FXTRACE(TOPIC_DETAIL,"True MSB 24bpp render\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)(val>>16);
        pix[1]=(FXuchar)(val>>8);
        pix[2]=(FXuchar)val;
        img+=4;
        pix+=3;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  else{                             // LSB
    FXTRACE(TOPIC_DETAIL,"True LSB 24bpp render\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)val;
        pix[1]=(FXuchar)(val>>8);
        pix[2]=(FXuchar)(val>>16);
        img+=4;
        pix+=3;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// True 32 bit color
void FXImage::render_true_32(void *xim,FXuchar *img){
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXuint jmp=((XImage*)xim)->bytes_per_line-(width<<2);
  FXPixel val;
  FXint w,h;

  // Byte order matches
  if(((XImage*)xim)->byte_order == FOX_BIGENDIAN){
    FXTRACE(TOPIC_DETAIL,"True MSB/LSB 32bpp render\n");
    h=height-1;
    do{
      w=width-1;
      do{
        *((FXuint*)pix)=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        img+=4;
        pix+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // MSB Byte order
  else if(((XImage*)xim)->byte_order==MSBFirst){
    FXTRACE(TOPIC_DETAIL,"True MSB 32bpp render\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)(val>>24);
        pix[1]=(FXuchar)(val>>16);
        pix[2]=(FXuchar)(val>>8);
        pix[3]=(FXuchar)val;
        img+=4;
        pix+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // LSB Byte order
  else{
    FXTRACE(TOPIC_DETAIL,"True LSB 32bpp render\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)val;
        pix[1]=(FXuchar)(val>>8);
        pix[2]=(FXuchar)(val>>16);
        pix[3]=(FXuchar)(val>>24);
        img+=4;
        pix+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// True 16 bit color
void FXImage::render_true_16_fast(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-(width<<1);
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXPixel val;
  FXint w,h;

  // Byte order matches
  if(((XImage*)xim)->byte_order == FOX_BIGENDIAN){
    FXTRACE(TOPIC_DETAIL,"True MSB/LSB 16bpp 5,6,5/5,5,5 render nearest\n");
    h=height-1;
    do{
      w=width-1;
      do{
        *((FXushort*)pix)=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // MSB Byte order
  else if(((XImage*)xim)->byte_order==MSBFirst){
    FXTRACE(TOPIC_DETAIL,"True MSB 16bpp 5,6,5/5,5,5 render nearest\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)(val>>8);
        pix[1]=(FXuchar)val;
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // LSB Byte order
  else{
    FXTRACE(TOPIC_DETAIL,"True LSB 16bpp 5,6,5/5,5,5 render nearest\n");
    h=height-1;
    do{
      w=width-1;
      do{
        val=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
        pix[0]=(FXuchar)val;
        pix[1]=(FXuchar)(val>>8);
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// True 16 bit color, dithered
void FXImage::render_true_16_dither(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-(width<<1);
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXPixel val;
  FXint w,h,d;

  // Byte order matches
  if(((XImage*)xim)->byte_order == FOX_BIGENDIAN){
    FXTRACE(TOPIC_DETAIL,"True MSB/LSB 16bpp 5,6,5/5,5,5 render dither\n");
    h=height-1;
    do{
      w=width-1;
      do{
        d=((h&3)<<2)|(w&3);
        *((FXushort*)pix)=visual->rpix[d][img[2]] | visual->gpix[d][img[1]] | visual->bpix[d][img[0]];
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // MSB Byte order
  else if(((XImage*)xim)->byte_order==MSBFirst){
    FXTRACE(TOPIC_DETAIL,"True MSB 16bpp 5,6,5/5,5,5 render dither\n");
    h=height-1;
    do{
      w=width-1;
      do{
        d=((h&3)<<2)|(w&3);
        val=visual->rpix[d][img[2]] | visual->gpix[d][img[1]] | visual->bpix[d][img[0]];
        pix[0]=(FXuchar)(val>>8);
        pix[1]=(FXuchar)val;
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }

  // LSB Byte order
  else{
    FXTRACE(TOPIC_DETAIL,"True LSB 16bpp 5,6,5/5,5,5 render dither\n");
    h=height-1;
    do{
      w=width-1;
      do{
        d=((h&3)<<2)|(w&3);
        val=visual->rpix[d][img[2]] | visual->gpix[d][img[1]] | visual->bpix[d][img[0]];
        pix[0]=(FXuchar)val;
        pix[1]=(FXuchar)(val>>8);
        img+=4;
        pix+=2;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// True 8 bit color
void FXImage::render_true_8_fast(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXint w,h;
  FXTRACE(TOPIC_DETAIL,"True MSB/LSB 8bpp render nearest\n");
  h=height-1;
  do{
    w=width-1;
    do{
      *pix=visual->rpix[1][img[2]] | visual->gpix[1][img[1]] | visual->bpix[1][img[0]];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// True 8 bit color, dithered
void FXImage::render_true_8_dither(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXint w,h,d;
  FXTRACE(TOPIC_DETAIL,"True MSB/LSB 8bpp render dither\n");
  h=height-1;
  do{
    w=width-1;
    do{
      d=((h&3)<<2)|(w&3);
      *pix=visual->rpix[d][img[2]] | visual->gpix[d][img[1]] | visual->bpix[d][img[0]];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// Render 4 bit index color mode
void FXImage::render_index_4_fast(void *xim,FXuchar *img){
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuint val,half;
  FXint w,h;
  if(((XImage*)xim)->byte_order==MSBFirst){    // MSB
    FXTRACE(TOPIC_DETAIL,"Index MSB 4bpp render nearest\n");
    h=height-1;
    do{
      w=width-1;
      half=0;
      do{
        val=visual->lut[visual->rpix[1][img[2]]+visual->gpix[1][img[1]]+visual->bpix[1][img[0]]];
        if(half) *pix++|=val;
        else *pix=val<<4;
        half^=1;
        img+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  else{                               // LSB
    FXTRACE(TOPIC_DETAIL,"Index LSB 4bpp render nearest\n");
    h=height-1;
    do{
      w=width-1;
      half=0;
      do{
        val=visual->lut[visual->rpix[1][img[2]]+visual->gpix[1][img[1]]+visual->bpix[1][img[0]]];
        if(half) *pix++|=val<<4;
        else *pix=val;
        half^=1;
        img+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// Render 4 bit index color mode
void FXImage::render_index_4_dither(void *xim,FXuchar *img){
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuint val,half,d;
  FXint w,h;
  if(((XImage*)xim)->byte_order==MSBFirst){    // MSB
    FXTRACE(TOPIC_DETAIL,"Index MSB 4bpp render dither\n");
    h=height-1;
    do{
      w=width-1;
      half=0;
      do{
        d=((h&3)<<2)|(w&3);
        val=visual->lut[visual->rpix[d][img[2]]+visual->gpix[d][img[1]]+visual->bpix[d][img[0]]];
        if(half) *pix++|=val;
        else *pix=val<<4;
        half^=1;
        img+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  else{                               // LSB
    FXTRACE(TOPIC_DETAIL,"Index LSB 4bpp render dither\n");
    h=height-1;
    do{
      w=width-1;
      half=0;
      do{
        d=((h&3)<<2)|(w&3);
        val=visual->lut[visual->rpix[d][img[2]]+visual->gpix[d][img[1]]+visual->bpix[d][img[0]]];
        if(half) *pix++|=val<<4;
        else *pix=val;
        half^=1;
        img+=4;
        }
      while(--w>=0);
      pix+=jmp;
      }
    while(--h>=0);
    }
  }


// Render 8 bit index color mode
void FXImage::render_index_8_fast(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXint w,h;
  FXTRACE(TOPIC_DETAIL,"Index MSB/LSB 8bpp render nearest\n");
  h=height-1;
  do{
    w=width-1;
    do{
      *pix=visual->lut[visual->rpix[1][img[2]]+visual->gpix[1][img[1]]+visual->bpix[1][img[0]]];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// Render 8 bit index color mode
void FXImage::render_index_8_dither(void *xim,FXuchar *img){
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXint w,h,d;
  FXTRACE(TOPIC_DETAIL,"Index MSB/LSB 8bpp render dither\n");
  h=height-1;
  do{
    w=width-1;
    do{
      d=((h&3)<<2)|(w&3);
      *pix=visual->lut[visual->rpix[d][img[2]]+visual->gpix[d][img[1]]+visual->bpix[d][img[0]]];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// Render generic N bit index color mode
void FXImage::render_index_N_fast(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"Index MSB/LSB N bpp render nearest\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->lut[visual->rpix[1][img[2]]+visual->gpix[1][img[1]]+visual->bpix[1][img[0]]]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render generic N bit index color mode
void FXImage::render_index_N_dither(void *xim,FXuchar *img){
  FXint x,y,d;
  FXTRACE(TOPIC_DETAIL,"Index MSB/LSB N bpp render dither\n");
  y=0;
  do{
    x=0;
    do{
      d=((y&3)<<2)|(x&3);
      XPutPixel(((XImage*)xim),x,y,visual->lut[visual->rpix[d][img[2]]+visual->gpix[d][img[1]]+visual->bpix[d][img[0]]]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render 8 bit gray mode
void FXImage::render_gray_8_fast(void *xim,FXuchar *img){
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXint w,h;
  FXTRACE(TOPIC_DETAIL,"Gray MSB/LSB 8bpp render nearest\n");
  h=height-1;
  do{
    w=width-1;
    do{
      *pix=visual->gpix[1][(77*img[2]+151*img[1]+29*img[0])>>8];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// Render 8 bit gray mode
void FXImage::render_gray_8_dither(void *xim,FXuchar *img){
  FXuchar *pix=(FXuchar*)((XImage*)xim)->data;
  FXuint jmp=((XImage*)xim)->bytes_per_line-width;
  FXint w,h;
  FXTRACE(TOPIC_DETAIL,"Gray MSB/LSB 8bpp render dither\n");
  h=height-1;
  do{
    w=width-1;
    do{
      *pix=visual->gpix[((h&3)<<2)|(w&3)][(77*img[2]+151*img[1]+29*img[0])>>8];
      img+=4;
      pix++;
      }
    while(--w>=0);
    pix+=jmp;
    }
  while(--h>=0);
  }


// Render generic N bit gray mode
void FXImage::render_gray_N_fast(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"Gray MSB/LSB N bpp render nearest\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->gpix[1][(77*img[2]+151*img[1]+29*img[0])>>8]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render generic N bit gray mode
void FXImage::render_gray_N_dither(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"Gray MSB/LSB N bpp render dither\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->gpix[((y&3)<<2)|(x&3)][(77*img[2]+151*img[1]+29*img[0])>>8]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render monochrome mode
void FXImage::render_mono_1_fast(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"Monochrome MSB/LSB 1bpp render nearest\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->gpix[1][(77*img[2]+151*img[1]+29*img[0])>>8]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render monochrome mode
void FXImage::render_mono_1_dither(void *xim,FXuchar *img){
  FXint x,y;
  FXTRACE(TOPIC_DETAIL,"Monochrome MSB/LSB 1bpp render dither\n");
  y=0;
  do{
    x=0;
    do{
      XPutPixel(((XImage*)xim),x,y,visual->gpix[((y&3)<<2)|(x&3)][(77*img[2]+151*img[1]+29*img[0])>>8]);
      img+=4;
      }
    while(++x<width);
    }
  while(++y<height);
  }


// Render into pixmap
void FXImage::render(){
  if(xid){
    FXbool shmi=false;
    XImage *xim=nullptr;
    XGCValues values;
    GC gc;
#ifdef HAVE_XSHM_H
    XShmSegmentInfo shminfo;
#endif

    FXTRACE(TOPIC_CREATION,"%s::render image %p\n",getClassName(),this);

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Make GC
      values.foreground=BlackPixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
      values.background=WhitePixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
      gc=XCreateGC(DISPLAY(getApp()),xid,GCForeground|GCBackground,&values);

      // Turn it on iff both supported and desired
#ifdef HAVE_XSHM_H
      if(options&IMAGE_SHMI) shmi=getApp()->shmi;
#endif

      // First try XShm
#ifdef HAVE_XSHM_H
      if(shmi){
        xim=XShmCreateImage(DISPLAY(getApp()),(Visual*)visual->visual,visual->depth,(visual->depth==1)?XYPixmap:ZPixmap,nullptr,&shminfo,width,height);
        if(!xim){ shmi=0; }
        if(shmi){
          shminfo.shmid=shmget(IPC_PRIVATE,xim->bytes_per_line*xim->height,IPC_CREAT|0777);
          if(shminfo.shmid==-1){ xim->data=nullptr; XDestroyImage(xim); xim=nullptr; shmi=0; }
          if(shmi){
            shminfo.shmaddr=xim->data=(char*)shmat(shminfo.shmid,0,0);
            shminfo.readOnly=false;
            XShmAttach(DISPLAY(getApp()),&shminfo);
            FXTRACE(TOPIC_DETAIL,"RGBPixmap XSHM attached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
            }
          }
        }
#endif

      // Try the old fashioned way
      if(!shmi){
        xim=XCreateImage(DISPLAY(getApp()),(Visual*)visual->visual,visual->depth,(visual->depth==1)?XYPixmap:ZPixmap,0,nullptr,width,height,32,0);
        if(!xim){ throw FXImageException("unable to render image"); }

        // Try create temp pixel store
        if(!allocElms(xim->data,xim->bytes_per_line*height)){ throw FXMemoryException("unable to render image"); }
        }

      // Should have succeeded
      FXASSERT(xim);

      FXTRACE(TOPIC_DETAIL,"im width = %d\n",xim->width);
      FXTRACE(TOPIC_DETAIL,"im height = %d\n",xim->height);
      FXTRACE(TOPIC_DETAIL,"im format = %s\n",xim->format==XYBitmap?"XYBitmap":xim->format==XYPixmap?"XYPixmap":"ZPixmap");
      FXTRACE(TOPIC_DETAIL,"im byte_order = %s\n",(xim->byte_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"im bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"im bitmap_bit_order = %s\n",(xim->bitmap_bit_order==MSBFirst)?"MSBFirst":"LSBFirst");
      FXTRACE(TOPIC_DETAIL,"im bitmap_pad = %d\n",xim->bitmap_pad);
      FXTRACE(TOPIC_DETAIL,"im bitmap_unit = %d\n",xim->bitmap_unit);
      FXTRACE(TOPIC_DETAIL,"im depth = %d\n",xim->depth);
      FXTRACE(TOPIC_DETAIL,"im bytes_per_line = %d\n",xim->bytes_per_line);
      FXTRACE(TOPIC_DETAIL,"im bits_per_pixel = %d\n",xim->bits_per_pixel);

      // Determine what to do
      switch(visual->getType()){
        case FXVisual::Color:
          switch(xim->bits_per_pixel){
            case 32:
              render_true_32(xim,(FXuchar*)data);
              break;
            case 24:
              render_true_24(xim,(FXuchar*)data);
              break;
            case 15:
            case 16:
              if(options&IMAGE_NEAREST)
                render_true_16_fast(xim,(FXuchar*)data);
              else
                render_true_16_dither(xim,(FXuchar*)data);
              break;
            case 8:
              if(options&IMAGE_NEAREST)
                render_true_8_fast(xim,(FXuchar*)data);
              else
                render_true_8_dither(xim,(FXuchar*)data);
              break;
            default:
              if(options&IMAGE_NEAREST)
                render_true_N_fast(xim,(FXuchar*)data);
              else
                render_true_N_dither(xim,(FXuchar*)data);
              break;
            }
          break;
        case FXVisual::Gray:
          switch(xim->bits_per_pixel){
            case 1:
              if(options&IMAGE_NEAREST)
                render_mono_1_fast(xim,(FXuchar*)data);
              else
                render_mono_1_dither(xim,(FXuchar*)data);
              break;
            case 8:
              if(options&IMAGE_NEAREST)
                render_gray_8_fast(xim,(FXuchar*)data);
              else
                render_gray_8_dither(xim,(FXuchar*)data);
              break;
            default:
              if(options&IMAGE_NEAREST)
                render_gray_N_fast(xim,(FXuchar*)data);
              else
                render_gray_N_dither(xim,(FXuchar*)data);
              break;
            }
          break;
        case FXVisual::Index:
          switch(xim->bits_per_pixel){
            case 4:
              if(options&IMAGE_NEAREST)
                render_index_4_fast(xim,(FXuchar*)data);
              else
                render_index_4_dither(xim,(FXuchar*)data);
              break;
            case 8:
              if(options&IMAGE_NEAREST)
                render_index_8_fast(xim,(FXuchar*)data);
              else
                render_index_8_dither(xim,(FXuchar*)data);
              break;
            default:
              if(options&IMAGE_NEAREST)
                render_index_N_fast(xim,(FXuchar*)data);
              else
                render_index_N_dither(xim,(FXuchar*)data);
              break;
            }
          break;
        case FXVisual::Mono:
          if(options&IMAGE_NEAREST)
            render_mono_1_fast(xim,(FXuchar*)data);
          else
            render_mono_1_dither(xim,(FXuchar*)data);
        }

      // Transfer image with shared memory
#ifdef HAVE_XSHM_H
      if(shmi){
        XShmPutImage(DISPLAY(getApp()),xid,gc,xim,0,0,0,0,width,height,False);
        XSync(DISPLAY(getApp()),False);
        FXTRACE(TOPIC_DETAIL,"RGBPixmap XSHM detached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height);
        XShmDetach(DISPLAY(getApp()),&shminfo);
        xim->data=nullptr;
        XDestroyImage(xim);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid,IPC_RMID,0);
        }
#endif

      // Transfer the image old way
      if(!shmi){
        XPutImage(DISPLAY(getApp()),xid,gc,xim,0,0,0,0,width,height);
        freeElms(xim->data);
        XDestroyImage(xim);
        }
      XFreeGC(DISPLAY(getApp()),gc);
      }
    }
  }


// Resize pixmap to the specified width and height; the data
// array is resized also, but its contents will be undefined.
void FXImage::resize(FXint w,FXint h){
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::resize(%d,%d)\n",getClassName(),w,h);
  if(width!=w || height!=h){

    // Resize device dependent pixmap
    if(xid){
      int dd=visual->getDepth();
      XFreePixmap(DISPLAY(getApp()),xid);
      xid=XCreatePixmap(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),w,h,dd);
      if(!xid){ throw FXImageException("unable to resize image"); }
      }
    }

  // Resize data array
  if(data){
    if(!(options&IMAGE_OWNED)){         // Need to own array
      if(!allocElms(data,w*h)){ throw FXMemoryException("unable to resize image"); }
      options|=IMAGE_OWNED;
      }
    else if(w*h!=width*height){
      if(!resizeElms(data,w*h)){ throw FXMemoryException("unable to resize image"); }
      }
    }

  // Remember new size
  width=w;
  height=h;
  }



// X11 has no device context handles
FXID FXImage::GetDC() const {
  return 0;
  }


// Nothing to release
int FXImage::ReleaseDC(FXID) const {
  return 0;
  }

}

#endif
