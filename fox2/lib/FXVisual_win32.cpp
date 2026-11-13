/********************************************************************************
*                                                                               *
*         V i s u a l   S y s t e m   F u n c t i o n s   ( W i n 3 2 )         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXVisual members which set up the native visual, colormap and pixel mapping; the common bookkeeping stays in FXVisual.cpp.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DEBUG     1003

using namespace FX;

/*******************************************************************************/

namespace FX {

// Deserialization
FXVisual::FXVisual():visual(nullptr),colormap(0),maxcolors(1000000),numcolors(0),numred(0),numgreen(0),numblue(0),depth(0),flags(VISUAL_DEFAULT),hint(32),type(Unknown),freemap(false){
  FXTRACE(TOPIC_CONSTRUCT,"FXVisual::FXVisual %p\n",this);
  }


// Construct
FXVisual::FXVisual(FXApp* a,FXuint flgs,FXuint hnt):FXId(a),visual(nullptr),colormap(0),maxcolors(1000000),numcolors(0),numred(0),numgreen(0),numblue(0),depth(0),flags(flgs),hint(hnt),type(Unknown),freemap(false){
  FXTRACE(TOPIC_CONSTRUCT,"FXVisual::FXVisual %p\n",this);
  }


// Pallette
struct LOGPALETTE256 {
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[257];
  };

// Bitmap info
struct BITMAPINFO256 {
  BITMAPINFOHEADER bmiHeader;
  DWORD            bmiColors[256];
  };

// Get number of bits in n
static inline FXuint findnbits(DWORD n){
  FXuint nb=0;
  while(n){nb+=(n&1);n>>=1;}
  return nb;
  }

// Make palette with first 20 colors equal to system colors
static HPALETTE createGenericPalette(){
  HPALETTE hStockPalette,hPalette;
  LOGPALETTE256 palette;
  FXint num,r,g,b;

  // We will use the stock palette
  hStockPalette=(HPALETTE)GetStockObject(DEFAULT_PALETTE);

  // Fill in first 20 entries from system color palette
  num=GetPaletteEntries(hStockPalette,0,20,palette.palPalEntry);

  // Calculate remaining 216 colors, 8 of which match the standard
  // 20 colors and 4 of which match the gray shades above
  for(r=0; r<256; r+=51){
    for(g=0; g<256; g+=51){
      for(b=0; b<256; b+=51){
        palette.palPalEntry[num].peRed=r;
        palette.palPalEntry[num].peGreen=g;
        palette.palPalEntry[num].peBlue=b;
        palette.palPalEntry[num].peFlags=0;
        num++;
        }
      }
    }

  // Fill in the rest
  palette.palVersion=0x300;
  palette.palNumEntries=num;

  // Create palette and we're done
  hPalette=CreatePalette((const LOGPALETTE*)&palette);

  // Return palette
  return hPalette;
  }

// Initialize
void FXVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      FXuint redbits,greenbits,bluebits,redmask,greenmask,bluemask;
      BITMAPINFO256 bmi;
      HBITMAP hbm;
      HDC hdc;

      // Check for palette support
      hdc=GetDC(GetDesktopWindow());

      // Check for palette mode; assume 8-bit for now
      if(GetDeviceCaps(hdc,RASTERCAPS)&RC_PALETTE){
        colormap=createGenericPalette();
        depth=8;
        numred=6;               // We have a 6x6x6 ramp, at least...
        numgreen=6;
        numblue=6;
        numcolors=256;
        freemap=true;
        type=Index;
        }

      // True color mode; find out how deep
      else{
        clearElms(&bmi,1);
        bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        // Get a device-dependent bitmap that's compatible with the
        // screen, then convert the DDB to a DIB.  We need to call GetDIBits
        // twice: the first call just fills in the BITMAPINFOHEADER; the
        // second fills in the bitfields or palette.
        hbm=CreateCompatibleBitmap(hdc,1,1);
        GetDIBits(hdc,hbm,0,1,nullptr,(LPBITMAPINFO)&bmi,DIB_RGB_COLORS);
        GetDIBits(hdc,hbm,0,1,nullptr,(LPBITMAPINFO)&bmi,DIB_RGB_COLORS);
        DeleteObject(hbm);
        if(bmi.bmiHeader.biCompression==BI_BITFIELDS){
          redmask=bmi.bmiColors[0];
          greenmask=bmi.bmiColors[1];
          bluemask=bmi.bmiColors[2];
          FXTRACE(TOPIC_DEBUG,"redmask   = %08x\n",redmask);
          FXTRACE(TOPIC_DEBUG,"greenmask = %08x\n",greenmask);
          FXTRACE(TOPIC_DEBUG,"bluemask  = %08x\n",bluemask);
          redbits=findnbits(redmask);
          greenbits=findnbits(greenmask);
          bluebits=findnbits(bluemask);
          numred=1<<redbits;
          numgreen=1<<greenbits;
          numblue=1<<bluebits;
          depth=redbits+greenbits+bluebits;
          numcolors=numred*numgreen*numblue;
          type=Color;
          }
        else{
          type=Unknown;
          }
        }
      ReleaseDC(GetDesktopWindow(),hdc);

      FXTRACE(TOPIC_DEBUG,"numred          = %d\n",numred);
      FXTRACE(TOPIC_DEBUG,"numgreen        = %d\n",numgreen);
      FXTRACE(TOPIC_DEBUG,"numblue         = %d\n",numblue);
      FXTRACE(TOPIC_DEBUG,"numcolors       = %d\n",numcolors);
      FXTRACE(TOPIC_DEBUG,"depth           = %d\n",depth);
      FXTRACE(TOPIC_DEBUG,"type            = %d\n",type);

      // Visual is realized
      xid=(FXID)1L;
      }
    }
  }


// Destroy visual
void FXVisual::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      if(freemap){ DeleteObject((HPALETTE)colormap); }
      }
    colormap=0;
    freemap=false;
    xid=0;
    }
  }


// Get pixel value for color
FXPixel FXVisual::getPixel(FXColor clr){
  return PALETTERGB(FXREDVAL(clr),FXGREENVAL(clr),FXBLUEVAL(clr));
  }


// Get color value for pixel
FXColor FXVisual::getColor(FXPixel pix){
  return PALETTEINDEX(pix);
  }

}

#endif
