/********************************************************************************
*                                                                               *
*              A p p l i c a t i o n   O b j e c t   ( W i n 3 2 )              *
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

#if defined(WIN32)

/*
  Notes:

  - DPI awareness and the DPI query functions are resolved dynamically; older
    Windows versions and older SDK's do not provide them, and the import
    libraries do not always export them.

  - The process DPI awareness must be established before any window is created;
    FXApp::init() calls fxplatformSetDPIAwareness() for that reason.
*/

// Trace topic shared with FXApp.cpp
#define TOPIC_DETAIL    1002


namespace FX {


// DPI awareness function types, available with a recent SDK
#if defined(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)

typedef BOOL (WINAPI *SETPROCESSDPIAWARENESSCONTEXTFUNC)(DPI_AWARENESS_CONTEXT value);
typedef HRESULT (WINAPI *SETPROCESSDPIAWARENESSFUNC)(int value);
typedef DPI_AWARENESS_CONTEXT (WINAPI *GETTHREADDPIAWARENESSCONTEXTFUNC)();
typedef DPI_AWARENESS (WINAPI *GETAWARENESSFROMDPIAWARENESSCONTEXTFUNC)(DPI_AWARENESS_CONTEXT value);
typedef BOOL (WINAPI *AREDPIAWARENESSCONTEXTSEQUALFUNC)(DPI_AWARENESS_CONTEXT a,DPI_AWARENESS_CONTEXT b);


// Establish the requested process DPI awareness.  The requested policy may be
// "permonitorv2" (default), "permonitor", "system", or "unaware".  If the
// function is not available, fall back to older API's; if the process is
// already aware (manifest), keep that state.  Returns the effective awareness
// level, which may differ from the request.
static FXint applyDPIAwareness(const FXString& policy){
  SETPROCESSDPIAWARENESSCONTEXTFUNC setprocesscontext=nullptr;
  SETPROCESSDPIAWARENESSFUNC setprocessawareness=nullptr;
  GETTHREADDPIAWARENESSCONTEXTFUNC getthreadcontext=nullptr;
  GETAWARENESSFROMDPIAWARENESSCONTEXTFUNC getawareness=nullptr;
  AREDPIAWARENESSCONTEXTSEQUALFUNC arecontextsequal=nullptr;
  DPI_AWARENESS_CONTEXT context=nullptr;
  FXint awareness=DPIAWARENESS_UNKNOWN;
  FXint wanted=DPIAWARENESS_PER_MONITOR_V2;
  HMODULE user32dll=GetModuleHandleA("user32.dll");
  HMODULE shcoredll=LoadLibraryA("shcore.dll");
  if(user32dll){
    setprocesscontext=(SETPROCESSDPIAWARENESSCONTEXTFUNC)GetProcAddress(user32dll,"SetProcessDpiAwarenessContext");
    getthreadcontext=(GETTHREADDPIAWARENESSCONTEXTFUNC)GetProcAddress(user32dll,"GetThreadDpiAwarenessContext");
    getawareness=(GETAWARENESSFROMDPIAWARENESSCONTEXTFUNC)GetProcAddress(user32dll,"GetAwarenessFromDpiAwarenessContext");
    arecontextsequal=(AREDPIAWARENESSCONTEXTSEQUALFUNC)GetProcAddress(user32dll,"AreDpiAwarenessContextsEqual");
    }
  if(shcoredll){
    setprocessawareness=(SETPROCESSDPIAWARENESSFUNC)GetProcAddress(shcoredll,"SetProcessDpiAwareness");
    }

  // Parse requested policy
  if(policy=="unaware"){
    wanted=DPIAWARENESS_UNAWARE;
    }
  else if(policy=="system"){
    wanted=DPIAWARENESS_SYSTEM;
    }
  else if(policy=="permonitor"){
    wanted=DPIAWARENESS_PER_MONITOR;
    }
  else if(policy!="permonitorv2" && policy!="auto" && !policy.empty()){
    fxwarning("DPI awareness: unknown policy '%s', using permonitorv2.\n",policy.text());
    }

  // Select matching awareness context
  switch(wanted){
    case DPIAWARENESS_PER_MONITOR_V2:
      context=DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
      break;
    case DPIAWARENESS_PER_MONITOR:
      context=DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
      break;
    case DPIAWARENESS_SYSTEM:
      context=DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
      break;
    default:
      context=DPI_AWARENESS_CONTEXT_UNAWARE;
      break;
    }

  // Apply, with fallbacks for older Windows versions
  if(!(setprocesscontext && setprocesscontext(context))){
    if((wanted==DPIAWARENESS_PER_MONITOR_V2 || wanted==DPIAWARENESS_PER_MONITOR) && setprocessawareness){
      setprocessawareness(2);           // PROCESS_PER_MONITOR_DPI_AWARE
      }
    else if(wanted!=DPIAWARENESS_UNAWARE){
      SetProcessDPIAware();
      }
    }

  // Report what the process actually got
  if(getthreadcontext && getawareness){
    DPI_AWARENESS_CONTEXT current=getthreadcontext();
    switch(getawareness(current)){
      case DPI_AWARENESS_UNAWARE:
        awareness=DPIAWARENESS_UNAWARE;
        break;
      case DPI_AWARENESS_SYSTEM_AWARE:
        awareness=DPIAWARENESS_SYSTEM;
        break;
      case DPI_AWARENESS_PER_MONITOR_AWARE:
        awareness=DPIAWARENESS_PER_MONITOR;
        break;
      default:
        awareness=DPIAWARENESS_UNKNOWN;
        break;
      }
    if(awareness==DPIAWARENESS_PER_MONITOR && wanted==DPIAWARENESS_PER_MONITOR_V2 && arecontextsequal && arecontextsequal(current,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)){
      awareness=DPIAWARENESS_PER_MONITOR_V2;
      }
    }
  else{
    awareness=wanted;                   // Cannot verify; assume request
    }

  if(awareness!=DPIAWARENESS_UNKNOWN && awareness<wanted){
    fxwarning("DPI awareness: requested %d but effective level is %d.\n",wanted,awareness);
    }
  FXTRACE(TOPIC_DETAIL,"DPI awareness: policy=%s effective=%d\n",policy.text(),awareness);

  if(shcoredll) FreeLibrary(shcoredll);
  return awareness;
  }

#else

// No DPI awareness context API available; fall back to system awareness
static FXint applyDPIAwareness(const FXString&){
  SetProcessDPIAware();
  return DPIAWARENESS_SYSTEM;
  }

#endif


typedef UINT (WINAPI *GETDPIFORSYSTEMFUNC)();
typedef UINT (WINAPI *GETDPIFORWINDOWFUNC)(HWND hwnd);


// Establish the requested process DPI awareness
FXint fxplatformSetDPIAwareness(const FXString& policy){
  return applyDPIAwareness(policy);
  }


// Return the system DPI
FXint fxplatformGetSystemDPI(){
  HMODULE user32dll=GetModuleHandleA("user32.dll");
  if(user32dll){
    GETDPIFORSYSTEMFUNC getdpiforsystem=(GETDPIFORSYSTEMFUNC)GetProcAddress(user32dll,"GetDpiForSystem");
    if(getdpiforsystem){
      UINT dpi=getdpiforsystem();
      if(dpi) return (FXint)dpi;
      }
    }
  HDC dc=GetDC(nullptr);
  FXint dpi=dc?GetDeviceCaps(dc,LOGPIXELSY):96;
  if(dc) ReleaseDC(nullptr,dc);
  return dpi>0?dpi:96;
  }


// Return the DPI of the monitor a native window is on, or 0 if unknown
FXint fxplatformGetWindowDPI(FXID window){
  HMODULE user32dll=GetModuleHandleA("user32.dll");
  if(user32dll && window){
    GETDPIFORWINDOWFUNC getdpiforwindow=(GETDPIFORWINDOWFUNC)GetProcAddress(user32dll,"GetDpiForWindow");
    if(getdpiforwindow){
      UINT dpivalue=getdpiforwindow((HWND)window);
      if(dpivalue) return (FXint)dpivalue;
      }
    }
  return 0;
  }


// Get an event
FXbool FXApp::getNextEvent(FXRawEvent& msg,FXTime blocking){
  FXint allinputs;
  DWORD signaled;

  // Set to no-op just in case
  msg.message=0;

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
    while(--nxt && !signals[nxt].notified);
    signalreceived=nxt;
    if(signals[sig].target && signals[sig].target->tryHandle(this,FXSEL(SEL_SIGNAL,signals[sig].message),(void*)(FXival)sig)) refresh();
    return false;
    }

  // Peek for messages; this marks the message queue as unsignalled, i.e.
  // MsgWaitForMultipleObjects would block even if there are unhandled events;
  // the fix is to call MsgWaitForMultipleObjects only AFTER having ascertained
  // that there are NO unhandled events queued up.
  if(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) return true;

  // Poll to see if any waitable objects are signalled
  allinputs=maxhandle+1;
  signaled=MsgWaitForMultipleObjects(allinputs,handles->hnd,false,0,QS_ALLINPUT);
// signaled=MsgWaitForMultipleObjectsEx(allinputs,handles,0,QS_ALLINPUT,MWMO_ALERTABLE|MWMO_INPUTAVAILABLE);

  // No objects were signalled, so perform background tasks now
  if(signaled==WAIT_TIMEOUT){

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

    // No updates or chores pending, so return at this point if not blocking
    if(blocking<=0) return false;

    // One more call to PeekMessage here because the preceding idle
    // processing may have caused some more messages to be posted to
    // our message queue:- a call to MsgWaitForMultipleObjects when
    // there are events already in the queue would NOT immediately fall
    // through but block until the next event comes in.
    if(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) return true;

    // If there are timers, block only a little time
    allinputs=maxhandle+1;
    if(timers || blocking<forever){
      FXTime interval;

      // All that testing above may have taken some time...
      if(timers && (interval=timers->due-FXThread::time())<blocking) blocking=interval;

      // Some timers are already due; do them right away!
      if(blocking<=0) return false;

      DWORD delta=(DWORD)(blocking/1000000);

      // Exit critical section
      appMutex.unlock();

      // Now we will block...
      signaled=MsgWaitForMultipleObjects(allinputs,handles->hnd,false,delta,QS_ALLINPUT);
      //signaled=MsgWaitForMultipleObjectsEx(allinputs,handles,delta,QS_ALLINPUT,MWMO_ALERTABLE|MWMO_INPUTAVAILABLE);

      // Enter critical section
      appMutex.lock();
      }

    // No timers, so block indefinitely
    else{

      // Exit critical section
      appMutex.unlock();

      // Now we will block...
      signaled=MsgWaitForMultipleObjects(allinputs,handles->hnd,false,INFINITE,QS_ALLINPUT);
      //signaled=MsgWaitForMultipleObjectsEx(allinputs,handles,INFINITE,QS_ALLINPUT,MWMO_ALERTABLE|MWMO_INPUTAVAILABLE);

      // Enter critical section
      appMutex.lock();
      }
    }

  // Timed out, so do timeouts
  if(signaled==WAIT_TIMEOUT) return false;

  // Signallable object was signalled
  if(WAIT_OBJECT_0<=signaled && signaled<WAIT_OBJECT_0+allinputs){

    // Process ALL objects which are signalled after returning from
    // MsgWaitForMultipleObjects. We copy the stuff out of the arrays
    // before issueing callbacks, in case an entry is removed.
    for(FXint i=0; i<=maxhandle; i++){
      FXInputHandle fff=handles->hnd[i];
      if((i==(FXint)(signaled-WAIT_OBJECT_0)) || (WaitForSingleObject(fff,0)==WAIT_OBJECT_0)){
        FXInput in=inputs[i];
        if(in.read.target && in.read.target->tryHandle(this,FXSEL(SEL_IO_READ,in.read.message),in.read.data)) refresh();
        if(in.write.target && in.write.target->tryHandle(this,FXSEL(SEL_IO_WRITE,in.write.message),in.write.data)) refresh();
        if(in.excpt.target && in.excpt.target->tryHandle(this,FXSEL(SEL_IO_EXCEPT,in.excpt.message),in.excpt.data)) refresh();
        }
      }
    }

  // Got message from the GUI?
  if(signaled!=WAIT_OBJECT_0+allinputs) return false;

  // Get the event; this used to be GetMessage(&msg,NULL,0,0),
  // but for some reason, this occasionally blocks even though we
  // have tried to make sure an event was indeed available.
  // The new code will always fall through, with an event if there
  // is one, or without one if despite our efforts above there wasn't.
  // Thanks to Hodju Petri <phodju@cc.hut.fi> for this suggestion.
  return PeekMessage(&msg,nullptr,0,0,PM_REMOVE)!=0;
  }


// Peek for event
FXbool FXApp::peekEvent(){
  if(initialized){
    MSG msg;

    // Still need GUI update
    if(refresher) return true;

    // Outstanding chores
    if(chores) return true;

    // Timers are due?
    if(timers){
      if(timers->due <= FXThread::time()) return true;
      }

    // Other events due?
    return PeekMessage(&msg,nullptr,0,0,PM_NOREMOVE)!=0;
    }
  return false;
  }


// Flush pending repaints
void FXApp::flush(FXbool){
  if(initialized){
    GdiFlush();
    }
  }


// Paint all windows marked for repainting
void FXApp::repaint(){
  if(initialized){
    for(FXWindow *top=getRootWindow()->getFirst(); top; top=top->getNext()){
      RedrawWindow((HWND)top->id(),nullptr,nullptr,RDW_ERASENOW|RDW_UPDATENOW|RDW_ALLCHILDREN);
      }
    }
  }


}

#endif
