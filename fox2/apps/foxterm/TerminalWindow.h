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

#include "Profiles.h"

#include <vector>

namespace FX {
class TerminalTabBar;
}


/// State for one tab
struct TerminalTab {
  FXTerminal       *view;
  FXTerminalLaunch  launch;
  FXString          profileid;
  FXString          title;
  FXString          directory;
  FXbool            exited;
  FXint             exitcode;
  };


/**
* One native terminal window. The switcher owns the live terminal views; the
* tab bar owns only titles and selection. Hidden tabs keep their PTY running.
*/
class TerminalWindow : public FXMainWindow {
  FXDECLARE(TerminalWindow)
protected:
  FXVerticalFrame *contents;
  FXHorizontalFrame *tabrow;
  FX::TerminalTabBar *tabbar;
  FXButton         *newbutton;
  FXMenuButton     *profilebutton;
  FXMenuPane       *profilemenu;
  FXMenuButton     *menubutton;
  FXMenuPane       *mainmenu;
  FXMenuPane       *contextmenu;
  FXMenuPane       *encodingmenu;
  FXMenuPane       *tabmenu;
  FXMenuPane       *opacitymenu;
  FXCanvas         *rule;
  FXSwitcher       *switcher;
  FXLabel          *status;
  FXHorizontalFrame *findbar;
  FXTextField      *findfield;
  FXLabel          *findstatus;
  FXTerminalLaunch  launch;
  std::vector<TerminalTab> tabs;
  Profiles          profiles;
  FXString          activeprofile;
  FXint             current;
  FXint             contexttab;        // tab the tab strip menu belongs to
  FXbool            autostart;
protected:
  TerminalWindow(){}
  void saveGeometry();
  FXint findTab(FXObject* view) const;
  /// Add a tab and start it; the launch is copied because the vector of tabs
  /// reallocates while the new tab is being started.  An empty encoding takes
  /// the one of the profile
  FXbool addTab(FXTerminalLaunch l,FXString profileid=FXString::null,const FXString& encoding=FXString::null);
  void selectTab(FXint index);
  void closeTab(FXint index,FXbool force=false);
  void updateTabTitle(FXint index);
  /// Move a tab into its own window; refused when it is the only tab
  void detachTab(FXint index);
  void rebuildProfileMenu();
  void terminateTabs();
  void showStatus(const FXString& text,FXbool transient=true);
  void hideStatus();
  const TerminalProfile& tabProfile(FXint index) const;
  FXbool confirmCloseTab(FXint index);
  FXbool confirmQuit();
public:

  /// Construct a terminal window; autostart adds the first tab in create()
  TerminalWindow(FXApp* a,const FXTerminalLaunch& launch,const FXString& active=FXString::null,FXbool autostart=true);

  /// Create the first tab and show the window
  virtual void create() override;

  /// Update one tab title
  long onCmdTitle(FXObject*,FXSelector,void*);

  /// Ring the bell
  long onCmdBell(FXObject*,FXSelector,void*);

  /// Show process exit state, and apply the close-on-exit policy
  long onCmdExited(FXObject*,FXSelector,void*);

  /// Close a tab whose child exited; deferred out of the widget callback
  long onChoreCloseTab(FXObject*,FXSelector,void*);

  /// Quit without the close confirmation, for a termination signal
  long onSignalQuit(FXObject*,FXSelector,void*);

  /// Paint the hairline between the tab strip and the grid
  long onPaintRule(FXObject*,FXSelector,void*);

  /// Hide the transient status line
  long onStatusTimeout(FXObject*,FXSelector,void*);

  /// Report a blocked clipboard request
  long onCmdClipboard(FXObject*,FXSelector,void*);

  /// Select a tab
  long onCmdTab(FXObject*,FXSelector,void*);

  /// Close a tab by index
  long onCmdCloseTab(FXObject*,FXSelector,void*);

  /// Close the active tab
  long onCmdCloseCurrent(FXObject*,FXSelector,void*);

  /// Open a new tab
  long onCmdNewTab(FXObject*,FXSelector,void*);

  /// Open another window
  long onCmdNewWindow(FXObject*,FXSelector,void*);

  /// Move the active tab into a new window
  long onCmdDetach(FXObject*,FXSelector,void*);

  /// Copy the active selection
  long onCmdCopy(FXObject*,FXSelector,void*);

  /// Paste into the active tab
  long onCmdPaste(FXObject*,FXSelector,void*);

  /// Save geometry and quit
  long onCmdQuit(FXObject*,FXSelector,void*);

  /// Show the find bar
  long onCmdFind(FXObject*,FXSelector,void*);

  /// Find the next match
  long onCmdFindNext(FXObject*,FXSelector,void*);

  /// Find the previous match
  long onCmdFindPrev(FXObject*,FXSelector,void*);

  /// Hide the find bar
  long onCmdFindClose(FXObject*,FXSelector,void*);

  /// Select the next tab
  long onCmdNextTab(FXObject*,FXSelector,void*);

  /// Select the previous tab
  long onCmdPrevTab(FXObject*,FXSelector,void*);

  /// Create a tab from a profile
  long onCmdProfile(FXObject*,FXSelector,void*);

  /// Edit the active profile
  long onCmdSettings(FXObject*,FXSelector,void*);

  /// Apply whole-window opacity
  long onCmdOpacity(FXObject*,FXSelector,void*);

  /// Show the context menu of the terminal view, posted by the widget
  long onCmdContextMenu(FXObject*,FXSelector,void*);

  /// Show the tab strip menu, posted by the strip
  long onCmdTabMenu(FXObject*,FXSelector,void*);

  /// Select the whole retained screen
  long onCmdSelectAll(FXObject*,FXSelector,void*);

  /// Drop the scrollback of the active tab
  long onCmdClearScrollback(FXObject*,FXSelector,void*);

  /// Reset the emulator of the active tab
  long onCmdResetTerminal(FXObject*,FXSelector,void*);

  /// Duplicate the tab the strip menu was posted for
  long onCmdDupTab(FXObject*,FXSelector,void*);

  /// Rename the tab the strip menu was posted for
  long onCmdRenameTab(FXObject*,FXSelector,void*);

  /// Detach the tab the strip menu was posted for
  long onCmdDetachTabMenu(FXObject*,FXSelector,void*);

  /// Close the tab the strip menu was posted for
  long onCmdCloseMenuTab(FXObject*,FXSelector,void*);

  /// Close every other tab of this window
  long onCmdCloseOthers(FXObject*,FXSelector,void*);

  /// Enable Copy only with a selection
  long onUpdCopy(FXObject*,FXSelector,void*);

  /// Enable Paste only while the child runs
  long onUpdPaste(FXObject*,FXSelector,void*);

  /// Enable entries that need a live tab
  long onUpdHasTab(FXObject*,FXSelector,void*);

  /// Enable Detach Tab only when a second tab stays behind
  long onUpdDetach(FXObject*,FXSelector,void*);

  /// Enable Clear Scrollback only when there is scrollback
  long onUpdClearScrollback(FXObject*,FXSelector,void*);

  /// Enable the tab entries only for a tab of the strip
  long onUpdTabMenu(FXObject*,FXSelector,void*);

  /// Enable Close Others only with more than one tab
  long onUpdCloseOthers(FXObject*,FXSelector,void*);

  /// Enable the strip menu's Detach only with a second tab
  long onUpdTabMenuDetach(FXObject*,FXSelector,void*);

  /// Select the character encoding of the active tab
  long onCmdEncoding(FXObject*,FXSelector,void*);

  /// Check the active encoding and enable the entries that can act
  long onUpdEncoding(FXObject*,FXSelector,void*);

  /// Report text the active encoding cannot express
  long onCmdEncodingLoss(FXObject*,FXSelector,void*);

  /// Show About
  long onCmdAbout(FXObject*,FXSelector,void*);
public:
  enum {
    ID_TITLE=FXMainWindow::ID_LAST,
    ID_BELL,
    ID_EXITED,
    ID_CLOSE_ON_EXIT,
    ID_RULE,
    ID_STATUSHIDE,
    ID_CLIPBOARD,
    ID_NEW_TAB,
    ID_NEW_WINDOW,
    ID_CLOSE_CURRENT,
    ID_DETACH,
    ID_FIND,
    ID_FIND_NEXT,
    ID_FIND_PREV,
    ID_FIND_CLOSE,
    ID_NEXT_TAB,
    ID_PREV_TAB,
    ID_PROFILE0,
    ID_PROFILE_LAST=ID_PROFILE0+15,
    ID_SETTINGS,
    ID_OPACITY0,
    ID_OPACITY_LAST=ID_OPACITY0+4,
    ID_COPY,
    ID_PASTE,
    ID_SELECT_ALL,
    ID_CLEAR_SCROLLBACK,
    ID_RESET_TERMINAL,
    ID_ENCODING0,
    ID_ENCODING_LAST=ID_ENCODING0+31,
    ID_DUP_TAB,
    ID_RENAME_TAB,
    ID_TABMENU_DETACH,
    ID_TABMENU_CLOSE,
    ID_CLOSE_OTHERS,
    ID_ABOUT,
    ID_QUIT,
    ID_LAST
    };
public:

  /// Destructor
  virtual ~TerminalWindow();
  };

#endif
