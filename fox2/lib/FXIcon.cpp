/********************************************************************************
*                                                                               *
*                               I c o n - O b j e c t                           *
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


/*
  Notes:
  - Debug the render function between different hosts.
  - Option to guess alpha color from corners.
  - The shape pixmap [X11] actually contains TWO shapes:- the
    shape mask (transparency) as well as the etch mask (gray out&transparency).
    We do this because most icons are 16x16 or so, and we can fit two of them
    side-by-side in the space of one:- because the bitmaps are typically
    padded to a width of 32 anyway :-)  Either way, the overhead for the
    header will be smaller.
  - When we make FXImage always to have an alpha channel, add method to set
    alpha channel to transparent when color is equal to alpha-color.
    This simplifies the shape/etch mask procedure.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001


using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXIcon,FXImage,nullptr,0)


// For deserialization
FXIcon::FXIcon():shape(0),etch(0),transp(0),thresh(382){
  }


// Initialize nicely
FXIcon::FXIcon(FXApp* a,const FXColor *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXImage(a,pix,opts,w,h),shape(0),etch(0),transp(clr),thresh(382){
  FXTRACE(TOPIC_CONSTRUCT,"FXIcon::FXIcon %p\n",this);
  }


// Guess alpha color based on corners; the initial guess is standard GUI color
FXColor FXIcon::guesstransp() const {
  FXColor guess=FXRGB(192,192,192);
  FXint best,t;
  FXColor color[4];
  if(data && 0<width && 0<height){
    best=-1;
    color[0]=getPixel(0,0);
    color[1]=getPixel(width-1,0);
    color[2]=getPixel(0,height-1);
    color[3]=getPixel(width-1,height-1);
    if((t=((color[0]==color[1])+(color[0]==color[2])+(color[0]==color[3])))>best){ guess=color[0]; best=t; }
    if((t=((color[1]==color[2])+(color[1]==color[3])+(color[1]==color[0])))>best){ guess=color[1]; best=t; }
    if((t=((color[2]==color[3])+(color[2]==color[0])+(color[2]==color[1])))>best){ guess=color[2]; best=t; }
    if((t=((color[3]==color[0])+(color[3]==color[1])+(color[3]==color[2])))>best){ guess=color[3]; }
    }
  return guess;
  }


// Determine threshold for etch mask
FXshort FXIcon::guessthresh() const {
  FXshort guess=382;
  if(data && 0<width && 0<height){
    FXint med=(width*height)>>1;
    FXint i,j,cum;
    FXint frequency[766];
    clearElms(frequency,ARRAYNUMBER(frequency));
    for(i=0; i<width*height; ++i){
      frequency[((const FXuchar*)(data+i))[2]+((const FXuchar*)(data+i))[1]+((const FXuchar*)(data+i))[0]]++;
      }
    for(i=0,cum=0; i<766; ++i){
      if((cum+=frequency[i])>=med) break;
      }
    for(j=765,cum=0; j>0; --j){
      if((cum+=frequency[j])>=med) break;
      }
    guess=((i+j+1)>>1)+1;               // Fanglin Zhu: raise threshold by one in case of single-color image
    }
  return guess;
  }


// Detach icon
void FXIcon::detach(){
  visual->detach();
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    shape=0;
    etch=0;
    xid=0;
    }
  }


// Clean up
FXIcon::~FXIcon(){
  FXTRACE(TOPIC_CONSTRUCT,"FXIcon::~FXIcon %p\n",this);
  destroy();
  }

}
