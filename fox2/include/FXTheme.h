/********************************************************************************
*                                                                               *
*                          T h e m e    C o l o r s                              *
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
#ifndef FXTHEME_H
#define FXTHEME_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

namespace FX {


class FXString;


/// Semantic color roles
enum {
  ROLE_WINDOW_BG,         /// Window background
  ROLE_SURFACE,           /// Control surface
  ROLE_SURFACE_HILITE,    /// Raised edge highlight
  ROLE_SURFACE_SHADOW,    /// Sunken edge shadow
  ROLE_TEXT,              /// Normal text
  ROLE_TEXT_DISABLED,     /// Disabled text
  ROLE_BORDER,            /// Border
  ROLE_SELECTION_BG,      /// Selection background
  ROLE_SELECTION_TEXT,    /// Selection text
  ROLE_MENU_SEL_BG,       /// Selected menu background
  ROLE_MENU_SEL_TEXT,     /// Selected menu text
  ROLE_TOOLTIP_BG,        /// Tooltip background
  ROLE_TOOLTIP_TEXT,      /// Tooltip text
  ROLE_ACCENT,            /// Accent color
  ROLE_FOCUS_RING,        /// Keyboard focus ring
  ROLE_LAST
  };


/// States in which a role may be queried
enum {
  THEME_STATE_NORMAL,     /// Normal state
  THEME_STATE_HOVER,      /// Pointer inside
  THEME_STATE_ACTIVE,     /// Pressed or engaged
  THEME_STATE_DISABLED,   /// Disabled
  THEME_STATE_SELECTED,   /// Selected
  THEME_STATE_FOCUSED,    /// Keyboard focused
  THEME_STATE_LAST
  };


/// Metric subjects
enum {
  METRIC_PADDING,         /// Interior padding
  METRIC_MARGIN,          /// Outer margin
  METRIC_BORDER_WIDTH,    /// Border width
  METRIC_FOCUS_WIDTH,     /// Focus ring width
  METRIC_SPACING,         /// Spacing between items
  METRIC_ICON_SIZE,       /// Default icon size
  METRIC_SMALL_ICON_SIZE, /// Small icon size
  METRIC_ROW_HEIGHT,      /// Default row height
  METRIC_CORNER_RADIUS,   /// Corner radius
  METRIC_LAST
  };


/**
* A theme holds semantic colors and metrics, resolved centrally by the
* application so that widgets do not repeat palette assumptions.  The
* classic theme reproduces the default FOX palette exactly.
*/
class FXAPI FXTheme {
protected:
  FXColor colors[ROLE_LAST];    // Base color for each role
  FXint   metrics[METRIC_LAST]; // Value for each metric
public:

  /// Construct classic theme
  FXTheme();

  /// Return color of given role in given state
  FXColor color(FXint role,FXint state=THEME_STATE_NORMAL) const;

  /// Change color of given role
  void setColor(FXint role,FXColor clr);

  /// Return value of given metric
  FXint metric(FXint subject) const;

  /// Change value of given metric
  void setMetric(FXint subject,FXint value);

  /// Recompute colors derived from base roles
  void updateDerived();

  /// Reset to the classic theme
  void setClassic();

  /// Switch to the dark preset
  void setDark();

  /// Load theme from an INI file; returns true if the file parsed
  FXbool load(const FXString& path);
  };

}

#endif
