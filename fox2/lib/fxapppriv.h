/********************************************************************************
*                                                                               *
*          F O X   A p p   P r i v a t e   E v e n t   R e c o r d s             *
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
#ifndef FXAPPPRIV_H
#define FXAPPPRIV_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Records behind the opaque types declared in FXApp.h.  They are shared by the
// portable application core and the platform event modules, so they live in a
// private header instead of inside FXApp.cpp.


#include "fxdefs.h"
#include "FXRectangle.h"
#include "FXThread.h"
#include "FXApp.h"


namespace FX {

class FXObject;
class FXWindow;


// Callback Record
struct FXCBSpec {
  FXObject      *target;            // Receiver object
  FXptr          data;              // User data
  FXSelector     message;           // Message sent to receiver
  };


// Timer record
struct FXTimer {
  FXTimer       *next;              // Next timeout in list
  FXObject      *target;            // Receiver object
  FXptr          data;              // User data
  FXSelector     message;           // Message sent to receiver
  FXTime         due;               // When timer is due (ns)
  };


// Signal record
struct FXSignal {
  FXObject      *target;            // Receiver object
  FXSelector     message;           // Message sent to receiver
  FXbool         handlerset;        // Handler was already set
  FXbool         notified;          // Signal has fired
  };


// Idle record
struct FXChore {
  FXChore       *next;              // Next chore in list
  FXObject      *target;            // Receiver object
  FXptr          data;              // User data
  FXSelector     message;           // Message sent to receiver
  };


// Input record
struct FXInput {
  FXCBSpec       read;              // Callback spec for read
  FXCBSpec       write;             // Callback spec for write
  FXCBSpec       excpt;             // Callback spec for except
  };


// A repaint event record
struct FXRepaint {
  FXRepaint     *next;              // Next repaint in list
  FXID           window;            // Window ID of the dirty window
  FXRectangle    rect;              // Dirty rectangle
  FXint          area;              // Area of this rectangle
  FXbool         synth;             // Synthetic expose event or real one?
  };


// Handles to be watched
struct FXHandles {
#if defined(WIN32)
  HANDLE hnd[MAXIMUM_WAIT_OBJECTS];     // Handle table
#else
  fd_set hnd[3];                        // Handle table
#endif
  };


// Recursive Event Loop Invocation
struct FXInvocation {
  FXInvocation **invocation;  // Pointer to variable holding pointer to current invocation
  FXInvocation  *upper;       // Invocation above this one
  FXWindow      *window;      // Modal window (if any)
  FXModality     modality;    // Modality mode
  FXint          code;        // Return code
  FXbool         done;        // True if breaking out

  // Enter modal loop
  FXInvocation(FXInvocation** inv,FXModality mode,FXWindow* win):invocation(inv),upper(*inv),window(win),modality(mode),code(0),done(false){
    *invocation=this;
    }

  // Exit modal loop
 ~FXInvocation(){
    *invocation=upper;
    }
  };


}

#endif
