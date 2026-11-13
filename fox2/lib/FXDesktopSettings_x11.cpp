/********************************************************************************
*                                                                               *
*                         Desktop settings   ( X 1 1 )                          *
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
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXSystem.h"
#include "FXDesktopSettings.h"
#include "fxpriv.h"

#if !defined(WIN32)

/*
  Notes:

  - X11 desktop settings: the GNOME settings store, queried through gsettings.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Ask gsettings; the command is only run when it is installed
FXint fxdesktopcolorscheme(const FXchar*& name){
  FXString path=FXSystem::getEnvironment("PATH");
  FXString result;
  FXchar buffer[256];
  FILE* pipe;
  name=nullptr;
  if(path.empty()) return SCHEME_UNKNOWN;
  pipe=popen("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null","r");
  if(!pipe) return SCHEME_UNKNOWN;
  while(std::fgets(buffer,sizeof(buffer),pipe)){
    result.append(buffer);
    }
  pclose(pipe);
  if(result.contains("prefer-dark")){
    name="gsettings:color-scheme=prefer-dark";
    return SCHEME_DARK;
    }
  if(result.contains("prefer-light")){
    name="gsettings:color-scheme=prefer-light";
    return SCHEME_LIGHT;
    }
  return SCHEME_UNKNOWN;
  }

}

#endif
