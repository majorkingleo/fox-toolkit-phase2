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
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Composing text on Wayland goes through text-input-v3, which the seat
    enables: the compositor picks the context for the surface that has the
    keyboard focus, so there is nothing to create per text entry widget.
  - A context of our own is only a place for a widget to say where its caret
    is: the text input of the seat is enabled while a window has the focus, and
    the compositor asks us for the caret rectangle through this object.  The
    committed text arrives as a key event of the focus widget, as it does with
    XIM on X11.
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
  if(xid){
    fxwaylandEnableTextInput(fxwayland(getApp()->getDisplay()));
    }
  }


// Kill focus to it
void FXComposeContext::focusOut(){
  if(xid){
    fxwaylandDisableTextInput(fxwayland(getApp()->getDisplay()));
    }
  }


// Set the font; the compositor draws the pre-edit with its own font
void FXComposeContext::setFont(FXFont*){
  }


// Set the spot; where the caret is decides where the pre-edit window goes
void FXComposeContext::setSpot(FXint x,FXint y){
  if(xid){
    FXWindow* win=window;
    fxwaylandSetTextInputSpot(fxwayland(getApp()->getDisplay()),x+(win?win->getX():0),y+(win?win->getY():0));
    }
  }


// Set the area, for widgets that compose more than one line
void FXComposeContext::setArea(FXint x,FXint y,FXint w,FXint h){
  if(xid){
    FXWindow* win=window;
    fxwaylandSetTextInputArea(fxwayland(getApp()->getDisplay()),x+(win?win->getX():0),y+(win?win->getY():0),w,h);
    }
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
