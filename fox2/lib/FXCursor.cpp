/********************************************************************************
*                                                                               *
*                         C u r s o r - O b j e c t                             *
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


/*
  Notes:
  - Cursor size should be less than or equal to 32x32; limitation in Windows!
  - Need standard glyph for "invisible" cursor.
  - Keep hotx and hoty INSIDE the cursor glyph!!
  - Thanks Niall Douglas <s_sourceforge@nedprod.com> for the changes for
    alpha-blended cursors.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

#define CURSOR_MASK      (CURSOR_KEEP)

using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXbool fxloadXBM(FXColor*& data,const FXuchar *pixels,const FXuchar *mask,FXint width,FXint height);


// Object implementation
FXIMPLEMENT(FXCursor,FXId,nullptr,0)


// Deserialization
FXCursor::FXCursor(){
  data=nullptr;
  width=0;
  height=0;
  hotx=0;
  hoty=0;
  options=CURSOR_ARROW;
  }


// Make stock cursor
FXCursor::FXCursor(FXApp* a,FXStockCursor curid):FXId(a){
  FXTRACE(TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this);
  data=nullptr;
  width=0;
  height=0;
  hotx=0;
  hoty=0;
  options=curid;
  }


// Make cursor from source and mask
FXCursor::FXCursor(FXApp* a,const FXuchar* src,const FXuchar* msk,FXint w,FXint h,FXint hx,FXint hy):FXId(a){
  FXTRACE(TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this);
  fxloadXBM(data,src,msk,w,h);
  width=w;
  height=h;
  hotx=FXCLAMP(0,hx,width-1);
  hoty=FXCLAMP(0,hy,height-1);
  options=CURSOR_OWNED;
  }


// Make cursor from FXColor pixels
FXCursor::FXCursor(FXApp* a,const FXColor *pix,FXint w,FXint h,FXint hx,FXint hy):FXId(a){
  FXTRACE(TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this);
  data=const_cast<FXColor*>(pix);
  width=w;
  height=h;
  hotx=FXCLAMP(0,hx,width-1);
  hoty=FXCLAMP(0,hy,height-1);
  options=0;
  }


// Return true if color cursor
FXbool FXCursor::isColor() const {
  if(data){
    for(FXint i=width*height-1; 0<=i; i--){
      if(data[i]!=FXColors::Black && data[i]!=FXColors::White && FXALPHAVAL(data[i])!=0) return true;
      }
    }
  return false;
  }


// Detach cursor
void FXCursor::detach(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    xid=0;
    }
  }


// Release pixels buffer if it was owned
void FXCursor::release(){
  if(options&CURSOR_OWNED){
    options&=~CURSOR_OWNED;
    freeElms(data);
    }
  data=nullptr;
  }


// Change options
void FXCursor::setOptions(FXuint opts){
  options=(options&~CURSOR_MASK) | (opts&CURSOR_MASK);
  }


// Set pixel data ownership flag
void FXCursor::setOwned(FXbool owned){
  options^=((0-owned)^options)&CURSOR_OWNED;
  }


// Get pixel ownership flag
FXbool FXCursor::isOwned() const {
  return (options&CURSOR_OWNED)!=0;
  }


// Save pixel data only
FXbool FXCursor::savePixels(FXStream& store) const {
  FXuint size=width*height;
  store.save(data,size);
  return true;
  }


// Load pixel data only
FXbool FXCursor::loadPixels(FXStream& store){
  FXuint size=width*height;
  if(options&CURSOR_OWNED){freeElms(data);}
  if(!allocElms(data,size)) return false;
  store.load(data,size);
  options|=CURSOR_OWNED;
  return true;
  }


// Save cursor to stream
void FXCursor::save(FXStream& store) const {
  FXuchar haspixels=(data!=nullptr);
  FXId::save(store);
  store << width << height << hotx << hoty;
  store << options;
  store << haspixels;
  if(haspixels) savePixels(store);
  }


// Load cursor from stream
void FXCursor::load(FXStream& store){
  FXuchar haspixels;
  FXId::load(store);
  store >> width >> height >> hotx >> hoty;
  store >> options;
  store >> haspixels;
  if(haspixels) loadPixels(store);
  }


// Clean up
FXCursor::~FXCursor(){
  FXTRACE(TOPIC_CONSTRUCT,"FXCursor::~FXCursor %p\n",this);
  destroy();
  if(options&CURSOR_OWNED){freeElms(data);}
  data=(FXColor *)-1L;
  }

}
