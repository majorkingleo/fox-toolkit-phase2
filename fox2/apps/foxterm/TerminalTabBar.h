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

namespace FX {


/**
* A compact single-tab strip. The first application phase uses it for one
* session; the Phase 5 work adds the vector of tabs, close buttons, overflow
* and drag handling without changing the window layout.
*/
class TerminalTabBar : public FXPacker {
  FXDECLARE(TerminalTabBar)
protected:
  FXFont  *font;
  FXString title;
  FXbool   active;
protected:
  TerminalTabBar(){}
public:

  /// Construct a tab strip
  TerminalTabBar(FXComposite* p,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=2,FXint pr=2,FXint pt=2,FXint pb=2);

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

  /// Change the tab title
  void setTitle(const FXString& text);

  /// Return the tab title
  const FXString& getTitle() const { return title; }

  /// Set the active state
  void setActive(FXbool flag);

  /// Return the active state
  FXbool isActive() const { return active; }

  /// Destructor
  virtual ~TerminalTabBar();
  };

}

#endif
