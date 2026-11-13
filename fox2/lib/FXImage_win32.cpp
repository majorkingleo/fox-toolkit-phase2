/********************************************************************************
*                                                                               *
*                             Image   ( W i n 3 2 )                             *
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

#if defined(WIN32)

/*
  Notes:
  - Platform part of the image: native bitmap creation, restore and render.
*/

#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Return the device context; the image already selected into it
FXID FXImage::GetDC() const {
  HDC hdc=::CreateCompatibleDC(nullptr);
  SelectObject(hdc,(HBITMAP)xid);
  return hdc;
  }


// Release it (no-op)
int FXImage::ReleaseDC(FXID hdc) const {
  return ::DeleteDC((HDC)hdc);
  }


// Create image
void FXImage::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Initialize visual
      visual->create();


      FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(HBITMAP));

      // Create a bitmap compatible with current display
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=CreateCompatibleBitmap(hdc,FXMAX(width,1),FXMAX(height,1));
      ::ReleaseDC(GetDesktopWindow(),hdc);


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
      DeleteObject(xid);
      }
    xid=0;
    }
  }


// Restore client-side pixel buffer from image
void FXImage::restore(){
  if(xid){
    FXint size,bytes_per_line,skip,x,y;
    FXuchar *pix,*img;
    FXuchar *pixels;
    BITMAPINFO bmi;
    HDC hdcmem;

    FXTRACE(TOPIC_CREATION,"%s::restore image %p\n",getClassName(),this);

    // Check for legal size
    if(width<1 || height<1){ fxerror("%s::restore: illegal image size %dx%d.\n",getClassName(),width,height); }

    // Make array for data if needed
    if(!data){
      size=width*height;
      if(!allocElms(data,size)){ throw FXMemoryException("unable to restore image"); }
      options|=IMAGE_OWNED;
      }

    // Got local buffer to receive into
    if(data){

      // Set up the bitmap info
      bytes_per_line=(width*3+3)/4*4;
      skip=bytes_per_line-width*3;

      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=-height; // Negative heights means upside down!
      bmi.bmiHeader.biPlanes=1;
      bmi.bmiHeader.biBitCount=24;
      bmi.bmiHeader.biCompression=BI_RGB;
      bmi.bmiHeader.biSizeImage=0;
      bmi.bmiHeader.biXPelsPerMeter=0;
      bmi.bmiHeader.biYPelsPerMeter=0;
      bmi.bmiHeader.biClrUsed=0;
      bmi.bmiHeader.biClrImportant=0;

      // DIB format pads to multiples of 4 bytes...
      pixels=(FXuchar*)VirtualAlloc(0,bytes_per_line*height,MEM_COMMIT,PAGE_READWRITE);
      if(!pixels){ throw FXMemoryException("unable to restore image"); }

      // Make device context
      hdcmem=::CreateCompatibleDC(nullptr);
      if(!GetDIBits(hdcmem,(HBITMAP)xid,0,height,pixels,&bmi,DIB_RGB_COLORS)){
        throw FXImageException("unable to restore image");
        }

      // Stuff it into our own data structure
      for(y=0,img=(FXuchar*)data,pix=pixels; y<height; y++){
        for(x=0; x<width; x++){
          img[0]=pix[0];
          img[1]=pix[1];
          img[2]=pix[2];
          img[3]=255;
          img+=4;
          pix+=3;
          }
        pix+=skip;
        }
      VirtualFree(pixels,0,MEM_RELEASE);
      ::DeleteDC(hdcmem);
      }
    }
  }


// Render into pixmap
void FXImage::render(){
  if(xid){
    FXint bytes_per_line,skip,h,w;
    FXuchar *src,*dst;
    BITMAPINFO bmi;
    FXuchar *pixels;
    HDC hdcmem;

    FXTRACE(TOPIC_CREATION,"%s::render %p\n",getClassName(),this);

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Set up the bitmap info
      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=height;
      bmi.bmiHeader.biPlanes=1;
      bmi.bmiHeader.biBitCount=24;
      bmi.bmiHeader.biCompression=BI_RGB;
      bmi.bmiHeader.biSizeImage=0;
      bmi.bmiHeader.biXPelsPerMeter=0;
      bmi.bmiHeader.biYPelsPerMeter=0;
      bmi.bmiHeader.biClrUsed=0;
      bmi.bmiHeader.biClrImportant=0;

      // DIB format pads to multiples of 4 bytes...
      bytes_per_line=(width*3+3)&~3;
      pixels=(FXuchar*)VirtualAlloc(0,bytes_per_line*height,MEM_COMMIT,PAGE_READWRITE);
      if(!pixels){ throw FXMemoryException("unable to render image"); }
      skip=-bytes_per_line-width*3;
      src=(FXuchar*)data;
      dst=pixels+height*bytes_per_line+width*3;
      h=height;
      do{
        dst+=skip;
        w=width;
        do{
          dst[0]=src[0];
          dst[1]=src[1];
          dst[2]=src[2];
          src+=4;
          dst+=3;
          }
        while(--w);
        }
      while(--h);
      // The MSDN documentation for SetDIBits() states that "the device context
      // identified by the (first) parameter is used only if the DIB_PAL_COLORS
      // constant is set for the (last) parameter". This may be true, but under
      // Win95 you must pass in a non-NULL hdc for the first parameter; otherwise
      // this call to SetDIBits() will fail (in contrast, it works fine under
      // Windows NT if you pass in a NULL hdc).
      hdcmem=::CreateCompatibleDC(nullptr);
      if(!SetDIBits(hdcmem,(HBITMAP)xid,0,height,pixels,&bmi,DIB_RGB_COLORS)){
//    if(!StretchDIBits(hdcmem,0,0,width,height,0,0,width,height,pixels,&bmi,DIB_RGB_COLORS,SRCCOPY)){
        throw FXImageException("unable to render image");
        }
      GdiFlush();
      VirtualFree(pixels,0,MEM_RELEASE);
      ::DeleteDC(hdcmem);
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
      DeleteObject(xid);
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=CreateCompatibleBitmap(hdc,w,h);
      ::ReleaseDC(GetDesktopWindow(),hdc);
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

}

#endif
