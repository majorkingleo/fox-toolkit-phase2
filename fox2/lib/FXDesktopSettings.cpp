/********************************************************************************
*                                                                              *
*              D e s k t o p   S e t t i n g s   (a d a p t e r)               *
*                                                                              *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                              *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                              *
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


/*
  Notes:
  - Detection must never be able to prevent an application from starting, so
    every step is optional and every failure falls through to the next one.
  - No desktop library is linked: the GTK and KDE answers come from their
    configuration files, and gsettings is only consulted when it exists.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// The setting that decided the answer, for diagnostics
static FXString schemename;


// Does the text mention darkness?
static FXbool mentionsDark(const FXString& text){
  return 0<=text.find("dark") || 0<=text.find("Dark") || 0<=text.find("DARK");
  }


// Does the text mention lightness?
static FXbool mentionsLight(const FXString& text){
  return 0<=text.find("light") || 0<=text.find("Light") || 0<=text.find("LIGHT");
  }


// Check the GTK 3/4 settings files for gtk-application-prefer-dark-theme
static FXint gtkColorScheme(const FXString& config){
  static const FXchar* places[2]={ "/gtk-4.0/settings.ini","/gtk-3.0/settings.ini" };
  FXint i;
  for(i=0; i<2; i++){
    FXSettings settings;
    FXString value;
    if(!settings.parseFile(config+places[i])) continue;

    // Read as text: FXString::toInt() cannot parse a bare "0" in this toolkit
    value=settings.readStringEntry("Settings","gtk-application-prefer-dark-theme",nullptr);
    if(value.empty()) continue;
    schemename=places[i];
    if(value=="1" || FXString::comparecase(value,"true")==0) return SCHEME_DARK;
    if(value=="0" || FXString::comparecase(value,"false")==0) return SCHEME_LIGHT;
    }
  return SCHEME_UNKNOWN;
  }


// Check the KDE color scheme name
static FXint kdeColorScheme(const FXString& config){
  FXSettings settings;
  FXString value;
  if(!settings.parseFile(config+"/kdeglobals")) return SCHEME_UNKNOWN;
  value=settings.readStringEntry("General","ColorScheme",nullptr);
  if(value.empty()) return SCHEME_UNKNOWN;
  schemename="kdeglobals:ColorScheme="+value;
  if(mentionsDark(value)) return SCHEME_DARK;
  if(mentionsLight(value)) return SCHEME_LIGHT;
  return SCHEME_UNKNOWN;
  }


// Return the color scheme the desktop prefers
FXint FXDesktopSettings::getColorScheme(){
  FXString value;
  const FXchar* name;
  FXint scheme;

  // An explicit GTK theme name is the strongest hint
  value=FXSystem::getEnvironment("GTK_THEME");
  if(!value.empty()){
    if(mentionsDark(value)){ schemename="GTK_THEME="+value; return SCHEME_DARK; }
    if(mentionsLight(value)){ schemename="GTK_THEME="+value; return SCHEME_LIGHT; }
    }

  // GTK applications record the preference in their settings file
  value=FXSystem::getEnvironment("XDG_CONFIG_HOME");
  if(value.empty()) value=FXSystem::getHomeDirectory()+"/.config";
  scheme=gtkColorScheme(value);
  if(scheme!=SCHEME_UNKNOWN) return scheme;

  // KDE names its color scheme in kdeglobals
  scheme=kdeColorScheme(value);
  if(scheme!=SCHEME_UNKNOWN) return scheme;

  // Finally ask the GNOME settings store, when it is available
  scheme=fxdesktopcolorscheme(name);
  if(scheme!=SCHEME_UNKNOWN){ schemename=name; return scheme; }

  schemename="none";
  return SCHEME_UNKNOWN;
  }


// Return the name of the setting that decided the answer
FXString FXDesktopSettings::getColorSchemeName(){
  return schemename;
  }

}
