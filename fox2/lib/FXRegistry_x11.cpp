/********************************************************************************
*                                                                               *
*                             Registry   ( X 1 1 )                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSystem.h"
#include "FXProcess.h"
#include "FXPath.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "FXStringDictionary.h"
#include "FXRegistry.h"

#if !defined(WIN32)

/*
  Notes:

  - X11 registry: .rc files under $FOXDIR, $XDG_CONFIG_DIRS and $HOME/.config.
*/

#define TOPIC_CONSTRUCT 1000

// Default locations and names
#define FOXRC           "fox.rc"
#define SYSTEMDIRS      "$FOXDIR:$XDG_CONFIG_DIRS:/etc/xdg"
#define USERDIR         "$HOME/.config"
#define FILEEXT         ".rc"

using namespace FX;

/*******************************************************************************/

namespace FX {

// File extension for settings files
const FXchar FXRegistry::ext[]=FILEEXT;


// File name of common settings file
const FXchar FXRegistry::foxrc[]=FOXRC;


// Make registry object
FXRegistry::FXRegistry(const FXString& akey,const FXString& vkey):applicationkey(akey),vendorkey(vkey),systemdirs(SYSTEMDIRS),userdir(USERDIR){
  FXTRACE(TOPIC_CONSTRUCT,"FXRegistry::FXRegistry\n");
  }

}

#endif
