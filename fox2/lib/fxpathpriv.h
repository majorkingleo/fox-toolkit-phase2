/********************************************************************************
*                                                                               *
*              P a t h   P r i v a t e   D e f i n i t i o n s                  *
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
#ifndef FXPATHPRIV_H
#define FXPATHPRIV_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Helper shared by FXPath.cpp and the platform modules.  Include after
// FXString.h.

namespace FX {

// Expand environment variables recursively, stopping when recursion goes too deep
extern FXString expandEnvironmentVariables(const FXString& file,FXint level);

}

#endif
