/********************************************************************************
*                                                                               *
*                       FOX Target Platform Selection                           *
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
#ifndef FXTARGET_H
#define FXTARGET_H

/*
  Notes:

  - Exactly one of WIN32, FOX_X11 or FOX_WAYLAND selects the windowing platform.
  - The build system defines it (CMake: FOX_PLATFORM, Autotools: --with-wayland).
  - If none is defined, the host default is taken: FOX_X11 on Unix.

  - Not named X11: that would clash with the FXFont::X11 enumerator.
*/

#if defined(WIN32)
#if defined(FOX_X11) || defined(FOX_WAYLAND)
#error "More than one FOX platform is defined (WIN32, FOX_X11, FOX_WAYLAND)"
#endif
#elif defined(FOX_X11)
#if defined(FOX_WAYLAND)
#error "More than one FOX platform is defined (WIN32, FOX_X11, FOX_WAYLAND)"
#endif
#elif !defined(FOX_WAYLAND)
#define FOX_X11
#endif

#endif
