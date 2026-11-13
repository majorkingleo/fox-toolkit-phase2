/********************************************************************************
*                                                                               *
*                        P i p e   C l a s s   ( X 1 1 )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXPipe.h"
#include "fxiopriv.h"

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXPipe members which create pipes; the constructors stay in FXPipe.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Open pipes with access mode m for this one and the reverse for the other
FXbool FXPipe::open(FXPipe& other,FXuint m){
  if(device==BadHandle && other.device==BadHandle){
    FXint hnd[2];
#if defined(HAVE_PIPE2)
    FXint flags=0;

    // Non-blocking
    if(m&FXIO::NonBlocking){ flags|=O_NONBLOCK; }

    // Inheritable
#if defined(O_CLOEXEC)
    if(!(m&FXIO::Inheritable)){ flags|=O_CLOEXEC; }
#endif

    // Create pipe
    if(pipe2(hnd,flags)==0){

      // Who's is the read-side?
      if(m&FXIO::ReadOnly){
        device=hnd[0];
        other.device=hnd[1];
        }
      else{
        device=hnd[1];
        other.device=hnd[0];
        }
      return true;
      }

#else

    // Create pipe
    if(pipe(hnd)==0){

      // Who's is the read-side?
      if(m&FXIO::ReadOnly){
        device=hnd[0];
        other.device=hnd[1];
        }
      else{
        device=hnd[1];
        other.device=hnd[0];
        }

      // Non-blocking
      if(m&FXIO::NonBlocking){
        fcntl(device,F_SETFL,O_NONBLOCK);
        }

      // Inheritable
#if defined(O_CLOEXEC)
      if(!(m&FXIO::Inheritable)){
        fcntl(device,F_SETFD,FD_CLOEXEC);
        }
#endif
      return true;
      }
#endif
    }
  return false;
  }


// Create a named pipe (not available on Windows)
FXbool FXPipe::create(const FXString& file,FXuint perm){
  if(!file.empty()){
    return ::mkfifo(file.text(),perm&0777)==0;
    }
  return false;
  }


// Remove a named pipe (not available on Windows)
FXbool FXPipe::remove(const FXString& file){
  if(!file.empty()){
    return ::unlink(file.text())==0;
    }
  return false;
  }

}

#endif
