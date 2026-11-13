/********************************************************************************
*                                                                               *
*                              M u t e x   C l a s s                            *
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

/*
  Notes:

  - Simple (optionally recursive) mutex variable.

  - An amorphous blob of memory is reserved in the declaration, without revealing
    implementation details.  This is better than allocating memory dynamically, both
    in terms of speed as well as the possibility that hitting malloc() will involve
    another mutex.

  - Hopefully it is big enough for all supported platforms.  But just in case, there's
    an assert in the constructor to verify that enough space is reserved for the
    implementation.

  - I do therefore recommend running this in debug mode first time around on a
    new platform.

  - If you run into this, try to figure out sizeof(pthread_mutex_t) and
    let me know about it (jeroen@fox-toolkit.net).
*/

#define TOPIC_CONSTRUCT 1000

using namespace FX;

/*******************************************************************************/

namespace FX {

// Test if locked
FXbool FXMutex::locked(){
  if(trylock()){
    unlock();
    return false;
    }
  return true;
  }

}
