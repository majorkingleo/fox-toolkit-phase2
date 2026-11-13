/********************************************************************************
*                                                                               *
*  D e v i c e   C o n t e x t   F o r   W i n d o w s   a n d   I m a g e s    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXFont.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXDrawable.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXRootWindow.h"
#include "FXShell.h"
#include "FXRegion.h"
#include "FXDCWindow.h"


/*
  Notes:

  - Associate a DC with a surface before you begin using it:

     long SomeWidget::onPaint(FXObject*,FXSelector,void* ptr){
       FXDCWindow dc(this,ptr);
       dc.drawLine(...);
       ... jadajadajada ...
       return 1;
       }

    The association is automatically broken when you go out of scope; the
    destructor of the FXDCWindow does this for you.

  - Optimizations: only perform style/attribute changes just before an actual
    drawing command takes place:- X-Windows apparently already does this;
    MS-Windows also?

  - We assume the following initial state:

    BLIT Function:        BLT_SRC
    Foreground:           black (0)
    Background:           white (1)
    Line Width:           0 (meaning thinnest/fastest, no guaranteed pixelation)
    Cap Style:            CAP_BUTT
    Join Style:           JOIN_MITER
    Line Style:           LINE_SOLID
    Fill Style:           FILL_SOLID
    Fill Rule:            RULE_EVEN_ODD
    Font:                 None
    Other Paremeters:     To Be Determined

  - Under X-Windows, end() will restore the GC to the state above; flags
    keeps track of which changes have been made to minimize the necessary
    updating.

  - Under X, graphics_exposures should be OFF:- at least some SGI IRIX machines
    have broken implementations of graphics_exposures.

  - Try suggestions from "Kevin Radke" <kmradke@isualum.com> below:

    Sorry about the huge delay with this code.  Work has been horribly busy
    and I wasn't able to dig up the CD with the original code.  However, if I
    remember
    correctly (and this code snippet I found was from that test), I changed from
    using
    cosmetic pens to use geometric pens and specifying a line join style of
    PS_JOIN_BEVEL.

    I.E.

        LOGBRUSH logBrush;
        logBrush.lbStyle = BS_SOLID;
        logBrush.lbColor = RGB(red, green, blue);
        logBrush.lbHatch = HS_CROSS;  // Not used

        // NYI Only solid lines are valid on Windows 95.  style is ignored
        return ExtCreatePen (PS_GEOMETRIC | PS_JOIN_BEVEL | style,
                                        width, &logBrush, 0, NULL);


    this returns an HPEN.

    I remember this being significantly slower than cosmetic pens (under NT4)
    and at the time that I scrapped the code, and dealt with the drawing
    differences
    between X and Win32 at a higher level.

    This won't work on Win95/Win98 (without using paths), and after a closer
    look at
    the docs here it makes more sense to specify PS_ENDCAP_SQUARE to draw
    the last pixel instead of the line join style.  I remember experimenting
    with both, so
    the code I found may have been in intermediate (unworking) version.

    In any case, it isn't too hard to experiment to see which has the required
    behavior.  I've unfortunately been away from FOX work for a few months
    or I'd try it myself.

  - Device caps for DirectX:
    http://www.molybdenium.de/devicecaps/e_index.html
*/

using namespace FX;

namespace FX {

}
