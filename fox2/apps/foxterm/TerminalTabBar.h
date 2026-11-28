/********************************************************************************
*                                                                               *
*                         T e r m i n a l   T a b   B a r                       *
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
#ifndef TERMINALTABBAR_H
#define TERMINALTABBAR_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif

#include <vector>

namespace FX {


/**
* A compact tab strip. The strip paints the titles, highlights the active tab,
* and sends ID_TAB, ID_CLOSE_TAB or ID_CONTEXT_TAB to its target. Overflow
* scrolling and drag reorder are added by the next phase.
*/
class TerminalTabBar : public FXPacker {
  FXDECLARE(TerminalTabBar)
protected:
  FXFont              *font;
  std::vector<FXString> titles;
  FXint                current;
  FXint                hover;
protected:
  TerminalTabBar(){}
  FXint tabWidth(FXint index) const;
  FXint tabAt(FXint x) const;
  FXbool closeAt(FXint index,FXint x) const;
public:
  enum {
    ID_TAB=FXPacker::ID_LAST,
    ID_CLOSE_TAB,
    ID_CONTEXT_TAB,
    ID_LAST
    };
public:

  /// Construct a tab strip
  TerminalTabBar(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=2,FXint pr=2,FXint pt=2,FXint pb=2);

  /// Create resources
  virtual void create() override;

  /// Detach resources
  virtual void detach() override;

  /// Re-apply theme font and colors
  virtual void applyTheme(FXTheme* theme) override;

  /// Return the default height
  virtual FXint getDefaultHeight() override;

  /// Paint the strip
  long onPaint(FXObject*,FXSelector,void*);

  /// Select a tab or its close box
  long onLeftBtnPress(FXObject*,FXSelector,void*);

  /// Post the tab menu for the tab under the pointer
  long onRightBtnPress(FXObject*,FXSelector,void*);

  /// Track the close box under the pointer
  long onMotion(FXObject*,FXSelector,void*);

  /// Clear the hovered tab
  long onLeave(FXObject*,FXSelector,void*);

  /// Set all titles
  void setTitles(const std::vector<FXString>& text);

  /// Append a title
  void addTitle(const FXString& text);

  /// Remove a title
  void removeTitle(FXint index);

  /// Change one title
  void setTitle(FXint index,const FXString& text);

  /// Return one title
  const FXString& getTitle(FXint index) const;

  /// Return number of tabs
  FXint getNumTabs() const { return (FXint)titles.size(); }

  /// Set the active tab
  void setCurrent(FXint index);

  /// Return the active tab
  FXint getCurrent() const { return current; }

  /// Destructor
  virtual ~TerminalTabBar();
  };

}

#endif
