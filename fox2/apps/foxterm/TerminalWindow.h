/********************************************************************************
*                                                                               *
*                         T e r m i n a l   W i n d o w                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
* You should have received a copy of the GNU General Public License             *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#ifndef TERMINALWINDOW_H
#define TERMINALWINDOW_H

#ifndef FXMAINWINDOW_H
#include "FXMainWindow.h"
#endif

#ifndef FXTERMINAL_H
#include "FXTerminal.h"
#endif

namespace FX {
class TerminalTabBar;
}


/**
* One native terminal window. Phase 4 uses a single tab; the tab bar already
* owns the strip that Phase 5 grows into a list of sessions.
*/
class TerminalWindow : public FXMainWindow {
  FXDECLARE(TerminalWindow)
protected:
  FXVerticalFrame *contents;
  FXHorizontalFrame *tabrow;
  TerminalTabBar   *tabbar;
  FXButton         *newbutton;
  FXMenuButton     *profilebutton;
  FXMenuPane       *profilemenu;
  FXMenuButton     *menubutton;
  FXMenuPane       *mainmenu;
  FXTerminal       *terminal;
  FXLabel          *status;
  FXTerminalLaunch  launch;
protected:
  TerminalWindow(){}
  void saveGeometry();
public:

  /// Construct a terminal window for one launch description
  TerminalWindow(FXApp* a,const FXTerminalLaunch& launch);

  /// Create and start the child
  virtual void create() override;

  /// Update the title from the terminal
  long onCmdTitle(FXObject*,FXSelector,void*);

  /// Ring the application bell
  long onCmdBell(FXObject*,FXSelector,void*);

  /// Show process exit state
  long onCmdExited(FXObject*,FXSelector,void*);

  /// Report a blocked clipboard request
  long onCmdClipboard(FXObject*,FXSelector,void*);

  /// Open another terminal window
  long onCmdNewWindow(FXObject*,FXSelector,void*);

  /// Copy the selection
  long onCmdCopy(FXObject*,FXSelector,void*);

  /// Paste the clipboard
  long onCmdPaste(FXObject*,FXSelector,void*);

  /// Save geometry and quit
  long onCmdQuit(FXObject*,FXSelector,void*);

  /// Show About
  long onCmdAbout(FXObject*,FXSelector,void*);
public:
  enum {
    ID_TITLE=FXMainWindow::ID_LAST,
    ID_BELL,
    ID_EXITED,
    ID_CLIPBOARD,
    ID_NEW,
    ID_COPY,
    ID_PASTE,
    ID_ABOUT,
    ID_QUIT,
    ID_LAST
    };
public:

  /// Destructor
  virtual ~TerminalWindow();
  };

#endif
