/********************************************************************************
*                                                                               *
*                        I / O   D e v i c e   C l a s s                        *
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
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "fxiopriv.h"


/*
  Notes:

  - An abstract class for low-level IO.
  - You can change some access mode flags of the file descriptor.
*/

#define TOPIC_DETAIL    1002


using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct
FXIODevice::FXIODevice():device(BadHandle){
  }


// Construct with given handle and mode
FXIODevice::FXIODevice(FXInputHandle h):device(BadHandle){
  attach(h);
  }


// Return true if open
FXbool FXIODevice::isOpen() const {
  return device!=BadHandle;
  }


// Attach existing file handle
FXbool FXIODevice::attach(FXInputHandle h){
  if(valid(h)){
    close();
    device=h;
    return true;
    }
  return false;
  }


// Detach existing file handle
FXbool FXIODevice::detach(){
  device=BadHandle;
  return true;
  }


// Truncate file
FXlong FXIODevice::truncate(FXlong){
  return FXIO::Error;
  }


// Synchronize disk with cached data
FXbool FXIODevice::flush(){
  return true;
  }


// Test if we're at the end; -1 if error
FXint FXIODevice::eof(){
  return FXIO::Error;
  }


// Return file size
FXlong FXIODevice::size(){
  return FXIO::Error;
  }


// Destroy
FXIODevice::~FXIODevice(){
  close();
  }


}

