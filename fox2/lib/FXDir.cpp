/********************************************************************************
*                                                                               *
*                    D i r e c t o r y   E n u m e r a t o r                    *
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
#include "fxchar.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXPath.h"
#include "FXDir.h"

/*
  Notes:
  - This class implements a way to list the files in a directory.
  - We just want to wrap directory iteration, nothing fancy.
  - Maybe add positioning for seek and tell type functions.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {


// Create a directories recursively
FXbool FXDir::createDirectories(const FXString& path,FXuint perm){
  if(FXPath::isAbsolute(path)){
    if(FXStat::isDirectory(path)) return true;
    if(createDirectories(FXPath::upLevel(path),perm)){
      if(FXDir::create(path,perm)) return true;
      }
    }
  return false;
  }


// Cleanup
FXDir::~FXDir(){
  close();
  }


}


