/********************************************************************************
*                                                                               *
*                      M a p p e d   F i l e   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2023,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "FXString.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXMappedFile.h"
#include "fxiopriv.h"

/*
  Notes:
  - Memory-mapped file implementation; platform specific members are in
    FXMappedFile_win32.cpp and FXMappedFile_posix.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create new map object
FXMappedFile::FXMappedFile():memhandle(BadHandle),mempointer(nullptr),memlength(0),memoffset(0){
  }


// Return true if serial access only
FXbool FXMappedFile::isSerial() const {
  return false;
  }


// Delete the mapping
FXMappedFile::~FXMappedFile(){
  close();
  }

}
