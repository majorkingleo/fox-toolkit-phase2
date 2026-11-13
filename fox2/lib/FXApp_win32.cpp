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
#include "FXComposeContext.h"
#include "fxapppriv.h"
#include "fxplatform.h"
#include "fxpriv.h"

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


// This window procedure is a static member function of class FXApp.
// Its sole purpose is to forward the message info on to FXApp::dispatchEvent().
FXival CALLBACK FXApp::wndproc(FXID hwnd,FXuint iMsg,FXuval wParam,FXival lParam){
  return app->dispatchEvent(hwnd,iMsg,wParam,lParam);
  }


// Begin of wait-cursor block; wait-cursor blocks may be nested.
void FXApp::beginWaitCursor(){
  if(initialized){
    if(waitCount==0){
      if(!waitCursor->id()){ fxerror("%s::beginWaitCursor: wait cursor not created yet.\n",getClassName()); }
      SetCursor((HCURSOR)waitCursor->id());
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
      if(cursorWindow){
        SetCursor((HCURSOR)cursorWindow->getDefaultCursor()->id());
        }
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
        SetCursor((HCURSOR)waitCursor->id());
        }
      }
    }
  }


// Register DND type
FXDragType FXApp::registerDragType(const FXString& name) const {
  if(initialized){
    return RegisterClipboardFormatA(name.text());
    }
  return 0;
  }


// Get name of registered drag type
FXString FXApp::getDragTypeName(FXDragType type) const {
  if(initialized){
    FXString result;
    if(0xC000<=type && type<=0xFFFF){
      FXchar buffer[256];
      GetClipboardFormatNameA(type,buffer,sizeof(buffer));
      result.assign(buffer);
      }
    else{
      result.assign("WIN32_DEFAULT_TYPE");
      }
    return result;
    }
  return FXString::null;
  }


// Add a signal message
void FXApp::addSignal(FXint sig,FXObject* tgt,FXSelector sel,FXbool immediate,FXuint){
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
#if defined(__IBMCPP__)
    if(signal(sig,(_SigFunc)handler)==SIG_ERR){ fxwarning("%s::addSignal: error setting signal handler\n",getClassName()); }
#else
    if(signal(sig,handler)==SIG_ERR){ fxwarning("%s::addSignal: error setting signal handler\n",getClassName()); }
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
    if(signal(sig,SIG_DFL)==SIG_ERR){ fxwarning("%s::removeSignal: error removing signal handler\n",getClassName()); }
    signals[sig].target=nullptr;
    signals[sig].message=0;
    signals[sig].handlerset=false;
    signals[sig].notified=false;
    }
  }


// Add input
FXbool FXApp::addInput(FXObject *tgt,FXSelector sel,FXInputHandle fd,FXuint mode,FXptr ptr){
  if(mode==INPUT_NONE) return false;
  FXint in;
  if(fd==INVALID_HANDLE_VALUE || fd==nullptr) return false;
  for(in=0; in<=maxhandle; in++){       // See if existing handle
    if(handles->hnd[in]==fd) goto r;    // If existing handle, just replace callbacks
    }
  if(maxhandle+1>=MAXIMUM_WAIT_OBJECTS) return false;
  if(maxhandle+1>=ninputs){             // Grow table of callbacks
    resizeElms(inputs,ninputs+1);
    clearElms(&inputs[ninputs],1);
    ninputs=ninputs+1;
    }
  in=++maxhandle;                       // One more handle
  handles->hnd[in]=fd;
r:FXASSERT(in<ninputs);
  if(mode&INPUT_READ){                  // Replace callbacks
    inputs[in].read.target=tgt;
    inputs[in].read.message=sel;
    inputs[in].read.data=ptr;
    }
  if(mode&INPUT_WRITE){
    inputs[in].write.target=tgt;
    inputs[in].write.message=sel;
    inputs[in].write.data=ptr;
    }
  if(mode&INPUT_EXCEPT){
    inputs[in].excpt.target=tgt;
    inputs[in].excpt.message=sel;
    inputs[in].excpt.data=ptr;
    }
  return true;
  }


// Remove input
FXbool FXApp::removeInput(FXInputHandle fd,FXuint mode){
  if(mode==INPUT_NONE) return false;
  FXint in;
  if(fd==INVALID_HANDLE_VALUE || fd==nullptr) return false;
  for(in=0; in<=maxhandle; in++){       // See if existing handle
    if(handles->hnd[in]==fd) goto r;
    }
  return false;                         // Handle didn't exist, so nothing to remove
r:if(mode&INPUT_READ){
    inputs[in].read.target=nullptr;
    inputs[in].read.message=0;
    inputs[in].read.data=nullptr;
    }
  if(mode&INPUT_WRITE){
    inputs[in].write.target=nullptr;
    inputs[in].write.message=0;
    inputs[in].write.data=nullptr;
    }
  if(mode&INPUT_EXCEPT){
    inputs[in].excpt.target=nullptr;
    inputs[in].excpt.message=0;
    inputs[in].excpt.data=nullptr;
    }
  if(!inputs[in].read.target && !inputs[in].write.target && !inputs[in].excpt.target){  // Removed them all
    handles->hnd[in]=handles->hnd[maxhandle];      // Compact handle table
    inputs[in]=inputs[maxhandle];
    maxhandle--;
    }
  return true;
  }


// Return true if input method support
FXbool FXApp::hasInputMethod() const {
  return true;
  }


// Find window from root x,y, starting from given window
FXWindow* FXApp::findWindowAt(FXint rx,FXint ry,FXID window) const {
  if(initialized){
    POINT point;
    FXID child;
    if(!window) window=(FXID)GetDesktopWindow();
    while(1){
      point.x=rx; point.y=ry;
      ScreenToClient((HWND)window,&point);
      child=ChildWindowFromPointEx((HWND)window,point,CWP_SKIPINVISIBLE|CWP_SKIPTRANSPARENT);   // Suggested by "PelleBert"
      if(!child || child==window) break;
      window=child;
      }
    return findWindowWithId(window);
    }
  return nullptr;
  }


// Return key state
FXbool FXApp::getKeyState(FXuint keysym) const {
  return GetKeyState(keysym)!=0;
  }


// Beep
void FXApp::beep(){
  if(initialized){
    FXTRACE(TOPIC_EVENT,"Beep\n");
    MessageBeep(0);
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

    // Atoms created using GlobalCreateAtom() are reference-counted by
    // the system; calling GlobalDeleteAtom() here just decrements the
    // reference count but doesn't necessarily free the memory.
    GlobalDeleteAtom(ddeTargets);
    GlobalDeleteAtom(xdndAware);

    // Free standard stipples
    DeleteObject(stipples[STIPPLE_0]);
    DeleteObject(stipples[STIPPLE_1]);
    DeleteObject(stipples[STIPPLE_2]);
    DeleteObject(stipples[STIPPLE_3]);
    DeleteObject(stipples[STIPPLE_4]);
    DeleteObject(stipples[STIPPLE_5]);
    DeleteObject(stipples[STIPPLE_6]);
    DeleteObject(stipples[STIPPLE_7]);
    DeleteObject(stipples[STIPPLE_8]);
    DeleteObject(stipples[STIPPLE_9]);
    DeleteObject(stipples[STIPPLE_10]);
    DeleteObject(stipples[STIPPLE_11]);
    DeleteObject(stipples[STIPPLE_12]);
    DeleteObject(stipples[STIPPLE_13]);
    DeleteObject(stipples[STIPPLE_14]);
    DeleteObject(stipples[STIPPLE_15]);
    DeleteObject(stipples[STIPPLE_16]);

    // Clear it
    display=nullptr;

    // Unlock the global mutex
    appMutex.unlock();

    // We are no longer initialized
    initialized=false;
    }
  return true;
  }




// Dispatch event to widget
FXbool FXApp::dispatchEvent(FXRawEvent& msg){
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  return true;
  }



#ifndef WM_SYNCPAINT
#define WM_SYNCPAINT 0x0088
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif


// Translate to string on KeyPress
FXString translateKeyEvent(FXuint,FXuval wParam,FXival lParam){
  FXnchar buffer[20]; BYTE keystate[256]; int n;
  GetKeyboardState(keystate);
  n=ToUnicodeEx((FXuint)wParam,HIWORD(lParam)&(KF_EXTENDED|KF_UP|0xFF),keystate,buffer,20,0,GetKeyboardLayout(0));
  if(n<=0) n=0;
  return FXString(buffer,n);
  }


// Message dispatching
FXival FXApp::dispatchEvent(FXID hwnd,FXuint iMsg,FXuval wParam,FXival lParam){
  const FXTime milliseconds=1000000;
  FXWindow *window,*ancestor,*win,*focuswin;
  static HWND lastmovehwnd=0;
  static LPARAM lastmovelParam=0;
  POINT ptRoot, pt;
  DWORD dwpts;
  PAINTSTRUCT ps;
  FXuint state;
  HANDLE hMap;
  HANDLE answer;

  if(!IsWindow((HWND)hwnd))
    return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

  // Get window
  window=findWindowWithId(hwnd);

  if(window==0 && iMsg!=WM_CREATE)
    return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

  // Translate Win32 message to FOX message type
  switch(iMsg){

    // Repaint event
    case WM_PAINT:
      event.type=SEL_PAINT;
      event.synthetic=1;              // FIXME when is it non-synthetic?
      BeginPaint((HWND)hwnd,&ps);
      event.rect.x=(FXshort)ps.rcPaint.left;
      event.rect.y=(FXshort)ps.rcPaint.top;
      event.rect.w=(FXshort)(ps.rcPaint.right-ps.rcPaint.left);
      event.rect.h=(FXshort)(ps.rcPaint.bottom-ps.rcPaint.top);
      window->handle(this,FXSEL(SEL_PAINT,0),&event);
      EndPaint((HWND)hwnd,&ps);
      return 0;

    // Start Input Method Editor
    case WM_IME_STARTCOMPOSITION:
      focuswin=getFocusWindow();
      if(inputstyle[1]!='o'){ // not root mode
        if(focuswin && focuswin->getComposeContext())
          focuswin->handle(this,FXSEL(SEL_IME_START,0),nullptr);
        else
          return 0;
        }
      // on-the-spot mode
      if(inputstyle[1]=='n') return 0;
      // other mode
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    // End Input Method Editor
    case WM_IME_ENDCOMPOSITION:
      focuswin=getFocusWindow();
      if(inputstyle[1]!='o'){ // not root mode
        if(focuswin && focuswin->getComposeContext())
          focuswin->handle(this,FXSEL(SEL_IME_END,0),nullptr);
        else
          return 0;
        }
      // on-the-spot mode
      if(inputstyle[1]=='n') return 0;
      // other mode
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    // Character Input from Input Method Editor
    case WM_IME_CHAR:
      // ignore this message, coming after WM_IME_COMPOSITION
      // because IME string is handled in WM_IME_COMPOSITION message
      return 0;

    // Keyboard
    case WM_IME_COMPOSITION:
      if((inputstyle[1]!='n') && !(lParam&GCS_RESULTSTR)){
        focuswin=getFocusWindow();
        if(focuswin && focuswin->getComposeContext())
          return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
        else
          return 0;
        }
      // else keep going
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
      FXTRACE(TOPIC_KEYBOARD,"%s virtkey=%c [0x%04x] hi=0x%04x rc=%d\n",iMsg==WM_KEYDOWN?"WM_KEYDOWN":iMsg==WM_KEYUP?"WM_KEYUP":iMsg==WM_SYSKEYDOWN?"WM_SYSKEYDOWN":"WM_SYSKEYUP",wParam,wParam,HIWORD(lParam),LOWORD(lParam));
      event.type=((iMsg==WM_KEYUP)||(iMsg==WM_SYSKEYUP)) ? SEL_KEYRELEASE : SEL_KEYPRESS;
      event.time=GetMessageTime();
      dwpts=GetMessagePos();
      event.root_x=pt.x=((int)(short)LOWORD(dwpts));
      event.root_y=pt.y=((int)(short)HIWORD(dwpts));
      ScreenToClient((HWND)hwnd,&pt);
      event.win_x=pt.x;
      event.win_y=pt.y;

      event.state=fxmodifierkeys();

      // Translate to keysym
      event.code=wkbMapKeyCode(iMsg,wParam,lParam);// FIXME not all codes match with those of X11

      // Translate to string on KeyPress
      if(event.type==SEL_KEYPRESS){
        focuswin=getFocusWindow();
        if( (iMsg==WM_IME_COMPOSITION) && (lParam&GCS_RESULTSTR) && focuswin && focuswin->getComposeContext()){
          FXRawEvent ev;
          ev.hwnd=(HWND)hwnd;
          ev.message=iMsg;
          ev.wParam=wParam;
          ev.lParam=lParam;
          event.text=focuswin->getComposeContext()->translateEvent(ev);
          }
        else{
          event.text=translateKeyEvent(iMsg,wParam,lParam);
          }
        }

      // Clear string on KeyRelease
      else{
        event.text.clear();
        }

/*
Alt key seems to repeat.
*/

      // Fix modifier state
      if(event.type==SEL_KEYPRESS){
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
        return 0;
        }

      // Remember window for later
      if(event.type==SEL_KEYPRESS) keyWindow=activeWindow;

      // Dispatch to key window
      if(keyWindow){

        // FIXME doesSaveUnder test should go away
        // Dispatch if not in a modal loop or in a modal loop for a window containing the focus window
        if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(keyWindow)) || keyWindow->getShell()->doesSaveUnder()){
          if(keyWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
          if((iMsg==WM_IME_COMPOSITION) && (inputstyle[1]!='n')){
            // it is possible that undetermined IME characters remain
            if(inputstyle[1]!='o'){
              focuswin=getFocusWindow();
              if(focuswin){
                focuswin->handle(this,FXSEL(SEL_IME_START,0),nullptr);
                }
              }
            return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
            }
          return 0;
          }

        // Beep if outside modal
        if(event.type==SEL_KEYPRESS) beep();
        }
      return 0;

    // The grab might be broken; in FOX, we ignore this!!
    case WM_CANCELMODE:
      //FXTRACE(TOPIC_EVENT,"WM_CANCELMODE\n");
      return 0;

    // Capture changed
    case WM_CAPTURECHANGED:
      //FXTRACE(TOPIC_EVENT,"WM_CAPTURECHANGED\n");
      // FIXME send SEL_UNGRABBED?
      return 0;

    // Motion
    case WM_MOUSEMOVE:
      event.time=GetMessageTime();
      pt.x=ptRoot.x=(int)((short)LOWORD(lParam));
      pt.y=ptRoot.y=(int)((short)HIWORD(lParam));
      ClientToScreen((HWND)hwnd,&ptRoot);
      event.root_x=ptRoot.x;
      event.root_y=ptRoot.y;
      event.state=fxmodifierkeys();

      //FXTRACE(TOPIC_EVENT,"WM_MOUSEMOVE hwnd=%d x=%d y=%d \n",hwnd,event.root_x,event.root_y);

      // Reset hover timer
      addTimeout(this,ID_HOVER,200000000);

      // Set moved flag
      if((FXABS(event.root_x-event.rootclick_x)>=dragDelta) || (FXABS(event.root_y-event.rootclick_y)>=dragDelta)) event.moved=1;

      // Was grabbed
      if(mouseGrabWindow){

        // Translate to grab window's coordinate system
        root->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.root_x,event.root_y);

        // Moved out of/into rectangle of grabbed window
        if(0<=event.win_x && event.win_x<mouseGrabWindow->getWidth() && 0<=event.win_y && event.win_y<mouseGrabWindow->getHeight()){
          window=mouseGrabWindow;
          }
        else{
          window=mouseGrabWindow->getParent();
          }
        }

      // Switched windows
      if(cursorWindow!=window){
        ancestor=FXWindow::commonAncestor(window,cursorWindow);
        event.code=CROSSINGNORMAL;
        leaveWindow(cursorWindow,ancestor);
        enterWindow(window,ancestor);
        refresh();
        }

      // Suppress spurious `tickling' motion events
      if(hwnd==lastmovehwnd && lParam==lastmovelParam) return 0;

      // Was still grabbed, but possibly new grab window!
      if(mouseGrabWindow){

        // Translate to grab window's coordinate system
        root->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.root_x,event.root_y);

        // Set event data
        event.type=SEL_MOTION;
        event.code=0;

        // Dispatch to grab-window
        if(mouseGrabWindow->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
        }

// FIXME Does window still exist?

      // Was not grabbed
      else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){ // FIXME doesSaveUnder test should go away

        // Set event data
        event.type=SEL_MOTION;
        event.code=0;
        event.win_x=pt.x;
        event.win_y=pt.y;

        // Dispatch to window under cursor
        if(window->handle(this,FXSEL(SEL_MOTION,0),&event)) refresh();
        }

      // Update most recent mouse position
      event.last_x=pt.x;
      event.last_y=pt.y;

      // Remember this for tickling test
      lastmovehwnd=(HWND)hwnd;
      lastmovelParam=lParam;

      return 0;

    // Button
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      event.time=GetMessageTime();
      event.win_x=pt.x=(int)((short)LOWORD(lParam));
      event.win_y=pt.y=(int)((short)HIWORD(lParam));
      ClientToScreen((HWND)hwnd,&pt);
      event.root_x=pt.x;
      event.root_y=pt.y;
      event.state=fxmodifierkeys();   // Get the state of the modifier keys and mouse buttons
      if(iMsg==WM_LBUTTONDOWN || iMsg==WM_MBUTTONDOWN || iMsg==WM_RBUTTONDOWN){
        if(iMsg==WM_LBUTTONDOWN){ event.type=SEL_LEFTBUTTONPRESS; event.code=LEFTBUTTON; }
        if(iMsg==WM_MBUTTONDOWN){ event.type=SEL_MIDDLEBUTTONPRESS; event.code=MIDDLEBUTTON; }
        if(iMsg==WM_RBUTTONDOWN){ event.type=SEL_RIGHTBUTTONPRESS; event.code=RIGHTBUTTON; }
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
      else{
        if(iMsg==WM_LBUTTONUP){ event.type=SEL_LEFTBUTTONRELEASE; event.code=LEFTBUTTON; }
        if(iMsg==WM_MBUTTONUP){ event.type=SEL_MIDDLEBUTTONRELEASE; event.code=MIDDLEBUTTON; }
        if(iMsg==WM_RBUTTONUP){ event.type=SEL_RIGHTBUTTONRELEASE; event.code=RIGHTBUTTON; }
        }
      if(mouseGrabWindow){
        window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
        if(mouseGrabWindow->handle(this,FXSEL(event.type,0),&event)) refresh();
        }
      // FIXME doesSaveUnder test should go away
      else if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder()){
        if(window->handle(this,FXSEL(event.type,0),&event)) refresh();
        }
      else if(iMsg==WM_LBUTTONDOWN || iMsg==WM_MBUTTONDOWN || iMsg==WM_RBUTTONDOWN){
        beep();
        }
      event.last_x=event.win_x;
      event.last_y=event.win_y;
      return 0;

    // Mouse wheel
    case WM_MOUSEWHEEL:
      event.type=SEL_MOUSEWHEEL;
      event.time=GetMessageTime();
      event.root_x=pt.x=(int)((short)LOWORD(lParam));
      event.root_y=pt.y=(int)((short)HIWORD(lParam));
      event.code=(int)((short)HIWORD(wParam));
      event.state=fxmodifierkeys();   // Get the state of the modifier keys and mouse buttons
      window=findWindowAt(event.root_x,event.root_y);
      if(window){
        ScreenToClient((HWND)window->id(),&pt);
        event.win_x=pt.x;
        event.win_y=pt.y;
        if(mouseGrabWindow){
          window->translateCoordinatesTo(event.win_x,event.win_y,mouseGrabWindow,event.win_x,event.win_y);
          if(mouseGrabWindow->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)) refresh();
          }
        else{
          // FIXME doesSaveUnder test should go away
          while(window && (!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window)) || window->getShell()->doesSaveUnder())){
            if(window->handle(this,FXSEL(SEL_MOUSEWHEEL,0),&event)){ refresh(); break; }
            window=window->getParent();
            }
          }
        }
      return 0;

    // Focus
    case WM_SETFOCUS:
      SendMessage((HWND)window,WM_NCACTIVATE,1,123456); // Suggestion from: Frank De prins <fdp@MCS.BE>
    case WM_KILLFOCUS:
      window=window->getShell();
      if(iMsg==WM_KILLFOCUS && activeWindow==window){
        event.type=SEL_FOCUSOUT;
        if(window->handle(this,FXSEL(SEL_FOCUSOUT,0),&event)) refresh();
        activeWindow=nullptr;
        }
      if(iMsg==WM_SETFOCUS && activeWindow!=window){
        event.type=SEL_FOCUSIN;
        if(window->handle(this,FXSEL(SEL_FOCUSIN,0),&event)) refresh();
        activeWindow=window;
        }
      return 0;

    // Map or Unmap
    case WM_SHOWWINDOW:
      if(wParam){
        event.type=SEL_MAP;
        if(window->handle(this,FXSEL(SEL_MAP,0),&event)) refresh();
        }
      else{
        event.type=SEL_UNMAP;
        if(window->handle(this,FXSEL(SEL_UNMAP,0),&event)) refresh();
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    // Create
    case WM_CREATE:
      event.type=SEL_CREATE;
      window=(FXWindow*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
      hash.insert((FXptr)hwnd,window);
      if(window->handle(this,FXSEL(SEL_CREATE,0),&event)) refresh();
      return 0;

    // Close
    case WM_CLOSE:

      ////// Semantics: SEL_CLOSE is a suggestion that the window be closed;
      ////// SEL_DESTROY is a notify that destruction has already taken place.
      ////// Thus, a toplevel window gets a close, and MAY decide not to be closed;
      ////// If it thinks its OK to close, it gets a SEL_DESTROY also.
      event.type=SEL_CLOSE;
      if(!invocation || invocation->modality==MODAL_FOR_NONE || (invocation->window && invocation->window->isOwnerOf(window))){
        if(window->handle(this,FXSEL(SEL_CLOSE,0),&event)) refresh();
        }
      else{
        beep();
        }
      return 0;

    // Destroy
    case WM_DESTROY:
      event.type=SEL_DESTROY;
      if(window->handle(this,FXSEL(SEL_DESTROY,0),&event)) refresh();
      return 0;

    // Configure (size)
    case WM_SIZE:
      switch(wParam){
        case SIZE_MINIMIZED:            // Send SEL_MINIMIZE to window if not yet minimized
          if(!IsIconic((HWND)hwnd)){
            event.type=SEL_MINIMIZE;
            if(window->handle(this,FXSEL(SEL_MINIMIZE,0),&event)) refresh();
            }
          break;
        case SIZE_MAXIMIZED:            // Send SEL_MAXIMIZE to window if not yet maximized
          if(!IsZoomed((HWND)hwnd)){
            event.type=SEL_MAXIMIZE;
            if(window->handle(this,FXSEL(SEL_MAXIMIZE,0),&event)) refresh();
            }
          event.type=SEL_CONFIGURE;
          event.rect.x=window->getX();
          event.rect.y=window->getY();
          event.rect.w=LOWORD(lParam);
          event.rect.h=HIWORD(lParam);
          if(window->handle(this,FXSEL(SEL_CONFIGURE,0),&event)) refresh();
          break;
        case SIZE_RESTORED:             // Send SEL_RESTORE to window if maximized or minimized
          if(IsZoomed((HWND)hwnd) || IsIconic((HWND)hwnd)){
            event.type=SEL_RESTORE;
            if(window->handle(this,FXSEL(SEL_RESTORE,0),&event)) refresh();
            }
        case SIZE_MAXHIDE:
        case SIZE_MAXSHOW:
          event.type=SEL_CONFIGURE;
          event.rect.x=window->getX();
          event.rect.y=window->getY();
          event.rect.w=LOWORD(lParam);
          event.rect.h=HIWORD(lParam);
          if(window->handle(this,FXSEL(SEL_CONFIGURE,0),&event)) refresh();
          break;
        }
      return 0;

    // Configure (move)
    case WM_MOVE:
      event.type=SEL_CONFIGURE;
      event.rect.x=(short)LOWORD(lParam);
      event.rect.y=(short)HIWORD(lParam);
      event.rect.w=window->getWidth();
      event.rect.h=window->getHeight();
      if(window->handle(this,FXSEL(SEL_CONFIGURE,0),&event)) refresh();
      return 0;

    // Configure position and size
    case WM_WINDOWPOSCHANGED:
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    // DPI of the window's monitor changed
    case WM_DPICHANGED:
      {
        RECT *suggested=(RECT*)lParam;
        FXint newdpi=(FXint)HIWORD(wParam);
        if(newdpi<=0) newdpi=(FXint)LOWORD(wParam);
        if(newdpi<=0) break;
        applyWindowDPI(window,newdpi);
        dpicurrent=newdpi;
        if(dpiautoscale) setThemeScale(dpifactor*FXdouble(newdpi)/96.0);
        if(suggested){
          SetWindowPos((HWND)hwnd,nullptr,suggested->left,suggested->top,suggested->right-suggested->left,suggested->bottom-suggested->top,SWP_NOZORDER|SWP_NOACTIVATE);
          }
      }
      return 0;

    // DPI changed because the parent window changed DPI
    case WM_DPICHANGED_AFTERPARENT:
      {
        FXint newdpi=getWindowDPI(hwnd);
        if(newdpi>0) applyWindowDPI(window,newdpi);
      }
      return 0;

    // Lost clipboard ownership
    case WM_DESTROYCLIPBOARD:
      if(clipboardWindow){
        event.time=GetMessageTime();
        event.type=SEL_CLIPBOARD_LOST;
        if(clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_LOST,0),&event)) refresh();
        clipboardWindow=nullptr;
        }
      return 0;

    // Safeguard it in the clipboard
    case WM_RENDERALLFORMATS:
      if(clipboardWindow){
        OpenClipboard((HWND)hwnd);
        EmptyClipboard();
        UINT iFormat=0;
        while((iFormat=EnumClipboardFormats(iFormat))!=0){
          event.type=SEL_CLIPBOARD_REQUEST;
          event.time=GetMessageTime();
          event.target=iFormat;
          if(clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_REQUEST,0),&event)) refresh();
          FXTRACE(TOPIC_EVENT,"Window %d being requested for CLIPBOARD DATA of type %d\n",hwnd,wParam);
          }
        CloseClipboard();
        }
      return 0;

    // We're asked to provide certain format to the clipboard
    case WM_RENDERFORMAT:
      if(clipboardWindow){
        event.type=SEL_CLIPBOARD_REQUEST;
        event.time=GetMessageTime();
        event.target=(FXDragType)wParam;
        clipboardWindow->handle(this,FXSEL(SEL_CLIPBOARD_REQUEST,0),&event);
        FXTRACE(TOPIC_EVENT,"Window %d being requested for CLIPBOARD DATA of type %d\n",hwnd,wParam);
        }
      return 0;

    // Change the cursor based on the window
    case WM_SETCURSOR:
      if(waitCount){
        SetCursor((HCURSOR)waitCursor->id());                     // Show wait cursor
        return 0;
        }
      if(!mouseGrabWindow && window->getDefaultCursor() && (LOWORD(lParam)==HTCLIENT)){
        SetCursor((HCURSOR)window->getDefaultCursor()->id());     // Show default cursor
        return 0;
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    case WM_PALETTECHANGED:     // Suggested by Boris Kogan <bkogan@j51.com>
      if((HWND)wParam==hwnd) break;
    case WM_QUERYNEWPALETTE:
      if(window->getVisual()->colormap){
        HDC hdc=GetDC((HWND)hwnd);
        HPALETTE paletteold=SelectPalette(hdc,(HPALETTE)window->getVisual()->colormap,false);
        UINT nchanged=RealizePalette(hdc);
        if(nchanged) InvalidateRect((HWND)hwnd,nullptr,true);
        SelectPalette(hdc,paletteold,true);
        ReleaseDC((HWND)hwnd,hdc);
        return nchanged;
        }
      return 0;

    case WM_QUERYENDSESSION:    // Session will end if this app thinks its OK
      event.type=SEL_SESSION_NOTIFY;
      return !window->handle(this,FXSEL(SEL_SESSION_NOTIFY,0),&event); // Return 1 if OK to terminate

    case WM_ENDSESSION:         // Session will now end for sure
      event.type=SEL_SESSION_CLOSED;
      if(wParam) window->handle(this,FXSEL(SEL_SESSION_CLOSED,0),&event);
      return 0;

    case WM_STYLECHANGING:
    case WM_SIZING:
    case WM_MOVING:
      return 0;
    case WM_ERASEBKGND:         // Do nothing, erasing background causes flashing
      return 1;

    case WM_ACTIVATE:
      //FXTRACE(TOPIC_EVENT,"WM_ACTIVATE %d\n",LOWORD(wParam));
      if(window->isMemberOf(FXMETACLASS(FXTopWindow)) && activeWindow && activeWindow!=window){   // Suggestion from: Frank De prins <fdp@MCS.BE>
        SendMessage((HWND)activeWindow->id(),WM_NCACTIVATE,0,123456);
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    case WM_GETMINMAXINFO:
      if(window->id() && window->shown() &&window->isMemberOf(FXMETACLASS(FXTopWindow))){
        RECT rect;
        //FXTRACE(TOPIC_EVENT,"WM_GETMINMAXINFO ptMaxSize=%d,%d ptMinTrackSize=%d,%d ptMaxTrackSize=%d,%d\n",((MINMAXINFO*)lParam)->ptMaxSize.x,((MINMAXINFO*)lParam)->ptMaxSize.y,((MINMAXINFO*)lParam)->ptMinTrackSize.x,((MINMAXINFO*)lParam)->ptMinTrackSize.y,((MINMAXINFO*)lParam)->ptMaxTrackSize.x,((MINMAXINFO*)lParam)->ptMaxTrackSize.y);
        if(!(((FXTopWindow*)window)->getDecorations()&DECOR_SHRINKABLE)){
          if(!(((FXTopWindow*)window)->getDecorations()&DECOR_STRETCHABLE)){    // Cannot change at all
            SetRect(&rect,0,0,window->getWidth(),window->getHeight());
            //SetRect(&rect,0,0,window->getDefaultWidth(),window->getDefaultHeight());
            AdjustWindowRectEx(&rect,GetWindowLong((HWND)hwnd,GWL_STYLE),false,GetWindowLong((HWND)hwnd,GWL_EXSTYLE));
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x=((MINMAXINFO*)lParam)->ptMaxTrackSize.x=rect.right-rect.left;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y=((MINMAXINFO*)lParam)->ptMaxTrackSize.y=rect.bottom-rect.top;
            }
          else{                                                                 // Cannot get smaller than default
            SetRect(&rect,0,0,window->getDefaultWidth(),window->getDefaultHeight());
            AdjustWindowRectEx(&rect,GetWindowLong((HWND)hwnd,GWL_STYLE),false,GetWindowLong((HWND)hwnd,GWL_EXSTYLE));
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x=rect.right-rect.left;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y=rect.bottom-rect.top;
            }
          }
        else if(!(((FXTopWindow*)window)->getDecorations()&DECOR_STRETCHABLE)){ // Cannot get larger than default
          SetRect(&rect,0,0,window->getDefaultWidth(),window->getDefaultHeight());
          AdjustWindowRectEx(&rect,GetWindowLong((HWND)hwnd,GWL_STYLE),false,GetWindowLong((HWND)hwnd,GWL_EXSTYLE));
          ((MINMAXINFO*)lParam)->ptMaxTrackSize.x=rect.right-rect.left;
          ((MINMAXINFO*)lParam)->ptMaxTrackSize.y=rect.bottom-rect.top;
          }
        //FXTRACE(TOPIC_EVENT,"width=%d height=%d\n",window->getWidth(),window->getHeight());
        //FXTRACE(TOPIC_EVENT,"WM_GETMINMAXINFO ptMaxSize=%d,%d ptMinTrackSize=%d,%d ptMaxTrackSize=%d,%d\n",((MINMAXINFO*)lParam)->ptMaxSize.x,((MINMAXINFO*)lParam)->ptMaxSize.y,((MINMAXINFO*)lParam)->ptMinTrackSize.x,((MINMAXINFO*)lParam)->ptMinTrackSize.y,((MINMAXINFO*)lParam)->ptMaxTrackSize.x,((MINMAXINFO*)lParam)->ptMaxTrackSize.y);
        }
      return 0;

    case WM_INITMENU:           // Patch from Robin Wilson <robin.wilson@abaqus.com>
    case WM_SYSCOMMAND:         // This pops down the menupane when clicking in non-client area
      for(win=getRootWindow()->getFirst(); win; win=win->getNext()){       // FIXME don't we already know popupWindow?
        if(win->shown() && win->isMemberOf(FXMETACLASS(FXMenuPane)) && window->containsChild(win->getOwner())) window->killFocus();
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
#if 0
    case WM_ENTERSIZEMOVE:
      if(!repostimer){
        repostimer=SetTimer((HWND)hwnd,1,10 /* lower doesn't work :( */,nullptr);
        return 0;
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
    case WM_EXITSIZEMOVE:
      if(repostimer){
        KillTimer((HWND)hwnd,1);
        repostimer=0;
        return 0;
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
    case WM_TIMER:
      if(repostimer){
        doIdleProcessing();
        return 0;
        }
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
#endif
    case WM_INPUT:
      //event.type=SEL_SPACEBALLMOTION;
      //if(window->handle(this,FXSEL(event.type,0),(void *)lParam)) refresh();
      return 0;

    case WM_TIMER:              // added by msh 2/DEC/99
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    case WM_ENTERMENULOOP:
    case WM_MENUCHAR:
    case WM_MENUSELECT:
    case WM_EXITMENULOOP:
    case WM_DISPLAYCHANGE:      // added by msh 2/DEC/99
    case WM_ENTERIDLE:          // 24/NOV/99 md
    case WM_NCCREATE:
    case WM_NCDESTROY:
    case WM_PARENTNOTIFY:
    case WM_SETTEXT:
    case WM_GETTEXT:
    case WM_QUERYOPEN:
    case WM_ENABLE:
    case WM_MOUSEACTIVATE:
    case WM_CHILDACTIVATE:
    case WM_SYNCPAINT:
    case WM_NCPAINT:
    case WM_NCHITTEST:
    case WM_NCMOUSEMOVE:
    case WM_NCCALCSIZE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_WINDOWPOSCHANGING:  // Leave whatever placement is suggested
    case WM_STYLECHANGED:
    case WM_MOUSELEAVE:         // We no longer use TrackMouseEvent it is not reliable
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    case WM_NCACTIVATE:         // Suggestion from: Frank De prins <fdp@MCS.BE>
      if(lParam!=123456) wParam=1;
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    case WM_ACTIVATEAPP:        // Suggestion from: Frank De prins <fdp@MCS.BE>
      SendMessage((HWND)hwnd,WM_NCACTIVATE,wParam,123456);
      return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);

    case WM_DROPFILES:
      //char ListFileName[MAX_PATH];
      //HDROP hDropInfo=(HDROP)WParam;
      //DragQueryFile(hDropInfo,0,ListFileName,sizeof(ListFileName)-1);
      //numfiles = DragQueryFile(hDropInfo, (DWORD)(-1), (LPSTR)nullptr, 0);
      //for (fileindex=0; fileindex<numfiles; fileindex++) {
      //  numc = DragQueryFile(hDropInfo,fileindex,ListFileName,sizeof(ListFileName)-1);
      //  }
      //DragFinish(hDropInfo);
      break;
#if 0
    case WM_DDE_INITIATE:
    case WM_DDE_EXECUTE:
    case WM_DDE_ACK:
#endif
    case WM_DND_ENTER:
      FXTRACE(TOPIC_EVENT,"DNDEnter from remote window %d\n",lParam);
      xdndSource=(FXID)lParam;
      if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
      hMap=OpenFileMapping(FILE_MAP_READ,false,TEXT("XdndTypeList"));
      if(hMap){
        FXDragType *dragtypes=(FXDragType*)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
        if(dragtypes){
          if(allocElms(ddeTypeList,dragtypes[0])){
            copyElms(ddeTypeList,&dragtypes[1],dragtypes[0]);
            ddeNumTypes=dragtypes[0];
            }
          UnmapViewOfFile(dragtypes);
          }
        CloseHandle(hMap);
        }
      return 0;

    case WM_DND_LEAVE:
      FXTRACE(TOPIC_EVENT,"DNDLeave from remote window %d\n",lParam);
      if(xdndSource!=(FXID)lParam) return 0;
      if(dropWindow){
        event.type=SEL_DND_LEAVE;
        if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
        dropWindow=nullptr;
        }
      if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
      xdndSource=0;
      return 0;

    case WM_DND_DROP:
      FXTRACE(TOPIC_EVENT,"DNDDrop from remote window %d\n",lParam);
      if(xdndSource!=(FXID)lParam) return 0;
      xdndFinishSent=false;
      event.type=SEL_DND_DROP;
      event.time=GetMessageTime();

      // Perform drop operation on drop window
      if(!dropWindow || !dropWindow->handle(this,FXSEL(SEL_DND_DROP,0),&event)) ansAction=DRAG_REJECT;

      // Didn't sent finish yet
      if(!xdndFinishSent){
        PostMessage((HWND)xdndSource,WM_DND_FINISH_REJECT+ansAction,0,(LPARAM)hwnd);
        }
      if(ddeTypeList){freeElms(ddeTypeList);ddeNumTypes=0;}
      dropWindow=nullptr;
      xdndSource=0;
      refresh();
      return 0;

    case WM_DND_POSITION_REJECT:
    case WM_DND_POSITION_ASK:
    case WM_DND_POSITION_COPY:
    case WM_DND_POSITION_MOVE:
    case WM_DND_POSITION_LINK:
    case WM_DND_POSITION_PRIVATE:
      if(xdndSource!=(FXID)lParam) return 0;
      event.time=GetMessageTime();
      event.root_x=(int)((short)LOWORD(wParam));
      event.root_y=(int)((short)HIWORD(wParam));
      win=findWindowAt(event.root_x,event.root_y);
      ddeAction=(FXDragAction)(iMsg-WM_DND_POSITION_REJECT);    // Action encoded in message
      FXTRACE(TOPIC_EVENT,"DNDPosition from remote window %d action %d\n",lParam,ddeAction);
      ansAction=DRAG_REJECT;
      xdndRect.x=event.root_x;
      xdndRect.y=event.root_y;
      xdndRect.w=1;
      xdndRect.h=1;
      if(win!=dropWindow){
        if(dropWindow){
          event.type=SEL_DND_LEAVE;
          if(dropWindow->handle(this,FXSEL(SEL_DND_LEAVE,0),&event)) refresh();
          }
        dropWindow=nullptr;
        if(win && win->isDropEnabled()){
          dropWindow=win;
          event.type=SEL_DND_ENTER;
          if(dropWindow->handle(this,FXSEL(SEL_DND_ENTER,0),&event)) refresh();
          }
        }
      if(dropWindow){
        pt.x=event.root_x;
        pt.y=event.root_y;
        ScreenToClient((HWND)dropWindow->id(),&pt);
        event.win_x=pt.x;
        event.win_y=pt.y;
        event.type=SEL_DND_MOTION;
        if(dropWindow->handle(this,FXSEL(SEL_DND_MOTION,0),&event)) refresh();
        event.last_x=event.win_x;
        event.last_y=event.win_y;
        }
      FXTRACE(TOPIC_EVENT,"accepting %d\n",ansAction);
      PostMessage((HWND)xdndSource,WM_DND_STATUS_REJECT+ansAction,MAKELONG(xdndRect.x,xdndRect.y),MAKELONG(xdndRect.w,xdndRect.h));
      return 0;

    case WM_DND_STATUS_REJECT:
    case WM_DND_STATUS_ASK:
    case WM_DND_STATUS_COPY:
    case WM_DND_STATUS_MOVE:
    case WM_DND_STATUS_LINK:
    case WM_DND_STATUS_PRIVATE:
      ansAction=(FXDragAction)(iMsg-WM_DND_STATUS_REJECT);
      xdndRect.x=(int)((short)LOWORD(wParam));
      xdndRect.y=(int)((short)HIWORD(wParam));
      xdndRect.w=(int)((short)LOWORD(lParam));
      xdndRect.h=(int)((short)HIWORD(lParam));
      xdndStatusReceived=true;
      xdndStatusPending=false;
      FXTRACE(TOPIC_EVENT,"DNDStatus from remote window action=%d rect x=%d y=%d w=%d h=%d\n",ansAction,xdndRect.x,xdndRect.y,xdndRect.w,xdndRect.h);
      return 0;

    case WM_DND_REQUEST:
      answer=0;
      if(dragWindow){
        event.type=SEL_DND_REQUEST;
        event.time=GetMessageTime();
        event.target=(FXDragType)wParam;
        ddeData=nullptr;
        ddeSize=0;
        dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
        FXTRACE(TOPIC_EVENT,"Window %d being requested by window %d for XDND DATA of type %d; sending %d bytes\n",hwnd,lParam,wParam,ddeSize);
        answer=fxsenddata((HWND)lParam,ddeData,ddeSize);
        freeElms(ddeData);
        ddeData=nullptr;
        ddeSize=0;
        }
      FXTRACE(TOPIC_EVENT,"sending handle %d from window %d to %d\n",answer,hwnd,lParam);
      PostMessage((HWND)lParam,WM_DND_REPLY,(WPARAM)answer,(LPARAM)hwnd);
      return 0;
    case WM_COPYDATA:
      return window->handle(this,FXSEL(SEL_COPYDATA,0),(void*)lParam);
    }
  return DefWindowProc((HWND)hwnd,iMsg,wParam,lParam);
  }



// 17 stipple patterns which match up exactly with the 4x4 dither kernel
// Note that each scan line must be word-aligned so we pad to the right
// with zeroes.
static const BYTE stipple_patterns[17][16]={
  {0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00},   // 0 (white)
  {0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},   // 8 (50% grey)
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}    // 16 (black)
  };




// Trick to find module handle of FOX library
static HINSTANCE GetOwnModuleHandle(){
  MEMORY_BASIC_INFORMATION mbi;
  VirtualQuery((const void*)GetOwnModuleHandle,&mbi,sizeof(mbi));
  return (HINSTANCE)mbi.AllocationBase;
  }


// Open Input devices
void FXApp::openInputDevices(){
#if 0 //(_WIN32_WINNT >= 0x0501)
  RAWINPUTDEVICELIST *devlist;
  RID_DEVICE_INFO devinfo;
  RAWINPUTDEVICE dev;
  FXuint num_devices;
  FXchar devname[256];
  FXuint i;
  FXuint bc;

  // Get the number of attached raw input devices
  GetRawInputDeviceList(nullptr,&num_devices,sizeof(RAWINPUTDEVICELIST));
  if(0<num_devices){

    // Allocate the list
    if(callocElms(devlist,num_devices)){

      // Get the attached input device list
      GetRawInputDeviceList(devlist,&num_devices,sizeof(RAWINPUTDEVICELIST));   // man page is wrong about arg2

      // Get more info about each device
      for(i=0; i<num_devices; i++){
        bc=sizeof(devname);
        GetRawInputDeviceInfo(devlist[i].hDevice,RIDI_DEVICENAME,devname,&bc);
        FXTRACE(TOPIC_CONFIGURE,"device[%d] handle=%p type=%d name=%s\n",i,devlist[i].hDevice,devlist[i].dwType,devname);
        bc=devinfo.cbSize=sizeof(devinfo);
        GetRawInputDeviceInfo(devlist[i].hDevice,RIDI_DEVICEINFO,&devinfo,&bc);
        switch(devinfo.dwType){
        case RIM_TYPEMOUSE:
#if 0
          FXTRACE(TOPIC_CONFIGURE,"dwId:\t%d\n", devinfo.mouse.dwId);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfButtons:\t%d\n", devinfo.mouse.dwNumberOfButtons);
          FXTRACE(TOPIC_CONFIGURE,"dwSampleRate:\t%d\n", devinfo.mouse.dwSampleRate);
#endif
          break;
        case RIM_TYPEKEYBOARD:
#if 0
          FXTRACE(TOPIC_CONFIGURE,"dwType:\t%d\n", devinfo.keyboard.dwType);
          FXTRACE(TOPIC_CONFIGURE,"dwSubType:\t%d\n", devinfo.keyboard.dwSubType);
          FXTRACE(TOPIC_CONFIGURE,"dwKeyboardMode:\t%d\n", devinfo.keyboard.dwKeyboardMode);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfFunctionKeys:\t%d\n", devinfo.keyboard.dwNumberOfFunctionKeys);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfIndicators:\t%d\n", devinfo.keyboard.dwNumberOfIndicators);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfKeysTotal:\t%d\n", devinfo.keyboard.dwNumberOfKeysTotal);
#endif
          break;
        case RIM_TYPEHID:
          //   FXTRACE(TOPIC_CONFIGURE,"dwVendorId:\t0x%x (%s)\n", devinfo.hid.dwVendorId, fmt_vendor(devinfo.hid.dwVendorId));
          //   FXTRACE(TOPIC_CONFIGURE,"dwProductId:\t0x%x (%s)\n", devinfo.hid.dwProductId, fmt_product(devinfo.hid.dwProductId));
          //   FXTRACE(TOPIC_CONFIGURE,"dwVersionNumber:\t%d\n", devinfo.hid.dwVersionNumber);
          //   FXTRACE(TOPIC_CONFIGURE,"usUsagePage:\t%d\n", devinfo.hid.usUsagePage);
          //   FXTRACE(TOPIC_CONFIGURE,"usUsage:\t%d\n", devinfo.hid.usUsage);
          //
          // to receive input from the SpaceNavigator
          //
          //  See:
          //      www.3dconnexion.com/forum/viewtopic.php?t=1031#5666
          //      www.usb.org/developers/hidpage/
          //

          // Match 3Dconnexion SpaceNavigator / Generic Desktop Controls / Multi-axis Controller
          if((devinfo.hid.dwVendorId==0x046d) && (devinfo.hid.dwProductId==0xc626) && (devinfo.hid.usUsagePage==1) && (devinfo.hid.usUsage==8)){
            dev.usUsagePage=1;
            dev.usUsage=8;
            dev.dwFlags=0;
            dev.hwndTarget=nullptr;               // follow keyboard focus
            if(RegisterRawInputDevices(&dev,1,sizeof(dev))){
              spaceNavHandle=devlist[i].hDevice;  // assume only one device is connected
              // FXTRACE(TOPIC_CONFIGURE,"\nRegistered for WM_INPUT!\n");
              }
            }
          break;
          }
        }
      }
    freeElms(devlist);
    }
#endif
  }


// Close Input devices
void FXApp::closeInputDevices(){
  }

#if 0
//
//	onMotion
//
long SpaceNav::onMotion(FXObject *, FXSelector, void *vp){
#if 1
  //
  // this is the "unbuffered read" method
  //
#pragma pack(1)
  struct SpaceNavStruct {
    RAWINPUTHEADER hdr;
    unsigned long bc;       // sizeof(u + id)
    unsigned long count;    // number of inputs
    unsigned char id;       // 1=translation, 2=rotation, 3=buttons
    union DataUnion {
      struct AxesStruct {
        signed short x,y,z;
        } axes;
      unsigned char buttons;
      } u;
    } snav;
  FXuint bc=sizeof(snav);
  FXint status;
#pragma pack()
  status = GetRawInputData(
          (HRAWINPUT)vp,              // hRawInput
          RID_INPUT,                  // uiCommand
          &snav,                      // pData
          &bc,                        // pcbSize
          sizeof(RAWINPUTHEADER));    // cbSizeHeader
  if(status==bc){
    switch (snav.id){
    case 1:
      dofValue[0] = snav.u.axes.x;
      dofValue[1] = snav.u.axes.y;
      dofValue[2] = snav.u.axes.z;
//    FXTRACE(TOPIC_CONFIGURE,"X{%6d %6d %6d}\n", snav.u.axes.x, snav.u.axes.y, snav.u.axes.z);
      break;
    case 2:
      dofValue[3] = snav.u.axes.x;
      dofValue[4] = snav.u.axes.y;
      dofValue[5] = snav.u.axes.z;
//    FXTRACE(TOPIC_CONFIGURE,"R{%6d %6d %6d}\n", snav.u.axes.x, snav.u.axes.y, snav.u.axes.z);
      break;
    case 3:
      FXTRACE(TOPIC_CONFIGURE,"B(%d)\n", snav.u.buttons);
      break;
    default:
      FXTRACE(TOPIC_CONFIGURE,"id=%d (?)\n", snav.id);
      break;
      }
    }
  else{
    FXTRACE(TOPIC_CONFIGURE,"Error %d != %d\n",bc,status);
    }
#else
  //
  // and the "buffered read" method..
  //
  //  I cannot get this to work.
  //  I posted a question on the MSDN forum to see if I'm doing something
  //  wrong.  I'll post more info as I get it.  In the meantime, the
  //  unbuffered read seems to work ok.
  //
  FXint status;
  FXuint num_inputs;
  FXuint sizeof_buffer;
  // can a function's semantics get any more ridiculous???
  sizeof_buffer=sizeof(RAWINPUT);
#if 1
  num_inputs=GetRawInputBuffer(nullptr, &sizeof_buffer, sizeof(RAWINPUTHEADER));
  num_inputs=sizeof_buffer/sizeof(RAWINPUT);
  if(bufAlloc<num_inputs){
    bufAlloc=num_inputs;
    resizeElms(inpBuffer,bufAlloc);
    }
#endif
  num_inputs=GetRawInputBuffer(inpBuffer,&sizeof_buffer,sizeof(RAWINPUTHEADER));
  FXTRACE(TOPIC_CONFIGURE,"onMotion(%d)\n",num_inputs);
  status=DefRawInputProc(&inpBuffer, 1/*num_inputs*/,sizeof(RAWINPUTHEADER));
#endif
  onPaint(nullptr,0,nullptr);
  return 1;
  }
#endif




// Open the display
FXbool FXApp::openDisplay(const FXchar* dpy){
  if(!initialized){


    // What's going on
    FXTRACE(TOPIC_DETAIL,"%s::openDisplay(%s)\n",getClassName(),dpy);

    // Set to HINSTANCE on Windows
    display=GetOwnModuleHandle();

    // Process DPI awareness was already established by init(), before the
    // system font metrics were read.

    // TARGETS
    ddeTargets=GlobalAddAtomA("TARGETS");

    // XDND protocol awareness
    xdndAware=GlobalAddAtomA("XdndAware");

    // DDE property
    ddeDelete=RegisterClipboardFormatA("DELETE");

    // Standard stipples
    stipples[STIPPLE_0]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_0]);
    stipples[STIPPLE_1]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_1]);
    stipples[STIPPLE_2]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_2]);
    stipples[STIPPLE_3]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_3]);
    stipples[STIPPLE_4]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_4]);
    stipples[STIPPLE_5]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_5]);
    stipples[STIPPLE_6]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_6]);
    stipples[STIPPLE_7]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_7]);
    stipples[STIPPLE_8]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_8]);
    stipples[STIPPLE_9]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_9]);
    stipples[STIPPLE_10]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_10]);
    stipples[STIPPLE_11]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_11]);
    stipples[STIPPLE_12]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_12]);
    stipples[STIPPLE_13]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_13]);
    stipples[STIPPLE_14]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_14]);
    stipples[STIPPLE_15]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_15]);
    stipples[STIPPLE_16]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_16]);

    // Register our child window classes
    WNDCLASSEX wndclass;

    // Child window
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW;     // Set to 0 for bit_gravity
    //wndclass.style=0;
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXWindow");
    RegisterClassEx(&wndclass);

    // Top window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW;
    //wndclass.style=0;
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=LoadIcon((HINSTANCE)display,IDI_APPLICATION);
    if(!wndclass.hIcon) wndclass.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
    wndclass.hIconSm=(HICON)LoadImage((HINSTANCE)display,IDI_APPLICATION,IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    if(!wndclass.hIconSm) wndclass.hIconSm=wndclass.hIcon;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXTopWindow");
    RegisterClassEx(&wndclass);

    // OpenGL window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;   // Redraw all when resized, OWNER DC for speed
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXGLCanvas");
    RegisterClassEx(&wndclass);

    // Dummy GL window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;   // Redraw all when resized, OWNER DC for speed
    wndclass.lpfnWndProc=DefWindowProc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=0;
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("GLTEMP");
    RegisterClassEx(&wndclass);

    // Popup window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS;   // Do save-under's
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXPopup");
    RegisterClassEx(&wndclass);

    // This should prevent the Abort/Retry/Ignore message
    // when switching to drive w/no media mounted in it...
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Preregister some drag types
    FXWindow::octetType=RegisterClipboardFormatA(FXWindow::octetTypeName);
    FXWindow::deleteType=RegisterClipboardFormatA(FXWindow::deleteTypeName);
    FXWindow::textType=RegisterClipboardFormatA(FXWindow::textTypeName);
    FXWindow::colorType=RegisterClipboardFormatA(FXWindow::colorTypeName);
    FXWindow::urilistType=RegisterClipboardFormatA(FXWindow::urilistTypeName);
    FXWindow::utf8Type=RegisterClipboardFormatA(FXWindow::utf8TypeName);
    FXWindow::utf16Type=CF_UNICODETEXT;
    FXWindow::actionType=RegisterClipboardFormatA(FXWindow::actionTypeName);


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


}

#endif
