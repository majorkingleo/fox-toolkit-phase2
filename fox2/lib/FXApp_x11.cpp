/********************************************************************************
*                                                                               *
*                A p p l i c a t i o n   O b j e c t   ( X 1 1 )                *
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
#include "fx.h"
#include "fxapppriv.h"
#include "fxplatform.h"

#if !defined(WIN32)

/*
  Notes:

  - The non-Windows backend currently is X11, selected at compile time; a
    Wayland adapter would replace this file rather than extend it.

  - X11 has no process DPI awareness, no system DPI, and no per-window DPI.
    The display scale is configured instead, through [display] scale=<percent>
    or FXApp::setDisplayScale().
*/


namespace FX {


// X11 has no process DPI awareness
FXint fxplatformSetDPIAwareness(const FXString&){
  return DPIAWARENESS_UNKNOWN;
  }


// X11 has no system DPI
FXint fxplatformGetSystemDPI(){
  return 0;
  }


// X11 has no per-window DPI
FXint fxplatformGetWindowDPI(FXID){
  return 0;
  }


// Peek for event
FXbool FXApp::peekEvent(){
  if(initialized){
#if defined(__USE_XOPEN2K)
    struct timespec delta;
#else
    struct timeval delta;
#endif
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    int    maxfds;
    int    nfds;

    // Outstanding repaints
    if(repaints) return true;

    // Still need GUI update
    if(refresher) return true;

    // Outstanding chores
    if(chores) return true;

    // Timers are due?
    if(timers){
      if(timers->due <= FXThread::time()) return true;
      }

    // Events queued up in client already (Shouldn't this not be QueuedAlready?)
    if(XEventsQueued((Display*)display,QueuedAfterFlush)) return true;

    // Prepare fd's to watch
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(ConnectionNumber((Display*)display),&readfds);

    // Add other inputs...
    maxfds=ConnectionNumber((Display*)display);

    // Do a quick poll for any ready events
#if defined(__USE_XOPEN2K)
    delta.tv_nsec=0;
    delta.tv_sec=0;
    nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,&delta,nullptr);
#else
    delta.tv_usec=0;
    delta.tv_sec=0;
    nfds=select(maxfds+1,&readfds,&writefds,&exceptfds,&delta);
#endif

    // Interrupt
    if(nfds<0 && errno!=EAGAIN && errno!=EINTR){
      fxerror("Application terminated: interrupt or lost connection errno=%d\n",errno);
      }

    // I/O activity
    if((nfds>0) && FD_ISSET(ConnectionNumber((Display*)display),&readfds)){
      if(XEventsQueued((Display*)display,QueuedAfterReading)) return true;
      }
    }
  return false;
  }


}

#endif
