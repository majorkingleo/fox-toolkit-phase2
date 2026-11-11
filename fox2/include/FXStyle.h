/********************************************************************************
*                                                                               *
*                          S t y l e    O b j e c t s                            *
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
#ifndef FXSTYLE_H
#define FXSTYLE_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXRECTANGLE_H
#include "FXRectangle.h"
#endif

namespace FX {


class FXTheme;
class FXDCWindow;


/// State handed to the style when drawing a button
struct FXButtonState {
  FXbool enabled;     /// Widget is enabled
  FXbool hover;       /// Pointer is inside the widget
  FXbool down;        /// Button is pressed
  FXbool engaged;     /// Button is engaged or checked
  FXbool focused;     /// Widget has keyboard focus
  FXbool defaulted;   /// Widget is the default button
  FXbool toolbar;     /// Toolbar style button
  FXint  border;      /// Border width of the widget
  FXuint options;     /// Frame options of the widget
  FXColor back;       /// Background color of the widget
  FXColor text;       /// Text color of the widget
  FXColor parentBack; /// Background color of the parent
  };


/**
* A style draws widget chrome from a theme's colors and metrics.  Widgets keep
* their behavior and their content layout; the style owns only the visual
* decoration, so rounded corners and borders never disturb icon or label
* placement.  The classic style reproduces FOX's original drawing exactly.
*/
class FXAPI FXStyle {
protected:
  struct FXCornerMask;
  FXTheme *theme;                 // Colors and metrics to draw with
  FXbool   xrender;               // XRender compositing available
  FXbool   xrenderChecked;        // Capability already probed
  void    *dpy;                   // Display used for the mask cache
  FXCornerMask *masks;            // Cached corner masks by radius
protected:

  /// Probe whether the drawable supports antialiased compositing
  FXbool hasXRender(FXDCWindow& dc);

  /// Return cached corner mask for the given radius
  FXCornerMask* getMask(FXint radius);

  /// Draw an antialiased rounded rectangle; returns false if unsupported
  FXbool drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth);
public:

  /// Construct style for the given theme
  FXStyle(FXTheme* thm=nullptr);

  /// Change theme
  void setTheme(FXTheme* thm){ theme=thm; }

  /// Drop cached chrome resources after a theme change
  virtual void invalidate();

  /// Return theme
  FXTheme* getTheme() const { return theme; }

  /// Draw button chrome; returns rectangle available to icon and label
  virtual FXRectangle drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXButtonState& state);

  /// Draw frame chrome for the given frame options
  virtual void drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint options);

  /// Draw keyboard focus ring
  virtual void drawFocusRing(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw border rectangle
  void drawBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw raised rectangle
  void drawRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw sunken rectangle
  void drawSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw double raised rectangle
  void drawDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw double sunken rectangle
  void drawDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw ridge rectangle
  void drawRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Draw groove rectangle
  void drawGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);

  /// Destructor
  virtual ~FXStyle();
  };

}

#endif
