/********************************************************************************
*                                                                               *
*                       M u t e x   C l a s s   ( X 1 1 )                       *
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

#if !defined(WIN32)

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
  // of sizeof(pthread_mutex_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.net!!
  //FXTRACE(TOPIC_CONSTRUCT,"sizeof(pthread_mutex_t)=%d\n",sizeof(pthread_mutex_t));
  FXSTATIC_ASSERT(sizeof(data)>=sizeof(pthread_mutex_t));
  pthread_mutexattr_t mutexatt;
  pthread_mutexattr_init(&mutexatt);
  pthread_mutexattr_settype(&mutexatt,recursive?PTHREAD_MUTEX_RECURSIVE:PTHREAD_MUTEX_DEFAULT);
  pthread_mutex_init((pthread_mutex_t*)data,&mutexatt);
  pthread_mutexattr_destroy(&mutexatt);
  }


// Lock the mutex
void FXMutex::lock(){
  pthread_mutex_lock((pthread_mutex_t*)data);
  }


// Try lock the mutex
FXbool FXMutex::trylock(){
  return pthread_mutex_trylock((pthread_mutex_t*)data)==0;
  }


// Unlock mutex
void FXMutex::unlock(){
  pthread_mutex_unlock((pthread_mutex_t*)data);
  }


// Delete mutex
FXMutex::~FXMutex(){
  pthread_mutex_destroy((pthread_mutex_t*)data);
  }

}

#endif
