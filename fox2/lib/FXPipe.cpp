/********************************************************************************
*                                                                               *
*                             P i p e   C l a s s                               *
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



/*
  Notes:

  - Create a pipe for reading or writing; if created for reading,
    the "other" is the write-side.  If created for writing, the
    "other" is the read-side.

  - The pipe may be inherited by a sub-process if so indicated.

  - On Linux, we prefer to use pipe2(), elsewhere we use pipe()
    and set the flags properly afterwards, using fcntl().

  - Both our end the other end FXPipe instances should not be open
    yet prior to a call to open().

  - FIXME Probably need special class to implement unix socket
    or named pipe, this will be just for anonymous pipes.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct and open pipes with access mode m for this one and the reverse for the other
FXPipe::FXPipe(FXPipe& other,FXuint m){
  open(other,m);
  }


// Construct file and attach existing handle h
FXPipe::FXPipe(FXInputHandle h){
  attach(h);
  }


/*

// FIXME move elsewhere
HANDLE WINAPI CreateNamedPipe(
  __in      LPCTSTR lpName,
  __in      DWORD dwOpenMode,
  __in      DWORD dwPipeMode,
  __in      DWORD nMaxInstances,
  __in      DWORD nOutBufferSize,
  __in      DWORD nInBufferSize,
  __in      DWORD nDefaultTimeOut,
  __in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes
);
*/

}

