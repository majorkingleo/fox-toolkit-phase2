/********************************************************************************
*                                                                               *
*        D e s k t o p   S e t t i n g s   ( W a y l a n d )                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXString.h"
#include "FXSystem.h"
#include "FXDesktopSettings.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - The file based answers are in FXDesktopSettings_unix.cpp, which this file
    replaces on Wayland: the setting of the desktop is asked from the portal
    first, because that is the only source a sandboxed application can reach,
    and the answer of gsettings is the fallback where no portal is running.
  - The portal is a D-Bus service, and this library links no D-Bus: the one
    call that is needed is made with gdbus, which is part of GLib and present
    wherever a portal is.  The answer is asked on every call, like the unix
    module does, so a change of the desktop preference is picked up.
  - A portal that is not there answers nothing, and the answer of the portal is
    also allowed to be "no preference", which is not the same as an error: both
    fall through to gsettings.
  - Nothing here may keep an application from starting, so every failure is
    silent and ends with SCHEME_UNKNOWN.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Run a command and collect what it prints on stdout, with a limit
static FXString runCommand(const FXchar* command){
  FXchar buffer[256];
  FXString result;
  FILE* pipe;
  if(!(pipe=popen(command,"r"))) return result;
  while(std::fgets(buffer,sizeof(buffer),pipe)){
    result.append(buffer);
    if(result.length()>4096) break;
    }
  pclose(pipe);
  return result;
  }


// Ask the desktop portal for the color scheme; 1 is dark and 2 is light
static FXint portalColorScheme(const FXchar*& name){
  FXString result=runCommand("gdbus call --session"
                             " --dest org.freedesktop.portal.Desktop"
                             " --object-path /org/freedesktop/portal/desktop"
                             " --method org.freedesktop.portal.Settings.Read"
                             " org.freedesktop.appearance color-scheme 2>/dev/null");
  if(result.empty()) return SCHEME_UNKNOWN;

  // The answer is a variant, printed as (<uint32 1>,)
  if(result.contains("uint32 1")){
    name="portal:org.freedesktop.appearance color-scheme=1";
    return SCHEME_DARK;
    }
  if(result.contains("uint32 2")){
    name="portal:org.freedesktop.appearance color-scheme=2";
    return SCHEME_LIGHT;
    }
  return SCHEME_UNKNOWN;
  }


// The GNOME settings store, for a desktop that runs no portal
static FXint gsettingsColorScheme(const FXchar*& name){
  FXString result=runCommand("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null");
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


// Ask the desktop for its preference, through the portal and then through
// gsettings; what can be read from the configuration files is done by the
// caller before this runs
FXint fxdesktopcolorscheme(const FXchar*& name){
  FXint scheme;
  name=nullptr;
  if((scheme=portalColorScheme(name))!=SCHEME_UNKNOWN) return scheme;
  if((scheme=gsettingsColorScheme(name))!=SCHEME_UNKNOWN) return scheme;
  name=nullptr;
  return SCHEME_UNKNOWN;
  }

}

#endif
