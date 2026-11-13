/********************************************************************************
*                                                                               *
*                     A p p l i c a t i o n   O b j e c t                       *
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
#include "fxchar.h"
#include "fxkeys.h"
#include "fxascii.h"
#include "FXMutex.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXColors.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXId.h"
#include "FXVisual.h"
#include "FXCursor.h"
#include "FXFont.h"
#include "FXDrawable.h"
#include "FXBitmap.h"
#include "FXImage.h"
#include "FXIcon.h"
#include "FXGIFIcon.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXCSS.h"
#include "FXThemeCache.h"
#include "FXDesktopSettings.h"
#include "fxplatform.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXComposite.h"
#include "FXRootWindow.h"
#include "FXShell.h"
#include "FXTopWindow.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXDialogBox.h"
#include "FXMessageBox.h"
#include "FXTranslator.h"
#include "FXComposeContext.h"
#include "fxapppriv.h"
#include "fxpriv.h"

/*
  Notes:

  - Major Contributions for Windows NT by Lyle Johnson.

  - Application should have a target to be notified of certain app-wide events,
    such as activation of [a window of] the application etc.

  - Need way to specify visual on command line (X11).

  - Need to be able to run event loop w/o display connection (just I/O, and timers).

  - Need to be able to ``detach'' from GUI more cleanly.

  - FXApp::exit() should be called by AFTER returning from FXApp::run(); also,
    FXApp::exit() should not call the global exit():- just because we're done doing
    GUI things does not mean we're done with the program!

  - When timer, signal, I/O, chore callback fires, need to go once round the event
    loop, because these callbacks may set a flag to break out of the event loop.
    This is done by letting getNextEvent() return false if it returns with NO
    event [The alternative would be to dispatch these events via dispatchEvent()
    which is currently a bit difficult].

  - Event logging (journalling) and playback.  We need some basic capabilities for
    journalling and playback of user-inputs [basically, mouse and keyboard events].
    How exactly this is going to work is not entirely clear, but there are a couple
    of issues:

      - Mapping window ID's to something we can save on the file. FXWindow now
        generates a window-key which can identify each window by means of a kind of
        Dewey Decimal system.
      - Which events need logging? Of course, mouse buttons, motion, and keyboard, but
        how about enter/leave? The real mouse may be moving around too while playing
        back!
      - And what should be recorded.
      - Plus, some difficulty with event dispatch on Windows.

  - Pre- and post-dispatch hooks. In the pre-dispatch hook, the event is presented to
    some user-defined function (or maybe message handler) and can be inspected prior to
    dispatch.  Returning a true or false from the pre-dispatch hook function will cause
    the actual dispatch to be blocked or not.

    Thus, the predispatch hook can act as an event filter, and events which are passed are
    dispatched normally.

    The post-dispatch hook is presented with the event AFTER it was dispatched.
    Either we present the event to the post-dispatch hook only when it was
    actually handled somewhere in the GUI, or we always present it and pass
    a flag that says whether it has been handled by some widget or not. The chief purpose
    of a post-dispatch hook is for event logging purposes.

  - Make sure keyboard gets dispatched to modal window [dialog or popup or whatever].

  - FXInvocation into a class to its dtor will be called [just in case someone tries to
    throw an exception].   Having a dtor allows clean up without using a try-catch
    construct.

  - Modal modes for FXInvocation:

      - Non-modal for unconstrained model loops, like e.g. toplevel loop.
      - Modal for window, typically a dialog but generally modal for any
        window and its inferiors.  Clicking outside the modal window will
        cause a beep.
      - Application modal, i.e. always beep no matter which window.  This
        is useful for complete blocking of user-events while still performing
        layouts and repaints and so on.
      - Popup modal.  Very similar to Modal for a window, except when clicking
        outside the popup stack is closed instead of issuing a beep.

*/

// Default maximum number of colors to allocate
#define MAXCOLORS  125

using namespace FX;

/*******************************************************************************/

namespace FX {

// Horizontal splitter cursor
#include "hsplit.xbm"
#include "hsplit_mask.xbm"

// Vertical splitter cursor
#include "vsplit.xbm"
#include "vsplit_mask.xbm"

// Cross splitter cursor
#include "xsplit.xbm"
#include "xsplit_mask.xbm"

// Color swatch drag-and-drop cursor
#include "swatch.xbm"
#include "swatch_mask.xbm"

// NO DROP drag-and-drop cursor
#include "dontdrop.xbm"
#include "dontdrop_mask.xbm"

// Upper or lower side MDI resize cursor
#include "resizetop.xbm"
#include "resizetop_mask.xbm"

// Right MDI resize cursor
#include "resizetopright.xbm"
#include "resizetopright_mask.xbm"

// Left MDI resize cursor
#include "resizetopleft.xbm"
#include "resizetopleft_mask.xbm"

// Left or right side MDI resize cursor
#include "resizeleft.xbm"
#include "resizeleft_mask.xbm"

// Move cursor
#include "drag.xbm"
#include "drag_mask.xbm"

// Drag and drop ASK
#include "dndask.xbm"
#include "dndask_mask.xbm"

// Drag and drop COPY
#include "dndcopy.xbm"
#include "dndcopy_mask.xbm"

// Drag and drop LINK
#include "dndlink.xbm"
#include "dndlink_mask.xbm"

// Drag and drop MOVE
#include "dndmove.xbm"
#include "dndmove_mask.xbm"

// Crosshair
#include "crosshair.xbm"
#include "crosshair_mask.xbm"

// NE,NW,SE,SW corner cursors
#include "ne.xbm"
#include "ne_mask.xbm"
#include "nw.xbm"
#include "nw_mask.xbm"
#include "se.xbm"
#include "se_mask.xbm"
#include "sw.xbm"
#include "sw_mask.xbm"

// Help arrow
#include "helparrow.xbm"
#include "helparrow_mask.xbm"

// Rotate cursor
#include "rotate.xbm"
#include "rotate_mask.xbm"

// Hand cursor
#include "hand.xbm"
#include "hand_mask.xbm"

// Blank cursor
#include "blank.xbm"
#include "blank_mask.xbm"

//#undef __USE_XOPEN2K

/*******************************************************************************/


/*******************************************************************************/


// Application object
FXApp* FXApp::app=nullptr;


// Copyright information
const FXuchar FXApp::copyright[]="Copyright (C) 1997,2026 Jeroen van der Zijp. All Rights Reserved.";


// Conversion
extern FXAPI FXuint __strtoul(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);


#if defined(WIN32)      // Windows

// 17 stipple patterns which match up exactly with the 4x4 dither kernel
// Note that each scan line must be word-aligned so we pad to the right
// with zeroes.
static const BYTE stipple_patterns[17][16]={
  {0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00},   // 0 (white)
  {0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},   // 8 (50% grey)
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}    // 16 (black)
  };

#else                   // X11

// 17 stipple patterns which match up exactly with the 4x4 dither kernel
static const unsigned char stipple_patterns[17][8]={
  {0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00},   // 0 (white)
  {0x00,0x00,0x00,0x88, 0x00,0x00,0x00,0x88},
  {0x00,0x22,0x00,0x88, 0x00,0x22,0x00,0x88},
  {0x00,0x22,0x00,0xaa, 0x00,0x22,0x00,0xaa},
  {0x00,0xaa,0x00,0xaa, 0x00,0xaa,0x00,0xaa},
  {0x00,0xaa,0x44,0xaa, 0x00,0xaa,0x44,0xaa},
  {0x11,0xaa,0x44,0xaa, 0x11,0xaa,0x44,0xaa},
  {0x11,0xaa,0x55,0xaa, 0x11,0xaa,0x55,0xaa},
  {0x55,0xaa,0x55,0xaa, 0x55,0xaa,0x55,0xaa},   // 8 (50% grey)
  {0x55,0xaa,0x55,0xee, 0x55,0xaa,0x55,0xee},
  {0x55,0xbb,0x55,0xee, 0x55,0xbb,0x55,0xee},
  {0x55,0xbb,0x55,0xff, 0x55,0xbb,0x55,0xff},
  {0x55,0xff,0x55,0xff, 0x55,0xff,0x55,0xff},
  {0x55,0xff,0xdd,0xff, 0x55,0xff,0xdd,0xff},
  {0x77,0xff,0xdd,0xff, 0x77,0xff,0xdd,0xff},
  {0x77,0xff,0xff,0xff, 0x77,0xff,0xff,0xff},
  {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff}    // 16 (black)
  };


// Standard-issue cross hatch pattern
static const unsigned char cross_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
  };

// Standard-issue diagonal cross hatch pattern
static const unsigned char crossdiag_bits[] = {
  0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x41,
  0x80, 0x80, 0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14,
  0x22, 0x22, 0x41, 0x41, 0x80, 0x80, 0x41, 0x41
  };

// Standard-issue diagonal hatch pattern
static const unsigned char diag_bits[] = {
  0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01, 0x01,
  0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04,
  0x02, 0x02, 0x01, 0x01, 0x80, 0x80, 0x40, 0x40
  };

// Standard-issue horizontal hatch pattern
static const unsigned char hor_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

// Standard-issue reverse diagonal hatch pattern
static const unsigned char revdiag_bits[] = {
  0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40,
  0x80, 0x80, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10,
  0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x01, 0x01
  };

// Standard-issue vertical hatch pattern
static const unsigned char ver_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
  };

#endif

/*******************************************************************************/


// Map
FXDEFMAP(FXApp) FXAppMap[]={
  FXMAPFUNC(SEL_TIMEOUT,FXApp::ID_HOVER,FXApp::onCmdHover),
  FXMAPFUNC(SEL_TIMEOUT,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_CHORE,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_IO_READ,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_IO_WRITE,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_IO_EXCEPT,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,FXApp::ID_QUIT,FXApp::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,FXApp::ID_DUMP,FXApp::onCmdDump),
  };


// Implementation
FXIMPLEMENT(FXApp,FXObject,FXAppMap,ARRAYNUMBER(FXAppMap))


/*******************************************************************************/



// Initialize application object
FXApp::FXApp(const FXString& name,const FXString& vendor):registry(name,vendor),theme(),style(&theme){
  FXTRACE(TOPIC_CONSTRUCT,"FXApp::FXApp %p\n",this);

  // Initialize private platform independent data
  display=nullptr;                        // Display connection
  activeWindow=nullptr;                   // Active toplevel window
  cursorWindow=nullptr;                   // Window under the cursor
  mouseGrabWindow=nullptr;                // Window which grabbed mouse
  keyboardGrabWindow=nullptr;             // Window which grabbed keyboard
  keyWindow=nullptr;                      // Window in which keyboard key was pressed
  selectionWindow=nullptr;                // Window which has the selection
  clipboardWindow=nullptr;                // Window which has the clipboard
  dropWindow=nullptr;                     // Drag source window
  dragWindow=nullptr;                     // Drop target window
  refresher=nullptr;                      // GUI refresher pointer
  refresherstop=nullptr;                  // GUI refresher end pointer
  popupWindow=nullptr;                    // No popup windows
  timers=nullptr;                         // No timers present
  chores=nullptr;                         // No chores present
  repaints=nullptr;                       // No outstanding repaints
  timerrecs=nullptr;                      // No timer records
  chorerecs=nullptr;                      // No chore records
  repaintrecs=nullptr;                    // No repaint records
  invocation=nullptr;                     // Modal loop invocation
  callocElms(signals,MAXSIGNALS);         // Signals array
  signalreceived=0;                       // Latest received signal
  callocElms(inputs,8);                   // Input file descriptors
  ninputs=8;                              // Number of these
  callocElms(handles,1);                  // Input handles
  maxhandle=-1;                           // Maximum handle number
  inputmethod="";                         // Input method name
  inputstyle="overthespot";               // Input method style
  maxcolors=MAXCOLORS;                    // Maximum number of colors to allocate
  ddeData=nullptr;                        // Data exchange array
  ddeSize=0;                              // Data exchange array size
  appArgc=0;                              // Number of program arguments
  appArgv=nullptr;                        // Program arguments
  waitCount=0;                            // Cursor wait count
  windowCount=0;                          // Number of windows
  initialized=false;                      // Not yet initialized
  dpiawareness=DPIAWARENESS_UNKNOWN;      // DPI awareness not established
  dpi=0;                                  // DPI not determined
  dpiautoscale=false;                     // Do not follow the DPI yet
  dpicurrent=0;                           // Monitor DPI not reported yet
  dpifactor=1.0;                          // No user factor
  fontsets=nullptr;                       // No per-DPI font sets

  // Clear sticky mod state
  stickyMods=0;

  // Monochrome visual
  monoVisual=new FXVisual(this,VISUAL_MONO);

  // Default visual
  defaultVisual=new FXVisual(this,VISUAL_DEFAULT);

  // Make some cursors
  cursor[DEF_ARROW_CURSOR]=new FXCursor(this,CURSOR_ARROW);
  cursor[DEF_RARROW_CURSOR]=new FXCursor(this,CURSOR_RARROW);
  cursor[DEF_TEXT_CURSOR]=new FXCursor(this,CURSOR_IBEAM);

  // Cursors for splitter
  cursor[DEF_HSPLIT_CURSOR]=new FXCursor(this,hsplit_bits,hsplit_mask_bits,hsplit_width,hsplit_height,hsplit_x_hot,hsplit_y_hot);
  cursor[DEF_VSPLIT_CURSOR]=new FXCursor(this,vsplit_bits,vsplit_mask_bits,vsplit_width,vsplit_height,vsplit_x_hot,vsplit_y_hot);
  cursor[DEF_XSPLIT_CURSOR]=new FXCursor(this,xsplit_bits,xsplit_mask_bits,xsplit_width,xsplit_height,xsplit_x_hot,xsplit_y_hot);

  // Color swatch
  cursor[DEF_SWATCH_CURSOR]=new FXCursor(this,swatch_bits,swatch_mask_bits,swatch_width,swatch_height,swatch_x_hot,swatch_y_hot);

  // Move
  cursor[DEF_MOVE_CURSOR]=new FXCursor(this,drag_bits,drag_mask_bits,drag_width,drag_height,drag_x_hot,drag_y_hot);

  // Dragging edges/corners
  cursor[DEF_DRAGH_CURSOR]=new FXCursor(this,resizetop_bits,resizetop_mask_bits,resizetop_width,resizetop_height,resizetop_x_hot,resizetop_y_hot);
  cursor[DEF_DRAGV_CURSOR]=new FXCursor(this,resizeleft_bits,resizeleft_mask_bits,resizeleft_width,resizeleft_height,resizeleft_x_hot,resizeleft_y_hot);
  cursor[DEF_DRAGTR_CURSOR]=new FXCursor(this,resizetopright_bits,resizetopright_mask_bits,resizetopright_width,resizetopright_height,resizetopright_x_hot,resizetopright_y_hot);
  cursor[DEF_DRAGTL_CURSOR]=new FXCursor(this,resizetopleft_bits,resizetopleft_mask_bits,resizetopleft_width,resizetopleft_height,resizetopleft_x_hot,resizetopleft_y_hot);

  // DND actions
  cursor[DEF_DNDSTOP_CURSOR]=new FXCursor(this,dontdrop_bits,dontdrop_mask_bits,dontdrop_width,dontdrop_height,dontdrop_x_hot,dontdrop_y_hot);
  cursor[DEF_DNDASK_CURSOR]=new FXCursor(this,dndask_bits,dndask_mask_bits,dndask_width,dndask_height,dndcopy_x_hot,dndask_y_hot);
  cursor[DEF_DNDCOPY_CURSOR]=new FXCursor(this,dndcopy_bits,dndcopy_mask_bits,dndcopy_width,dndcopy_height,dndcopy_x_hot,dndcopy_y_hot);
  cursor[DEF_DNDMOVE_CURSOR]=new FXCursor(this,dndmove_bits,dndmove_mask_bits,dndmove_width,dndmove_height,dndmove_x_hot,dndmove_y_hot);
  cursor[DEF_DNDLINK_CURSOR]=new FXCursor(this,dndlink_bits,dndlink_mask_bits,dndlink_width,dndlink_height,dndlink_x_hot,dndlink_y_hot);

  // Crosshairs
  cursor[DEF_CROSSHAIR_CURSOR]=new FXCursor(this,crosshair_bits,crosshair_mask_bits,crosshair_width,crosshair_height,crosshair_x_hot,crosshair_y_hot);

  // NE,NW,SE,SW corners
  cursor[DEF_CORNERNE_CURSOR]=new FXCursor(this,ne_bits,ne_mask_bits,ne_width,ne_height,ne_x_hot,ne_y_hot);
  cursor[DEF_CORNERNW_CURSOR]=new FXCursor(this,nw_bits,nw_mask_bits,nw_width,nw_height,nw_x_hot,nw_y_hot);
  cursor[DEF_CORNERSE_CURSOR]=new FXCursor(this,se_bits,se_mask_bits,se_width,se_height,se_x_hot,se_y_hot);
  cursor[DEF_CORNERSW_CURSOR]=new FXCursor(this,sw_bits,sw_mask_bits,sw_width,sw_height,sw_x_hot,sw_y_hot);

  // Help arrow cursor
  cursor[DEF_HELP_CURSOR]=new FXCursor(this,helparrow_bits,helparrow_mask_bits,helparrow_width,helparrow_height,helparrow_x_hot,helparrow_y_hot);

  // Hand cursor
  cursor[DEF_HAND_CURSOR]=new FXCursor(this,hand_bits,hand_mask_bits,hand_width,hand_height,hand_x_hot,hand_y_hot);

  // Rotate
  cursor[DEF_ROTATE_CURSOR]=new FXCursor(this,rotate_bits,rotate_mask_bits,rotate_width,rotate_height,rotate_x_hot,rotate_y_hot);

  // Blank cursor
  cursor[DEF_BLANK_CURSOR]=new FXCursor(this,blank_bits,blank_mask_bits,blank_width,blank_height,blank_x_hot,blank_y_hot);

  // Wait cursor
  cursor[DEF_WAIT_CURSOR]=new FXCursor(this,CURSOR_WATCH);

  // Set current wait cursor
  waitCursor=cursor[DEF_WAIT_CURSOR];

  // Root window
  root=new FXRootWindow(this,defaultVisual);


#if defined(WIN32)      // MS-Windows specific inits

  // DDE
  ddeTargets=0;                           // Data exchange to get list of types
  ddeAtom=0;                              // Data exchange atom
  ddeDelete=0;                            // Data exchange delete request
  ddeTypeList=nullptr;                    // Data types list available
  ddeNumTypes=0;                          // Number of data types
  ddeAction=DRAG_REJECT;                  // Drag and drop action requested
  ansAction=DRAG_REJECT;                  // Drag and drop action suggested

  // SELECTION
  xselTypeList=nullptr;                   // List of primary selection types
  xselNumTypes=0;                         // How many types in list

  // XDND
  xdndFinishPending=false;                // XDND waiting for drop-confirmation
  xdndAware=0;                            // XDND awareness atom
  xdndTypes=nullptr;
  xdndSource=0;                           // XDND drag source window
  xdndTarget=0;                           // XDND drop target window
  xdndStatusPending=false;                // XDND waiting for status feedback
  xdndStatusReceived=false;               // XDND received at least one status
  xdndFinishSent=false;                   // XDND finish sent
  xdndRect.x=0;                           // XDND motion rectangle
  xdndRect.y=0;
  xdndRect.w=0;
  xdndRect.h=0;

#else                   // X Window specific inits

  wmDeleteWindow=0;                       // Window Manager stuff
  wmSaveYourself=0;
  wmQuitApp=0;
  wmProtocols=0;
  wmMotifHints=0;
  wmTakeFocus=0;
  wmClientMachine=0;
  wmState=0;

  wmNetState=0;
  wmNetIconName=0;
  wmNetWindowName=0;
  wmNetSupported=0;                       // Extended Window Manager stuff
  wmNetHidden=0;
  wmNetShaded=0;
  wmNetHMaximized=0;
  wmNetVMaximized=0;
  wmNetFullScreen=0;
  wmNetBelowOthers=0;
  wmNetAboveOthers=0;
  wmNetNeedAttention=0;
  wmNetActiveWindow=0;
  wmNetMoveResize=0;
  wmNetRestack=0;
  wmNetPing=0;
  wmNetProcessId=0;
  wmNetWindowType=0;

  wmWindowRole=0;                         // Window Role
  wmClientLeader=0;                       // Client leader
  wmClientId=0;                           // Session management id

  embedAtom=0;                            // XEMBED support
  embedInfoAtom=0;                        // XEMBED info support
  timestampAtom=0;                        // Server time

  // DDE
  ddeTargets=0;                           // Data exchange to get list of types
  ddeAtom=0;                              // Data exchange atom
  ddeDelete=0;                            // Data exchange delete request
  ddeTypeList=nullptr;                    // Data types list available
  ddeNumTypes=0;                          // Number of data types
  ddeAction=DRAG_REJECT;                  // Drag and drop action requested
  ansAction=DRAG_REJECT;                  // Drag and drop action suggested

  // CLIPBOARD
  xcbSelection=0;                         // Clipboard selection atom
  xcbTypeList=nullptr;                    // List of clipboard types
  xcbNumTypes=0;                          // How many types clipped

  // SELECTION
  xselTypeList=nullptr;                   // List of primary selection types
  xselNumTypes=0;                         // How many types in list

  // XDND
  xdndTypeList=nullptr;                   // List of XDND types
  xdndNumTypes=0;                         // How many types in list
  xdndProxy=0;                            // XDND proxy atom
  xdndAware=0;                            // XDND awareness atom
  xdndEnter=0;                            // XDND Message types
  xdndLeave=0;
  xdndPosition=0;
  xdndStatus=0;
  xdndDrop=0;
  xdndFinished=0;
  xdndSelection=0;

  xdndTypes=0;                            // XDND type list atom
  xdndActions=0;                          // XDND action list atom
  xdndActionList[DRAG_REJECT]=0;          // XDND Reject
  xdndActionList[DRAG_ASK]=0;             // XDND Ask action
  xdndActionList[DRAG_COPY]=0;            // XDND Copy action
  xdndActionList[DRAG_MOVE]=0;            // XDND Move action
  xdndActionList[DRAG_LINK]=0;            // XDND Link action
  xdndActionList[DRAG_PRIVATE]=0;         // XDND Private action

  xdndSource=0;                           // XDND drag source window
  xdndTarget=0;                           // XDND drop target window
  xdndProxyTarget=0;                      // XDND window to set messages to
  xdndStatusPending=false;                // XDND waiting for status feedback
  xdndStatusReceived=false;               // XDND received at least one status
  xdndWantUpdates=true;                   // XDND target always wants new positions
  xdndFinishSent=false;                   // XDND finish sent
  xdndRect.x=0;                           // XDND motion rectangle
  xdndRect.y=0;
  xdndRect.w=0;
  xdndRect.h=0;
  xrrScreenChange=0;                      // Xrandr event
  xfxFixesSelection=0;                    // Xfixes event
  xInputOpcode=0;                         // XInput2 Opcode
  xsbDevice=0;                            // Space ball device
  xim=nullptr;                            // Input method stuff

  // Miscellaneous stuff
  shmi=true;
  shmp=true;
  xrender=false;
  synchronize=false;

#endif

  // Timing constants
  typingSpeed=1000000000;
  clickSpeed=400000000;
  scrollSpeed=80000000;
  scrollDelay=600000000;
  blinkSpeed=500000000;
  animSpeed=20000000;
  menuPause=400000000;
  toolTipPause=800000000;
  toolTipTime=2000000000;
  autoHideDelay=2000000000;

  // Miscellaneous settings
  dragDelta=6;
  wheelLines=10;
  scrollBarSize=15;

  // Make font
#if defined(HAVE_XFT_H)
  normalFont=new FXFont(this,"Sans,90");
#else
  normalFont=new FXFont(this,"helvetica,90");
#endif

  // Theme font roles inherit the normal font until a theme names one
  clearElms(roleFonts,FONT_LAST);

  // We delete the stock font
  stockFont=normalFont;

  // Default translator
  translator=new FXTranslator;

  // Init colors
  borderColor=FXRGB(0,0,0);
  baseColor=FXRGB(212,208,200);
  hiliteColor=makeHiliteColor(baseColor);
  shadowColor=makeShadowColor(baseColor);
  backColor=FXRGB(255,255,255);
  foreColor=FXRGB(0,0,0);
  selforeColor=FXRGB(255,255,255);
  selbackColor=FXRGB(10,36,106);
  tipforeColor=FXRGB(0,0,0);
  tipbackColor=FXRGB(255,255,225);
  selMenuTextColor=FXRGB(255,255,255);
  selMenuBackColor=FXRGB(10,36,106);

  // Pointer to FXApp
  app=this;
  }


/*******************************************************************************/

// Change application name
void FXApp::setAppName(const FXString& name){
  registry.setAppKey(name);
  }


// Get application name
const FXString& FXApp::getAppName() const {
  return registry.getAppKey();
  }


// Change vendor name
void FXApp::setVendorName(const FXString& name){
  registry.setVendorKey(name);
  }


// Get vendor name
const FXString& FXApp::getVendorName() const {
  return registry.getVendorKey();
  }


// Change default visual
void FXApp::setDefaultVisual(FXVisual* vis){
  if(!vis){ fxerror("%s::setDefaultVisual: NULL visual.\n",getClassName()); }
  defaultVisual=vis;
  }


// Set root Window
void FXApp::setRootWindow(FXRootWindow* rt){
  if(!rt){ fxerror("%s::setRootWindow: NULL root window.\n",getClassName()); }
  if(root->getFirst()){ fxerror("%s::setRootWindow: already have windows.\n",getClassName()); }
  if(rt->getVisual()!=root->getVisual()){ fxerror("%s::setRootWindow: has different visual.\n",getClassName()); }
  root=rt;
  }


// Return window at the end of the focus chain
FXWindow *FXApp::getFocusWindow() const {
  FXWindow *result=getActiveWindow();
  if(result){
    while(result->getFocus()){
      result=result->getFocus();
      }
    }
  return result;
  }

#if 0
// Get foreground window
FXWindow* FXApp::getForegroundWindow() const {
#ifdef WIN32
  return findWindowWithId((FXID)GetForegroundWindow());
#else
  unsigned long n,i; Atom type; unsigned char *prop; int format;
  FXWindow *win=nullptr;
  if(Success==XGetWindowProperty((Display*)display,XDefaultRootWindow((Display*)display),wmNetActiveWindow,0,1,false,XA_WINDOW,&type,&format,&n,&i,&prop)){
    if(type==XA_WINDOW && format==32){ win=findWindowWithId(*((Window*)prop)); }
    XFree(prop);
    }
  return win;
#endif
  }
#endif

// Find window from id
FXWindow* FXApp::findWindowWithId(FXID xid) const {
  return (FXWindow*)hash.at((FXptr)xid);
  }


// Change normal font
void FXApp::setNormalFont(FXFont* font){
  if(!font){ fxerror("%s::setNormalFont: NULL font.\n",getClassName()); }
  normalFont=font;
  }

/*******************************************************************************/

// Change default cursor
void FXApp::setDefaultCursor(FXDefaultCursor which,FXCursor* cur){
  if(!cur){ fxerror("%s::setDefaultCursor: NULL default cursor.\n",getClassName()); }
  cursor[which]=cur;
  }

/*******************************************************************************/


// Change message translator
void FXApp::setTranslator(FXTranslator* trans){
  translator=trans;
  }


#if defined(WIN32)      // WIN32

// Trick to find module handle of FOX library
static HINSTANCE GetOwnModuleHandle(){
  MEMORY_BASIC_INFORMATION mbi;
  VirtualQuery((const void*)GetOwnModuleHandle,&mbi,sizeof(mbi));
  return (HINSTANCE)mbi.AllocationBase;
  }


// Open Input devices
void FXApp::openInputDevices(){
#if 0 //(_WIN32_WINNT >= 0x0501)
  RAWINPUTDEVICELIST *devlist;
  RID_DEVICE_INFO devinfo;
  RAWINPUTDEVICE dev;
  FXuint num_devices;
  FXchar devname[256];
  FXuint i;
  FXuint bc;

  // Get the number of attached raw input devices
  GetRawInputDeviceList(nullptr,&num_devices,sizeof(RAWINPUTDEVICELIST));
  if(0<num_devices){

    // Allocate the list
    if(callocElms(devlist,num_devices)){

      // Get the attached input device list
      GetRawInputDeviceList(devlist,&num_devices,sizeof(RAWINPUTDEVICELIST));   // man page is wrong about arg2

      // Get more info about each device
      for(i=0; i<num_devices; i++){
        bc=sizeof(devname);
        GetRawInputDeviceInfo(devlist[i].hDevice,RIDI_DEVICENAME,devname,&bc);
        FXTRACE(TOPIC_CONFIGURE,"device[%d] handle=%p type=%d name=%s\n",i,devlist[i].hDevice,devlist[i].dwType,devname);
        bc=devinfo.cbSize=sizeof(devinfo);
        GetRawInputDeviceInfo(devlist[i].hDevice,RIDI_DEVICEINFO,&devinfo,&bc);
        switch(devinfo.dwType){
        case RIM_TYPEMOUSE:
#if 0
          FXTRACE(TOPIC_CONFIGURE,"dwId:\t%d\n", devinfo.mouse.dwId);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfButtons:\t%d\n", devinfo.mouse.dwNumberOfButtons);
          FXTRACE(TOPIC_CONFIGURE,"dwSampleRate:\t%d\n", devinfo.mouse.dwSampleRate);
#endif
          break;
        case RIM_TYPEKEYBOARD:
#if 0
          FXTRACE(TOPIC_CONFIGURE,"dwType:\t%d\n", devinfo.keyboard.dwType);
          FXTRACE(TOPIC_CONFIGURE,"dwSubType:\t%d\n", devinfo.keyboard.dwSubType);
          FXTRACE(TOPIC_CONFIGURE,"dwKeyboardMode:\t%d\n", devinfo.keyboard.dwKeyboardMode);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfFunctionKeys:\t%d\n", devinfo.keyboard.dwNumberOfFunctionKeys);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfIndicators:\t%d\n", devinfo.keyboard.dwNumberOfIndicators);
          FXTRACE(TOPIC_CONFIGURE,"dwNumberOfKeysTotal:\t%d\n", devinfo.keyboard.dwNumberOfKeysTotal);
#endif
          break;
        case RIM_TYPEHID:
          //   FXTRACE(TOPIC_CONFIGURE,"dwVendorId:\t0x%x (%s)\n", devinfo.hid.dwVendorId, fmt_vendor(devinfo.hid.dwVendorId));
          //   FXTRACE(TOPIC_CONFIGURE,"dwProductId:\t0x%x (%s)\n", devinfo.hid.dwProductId, fmt_product(devinfo.hid.dwProductId));
          //   FXTRACE(TOPIC_CONFIGURE,"dwVersionNumber:\t%d\n", devinfo.hid.dwVersionNumber);
          //   FXTRACE(TOPIC_CONFIGURE,"usUsagePage:\t%d\n", devinfo.hid.usUsagePage);
          //   FXTRACE(TOPIC_CONFIGURE,"usUsage:\t%d\n", devinfo.hid.usUsage);
          //
          // to receive input from the SpaceNavigator
          //
          //  See:
          //      www.3dconnexion.com/forum/viewtopic.php?t=1031#5666
          //      www.usb.org/developers/hidpage/
          //

          // Match 3Dconnexion SpaceNavigator / Generic Desktop Controls / Multi-axis Controller
          if((devinfo.hid.dwVendorId==0x046d) && (devinfo.hid.dwProductId==0xc626) && (devinfo.hid.usUsagePage==1) && (devinfo.hid.usUsage==8)){
            dev.usUsagePage=1;
            dev.usUsage=8;
            dev.dwFlags=0;
            dev.hwndTarget=nullptr;               // follow keyboard focus
            if(RegisterRawInputDevices(&dev,1,sizeof(dev))){
              spaceNavHandle=devlist[i].hDevice;  // assume only one device is connected
              // FXTRACE(TOPIC_CONFIGURE,"\nRegistered for WM_INPUT!\n");
              }
            }
          break;
          }
        }
      }
    freeElms(devlist);
    }
#endif
  }


// Close Input devices
void FXApp::closeInputDevices(){
  }

#if 0
//
//	onMotion
//
long SpaceNav::onMotion(FXObject *, FXSelector, void *vp){
#if 1
  //
  // this is the "unbuffered read" method
  //
#pragma pack(1)
  struct SpaceNavStruct {
    RAWINPUTHEADER hdr;
    unsigned long bc;       // sizeof(u + id)
    unsigned long count;    // number of inputs
    unsigned char id;       // 1=translation, 2=rotation, 3=buttons
    union DataUnion {
      struct AxesStruct {
        signed short x,y,z;
        } axes;
      unsigned char buttons;
      } u;
    } snav;
  FXuint bc=sizeof(snav);
  FXint status;
#pragma pack()
  status = GetRawInputData(
          (HRAWINPUT)vp,              // hRawInput
          RID_INPUT,                  // uiCommand
          &snav,                      // pData
          &bc,                        // pcbSize
          sizeof(RAWINPUTHEADER));    // cbSizeHeader
  if(status==bc){
    switch (snav.id){
    case 1:
      dofValue[0] = snav.u.axes.x;
      dofValue[1] = snav.u.axes.y;
      dofValue[2] = snav.u.axes.z;
//    FXTRACE(TOPIC_CONFIGURE,"X{%6d %6d %6d}\n", snav.u.axes.x, snav.u.axes.y, snav.u.axes.z);
      break;
    case 2:
      dofValue[3] = snav.u.axes.x;
      dofValue[4] = snav.u.axes.y;
      dofValue[5] = snav.u.axes.z;
//    FXTRACE(TOPIC_CONFIGURE,"R{%6d %6d %6d}\n", snav.u.axes.x, snav.u.axes.y, snav.u.axes.z);
      break;
    case 3:
      FXTRACE(TOPIC_CONFIGURE,"B(%d)\n", snav.u.buttons);
      break;
    default:
      FXTRACE(TOPIC_CONFIGURE,"id=%d (?)\n", snav.id);
      break;
      }
    }
  else{
    FXTRACE(TOPIC_CONFIGURE,"Error %d != %d\n",bc,status);
    }
#else
  //
  // and the "buffered read" method..
  //
  //  I cannot get this to work.
  //  I posted a question on the MSDN forum to see if I'm doing something
  //  wrong.  I'll post more info as I get it.  In the meantime, the
  //  unbuffered read seems to work ok.
  //
  FXint status;
  FXuint num_inputs;
  FXuint sizeof_buffer;
  // can a function's semantics get any more ridiculous???
  sizeof_buffer=sizeof(RAWINPUT);
#if 1
  num_inputs=GetRawInputBuffer(nullptr, &sizeof_buffer, sizeof(RAWINPUTHEADER));
  num_inputs=sizeof_buffer/sizeof(RAWINPUT);
  if(bufAlloc<num_inputs){
    bufAlloc=num_inputs;
    resizeElms(inpBuffer,bufAlloc);
    }
#endif
  num_inputs=GetRawInputBuffer(inpBuffer,&sizeof_buffer,sizeof(RAWINPUTHEADER));
  FXTRACE(TOPIC_CONFIGURE,"onMotion(%d)\n",num_inputs);
  status=DefRawInputProc(&inpBuffer, 1/*num_inputs*/,sizeof(RAWINPUTHEADER));
#endif
  onPaint(nullptr,0,nullptr);
  return 1;
  }
#endif


#else                   // X11

// Perhaps should do something else...
static int xerrorhandler(Display* dpy,XErrorEvent* eev){
  char buf[256];

  // A BadWindow due to X_SendEvent is likely due to XDND
  if(eev->error_code==BadWindow && eev->request_code==25) return 0;

  // WM_TAKE_FOCUS causes sporadic errors for X_SetInputFocus
  if(eev->request_code==42) return 0;

  // Get error codes
  XGetErrorText(dpy,eev->error_code,buf,sizeof(buf));

  // Print out meaningful warning
  fxwarning("X Error: code %d major %d minor %d: %s.\n",eev->error_code,eev->request_code,eev->minor_code,buf);
  return 1;
  }


// Fatal error (e.g. lost connection)
static int xfatalerrorhandler(Display*){
  fxerror("X Fatal error.\n");
  return 1;
  }


// Input method was created; unregister the callback, and initialize it
void FXApp::imcreatecallback(void*,FXApp* a,void*){
#ifndef NO_XIM
  XIMCallback ximcallback;
  fxwarning("Warning: input method server restarted.\n");
  XUnregisterIMInstantiateCallback((Display*)app->getDisplay(),nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)a);
  a->xim=XOpenIM((Display*)app->display,nullptr,nullptr,nullptr);
  if(a->xim){
    ximcallback.callback=(XIMProc)imdestroycallback;
    ximcallback.client_data=(XPointer)a;
    XSetIMValues((XIM)a->xim,XNDestroyCallback,&ximcallback,nullptr);
    }
#endif
  }


// Input method was deleted; set callback to recreate it when the IM restarts.
void FXApp::imdestroycallback(void*,FXApp* a,void*){
#ifndef NO_XIM
  fxwarning("Warning: input method server terminated.\n");
  XRegisterIMInstantiateCallback((Display*)a->getDisplay(),nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)a);
  a->xim=nullptr;
#endif
  }


// Open Input devices
void FXApp::openInputDevices(){
#if defined(HAVE_XINPUT2_H)
  int majoropcode,eventbase,errorbase,ndevices;
  if(XQueryExtension((Display*)display,"XInputExtension",&majoropcode,&eventbase,&errorbase)){
    int majorversion=XI_2_Major,minorversion=XI_2_Minor;
    FXTRACE(TOPIC_DETAIL,"XInputExtension maj:%d, evt:%d, err:%d\n",majoropcode,eventbase,errorbase);
    if(XIQueryVersion((Display*)display,&majorversion,&minorversion)==Success){
      FXTRACE(TOPIC_DETAIL,"XInput2 ver:%d.%d\n",majorversion,minorversion);
      unsigned char maskbits[4]={0,0,0,0};
      XIEventMask eventmask={XIAllDevices,sizeof(maskbits),maskbits};
      XISetMask(eventmask.mask,XI_DeviceChanged);
      XISetMask(eventmask.mask,XI_HierarchyChanged);
      XISetMask(eventmask.mask,XI_PropertyEvent);
      XISelectEvents((Display*)display,XDefaultRootWindow((Display*)display),&eventmask,1);
      XIDeviceInfo *devices=XIQueryDevice((Display*)display,XIAllDevices,&ndevices);
      if(devices){
        for(FXint i=0; i<ndevices; i++){
          FXTRACE(TOPIC_DETAIL,"Device %s id: %d enabled: %d attached to: %d is a ",devices[i].name,devices[i].deviceid,devices[i].enabled,devices[i].attachment);
          switch(devices[i].use){
            case XIMasterPointer: FXTRACE(TOPIC_DETAIL,"master pointer\n"); break;
            case XIMasterKeyboard: FXTRACE(TOPIC_DETAIL,"master keyboard\n"); break;
            case XISlavePointer: FXTRACE(TOPIC_DETAIL,"slave pointer\n"); break;
            case XISlaveKeyboard: FXTRACE(TOPIC_DETAIL,"slave keyboard\n"); break;
            case XIFloatingSlave: FXTRACE(TOPIC_DETAIL,"floating slave\n"); break;
            }
          }
/*
unsigned char maskbits[4]={0,0,0,0};
XIEventMask eventmask={XIAllDevices,sizeof(maskbits),maskbits};
XISetMask(eventmask.mask,XI_DeviceChanged);
XISetMask(eventmask.mask,XI_HierarchyChanged);
XISetMask(eventmask.mask,XI_PropertyEvent);
XISelectEvents(DISPLAY(getApp()),xid,&eventmask,1);
XIClearMask(eventmask.mask,XI_DeviceChanged);
XIClearMask(eventmask.mask,XI_HierarchyChanged);
XIClearMask(eventmask.mask,XI_PropertyEvent);
eventmask.deviceid=12;
XISetMask(eventmask.mask,XI_RawKeyPress);
XISetMask(eventmask.mask,XI_RawKeyRelease);
XISetMask(eventmask.mask,XI_RawButtonPress);
XISetMask(eventmask.mask,XI_RawButtonRelease);
XISetMask(eventmask.mask,XI_RawMotion);
XISelectEvents(DISPLAY(getApp()),xid,&eventmask,1);
*/
        XIFreeDeviceInfo(devices);
        xInputOpcode=majoropcode;
        }
      }
    }
#endif
  }


// Close Input devices
void FXApp::closeInputDevices(){
#if defined(HAVE_XINPUT2_H)
  // FIXME
#endif
  }


// Bunch-o-atoms
const FXchar *const windowTypeAtoms[14]={
  "_NET_WM_WINDOW_TYPE_DESKTOP",
  "_NET_WM_WINDOW_TYPE_DOCK",
  "_NET_WM_WINDOW_TYPE_TOOLBAR",
  "_NET_WM_WINDOW_TYPE_MENU",
  "_NET_WM_WINDOW_TYPE_UTILITY",
  "_NET_WM_WINDOW_TYPE_SPLASH",
  "_NET_WM_WINDOW_TYPE_DIALOG",
  "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
  "_NET_WM_WINDOW_TYPE_POPUP_MENU",
  "_NET_WM_WINDOW_TYPE_TOOLTIP",
  "_NET_WM_WINDOW_TYPE_NOTIFICATION",
  "_NET_WM_WINDOW_TYPE_COMBO",
  "_NET_WM_WINDOW_TYPE_DND",
  "_NET_WM_WINDOW_TYPE_NORMAL"
  };


#endif

/*******************************************************************************/

// Return the DPI of the monitor a native window is on, or 0 if unknown
FXint FXApp::getWindowDPI(FXID hwnd) const {
  FXint value=fxplatformGetWindowDPI(hwnd);
  return value>0?value:dpi;
  }


// Recursively record the DPI of a window and its descendants
void FXApp::applyWindowDPI(FXWindow* window,FXint dpivalue){
  FXWindow *child;
  window->dpi=dpivalue;
  for(child=window->getFirst(); child; child=child->getNext()){
    applyWindowDPI(child,dpivalue);
    }
  }


// Return the factor applied to font descriptions; fonts already follow the
// monitor DPI, so only the difference between display scale and DPI applies.
FXdouble FXApp::getFontScale() const {
  FXdouble native=dpi?FXdouble(dpi)/96.0:1.0;
  return native>0.0?theme.getScale()/native:theme.getScale();
  }


// Apply the display scale to the normal font and drop the cached role fonts
void FXApp::applyFontScale(){
  FXFontDesc desc;
  FXdouble factor=getFontScale();
  if(basefontdesc.size){
    desc=basefontdesc;
    desc.size=FXint(FXdouble(desc.size)*factor+0.5);
    normalFont->setFontDesc(desc);
    if(isInitialized()){
      normalFont->destroy();
      normalFont->create();
      }
    }
  releaseRoleFonts();
  }


// Change the theme scale and relayout if it actually changed
void FXApp::setThemeScale(FXdouble scale){
  if(theme.getScale()!=scale){
    theme.setScale(scale);
    applyFontScale();
    style.invalidate();
    themeChanged();
    if(root){
      FXWindow *win;
      for(win=root->getFirst(); win; win=win->getNext()){
        win->recalc();
        }
      }
    }
  }

// Open the display
FXbool FXApp::openDisplay(const FXchar* dpy){
  if(!initialized){

#if defined(WIN32)      // MS-Windows

    // What's going on
    FXTRACE(TOPIC_DETAIL,"%s::openDisplay(%s)\n",getClassName(),dpy);

    // Set to HINSTANCE on Windows
    display=GetOwnModuleHandle();

    // Process DPI awareness was already established by init(), before the
    // system font metrics were read.

    // TARGETS
    ddeTargets=GlobalAddAtomA("TARGETS");

    // XDND protocol awareness
    xdndAware=GlobalAddAtomA("XdndAware");

    // DDE property
    ddeDelete=RegisterClipboardFormatA("DELETE");

    // Standard stipples
    stipples[STIPPLE_0]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_0]);
    stipples[STIPPLE_1]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_1]);
    stipples[STIPPLE_2]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_2]);
    stipples[STIPPLE_3]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_3]);
    stipples[STIPPLE_4]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_4]);
    stipples[STIPPLE_5]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_5]);
    stipples[STIPPLE_6]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_6]);
    stipples[STIPPLE_7]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_7]);
    stipples[STIPPLE_8]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_8]);
    stipples[STIPPLE_9]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_9]);
    stipples[STIPPLE_10]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_10]);
    stipples[STIPPLE_11]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_11]);
    stipples[STIPPLE_12]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_12]);
    stipples[STIPPLE_13]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_13]);
    stipples[STIPPLE_14]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_14]);
    stipples[STIPPLE_15]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_15]);
    stipples[STIPPLE_16]=CreateBitmap(8,8,1,1,stipple_patterns[STIPPLE_16]);

    // Register our child window classes
    WNDCLASSEX wndclass;

    // Child window
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW;     // Set to 0 for bit_gravity
    //wndclass.style=0;
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXWindow");
    RegisterClassEx(&wndclass);

    // Top window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW;
    //wndclass.style=0;
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=LoadIcon((HINSTANCE)display,IDI_APPLICATION);
    if(!wndclass.hIcon) wndclass.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
    wndclass.hIconSm=(HICON)LoadImage((HINSTANCE)display,IDI_APPLICATION,IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    if(!wndclass.hIconSm) wndclass.hIconSm=wndclass.hIcon;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXTopWindow");
    RegisterClassEx(&wndclass);

    // OpenGL window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;   // Redraw all when resized, OWNER DC for speed
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXGLCanvas");
    RegisterClassEx(&wndclass);

    // Dummy GL window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;   // Redraw all when resized, OWNER DC for speed
    wndclass.lpfnWndProc=DefWindowProc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=0;
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=nullptr;
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("GLTEMP");
    RegisterClassEx(&wndclass);

    // Popup window class
    wndclass.cbSize=sizeof(WNDCLASSEX);
    wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS;   // Do save-under's
    wndclass.lpfnWndProc=(WNDPROC) FXApp::wndproc;
    wndclass.cbClsExtra=0;
    wndclass.cbWndExtra=sizeof(FXWindow*);
    wndclass.hInstance=(HINSTANCE)display;
    wndclass.hIcon=nullptr;
    wndclass.hIconSm=nullptr;
    wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wndclass.hbrBackground=nullptr;
    wndclass.lpszMenuName=nullptr;
    wndclass.lpszClassName=TEXT("FXPopup");
    RegisterClassEx(&wndclass);

    // This should prevent the Abort/Retry/Ignore message
    // when switching to drive w/no media mounted in it...
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Preregister some drag types
    FXWindow::octetType=RegisterClipboardFormatA(FXWindow::octetTypeName);
    FXWindow::deleteType=RegisterClipboardFormatA(FXWindow::deleteTypeName);
    FXWindow::textType=RegisterClipboardFormatA(FXWindow::textTypeName);
    FXWindow::colorType=RegisterClipboardFormatA(FXWindow::colorTypeName);
    FXWindow::urilistType=RegisterClipboardFormatA(FXWindow::urilistTypeName);
    FXWindow::utf8Type=RegisterClipboardFormatA(FXWindow::utf8TypeName);
    FXWindow::utf16Type=CF_UNICODETEXT;
    FXWindow::actionType=RegisterClipboardFormatA(FXWindow::actionTypeName);

#else                   // X11

    // Using thread-safe X11 if possible
    //XInitThreads();

    // Set error handler
    XSetErrorHandler(xerrorhandler);

    // Set fatal handler
    XSetIOErrorHandler(xfatalerrorhandler);

    // What's going on
    FXTRACE(TOPIC_DETAIL,"%s::openDisplay(%s)\n",getClassName(),dpy);

    // Open display
    display=XOpenDisplay(dpy);
    if(!display) return false;

    // For debugging
    if(synchronize) XSynchronize((Display*)display,true);

    // Setup locales and input method if given
    if(XSupportsLocale()){
      FXchar mods[100]="";
      if(inputmethod[0]){
        fxstrlcpy(mods,"@im=",sizeof(mods));
        fxstrlcat(mods,inputmethod,sizeof(mods));
        }
      if(!XSetLocaleModifiers(mods)){
        if(!XSetLocaleModifiers("")){
          fxwarning("Cannot set locale modifiers.\n");
          }
        }
      }

#if defined(HAVE_XSHM_H)

    // Displaying remotely turns it off for sure
    if(!(dpy[0]==':' && Ascii::isDigit(dpy[1]))){
      shmi=false;
      shmp=false;
      }

    // Its potentially on, see if local display groks it
    if(shmi || shmp){
      int maj,min,dum; Bool pm;
      shmi=false;
      shmp=false;
      if(XQueryExtension((Display*)display,"MIT-SHM",&dum,&dum,&dum)){
        if(XShmQueryVersion((Display*)display,&maj,&min,&pm)){
          shmp=pm && (XShmPixmapFormat((Display*)display)==ZPixmap);
          shmi=true;
          }
        }
      }

#else

    // Don't have it!
    shmi=false;
    shmp=false;

#endif

    // Report the result
    FXTRACE(TOPIC_DETAIL,"X Shared Images  = %d\n",shmi);
    FXTRACE(TOPIC_DETAIL,"X Shared Pixmaps = %d\n",shmp);

    // X Render Extension
#if defined(HAVE_XRENDER_H)
    int xrenderevent,xrendererror;
    if(XRenderQueryExtension((Display*)display,&xrenderevent,&xrendererror)){
      int xrendermajor,xrenderminor;
      if(XRenderQueryVersion((Display*)display,&xrendermajor,&xrenderminor)){
        xrender=true;
        FXTRACE(TOPIC_DETAIL,"X Render %d.%d available\n",xrendermajor,xrenderminor);
        }
      }
#endif

    // Initialize Xft and fontconfig
#if defined(HAVE_XFT_H)
    if(!XftInit(nullptr)) return false;
    if(XftGetVersion()<XftVersion){
      fxwarning("Expected Xft library version %d or greater; was %d.\n",XftVersion,XftGetVersion());
      }
#endif

    // Open input method
#ifndef NO_XIM
    xim=XOpenIM((Display*)display,nullptr,nullptr,nullptr);
    if(xim){
      XIMCallback ximcallback;
      ximcallback.callback=(XIMProc)imdestroycallback;
      ximcallback.client_data=(XPointer)this;
      XSetIMValues((XIM)xim,XNDestroyCallback,&ximcallback,nullptr);
      FXTRACE(TOPIC_DETAIL,"Got Input Method\n");
      }
    else{
      XRegisterIMInstantiateCallback((Display*)display,nullptr,nullptr,nullptr,(XIDProc)imcreatecallback,(XPointer)this);
      }
#endif

    // Check for X Rotation and Reflection support
#if defined(HAVE_XRANDR_H)
    int rreventbase,rrerrorbase;
    if(XRRQueryExtension((Display*)display,&rreventbase,&rrerrorbase)){
      XRRSelectInput((Display*)display,XDefaultRootWindow((Display*)display),True);
      xrrScreenChange=rreventbase+RRScreenChangeNotify;
      FXTRACE(TOPIC_DETAIL,"xrrScreenChange: %d\n",xrrScreenChange);
      }
#endif

    // Check for X Fixes support
#if defined(HAVE_XFIXES_H)
    int fxeventbase,fxerrorbase;
    if(XFixesQueryExtension((Display*)display,&fxeventbase,&fxerrorbase)){
      xfxFixesSelection=fxeventbase+XFixesSelectionNotify;
      FXTRACE(TOPIC_DETAIL,"xfxFixesSelection: %d\n",xfxFixesSelection);
      //int major_version=XFIXES_MAJOR;
      //int minor_version=XFIXES_MINOR;
      //XFixesQueryVersion((Display*)display,&major_version,&minor_version);
      //XFixesSelectSelectionInput((Display*)display,XDefaultRootWindow((Display*)display),xcbSelection,XFixesSetSelectionOwnerNotifyMask|XFixesSelectionWindowDestroyNotifyMask|XFixesSelectionClientCloseNotifyMask);
      }
#endif

    // Window Manager communication
    wmDeleteWindow=XInternAtom((Display*)display,"WM_DELETE_WINDOW",0);
    wmSaveYourself=XInternAtom((Display*)display,"WM_SAVE_YOURSELF",0);
    wmQuitApp=XInternAtom((Display*)display,"_WM_QUIT_APP",0);
    wmProtocols=XInternAtom((Display*)display,"WM_PROTOCOLS",0);
    wmMotifHints=XInternAtom((Display*)display,"_MOTIF_WM_HINTS",0);
    wmTakeFocus=XInternAtom((Display*)display,"WM_TAKE_FOCUS",0);
    wmClientMachine=XInternAtom((Display*)display,"WM_CLIENT_MACHINE",0);
    wmState=XInternAtom((Display*)display,"WM_STATE",0);

    // Extended Window Manager support
    wmNetState=XInternAtom((Display*)display,"_NET_WM_STATE",0);
    wmNetIconName=XInternAtom((Display*)display,"_NET_WM_ICON_NAME",0);
    wmNetWindowName=XInternAtom((Display*)display,"_NET_WM_NAME",0);
    wmNetSupported=XInternAtom((Display*)display,"_NET_SUPPORTED",0);
    wmNetHidden=XInternAtom((Display*)display,"_NET_WM_STATE_HIDDEN",0);
    wmNetShaded=XInternAtom((Display*)display,"_NET_WM_STATE_SHADED",0);
    wmNetHMaximized=XInternAtom((Display*)display,"_NET_WM_STATE_MAXIMIZED_HORZ",0);
    wmNetVMaximized=XInternAtom((Display*)display,"_NET_WM_STATE_MAXIMIZED_VERT",0);
    wmNetFullScreen=XInternAtom((Display*)display,"_NET_WM_STATE_FULLSCREEN",0);
    wmNetBelowOthers=XInternAtom((Display*)display,"_NET_WM_STATE_BELOW",0);
    wmNetAboveOthers=XInternAtom((Display*)display,"_NET_WM_STATE_ABOVE",0);
    wmNetNeedAttention=XInternAtom((Display*)display,"_NET_WM_STATE_DEMANDS_ATTENTION",0);
    wmNetActiveWindow=XInternAtom((Display*)display,"_NET_ACTIVE_WINDOW",0);

    wmNetMoveResize=XInternAtom((Display*)display,"_NET_WM_MOVERESIZE",0);
    wmNetRestack=XInternAtom((Display*)display,"_NET_RESTACK_WINDOW",0);
    wmNetPing=XInternAtom((Display*)display,"_NET_WM_PING",0);
    wmNetProcessId=XInternAtom((Display*)display,"_NET_WM_PID",0);
    wmNetWindowType=XInternAtom((Display*)display,"_NET_WM_WINDOW_TYPE",0);

    // Register window types
    XInternAtoms((Display*)display,const_cast<char**>(windowTypeAtoms),14,0,wmWindowTypes);

    // Session management
    wmWindowRole=XInternAtom((Display*)display,"WM_WINDOW_ROLE",0);
    wmClientLeader=XInternAtom((Display*)display,"WM_CLIENT_LEADER",0);
    wmClientId=XInternAtom((Display*)display,"SM_CLIENT_ID",0);

    // XEMBED support
    embedAtom=XInternAtom((Display*)display,"_XEMBED",0);
    embedInfoAtom=XInternAtom((Display*)display,"_XEMBED_INFO",0);
    timestampAtom=XInternAtom((Display*)display,"FOX_TIMESTAMP_PROP",0);

    // DDE property
    ddeAtom=(FXID)XInternAtom((Display*)display,"_FOX_DDE",0);
    ddeDelete=(FXID)XInternAtom((Display*)display,"DELETE",0);
    ddeTargets=(FXID)XInternAtom((Display*)display,"TARGETS",0);
    ddeIncr=(FXID)XInternAtom((Display*)display,"INCR",0);

    // Clipboard
    xcbSelection=(FXID)XInternAtom((Display*)display,"CLIPBOARD",0);

    // XDND protocol awareness
    xdndProxy=(FXID)XInternAtom((Display*)display,"XdndProxy",0);
    xdndAware=(FXID)XInternAtom((Display*)display,"XdndAware",0);

    // XDND Messages
    xdndEnter=(FXID)XInternAtom((Display*)display,"XdndEnter",0);
    xdndLeave=(FXID)XInternAtom((Display*)display,"XdndLeave",0);
    xdndPosition=(FXID)XInternAtom((Display*)display,"XdndPosition",0);
    xdndStatus=(FXID)XInternAtom((Display*)display,"XdndStatus",0);
    xdndDrop=(FXID)XInternAtom((Display*)display,"XdndDrop",0);
    xdndFinished=(FXID)XInternAtom((Display*)display,"XdndFinished",0);

    // XDND Selection atom
    xdndSelection=(FXID)XInternAtom((Display*)display,"XdndSelection",0);

    // XDND Types list
    xdndTypes=(FXID)XInternAtom((Display*)display,"XdndTypeList",0);

    // XDND Action list
    xdndActions=(FXID)XInternAtom((Display*)display,"XdndActionList",0);

    // XDND Actions
    xdndActionList[DRAG_REJECT]=None;
    xdndActionList[DRAG_ASK]=(FXID)XInternAtom((Display*)display,"XdndActionAsk",0);
    xdndActionList[DRAG_COPY]=(FXID)XInternAtom((Display*)display,"XdndActionCopy",0);
    xdndActionList[DRAG_MOVE]=(FXID)XInternAtom((Display*)display,"XdndActionMove",0);
    xdndActionList[DRAG_LINK]=(FXID)XInternAtom((Display*)display,"XdndActionLink",0);
    xdndActionList[DRAG_PRIVATE]=(FXID)XInternAtom((Display*)display,"XdndActionPrivate",0);

    // Standard stipples
    stipples[STIPPLE_0]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_0],8,8);
    stipples[STIPPLE_1]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_1],8,8);
    stipples[STIPPLE_2]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_2],8,8);
    stipples[STIPPLE_3]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_3],8,8);
    stipples[STIPPLE_4]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_4],8,8);
    stipples[STIPPLE_5]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_5],8,8);
    stipples[STIPPLE_6]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_6],8,8);
    stipples[STIPPLE_7]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_7],8,8);
    stipples[STIPPLE_8]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_8],8,8);
    stipples[STIPPLE_9]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_9],8,8);
    stipples[STIPPLE_10]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_10],8,8);
    stipples[STIPPLE_11]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_11],8,8);
    stipples[STIPPLE_12]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_12],8,8);
    stipples[STIPPLE_13]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_13],8,8);
    stipples[STIPPLE_14]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_14],8,8);
    stipples[STIPPLE_15]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_15],8,8);
    stipples[STIPPLE_16]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)stipple_patterns[STIPPLE_16],8,8);

    // Hatch patterns
    stipples[STIPPLE_HORZ]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)hor_bits,24,24);
    stipples[STIPPLE_VERT]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)ver_bits,24,24);
    stipples[STIPPLE_CROSS]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)cross_bits,24,24);
    stipples[STIPPLE_DIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)diag_bits,16,16);
    stipples[STIPPLE_REVDIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)revdiag_bits,16,16);
    stipples[STIPPLE_CROSSDIAG]=(FXID)XCreateBitmapFromData((Display*)display,XDefaultRootWindow((Display*)display),(const char*)crossdiag_bits,16,16);

    // Preregister some drag types
    FXWindow::octetType=(FXDragType)XInternAtom((Display*)display,FXWindow::octetTypeName,0);
    FXWindow::deleteType=(FXDragType)XInternAtom((Display*)display,FXWindow::deleteTypeName,0);
    FXWindow::textType=(FXDragType)XInternAtom((Display*)display,FXWindow::textTypeName,0);
    FXWindow::colorType=(FXDragType)XInternAtom((Display*)display,FXWindow::colorTypeName,0);
    FXWindow::urilistType=(FXDragType)XInternAtom((Display*)display,FXWindow::urilistTypeName,0);
    FXWindow::utf8Type=(FXDragType)XInternAtom((Display*)display,FXWindow::utf8TypeName,0);
    FXWindow::utf16Type=(FXDragType)XInternAtom((Display*)display,FXWindow::utf16TypeName,0);
    FXWindow::actionType=(FXDragType)XInternAtom((Display*)display,FXWindow::actionTypeName,0);
#endif

    // Open input devices
    openInputDevices();

    // Clear sticky mod state
    stickyMods=0;

    // Lock the global mutex
    appMutex.lock();

    // We have been initialized
    initialized=true;
    }
  return true;
  }


/*******************************************************************************/

// Add deadline in nanoseconds
FXptr FXApp::addDeadline(FXObject* tgt,FXSelector sel,FXTime due,FXptr ptr){
  FXptr result=nullptr;
  FXTimer *t,**tt;
  for(tt=&timers; (t=*tt)!=nullptr; tt=&t->next){
    if(t->target==tgt && t->message==sel){ *tt=t->next; result=t->data; goto a; }
    }
  if(timerrecs){
    t=timerrecs;
    timerrecs=t->next;
    }
  else{
    t=new FXTimer;
    }
a:t->data=ptr;
  t->target=tgt;
  t->message=sel;
  t->due=due;
  for(tt=&timers; *tt!=nullptr && ((*tt)->due < t->due); tt=&(*tt)->next){}
  t->next=*tt;
  *tt=t;
  return result;
  }


// Add timeout in nanoseconds
FXptr FXApp::addTimeout(FXObject* tgt,FXSelector sel,FXTime ns,FXptr ptr){
  return addDeadline(tgt,sel,FXThread::time()+ns,ptr);
  }


// Check if timeout identified by tgt and sel has been set
FXbool FXApp::hasTimeout(FXObject* tgt,FXSelector sel) const {
  for(FXTimer *t=timers; t; t=t->next){
    if(t->target==tgt && (sel==0 || t->message==sel)) return true;
    }
  return false;
  }


// Remove timeout(s) identified by tgt and sel from the list
FXptr FXApp::removeTimeout(FXObject* tgt,FXSelector sel){
  FXptr result=nullptr;
  FXTimer **tt=&timers;
  FXTimer *t;
  while((t=*tt)!=nullptr){
    if(t->target==tgt && (sel==0 || t->message==sel)){
      *tt=t->next; result=t->data; t->next=timerrecs; timerrecs=t;
      continue;
      }
    tt=&t->next;
    }
  return result;
  }


// Return the remaining time, in nanoseconds
FXTime FXApp::remainingTimeout(FXObject *tgt,FXSelector sel) const {
  for(FXTimer *t=timers; t; t=t->next){
    if(t->target==tgt && (sel==0 || t->message==sel)){
      FXTime now=FXThread::time();
      return t->due>now ? t->due-now : 0L;
      }
    }
  return forever;
  }


/*******************************************************************************/


// Signal handler; note this is a single write operation
// which can not be interrupted by another handler!
void FXApp::signalhandler(int sig){
  app->signals[sig].notified=true;
  app->signalreceived=sig;
  }


// This signal handler is potentially dangerous as it dispatches
// the message to the target right here in the handler; you probably
// want to use this one only in ``desperate'' situations.
void FXApp::immediatesignalhandler(int sig){
  if(app->signals[sig].target) app->signals[sig].target->handle(app,FXSEL(SEL_SIGNAL,app->signals[sig].message),(void*)(FXival)sig);
  }


/*******************************************************************************/


// Add chore to the END of the list
FXptr FXApp::addChore(FXObject* tgt,FXSelector sel,FXptr ptr){
  FXptr result=nullptr;
  FXChore *c,**cc;
  for(cc=&chores; (c=*cc)!=nullptr; cc=&c->next){
    if(c->target==tgt && c->message==sel){ *cc=c->next; result=c->data; goto a; }
    }
  if(chorerecs){
    c=chorerecs;
    chorerecs=c->next;
    }
  else{
    c=new FXChore;
    }
a:c->data=ptr;
  c->target=tgt;
  c->message=sel;
  for(cc=&chores; *cc; cc=&(*cc)->next){}
  c->next=nullptr;
  *cc=c;
  return result;
  }


// Remove chore(s) identified by tgt and sel from the list
FXptr FXApp::removeChore(FXObject* tgt,FXSelector sel){
  FXptr result=nullptr;
  FXChore **cc=&chores;
  FXChore *c;
  while((c=*cc)!=nullptr){
    if(c->target==tgt && (sel==0 || c->message==sel)){
      *cc=c->next; result=c->data; c->next=chorerecs; chorerecs=c;
      continue;
      }
    cc=&c->next;
    }
  return result;
  }


// Check if chore identified by tgt and sel has been set
FXbool FXApp::hasChore(FXObject* tgt,FXSelector sel) const {
  for(FXChore *c=chores; c; c=c->next){
    if(c->target==tgt && (sel==0 || c->message==sel)) return true;
    }
  return false;
  }

/*******************************************************************************/


/*******************************************************************************/


// Generate SEL_LEAVE
void FXApp::leaveWindow(FXWindow *window,FXWindow *ancestor){
  if(window && window->getParent() && window!=ancestor){
    event.type=SEL_LEAVE;
    window->translateCoordinatesFrom(event.win_x,event.win_y,root,event.root_x,event.root_y);
    if(window->handle(this,FXSEL(SEL_LEAVE,0),&event)) refresh();
    cursorWindow=window->getParent();
    leaveWindow(window->getParent(),ancestor);
    }
  }


// Generate SEL_ENTER
void FXApp::enterWindow(FXWindow *window,FXWindow *ancestor){
  if(window && window->getParent() && window!=ancestor){
    enterWindow(window->getParent(),ancestor);
    event.type=SEL_ENTER;
    window->translateCoordinatesFrom(event.win_x,event.win_y,root,event.root_x,event.root_y);
    cursorWindow=window;
    if(window->handle(this,FXSEL(SEL_ENTER,0),&event)) refresh();
    }
  }


/*******************************************************************************/



/*******************************************************************************/


// Force GUI refresh of every widget in this application
void FXApp::forceRefresh(){
  getRootWindow()->forceRefresh();
  }


// Schedule a future refresh; if we were in the middle of
// one, we continue with the current cycle until we wrap
// around to the current widget about to be updated.
void FXApp::refresh(){
  if(!refresher) refresher=root;
  refresherstop=refresher;
  }


// Run application
FXint FXApp::run(){
  FXInvocation inv(&invocation,MODAL_FOR_NONE,nullptr);
  FXTRACE(TOPIC_DETAIL,"Start run\n");
  while(!inv.done){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End run\n");
  return inv.code;
  }


// Run till some flag becomes non-zero
FXint FXApp::runUntil(FXuint& condition){
  FXInvocation inv(&invocation,MODAL_FOR_NONE,nullptr);
  FXTRACE(TOPIC_DETAIL,"Start runUntil\n");
  while(!inv.done && condition==0){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End runUntil\n");
  return condition;
  }


// Run event loop while events are available
FXint FXApp::runWhileEvents(FXTime blocking){
  FXInvocation inv(&invocation,MODAL_FOR_NONE,nullptr);
  FXTRACE(TOPIC_DETAIL,"Start runWhileEvents\n");
  while(!inv.done && runOneEvent(blocking)) blocking=1000;
  FXTRACE(TOPIC_DETAIL,"End runWhileEvents\n");
  return !inv.done;
  }


// Run event loop while events are available
FXint FXApp::runModalWhileEvents(FXWindow* window,FXTime blocking){
  FXInvocation inv(&invocation,MODAL_FOR_WINDOW,window);
  FXTRACE(TOPIC_DETAIL,"Start runModalWhileEvents\n");
  while(!inv.done && runOneEvent(blocking)) blocking=1000;
  FXTRACE(TOPIC_DETAIL,"End runModalWhileEvents\n");
  return !inv.done;
  }


// Perform one event dispatch
FXbool FXApp::runOneEvent(FXTime blocking){
  FXRawEvent ev;
  if(getNextEvent(ev,blocking)){
    dispatchEvent(ev);
    return true;
    }
  return false;
  }


// Run modal event loop, blocking events to all windows, until stopModal is called.
FXint FXApp::runModal(){
  FXInvocation inv(&invocation,MODAL_FOR_WINDOW,nullptr);
  FXTRACE(TOPIC_DETAIL,"Start runModal\n");
  while(!inv.done){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End runModal\n");
  return inv.code;
  }


// Run modal for window
FXint FXApp::runModalFor(FXWindow* window){
  FXInvocation inv(&invocation,MODAL_FOR_WINDOW,window);
  FXTRACE(TOPIC_DETAIL,"Start runModalFor\n");
  while(!inv.done){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End runModalFor\n");
  return inv.code;
  }


// Run modal while window is shown, or until stopModal is called
FXint FXApp::runModalWhileShown(FXWindow* window){
  FXInvocation inv(&invocation,MODAL_FOR_WINDOW,window);
  FXTRACE(TOPIC_DETAIL,"Start runModalWhileShown\n");
  while(!inv.done && window->shown()){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End runModalWhileShown\n");
  return inv.code;
  }


// Run popup menu
FXint FXApp::runPopup(FXWindow* window){
  FXInvocation inv(&invocation,MODAL_FOR_POPUP,window);
  FXTRACE(TOPIC_DETAIL,"Start runPopup\n");
  while(!inv.done && window->shown()){
    runOneEvent();
    }
  FXTRACE(TOPIC_DETAIL,"End runPopup\n");
  return inv.code;
  }


// Test if the window is involved in a modal invocation
FXbool FXApp::isModal(FXWindow *window) const {
  FXInvocation* inv;
  for(inv=invocation; inv; inv=inv->upper){
    if(inv->window==window && inv->modality!=MODAL_FOR_NONE) return true;
    }
  return false;
  }


// Return current modal window, if any
FXWindow* FXApp::getModalWindow() const {
  return invocation ? invocation->window : nullptr;
  }


// Return mode of current modal loop
FXModality FXApp::getModality() const {
  return invocation ? invocation->modality : MODAL_FOR_NONE;
  }


// Break out of topmost event loop, closing all nested loops also
void FXApp::stop(FXint value){
  FXInvocation* inv;
  for(inv=invocation; inv; inv=inv->upper){
    inv->done=true;
    inv->code=0;
    if(!inv->upper){
      inv->code=value;
      return;
      }
    }
  }


// Break out of modal loop matching window, and all deeper ones
void FXApp::stopModal(FXWindow* window,FXint value){
  FXInvocation* inv;
  if(isModal(window)){
    for(inv=invocation; inv; inv=inv->upper){
      inv->done=true;
      inv->code=0;
      if(inv->window==window && inv->modality!=MODAL_FOR_NONE){
        inv->code=value;
        return;
        }
      }
    }
  }


// Break out of innermost modal loop, and all deeper non-modal ones
void FXApp::stopModal(FXint value){
  FXInvocation* inv;
  for(inv=invocation; inv; inv=inv->upper){
    inv->done=true;
    inv->code=0;
    if(inv->modality!=MODAL_FOR_NONE){
      inv->code=value;
      return;
      }
    }
  }


// Obtain system color and translate to FXColor
#if defined(WIN32)
static FXColor getSystemColor(FXuint which){
  DWORD dwColor=GetSysColor(which);
  return FXRGB(GetRValue(dwColor),GetGValue(dwColor),GetBValue(dwColor));
  }
#endif


// Obtain system font and translate to FXFontDesc
#if defined(WIN32)
static void getSystemFont(FXFontDesc& fontdesc){
  NONCLIENTMETRICS ncm;
  clearElms(&ncm,1);
  ncm.cbSize=sizeof(NONCLIENTMETRICS);
#if (WINVER >= 0x0600)                          // Fix from: Daniel Stine <robodanny@gmail.com>
  OSVERSIONINFO osvi;                           // Adjust size based on operating system version
  clearElms(&osvi,1);                           // So get run-time operating system version first
  osvi.dwOSVersionInfoSize=sizeof(osvi);        // if headers include new member iPaddedBorderWidth
  GetVersionEx(&osvi);                          // in NONCLIENTMETRICS struct
  if(osvi.dwMajorVersion<6) ncm.cbSize-=sizeof(ncm.iPaddedBorderWidth);
#endif
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0);
#if defined(UNICODE)
  ncs2utf(fontdesc.face,ncm.lfMenuFont.lfFaceName,sizeof(fontdesc.face),LF_FACESIZE);
#else
  fxstrlcpy(fontdesc.face,ncm.lfMenuFont.lfFaceName,sizeof(fontdesc.face));
#endif
  fontdesc.face[sizeof(fontdesc.face)-1]='\0';
  HDC hDC=CreateCompatibleDC(nullptr);
  fontdesc.size=-10*MulDiv(ncm.lfMenuFont.lfHeight,72,GetDeviceCaps(hDC,LOGPIXELSY));
  DeleteDC(hDC);
  fontdesc.weight=(FXushort)ncm.lfMenuFont.lfWeight/10;
  fontdesc.slant=ncm.lfMenuFont.lfItalic?FXFont::Italic:FXFont::Straight;
  fontdesc.encoding=FONTENCODING_DEFAULT;
  fontdesc.setwidth=0;
  fontdesc.flags=0;
  }
#endif


// Initialize application and optionally open display
void FXApp::init(int& argc,char** argv,FXbool connect){
  const FXchar *fontspec=nullptr;
  const FXchar *istyle=nullptr;
  const FXchar *dpy=":0.0";
  const FXchar *str=nullptr;
  FXuint maxcols=0;
  FXint i,j;

  // Verify implementation invariants
  FXSTATIC_ASSERT(sizeof(FXuchar)==1);
  FXSTATIC_ASSERT(sizeof(FXchar)==1);
  FXSTATIC_ASSERT(sizeof(FXushort)==2);
  FXSTATIC_ASSERT(sizeof(FXshort)==2);
  FXSTATIC_ASSERT(sizeof(FXuint)==4);
  FXSTATIC_ASSERT(sizeof(FXnchar)==2);
  FXSTATIC_ASSERT(sizeof(FXwchar)==4);
  FXSTATIC_ASSERT(sizeof(FXint)==4);
  FXSTATIC_ASSERT(sizeof(FXulong)==8);
  FXSTATIC_ASSERT(sizeof(FXlong)==8);
  FXSTATIC_ASSERT(sizeof(FXfloat)==4);
  FXSTATIC_ASSERT(sizeof(FXdouble)==8);
  FXSTATIC_ASSERT(sizeof(FXival)==sizeof(void*));
  FXSTATIC_ASSERT(sizeof(FXuval)==sizeof(void*));
  FXSTATIC_ASSERT(sizeof(FXString)==sizeof(void*));

  // Check arguments
  if(argc<1 || argv==nullptr || argv[0]==nullptr){
    fxerror("%s::init: bad arguments.\n",getClassName());
    }

  // Initialize locale; follow locale settings except
  // for numerical stuff where we need it to behave normally;
  // we no longer care about LC_CTYPE since we have our own.
  setlocale(LC_ALL,"");
  setlocale(LC_NUMERIC,"C");

  // Try locate display
#ifndef WIN32
  if((str=getenv("DISPLAY"))!=nullptr) dpy=str;
#endif

  //fxisconsole(argv[0]);

  // Parse out FOX args
  i=j=1;
  while(j<argc){

#ifndef WIN32

    // Start synchronized mode
    if(FXString::compare(argv[j],"-sync")==0){
      synchronize=true;
      j++;
      continue;
      }

    // Do not use X shared memory extension, even if available
    if(FXString::compare(argv[j],"-noshm")==0){
      shmi=false;
      shmp=false;
      j++;
      continue;
      }

    // Force use X shared memory extension, if available
    if(FXString::compare(argv[j],"-shm")==0){
      shmi=true;
      shmp=true;
      j++;
      continue;
      }

    // Alternative display
    if(FXString::compare(argv[j],"-display")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -display.\n",getClassName()); ::exit(1); }
      dpy=argv[j];
      j++;
      continue;
      }

    // Set input method
    if(FXString::compare(argv[j],"-im")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -im.\n",getClassName()); ::exit(1); }
      inputmethod=argv[j];
      j++;
      continue;
      }

    // Input style
    if(FXString::compare(argv[j],"-is")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -is.\n",getClassName()); ::exit(1); }
      istyle=argv[j];
      j++;
      continue;
      }

#endif

    // Set trace level
    if(FXString::compare(argv[j],"-tracelevel")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -tracelevel.\n",getClassName()); ::exit(1); }
      setTraceLevel(__strtoul(argv[j]));
      j++;
      continue;
      }

    // Set trace level
    if(FXString::compare(argv[j],"-tracetopics")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -tracetopics.\n",getClassName()); ::exit(1); }
      if(!setTraceTopics(argv[j++])){ fxwarning("%s:init: bad syntax: %s for -tracetopics.\n",getClassName(),argv[j-1]); ::exit(1); }
      continue;
      }

    // Set per-user configuration directory
    if(FXString::compare(argv[j],"-config")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -config.\n",getClassName()); ::exit(1); }
      registry.setUserDirectory(argv[j]);
      j++;
      continue;
      }

    // Set maximum number of colors
    if(FXString::compare(argv[j],"-maxcolors")==0){
      if(++j>=argc){ fxwarning("%s:init: missing argument for -maxcolors.\n",getClassName()); ::exit(1); }
      maxcols=__strtoul(argv[j]);
      if(maxcols<2 || maxcols>256){ fxwarning("%s::init: expected value between 2 and 256.\n",getClassName()); ::exit(1); }
      j++;
      continue;
      }

    // Copy program arguments
    argv[i++]=argv[j++];
    }

  // Adjust argment count
  argv[i]=nullptr;
  argc=i;

  // Remember arguments
  appArgv=argv;
  appArgc=argc;

  // Log message
  FXTRACE(TOPIC_CREATION,"%s::init %d.%d.%d\n",getClassName(),FOX_MAJOR,FOX_MINOR,FOX_LEVEL);

  // Read the registry
  registry.read();

  // Establish DPI awareness before any metrics are queried or windows are
  // created.  The policy is configurable in the registry as
  // [SETTINGS] dpiawareness=permonitorv2|permonitor|system|unaware.
  // Platforms without process DPI awareness return unknown and no system DPI.
  dpiawareness=fxplatformSetDPIAwareness(FXString(registry.readStringEntry("SETTINGS","dpiawareness","permonitorv2")));
  dpi=fxplatformGetSystemDPI();

#if defined(WIN32)

  // Get font face and metrics
  FXFontDesc fontdesc;
  getSystemFont(fontdesc);
  normalFont->setFontDesc(fontdesc);

  // Read colors from system
  baseColor=getSystemColor(COLOR_3DFACE);
  hiliteColor=getSystemColor(COLOR_3DHILIGHT);
  shadowColor=getSystemColor(COLOR_3DSHADOW);
  backColor=getSystemColor(COLOR_WINDOW);
  borderColor=getSystemColor(COLOR_WINDOWFRAME);
  foreColor=getSystemColor(COLOR_BTNTEXT);
  selforeColor=getSystemColor(COLOR_HIGHLIGHTTEXT);
  selbackColor=getSystemColor(COLOR_HIGHLIGHT);
  tipforeColor=getSystemColor(COLOR_INFOTEXT);
  tipbackColor=getSystemColor(COLOR_INFOBK);

  // Windows XP or later
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  GetVersionEx((OSVERSIONINFO*)&osvi);

  // Flat looking menus
//  if((osvi.dwMajorVersion>5) || (osvi.dwMajorVersion==5 && osvi.dwMinorVersion>=1)){	// FIXME
//    selMenuTextColor=getSystemColor(COLOR_HIGHLIGHT);
//    selMenuBackColor=getSystemColor(COLOR_MENUHILIGHT);
//    //selMenuBackColor=getSystemColor(COLOR_MENUTEXT);		// Menu text
//    //selMenuBackColor=getSystemColor(COLOR_MENU);		// Menu background
//    }
//  else{
    selMenuTextColor=getSystemColor(COLOR_HIGHLIGHTTEXT);
    selMenuBackColor=getSystemColor(COLOR_HIGHLIGHT);
//    }

  // Get wheel lines
  SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&wheelLines,0);

#endif

  // Parse font and change default font if set
  fontspec=registry.readStringEntry("SETTINGS","normalfont",nullptr);
  if(fontspec){
    normalFont->setFont(fontspec);
    }

  // Remember the unscaled UI font; the display scale is applied on top
  basefontdesc=normalFont->getFontDesc();

  // Load timing constants
  typingSpeed=registry.readLongEntry("SETTINGS","typingspeed",typingSpeed);
  clickSpeed=registry.readLongEntry("SETTINGS","clickspeed",clickSpeed);
  scrollSpeed=registry.readLongEntry("SETTINGS","scrollspeed",scrollSpeed);
  scrollDelay=registry.readLongEntry("SETTINGS","scrolldelay",scrollDelay);
  blinkSpeed=registry.readLongEntry("SETTINGS","blinkspeed",blinkSpeed);
  animSpeed=registry.readLongEntry("SETTINGS","animspeed",animSpeed);
  menuPause=registry.readLongEntry("SETTINGS","menupause",menuPause);
  toolTipPause=registry.readLongEntry("SETTINGS","tippause",toolTipPause);
  toolTipTime=registry.readLongEntry("SETTINGS","tiptime",toolTipTime);
  autoHideDelay=registry.readLongEntry("SETTINGS","autohide",autoHideDelay);

  // Load miscellaneous settings
  dragDelta=registry.readIntEntry("SETTINGS","dragdelta",dragDelta);
  wheelLines=registry.readIntEntry("SETTINGS","wheellines",wheelLines);
  scrollBarSize=registry.readIntEntry("SETTINGS","scrollbarsize",scrollBarSize);

  // Load colors settings
  borderColor=registry.readColorEntry("SETTINGS","bordercolor",borderColor);
  baseColor=registry.readColorEntry("SETTINGS","basecolor",baseColor);
  hiliteColor=registry.readColorEntry("SETTINGS","hilitecolor",hiliteColor);
  shadowColor=registry.readColorEntry("SETTINGS","shadowcolor",shadowColor);
  backColor=registry.readColorEntry("SETTINGS","backcolor",backColor);
  foreColor=registry.readColorEntry("SETTINGS","forecolor",foreColor);
  selforeColor=registry.readColorEntry("SETTINGS","selforecolor",selforeColor);
  selbackColor=registry.readColorEntry("SETTINGS","selbackcolor",selbackColor);
  tipforeColor=registry.readColorEntry("SETTINGS","tipforecolor",tipforeColor);
  tipbackColor=registry.readColorEntry("SETTINGS","tipbackcolor",tipbackColor);
  selMenuTextColor=registry.readColorEntry("SETTINGS","selmenutextcolor",selforeColor);
  selMenuBackColor=registry.readColorEntry("SETTINGS","selmenubackcolor",selbackColor);

  // Refresh theme from the palette
  refreshTheme();

  // Load theme selection: "classic", "dark", a .foxtheme file, or a GTK
  // theme directory/style sheet which is imported on the spot and cached.
  // "auto" or no selection at all follows the desktop's light/dark preference
  FXString themename=registry.readStringEntry("SETTINGS","theme",nullptr);
  if(themename.empty() || themename=="auto"){
    if(FXDesktopSettings::getColorScheme()==SCHEME_DARK){
      theme.setDark();
      setTheme(theme);
      }
    }
  else if(themename=="dark"){
    theme.setDark();
    setTheme(theme);
    }
  else if(themename!="classic"){
    FXTheme loaded;

    // A .foxtheme file is read directly; anything else is an external source
    // (GTK style sheet, gtkrc or KDE color scheme) which is imported and cached
    if(themename.right(9)==".foxtheme"){
      if(loaded.load(themename)){
        setTheme(loaded);
        }
      }
    else{
      if(!FXThemeCache::fetch(themename,loaded)){
        FXCSS css;
        if(css.load(themename)){
          loaded.setClassic();
          css.apply(loaded);
          FXThemeCache::store(themename,loaded);
          }
        }
      setTheme(loaded);
      }
    }

  // Display scale: "auto" follows the monitor DPI, a number is a percentage.
  // The registry value overrides the scale defined by a theme file.
  const FXchar *scalespec=registry.readStringEntry("display","scale",nullptr);
#if defined(WIN32)
  if(scalespec==nullptr) scalespec="auto";
#endif
  if(scalespec){
    if(FXString::comparecase(scalespec,"auto")==0){
      dpiautoscale=true;
      dpifactor=1.0;
      setThemeScale(dpi?FXdouble(dpi)/96.0:1.0);
      }
    else{
      dpiautoscale=false;
      dpifactor=1.0;
      setThemeScale(FXdouble(FXString(scalespec).toInt())/100.0);
      }
    }

  // Maximum number of colors to allocate
  maxcolors=registry.readUIntEntry("SETTINGS","maxcolors",maxcolors);

  // Command line takes precedence
  if(maxcols) maxcolors=maxcols;

  // Read input method style if not set from command line
  inputstyle=registry.readStringEntry("SETTINGS","inputstyle",inputstyle);

  // Command line takes precedence
  if(istyle) inputstyle=istyle;

  // Set maximum number of colors in default visual to be nice to legacy
  // Motif applications which don't handle color allocation gracefully.
  getRootWindow()->getVisual()->setMaxColors(maxcolors);

  // Open display; this also applies any system-defined settings.
  // We only override settings if NOT set from our own registry, since
  // it is possible that we don't open the display until later, so we
  // can't simply overwrite system-defined settings after opening the
  // display here.
  if(connect){
    if(!openDisplay(dpy)){
      fxwarning("%s::openDisplay: unable to open display %s\n",getClassName(),dpy);
      ::exit(1);
      }
    }
  }


// Exit application and close display
void FXApp::exit(FXint code){
  FXTRACE(TOPIC_CREATION,"%s::exit\n",getClassName());

  // Close the display
  closeDisplay();

  // Write the registry
  registry.write();

  // Exit the program
  stop(code);
  }


// Create application's windows
void FXApp::create(){
  FXTRACE(TOPIC_CREATION,"%s::create\n",getClassName());
  FXint i;

  // Create visuals
  monoVisual->create();
  defaultVisual->create();

  // Create default font
  normalFont->create();
  stockFont->create();

  // Create the fonts of the theme's font roles
  for(i=0; i<FONT_LAST; i++){
    if(roleFonts[i]) roleFonts[i]->create();
    }

  // Create wait cursor
  waitCursor->create();

  // Create stock cursors
  cursor[DEF_ARROW_CURSOR]->create();
  cursor[DEF_RARROW_CURSOR]->create();
  cursor[DEF_TEXT_CURSOR]->create();
  cursor[DEF_HSPLIT_CURSOR]->create();
  cursor[DEF_VSPLIT_CURSOR]->create();
  cursor[DEF_XSPLIT_CURSOR]->create();
  cursor[DEF_SWATCH_CURSOR]->create();
  cursor[DEF_MOVE_CURSOR]->create();
  cursor[DEF_DRAGH_CURSOR]->create();
  cursor[DEF_DRAGV_CURSOR]->create();
  cursor[DEF_DRAGTL_CURSOR]->create();
  cursor[DEF_DRAGTR_CURSOR]->create();
  cursor[DEF_DNDSTOP_CURSOR]->create();
  cursor[DEF_DNDASK_CURSOR]->create();
  cursor[DEF_DNDCOPY_CURSOR]->create();
  cursor[DEF_DNDMOVE_CURSOR]->create();
  cursor[DEF_DNDLINK_CURSOR]->create();
  cursor[DEF_CROSSHAIR_CURSOR]->create();
  cursor[DEF_CORNERNE_CURSOR]->create();
  cursor[DEF_CORNERNW_CURSOR]->create();
  cursor[DEF_CORNERSE_CURSOR]->create();
  cursor[DEF_CORNERSW_CURSOR]->create();
  cursor[DEF_HELP_CURSOR]->create();
  cursor[DEF_HAND_CURSOR]->create();
  cursor[DEF_ROTATE_CURSOR]->create();
  cursor[DEF_BLANK_CURSOR]->create();
  cursor[DEF_WAIT_CURSOR]->create();

  // Create all windows
  root->create();
  }


// Detach application's windows
void FXApp::detach(){
  FXTRACE(TOPIC_CREATION,"%s::detach\n",getClassName());
  FXint i;

  root->detach();

  // Detach default font
  normalFont->detach();
  stockFont->detach();

  // Detach the fonts of the theme's font roles
  for(i=0; i<FONT_LAST; i++){
    if(roleFonts[i]) roleFonts[i]->detach();
    }

  // Detach wait cursor
  waitCursor->detach();

  // Detach stock cursors
  cursor[DEF_ARROW_CURSOR]->detach();
  cursor[DEF_RARROW_CURSOR]->detach();
  cursor[DEF_TEXT_CURSOR]->detach();
  cursor[DEF_HSPLIT_CURSOR]->detach();
  cursor[DEF_VSPLIT_CURSOR]->detach();
  cursor[DEF_XSPLIT_CURSOR]->detach();
  cursor[DEF_SWATCH_CURSOR]->detach();
  cursor[DEF_MOVE_CURSOR]->detach();
  cursor[DEF_DRAGH_CURSOR]->detach();
  cursor[DEF_DRAGV_CURSOR]->detach();
  cursor[DEF_DRAGTL_CURSOR]->detach();
  cursor[DEF_DRAGTR_CURSOR]->detach();
  cursor[DEF_DNDSTOP_CURSOR]->detach();
  cursor[DEF_DNDASK_CURSOR]->detach();
  cursor[DEF_DNDCOPY_CURSOR]->detach();
  cursor[DEF_DNDMOVE_CURSOR]->detach();
  cursor[DEF_DNDLINK_CURSOR]->detach();
  cursor[DEF_CROSSHAIR_CURSOR]->detach();
  cursor[DEF_CORNERNE_CURSOR]->detach();
  cursor[DEF_CORNERNW_CURSOR]->detach();
  cursor[DEF_CORNERSE_CURSOR]->detach();
  cursor[DEF_CORNERSW_CURSOR]->detach();
  cursor[DEF_HELP_CURSOR]->detach();
  cursor[DEF_HAND_CURSOR]->detach();
  cursor[DEF_ROTATE_CURSOR]->detach();
  cursor[DEF_BLANK_CURSOR]->detach();
  cursor[DEF_WAIT_CURSOR]->detach();

  // Detach visuals
  monoVisual->detach();
  defaultVisual->detach();
  }


// Destroy application's windows
void FXApp::destroy(){
  FXTRACE(TOPIC_CREATION,"%s::destroy\n",getClassName());

  root->destroy();

  // Destroy default font
  normalFont->destroy();
  stockFont->destroy();

  // Destroy the fonts of the theme's font roles
  releaseRoleFonts();

  // Destroy wait cursor
  waitCursor->destroy();

  // Destroy stock cursors
  cursor[DEF_ARROW_CURSOR]->destroy();
  cursor[DEF_RARROW_CURSOR]->destroy();
  cursor[DEF_TEXT_CURSOR]->destroy();
  cursor[DEF_HSPLIT_CURSOR]->destroy();
  cursor[DEF_VSPLIT_CURSOR]->destroy();
  cursor[DEF_XSPLIT_CURSOR]->destroy();
  cursor[DEF_SWATCH_CURSOR]->destroy();
  cursor[DEF_MOVE_CURSOR]->destroy();
  cursor[DEF_DRAGH_CURSOR]->destroy();
  cursor[DEF_DRAGV_CURSOR]->destroy();
  cursor[DEF_DRAGTL_CURSOR]->destroy();
  cursor[DEF_DRAGTR_CURSOR]->destroy();
  cursor[DEF_DNDSTOP_CURSOR]->destroy();
  cursor[DEF_DNDASK_CURSOR]->destroy();
  cursor[DEF_DNDCOPY_CURSOR]->destroy();
  cursor[DEF_DNDMOVE_CURSOR]->destroy();
  cursor[DEF_DNDLINK_CURSOR]->destroy();
  cursor[DEF_CROSSHAIR_CURSOR]->destroy();
  cursor[DEF_CORNERNE_CURSOR]->destroy();
  cursor[DEF_CORNERNW_CURSOR]->destroy();
  cursor[DEF_CORNERSE_CURSOR]->destroy();
  cursor[DEF_CORNERSW_CURSOR]->destroy();
  cursor[DEF_HELP_CURSOR]->destroy();
  cursor[DEF_HAND_CURSOR]->destroy();
  cursor[DEF_ROTATE_CURSOR]->destroy();
  cursor[DEF_BLANK_CURSOR]->destroy();
  cursor[DEF_WAIT_CURSOR]->destroy();

  // Destroy visuals
  monoVisual->destroy();
  defaultVisual->destroy();
  }


// Fonts realized for a monitor with a different DPI
struct FXApp::FXFontSet {
  FXint      dpi;                       // Monitor DPI
  FXFont    *normal;                    // Normal font for this DPI
  FXFont    *roles[FONT_LAST];          // Role fonts for this DPI
  FXFontSet *next;                      // Next font set
  };


// Destroy and forget the cached theme role fonts
void FXApp::releaseRoleFonts(){
  FXFontSet *set,*nextset;
  FXint i;
  for(i=0; i<FONT_LAST; i++){
    if(roleFonts[i] && roleFonts[i]!=normalFont){
      roleFonts[i]->destroy();
      delete roleFonts[i];
      }
    roleFonts[i]=nullptr;
    }
  for(set=fontsets; set; set=nextset){
    nextset=set->next;
    if(set->normal){ set->normal->destroy(); delete set->normal; }
    for(i=0; i<FONT_LAST; i++){
      if(set->roles[i]){ set->roles[i]->destroy(); delete set->roles[i]; }
      }
    delete set;
    }
  fontsets=nullptr;
  }


// Return font for the given theme font role; inherits the normal font when the theme leaves the role open
FXFont* FXApp::getRoleFont(FXint role){
  const FXFontDesc& desc=theme.font(role);

  // Unknown role or a role the theme did not fill in
  if(role<0 || FONT_LAST<=role || desc.face[0]==0) return normalFont;

  // Create the font the first time it is asked for
  if(!roleFonts[role]){
    FXFontDesc fontdesc=desc;
    FXdouble factor=getFontScale();
    if(fontdesc.size) fontdesc.size=FXint(FXdouble(fontdesc.size)*factor+0.5);
    roleFonts[role]=new FXFont(this,fontdesc);
    if(isInitialized()) roleFonts[role]->create();
    }
  return roleFonts[role];
  }


// Return the role font realized for a given DPI
FXFont* FXApp::getRoleFont(FXint role,FXint dpivalue){
  FXFontDesc desc;
  FXFontSet *set;
  FXdouble factor;
  FXint i;
  if(dpivalue<=0 || dpivalue==dpi) return getRoleFont(role);
  desc=theme.font(role);
  if(role<0 || FONT_LAST<=role || desc.face[0]==0) return getNormalFont(dpivalue);
  for(set=fontsets; set && set->dpi!=dpivalue; set=set->next){}
  if(!set){
    set=new FXFontSet;
    set->dpi=dpivalue;
    set->normal=nullptr;
    for(i=0; i<FONT_LAST; i++) set->roles[i]=nullptr;
    set->next=fontsets;
    fontsets=set;
    }
  if(!set->roles[role]){
    factor=theme.getScale()/(FXdouble(dpivalue)/96.0);
    if(desc.size) desc.size=FXint(FXdouble(desc.size)*factor+0.5);
    set->roles[role]=new FXFont(this,desc);
    set->roles[role]->setResolution(dpivalue);
    if(isInitialized()) set->roles[role]->create();
    }
  return set->roles[role];
  }


// Return the normal font realized for a given DPI
FXFont* FXApp::getNormalFont(FXint dpivalue){
  FXFontDesc desc;
  FXFontSet *set;
  FXdouble factor;
  FXint i;
  if(dpivalue<=0 || dpivalue==dpi) return normalFont;
  for(set=fontsets; set && set->dpi!=dpivalue; set=set->next){}
  if(!set){
    set=new FXFontSet;
    set->dpi=dpivalue;
    set->normal=nullptr;
    for(i=0; i<FONT_LAST; i++) set->roles[i]=nullptr;
    set->next=fontsets;
    fontsets=set;
    }
  if(!set->normal){
    desc=basefontdesc;
    factor=theme.getScale()/(FXdouble(dpivalue)/96.0);
    if(desc.size) desc.size=FXint(FXdouble(desc.size)*factor+0.5);
    set->normal=new FXFont(this,desc);
    set->normal->setResolution(dpivalue);
    if(isInitialized()) set->normal->create();
    }
  return set->normal;
  }


// Generates SEL_LEAVE event when cursor is not inside ancestor of
// cursorWindow anymore. Note that cursor may still be phyisally inside
// the borders of the cursorWindow's shell but is considered outside if
// another window obscures it.  This mechanism replaces the dysfunctional
// and unreliable TrackMouseEvent method.
long FXApp::onCmdHover(FXObject*,FXSelector,void*){
  FXint x,y; FXuint buttons;
  FXWindow *window;
  if(!mouseGrabWindow && cursorWindow && cursorWindow!=root){
    root->getCursorPosition(x,y,buttons);
    if((window=findWindowAt(x,y))==nullptr || !window->getShell()->containsChild(cursorWindow)){
      event.type=SEL_LEAVE;
      event.root_x=x;
      event.root_y=y;
      leaveWindow(cursorWindow,root);
      return 0;
      }
    }
  addTimeout(this,ID_HOVER,200000000);
  return 0;
  }




// Handle quit
long FXApp::onCmdQuit(FXObject*,FXSelector,void*){
  exit(0);
  return 1;
  }


/*******************************************************************************/

// Dump widgets
long FXApp::onCmdDump(FXObject*,FXSelector,void*){
  dumpWidgets();
  return 1;
  }


// Dump widget information
void FXApp::dumpWidgets() const {
  FXWindow *w=getRootWindow();
  FXObject *t;
  FXint lev=0;
  FXchar s;
  while(w){
    t=w->getTarget();
    s=w->shown()?'+':'-';
    if(t){
      fxmessage("%*c%s (%p): wk=%d id=%lu target=%s (%p) sel=%d x=%d y=%d w=%d h=%d\n",lev*2,s,w->getClassName(),w,w->getKey(),w->id(),t->getClassName(),t,w->getSelector(),w->getX(),w->getY(),w->getWidth(),w->getHeight());
      }
    else{
      fxmessage("%*c%s (%p): wk=%d id=%lu x=%d y=%d w=%d h=%d\n",lev*2,s,w->getClassName(),w,w->getKey(),w->id(),w->getX(),w->getY(),w->getWidth(),w->getHeight());
      }
    if(w->getFirst()){
      w=w->getFirst();
      lev++;
      continue;
      }
    while(!w->getNext() && w->getParent()){
      w=w->getParent();
      lev--;
      if(lev==1) fxmessage("\n");
      }
    w=w->getNext();
    }
  }


// Save to stream
void FXApp::save(FXStream& store) const {
  FXObject::save(store);
  store << clickSpeed;
  store << animSpeed;
  store << scrollSpeed;
  store << blinkSpeed;
  store << menuPause;
  store << toolTipPause;
  store << toolTipTime;
  store << autoHideDelay;
  store << dragDelta;
  store << borderColor;
  store << baseColor;
  store << hiliteColor;
  store << shadowColor;
  store << backColor;
  store << foreColor;
  store << selforeColor;
  store << selbackColor;
  store << tipforeColor;
  store << tipbackColor;
  store << selMenuTextColor;
  store << selMenuBackColor;
  }


// Load from stream
void FXApp::load(FXStream& store){
  FXObject::load(store);
  store >> clickSpeed;
  store >> animSpeed;
  store >> scrollSpeed;
  store >> blinkSpeed;
  store >> menuPause;
  store >> toolTipPause;
  store >> toolTipTime;
  store >> autoHideDelay;
  store >> dragDelta;
  store >> borderColor;
  store >> baseColor;
  store >> hiliteColor;
  store >> shadowColor;
  store >> backColor;
  store >> foreColor;
  store >> selforeColor;
  store >> selbackColor;
  store >> tipforeColor;
  store >> tipbackColor;
  store >> selMenuTextColor;
  store >> selMenuBackColor;
  }


// Change typing speed
void FXApp::setTypingSpeed(FXTime speed){
  typingSpeed=speed;
  }

// Change double-click speed
void FXApp::setClickSpeed(FXTime speed){
  clickSpeed=speed;
  }

// Change scroll speed
void FXApp::setScrollSpeed(FXTime speed){
  scrollSpeed=speed;
  }

// Change scroll delay
void FXApp::setScrollDelay(FXTime delay){
  scrollDelay=delay;
  }

// Change cursor blink speed
void FXApp::setBlinkSpeed(FXTime speed){
  blinkSpeed=speed;
  }

// Change animation speed
void FXApp::setAnimSpeed(FXTime speed){
  animSpeed=speed;
  }

// Change menu popup delay
void FXApp::setMenuPause(FXTime pause){
  menuPause=pause;
  }

// Change tooltip popup pause
void FXApp::setToolTipPause(FXTime pause){
  toolTipPause=pause;
  }

// Change tooltip visibility time
void FXApp::setToolTipTime(FXTime time){
  toolTipTime=time;
  }


// Change autohide delay time
void FXApp::setAutoHideDelay(FXTime time){
  autoHideDelay=time;
  }


// Change drag delta
void FXApp::setDragDelta(FXint delta){
  dragDelta=delta;
  }

// Change mouse wheel lines
void FXApp::setWheelLines(FXint lines){
  wheelLines=lines;
  }

// Change scroll bar size
void FXApp::setScrollBarSize(FXint size){
  scrollBarSize=size;
  }


// Change the display scale explicitly
void FXApp::setDisplayScale(FXdouble scale){
  dpiautoscale=false;
  dpifactor=1.0;
  setThemeScale(scale);
  }


// Follow the DPI of the monitor, multiplied by a user factor
void FXApp::setDisplayScaleFactor(FXdouble factor){
  FXint monitor=dpicurrent?dpicurrent:dpi;
  dpiautoscale=true;
  dpifactor=factor;
  setThemeScale(dpifactor*(monitor?FXdouble(monitor)/96.0:1.0));
  }


// Follow the DPI for the display scale
void FXApp::setDisplayScaleAuto(){
  FXint monitor=dpicurrent?dpicurrent:dpi;
  dpiautoscale=true;
  dpifactor=1.0;
  setThemeScale(monitor?FXdouble(monitor)/96.0:1.0);
  }


// Change theme; updates the default palette colors
void FXApp::setTheme(const FXTheme& thm){
  FXbool relayout=false;
  FXint m;

  // Relayout when metrics or font roles changed
  for(m=0; m<METRIC_LAST; m++){
    if(theme.metric(m)!=thm.metric(m)) relayout=true;
    }
  for(m=0; m<FONT_LAST; m++){
    if(theme.font(m).getFont()!=thm.font(m).getFont()) relayout=true;
    }

  // The cached role fonts belong to the old theme
  releaseRoleFonts();

  theme=thm;
  applyFontScale();
  borderColor=theme.color(ROLE_BORDER);
  baseColor=theme.color(ROLE_SURFACE);
  hiliteColor=theme.color(ROLE_SURFACE_HILITE);
  shadowColor=theme.color(ROLE_SURFACE_SHADOW);
  backColor=theme.color(ROLE_WINDOW_BG);
  foreColor=theme.color(ROLE_TEXT);
  selforeColor=theme.color(ROLE_SELECTION_TEXT);
  selbackColor=theme.color(ROLE_SELECTION_BG);
  tipforeColor=theme.color(ROLE_TOOLTIP_TEXT);
  tipbackColor=theme.color(ROLE_TOOLTIP_BG);
  selMenuTextColor=theme.color(ROLE_MENU_SEL_TEXT);
  selMenuBackColor=theme.color(ROLE_MENU_SEL_BG);

  themeChanged();
  if(relayout){
    FXWindow *win;
    for(win=(root?root->getFirst():nullptr); win; win=win->getNext()){
      win->recalc();
      }
    }
  }


// Recursively re-apply the theme and repaint each window
static void applyThemeToTree(FXWindow* win,FXTheme* theme){
  while(win){
    win->applyTheme(theme);
    win->update();
    if(win->getFirst()) applyThemeToTree(win->getFirst(),theme);
    win=win->getNext();
    }
  }


// Repaint all windows after a theme change
void FXApp::themeChanged(){
  style.invalidate();
  if(root) applyThemeToTree(root->getFirst(),&theme);
  }


// Refresh theme colors from the default palette
void FXApp::refreshTheme(){
  theme.setColor(ROLE_BORDER,borderColor);
  theme.setColor(ROLE_SURFACE,baseColor);
  theme.setColor(ROLE_SURFACE_HILITE,hiliteColor);
  theme.setColor(ROLE_SURFACE_SHADOW,shadowColor);
  theme.setColor(ROLE_WINDOW_BG,backColor);
  theme.setColor(ROLE_TEXT,foreColor);
  theme.setColor(ROLE_SELECTION_TEXT,selforeColor);
  theme.setColor(ROLE_SELECTION_BG,selbackColor);
  theme.setColor(ROLE_TOOLTIP_TEXT,tipforeColor);
  theme.setColor(ROLE_TOOLTIP_BG,tipbackColor);
  theme.setColor(ROLE_MENU_SEL_TEXT,selMenuTextColor);
  theme.setColor(ROLE_MENU_SEL_BG,selMenuBackColor);
  theme.updateDerived();
  }


// Change border color
void FXApp::setBorderColor(FXColor color){
  borderColor=color;
  theme.setColor(ROLE_BORDER,color);
  }

// Change base color
void FXApp::setBaseColor(FXColor color){
  baseColor=color;
  theme.setColor(ROLE_SURFACE,color);
  theme.updateDerived();
  }

// Change highlight color
void FXApp::setHiliteColor(FXColor color){
  hiliteColor=color;
  theme.setColor(ROLE_SURFACE_HILITE,color);
  }

// Change shadow color
void FXApp::setShadowColor(FXColor color){
  shadowColor=color;
  theme.setColor(ROLE_SURFACE_SHADOW,color);
  }

// Change background color
void FXApp::setBackColor(FXColor color){
  backColor=color;
  theme.setColor(ROLE_WINDOW_BG,color);
  }

// Change foreground color
void FXApp::setForeColor(FXColor color){
  foreColor=color;
  theme.setColor(ROLE_TEXT,color);
  theme.updateDerived();
  }

// Change selected foreground color
void FXApp::setSelforeColor(FXColor color){
  selforeColor=color;
  theme.setColor(ROLE_SELECTION_TEXT,color);
  }

// Change selected background color
void FXApp::setSelbackColor(FXColor color){
  selbackColor=color;
  theme.setColor(ROLE_SELECTION_BG,color);
  theme.updateDerived();
  }

// Change tip foreground color
void FXApp::setTipforeColor(FXColor color){
  tipforeColor=color;
  theme.setColor(ROLE_TOOLTIP_TEXT,color);
  }

// Change tip background color
void FXApp::setTipbackColor(FXColor color){
  tipbackColor=color;
  theme.setColor(ROLE_TOOLTIP_BG,color);
  }


// Change selected menu text color
void FXApp::setSelMenuTextColor(FXColor color){
  selMenuTextColor=color;
  theme.setColor(ROLE_MENU_SEL_TEXT,color);
  }


// Change selected menu back color
void FXApp::setSelMenuBackColor(FXColor color){
  selMenuBackColor=color;
  theme.setColor(ROLE_MENU_SEL_BG,color);
  }


// Virtual destructor
FXApp::~FXApp(){
  FXTRACE(TOPIC_CONSTRUCT,"FXApp::~FXApp %p\n",this);
  FXRepaint *r;
  FXTimer *t;
  FXChore *c;

  // Release style resources while the display is still open
  style.invalidate();

  // Release the theme role fonts too
  releaseRoleFonts();

  // Close display
  closeDisplay();

  // Delete root window & its children
  delete root;

  // Delete visuals
  delete defaultVisual;
  delete monoVisual;

  // Delete stock font only
  delete stockFont;

  // Delete cursors
  delete cursor[DEF_ARROW_CURSOR];
  delete cursor[DEF_RARROW_CURSOR];
  delete cursor[DEF_TEXT_CURSOR];
  delete cursor[DEF_HSPLIT_CURSOR];
  delete cursor[DEF_VSPLIT_CURSOR];
  delete cursor[DEF_XSPLIT_CURSOR];
  delete cursor[DEF_SWATCH_CURSOR];
  delete cursor[DEF_MOVE_CURSOR];
  delete cursor[DEF_DRAGH_CURSOR];
  delete cursor[DEF_DRAGV_CURSOR];
  delete cursor[DEF_DRAGTR_CURSOR];
  delete cursor[DEF_DRAGTL_CURSOR];
  delete cursor[DEF_DNDSTOP_CURSOR];
  delete cursor[DEF_DNDASK_CURSOR];
  delete cursor[DEF_DNDCOPY_CURSOR];
  delete cursor[DEF_DNDMOVE_CURSOR];
  delete cursor[DEF_DNDLINK_CURSOR];
  delete cursor[DEF_CROSSHAIR_CURSOR];
  delete cursor[DEF_CORNERNE_CURSOR];
  delete cursor[DEF_CORNERNW_CURSOR];
  delete cursor[DEF_CORNERSE_CURSOR];
  delete cursor[DEF_CORNERSW_CURSOR];
  delete cursor[DEF_HELP_CURSOR];
  delete cursor[DEF_HAND_CURSOR];
  delete cursor[DEF_ROTATE_CURSOR];
  delete cursor[DEF_BLANK_CURSOR];
  delete cursor[DEF_WAIT_CURSOR];

  // Delete translator
  delete translator;

  // Free inputs and handles
  freeElms(inputs);
  freeElms(handles);

  // Free signals list
  freeElms(signals);

  // Free left-over dde data
  freeElms(ddeData);
  ddeSize=0;

  // Free left-over selection type data
#if defined(WIN32)
  freeElms(xselTypeList);
  freeElms(ddeTypeList);
#else
  freeElms(xselTypeList);
  freeElms(xcbTypeList);
  freeElms(xdndTypeList);
  freeElms(ddeTypeList);
#endif

  // Remove outstanding repaints
  while(repaints){
    r=repaints;
    repaints=repaints->next;
    delete r;
    }

  // Free recycled repaint records
  while(repaintrecs){
    r=repaintrecs;
    repaintrecs=repaintrecs->next;
    delete r;
    }

  // Kill outstanding timers
  while(timers){
    t=timers;
    timers=timers->next;
    delete t;
    }

  // Free recycled timer records
  while(timerrecs){
    t=timerrecs;
    timerrecs=timerrecs->next;
    delete t;
    }

  // Kill outstanding chores
  while(chores){
    c=chores;
    chores=chores->next;
    delete c;
    }

  // Free recycled chore records
  while(chorerecs){
    c=chorerecs;
    chorerecs=chorerecs->next;
    delete c;
    }

  // Thrash dangling pointers
  root=(FXRootWindow*)-1L;
  defaultVisual=(FXVisual*)-1L;
  monoVisual=(FXVisual*)-1L;
  normalFont=(FXFont*)-1L;
  stockFont=(FXFont*)-1L;
  waitCursor=(FXCursor*)-1L;
  signals=(FXSignal*)-1L;

  // Zap cursors
  cursor[DEF_ARROW_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_RARROW_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_TEXT_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_HSPLIT_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_VSPLIT_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_XSPLIT_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_SWATCH_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_MOVE_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DRAGH_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DRAGV_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DRAGTL_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DRAGTR_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DNDSTOP_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DNDASK_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DNDCOPY_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DNDMOVE_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_DNDLINK_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_CROSSHAIR_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_CORNERNE_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_CORNERNW_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_CORNERSE_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_CORNERSW_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_HELP_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_HAND_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_ROTATE_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_BLANK_CURSOR]=(FXCursor*)-1L;
  cursor[DEF_WAIT_CURSOR]=(FXCursor*)-1L;

  // Do this last
  app=nullptr;
  }

}
