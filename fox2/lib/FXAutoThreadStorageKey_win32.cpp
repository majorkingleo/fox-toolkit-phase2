/********************************************************************************
*                                                                               *
*      T h r e a d - L o c a l   S t o r a g e   C l a s s   ( W i n 3 2 )      *
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
#include "fxmath.h"
#include "FXException.h"
#include "FXAutoThreadStorageKey.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of FXAutoThreadStorageKey; the whole class is platform specific.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Automatically acquire a thread-local storage key
FXAutoThreadStorageKey::FXAutoThreadStorageKey(){
  FXSTATIC_ASSERT(sizeof(FXThreadStorageKey)>=sizeof(DWORD));
  if((value=(FXThreadStorageKey)TlsAlloc())==TLS_OUT_OF_INDEXES){ throw FXMemoryException("FXAutoThreadStorageKey::FXAutoThreadStorageKey: out of memory\n"); }
  }


// Set thread local storage associated with this key
void FXAutoThreadStorageKey::set(FXptr ptr) const {
  TlsSetValue((DWORD)value,ptr);
  }


// Get thread local storage associated with this key
FXptr FXAutoThreadStorageKey::get() const {
  return TlsGetValue((DWORD)value);
  }


// Automatically release a thread-local storage key
FXAutoThreadStorageKey::~FXAutoThreadStorageKey(){
  TlsFree((DWORD)value);
  }

}

#endif
