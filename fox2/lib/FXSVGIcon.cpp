/********************************************************************************
*                                                                               *
*                           S V G   I c o n   O b j e c t                       *
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
#include "FXSVGIcon.h"


/*
  Notes:

  - The icon is rasterized when the pixels are loaded; construct it with the
    target width and height to get a sharp result at that size.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXSVGIcon::fileExt[]="svg";


// Suggested mime type
const FXchar FXSVGIcon::mimeType[]="image/svg+xml";


// Object implementation
FXIMPLEMENT(FXSVGIcon,FXIcon,nullptr,0)


const FXbool FXSVGIcon::supported=true;


// Initialize
FXSVGIcon::FXSVGIcon(FXApp* a,const FXuchar *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXIcon(a,nullptr,clr,opts,w,h),document(){
  if(pix){
    document.assign(reinterpret_cast<const FXchar*>(pix));
    FXMemoryStream ms(FXStreamLoad,const_cast<FXuchar*>(pix),strlen((const FXchar*)pix)+1);
    loadPixels(ms);
    }
  }


// Saving SVG is not supported
FXbool FXSVGIcon::savePixels(FXStream&) const {
  return false;
  }


// Load pixels only
FXbool FXSVGIcon::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadSVG(store,pixels,w,h,width>1?width:0,height>1?height:0)){
    setData(pixels,IMAGE_OWNED,w,h);
    if(options&IMAGE_ALPHAGUESS) setTransparentColor(guesstransp());
    if(options&IMAGE_THRESGUESS) setThresholdValue(guessthresh());
    return true;
    }
  return false;
  }


// Return the icon for this display scale, rasterized again at its real size
FXIcon* FXSVGIcon::getScaledIcon(FXdouble scale){
  FXint pct=(FXint)(scale*100.0+0.5);
  if(percent!=0 || document.empty() || pct<=0 || pct==100) return this;

  // Already made for this scale
  for(FXSVGIcon* icn=static_cast<FXSVGIcon*>(scaled); icn; icn=static_cast<FXSVGIcon*>(icn->sibling)){
    if(icn->percent==pct) return icn;
    }

  // Size of the copy
  FXint w=FXMAX((width*pct+50)/100,1);
  FXint h=FXMAX((height*pct+50)/100,1);
  if(w==width && h==height) return this;

  // Rasterize the document at the scaled size; no pixel filtering is involved
  FXSVGIcon* icn=new FXSVGIcon(getApp(),reinterpret_cast<const FXuchar*>(document.text()),transp,options,w,h);
  if(icn->getData()==nullptr){
    delete icn;
    return this;
    }
  icn->percent=pct;
  icn->thresh=thresh;
  icn->create();
  icn->sibling=scaled;
  scaled=icn;
  return icn;
  }


// Clean up
FXSVGIcon::~FXSVGIcon(){
  }

}
