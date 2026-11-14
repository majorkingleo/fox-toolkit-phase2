/********************************************************************************
*                                                                               *
*              D e s k t o p   S e t t i n g s   T e s t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#include "fx.h"
#include <cstdio>


/*

  Prints the light/dark preference the desktop reports, and which setting
  decided it.  It needs no display: the answer comes from the environment and
  from the configuration files of the desktop, which is exactly what a gate can
  control.

*/

int main(int,char**){
  FXint scheme=FXDesktopSettings::getColorScheme();
  const FXchar* value="unknown";
  if(scheme==SCHEME_DARK) value="dark";
  if(scheme==SCHEME_LIGHT) value="light";
  printf("scheme: %s\n",value);
  printf("setting: %s\n",FXDesktopSettings::getColorSchemeName().text());
  return 0;
  }
