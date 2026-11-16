/********************************************************************************
*                                                                               *
*                          S V G   I m a g e   O b j e c t                      *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
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
#include "FXSVGImage.h"


/*
  Notes:

  - FXSVGImage has an alpha channel.
  - The image is rasterized when the pixels are loaded; construct it with the
    target width and height to get a sharp result at that size.  A width or
    height of 1 (the default) means the natural size of the document.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXSVGImage::fileExt[]="svg";


// Suggested mime type
const FXchar FXSVGImage::mimeType[]="image/svg+xml";


// Object implementation
FXIMPLEMENT(FXSVGImage,FXImage,nullptr,0)


const FXbool FXSVGImage::supported=true;


// Initialize
FXSVGImage::FXSVGImage(FXApp* a,const FXuchar *pix,FXuint opts,FXint w,FXint h):FXImage(a,nullptr,opts,w,h){
  if(pix){
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix),strlen((const FXchar*)pix)+1);
    loadPixels(ms);
    }
  }


// Saving SVG is not supported
FXbool FXSVGImage::savePixels(FXStream&) const {
  return false;
  }


// Load pixels only
FXbool FXSVGImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadSVG(store,pixels,w,h,width>1?width:0,height>1?height:0)){
    setData(pixels,IMAGE_OWNED,w,h);
    return true;
    }
  return false;
  }


// Clean up
FXSVGImage::~FXSVGImage(){
  }

}
