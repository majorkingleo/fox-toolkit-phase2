/********************************************************************************
*                                                                               *
*              C o m p o s e   C o n t e x t   ( W a y l a n d )                *
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

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Composing text on Wayland goes through text-input-v3, which the seat
    enables: the compositor picks the context for the surface that has the
    keyboard focus, so there is nothing to create per text entry widget.
  - The protocol XML is vendored already; the text input modules of phase 6
    fill in the commit string, the pre-edit and the caret rectangle.  Until
    then the context exists, so widgets can hold one, and composition does not
    happen.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Deserialization
FXComposeContext::FXComposeContext():window(nullptr),message(0),fontset(nullptr){
  FXTRACE(TOPIC_CONSTRUCT,"FXComposeContext::FXComposeContext %p\n",this);
  }


// Construct compose context for the given window
FXComposeContext::FXComposeContext(FXApp* a,FXWindow* win,FXSelector sel):FXId(a),window(win),message(sel),fontset(nullptr){
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
      }
    }
  }


// Set focus to it
void FXComposeContext::focusIn(){
  }


// Kill focus to it
void FXComposeContext::focusOut(){
  }


// Set the font
void FXComposeContext::setFont(FXFont*){
  }


// Set the spot
void FXComposeContext::setSpot(FXint,FXint){
  }


// Set the area
void FXComposeContext::setArea(FXint,FXint,FXint,FXint){
  }


// Translate key event
FXString FXComposeContext::translateEvent(FXRawEvent&){
  return FXString::null;
  }


// Delete input context
FXComposeContext::~FXComposeContext(){
  FXTRACE(TOPIC_CONSTRUCT,"FXComposeContext::~FXComposeContext %p\n",this);
  }

}

#endif
