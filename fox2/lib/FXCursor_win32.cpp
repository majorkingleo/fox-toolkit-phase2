/********************************************************************************
*                                                                               *
*         C u r s o r   S y s t e m   F u n c t i o n s   ( W i n 3 2 )         *
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
#include "FXMutex.h"
#include "FXElement.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXColors.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXException.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of FXCursor::create() and FXCursor::destroy(), which build and release the native cursor handle; the rest stays in FXCursor.cpp.
*/

#define TOPIC_CREATION  1001
#define DARKCOLOR(r,g,b) (((r)+(g)+(b))<382)
#define CURSOR_STOCK     255

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create cursor
void FXCursor::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

      // Mapping to standard WIN32 cursors
      const LPCTSTR stock[]={IDC_ARROW,IDC_ARROW,IDC_ARROW,IDC_IBEAM,IDC_WAIT,IDC_CROSS,IDC_SIZENS,IDC_SIZEWE,IDC_SIZEALL};

      FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(HCURSOR));

      // Building stock cursor
      if(options&CURSOR_STOCK){
        FXTRACE(TOPIC_CREATION,"%s::create: stock cursor\n",getClassName());
        xid=LoadCursor(nullptr,stock[options&CURSOR_STOCK]);
        }

      // Building custom cursor
      else{

        // Should have data
        if(!data){ fxerror("%s::create: cursor needs pixel data.\n",getClassName()); }

        // Let's hope it's the correct size!
        if(width>32 || height>32){ fxerror("%s::create: cursor exceeds maximum size of 32x32 pixels\n",getClassName()); }

        // We have support for color cursors (WindowXP and up) and its a color cursor
        OSVERSIONINFO osvi={sizeof(OSVERSIONINFO)};
        if(isColor() && GetVersionEx((OSVERSIONINFO*)&osvi) && (osvi.dwPlatformId==VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion==5) && (osvi.dwMinorVersion>=0 || osvi.dwMajorVersion>5)){
          const BITMAPV4HEADER bi={sizeof(BITMAPV4HEADER),32,-32,1,32,BI_BITFIELDS,0,0,0,0,0,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000,0,{{0,0,0},{0,0,0},{0,0,0}},0,0,0};
          FXTRACE(TOPIC_CREATION,"%s::create: custom color %dx%d cursor\n",getClassName(),width,height);

          // Make a DIB
          void *imgdata=0;
          HDC hdc=GetDC(nullptr);
          HBITMAP himage=CreateDIBSection(hdc,(BITMAPINFO*)&bi,DIB_RGB_COLORS,&imgdata,nullptr,0);
          ReleaseDC(nullptr,hdc);
          if(!himage){ throw FXImageException("unable to create cursor"); }

          // Fill in data
          FXColor *imgptr=(FXColor*)imgdata;
          FXColor *srcimgptr=data;
          clearElms(imgptr,32*32);
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              *imgptr++=*srcimgptr++;
              }
            imgptr+=(32-width);
            }

          // Strawman mask bitmap
          HBITMAP hmask=CreateBitmap(32,32,1,1,nullptr);
          if(!hmask){ throw FXImageException("unable to create cursor"); }

          // Create cursor
          ICONINFO iconinfo;
          iconinfo.fIcon=false;
          iconinfo.xHotspot=hotx;
          iconinfo.yHotspot=hoty;
          iconinfo.hbmMask=hmask;
          iconinfo.hbmColor=himage;
          xid=CreateIconIndirect(&iconinfo);

          // No longer needed
          DeleteObject(hmask);
          DeleteObject(himage);
          }

        // No support for color cursor or simple black/white cursor
        else{
          FXuchar tmpxor[128];
          FXuchar tmpand[128];
          FXint   srcoffset=0;
          FXint   dstoffset=0;
          FXTRACE(TOPIC_CREATION,"%s::create: custom b/w %dx%d cursor\n",getClassName(),width,height);
          fillElms(tmpand,0xff,ARRAYNUMBER(tmpand));
          fillElms(tmpxor,0x00,ARRAYNUMBER(tmpxor));
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              if(((FXuchar*)(data+srcoffset+i))[3]>=128){
                tmpand[dstoffset+(i>>3)]&=~(128>>(i&7));
                if(!DARKCOLOR(((FXuchar*)(data+srcoffset+i))[2],((FXuchar*)(data+srcoffset+i))[1],((FXuchar*)(data+srcoffset+i))[0])){
                  tmpxor[dstoffset+(i>>3)]|=(128>>(i&7));
                  }
                }
              }
            srcoffset+=width;
            dstoffset+=4;
            }
          xid=CreateCursor((HINSTANCE)(getApp()->display),hotx,hoty,32,32,tmpand,tmpxor);
          }
        }

      // Were we successful?
      if(!xid){ throw FXImageException("unable to create cursor"); }

      // Release pixel buffer
      if(!(options&CURSOR_KEEP)) release();
      }
    }
  }


// Destroy cursor
void FXCursor::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      DestroyCursor((HCURSOR)xid);
      }
    xid=0;
    }
  }

}

#endif
