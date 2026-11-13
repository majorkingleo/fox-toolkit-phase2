/********************************************************************************
*                                                                               *
*                  S o f t w a r e   R a s t e r i z e r                        *
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
#ifndef FXRASTER_H
#define FXRASTER_H

/*
  Notes:

  - Private header: not installed, not part of the FOX API.
  - Draws the platform neutral primitives of FXDC into a memory buffer for
    platforms that have no drawing requests of their own (Wayland, the browser
    frontend), and for headless tests on any host.
  - The target is 0xAARRGGBB in host byte order, one FXColor per pixel, rows
    separated by a stride in pixels.  Drawing is opaque: every pixel written
    has alpha 255, which is a valid premultiplied wl_shm ARGB8888 buffer, and
    the representation a compositor wants without a conversion pass.
  - Drawing follows the X11 server rules, because the X11 build is the visual
    reference: rectangle fills cover [x,x+w) x [y,y+h), zero width lines are
    one pixel wide, only fill requests use the fill style and stipple, and
    points, lines and arcs are drawn with the foreground color and the raster
    function.
  - Known deviations from the X11 server are listed in docs/wayland-plan.md:
    zero width diagonal lines, wide lines and the outline of arcs follow the
    analytic shape rather than the X11 rasterizer, so a few pixels may differ.
*/

namespace FX {


/// Software rasterizer for FXDC's platform neutral drawing primitives
class FXAPI FXRaster {
private:
  FXColor*        target;       // Target pixels
  FXint           twidth;       // Target width
  FXint           theight;      // Target height
  FXint           tstride;      // Target stride in pixels
  FXRectangle     clip;         // Clip rectangle
  FXRegion*       clipregion;   // Clip region or nullptr
  const FXuchar*  maskbits;     // Clip mask bits or nullptr
  FXint           maskwidth;    // Clip mask width
  FXint           maskheight;   // Clip mask height
  FXint           maskbytew;    // Clip mask bytes per row
  FXint           mx;           // Clip mask origin x
  FXint           my;           // Clip mask origin y
  FXColor         fg;           // Foreground color
  FXColor         bg;           // Background color
  FXuint          linewidth;    // Line width
  FXCapStyle      cap;          // Line cap style
  FXJoinStyle     join;         // Line join style
  FXLineStyle     linestyle;    // Line style
  FXFillStyle     fillstyle;    // Fill style
  FXFillRule      fillrule;     // Fill rule
  FXFunction      function;     // Raster function
  FXuchar         dashpat[32];  // Dash pattern data
  FXuint          dashlen;      // Dash pattern length
  FXuint          dashoff;      // Dash pattern offset
  const FXImage*  tile;         // Tile image
  const FXColor*  tilepix;      // Tile pixels
  FXint           tilewidth;    // Tile width
  FXint           tileheight;   // Tile height
  FXint           tx;           // Tile origin x
  FXint           ty;           // Tile origin y
  const FXBitmap* stipple;      // Stipple bitmap
  FXStipplePattern pattern;     // Stipple pattern
  const FXuchar*  stipbits;     // Stipple bits
  FXint           stipwidth;    // Stipple width
  FXint           stipheight;   // Stipple height
  FXint           stipbytew;    // Stipple bytes per row
  FXint           stx;          // Stipple origin x
  FXint           sty;          // Stipple origin y
private:
  FXRaster(const FXRaster&);
  FXRaster& operator=(const FXRaster&);
  FXbool inClip(FXint x,FXint y) const;
  FXbool stipBit(FXint x,FXint y) const;
  void plot(FXint x,FXint y,FXColor src);
  void plotStyled(FXint x,FXint y);
  void span(FXint y,FXint x1,FXint x2);
  void plotLine(FXint x1,FXint y1,FXint x2,FXint y2,FXuint& dashindex,FXuint& dashremain,FXbool& dashon,FXbool skipfirst);
  void wideLine(FXint x1,FXint y1,FXint x2,FXint y2);
  void fillFgRectangle(FXint x,FXint y,FXint w,FXint h);
  void fillFgEllipse(FXint x,FXint y,FXint w,FXint h);
  void fillFgPolygon(const FXPoint* points,FXuint npoints);
  void fillPolyspan(const FXPoint* points,FXuint npoints,FXbool relative);
  void fillArcSpan(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2,FXbool chord);
public:

  /// Construct rasterizer with no target
  FXRaster();

  /// Construct rasterizer for a target buffer of width times height pixels
  FXRaster(FXColor* pixels,FXint width,FXint height,FXint stride=0);

  /// Change target buffer; a stride of 0 means width pixels per row
  void setTarget(FXColor* pixels,FXint width,FXint height,FXint stride=0);

  /// Return target buffer
  FXColor* getTarget() const { return target; }

  /// Return target width
  FXint getWidth() const { return twidth; }

  /// Return target height
  FXint getHeight() const { return theight; }

  /// Return target stride in pixels
  FXint getStride() const { return tstride; }

  /// Return the clip rectangle limited to the target
  FXRectangle getClipRectangle() const;

  /// Read back pixel
  FXColor readPixel(FXint x,FXint y) const;

  /// Draw points
  void drawPoint(FXint x,FXint y);
  void drawPoints(const FXPoint* points,FXuint npoints);
  void drawPointsRel(const FXPoint* points,FXuint npoints);

  /// Draw lines
  void drawLine(FXint x1,FXint y1,FXint x2,FXint y2);
  void drawLines(const FXPoint* points,FXuint npoints);
  void drawLinesRel(const FXPoint* points,FXuint npoints);
  void drawLineSegments(const FXSegment* segments,FXuint nsegments);

  /// Draw rectangles
  void drawRectangle(FXint x,FXint y,FXint w,FXint h);
  void drawRectangles(const FXRectangle* rectangles,FXuint nrectangles);

  /// Draw rounded rectangle
  void drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);

  /// Draw arcs
  void drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
  void drawArcs(const FXArc* arcs,FXuint narcs);

  /// Draw ellipse
  void drawEllipse(FXint x,FXint y,FXint w,FXint h);

  /// Fill rectangles
  void fillRectangle(FXint x,FXint y,FXint w,FXint h);
  void fillRectangles(const FXRectangle* rectangles,FXuint nrectangles);

  /// Fill rounded rectangle
  void fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);

  /// Fill chords
  void fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
  void fillChords(const FXArc* chords,FXuint nchords);

  /// Fill arcs
  void fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
  void fillArcs(const FXArc* arcs,FXuint narcs);

  /// Fill ellipse
  void fillEllipse(FXint x,FXint y,FXint w,FXint h);

  /// Fill polygons
  void fillPolygon(const FXPoint* points,FXuint npoints);
  void fillConcavePolygon(const FXPoint* points,FXuint npoints);
  void fillComplexPolygon(const FXPoint* points,FXuint npoints);
  void fillPolygonRel(const FXPoint* points,FXuint npoints);
  void fillConcavePolygonRel(const FXPoint* points,FXuint npoints);
  void fillComplexPolygonRel(const FXPoint* points,FXuint npoints);

  /// Fill vertical gradient rectangle
  void fillVerticalGradient(FXint x,FXint y,FXint w,FXint h,FXColor top,FXColor bottom);

  /// Fill horizontal gradient rectangle
  void fillHorizontalGradient(FXint x,FXint y,FXint w,FXint h,FXColor left,FXColor right);

  /// Draw hashed box
  void drawHashBox(FXint x,FXint y,FXint w,FXint h,FXint b=1);

  /// Draw focus rectangle
  void drawFocusRectangle(FXint x,FXint y,FXint w,FXint h);

  /// Draw area from image
  void drawArea(const FXImage* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy);
  void drawArea(const FXImage* source,FXint sx,FXint sy,FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh);

  /// Draw image, bitmap and icon
  void drawImage(const FXImage* image,FXint dx,FXint dy);
  void drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy);
  void drawIcon(const FXIcon* icon,FXint dx,FXint dy);

  /// Set foreground drawing color
  void setForeground(FXColor clr);

  /// Get foreground drawing color
  FXColor getForeground() const { return fg; }

  /// Set background drawing color
  void setBackground(FXColor clr);

  /// Get background drawing color
  FXColor getBackground() const { return bg; }

  /// Set dash pattern and dash offset
  void setDashes(FXuint dashoffset,const FXuchar* dashpattern,FXuint dashlength);

  /// Get dash pattern
  const FXuchar* getDashPattern() const { return dashpat; }

  /// Get dash offset
  FXuint getDashOffset() const { return dashoff; }

  /// Get dash length
  FXuint getDashLength() const { return dashlen; }

  /// Set line width
  void setLineWidth(FXuint linewidth=0);

  /// Get line width
  FXuint getLineWidth() const { return linewidth; }

  /// Set line cap style
  void setLineCap(FXCapStyle capstyle=CAP_BUTT);

  /// Get line cap style
  FXCapStyle getLineCap() const { return cap; }

  /// Set line join style
  void setLineJoin(FXJoinStyle joinstyle=JOIN_MITER);

  /// Get line join style
  FXJoinStyle getLineJoin() const { return join; }

  /// Set line style
  void setLineStyle(FXLineStyle linestyle=LINE_SOLID);

  /// Get line style
  FXLineStyle getLineStyle() const { return linestyle; }

  /// Set fill style
  void setFillStyle(FXFillStyle fillstyle=FILL_SOLID);

  /// Get fill style
  FXFillStyle getFillStyle() const { return fillstyle; }

  /// Set fill rule
  void setFillRule(FXFillRule fillrule=RULE_EVEN_ODD);

  /// Get fill rule
  FXFillRule getFillRule() const { return fillrule; }

  /// Set rasterop function
  void setFunction(FXFunction func=BLT_SRC);

  /// Get rasterop function
  FXFunction getFunction() const { return function; }

  /// Set the tile image
  void setTile(const FXImage* image,FXint dx=0,FXint dy=0);

  /// Get the tile image
  const FXImage* getTile() const;

  /// Set the stipple bitmap
  void setStipple(const FXBitmap* bitmap,FXint dx=0,FXint dy=0);

  /// Get stipple bitmap
  const FXBitmap* getStippleBitmap() const;

  /// Set the stipple pattern
  void setStipple(FXStipplePattern pat,FXint dx=0,FXint dy=0);

  /// Get pattern
  FXStipplePattern getStipplePattern() const;

  /// Set clip region; combines with the clip rectangle
  void setClipRegion(const FXRegion& region);

  /// Clear clip region
  void clearClipRegion();

  /// Set clip rectangle
  void setClipRectangle(FXint x,FXint y,FXint w,FXint h);
  void setClipRectangle(const FXRectangle& rectangle);

  /// Clear clipping
  void clearClipRectangle();

  /// Set clip mask
  void setClipMask(const FXBitmap* bitmap,FXint dx=0,FXint dy=0);

  /// Clear clip mask
  void clearClipMask();

  /// Destroy rasterizer
 ~FXRaster();
  };

}

#endif
