/********************************************************************************
*                                                                               *
*                    M a i n   W i n d o w   ( W a y l a n d )                  *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRootWindow.h"
#include "FXMainWindow.h"

#if defined(FOX_WAYLAND)

/*
  Notes:

  - A main window differs from a top window only in what the X11 module has to
    do for it: the command line it was started with is a window manager
    property there.  There is no such property on this platform, and the
    application id of the toplevel already groups the windows of one program,
    so creating the top window is all that is left.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Create server-side resources
void FXMainWindow::create(){
  FXTopWindow::create();
  }

}

#endif
