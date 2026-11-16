/********************************************************************************
*                                                                               *
*                     J P E G - X L   I m a g e   O b j e c t                   *
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
#include "FXStream.h"
#include "FXMemoryStream.h"
#include "FXJXLImage.h"


/*
  Notes:
  - Requires JPEG-XL library.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXJXLImage::fileExt[]="jxl";


// Suggested mime type
const FXchar FXJXLImage::mimeType[]="image/jxl";


// Object implementation
FXIMPLEMENT(FXJXLImage,FXImage,nullptr,0)


#ifdef HAVE_JXL_H
const FXbool FXJXLImage::supported=true;
#else
const FXbool FXJXLImage::supported=false;
#endif


// Initialize
FXJXLImage::FXJXLImage(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save the pixels only
FXbool FXJXLImage::savePixels(FXStream& store) const {
  if(fxsaveJXL(store,data,width,height)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXJXLImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadJXL(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXJXLImage::~FXJXLImage(){
  }

}
