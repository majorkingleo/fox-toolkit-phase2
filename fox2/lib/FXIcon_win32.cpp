/********************************************************************************
*                                                                               *
*                             Icon   ( W i n 3 2 )                              *
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

#if defined(WIN32)

/*
  Notes:

  - Windows icon: GDI bitmaps and masks, rendering and resizing.
*/

#define TOPIC_CREATION  1001
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

      // Create a memory DC compatible with current display
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=::CreateCompatibleBitmap(hdc,FXMAX(width,1),FXMAX(height,1));
      ::ReleaseDC(GetDesktopWindow(),hdc);
      if(!xid){ fxerror("%s::create: unable to create image.\n",getClassName()); }

      // Make shape bitmap
      shape=::CreateBitmap(FXMAX(width,1),FXMAX(height,1),1,1,nullptr);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch bitmap
      etch=::CreateBitmap(FXMAX(width,1),FXMAX(height,1),1,1,nullptr);
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
      ::DeleteObject(shape);
      ::DeleteObject(etch);
      ::DeleteObject(xid);
      }
    shape=0;
    etch=0;
    xid=0;
    }
  }


struct BITMAPINFO2 {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[2];
  };


// Render Icon MS-Windows
void FXIcon::render(){
  if(xid){
    FXint bytes_per_line,x,y;
    FXuchar *msk,*ets;
    FXColor *img;
    FXuchar *maskdata;
    FXuchar *etchdata;
    BITMAPINFO2 bmi;
    HDC hdcmsk;

    FXTRACE(TOPIC_CREATION,"%s::render %p\n",getClassName(),this);

    // Render the image (color) pixels as usual
    FXImage::render();

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Set up the bitmap info
      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=height;
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

      // Allocate temp bit buffer
      bytes_per_line=((width+31)&~31)>>3;
      callocElms(maskdata,height*bytes_per_line);
      callocElms(etchdata,height*bytes_per_line);

      msk=maskdata+height*bytes_per_line;
      ets=etchdata+height*bytes_per_line;
      if(options&IMAGE_OPAQUE){                 // Opaque image
        img=data;
        for(y=0; y<height; y++){
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        img=data;
        for(y=0; y<height; y++){
          msk-=bytes_per_line;
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            if(img[x]==transp){ msk[x>>3]|=0x80>>(x&7); ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }
      else{                                     // Transparency channel
        img=data;
        for(y=0; y<height; y++){
          msk-=bytes_per_line;
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            if(((FXuchar*)(img+x))[3]==0){ msk[x>>3]|=0x80>>(x&7); ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }

      // The MSDN documentation for SetDIBits() states that "the device context
      // identified by the (first) parameter is used only if the DIB_PAL_COLORS
      // constant is set for the (last) parameter". This may be true, but under
      // Win95 you must pass in a non-NULL hdc for the first parameter; otherwise
      // this call to SetDIBits() will fail (in contrast, it works fine under
      // Windows NT if you pass in a NULL hdc).
      hdcmsk=::CreateCompatibleDC(nullptr);

      // Set mask data
      if(!::SetDIBits(hdcmsk,(HBITMAP)shape,0,height,maskdata,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        fxerror("%s::render: unable to render pixels\n",getClassName());
        }

      // Set etch data
      if(!::SetDIBits(hdcmsk,(HBITMAP)etch,0,height,etchdata,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        fxerror("%s::render: unable to render pixels\n",getClassName());
        }
      freeElms(maskdata);
      freeElms(etchdata);
      ::GdiFlush();

      // We AND the image with the mask, then we can do faster and more
      // flicker-free icon painting later using the `black source' method
      HBITMAP hmsk=(HBITMAP)::SelectObject(hdcmsk,(HBITMAP)shape);
      HDC hdcmem=::CreateCompatibleDC(nullptr);
      HBITMAP hbmp=(HBITMAP)::SelectObject(hdcmem,(HBITMAP)xid);
      ::SetBkColor(hdcmem,RGB(0,0,0));                // 1 -> black
      ::SetTextColor(hdcmem,RGB(255,255,255));        // 0 -> white
      ::BitBlt(hdcmem,0,0,width,height,hdcmsk,0,0,SRCAND);
      ::SelectObject(hdcmem,hbmp);
      ::SelectObject(hdcmsk,hmsk);
      ::DeleteDC(hdcmem);
      ::DeleteDC(hdcmsk);
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
      // Delete old bitmaps
      ::DeleteObject(xid);
      ::DeleteObject(shape);
      ::DeleteObject(etch);

      // Create a bitmap compatible with current display
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=::CreateCompatibleBitmap(hdc,w,h);
      ::ReleaseDC(GetDesktopWindow(),hdc);
      if(!xid){ fxerror("%s::resize: unable to resize image.\n",getClassName()); }

      // Make shape bitmap
      shape=::CreateBitmap(w,h,1,1,nullptr);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch bitmap
      etch=::CreateBitmap(w,h,1,1,nullptr);
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
