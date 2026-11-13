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
#include "FXComposeContext.h"
#include "fxapppriv.h"
#include "fxplatform.h"
#include "fxpriv.h"

#if defined(FOX_X11)

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


// Add a signal message
void FXApp::addSignal(FXint sig,FXObject* tgt,FXSelector sel,FXbool immediate,FXuint flags){
  void (CDECL *handler)(int);
  if(sig<0 || MAXSIGNALS<sig){ fxerror("%s::addSignal: bad signal number\n",getClassName()); }

  // May have just changed the message and/or target
  signals[sig].target=tgt;
  signals[sig].message=sel;

  // Has handler been set?
  if(!signals[sig].handlerset){
    if(immediate)
      handler=immediatesignalhandler;
    else
      handler=signalhandler;
#if defined(_POSIX_SOURCE) || defined(_INCLUDE_POSIX_SOURCE)
    struct sigaction sigact;                    // FIXME now that we're using pselect(), we should block non-immediate
    sigact.sa_handler=handler;                  // signals except while in pselect() so that they're deferred until
    sigemptyset(&sigact.sa_mask);               // return to the main event loop.  This will eliminate the race condition
    sigact.sa_flags=flags;                      // where we could be blocking while actually having a raised signal around!
    if(sigaction(sig,&sigact,nullptr)!=0){ fxwarning("%s::addSignal: error setting signal handler\n",getClassName()); }
#else
    signal(sig,handler);
#endif
    signals[sig].notified=false;
    signals[sig].handlerset=true;
    }
  }


// Remove all signal messages for signal sig
void FXApp::removeSignal(FXint sig){
  if(sig<0 || MAXSIGNALS<sig){ fxerror("%s::removeSignal: bad signal number\n",getClassName()); }

  // Has a handler been set?
  if(signals[sig].handlerset){
#if defined(_POSIX_SOURCE) || defined(_INCLUDE_POSIX_SOURCE)
    struct sigaction sigact;
    sigact.sa_handler=SIG_DFL;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags=0;
    if(sigaction(sig,&sigact,nullptr)!=0){ fxwarning("%s::removeSignal: error removing signal handler\n",getClassName()); }
#else
    signal(sig,SIG_DFL);
#endif
    signals[sig].target=nullptr;
    signals[sig].message=0;
    signals[sig].handlerset=false;
    signals[sig].notified=false;
    }
  }


// Add input
FXbool FXApp::addInput(FXObject *tgt,FXSelector sel,FXInputHandle fd,FXuint mode,FXptr ptr){
  if(mode==INPUT_NONE) return false;
  if(fd<0 || fd>=FD_SETSIZE) return false;
  if(fd>=ninputs){                      // Grow table of callbacks
    resizeElms(inputs,fd+1);
    clearElms(&inputs[ninputs],fd+1-ninputs);
    ninputs=fd+1;
    }
  FXASSERT(inputs);
  FXASSERT(fd<ninputs);
  if(mode&INPUT_READ){
    inputs[fd].read.target=tgt;
    inputs[fd].read.message=sel;
    inputs[fd].read.data=ptr;
    FD_SET(fd,&handles->hnd[0]);
    }
  if(mode&INPUT_WRITE){
    inputs[fd].write.target=tgt;
    inputs[fd].write.message=sel;
    inputs[fd].write.data=ptr;
    FD_SET(fd,&handles->hnd[1]);
    }
  if(mode&INPUT_EXCEPT){
    inputs[fd].excpt.target=tgt;
    inputs[fd].excpt.message=sel;
    inputs[fd].excpt.data=ptr;
    FD_SET(fd,&handles->hnd[2]);
    }
  if(fd>maxhandle) maxhandle=fd;
  return true;
  }


// Remove input
FXbool FXApp::removeInput(FXInputHandle fd,FXuint mode){
  if(mode==INPUT_NONE) return false;
  if(fd<0 || fd>maxhandle) return false;
  if(mode&INPUT_READ){
    inputs[fd].read.target=nullptr;
    inputs[fd].read.message=0;
    inputs[fd].read.data=nullptr;
    FD_CLR(fd,&handles->hnd[0]);
    }
  if(mode&INPUT_WRITE){
    inputs[fd].write.target=nullptr;
    inputs[fd].write.message=0;
    inputs[fd].write.data=nullptr;
    FD_CLR(fd,&handles->hnd[1]);
    }
  if(mode&INPUT_EXCEPT){
    inputs[fd].excpt.target=nullptr;
    inputs[fd].excpt.message=0;
    inputs[fd].excpt.data=nullptr;
    FD_CLR(fd,&handles->hnd[2]);
    }
  if(maxhandle==fd){
    while(maxhandle>=0 && !FD_ISSET(maxhandle,&handles->hnd[0]) && !FD_ISSET(maxhandle,&handles->hnd[1]) && !FD_ISSET(maxhandle,&handles->hnd[2])){
      --maxhandle;
      }
    }
  return true;
  }


// Return true if input method support
FXbool FXApp::hasInputMethod() const {
  return xim!=nullptr;
  }


// Find window from root x,y, starting from given window
FXWindow* FXApp::findWindowAt(FXint rx,FXint ry,FXID window) const {
  if(initialized){
    Window rootwin=XDefaultRootWindow((Display*)display);
    Window child;
    int wx,wy;
    if(!window) window=rootwin;
    while(1){
      if(!XTranslateCoordinates((Display*)display,rootwin,window,rx,ry,&wx,&wy,&child)) return nullptr;
      if(child==None) break;
      window=child;
      }
    return findWindowWithId(window);
    }
  return nullptr;
  }


// Return key state
FXbool FXApp::getKeyState(FXuint keysym) const {
  KeyCode keycode=XKeysymToKeycode((Display*)display,keysym);
  char keys[32];
  if(keycode==NoSymbol) return false;
  XQueryKeymap((Display*)display,keys);
  return (keys[keycode>>3]>>(keycode&7))&1;
  }


// Beep
void FXApp::beep(){
  if(initialized){
    FXTRACE(TOPIC_EVENT,"Beep\n");
    XBell((Display*)display,0);
    }
  }


// Close display
FXbool FXApp::closeDisplay(){
  if(initialized){

    // What's going on
    FXTRACE(TOPIC_DETAIL,"%s::closeDisplay: closing display.\n",getClassName());

    // Destroy widgets
    destroy();

    // Close input devices
    closeInputDevices();

    // Free standard stipples
    XFreePixmap((Display*)display,stipples[STIPPLE_0]);
    XFreePixmap((Display*)display,stipples[STIPPLE_1]);
    XFreePixmap((Display*)display,stipples[STIPPLE_2]);
    XFreePixmap((Display*)display,stipples[STIPPLE_3]);
    XFreePixmap((Display*)display,stipples[STIPPLE_4]);
    XFreePixmap((Display*)display,stipples[STIPPLE_5]);
    XFreePixmap((Display*)display,stipples[STIPPLE_6]);
    XFreePixmap((Display*)display,stipples[STIPPLE_7]);
    XFreePixmap((Display*)display,stipples[STIPPLE_8]);
    XFreePixmap((Display*)display,stipples[STIPPLE_9]);
    XFreePixmap((Display*)display,stipples[STIPPLE_10]);
    XFreePixmap((Display*)display,stipples[STIPPLE_11]);
    XFreePixmap((Display*)display,stipples[STIPPLE_12]);
    XFreePixmap((Display*)display,stipples[STIPPLE_13]);
    XFreePixmap((Display*)display,stipples[STIPPLE_14]);
    XFreePixmap((Display*)display,stipples[STIPPLE_15]);
    XFreePixmap((Display*)display,stipples[STIPPLE_16]);

    // Free hatch patterns
    XFreePixmap((Display*)display,stipples[STIPPLE_HORZ]);
    XFreePixmap((Display*)display,stipples[STIPPLE_VERT]);
    XFreePixmap((Display*)display,stipples[STIPPLE_CROSS]);
    XFreePixmap((Display*)display,stipples[STIPPLE_DIAG]);
    XFreePixmap((Display*)display,stipples[STIPPLE_REVDIAG]);
    XFreePixmap((Display*)display,stipples[STIPPLE_CROSSDIAG]);

    // Close input method
#ifndef NO_XIM
    if(xim){XCloseIM((XIM)xim);}
#endif

    // Close display
    XCloseDisplay((Display*)display);

    // Clear it
    display=nullptr;

    // Unlock the global mutex
    appMutex.unlock();

    // We are no longer initialized
    initialized=false;
    }
  return true;
  }




// Smart rectangle compositing algorithm
void FXApp::addRepaint(FXID win,FXint x,FXint y,FXint w,FXint h,FXbool synth){
  FXint px,py,pw,ph,newarea,area;
  FXRepaint *r,**pr;
  area=w*h;
  w+=x;
  h+=y;
  do{

    // Find overlap with outstanding rectangles
    for(r=repaints,pr=&repaints; r; pr=&r->next,r=r->next){
      if(r->window==win){

        // Tentatively conglomerate rectangles
        px=FXMIN(x,r->rect.x);
        py=FXMIN(y,r->rect.y);
        pw=FXMAX(w,r->rect.w);
        ph=FXMAX(h,r->rect.h);
        newarea=(pw-px)*(ph-py);

        // New area bigger than sum; forget about it
        if(newarea > area+r->area) continue;

        // Take old paintrect out of the list
        *pr=r->next;
        r->next=repaintrecs;
        repaintrecs=r;

        // New rectangle
        synth|=r->synth;        // Synthethic is preserved!
        area=newarea;
        x=px;
        y=py;
        w=pw;
        h=ph;
        break;
        }
      }
    }
  while(r);

  // Get rectangle, recycled if possible
  if(repaintrecs){
    r=repaintrecs;
    repaintrecs=r->next;
    }
  else{
    r=new FXRepaint;
    }

  // Fill it
  r->window=win;
  r->rect.x=x;
  r->rect.y=y;
  r->rect.w=w;
  r->rect.h=h;
  r->area=area;
  r->synth=synth;
  r->next=nullptr;
  *pr=r;
  }


// Remove repaints by dispatching them
void FXApp::removeRepaints(FXID win,FXint x,FXint y,FXint w,FXint h){
  FXRepaint *r,**rr;
  XEvent ev;

  w+=x;
  h+=y;

  // Flush the buffer and wait till the X server catches up;
  // resulting events, if any, are buffered in the client.
  XSync((Display*)display,false);

  // Fish out the expose events and compound them
  while(XCheckMaskEvent((Display*)display,ExposureMask,&ev)){
    if(ev.xany.type==NoExpose) continue;
    addRepaint(ev.xexpose.window,ev.xexpose.x,ev.xexpose.y,ev.xexpose.width,ev.xexpose.height,false);
    }

  // Then process events pertaining to window win and overlapping
  // with the given rectangle; other events are left in the queue.
  rr=&repaints;
  while((r=*rr)!=nullptr){
    if(!win || (win==r->window && x<r->rect.w && y<r->rect.h && r->rect.x<w && r->rect.y<h)){
      *rr=r->next;
      ev.xany.type=Expose;
      ev.xexpose.window=r->window;
      ev.xexpose.x=r->rect.x;
      ev.xexpose.y=r->rect.y;
      ev.xexpose.width=r->rect.w-r->rect.x;
      ev.xexpose.height=r->rect.h-r->rect.y;
      r->next=repaintrecs;
      repaintrecs=r;
      dispatchEvent(ev);
      continue;
      }
    rr=&r->next;
    }

  // Flush the buffer again
  XFlush((Display*)display);
  }


// Scroll repaint rectangles; some slight trickyness here:- the
// rectangles don't just move, they stretch in the scroll direction
// This means the original dirty area will remain part of the area to
// be painted.
void FXApp::scrollRepaints(FXID win,FXint dx,FXint dy){
  FXRepaint *r;
  for(r=repaints; r; r=r->next){
    if(r->window==win){
      if(dx>0) r->rect.w+=dx; else r->rect.x+=dx;
      if(dy>0) r->rect.h+=dy; else r->rect.y+=dy;
      }
    }
  }



/*******************************************************************************/




/*******************************************************************************/

// Translate key code to utf8 text
FXString translateKeyEvent(FXRawEvent& event){
  char buffer[40]; KeySym sym; FXwchar w;
  XLookupString(&event.xkey,buffer,sizeof(buffer),&sym,nullptr);
  w=fxkeysym2ucs(sym);
  return FXString(&w,1);
  }


// Get keysym; interprets the modifiers!
static FXuint keysym(FXRawEvent& event){
  KeySym sym=KEY_VoidSymbol;
  char buffer[40];
  XLookupString(&event.xkey,buffer,sizeof(buffer),&sym,nullptr);
  return sym;
  }


/*
// Translate key code to utf8 text
FXString translateKeyEvent(XEvent& event,XIC xic){
  char buffer[40]; KeySym sym; Status s; int n;
  FXString result;
  FXwchar w=0;
#ifndef NO_XIM
  if(xic){
    n=XmbLookupString((XIC)xic,&event.xkey,buffer,sizeof(buffer),&sym,&s);
    if(s!=XLookupChars && s!=XLookupBoth) n=0;
    // FIXME decode buffer based on XLocaleOfIM(XIMOfIC((XIC)xic))
    buffer[n]=0;
    FXTRACE(TOPIC_DETAIL,"XLocaleOfIM=%s\n",XLocaleOfIM(XIMOfIC((XIC)xic)));
    result.assign(buffer,n);
    }
  else{
#endif
    n=XLookupString(&event.xkey,buffer,sizeof(buffer),&sym,nullptr);
    w=fxkeysym2ucs(sym);
    result.assign(&w,1);
#ifndef NO_XIM
    }
#endif
  return result;
  }
*/

/*
  KeyCode keycode=XKeysymToKeycode((Display*)display,keysym);
  char keys[32];
  if(keycode==NoSymbol) return false;
  XQueryKeymap((Display*)display,keys);
  return (keys[keycode>>3]>>(keycode&7))&1;
*/




#if 0
The XIHierarchyEvent has a couple of interesting fields:


typedef struct {
    int           type;         /* GenericEvent */
    unsigned long serial;       /* # of last request processed by server */
    Bool          send_event;   /* true if this came from a SendEvent request */
    Display       *display;     /* Display the event was read from */
    int           extension;    /* XI extension offset */
    int           evtype;       /* XI_DeviceHierarchyChangedNotify */
    Time          time;
    int           flags;
    int           num_devices;
    XIHierarchyInfo *info;
} XIHierarchyEvent;


The first couple of fields are standard. The flags field lists all changes that occurred, a combination of:


XIMasterAdded           A new master device has been created.
XIMasterRemoved         A master device has been deleted.
XISlaveAdded            A new slave device has been added (e.g. plugged in).
XISlaveRemoved          A slave device has been removed (e.g. unplugged).
XISlaveAttached         A slave device has been attached to a master device.
XISlaveDetached         A slave device has been detached (set floating).
XIDeviceEnabled         A device has been enabled (i.e. it may send events now)).
XIDeviceDisabled        A device has been disabled (i.e. it will not send events until enabled again).


The info field contains the details for all devices currently present and those
removed with this change. Each info->flags states what happened to this particular device.
For example, one could search for the slave device that just got removed by searching
for the XISlaveRemoved flag.


if (!(ev.flags & XISlaveRemoved)) {
  printf("No slave device removed.\n");
  return;
  }

for (i = 0; i < ev.num_devices; i++) {
  if (info[i].flags & XISlaveRemoved) {
    printf("Device %d has been removed.\n", info[i].deviceid);
    }
  }


With this information, it is possible to keep track of the device hierarchy at all times and
adjust the user interface as necessary.

#endif


#if 0

        if(ev.type==xsbBallMotion){     // FIXME consecutive xsbBallMotion events should be compressed!
          static int axis_data[6];
          static Window prevwindow=0;
          Window pw;
          XDeviceMotionEvent& sbe=(XDeviceMotionEvent&)ev;
          event.type=SEL_SPACEBALLMOTION;
          event.time=sbe.time;
          //event.win_x=sbe.x;
          //event.win_y=sbe.y;
          //event.root_x=sbe.x_root;
          //event.root_y=sbe.y_root;
          event.values[0]=sbe.axis_data[0]-axis_data[0];
          event.values[1]=sbe.axis_data[1]-axis_data[1];
          event.values[2]=sbe.axis_data[2]-axis_data[2];
          event.values[3]=sbe.axis_data[3]-axis_data[3];
          event.values[4]=sbe.axis_data[4]-axis_data[4];
          event.values[5]=sbe.axis_data[5]-axis_data[5];

//          FXTRACE(TOPIC_DETAIL,"values: %+3d %+3d %+3d %+3d %+3d %+3d\n",sbe.axis_data[0],sbe.axis_data[1],sbe.axis_data[2],sbe.axis_data[3],sbe.axis_data[4],sbe.axis_data[5]);
          axis_data[0]=sbe.axis_data[0];
          axis_data[1]=sbe.axis_data[1];
          axis_data[2]=sbe.axis_data[2];
          axis_data[3]=sbe.axis_data[3];
          axis_data[4]=sbe.axis_data[4];
          axis_data[5]=sbe.axis_data[5];
          pw=prevwindow;
          prevwindow=ev.xmotion.window;
          if(pw!=ev.xmotion.window) return false;
         // if((FXABS(event.values[0])>400) || (FXABS(event.values[1])>400) || (FXABS(event.values[2])>400) || (FXABS(event.values[3])>400) || (FXABS(event.values[4])>400) || (FXABS(event.values[5])>400)) return false;
          //FXTRACE(TOPIC_DETAIL,"spaceballmotion\n");
          //FXTRACE(TOPIC_DETAIL,"state : %08x axes_count=%d first_axis=%d\n",sbe.device_state,sbe.axes_count,sbe.first_axis);
        //  FXTRACE(TOPIC_DETAIL,"values: %+3d %+3d %+3d %+3d %+3d %+3d\n",sbe.axis_data[0],sbe.axis_data[1],sbe.axis_data[2],sbe.axis_data[3],sbe.axis_data[4],sbe.axis_data[5]);
          // FIXME

          // Dispatch to grab window
          if(mouseGrabWindow){
            window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
            if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
            }

          // Dispatch if inside model window
          // FIXME doesSaveUnder test should go away
          else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
            if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
            }
          }
        else if(ev.type==xsbButtonPress){
          event.type=SEL_SPACEBALLBUTTONPRESS;
          event.time=ev.xmotion.time;
          event.win_x=ev.xmotion.x;
          event.win_y=ev.xmotion.y;
          event.root_x=ev.xmotion.x_root;
          event.root_y=ev.xmotion.y_root;

          // Dispatch to grab window
          if(mouseGrabWindow){
            window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
            if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
            }

          // Dispatch if inside model window
          // FIXME doesSaveUnder test should go away
          else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
            if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
            }
          }
        else if(ev.type==xsbButtonRelease){
          event.type=SEL_SPACEBALLBUTTONRELEASE;
          event.time=ev.xmotion.time;
          event.win_x=ev.xmotion.x;
          event.win_y=ev.xmotion.y;
          event.root_x=ev.xmotion.x_root;
          event.root_y=ev.xmotion.y_root;

          // Dispatch to grab window
          if(mouseGrabWindow){
            window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
            if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
            }

          // Dispatch if inside model window
          // FIXME doesSaveUnder test should go away
          else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
            if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
            }
          }


      // Enable extra events when input extension supported
#if defined(HAVE_XINPUT2_H)
/*
      if(1){
        unsigned char maskbits[4]={0,0,0,0};
        XIEventMask eventmask={11,sizeof(maskbits),maskbits};
        XISetMask(eventmask.mask,XI_Enter);
        XISetMask(eventmask.mask,XI_Leave);
        XISetMask(eventmask.mask,XI_FocusIn);
        XISetMask(eventmask.mask,XI_FocusOut);
        XISetMask(eventmask.mask,XI_PropertyEvent);
        if(isEnabled()){
//          XISetMask(eventmask.mask,XI_KeyPress);
//          XISetMask(eventmask.mask,XI_KeyRelease);
//          XISetMask(eventmask.mask,XI_ButtonPress);
//          XISetMask(eventmask.mask,XI_ButtonRelease);
//          XISetMask(eventmask.mask,XI_Motion);
          }
        XISelectEvents((Display*)getApp()->getDisplay(),xid,&eventmask,1);

//XIGrabModifiers modifiers[]={{0,0},{0,0x10},{0,0x1},{0,0x11}};
//int nmods=sizeof(modifiers)/sizeof(modifiers[0]);
//XIGrabButton((Display*)getApp()->getDisplay(),eventmask.deviceid,1,xid,None,GrabModeAsync,GrabModeAsync,False,&eventmask,nmods,modifiers);
//XIUngrabButton((Display*)getApp()->getDisplay(),eventmask.deviceid,1,xid,nmods-2,&modifiers[2]);
//XIGrabKeycode((Display*)getApp()->getDisplay(),eventmask.deviceid, 24, xid, GrabModeAsync, GrabModeAsync,False, &eventmask, nmods, modifiers);
//XIUngrabKeycode((Display*)getApp()->getDisplay(),eventmask.deviceid, 24, xid, nmods - 2, &modifiers[2]);
        }
*/
#endif

#endif

#if defined(HAVE_XINPUT2_H)
static void print_hierarchychangedevent(XIHierarchyEvent *event){
  const char *use;
  int i;
  FXTRACE(TOPIC_DETAIL,"    Changes happened: %s %s %s %s %s %s %s %s\n",(event->flags & XIMasterAdded) ? "[new master]" : "",(event->flags & XIMasterRemoved) ? "[master removed]" : "",(event->flags & XISlaveAdded) ? "[new slave]" : "",(event->flags & XISlaveRemoved) ? "[slave removed]" : "",(event->flags & XISlaveAttached) ? "[slave attached]" : "",(event->flags & XISlaveDetached) ? "[slave detached]" : "",(event->flags & XIDeviceEnabled) ? "[device enabled]" : "",(event->flags & XIDeviceDisabled) ? "[device disabled]" : "");

  for(i=0; i<event->num_info; i++){
    switch(event->info[i].use){
      case XIMasterPointer: use = "master pointer"; break;
      case XIMasterKeyboard: use = "master keyboard"; break;
      case XISlavePointer: use = "slave pointer"; break;
      case XISlaveKeyboard: use = "slave keyboard"; break;
      case XIFloatingSlave: use = "floating slave"; break;
      default: use=""; break;
      }
    FXTRACE(TOPIC_DETAIL,"    device %d [%s (%d)] is %s\n",event->info[i].deviceid,use,event->info[i].attachment,(event->info[i].enabled) ? "enabled" : "disabled");
    if(event->info[i].flags){
      FXTRACE(TOPIC_DETAIL,"    changes: %s %s %s %s %s %s %s %s\n",(event->info[i].flags & XIMasterAdded) ? "[new master]" : "",(event->info[i].flags & XIMasterRemoved) ? "[master removed]" : "",(event->info[i].flags & XISlaveAdded) ? "[new slave]" : "",(event->info[i].flags & XISlaveRemoved) ? "[slave removed]" : "",(event->info[i].flags & XISlaveAttached) ? "[slave attached]" : "",(event->info[i].flags & XISlaveDetached) ? "[slave detached]" : "",(event->info[i].flags & XIDeviceEnabled) ? "[device enabled]" : "",(event->info[i].flags & XIDeviceDisabled) ? "[device disabled]" : "");
      }
    }
  }


static void print_deviceevent(XIDeviceEvent* event){
  double *val;
  int i;
  FXTRACE(TOPIC_DETAIL,"    device: %d (%d)\n", event->deviceid, event->sourceid);
  FXTRACE(TOPIC_DETAIL,"    detail: %d\n", event->detail);
  FXTRACE(TOPIC_DETAIL,"    flags: %s\n", (event->flags & XIKeyRepeat) ? "repeat" : "");
  FXTRACE(TOPIC_DETAIL,"    root: %.2f/%.2f\n", event->root_x, event->root_y);
  FXTRACE(TOPIC_DETAIL,"    event: %.2f/%.2f\n", event->event_x, event->event_y);
  FXTRACE(TOPIC_DETAIL,"    buttons:");
  for(i=0; i<event->buttons.mask_len*8; i++){
    if(XIMaskIsSet(event->buttons.mask,i)) FXTRACE(TOPIC_DETAIL," %d", i);
    }
  FXTRACE(TOPIC_DETAIL,"\n");
  FXTRACE(TOPIC_DETAIL,"    modifiers: locked %#x latched %#x base %#x effective: %#x\n",event->mods.locked, event->mods.latched,event->mods.base, event->mods.effective);
  FXTRACE(TOPIC_DETAIL,"    group: locked %#x latched %#x base %#x effective: %#x\n",event->group.locked, event->group.latched,event->group.base, event->group.effective);
  FXTRACE(TOPIC_DETAIL,"    valuators:");
  val = event->valuators.values;
  for(i=0; i<event->valuators.mask_len*8; i++){
    if(XIMaskIsSet(event->valuators.mask,i)) FXTRACE(TOPIC_DETAIL," %.2f", *val++);
    }
  FXTRACE(TOPIC_DETAIL,"\n");
  FXTRACE(TOPIC_DETAIL,"    windows: root 0x%lx event 0x%lx child 0x%ld\n",event->root, event->event, event->child);
  }


static void print_rawevent(XIRawEvent *event){
  int i;
  double *val, *raw_val;
  FXTRACE(TOPIC_DETAIL,"    device: %d\n", event->deviceid);
  FXTRACE(TOPIC_DETAIL,"    detail: %d\n", event->detail);
  FXTRACE(TOPIC_DETAIL,"    valuators:\n");
  val = event->valuators.values;
  raw_val = event->raw_values;
  for (i = 0; i < event->valuators.mask_len * 8; i++){
    if (XIMaskIsSet(event->valuators.mask, i)) FXTRACE(TOPIC_DETAIL,"         %2d: %.2f (%.2f)\n", i, *val++, *raw_val++);
    }
  FXTRACE(TOPIC_DETAIL,"\n");
  }

#endif



// Dispatch event to widget
FXbool FXApp::dispatchEvent(FXRawEvent& ev){
  const FXTime milliseconds=1000000;
  FXWindow *window,*ancestor,*focuswin;
  Atom      answer;
  FXuint    state;
  XEvent    se;
  Window    tmp;

  // Get window
  window=findWindowWithId(ev.xany.window);

  // Workaround XInput2
#ifdef HAVE_XINPUT2_H
  if(ev.xany.type==GenericEvent) window=getRootWindow();
#endif

  // Was one of our windows, so dispatch
  if(window){

    switch(ev.xany.type){

      // Repaint event
      case GraphicsExpose:
      case Expose:
        event.type=SEL_PAINT;
        event.rect.x=ev.xexpose.x;
        event.rect.y=ev.xexpose.y;
        event.rect.w=ev.xexpose.width;
        event.rect.h=ev.xexpose.height;
        event.synthetic=ev.xexpose.send_event;
        window->handle(this,FXSEL(SEL_PAINT,0),&event);
        return true;
      case NoExpose:
        return true;

      // Keymap Notify
      case KeymapNotify:
        //memcpy(keystate,ev.xkeymap.key_vector,32);
        return true;

      // Keyboard
      case KeyPress:            // [Patch from Roland Baudin] FIXME should this go?
#if 0
#ifndef NO_XIM
        focuswin=getFocusWindow();
        if((ev.xkey.keycode!=0) && focuswin && focuswin->getComposeContext()){
          Window w;
          XGetICValues((XIC)focuswin->getComposeContext()->id(),XNFocusWindow,&w,nullptr);
          if((focuswin->id()!=w) && XFilterEvent(&ev,(Window)focuswin->id())){
            return true;
            }
          }
#endif
#endif
      case KeyRelease:
        event.type=SEL_KEYPRESS+ev.xkey.type-KeyPress;
        event.time=ev.xkey.time;
        event.win_x=ev.xkey.x;
        event.win_y=ev.xkey.y;
        event.root_x=ev.xkey.x_root;
        event.root_y=ev.xkey.y_root;

        // Translate to keysym; must interpret modifiers!
        event.code=keysym(ev);
//      XModifierKeymap *XGetModifierMapping((Display*)display);

        // Translate to string on KeyPress
        if(ev.xkey.type==KeyPress){
//        FXTRACE(TOPIC_DETAIL,"getFocusWindow()=%s\n",getFocusWindow()?getFocusWindow()->getClassName():"nil");
          focuswin=getFocusWindow();
          if(focuswin && focuswin->getComposeContext())
            event.text=focuswin->getComposeContext()->translateEvent(ev);
          else
            event.text=translateKeyEvent(ev);
          }

        // Clear string on KeyRelease
        else{
          event.text.clear();
          }

        // Mouse buttons and modifiers but no wheel buttons
        event.state=ev.xkey.state&~(Button4Mask|Button5Mask);

        // Fix modifier state
        if(ev.xkey.type==KeyPress){
          if(event.code==KEY_Shift_L) event.state|=SHIFTMASK;
          else if(event.code==KEY_Shift_R) event.state|=SHIFTMASK;
          else if(event.code==KEY_Control_L) event.state|=CONTROLMASK;
          else if(event.code==KEY_Control_R) event.state|=CONTROLMASK;
          else if(event.code==KEY_F13) event.state|=METAMASK;     // Key between Ctrl and Alt (on most keyboards)
          else if(event.code==KEY_Alt_L) event.state|=ALTMASK;
          else if(event.code==KEY_Alt_R) event.state|=ALTMASK;    // FIXME do we need ALTGR flag instead/in addition?
          else if(event.code==KEY_Num_Lock) event.state|=NUMLOCKMASK;
          else if(event.code==KEY_Caps_Lock) event.state|=CAPSLOCKMASK;
          else if(event.code==KEY_Scroll_Lock) event.state|=SCROLLLOCKMASK;
          else if(event.code==KEY_Super_L) event.state|=METAMASK;
          else if(event.code==KEY_Super_R) event.state|=METAMASK;
          else{ stickyMods=event.state&(SHIFTMASK|CONTROLMASK|METAMASK|ALTMASK); }
          }
        else{
          if(event.code==KEY_Shift_L) event.state&=~SHIFTMASK;
          else if(event.code==KEY_Shift_R) event.state&=~SHIFTMASK;
          else if(event.code==KEY_Control_L) event.state&=~CONTROLMASK;
          else if(event.code==KEY_Control_R) event.state&=~CONTROLMASK;
          else if(event.code==KEY_F13) event.state&=~METAMASK;    // Key between Ctrl and Alt (on most keyboards)
          else if(event.code==KEY_Alt_L) event.state&=~ALTMASK;
          else if(event.code==KEY_Alt_R) event.state&=~ALTMASK;   // FIXME do we need ALTGR flag instead/in addition?
          else if(event.code==KEY_Num_Lock) event.state&=~NUMLOCKMASK;
          else if(event.code==KEY_Caps_Lock) event.state&=~CAPSLOCKMASK;
          else if(event.code==KEY_Scroll_Lock) event.state&=~SCROLLLOCKMASK;
          else if(event.code==KEY_Super_L) event.state&=~METAMASK;
          else if(event.code==KEY_Super_R) event.state&=~METAMASK;
          else{ event.state|=stickyMods; stickyMods=0; }
          }

        FXTRACE(TOPIC_KEYBOARD,"%s code=%04x state=%04x stickyMods=%04x text=\"%s\"\n",(event.type==SEL_KEYPRESS)?"SEL_KEYPRESS":"SEL_KEYRELEASE",event.code,event.state,stickyMods,event.text.text());

        // Keyboard grabbed by specific window
        if(keyboardGrabWindow){
          if(keyboardGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
          return true;
          }

//        // Try popup window
//        if(popupWindow){
//          if(popupWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
//          return true;
//          }

        // Remember window for later
        if(ev.xkey.type==KeyPress) keyWindow=activeWindow;

        // Dispatch to key window
        if(keyWindow){

          // FIXME doesSaveUnder test should go away
          // Dispatch if not in a modal loop or in a modal loop for a window containing the focus window
          if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(keyWindow)) || keyWindow->getShell()->doesSaveUnder()){
            if(keyWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
            if(inputstyle[1]!='o'){ // not root mode
              focuswin=getFocusWindow();
              if(focuswin && focuswin->getComposeContext()){
                focuswin->handle(this,FXSEL(SEL_IME_START,0),nullptr);
                }
              }
            return true;
            }

          // Beep if outside modal
          if(ev.xany.type==KeyPress) beep();
          }
        return true;

      // Motion
      case MotionNotify:
        event.type=SEL_MOTION;
        event.time=ev.xmotion.time;
        event.win_x=ev.xmotion.x;
        event.win_y=ev.xmotion.y;
        event.root_x=ev.xmotion.x_root;
        event.root_y=ev.xmotion.y_root;
        event.code=0;

        // Mouse buttons and modifiers but no wheel buttons
        event.state=(ev.xmotion.state&~(Button4Mask|Button5Mask)) | stickyMods;

        // Moved more that delta
        if((FXABS(event.root_x-event.rootclick_x)>=dragDelta) || (FXABS(event.root_y-event.rootclick_y)>=dragDelta)) event.moved=1;

        // Dispatch to grab window
        if(mouseGrabWindow){
          window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
          if(mouseGrabWindow->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
          }

        // FIXME doesSaveUnder test should go away
        // Dispatch if inside model window
        else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
          if(window->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
          }

        // Remember last mouse
        event.last_x=event.win_x;
        event.last_y=event.win_y;
        return true;

      // Button
      case ButtonPress:
      case ButtonRelease:
        event.time=ev.xbutton.time;
        event.win_x=ev.xbutton.x;
        event.win_y=ev.xbutton.y;
        event.root_x=ev.xbutton.x_root;
        event.root_y=ev.xbutton.y_root;

        // Mouse buttons and modifiers but no wheel buttons
        event.state=(ev.xmotion.state&~(Button4Mask|Button5Mask)) | stickyMods;

        // Mouse Wheel
        if(ev.xbutton.button==Button4 || ev.xbutton.button==Button5){
          event.type=SEL_MOUSEWHEEL;
          event.code=((ev.xbutton.button==Button4)?120:-120)*ev.xbutton.subwindow;

          // Dispatch to grab window
          if(mouseGrabWindow){
            window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
            if(mouseGrabWindow->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)) refresh();
            return true;
            }

          // Dispatch to window under cursor
          // FIXME doesSaveUnder test should go away
          while(window && (!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder())){
            if(window->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)){ refresh(); break; }
            window=window->getParent();
            }
          return true;
          }

        // Mouse Button
        event.code=ev.xbutton.button;
        if(ev.xbutton.type==ButtonPress){                               // Mouse button press
          if(ev.xbutton.button==Button1){ event.type=SEL_LEFTBUTTONPRESS; event.state|=LEFTBUTTONMASK; }
          if(ev.xbutton.button==Button2){ event.type=SEL_MIDDLEBUTTONPRESS; event.state|=MIDDLEBUTTONMASK; }
          if(ev.xbutton.button==Button3){ event.type=SEL_RIGHTBUTTONPRESS; event.state|=RIGHTBUTTONMASK; }
          if(!event.moved && (event.code==event.click_button) && (milliseconds*(event.time-event.click_time)<clickSpeed)){
            event.click_count++;
            event.click_time=event.time;
            }
          else{
            event.click_count=1;
            event.click_x=event.win_x;
            event.click_y=event.win_y;
            event.rootclick_x=event.root_x;
            event.rootclick_y=event.root_y;
            event.click_button=event.code;
            event.click_time=event.time;
            }
          state=event.state&(LEFTBUTTONMASK|MIDDLEBUTTONMASK|RIGHTBUTTONMASK);
          if((state==LEFTBUTTONMASK)||(state==MIDDLEBUTTONMASK)||(state==RIGHTBUTTONMASK)) event.moved=0;
          }
        else{                                                           // Mouse button release
          if(ev.xbutton.button==Button1){ event.type=SEL_LEFTBUTTONRELEASE; event.state&=~LEFTBUTTONMASK; }
          if(ev.xbutton.button==Button2){ event.type=SEL_MIDDLEBUTTONRELEASE; event.state&=~MIDDLEBUTTONMASK; }
          if(ev.xbutton.button==Button3){ event.type=SEL_RIGHTBUTTONRELEASE; event.state&=~RIGHTBUTTONMASK; }
          }

        // Dispatch to grab window
        if(mouseGrabWindow){
          window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
          if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
          }

        // Dispatch if inside model window
        // FIXME doesSaveUnder test should go away
        else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
          if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
          }

        // Beep if outside modal window
        else{
          if(ev.xany.type==ButtonPress) beep();
          }

        // Remember last mouse
        event.last_x=event.win_x;
        event.last_y=event.win_y;
        return true;

      // Crossing
      case EnterNotify:
        event.time=ev.xcrossing.time;
        if(cursorWindow!=window){
          if(ev.xcrossing.mode==NotifyGrab || ev.xcrossing.mode==NotifyUngrab || (ev.xcrossing.mode==NotifyNormal && ev.xcrossing.detail!=NotifyInferior)){
            ancestor=FXWindow::commonAncestor(window,cursorWindow);
            event.root_x=ev.xcrossing.x_root;
            event.root_y=ev.xcrossing.y_root;
            event.code=ev.xcrossing.mode;
            leaveWindow(cursorWindow,ancestor);
            enterWindow(window,ancestor);
            }
          }
        return true;

      // Crossing
      case LeaveNotify:
        event.time=ev.xcrossing.time;
        if(cursorWindow==window){
          if(ev.xcrossing.mode==NotifyGrab || ev.xcrossing.mode==NotifyUngrab || (ev.xcrossing.mode==NotifyNormal && ev.xcrossing.detail!=NotifyInferior)){
            event.root_x=ev.xcrossing.x_root;
            event.root_y=ev.xcrossing.y_root;
            event.code=ev.xcrossing.mode;
            FXASSERT(cursorWindow==window);
            leaveWindow(window,window->getParent());
            }
          }
        return true;

      // Focus change on shell window
      case FocusIn:
      case FocusOut:
        window=window->getShell();
        if(ev.xfocus.type==FocusOut && activeWindow==window){
          event.type=SEL_FOCUSOUT;
          if(window->handle(this,FXSEL(SEL_FOCUSOUT,0),&event)) refresh();
          activeWindow=nullptr;
          if(inputstyle[1]!='o'){ // not root mode
            focuswin=getFocusWindow();
            if(focuswin && focuswin->getComposeContext()){
              focuswin->handle(this,FXSEL(SEL_IME_END,0),nullptr);
              }
            }
          }
        if(ev.xfocus.type==FocusIn && activeWindow!=window){
          event.type=SEL_FOCUSIN;
          if(window->handle(this,FXSEL(SEL_FOCUSIN,0),&event)) refresh();
          activeWindow=window;
          if(inputstyle[1]!='o'){ // not root mode
            focuswin=getFocusWindow();
            if(focuswin && focuswin->getComposeContext()){
              focuswin->handle(this,FXSEL(SEL_IME_START,0),nullptr);
              }
            }
          }
        return true;

      // Map
      case MapNotify:
        event.type=SEL_MAP;
        if(window->handle(this,FXSEL(SEL_MAP,0),&event)) refresh();
        return true;

      // Unmap
      case UnmapNotify:
        event.type=SEL_UNMAP;
        if(window->handle(this,FXSEL(SEL_UNMAP,0),&event)) refresh();
        return true;

      // Create
      case CreateNotify:
        event.type=SEL_CREATE;
        if(window->handle(this,FXSEL(SEL_CREATE,0),&event)) refresh();
        return true;

      // Destroy
      case DestroyNotify:
        event.type=SEL_DESTROY;
        if(window->handle(this,FXSEL(SEL_DESTROY,0),&event)) refresh();
        return true;

      // Configure
      case ConfigureNotify:
        event.type=SEL_CONFIGURE;
        // According to the ICCCM, if its synthetic, the coordinates are relative
        // to root window; otherwise, they're relative to the parent; so we use
        // the old coordinates if its not a synthetic configure notify
        if(window->getShell()==window && !ev.xconfigure.send_event){
          ev.xconfigure.x=window->getX();
          ev.xconfigure.y=window->getY();
          }
        event.rect.x=ev.xconfigure.x;
        event.rect.y=ev.xconfigure.y;
        event.rect.w=ev.xconfigure.width;
        event.rect.h=ev.xconfigure.height;
        event.synthetic=ev.xconfigure.send_event;
        if(window->handle(this,FXSEL(SEL_CONFIGURE,0),&event)) refresh();
        return true;

      // Circulate
      case CirculateNotify:
        event.type=SEL_RAISED+(ev.xcirculate.place&1);
        if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
        return true;

      // Selection Clear
      case SelectionClear:
        if(ev.xselectionclear.selection==XA_PRIMARY){

          // We lost the primary selection if the new selection owner is different from selectionWindow
          if(selectionWindow && selectionWindow->id()!=XGetSelectionOwner((Display*)display,XA_PRIMARY)){
            event.type=SEL_SELECTION_LOST;
            event.time=ev.xselectionclear.time;
            if(selectionWindow->handle(this,FXSEL(SEL_SELECTION_LOST,0),&event)) refresh();
            selectionWindow=nullptr;
            }
          freeElms(xselTypeList);
          xselNumTypes=0;
          }
        else if(ev.xselectionclear.selection==xcbSelection){

          // We lost the clipboard selection if the new clipboard owner is different from clipboardWindow
          if(clipboardWindow && clipboardWindow->id()!=XGetSelectionOwner((Display*)display,xcbSelection)){
            event.time=ev.xselectionclear.time;
            event.type=SEL_CLIPBOARD_LOST;
            if(clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_LOST,0),&event)) refresh();
            clipboardWindow=nullptr;
            }
          freeElms(xcbTypeList);
          xcbNumTypes=0;
          }
        return true;

      // Selection Request
      case SelectionRequest:
        answer=None;
        FXTRACE(TOPIC_DETAIL,"Request selection (%ld) from requestor=%ld\n",ev.xselectionrequest.selection,ev.xselectionrequest.requestor);
        if(ev.xselectionrequest.selection==XA_PRIMARY){
          if(selectionWindow){
            if(ev.xselectionrequest.target==ddeTargets){            // Request for TYPES
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for SELECTION TYPES; sending %d types\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,xselNumTypes);
              answer=fxsendtypes((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,xselTypeList,xselNumTypes);
              }
            else{                                                   // Request for DATA
              event.type=SEL_SELECTION_REQUEST;
              event.time=ev.xselectionrequest.time;
              event.target=ev.xselectionrequest.target;
              ddeData=nullptr;
              ddeSize=0;
              selectionWindow->handle(this,FXSEL(SEL_SELECTION_REQUEST,0),&event);
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for SELECTION DATA of type %ld; sending %d bytes\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,ev.xselectionrequest.target,ddeSize);
              answer=fxsenddata((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,ev.xselectionrequest.target,ddeData,ddeSize);
              freeElms(ddeData);
              ddeData=nullptr;
              ddeSize=0;
              }
            }
          }
        else if(ev.xselectionrequest.selection==xcbSelection){
          if(clipboardWindow){
            if(ev.xselectionrequest.target==ddeTargets){            // Request for TYPES
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for CLIPBOARD TYPES; sending %d types\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,xcbNumTypes);
              answer=fxsendtypes((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,xcbTypeList,xcbNumTypes);
              }
            else{                                                   // Request for DATA
              event.type=SEL_CLIPBOARD_REQUEST;
              event.time=ev.xselectionrequest.time;
              event.target=ev.xselectionrequest.target;
              ddeData=nullptr;
              ddeSize=0;
              clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_REQUEST,0),&event);
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for CLIPBOARD DATA of type %ld; sending %d bytes\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,ev.xselectionrequest.target,ddeSize);
              answer=fxsenddata((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,ev.xselectionrequest.target,ddeData,ddeSize);
              freeElms(ddeData);
              ddeData=nullptr;
              ddeSize=0;
              }
            }
          }
        else if(ev.xselectionrequest.selection==xdndSelection){
          if(dragWindow){
            if(ev.xselectionrequest.target==ddeTargets){            // Request for TYPES
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for XDND TYPES; sending %d types\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,xdndNumTypes);
              answer=fxsendtypes((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,xdndTypeList,xdndNumTypes);
              }
            else{                                                   // Request for DATA
              event.type=SEL_DND_REQUEST;
              event.time=ev.xselectionrequest.time;
              event.target=ev.xselectionrequest.target;
              ddeData=nullptr;
              ddeSize=0;
              dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
              FXTRACE(TOPIC_EVENT,"Window %ld being requested by window %ld for XDND DATA of type %ld; sending %d bytes\n",ev.xselectionrequest.owner,ev.xselectionrequest.requestor,ev.xselectionrequest.target,ddeSize);
              answer=fxsenddata((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.property,ev.xselectionrequest.target,ddeData,ddeSize);
              freeElms(ddeData);
              ddeData=nullptr;
              ddeSize=0;
              }
            }
          }
        FXTRACE(TOPIC_EVENT,"Sending back response to requestor=%ld\n",ev.xselectionrequest.requestor);
        fxsendreply((Display*)display,ev.xselectionrequest.requestor,ev.xselectionrequest.selection,answer,ev.xselectionrequest.target,ev.xselectionrequest.time);
        return true;

      // Selection Notify
      case SelectionNotify:
        return true;

      // Client message
      case ClientMessage:

        // WM_PROTOCOLS
        if(ev.xclient.message_type==wmProtocols){
          if((FXID)ev.xclient.data.l[0]==wmDeleteWindow){           // WM_DELETE_WINDOW
            FXTRACE(TOPIC_EVENT,"WM_DELETE_WINDOW\n");
            event.type=SEL_CLOSE;
            if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window))){
              if(window->handle(this,FXSEL(SEL_CLOSE,0),&event)) refresh();
              }
            else{
              beep();
              }
            }
          else if((FXID)ev.xclient.data.l[0]==wmSaveYourself){      // WM_SAVE_YOURSELF
            FXTRACE(TOPIC_EVENT,"WM_SAVE_YOURSELF\n");
            //XSetCommand((Display*)display,ev.xany.window,(char**)appArgv,appArgc);
            //XChangeProperty((Display*)display,ev.xany.window,wmCommand,XA_STRING,8,PropModeReplace,(unsigned char*)"",0);
            event.type=SEL_SESSION_NOTIFY;
            return !window->handle(this,FXSEL(SEL_SESSION_NOTIFY,0),&event); // Return 1 if OK to terminate
/*

          case WM_ENDSESSION:         // Session will now end for sure
            event.type=SEL_SESSION_CLOSED;
            if(wParam) window->handle(this,FXSEL(SEL_SESSION_CLOSED,0),&event);
            return 0;
*/
            }
          else if((FXID)ev.xclient.data.l[0]==wmQuitApp){           // WM_QUIT_APP
            FXTRACE(TOPIC_EVENT,"WM_QUIT_APP\n");
            event.type=SEL_CLOSE;
            if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window))){
              if(window->handle(this,FXSEL(SEL_CLOSE,0),&event)) refresh();
              }
            else{
              beep();
              }
            }
          else if((FXID)ev.xclient.data.l[0]==wmTakeFocus){         // WM_TAKE_FOCUS
            if(invocation && invocation->window && invocation->window->id()) ev.xclient.window=invocation->window->id();
            // Assign focus to innermost modal dialog, even when trying to focus
            // on another window; these other windows are dead to inputs anyway.
            // XSetInputFocus causes a spurious BadMatch error; we ignore this in xerrorhandler
            XSetInputFocus((Display*)display,ev.xclient.window,RevertToParent,ev.xclient.data.l[1]);
            }
          else if((FXID)ev.xclient.data.l[0]==wmNetPing){          // NET_WM_PING
            FXTRACE(TOPIC_EVENT,"NET_WM_PING %ld\n",ev.xclient.data.l[1]);
            se.xclient.type=ClientMessage;
            se.xclient.display=(Display*)display;                       // This lets a window manager know that
            se.xclient.message_type=wmProtocols;                        // we're still alive after having received
            se.xclient.format=32;                                       // a WM_DELETE_WINDOW message
            se.xclient.window=XDefaultRootWindow((Display*)display);
            se.xclient.data.l[0]=ev.xclient.data.l[0];
            se.xclient.data.l[1]=ev.xclient.data.l[1];
            se.xclient.data.l[2]=ev.xclient.data.l[2];
            se.xclient.data.l[3]=0;
            se.xclient.data.l[4]=0;
            XSendEvent((Display*)display,se.xclient.window,False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
            }
          }

        // XDND Enter from source
        else if(ev.xclient.message_type==xdndEnter){
          FXint ver=(ev.xclient.data.l[1]>>24)&255;
          FXTRACE(TOPIC_EVENT,"DNDEnter from remote window %ld (ver %d)\n",ev.xclient.data.l[0],ver);
          if(ver>XDND_PROTOCOL_VERSION) return true;
          xdndSource=ev.xclient.data.l[0];                                  // Now we're talking to this guy
          if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
          if(ev.xclient.data.l[1]&1){
            fxrecvtypes((Display*)display,xdndSource,xdndTypes,ddeTypeList,ddeNumTypes,false);
            }
          else{
            allocElms(ddeTypeList,3);
            ddeNumTypes=0;
            if(ev.xclient.data.l[2]){ddeTypeList[0]=ev.xclient.data.l[2];ddeNumTypes++;}
            if(ev.xclient.data.l[3]){ddeTypeList[1]=ev.xclient.data.l[3];ddeNumTypes++;}
            if(ev.xclient.data.l[4]){ddeTypeList[2]=ev.xclient.data.l[4];ddeNumTypes++;}
            }
          }

        // XDND Leave from source
        else if(ev.xclient.message_type==xdndLeave){
          FXTRACE(TOPIC_EVENT,"DNDLeave from remote window %ld\n",ev.xclient.data.l[0]);
          if(xdndSource!=(FXID)ev.xclient.data.l[0]) return true;   // We're not talking to this guy
          if(dropWindow){
            event.type=SEL_DND_LEAVE;
            if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
            dropWindow=nullptr;
            }
          if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
          xdndSource=0;
          }

        // XDND Position from source
        else if(ev.xclient.message_type==xdndPosition){
          if(xdndSource!=(FXID)ev.xclient.data.l[0]) return true;   // We're not talking to this guy
          event.time=ev.xclient.data.l[3];
          event.root_x=((FXuint)ev.xclient.data.l[2])>>16;
          event.root_y=((FXuint)ev.xclient.data.l[2])&0xffff;
          // Search from target window down; there may be another window
          // (like e.g. the dragged shape window) right under the cursor.
          // Note this is the target window, not the proxy target....
          window=findWindowAt(event.root_x,event.root_y,ev.xclient.window);
          if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_ASK]) ddeAction=DRAG_ASK;
          else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_MOVE]) ddeAction=DRAG_MOVE;
          else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_COPY]) ddeAction=DRAG_COPY;
          else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_LINK]) ddeAction=DRAG_LINK;
          else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_PRIVATE]) ddeAction=DRAG_PRIVATE;
          else ddeAction=DRAG_COPY;
          FXTRACE(TOPIC_EVENT,"DNDPosition from remote window %ld action %d\n",ev.xclient.data.l[0],ddeAction);
          ansAction=DRAG_REJECT;
          xdndWantUpdates=true;
          xdndRect.x=event.root_x;
          xdndRect.y=event.root_y;
          xdndRect.w=1;
          xdndRect.h=1;
          if(window!=dropWindow){
            if(dropWindow){
              event.type=SEL_DND_LEAVE;
              if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
              }
            dropWindow=nullptr;
            if(window && window->isDropEnabled()){
              dropWindow=window;
              event.type=SEL_DND_ENTER;
              if(dropWindow->handle(this,FXSEL(SEL_DND_ENTER,0),&event)) refresh();
              }
            }
          if(dropWindow){
            event.type=SEL_DND_MOTION;
            XTranslateCoordinates((Display*)display,XDefaultRootWindow((Display*)display),dropWindow->id(),event.root_x,event.root_y,&event.win_x,&event.win_y,&tmp);
            if(dropWindow->handle(this,FXSEL(SEL_DND_MOTION,0),&event)) refresh();
            event.last_x=event.win_x;
            event.last_y=event.win_y;
            }
          se.xclient.type=ClientMessage;
          se.xclient.display=(Display*)display;
          se.xclient.message_type=xdndStatus;
          se.xclient.format=32;
          se.xclient.window=xdndSource;
          se.xclient.data.l[0]=ev.xclient.window;                       // Proxy Target window
          se.xclient.data.l[1]=0;
          if(ansAction!=DRAG_REJECT) se.xclient.data.l[1]|=1;           // Target accepted
          if(xdndWantUpdates) se.xclient.data.l[1]|=2;                  // Target wants continuous position updates
          se.xclient.data.l[2]=MKUINT(xdndRect.y,xdndRect.x);
          se.xclient.data.l[3]=MKUINT(xdndRect.h,xdndRect.w);
          se.xclient.data.l[4]=xdndActionList[ansAction];               // Drag and Drop Action accepted
          XSendEvent((Display*)display,xdndSource,True,NoEventMask,&se);
          }

        // XDND Drop from source
        else if(ev.xclient.message_type==xdndDrop){
          FXTRACE(TOPIC_EVENT,"DNDDrop from remote window %ld\n",ev.xclient.data.l[0]);
          if(xdndSource!=(FXID)ev.xclient.data.l[0]) return true;       // We're not talking to this guy
          xdndFinishSent=false;
          event.type=SEL_DND_DROP;
          event.time=ev.xclient.data.l[2];

          // Perform drop operation on drop window
          if(!dropWindow || !dropWindow->handle(this,FXSEL(SEL_DND_DROP,0),&event)) ansAction=DRAG_REJECT;

          // Didn't sent finish yet
          if(!xdndFinishSent){
            se.xclient.type=ClientMessage;
            se.xclient.display=(Display*)display;
            se.xclient.message_type=xdndFinished;
            se.xclient.format=32;
            se.xclient.window=xdndSource;
            se.xclient.data.l[0]=ev.xclient.window;                     // Proxy Target window
            se.xclient.data.l[1]=(ansAction==DRAG_REJECT)?0:1;          // Bit #0 means accepted
            se.xclient.data.l[2]=xdndActionList[ansAction];             // Drag and Drop Action accepted
            se.xclient.data.l[3]=0;
            se.xclient.data.l[4]=0;
            XSendEvent((Display*)display,xdndSource,True,NoEventMask,&se);
            }

          // Clean up
          if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
          dropWindow=nullptr;
          xdndSource=0;
          refresh();
          }

        // XDND Status from target
        else if(ev.xclient.message_type==xdndStatus){
          // We ignore ev.xclient.data.l[0], because some other
          // toolkits, e.g. Qt, do not place the proper value there;
          // the proper value is xdndTarget, NOT xdndProxyTarget or None
          //if(xdndTarget!=(FXID)ev.xclient.data.l[0]) return true; // We're not talking to this guy
          ansAction=DRAG_REJECT;
          if(ev.xclient.data.l[1]&1){
            if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_ASK]) ansAction=DRAG_ASK;
            else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_MOVE]) ansAction=DRAG_MOVE;
            else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_COPY]) ansAction=DRAG_COPY;
            else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_LINK]) ansAction=DRAG_LINK;
            else if((FXID)ev.xclient.data.l[4]==xdndActionList[DRAG_PRIVATE]) ansAction=DRAG_PRIVATE;
            }
          xdndWantUpdates=ev.xclient.data.l[1]&2;
          xdndRect.x=((FXuint)ev.xclient.data.l[2])>>16;
          xdndRect.y=((FXuint)ev.xclient.data.l[2])&0xffff;
          xdndRect.w=((FXuint)ev.xclient.data.l[3])>>16;
          xdndRect.h=((FXuint)ev.xclient.data.l[3])&0xffff;
          xdndStatusReceived=true;
          xdndStatusPending=false;
          FXTRACE(TOPIC_EVENT,"DNDStatus from remote window %ld action=%d rect=%d,%d,%d,%d updates=%d\n",ev.xclient.data.l[0],ansAction,xdndRect.x,xdndRect.y,xdndRect.w,xdndRect.h,xdndWantUpdates);
          }
        return true;

      // Property change
      case PropertyNotify:
        FXTRACE(TOPIC_EVENT,"PropertyNotify %ld\n",ev.xproperty.atom);

        // Update window position after minimize/maximize/restore whatever
        if(ev.xproperty.atom==wmState || ev.xproperty.atom==wmNetState){
          unsigned long n,i; Atom type; unsigned char *prop; int format;
          if(Success==XGetWindowProperty((Display*)display,ev.xproperty.window,ev.xproperty.atom,0,10,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
            event.type=SEL_RESTORE;
            event.time=ev.xproperty.time;
            if(format==32){
              for(i=0; i<n; i++){
                if(((FXID*)prop)[i]==wmNetHidden){ event.type=SEL_MINIMIZE; break; }
                if(((FXID*)prop)[i]==wmNetShaded){ event.type=SEL_MINIMIZE; break; }
                if(((FXID*)prop)[i]==wmNetHMaximized){ event.type=SEL_MAXIMIZE; break; }
                if(((FXID*)prop)[i]==wmNetVMaximized){ event.type=SEL_MAXIMIZE; break; }
                if(((FXID*)prop)[i]==wmNetFullScreen){ event.type=SEL_MAXIMIZE; break; }
                if(((FXID*)prop)[i]==IconicState){ event.type=SEL_MINIMIZE; break; }
                if(((FXID*)prop)[i]==NormalState){ event.type=SEL_RESTORE; break; }
                if(((FXID*)prop)[i]==WithdrawnState){ event.type=SEL_MINIMIZE; break; }
                }
              }
            XFree(prop);
            if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
            }
          }
        return true;

      // Keyboard mapping
      case MappingNotify:
        FXTRACE(TOPIC_EVENT,"MappingNotify\n");
        if(ev.xmapping.request!=MappingPointer) XRefreshKeyboardMapping(&ev.xmapping);
        return true;

      // Visibility notification
      case VisibilityNotify:
        FXTRACE(TOPIC_EVENT,"VisibilityNotify window=%ld %s\n",ev.xvisibility.window,(ev.xvisibility.state==VisibilityUnobscured)?"VisibilityUnobscured":(ev.xvisibility.state==VisibilityPartiallyObscured)?"VisibilityPartiallyObscured":"VisibilityFullyObscured");
        return true;

#if defined(HAVE_XINPUT2_H)
      case GenericEvent:
        FXTRACE(TOPIC_EVENT,"GenericEvent\n");
        if(ev.xcookie.extension==xInputOpcode && XGetEventData((Display*)display,&ev.xcookie)){
          FXTRACE(TOPIC_EVENT,"XInput2 type: %d\n",ev.xcookie.evtype);
          switch(ev.xcookie.evtype){
            case XI_ButtonPress:
            case XI_ButtonRelease:
            case XI_Motion:
            case XI_KeyPress:
            case XI_KeyRelease:
              print_deviceevent((XIDeviceEvent*)ev.xcookie.data);
              break;
            case XI_RawKeyPress:
            case XI_RawKeyRelease:
            case XI_RawButtonPress:
            case XI_RawButtonRelease:
            case XI_RawMotion:
              print_rawevent((XIRawEvent*)ev.xcookie.data);
              break;
            case XI_HierarchyChanged:
              print_hierarchychangedevent((XIHierarchyEvent*)ev.xcookie.data);
              break;
            case XI_DeviceChanged:
//              print_devicechangedevent(display,cookie->data);
              break;
            }
          XFreeEventData((Display*)display,&ev.xcookie);
          }
        return true;
#endif

      // Other events
      default:
#if defined(HAVE_XRANDR_H)
        if(ev.type==xrrScreenChange){
          XRRUpdateConfiguration(&ev);
          root->setWidth(root->getDefaultWidth());
          root->setHeight(root->getDefaultHeight());
          //int screen=XRRRootToScreen((Display*)display,ev.xany.window);
          FXTRACE(TOPIC_EVENT,"RRScreenChangeNotify w=%d h=%d\n",DisplayWidth((Display*)display,DefaultScreen((Display*)display)),DisplayHeight((Display*)display,DefaultScreen((Display*)display)));
          // FIXME This should be a SEL_CONFIGURE for the root window, eventually
          }
#endif
#if defined(HAVE_XFIXES_H)
        if(ev.type==xfxFixesSelection){
          FXTRACE(TOPIC_EVENT,"XFixesSelectionNotifyEvent window=%lu owner=%lu selection=%lu\n",((XFixesSelectionNotifyEvent*)&ev)->window,((XFixesSelectionNotifyEvent*)&ev)->owner,((XFixesSelectionNotifyEvent*)&ev)->selection);
          }
#endif
        return true;
      }
    }
  return false;
  }



// 17 stipple patterns which match up exactly with the 4x4 dither kernel
static const unsigned char stipple_patterns[17][8]={
  {0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00},   // 0 (white)
  {0x00,0x00,0x00,0x88, 0x00,0x00,0x00,0x88},
  {0x00,0x22,0x00,0x88, 0x00,0x22,0x00,0x88},
  {0x00,0x22,0x00,0xaa, 0x00,0x22,0x00,0xaa},
  {0x00,0xaa,0x00,0xaa, 0x00,0xaa,0x00,0xaa},
  {0x00,0xaa,0x44,0xaa, 0x00,0xaa,0x44,0xaa},
  {0x11,0xaa,0x44,0xaa, 0x11,0xaa,0x44,0xaa},
  {0x11,0xaa,0x55,0xaa, 0x11,0xaa,0x55,0xaa},
  {0x55,0xaa,0x55,0xaa, 0x55,0xaa,0x55,0xaa},   // 8 (50% grey)
  {0x55,0xaa,0x55,0xee, 0x55,0xaa,0x55,0xee},
  {0x55,0xbb,0x55,0xee, 0x55,0xbb,0x55,0xee},
  {0x55,0xbb,0x55,0xff, 0x55,0xbb,0x55,0xff},
  {0x55,0xff,0x55,0xff, 0x55,0xff,0x55,0xff},
  {0x55,0xff,0xdd,0xff, 0x55,0xff,0xdd,0xff},
  {0x77,0xff,0xdd,0xff, 0x77,0xff,0xdd,0xff},
  {0x77,0xff,0xff,0xff, 0x77,0xff,0xff,0xff},
  {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff}    // 16 (black)
  };


// Standard-issue cross hatch pattern
static const unsigned char cross_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
  };

// Standard-issue diagonal cross hatch pattern
static const unsigned char crossdiag_bits[] = {
  0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x41,
  0x80, 0x80, 0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14,
  0x22, 0x22, 0x41, 0x41, 0x80, 0x80, 0x41, 0x41
  };

// Standard-issue diagonal hatch pattern
static const unsigned char diag_bits[] = {
  0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01, 0x01,
  0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04,
  0x02, 0x02, 0x01, 0x01, 0x80, 0x80, 0x40, 0x40
  };

// Standard-issue horizontal hatch pattern
static const unsigned char hor_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

// Standard-issue reverse diagonal hatch pattern
static const unsigned char revdiag_bits[] = {
  0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40,
  0x80, 0x80, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10,
  0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x01, 0x01
  };

// Standard-issue vertical hatch pattern
static const unsigned char ver_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
  };




// Perhaps should do something else...
static int xerrorhandler(Display* dpy,XErrorEvent* eev){
  char buf[256];

  // A BadWindow due to X_SendEvent is likely due to XDND
  if(eev->error_code==BadWindow && eev->request_code==25) return 0;

  // WM_TAKE_FOCUS causes sporadic errors for X_SetInputFocus
  if(eev->request_code==42) return 0;

  // Get error codes
  XGetErrorText(dpy,eev->error_code,buf,sizeof(buf));

  // Print out meaningful warning
  fxwarning("X Error: code %d major %d minor %d: %s.\n",eev->error_code,eev->request_code,eev->minor_code,buf);
  return 1;
  }


// Fatal error (e.g. lost connection)
static int xfatalerrorhandler(Display*){
  fxerror("X Fatal error.\n");
  return 1;
  }


// Input method was created; unregister the callback, and initialize it
void FXApp::imcreatecallback(void*,FXApp* a,void*){
#ifndef NO_XIM
  XIMCallback ximcallback;
  fxwarning("Warning: input method server restarted.\n");
  XUnregisterIMInstantiateCallback((Display*)app->getDisplay(),nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)a);
  a->xim=XOpenIM((Display*)app->display,nullptr,nullptr,nullptr);
  if(a->xim){
    ximcallback.callback=(XIMProc)imdestroycallback;
    ximcallback.client_data=(XPointer)a;
    XSetIMValues((XIM)a->xim,XNDestroyCallback,&ximcallback,nullptr);
    }
#endif
  }


// Input method was deleted; set callback to recreate it when the IM restarts.
void FXApp::imdestroycallback(void*,FXApp* a,void*){
#ifndef NO_XIM
  fxwarning("Warning: input method server terminated.\n");
  XRegisterIMInstantiateCallback((Display*)a->getDisplay(),nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)a);
  a->xim=nullptr;
#endif
  }


// Open Input devices
void FXApp::openInputDevices(){
#if defined(HAVE_XINPUT2_H)
  int majoropcode,eventbase,errorbase,ndevices;
  if(XQueryExtension((Display*)display,"XInputExtension",&majoropcode,&eventbase,&errorbase)){
    int majorversion=XI_2_Major,minorversion=XI_2_Minor;
    FXTRACE(TOPIC_DETAIL,"XInputExtension maj:%d, evt:%d, err:%d\n",majoropcode,eventbase,errorbase);
    if(XIQueryVersion((Display*)display,&majorversion,&minorversion)==Success){
      FXTRACE(TOPIC_DETAIL,"XInput2 ver:%d.%d\n",majorversion,minorversion);
      unsigned char maskbits[4]={0,0,0,0};
      XIEventMask eventmask={XIAllDevices,sizeof(maskbits),maskbits};
      XISetMask(eventmask.mask,XI_DeviceChanged);
      XISetMask(eventmask.mask,XI_HierarchyChanged);
      XISetMask(eventmask.mask,XI_PropertyEvent);
      XISelectEvents((Display*)display,XDefaultRootWindow((Display*)display),&eventmask,1);
      XIDeviceInfo *devices=XIQueryDevice((Display*)display,XIAllDevices,&ndevices);
      if(devices){
        for(FXint i=0; i<ndevices; i++){
          FXTRACE(TOPIC_DETAIL,"Device %s id: %d enabled: %d attached to: %d is a ",devices[i].name,devices[i].deviceid,devices[i].enabled,devices[i].attachment);
          switch(devices[i].use){
            case XIMasterPointer: FXTRACE(TOPIC_DETAIL,"master pointer\n"); break;
            case XIMasterKeyboard: FXTRACE(TOPIC_DETAIL,"master keyboard\n"); break;
            case XISlavePointer: FXTRACE(TOPIC_DETAIL,"slave pointer\n"); break;
            case XISlaveKeyboard: FXTRACE(TOPIC_DETAIL,"slave keyboard\n"); break;
            case XIFloatingSlave: FXTRACE(TOPIC_DETAIL,"floating slave\n"); break;
            }
          }
/*
unsigned char maskbits[4]={0,0,0,0};
XIEventMask eventmask={XIAllDevices,sizeof(maskbits),maskbits};
XISetMask(eventmask.mask,XI_DeviceChanged);
XISetMask(eventmask.mask,XI_HierarchyChanged);
XISetMask(eventmask.mask,XI_PropertyEvent);
XISelectEvents(DISPLAY(getApp()),xid,&eventmask,1);
XIClearMask(eventmask.mask,XI_DeviceChanged);
XIClearMask(eventmask.mask,XI_HierarchyChanged);
XIClearMask(eventmask.mask,XI_PropertyEvent);
eventmask.deviceid=12;
XISetMask(eventmask.mask,XI_RawKeyPress);
XISetMask(eventmask.mask,XI_RawKeyRelease);
XISetMask(eventmask.mask,XI_RawButtonPress);
XISetMask(eventmask.mask,XI_RawButtonRelease);
XISetMask(eventmask.mask,XI_RawMotion);
XISelectEvents(DISPLAY(getApp()),xid,&eventmask,1);
*/
        XIFreeDeviceInfo(devices);
        xInputOpcode=majoropcode;
        }
      }
    }
#endif
  }


// Close Input devices
void FXApp::closeInputDevices(){
#if defined(HAVE_XINPUT2_H)
  // FIXME
#endif
  }


// Bunch-o-atoms
const FXchar *const windowTypeAtoms[14]={
  "_NET_WM_WINDOW_TYPE_DESKTOP",
  "_NET_WM_WINDOW_TYPE_DOCK",
  "_NET_WM_WINDOW_TYPE_TOOLBAR",
  "_NET_WM_WINDOW_TYPE_MENU",
  "_NET_WM_WINDOW_TYPE_UTILITY",
  "_NET_WM_WINDOW_TYPE_SPLASH",
  "_NET_WM_WINDOW_TYPE_DIALOG",
  "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
  "_NET_WM_WINDOW_TYPE_POPUP_MENU",
  "_NET_WM_WINDOW_TYPE_TOOLTIP",
  "_NET_WM_WINDOW_TYPE_NOTIFICATION",
  "_NET_WM_WINDOW_TYPE_COMBO",
  "_NET_WM_WINDOW_TYPE_DND",
  "_NET_WM_WINDOW_TYPE_NORMAL"
  };




// Open the display
FXbool FXApp::openDisplay(const FXchar* dpy){
  if(!initialized){


    // Using thread-safe X11 if possible
    //XInitThreads();

    // Set error handler
    XSetErrorHandler(xerrorhandler);

    // Set fatal handler
    XSetIOErrorHandler(xfatalerrorhandler);

    // What's going on
    FXTRACE(TOPIC_DETAIL,"%s::openDisplay(%s)\n",getClassName(),dpy);

    // Open display
    display=XOpenDisplay(dpy);
    if(!display) return false;

    // For debugging
    if(synchronize) XSynchronize((Display*)display,true);

    // Setup locales and input method if given
    if(XSupportsLocale()){
      FXchar mods[100]="";
      if(inputmethod[0]){
        fxstrlcpy(mods,"@im=",sizeof(mods));
        fxstrlcat(mods,inputmethod,sizeof(mods));
        }
      if(!XSetLocaleModifiers(mods)){
        if(!XSetLocaleModifiers("")){
          fxwarning("Cannot set locale modifiers.\n");
          }
        }
      }

#if defined(HAVE_XSHM_H)

    // Displaying remotely turns it off for sure
    if(!(dpy[0]==':' && Ascii::isDigit(dpy[1]))){
      shmi=false;
      shmp=false;
      }

    // Its potentially on, see if local display groks it
    if(shmi || shmp){
      int maj,min,dum; Bool pm;
      shmi=false;
      shmp=false;
      if(XQueryExtension((Display*)display,"MIT-SHM",&dum,&dum,&dum)){
        if(XShmQueryVersion((Display*)display,&maj,&min,&pm)){
          shmp=pm && (XShmPixmapFormat((Display*)display)==ZPixmap);
          shmi=true;
          }
        }
      }

#else

    // Don't have it!
    shmi=false;
    shmp=false;

#endif

    // Report the result
    FXTRACE(TOPIC_DETAIL,"X Shared Images  = %d\n",shmi);
    FXTRACE(TOPIC_DETAIL,"X Shared Pixmaps = %d\n",shmp);

    // X Render Extension
#if defined(HAVE_XRENDER_H)
    int xrenderevent,xrendererror;
    if(XRenderQueryExtension((Display*)display,&xrenderevent,&xrendererror)){
      int xrendermajor,xrenderminor;
      if(XRenderQueryVersion((Display*)display,&xrendermajor,&xrenderminor)){
        xrender=true;
        FXTRACE(TOPIC_DETAIL,"X Render %d.%d available\n",xrendermajor,xrenderminor);
        }
      }
#endif

    // Initialize Xft and fontconfig
#if defined(HAVE_XFT_H)
    if(!XftInit(nullptr)) return false;
    if(XftGetVersion()<XftVersion){
      fxwarning("Expected Xft library version %d or greater; was %d.\n",XftVersion,XftGetVersion());
      }
#endif

    // Open input method
#ifndef NO_XIM
    xim=XOpenIM((Display*)display,nullptr,nullptr,nullptr);
    if(xim){
      XIMCallback ximcallback;
      ximcallback.callback=(XIMProc)imdestroycallback;
      ximcallback.client_data=(XPointer)this;
      XSetIMValues((XIM)xim,XNDestroyCallback,&ximcallback,nullptr);
      FXTRACE(TOPIC_DETAIL,"Got Input Method\n");
      }
    else{
      XRegisterIMInstantiateCallback((Display*)display,nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)this);
      }
#endif

    // Check for X Rotation and Reflection support
#if defined(HAVE_XRANDR_H)
    int rreventbase,rrerrorbase;
    if(XRRQueryExtension((Display*)display,&rreventbase,&rrerrorbase)){
      XRRSelectInput((Display*)display,XDefaultRootWindow((Display*)display),True);
      xrrScreenChange=rreventbase+RRScreenChangeNotify;
      FXTRACE(TOPIC_DETAIL,"xrrScreenChange: %d\n",xrrScreenChange);
      }
#endif

    // Check for X Fixes support
#if defined(HAVE_XFIXES_H)
    int fxeventbase,fxerrorbase;
    if(XFixesQueryExtension((Display*)display,&fxeventbase,&fxerrorbase)){
      xfxFixesSelection=fxeventbase+XFixesSelectionNotify;
      FXTRACE(TOPIC_DETAIL,"xfxFixesSelection: %d\n",xfxFixesSelection);
      //int major_version=XFIXES_MAJOR;
      //int minor_version=XFIXES_MINOR;
      //XFixesQueryVersion((Display*)display,&major_version,&minor_version);
      //XFixesSelectSelectionInput((Display*)display,XDefaultRootWindow((Display*)display),xcbSelection,XFixesSetSelectionOwnerNotifyMask|XFixesSelectionWindowDestroyNotifyMask|XFixesSelectionClientCloseNotifyMask);
      }
#endif

    // Window Manager communication
    wmDeleteWindow=XInternAtom((Display*)display,"WM_DELETE_WINDOW",0);
    wmSaveYourself=XInternAtom((Display*)display,"WM_SAVE_YOURSELF",0);
    wmQuitApp=XInternAtom((Display*)display,"_WM_QUIT_APP",0);
    wmProtocols=XInternAtom((Display*)display,"WM_PROTOCOLS",0);
    wmMotifHints=XInternAtom((Display*)display,"_MOTIF_WM_HINTS",0);
    wmTakeFocus=XInternAtom((Display*)display,"WM_TAKE_FOCUS",0);
    wmClientMachine=XInternAtom((Display*)display,"WM_CLIENT_MACHINE",0);
    wmState=XInternAtom((Display*)display,"WM_STATE",0);

    // Extended Window Manager support
    wmNetState=XInternAtom((Display*)display,"_NET_WM_STATE",0);
    wmNetIconName=XInternAtom((Display*)display,"_NET_WM_ICON_NAME",0);
    wmNetWindowName=XInternAtom((Display*)display,"_NET_WM_NAME",0);
    wmNetSupported=XInternAtom((Display*)display,"_NET_SUPPORTED",0);
    wmNetHidden=XInternAtom((Display*)display,"_NET_WM_STATE_HIDDEN",0);
    wmNetShaded=XInternAtom((Display*)display,"_NET_WM_STATE_SHADED",0);
    wmNetHMaximized=XInternAtom((Display*)display,"_NET_WM_STATE_MAXIMIZED_HORZ",0);
    wmNetVMaximized=XInternAtom((Display*)display,"_NET_WM_STATE_MAXIMIZED_VERT",0);
    wmNetFullScreen=XInternAtom((Display*)display,"_NET_WM_STATE_FULLSCREEN",0);
    wmNetBelowOthers=XInternAtom((Display*)display,"_NET_WM_STATE_BELOW",0);
    wmNetAboveOthers=XInternAtom((Display*)display,"_NET_WM_STATE_ABOVE",0);
    wmNetNeedAttention=XInternAtom((Display*)display,"_NET_WM_STATE_DEMANDS_ATTENTION",0);
    wmNetActiveWindow=XInternAtom((Display*)display,"_NET_ACTIVE_WINDOW",0);

    wmNetMoveResize=XInternAtom((Display*)display,"_NET_WM_MOVERESIZE",0);
    wmNetRestack=XInternAtom((Display*)display,"_NET_RESTACK_WINDOW",0);
    wmNetPing=XInternAtom((Display*)display,"_NET_WM_PING",0);
    wmNetProcessId=XInternAtom((Display*)display,"_NET_WM_PID",0);
    wmNetWindowType=XInternAtom((Display*)display,"_NET_WM_WINDOW_TYPE",0);

    // Register window types
    XInternAtoms((Display*)display,const_cast<char**>(windowTypeAtoms),14,0,wmWindowTypes);

    // Session management
    wmWindowRole=XInternAtom((Display*)display,"WM_WINDOW_ROLE",0);
    wmClientLeader=XInternAtom((Display*)display,"WM_CLIENT_LEADER",0);
    wmClientId=XInternAtom((Display*)display,"SM_CLIENT_ID",0);

    // XEMBED support
    embedAtom=XInternAtom((Display*)display,"_XEMBED",0);
    embedInfoAtom=XInternAtom((Display*)display,"_XEMBED_INFO",0);
    timestampAtom=XInternAtom((Display*)display,"FOX_TIMESTAMP_PROP",0);

    // DDE property
    ddeAtom=(FXID)XInternAtom((Display*)display,"_FOX_DDE",0);
    ddeDelete=(FXID)XInternAtom((Display*)display,"DELETE",0);
    ddeTargets=(FXID)XInternAtom((Display*)display,"TARGETS",0);
    ddeIncr=(FXID)XInternAtom((Display*)display,"INCR",0);

    // Clipboard
    xcbSelection=(FXID)XInternAtom((Display*)display,"CLIPBOARD",0);

    // XDND protocol awareness
    xdndProxy=(FXID)XInternAtom((Display*)display,"XdndProxy",0);
    xdndAware=(FXID)XInternAtom((Display*)display,"XdndAware",0);

    // XDND Messages
    xdndEnter=(FXID)XInternAtom((Display*)display,"XdndEnter",0);
    xdndLeave=(FXID)XInternAtom((Display*)display,"XdndLeave",0);
    xdndPosition=(FXID)XInternAtom((Display*)display,"XdndPosition",0);
    xdndStatus=(FXID)XInternAtom((Display*)display,"XdndStatus",0);
    xdndDrop=(FXID)XInternAtom((Display*)display,"XdndDrop",0);
    xdndFinished=(FXID)XInternAtom((Display*)display,"XdndFinished",0);

    // XDND Selection atom
    xdndSelection=(FXID)XInternAtom((Display*)display,"XdndSelection",0);

    // XDND Types list
    xdndTypes=(FXID)XInternAtom((Display*)display,"XdndTypeList",0);

    // XDND Action list
    xdndActions=(FXID)XInternAtom((Display*)display,"XdndActionList",0);

    // XDND Actions
    xdndActionList[DRAG_REJECT]=None;
    xdndActionList[DRAG_ASK]=(FXID)XInternAtom((Display*)display,"XdndActionAsk",0);
    xdndActionList[DRAG_COPY]=(FXID)XInternAtom((Display*)display,"XdndActionCopy",0);
    xdndActionList[DRAG_MOVE]=(FXID)XInternAtom((Display*)display,"XdndActionMove",0);
    xdndActionList[DRAG_LINK]=(FXID)XInternAtom((Display*)display,"XdndActionLink",0);
    xdndActionList[DRAG_PRIVATE]=(FXID)XInternAtom((Display*)display,"XdndActionPrivate",0);

    // Standard stipples
    stipples[STIPPLE_0]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_0],8,8);
    stipples[STIPPLE_1]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_1],8,8);
    stipples[STIPPLE_2]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_2],8,8);
    stipples[STIPPLE_3]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_3],8,8);
    stipples[STIPPLE_4]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_4],8,8);
    stipples[STIPPLE_5]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_5],8,8);
    stipples[STIPPLE_6]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_6],8,8);
    stipples[STIPPLE_7]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_7],8,8);
    stipples[STIPPLE_8]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_8],8,8);
    stipples[STIPPLE_9]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_9],8,8);
    stipples[STIPPLE_10]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_10],8,8);
    stipples[STIPPLE_11]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_11],8,8);
    stipples[STIPPLE_12]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_12],8,8);
    stipples[STIPPLE_13]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_13],8,8);
    stipples[STIPPLE_14]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_14],8,8);
    stipples[STIPPLE_15]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_15],8,8);
    stipples[STIPPLE_16]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_16],8,8);

    // Hatch patterns
    stipples[STIPPLE_HORZ]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)hor_bits,24,24);
    stipples[STIPPLE_VERT]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)ver_bits,24,24);
    stipples[STIPPLE_CROSS]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)cross_bits,24,24);
    stipples[STIPPLE_DIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)diag_bits,16,16);
    stipples[STIPPLE_REVDIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)revdiag_bits,16,16);
    stipples[STIPPLE_CROSSDIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)crossdiag_bits,16,16);

    // Preregister some drag types
    FXWindow::octetType=(FXDragType)XInternAtom((Display*)display,FXWindow::octetTypeName,0);
    FXWindow::deleteType=(FXDragType)XInternAtom((Display*)display,FXWindow::deleteTypeName,0);
    FXWindow::textType=(FXDragType)XInternAtom((Display*)display,FXWindow::textTypeName,0);
    FXWindow::colorType=(FXDragType)XInternAtom((Display*)display,FXWindow::colorTypeName,0);
    FXWindow::urilistType=(FXDragType)XInternAtom((Display*)display,FXWindow::urilistTypeName,0);
    FXWindow::utf8Type=(FXDragType)XInternAtom((Display*)display,FXWindow::utf8TypeName,0);
    FXWindow::utf16Type=(FXDragType)XInternAtom((Display*)display,FXWindow::utf16TypeName,0);
    FXWindow::actionType=(FXDragType)XInternAtom((Display*)display,FXWindow::actionTypeName,0);

    // Open input devices
    openInputDevices();

    // Clear sticky mod state
    stickyMods=0;

    // Lock the global mutex
    appMutex.lock();

    // We have been initialized
    initialized=true;
    }
  return true;
  }




// Initialize platform specific members
void FXApp::initPlatformMembers(){

  wmDeleteWindow=0;                       // Window Manager stuff
  wmSaveYourself=0;
  wmQuitApp=0;
  wmProtocols=0;
  wmMotifHints=0;
  wmTakeFocus=0;
  wmClientMachine=0;
  wmState=0;

  wmNetState=0;
  wmNetIconName=0;
  wmNetWindowName=0;
  wmNetSupported=0;                       // Extended Window Manager stuff
  wmNetHidden=0;
  wmNetShaded=0;
  wmNetHMaximized=0;
  wmNetVMaximized=0;
  wmNetFullScreen=0;
  wmNetBelowOthers=0;
  wmNetAboveOthers=0;
  wmNetNeedAttention=0;
  wmNetActiveWindow=0;
  wmNetMoveResize=0;
  wmNetRestack=0;
  wmNetPing=0;
  wmNetProcessId=0;
  wmNetWindowType=0;

  wmWindowRole=0;                         // Window Role
  wmClientLeader=0;                       // Client leader
  wmClientId=0;                           // Session management id

  embedAtom=0;                            // XEMBED support
  embedInfoAtom=0;                        // XEMBED info support
  timestampAtom=0;                        // Server time

  // DDE
  ddeTargets=0;                           // Data exchange to get list of types
  ddeAtom=0;                              // Data exchange atom
  ddeDelete=0;                            // Data exchange delete request
  ddeTypeList=nullptr;                    // Data types list available
  ddeNumTypes=0;                          // Number of data types
  ddeAction=DRAG_REJECT;                  // Drag and drop action requested
  ansAction=DRAG_REJECT;                  // Drag and drop action suggested

  // CLIPBOARD
  xcbSelection=0;                         // Clipboard selection atom
  xcbTypeList=nullptr;                    // List of clipboard types
  xcbNumTypes=0;                          // How many types clipped

  // SELECTION
  xselTypeList=nullptr;                   // List of primary selection types
  xselNumTypes=0;                         // How many types in list

  // XDND
  xdndTypeList=nullptr;                   // List of XDND types
  xdndNumTypes=0;                         // How many types in list
  xdndProxy=0;                            // XDND proxy atom
  xdndAware=0;                            // XDND awareness atom
  xdndEnter=0;                            // XDND Message types
  xdndLeave=0;
  xdndPosition=0;
  xdndStatus=0;
  xdndDrop=0;
  xdndFinished=0;
  xdndSelection=0;

  xdndTypes=0;                            // XDND type list atom
  xdndActions=0;                          // XDND action list atom
  xdndActionList[DRAG_REJECT]=0;          // XDND Reject
  xdndActionList[DRAG_ASK]=0;             // XDND Ask action
  xdndActionList[DRAG_COPY]=0;            // XDND Copy action
  xdndActionList[DRAG_MOVE]=0;            // XDND Move action
  xdndActionList[DRAG_LINK]=0;            // XDND Link action
  xdndActionList[DRAG_PRIVATE]=0;         // XDND Private action

  xdndSource=0;                           // XDND drag source window
  xdndTarget=0;                           // XDND drop target window
  xdndProxyTarget=0;                      // XDND window to set messages to
  xdndStatusPending=false;                // XDND waiting for status feedback
  xdndStatusReceived=false;               // XDND received at least one status
  xdndWantUpdates=true;                   // XDND target always wants new positions
  xdndFinishSent=false;                   // XDND finish sent
  xdndRect.x=0;                           // XDND motion rectangle
  xdndRect.y=0;
  xdndRect.w=0;
  xdndRect.h=0;
  xdndFinishPending=false;                // Used by the Win32 module only
  xrrScreenChange=0;                      // Xrandr event
  xfxFixesSelection=0;                    // Xfixes event
  xInputOpcode=0;                         // XInput2 Opcode
  xsbDevice=0;                            // Space ball device
  xim=nullptr;                            // Input method stuff

  // Miscellaneous stuff
  shmi=true;
  shmp=true;
  xrender=false;
  synchronize=false;
  }


// Windows messages are not dispatched on X11
FXival FXApp::dispatchEvent(FXID,FXuint,FXuval,FXival){
  return 0;
  }


// Free platform specific members
void FXApp::freePlatformMembers(){
  freeElms(xselTypeList);
  freeElms(xcbTypeList);
  freeElms(xdndTypeList);
  freeElms(ddeTypeList);
  }


// X11 has no system settings to read before the display is opened
void FXApp::initPlatformSettings(){
  }


// Default display name from the environment
const FXchar* fxplatformDefaultDisplay(const FXchar* fallback){
  const FXchar* str=getenv("DISPLAY");
  return str?str:fallback;
  }


// Default font of the application
const FXchar* fxplatformDefaultFont(){
#if defined(HAVE_XFT_H)
  return "Sans,90";
#else
  return "helvetica,90";
#endif
  }


// X11 has no system DPI, so the scale is only set when configured
const FXchar* fxplatformDefaultScale(){
  return nullptr;
  }


// Parse X11 specific arguments; return true if the argument at j was consumed
FXbool FXApp::parsePlatformArgument(int argc,char** argv,FXint& j,const FXchar*& dpy,const FXchar*& istyle){
  // Start synchronized mode
  if(FXString::compare(argv[j],"-sync")==0){
    synchronize=true;
    j++;
    return true;
    }

  // Do not use X shared memory extension, even if available
  if(FXString::compare(argv[j],"-noshm")==0){
    shmi=false;
    shmp=false;
    j++;
    return true;
    }

  // Force use X shared memory extension, if available
  if(FXString::compare(argv[j],"-shm")==0){
    shmi=true;
    shmp=true;
    j++;
    return true;
    }

  // Alternative display
  if(FXString::compare(argv[j],"-display")==0){
    if(++j>=argc){ fxwarning("%s:init: missing argument for -display.\n",getClassName()); ::exit(1); }
    dpy=argv[j];
    j++;
    return true;
    }

  // Set input method
  if(FXString::compare(argv[j],"-im")==0){
    if(++j>=argc){ fxwarning("%s:init: missing argument for -im.\n",getClassName()); ::exit(1); }
    inputmethod=argv[j];
    j++;
    return true;
    }

  // Input style
  if(FXString::compare(argv[j],"-is")==0){
    if(++j>=argc){ fxwarning("%s:init: missing argument for -is.\n",getClassName()); ::exit(1); }
    istyle=argv[j];
    j++;
    return true;
    }
  return false;
  }


}

#endif
