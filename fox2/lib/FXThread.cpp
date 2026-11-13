/********************************************************************************
*                                                                               *
*                          T h r e a d   S u p p o r t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
#include "fxmath.h"
#include "FXException.h"
#include "FXString.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"

/*
  Notes:

  - Note that the FXThreadID is only valid when busy==true, except insofar
    as when its used to harvest thread exit status like e.g. join!

  - The busy flag is set BEFORE actually spawning the thread, and reset
    if we were unable to spawn the thread.
    This is because we don't know how the thread creation is implemented:-
    its possible that the new thread may already be running for some time
    before pthread_create() returns successfully.
    We want to be sure that the flag reflects running state from within
    the newly spawned thread [we need this in several API's].

  - Note that cancel() also resets busy since it kills the thread
    right away; however join() doesn't because then we wait for the
    thread to finish normally.

  - About thread suspend/resume.  This does not work on Linux since there is
    no pthread equivalent for SuspendThread() and ResumeThread().  There is,
    however, an exceptionally inelegant solution in Boehm's GC code (file
    linux_threads.c).  But its so ugly we'd rather live without until a real
    suspend/resume facility is implemented in the linux kernel.

*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Generate one for the thread itself
FXAutoThreadStorageKey FXThread::selfKey;


// Initialize thread
FXThread::FXThread():tid(0),busy(false){
  }


// Return thread id of this thread object.
// Purposefully NOT inlined, the tid may be changed by another
// thread and therefore we must force the compiler to fetch
// this value fresh each time it is needed!
FXThreadID FXThread::id() const {
  return tid;
  }


// Return true if this thread is running
FXbool FXThread::running() const {
  return busy;
  }


// Change pointer to thread
void FXThread::self(FXThread* t){
  FXThread::selfKey.set(t);
  }


// Return pointer to calling thread
FXThread* FXThread::self(){
  return (FXThread*)FXThread::selfKey.get();
  }


// Destroy
FXThread::~FXThread(){
  if(self()==this){
    self(nullptr);
    detach();
    }
  else{
    cancel();
    }
  }

}
