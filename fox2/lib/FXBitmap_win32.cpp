/********************************************************************************
*                                                                               *
*                           Bitmap   ( W i n 3 2 )                              *
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

#if defined(WIN32)

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

      xid=CreateBitmap(FXMAX(width,1),FXMAX(height,1),1,1,nullptr);

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
      DeleteObject(xid);
      }
    xid=0;
    }
  }




struct BITMAPINFO256 {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[256];
  };


// Restore client-side pixel buffer from bitmap
void FXBitmap::restore(){
  if(xid){

    FXTRACE(TOPIC_CREATION,"%s::restore image %p\n",getClassName(),this);

    // Check for legal size
    if(width<1 || height<1){ fxerror("%s::restore: illegal image size %dx%d.\n",getClassName(),width,height); }

    // Make array for data if needed
    if(!data){
      if(!callocElms(data,height*bytewidth)){ throw FXMemoryException("unable to restore image"); }
      options|=BITMAP_OWNED;
      }

    // Got local buffer to receive into
    if(data){
      FXuchar *pixels,*p,*q;;
      FXint x,y;

      // Bytes per line, rounded to nearest DWORD
      FXint bytes_per_line=((width+31)&~31)>>3;

      // Set up the bitmap info
      BITMAPINFO256 bmi;
      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=-height;   // Negative heights means upside down!
      bmi.bmiHeader.biPlanes=1;
      bmi.bmiHeader.biBitCount=1;
      bmi.bmiHeader.biCompression=BI_RGB;
      bmi.bmiHeader.biSizeImage=0;
      bmi.bmiHeader.biXPelsPerMeter=0;
      bmi.bmiHeader.biYPelsPerMeter=0;
      bmi.bmiHeader.biClrUsed=0;
      bmi.bmiHeader.biClrImportant=0;
      bmi.bmiColors[0].rgbBlue=0;
      bmi.bmiColors[0].rgbGreen=0;
      bmi.bmiColors[0].rgbRed=0;
      bmi.bmiColors[0].rgbReserved=0;
      bmi.bmiColors[1].rgbBlue=255;
      bmi.bmiColors[1].rgbGreen=255;
      bmi.bmiColors[1].rgbRed=255;
      bmi.bmiColors[1].rgbReserved=0;

      // DIB format pads to multiples of 4 bytes...
      if(!allocElms(pixels,height*bytes_per_line)){ throw FXImageException("unable to restore image"); }

      // Make device context
      HDC hdcmem=::CreateCompatibleDC(nullptr);
      if(!GetDIBits(hdcmem,(HBITMAP)xid,0,height,pixels,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        throw FXImageException("unable to restore image");
        }

      // Fill our own data from pixels
      for(y=0,p=pixels,q=data; y<height; y++){
        for(x=0; x<bytewidth; x++){
          q[x]=~reverse8(p[x]);
          }
        q+=bytewidth;
        p+=bytes_per_line;
        }

      // Clean up
      ::DeleteDC(hdcmem);
      freeElms(pixels);
      }
    }
  }


// Render into pixmap
void FXBitmap::render(){
  if(xid){

    FXTRACE(TOPIC_CREATION,"%s::render bitmap %p\n",getClassName(),this);

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){
      FXuchar *pixels,*p,*q;
      FXint x,y;

      // Bytes per line, rounded to nearest DWORD
      FXint bytes_per_line=((width+31)&~31)>>3;

      // Set up the bitmap info
      BITMAPINFO256 bmi;
      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=-height;   // Negative heights means upside down!
      bmi.bmiHeader.biPlanes=1;
      bmi.bmiHeader.biBitCount=1;
      bmi.bmiHeader.biCompression=0;
      bmi.bmiHeader.biSizeImage=0;
      bmi.bmiHeader.biXPelsPerMeter=0;
      bmi.bmiHeader.biYPelsPerMeter=0;
      bmi.bmiHeader.biClrUsed=0;
      bmi.bmiHeader.biClrImportant=0;
      bmi.bmiColors[0].rgbBlue=0;
      bmi.bmiColors[0].rgbGreen=0;
      bmi.bmiColors[0].rgbRed=0;
      bmi.bmiColors[0].rgbReserved=0;
      bmi.bmiColors[1].rgbBlue=255;
      bmi.bmiColors[1].rgbGreen=255;
      bmi.bmiColors[1].rgbRed=255;
      bmi.bmiColors[1].rgbReserved=0;

      // Fill temp array
      if(!callocElms(pixels,height*bytes_per_line)){ throw FXMemoryException("unable to render bitmap"); }

      // Fill pixels from our own data
      for(y=0,p=pixels,q=data; y<height; y++){
        for(x=0; x<bytewidth; x++){
          p[x]=~reverse8(q[x]);
          }
        q+=bytewidth;
        p+=bytes_per_line;
        }

      // Get memory device context
      HDC hdcmem=::CreateCompatibleDC(nullptr);
      if(!SetDIBits(hdcmem,(HBITMAP)xid,0,height,pixels,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        throw FXImageException("unable to render bitmap");
        }

      // Push to GDI
      ::GdiFlush();

      // Clean up
      ::DeleteDC(hdcmem);
      freeElms(pixels);
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

    // Delete old bitmap
    DeleteObject(xid);

    // Create a bitmap compatible with current display
    xid=CreateBitmap(w,h,1,1,nullptr);
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



// Get the image's device context
FXID FXBitmap::GetDC() const {
  HDC hdc=::CreateCompatibleDC(nullptr);
  SelectObject(hdc,(HBITMAP)xid);
  return hdc;
  }


// Release it (no-op)
int FXBitmap::ReleaseDC(FXID hdc) const {
  return ::DeleteDC((HDC)hdc);
  }


}

#endif
