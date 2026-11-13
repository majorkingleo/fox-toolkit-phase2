/********************************************************************************
*                                                                               *
*                       G l y p h   C a c h e                                   *
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
#ifndef FXGLYPHS_H
#define FXGLYPHS_H

/*
  Notes:

  - Private header: not installed, not part of the FOX API.
  - One instance caches the glyphs of one font at one size and resolution:
    fontconfig selects the font file, FreeType loads and renders the glyphs.
    That is the same pair of libraries the Xft path uses, so the font selection
    and the metrics agree with the X11 build instead of being a second opinion.
  - FreeType and fontconfig are optional; the build defines HAVE_XFT_H when it
    finds them (the Xft path needs the same two libraries).  Without them the
    class exists but open() fails, so the rasterizer compiles everywhere and
    simply has no text.
*/

#if defined(HAVE_XFT_H) || defined(FOX_HAVE_FREETYPE)
#define FOX_GLYPHS_AVAILABLE 1
#endif


namespace FX {


/// Decode one UTF-8 character, advancing p; invalid bytes yield the replacement character
FXAPI FXuint fxutf8decode(const FXchar* text,FXint len,FXint& p);


/// One cached glyph: coverage bitmap plus where to put it
struct FXGlyph {
  FXuint   ch;                  // Unicode character
  FXint    advance;             // Advance width in pixels
  FXint    bearing;             // Bitmap left bearing
  FXint    top;                 // Rows above the baseline
  FXint    width;               // Bitmap width
  FXint    height;              // Bitmap height
  FXuchar* coverage;            // width*height coverage values
  };


/// Glyph cache of one font at one size and resolution
class FXAPI FXGlyphs {
private:
  void*    library;             // FreeType library handle
  void*    face;                // FreeType face handle
  FXGlyph* glyphs;              // Cached glyphs
  FXuint   count;               // Number of cached glyphs
  FXuint   alloced;             // Allocated glyphs
  FXint    ascent;              // Distance from baseline to top
  FXint    descent;             // Distance from baseline to bottom
  FXint    height;              // Ascent plus descent
  FXint    maxadvance;          // Widest advance
  FXint    pixelsize;           // Pixel size
  FXbool   antialias;           // Antialiased rendering
  FXbool   hinting;             // Hinted rendering
  FXbool   autohint;            // Use the auto hinter
  FXString filename;            // Font file chosen by fontconfig
  FXString fontname;            // Family name chosen by fontconfig
private:
  FXGlyphs(const FXGlyphs&);
  FXGlyphs& operator=(const FXGlyphs&);
  FXGlyph* load(FXuint ch);
public:

  /// Construct a cache without a font
  FXGlyphs();

  /// Construct a cache and open the font described by the arguments
  FXGlyphs(const FXchar* family,FXuint size,FXuint weight=40,FXuint slant=5,FXuint setwidth=100,FXint dpi=96,FXbool antialias=true,FXbool hinting=true,FXbool autohint=false);

  /// Open a font by family name, size in deci-points, weight, slant, set width and resolution
  FXbool open(const FXchar* family,FXuint size,FXuint weight=40,FXuint slant=5,FXuint setwidth=100,FXint dpi=96,FXbool antialias=true,FXbool hinting=true,FXbool autohint=false);

  /// Close the font and drop the cache
  void close();

  /// Return true if a font is open
  FXbool isOpen() const { return face!=nullptr; }

  /// Return the font file fontconfig selected
  const FXString& getFontFile() const { return filename; }

  /// Return the family name fontconfig selected
  const FXString& getFontName() const { return fontname; }

  /// Return pixel size
  FXint getPixelSize() const { return pixelsize; }

  /// Return distance from the baseline to the top of the font
  FXint getAscent() const { return ascent; }

  /// Return distance from the baseline to the bottom of the font
  FXint getDescent() const { return descent; }

  /// Return the line height
  FXint getHeight() const { return height; }

  /// Return the widest advance
  FXint getMaxAdvance() const { return maxadvance; }

  /// Return the cached glyph of a character, loading it if needed
  const FXGlyph* getGlyph(FXuint ch);

  /// Return true if the font has a glyph for the character
  FXbool hasChar(FXuint ch);

  /// Return the advance width of a character
  FXint getCharWidth(FXuint ch);

  /// Return the width of a UTF-8 string of length len (len<0 means 0-terminated)
  FXint getTextWidth(const FXchar* text,FXint len=-1);

  /// Return the character index at pixel position x in a UTF-8 string
  FXint getCharAt(const FXchar* text,FXint len,FXint x);

  /// Destroy the cache
 ~FXGlyphs();
  };

}

#endif
