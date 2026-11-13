/********************************************************************************
*                                                                               *
*                     R e c t a n g l e   R e g i o n                          *
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
#ifndef FXRECTS_H
#define FXRECTS_H

/*
  Notes:

  - Private header: not installed, not part of the FOX API.
  - A region kept as a list of horizontal spans, one entry per run of pixels on
    a scan line.  The list is sorted by scan line and start, spans on the same
    scan line never overlap and never touch, so the list is a canonical form:
    equal pixel sets have equal lists.
  - This is the portable counterpart of the X11 and Win32 regions, used by the
    Wayland platform module and by the software rasterizer; it has no
    dependency on a display, so it can be tested on any host.
  - Spans follow the X11 rule for rectangles: a rectangle (x,y,w,h) covers the
    pixels x..x+w-1 on the scan lines y..y+h-1.
*/

namespace FX {


/// One run of pixels on a scan line, end points included
struct FXSpan {
  FXint y;                      // Scan line
  FXint x1;                     // First pixel
  FXint x2;                     // Last pixel
  };


/// Region as a canonical list of spans
class FXAPI FXRects {
private:
  FXSpan* spans;                // Span list
  FXuint  count;                // Number of spans
  FXuint  alloced;              // Allocated spans
private:
  FXbool push(const FXSpan& span);
  FXbool pushMerged(const FXSpan& span);
  void   setSpans(FXSpan* list,FXuint n,FXuint cap);
public:
  FXRects();
  FXRects(FXint x,FXint y,FXint w,FXint h);
  FXRects(const FXRects& r);
  FXRects& operator=(const FXRects& r);

  /// Return true if region is empty
  FXbool empty() const { return count==0; }

  /// Return number of spans
  FXuint getSpanCount() const { return count; }

  /// Return span list
  const FXSpan* getSpans() const { return spans; }

  /// Reset region to empty
  void clear();

  /// Make region a single rectangle
  void setRectangle(FXint x,FXint y,FXint w,FXint h);

  /// Make region a polygon, filled by the even-odd or the winding rule
  void setPolygon(const FXPoint* points,FXuint npoints,FXbool winding=false);

  /// Return true if region contains point
  FXbool contains(FXint x,FXint y) const;

  /// Return true if region contains the whole rectangle
  FXbool contains(FXint x,FXint y,FXint w,FXint h) const;

  /// Return bounding box
  FXRectangle bounds() const;

  /// Offset region by dx,dy
  FXRects& offset(FXint dx,FXint dy);

  /// Union with region r
  FXRects& unite(const FXRects& r);

  /// Intersect with region r
  FXRects& intersect(const FXRects& r);

  /// Subtract region r
  FXRects& subtract(const FXRects& r);

  /// Exclusive or with region r
  FXRects& xorbit(const FXRects& r);

  /// Return true if the two regions cover the same pixels
  FXbool operator==(const FXRects& r) const;

  /// Return true if the regions differ
  FXbool operator!=(const FXRects& r) const { return !(*this==r); }

  /// Destroy region
 ~FXRects();
  };

}

#endif
