/********************************************************************************
*                                                                               *
*                     M u t e x   C l a s s   ( W i n 3 2 )                     *
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
#include "FXMutex.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXMutex members; the portable locked() stays in FXMutex.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Initialize mutex
FXMutex::FXMutex(FXbool recursive){
  // If this fails on your machine, determine what value
  // of sizeof(CRITICAL_SECTION) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE(TOPIC_CONSTRUCT,"sizeof(CRITICAL_SECTION)=%d\n",sizeof(CRITICAL_SECTION));
  FXSTATIC_ASSERT(sizeof(data)>=sizeof(CRITICAL_SECTION));
  InitializeCriticalSection((CRITICAL_SECTION*)data);
  }


// Lock the mutex
void FXMutex::lock(){
  EnterCriticalSection((CRITICAL_SECTION*)data);
  }


// Try lock the mutex
FXbool FXMutex::trylock(){
  return TryEnterCriticalSection((CRITICAL_SECTION*)data)!=0;
  }


// Unlock mutex
void FXMutex::unlock(){
  LeaveCriticalSection((CRITICAL_SECTION*)data);
  }


// Delete mutex
FXMutex::~FXMutex(){
  DeleteCriticalSection((CRITICAL_SECTION*)data);
  }

}

#endif
