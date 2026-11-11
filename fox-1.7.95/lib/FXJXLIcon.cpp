/********************************************************************************
*                                                                               *
*                       J P E G - X L   I c o n   O b j e c t                   *
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
#include "FXJXLIcon.h"


/*
  Notes:
  - Requires JPEG-XL library.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXJXLIcon::fileExt[]="jxl";


// Suggested mime type
const FXchar FXJXLIcon::mimeType[]="image/jxl";


// Object implementation
FXIMPLEMENT(FXJXLIcon,FXIcon,nullptr,0)


#ifdef HAVE_JXL_H
const FXbool FXJXLIcon::supported=true;
#else
const FXbool FXJXLIcon::supported=false;
#endif


// Initialize
FXJXLIcon::FXJXLIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix));
    loadPixels(ms);
    }
  }


// Save pixels only
FXbool FXJXLIcon::savePixels(FXStream& store) const {
  if(fxsaveJXL(store,data,width,height)){
    return true;
    }
  return false;
  }


// Load pixels only
FXbool FXJXLIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadJXL(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Clean up
FXJXLIcon::~FXJXLIcon(){
  }

}
