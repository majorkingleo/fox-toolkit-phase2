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


}

#endif
