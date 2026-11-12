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


// Get an event
FXbool FXApp::getNextEvent(FXRawEvent& ev,FXTime blocking){
  FXRawEvent e;

  // Assume non-event
a:ev.xany.type=0;

  // If a timer is due, handle it
  if(timers && timers->due<=FXThread::time()){
    FXTimer* t=timers;
    timers=t->next;
    t->next=timerrecs;
    timerrecs=t;
    if(t->target && t->target->tryHandle(this,FXSEL(SEL_TIMEOUT,t->message),t->data)) refresh();
    return false;
    }

  // Check non-immediate signals that may have fired
  if(signalreceived){
    FXint sig=signalreceived;
    FXint nxt=MAXSIGNALS;
    signals[sig].notified=false;
    while(--nxt && !signals[nxt].notified){}
    signalreceived=nxt;
    if(signals[sig].target && signals[sig].target->tryHandle(this,FXSEL(SEL_SIGNAL,signals[sig].message),(void*)(FXival)sig)) refresh();
    return false;
    }

  // Are there no events already queued up?
  if(!initialized || !XEventsQueued((Display*)display,QueuedAfterFlush)){
#if defined(__USE_XOPEN2K)
    struct timespec delta;
#else
    struct timeval delta;
#endif
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    int    maxfds;
    int    nfds=0;


    // Prepare fd's to check
    maxfds=maxhandle;
    readfds=handles->hnd[0];
    writefds=handles->hnd[1];
    exceptfds=handles->hnd[2];

    // Add connection to display if its open
    if(initialized){
      FD_SET(ConnectionNumber((Display*)display),&readfds);
      if(ConnectionNumber((Display*)display)>maxfds) maxfds=ConnectionNumber((Display*)display);
      }

    // Do a quick poll for any ready events or inputs
#if defined(__USE_XOPEN2K)
    delta.tv_nsec=0;
    delta.tv_sec=0;
    nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,&delta,nullptr);
#else
    delta.tv_usec=0;
    delta.tv_sec=0;
    nfds=select(maxfds+1,&readfds,&writefds,&exceptfds,&delta);
#endif

    // Nothing to do, so perform idle processing
    if(nfds==0){

      // Release the expose events
      if(repaints){
        FXRepaint *r=repaints;
        ev.xany.type=Expose;
        ev.xexpose.window=r->window;
        ev.xexpose.send_event=r->synth;
        ev.xexpose.x=r->rect.x;
        ev.xexpose.y=r->rect.y;
        ev.xexpose.width=r->rect.w-r->rect.x;
        ev.xexpose.height=r->rect.h-r->rect.y;
        repaints=r->next;
        r->next=repaintrecs;
        repaintrecs=r;
        return true;
        }

      // Do our chores :-)
      if(chores){
        FXChore *c=chores;
        chores=c->next;
        c->next=chorerecs;
        chorerecs=c;
        if(c->target && c->target->tryHandle(this,FXSEL(SEL_CHORE,c->message),c->data)) refresh();
        }

      // GUI updating:- walk the whole widget tree, stop after updating refresherstop
      if(refresher){
        if(refresher->getFirst()){
          refresher=refresher->getFirst();
          }
        else{
          while(refresher->getParent()){
            if(refresher->getNext()){ refresher=refresher->getNext(); break; }
            refresher=refresher->getParent();
            }
          }
        refresher->handle(this,FXSEL(SEL_UPDATE,0),nullptr);
        if(refresher!=refresherstop) return false;
        refresher=refresherstop=nullptr;
        }

      // There are more chores to do
      if(chores) return false;

      // We're not blocking
      if(blocking<=0) return false;

      // Now, block till timeout, i/o, or event
      maxfds=maxhandle;
      readfds=handles->hnd[0];
      writefds=handles->hnd[1];
      exceptfds=handles->hnd[2];

      // Add connection to display if its open
      if(initialized){
        FD_SET(ConnectionNumber((Display*)display),&readfds);
        if(ConnectionNumber((Display*)display)>maxfds) maxfds=ConnectionNumber((Display*)display);
        }

      // If there are timers, we block only for a little while.
      if(timers || blocking<forever){
        FXTime interval;

        // All that testing above may have taken some time...
        if(timers && (interval=timers->due-FXThread::time())<blocking) blocking=interval;

        // Some timers are already due; do them right away!
        if(blocking<=0) return false;

        // Exit critical section
        appMutex.unlock();

        // Block till timer or event or interrupt
#if defined(__USE_XOPEN2K)
        delta.tv_nsec=blocking%1000000000;
        delta.tv_sec=blocking/1000000000;
        nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,&delta,nullptr);
#else
        delta.tv_usec=(blocking/1000)%1000000;
        delta.tv_sec=blocking/1000000000;
        nfds=select(maxfds+1,&readfds,&writefds,&exceptfds,&delta);
#endif

        // Enter critical section
        appMutex.lock();
        }

      // If no timers, we block till event or interrupt
      else{

        // Exit critical section
        appMutex.unlock();

        // Block until something happens
#if defined(__USE_XOPEN2K)
        nfds=pselect(maxfds+1,&readfds,&writefds,&exceptfds,nullptr,nullptr);
#else
        nfds=select(maxfds+1,&readfds,&writefds,&exceptfds,nullptr);
#endif

        // Enter critical section
        appMutex.lock();
        }
      }

    // Timed out or interrupted
    if(nfds<=0){
      if(nfds<0 && errno!=EAGAIN && errno!=EINTR){fxerror("Application terminated: interrupt or lost connection errno=%d\n",errno);}
      return false;
      }

// FIXME
// When handling callback and entering recursive event loop, the value
// in readfds in the upper invocation is no longer correct.  This needs
// to be fixed. [do this in FXDispatcher at some point].

    // Any other file descriptors set?
    if(0<=maxhandle){

      // Examine I/O file descriptors
      for(FXInputHandle fff=0; fff<=maxhandle; fff++){

        // Copy the record as the callbacks may try to change things
        FXInput in=inputs[fff];

        // Skip the display connection, which is treated differently
        if(initialized && (fff==ConnectionNumber((Display*)display))) continue;

        // Check file descriptors
        if(FD_ISSET(fff,&readfds)){
          if(in.read.target && in.read.target->tryHandle(this,FXSEL(SEL_IO_READ,in.read.message),in.read.data)) refresh();
          }
        if(FD_ISSET(fff,&writefds)){
          if(in.write.target && in.write.target->tryHandle(this,FXSEL(SEL_IO_WRITE,in.write.message),in.write.data)) refresh();
          }
        if(FD_ISSET(fff,&exceptfds)){
          if(in.excpt.target && in.excpt.target->tryHandle(this,FXSEL(SEL_IO_EXCEPT,in.excpt.message),in.excpt.data)) refresh();
          }
        }
      }

    // If there is no event, we're done
    if(!initialized || !FD_ISSET(ConnectionNumber((Display*)display),&readfds) || !XEventsQueued((Display*)display,QueuedAfterReading)) return false;
    }

  // Get an event
  XNextEvent((Display*)display,&ev);

#if 0
  // Event was filtered by input method; get next one
  if(xim && XFilterEvent(&ev,None)){
    goto a;
    }
  if(xim && getFocusWindow() && XFilterEvent(&ev,(Window)getFocusWindow()->id())){    // FIXME
    goto a;
    }
#endif

  // Passing in focuswindow to XFilterEvent just didn't work on Gnome3 with either scim or ibus
  if(xim && getFocusWindow() && XFilterEvent(&ev,None)){      // [Patch from Roland Baudin] FIXME but also need to deal with keyboard grabs
    return false;
    }

  // Event was repaint event; get next one
  if(ev.xany.type==Expose || ev.xany.type==GraphicsExpose){
    addRepaint((FXID)ev.xexpose.window,ev.xexpose.x,ev.xexpose.y,ev.xexpose.width,ev.xexpose.height,false);
    goto a;
    }

  // Compress motion events
  if(ev.xany.type==MotionNotify){
    while(XPending((Display*)display)){
      XPeekEvent((Display*)display,&e);
      if((e.xany.type!=MotionNotify) || (ev.xmotion.window!=e.xmotion.window) || (ev.xmotion.state!=e.xmotion.state)) break;
      XNextEvent((Display*)display,&ev);
      }
    }

  // Compress wheel events
  else if((ev.xany.type==ButtonPress) && (ev.xbutton.button==Button4 || ev.xbutton.button==Button5)){
    FXint ticks=1;
    while(XPending((Display*)display)){
      XPeekEvent((Display*)display,&e);
      if((e.xany.type!=ButtonPress && e.xany.type!=ButtonRelease) || (ev.xany.window!=e.xany.window) || (ev.xbutton.button!=e.xbutton.button)) break;
      ticks+=(e.xany.type==ButtonPress);
      XNextEvent((Display*)display,&ev);
      }
    ev.xbutton.subwindow=(Window)ticks;   // Stick it here for later
    }

  // Compress configure events
  else if(ev.xany.type==ConfigureNotify){
    while(XCheckTypedWindowEvent((Display*)display,ev.xconfigure.window,ConfigureNotify,&e)){
      ev.xconfigure.width=e.xconfigure.width;
      ev.xconfigure.height=e.xconfigure.height;
      if(e.xconfigure.send_event){
        ev.xconfigure.x=e.xconfigure.x;
        ev.xconfigure.y=e.xconfigure.y;
        }
      }
    }

  // Regular event
  return true;
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


// Flush pending repaints
void FXApp::flush(FXbool sync){
  if(initialized){
    if(sync)
      XSync((Display*)display,false);
    else
      XFlush((Display*)display);
    }
  }


// Paint all windows marked for repainting
void FXApp::repaint(){
  if(initialized){
    removeRepaints(0,0,0,0,0);
    }
  }


// Begin of wait-cursor block; wait-cursor blocks may be nested.
void FXApp::beginWaitCursor(){
  if(initialized){
    if(waitCount==0){
      if(!waitCursor->id()){ fxerror("%s::beginWaitCursor: wait cursor not created yet.\n",getClassName()); }
      FXWindow* child;
      FXASSERT(display);
      child=getRootWindow()->getFirst();
      while(child){
        if(child->id()){
          XDefineCursor((Display*)display,child->id(),waitCursor->id());
          if(child->getFirst()){child=child->getFirst();continue;}
          }
        while(!child->getNext()&&child->getParent()){child=child->getParent();}
        child=child->getNext();
        }
      if(mouseGrabWindow){
        XChangeActivePointerGrab((Display*)display,(ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask),waitCursor->id(),CurrentTime);
        }
      XFlush((Display*)display);
      }
    waitCount++;
    }
  }


// End of wait-cursor block
void FXApp::endWaitCursor(){
  if(initialized){
    if(waitCount==0) return;
    waitCount--;
    if(waitCount==0){
      if(!waitCursor->id()){ fxerror("%s::endWaitCursor: wait cursor not created yet.\n",getClassName()); }
      FXWindow* child;
      child=getRootWindow()->getFirst();
      while(child){
        if(child->id()){
          XDefineCursor((Display*)display,child->id(),child->getDefaultCursor()->id());
          if(child->getFirst()){child=child->getFirst();continue;}
          }
        while(!child->getNext()&&child->getParent()){child=child->getParent();}
        child=child->getNext();
        }
      if(mouseGrabWindow){
        XChangeActivePointerGrab((Display*)display,(ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask),mouseGrabWindow->getDragCursor()->id(),CurrentTime);
        }
      XFlush((Display*)display);
      }
    }
  }


// Change to a new wait cursor
void FXApp::setWaitCursor(FXCursor *cur){
  if(initialized){
    if(!cur){ fxerror("%s::setWaitCursor: NULL wait cursor.\n",getClassName()); }
    if(waitCursor!=cur){
      waitCursor=cur;
      if(waitCount){
        if(!waitCursor->id()){ fxerror("%s::setWaitCursor: wait cursor not created yet.\n",getClassName()); }
        FXWindow* child;
        child=getRootWindow()->getFirst();
        while(child){
          if(child->id()){
            XDefineCursor((Display*)display,child->id(),waitCursor->id());
            if(child->getFirst()){child=child->getFirst();continue;}
            }
          while(!child->getNext()&&child->getParent()){child=child->getParent();}
          child=child->getNext();
          }
        XFlush((Display*)display);
        }
      }
    }
  }


// Register DND type
FXDragType FXApp::registerDragType(const FXString& name) const {
  if(initialized){
    return (FXDragType)XInternAtom((Display*)display,name.text(),0);
    }
  return 0;
  }


// Get name of registered drag type
FXString FXApp::getDragTypeName(FXDragType type) const {
  if(initialized){
    FXString result;
    FXchar *name=XGetAtomName((Display*)display,type);
    result.assign(name);
    XFree(name);
    return result;
    }
  return FXString::null;
  }


}

#endif
