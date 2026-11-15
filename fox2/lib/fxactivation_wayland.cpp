/********************************************************************************
*                                                                               *
*                    A c t i v a t i o n   ( W a y l a n d )                    *
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
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXEvent.h"
#include "fxwinmap.h"
#include "fxwaylandpriv.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A Wayland compositor does not let a client raise or focus its own windows:
    the user decides.  The one thing a client may do is ask for an activation
    token, which carries the evidence that the user did something that justifies
    taking the focus, and hand that token to the compositor together with the
    surface it wants activated.
  - The evidence is the serial of the input event that caused the request.  A
    client started from a terminal has none, and the compositor is then free to
    grant the token anyway, which is what compositors do for the first window of
    an application; a request without a serial is therefore still worth making.
  - The token arrives asynchronously through done(), and only then may it be
    used; the answer can also be a token the compositor refuses to honour, so
    nothing here waits for the focus to arrive.
  - This is what _NET_ACTIVE_WINDOW asks for on X11, and it is why FXWindow::
    raise() and the first show of a toplevel go through the same request.
*/

namespace FX {

// What a token needs to know before its answer arrives
struct FXActivation {
  FXWaylandApp*          app;
  wl_surface*            surface;
  xdg_activation_token_v1* token;
  FXActivation*          next;
  };


/*******************************************************************************/

// The compositor answered with a token: this is the moment it may be used
static void tokenDone(void* data,xdg_activation_token_v1* token,const char* name){
  FXActivation* act=(FXActivation*)data;
  FXActivation** link;
  FXWaylandApp* app=act?act->app:nullptr;
  if(app){
    for(link=(FXActivation**)&app->activations; *link; link=&(*link)->next){
      if(*link==act){ *link=act->next; break; }
      }
    if(name && act->surface && app->activation){
      xdg_activation_v1_activate(app->activation,name,act->surface);
      wl_display_flush(app->display);
      fxwaylandTrace("activated surface %p with token \"%s\"\n",(void*)act->surface,name);
      }
    xdg_activation_token_v1_destroy(token);
    }
  delete act;
  }


static const xdg_activation_token_v1_listener tokenListener={
  tokenDone
  };


/*******************************************************************************/

// Ask the compositor to activate a window of ours
void fxwaylandActivateWindow(FXWaylandApp* app,FXID window){
  FXWaylandSurface* surf;
  FXActivation* act;
  xdg_activation_token_v1* token;
  FXuint serial;
  if(!app || !app->activation) return;
  if((surf=fxwaylandFindSurface(app,window))==nullptr || !surf->surface) return;

  // A token must not be destroyed before its done arrives, so several requests
  // may be outstanding at once; the compositor answers them in its own time
  if(!(act=new FXActivation())) return;
  act->app=app;
  act->surface=surf->surface;
  act->token=nullptr;
  act->next=(FXActivation*)app->activations;
  app->activations=act;
  token=xdg_activation_v1_get_activation_token(app->activation);
  if(!token){
    app->activations=act->next;
    delete act;
    return;
    }
  act->token=token;
  xdg_activation_token_v1_add_listener(token,&tokenListener,act);

  // The press or key that caused the request is the evidence; without one the
  // compositor still decides, and usually grants it to the first window
  serial=app->keyboardserial?app->keyboardserial:app->pointerserial;
  if(serial && app->seat){
    xdg_activation_token_v1_set_serial(token,serial,app->seat);
    }
  xdg_activation_token_v1_set_surface(token,surf->surface);
  xdg_activation_token_v1_commit(token);
  wl_display_flush(app->display);
  fxwaylandTrace("asked for an activation token for window %u, serial %u\n",(unsigned int)window,serial);
  }


void fxwaylandCloseActivation(FXWaylandApp* app){
  FXActivation* act;
  if(!app) return;
  while((act=(FXActivation*)app->activations)!=nullptr){
    app->activations=act->next;
    if(act->token) xdg_activation_token_v1_destroy(act->token);
    delete act;
    }
  if(app->activation){
    xdg_activation_v1_destroy(app->activation);
    app->activation=nullptr;
    }
  }

}

#endif
