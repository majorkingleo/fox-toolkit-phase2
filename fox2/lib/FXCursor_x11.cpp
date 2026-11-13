/********************************************************************************
*                                                                               *
*           C u r s o r   S y s t e m   F u n c t i o n s   ( X 1 1 )           *
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

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of FXCursor::create() and FXCursor::destroy(), which build and release the native cursor handle; the rest stays in FXCursor.cpp.
*/

#define TOPIC_CREATION  1001
#define DISPLAY(app)     ((Display*)((app)->display))
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

      // Mapping to standard X11 cursors
      const FXuint stock[]={XC_left_ptr,XC_left_ptr,XC_right_ptr,XC_xterm,XC_watch,XC_crosshair,XC_sb_h_double_arrow,XC_sb_v_double_arrow,XC_fleur};

      FXSTATIC_ASSERT(sizeof(FXID)>=sizeof(Cursor));

      // Building stock cursor
      if(options&CURSOR_STOCK){
        FXTRACE(TOPIC_CREATION,"%s::create: stock cursor\n",getClassName());
        xid=XCreateFontCursor(DISPLAY(getApp()),stock[options&CURSOR_STOCK]);
        }

      // Building custom cursor
      else{

        // Should have data
        if(!data){ fxerror("%s::create: cursor needs pixel data.\n",getClassName()); }

        // Let's hope it's the correct size!
        if(width>32 || height>32){ fxerror("%s::create: cursor exceeds maximum size of 32x32 pixels\n",getClassName()); }

        // We have support for color cursors and its a color cursor
#ifdef HAVE_XCURSOR_H
        if(isColor() && XcursorSupportsARGB(DISPLAY(getApp()))){
          FXTRACE(TOPIC_CREATION,"%s::create: custom color %dx%d cursor\n",getClassName(),width,height);
          XcursorImage *image=XcursorImageCreate(width,height);
          image->xhot=hotx;
          image->yhot=hoty;
          for(FXint s=0; s<width*height; s++){
#ifdef __APPLE__
            // A bug in Apple's X11 implementation has components reversed
            ((FXuchar(*)[4])image->pixels)[s][0]=((FXuchar(*)[4])data)[s][3];   // A
            ((FXuchar(*)[4])image->pixels)[s][1]=((FXuchar(*)[4])data)[s][2];   // R
            ((FXuchar(*)[4])image->pixels)[s][2]=((FXuchar(*)[4])data)[s][1];   // G
            ((FXuchar(*)[4])image->pixels)[s][3]=((FXuchar(*)[4])data)[s][0];   // B
#else
            image->pixels[s]=data[s];
#endif
            }
          xid=XcursorImageLoadCursor(DISPLAY(getApp()),image);
          XcursorImageDestroy(image);
          }

        // No support for color cursor or simple black/white cursor
        else{
#endif
          FXuchar shapebits[128];
          FXuchar maskbits[128];
          XColor  color[2];
          FXint   dstbytes=(width+7)/8;
          FXint   srcoffset=0;
          FXint   dstoffset=0;
          FXTRACE(TOPIC_CREATION,"%s::create: custom b/w %dx%d cursor\n",getClassName(),width,height);
          color[0].pixel=BlackPixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
          color[1].pixel=WhitePixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
          color[0].flags=DoRed|DoGreen|DoBlue;
          color[1].flags=DoRed|DoGreen|DoBlue;
          XQueryColors(DISPLAY(getApp()),DefaultColormap(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp()))),color,2);
          fillElms(shapebits,0,ARRAYNUMBER(shapebits));
          fillElms(maskbits,0,ARRAYNUMBER(maskbits));
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              if(((FXuchar*)(data+srcoffset+i))[3]>=128){
                maskbits[dstoffset+(i>>3)]|=(1<<(i&7));
                if(DARKCOLOR(((FXuchar*)(data+srcoffset+i))[2],((FXuchar*)(data+srcoffset+i))[1],((FXuchar*)(data+srcoffset+i))[0])) shapebits[dstoffset+(i>>3)]|=(1<<(i&7));
                }
              }
            srcoffset+=width;
            dstoffset+=dstbytes;
            }
          Pixmap srcpix=XCreateBitmapFromData(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),(char*)shapebits,width,height);
          if(!srcpix){ throw FXImageException("unable to create cursor"); }
          Pixmap mskpix=XCreateBitmapFromData(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),(char*)maskbits,width,height);
          if(!mskpix){ throw FXImageException("unable to create cursor"); }
          xid=XCreatePixmapCursor(DISPLAY(getApp()),srcpix,mskpix,&color[0],&color[1],hotx,hoty);
          XFreePixmap(DISPLAY(getApp()),srcpix);
          XFreePixmap(DISPLAY(getApp()),mskpix);
#ifdef HAVE_XCURSOR_H
          }
#endif
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
      XFreeCursor(DISPLAY(getApp()),xid);
      }
    xid=0;
    }
  }

}

#endif
