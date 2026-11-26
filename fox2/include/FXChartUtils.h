/********************************************************************************
*                                                                               *
*                          U t i l i t y - F u n c t i o n s                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXCHARTUTILS_H
#define FXCHARTUTILS_H

#ifndef FXPOINT_H
#include "FXPoint.h"
#endif

#include <vector>

namespace FXCHART {

/// A contiguous polyline part or dotted bridge over a missing-data gap
struct FXCurveSegment {
	FXint first;                  /// Index of the first point
	FXint count;                  /// Number of points in this segment
	FXbool gap;                   /// True for a dotted bridge between valid runs
	};

// Helper functions
extern FXCHARTAPI FXint textWidth(FXFont *font,const FXString& string);
extern FXCHARTAPI FXint textHeight(FXFont *font,const FXString& string);
/// Find the nearest point or segment within radius pixels; return its first point index or -1
/// Optional distance output is squared pixel distance, or -1 when no hit was found
extern FXCHARTAPI FXint pickNearest(const FXPointF* points,FXint count,FXint x,FXint y,FXint radius,FXdouble* distance=nullptr);
/// Split points at non-finite samples and add dotted bridges between valid runs separated by gaps
extern FXCHARTAPI void getCurveSegments(const FXPointF* points,FXint count,std::vector<FXCurveSegment>& segments);
extern FXCHARTAPI void drawText(FXDC& dc,const FXString& string,FXint x,FXint y,FXuint style);
extern FXCHARTAPI void drawRectangle(FXDC& dc,const FillStyle& fs,FXint x,FXint y,FXint w,FXint h);
extern FXCHARTAPI void drawMarker(FXDC& dc,const Marker& ms,FXint x,FXint y);
extern FXCHARTAPI void getTickSpacing(FXdouble& major,FXdouble& minor,FXdouble min,FXdouble max);
extern FXCHARTAPI FXColor readableColor(FXColor color,FXColor background);
extern FXCHARTAPI FXColor contrastColor(FXColor background,FXint percent);

}

#endif
