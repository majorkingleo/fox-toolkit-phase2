/********************************************************************************
*                                                                               *
*                          S e m a p h o r e   C l a s s                        *
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
#include "FXSemaphore.h"

#if defined(WIN32)

/*
  Notes:

  - Semaphore variable.

  - Implementation using Condition and Mutex now used for MacOSX and Minix.
    This may be less performant than a true semaphore, but its a nice and fully functional
    fallback until full posix semaphore implementation is available.

  - Increased reserved for pthread_mutex_t as up to 11 FXuval's in size on MacOSX,
    worst case is on 32-bit versions.

  - Keep in mind we can not do sizeof() in the header file, as we're trying to avoid
    including system-headers from public accessible FOX header files.  This is so
    as to avoid "accidental" incorporation of declarations or unexpected macros,
    that may clash.

  - Also we want to avoid calls to memory allocators as that would incur a big
    performance penalty; besides, a memory allocator may need to call locking
    primitives since memory is a resource shared by all threads.

  - The upshot is that the space allocated for the semaphore may be a bit more
    than strictly necessary on some machines.  It may actually be a good thing
    as this increases the odds of these datastructures living in dedicated
    cache-lines.
*/

#define TOPIC_CONSTRUCT 1000

using namespace FX;

/*******************************************************************************/

namespace FX {


// Initialize semaphore with given count
FXSemaphore::FXSemaphore(FXint count){
  FXSTATIC_ASSERT(sizeof(data)>=sizeof(HANDLE));
  data[0]=(FXuval)CreateSemaphore(nullptr,count,0x7fffffff,nullptr);
  }


// Decrement semaphore, waiting if count is zero
FXbool FXSemaphore::wait(){
  return WaitForSingleObject((HANDLE)data[0],INFINITE)==WAIT_OBJECT_0;
  }


// Try decrement semaphore; return false if timed out
FXbool FXSemaphore::wait(FXTime nsec){
  if(0<nsec){
    if(nsec<forever){
      DWORD delay=(DWORD)(nsec/1000000);
      return WaitForSingleObject((HANDLE)data[0],delay)==WAIT_OBJECT_0;
      }
    return WaitForSingleObject((HANDLE)data[0],INFINITE)==WAIT_OBJECT_0;
    }
  return WaitForSingleObject((HANDLE)data[0],0)==WAIT_OBJECT_0;
  }


// Try decrement semaphore; and return false if count was zero
FXbool FXSemaphore::trywait(){
  return WaitForSingleObject((HANDLE)data[0],0)==WAIT_OBJECT_0;
  }


// Increment semaphore
FXbool FXSemaphore::post(){
  return ReleaseSemaphore((HANDLE)data[0],1,nullptr)!=0;
  }


// Delete semaphore
FXSemaphore::~FXSemaphore(){
  CloseHandle((HANDLE)data[0]);
  }

}

#endif
