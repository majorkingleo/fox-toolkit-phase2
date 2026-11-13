/********************************************************************************
*                                                                               *
*                        Compose context   ( W i n 3 2 )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXFont.h"
#include "FXComposeContext.h"
#include "FXException.h"

#if defined(WIN32)

/*
  Notes:

  - Windows compose context: the window itself is the input context.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Deserialization
FXComposeContext::FXComposeContext():window(nullptr),message(0){
  FXTRACE(TOPIC_CONSTRUCT,"FXComposeContext::FXComposeContext %p\n",this);
  }


// Create input context
FXComposeContext::FXComposeContext(FXApp* a,FXWindow* win,FXSelector sel):FXId(a),window(win),message(sel){
  FXTRACE(TOPIC_CONSTRUCT,"FXComposeContext::FXComposeContext %p\n",this);
  }


// Realize the input context
void FXComposeContext::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      xid=window->id();
      }
    }
  }


// Unrealize the input context
void FXComposeContext::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      ///////
      }
    }
  }


// Set focus to it
void FXComposeContext::focusIn(){
  if(xid){
    ///////
    }
  }


// Kill focus to it
void FXComposeContext::focusOut(){
  if(xid){
    ///////
    }
  }


// Set the font
void FXComposeContext::setFont(FXFont* font){
  if(xid && font && font->id()){
    HIMC himc=ImmGetContext((HWND)xid);
    LOGFONT lf;
    GetObject((HFONT)font->id(),sizeof(LOGFONT),&lf);
    ImmSetCompositionFont(himc,&lf);
    ImmReleaseContext((HWND)xid,himc);
    }
  }


// Set the spot
void FXComposeContext::setSpot(FXint x,FXint y){
  if(xid){
    HIMC himc=ImmGetContext((HWND)xid);
    COMPOSITIONFORM cf;
    cf.dwStyle=CFS_POINT;
    FXint tox,toy;
    window->translateCoordinatesTo(tox,toy,window->getShell(),x,y);
    cf.ptCurrentPos.x=tox;
    cf.ptCurrentPos.y=toy;
    ImmSetCompositionWindow(himc,&cf);
    ImmReleaseContext((HWND)xid,himc);
    }
  }


// Set the area
void FXComposeContext::setArea(FXint x,FXint y,FXint w,FXint h){
  if(xid){
    ///////
    }
  }


// Translate key event
FXString FXComposeContext::translateEvent(FXRawEvent& event){
  FXString result;
  if(xid){
    HIMC himc=ImmGetContext(event.hwnd);
    LONG mlen=0;
    FXnchar* mstr;
    if(event.lParam&GCS_RESULTSTR){
      mlen=ImmGetCompositionString(himc,GCS_RESULTSTR,nullptr,0);
      mstr=new FXnchar [mlen];
      ImmGetCompositionString(himc,GCS_RESULTSTR,mstr,mlen);
      }
    else{
      mlen=ImmGetCompositionString(himc,GCS_COMPSTR,nullptr,0);
      mstr=new FXnchar [mlen+1];
      ImmGetCompositionString(himc,GCS_COMPSTR,mstr,mlen);
      }
    mstr[mlen/sizeof(FXnchar)]=0;
    ImmReleaseContext(event.hwnd,himc);
    int utf8len=WideCharToMultiByte(CP_UTF8,0,mstr,-1,nullptr,0,nullptr,nullptr);
    FXchar* utf8str=new FXchar [utf8len];
    WideCharToMultiByte(CP_UTF8,0,mstr,-1,utf8str,utf8len,nullptr,nullptr);
    result.assign(utf8str,utf8len);

    delete [] mstr;
    delete [] utf8str;
    }
  return result;
  }


// Delete input context
FXComposeContext::~FXComposeContext(){
  FXTRACE(TOPIC_CONSTRUCT,"FXComposeContext::~FXComposeContext %p\n",this);
  }

}

#endif
