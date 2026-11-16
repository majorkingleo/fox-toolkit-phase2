/********************************************************************************
*                                                                               *
*                      J P E G - X L   I n p u t / O u t p u t                  *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"
#ifdef HAVE_JXL_H
#include <jxl/version.h>
#include <jxl/decode.h>
#include <jxl/encode.h>
#endif


/*
  Notes:
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADJXL
extern FXAPI FXbool fxcheckJXL(FXStream& store);
extern FXAPI FXbool fxloadJXL(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveJXL(FXStream& store,const FXColor* data,FXint width,FXint height);
#endif


#ifdef HAVE_JXL_H

// Check if stream contains a JPEG-XL
FXbool fxcheckJXL(FXStream& store){
  if(store.direction()==FXStreamLoad){
    FXuchar h[12];
    store.load(h,12);
    store.position(-12,FXFromCurrent);
    if(h[0]==0xFF && h[1]==0x0A) return true;
    if(h[0]==0x00 && h[1]==0x00 && h[2]==0x00 && h[3]==0x0C && h[4]==0x4A && h[5]==0x58 && h[6]==0x4C && h[7]==0x20 && h[8]==0x0D && h[9]==0x0A && h[10]==0x87 && h[11]==0x0A) return true;
    }
  return false;
  }


// Load a JPEG-XL image
FXbool fxloadJXL(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  data=nullptr;
  width=0;
  height=0;
  if(store.direction()==FXStreamLoad){
    JxlDecoder* decoder=JxlDecoderCreate(nullptr);
    if(decoder){
      JxlDecoderDestroy(decoder);
      return true;
      }
    }
  return false;
  }


/*******************************************************************************/

// Save a JPEG image
FXbool fxsaveJXL(FXStream& store,const FXColor* data,FXint width,FXint height){
  if(store.direction()==FXStreamSave){
    return true;
    }
  return false;
  }

/*******************************************************************************/

#else

// Check if stream contains a JPG
FXbool fxcheckJXL(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadJXL(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  if(store.direction()==FXStreamLoad){
    static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
    static FXuchar bitmap_bits[] = {
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0xfc, 0x82, 0x02, 0x00, 0x80, 0x82, 0x02, 0x00,
       0x80, 0x44, 0x02, 0x00, 0x80, 0x44, 0x02, 0x00, 0x80, 0x28, 0x02, 0x00,
       0x80, 0x38, 0x02, 0x00, 0x80, 0x28, 0x02, 0x00, 0x80, 0x44, 0x02, 0x00,
       0x80, 0x44, 0x02, 0x00, 0x44, 0x82, 0x02, 0x00, 0x38, 0x82, 0x7e, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if(allocElms(data,32*32)){
      for(FXint p=0; p<32*32; p++){
        data[p]=color[(bitmap_bits[p>>3]>>(p&7))&1];
        }
      width=32;
      height=32;
      return true;
      }
    }
  return false;
  }


// Stub routine
FXbool fxsaveJXL(FXStream&,const FXColor*,FXint,FXint){
  return false;
  }

#endif

}
