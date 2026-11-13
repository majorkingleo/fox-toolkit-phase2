/********************************************************************************
*                                                                               *
*                               Shell   ( X 1 1 )                               *
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
#include "fxkeys.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXShell.h"

#if defined(FOX_X11) || defined(FOX_WAYLAND)

/*
  Notes:

  - Unix shell: configure schedules a delayed layout.
*/

#define TOPIC_CONFIGURE 1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Handle configure notify
long FXShell::onConfigure(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXComposite::onConfigure(sender,sel,ptr);
  FXTRACE(TOPIC_CONFIGURE,"%s::onConfigure x=%d y=%d w=%d h=%d\n",getClassName(),ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  xpos=ev->rect.x;
  ypos=ev->rect.y;
  if((ev->rect.w!=width) || (ev->rect.h!=height)){
    width=ev->rect.w;               // Record new size
    height=ev->rect.h;
    // Delayed layout optimization. The delayed layout optimization
    // currently only works on UNIX.  On Windows, the program enters
    // a modal loop during a window-resize operation.  During this
    // modal loop, which is somewhere inside WIN32 code, we are completely
    // deaf to other event sources such as timers, chores, file i/o, and
    // are unable to perform idle processing.  So the chore we would set
    // in recalc() would never fire until we're all done with the resizing.
    // We'd love to have a fix for this, but it seems difficult because of
    // the need to pass "non-client" events over to the DefWindowProc...
    recalc();           // On UNIX, we process idle messages during a resize
    }
  return 1;
  }

}

#endif
