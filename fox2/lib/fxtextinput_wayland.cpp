/********************************************************************************
*                                                                               *
*                  T e x t   I n p u t   ( W a y l a n d )                      *
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
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXString.h"
#include "FXElement.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - Text input goes through text-input-v3.  The compositor does not hand out a
    context per widget: one context per seat is enabled while a surface of ours
    has the keyboard focus, and the compositor decides where the pre-edit
    window goes from the caret rectangle we report.
  - The protocol is double buffered: enable, disable, content type, caret
    rectangle and surrounding text only take effect when commit() follows, so
    every change here ends with a commit.
  - An input method does not send key events; it sends a pre-edit string while
    it composes and a commit string when the user is done.  The commit is
    delivered to the focus window as a key press and release that carry the
    text, which is what XIM does on X11, so a widget that inserts
    event.text does not have to know where the text came from.
  - An empty pre-edit string ends the composition; the commit then carries the
    text that was composed.
*/

namespace FX {

/*******************************************************************************/

// The surface the input method may put its window next to
static void textInputEnter(void* data,zwp_text_input_v3*,wl_surface* surface){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  app->tisurface=surface;
  fxwaylandTrace("text input on surface %p\n",(void*)surface);
  }


static void textInputLeave(void* data,zwp_text_input_v3*,wl_surface* surface){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  if(app->tisurface==surface) app->tisurface=nullptr;
  }


// The method is composing; the string is not ours until it is committed
static void textInputPreedit(void* data,zwp_text_input_v3*,const char* text,int32_t cursorbegin,int32_t cursorend){
  FXWaylandApp* app=fxwayland(data);
  if(!app) return;
  app->preedit=text?text:"";
  fxwaylandTrace("preedit \"%s\" %d-%d\n",app->preedit.text(),cursorbegin,cursorend);
  }


// Text the method committed; hand it to the focus window like a key event
static void textInputCommit(void* data,zwp_text_input_v3*,const char* text){
  FXWaylandApp* app=fxwayland(data);
  FXWaylandEvent ev;
  FXWaylandSurface* s;
  if(!app || !text || !*text) return;
  s=app->keyboardsurface;
  if(!s){
    fxwaylandTrace("commit \"%s\" dropped, no focus window\n",text);
    return;
    }
  clearElms(&ev,1);
  ev.type=0;
  ev.id=s->window;
  ev.code=FXXWL_IME;
  strncpy(ev.text,text,sizeof(ev.text)-1);
  ev.text[sizeof(ev.text)-1]='\0';
  app->preedit.clear();
  fxwaylandPost(app,ev);
  fxwaylandTrace("commit \"%s\" to window %u\n",ev.text,(unsigned int)s->window);
  }


// The method wants text around the caret removed; a widget without a caret
// rectangle has nothing to give, so this is only reported
static void textInputDeleteSurrounding(void* data,zwp_text_input_v3*,uint32_t before,uint32_t after){
  fxwaylandTrace("text input wants %u/%u characters removed\n",(unsigned int)before,(unsigned int)after);
  (void)data;
  }


static void textInputDone(void*,zwp_text_input_v3*,uint32_t serial){
  fxwaylandTrace("text input done %u\n",(unsigned int)serial);
  }


static const zwp_text_input_v3_listener textInputListener={
  textInputEnter,
  textInputLeave,
  textInputPreedit,
  textInputCommit,
  textInputDeleteSurrounding,
  textInputDone
  };


/*******************************************************************************/

// Report the caret so the pre-edit window lands next to it
static void commitCaret(FXWaylandApp* app){
  if(!app->tienabled) return;
  zwp_text_input_v3_set_cursor_rectangle(app->tiinput,app->tix,app->tiy,app->tiw,app->tih);
  zwp_text_input_v3_commit(app->tiinput);
  wl_display_flush(app->display);
  }


// Make the context of the seat, once the manager and the seat are both there
void fxwaylandOpenTextInput(FXWaylandApp* app){
  if(!app || !app->timanager || !app->seat || app->tiinput) return;
  app->tiinput=zwp_text_input_manager_v3_get_text_input(app->timanager,app->seat);
  if(app->tiinput){
    zwp_text_input_v3_add_listener(app->tiinput,&textInputListener,app);
    }
  fxwaylandTrace("text input %s\n",app->tiinput?"created":"failed");
  }


void fxwaylandCloseTextInput(FXWaylandApp* app){
  if(!app || !app->tiinput) return;
  zwp_text_input_v3_destroy(app->tiinput);
  app->tiinput=nullptr;
  app->tisurface=nullptr;
  app->tienabled=false;
  app->preedit.clear();
  }


// Enable while a window of ours has the focus; the caret starts at its corner
void fxwaylandEnableTextInput(FXWaylandApp* app){
  if(!app || !app->tiinput || app->tienabled) return;
  app->tienabled=true;
  zwp_text_input_v3_enable(app->tiinput);
  zwp_text_input_v3_set_content_type(app->tiinput,ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL);
  zwp_text_input_v3_set_cursor_rectangle(app->tiinput,app->tix,app->tiy,app->tiw?app->tiw:2,app->tih?app->tih:2);
  zwp_text_input_v3_commit(app->tiinput);
  wl_display_flush(app->display);
  fxwaylandTrace("text input enabled at %d,%d\n",app->tix,app->tiy);
  }


// Disable on focus out; the protocol allows disable() only after enable()
void fxwaylandDisableTextInput(FXWaylandApp* app){
  if(!app || !app->tiinput || !app->tienabled) return;
  app->tienabled=false;
  app->preedit.clear();
  zwp_text_input_v3_disable(app->tiinput);
  zwp_text_input_v3_commit(app->tiinput);
  wl_display_flush(app->display);
  fxwaylandTrace("text input disabled\n");
  }


void fxwaylandSetTextInputSpot(FXWaylandApp* app,FXint x,FXint y){
  if(!app || !app->tiinput) return;
  app->tix=x;
  app->tiy=y;
  app->tiw=2;
  app->tih=2;
  commitCaret(app);
  }


void fxwaylandSetTextInputArea(FXWaylandApp* app,FXint x,FXint y,FXint w,FXint h){
  if(!app || !app->tiinput) return;
  app->tix=x;
  app->tiy=y;
  app->tiw=w;
  app->tih=h;
  commitCaret(app);
  }


FXString fxwaylandPreedit(FXWaylandApp* app){
  return app?app->preedit:FXString::null;
  }

}

#endif
