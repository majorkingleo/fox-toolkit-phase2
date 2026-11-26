/********************************************************************************
*                                                                               *
*                         F o l l o w   V i e w   S t a t e                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXFOLLOWVIEW_H
#define FXFOLLOWVIEW_H

#ifndef FXCHARTDEFS_H
#include "FXChartDefs.h"
#endif

namespace FXCHART {


/// Pure logic for a time-based viewport that follows new data until scrolled back
class FXCHARTAPI FXFollowView {
private:
  FXdouble windowlength;
  FXdouble viewleft;
  FXdouble latesttime;
  FXbool following;
  FXbool havetime;
public:
  enum {
    SCROLLBAR_RANGE=1000000
    };
public:
  /// Construct a following view with a 30-second window
  FXFollowView();

  /// Set visible time span in seconds; values are clamped to 1..7200
  void setWindowLength(FXdouble seconds);

  /// Return visible time span in seconds
  FXdouble getWindowLength() const { return windowlength; }

  /// Return left edge of the current time view
  FXdouble getViewLeft() const { return viewleft; }

  /// Return right edge of the current time view
  FXdouble getViewRight() const { return viewleft+windowlength; }

  /// Return true while new data advances the view
  FXbool isFollowing() const { return following; }

  /// Update latest timestamp and move the view when following
  void update(FXdouble latestTime);

  /// Map the data interval and current view to scrollbar range, page, and position
  void getScrollBar(FXdouble dataStart,FXdouble latestTime,FXint& range,FXint& page,FXint& position) const;

  /// Apply a user scrollbar position; reaching the end resumes following
  void scrollTo(FXint position,FXint range,FXdouble dataStart,FXdouble latestTime);
  };

}

#endif
