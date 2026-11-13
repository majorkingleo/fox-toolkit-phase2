/********************************************************************************
*                                                                               *
*                          F o n t   ( W a y l a n d )                          *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXMutex.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXFont.h"
#include "fxglyphs.h"

#if defined(FOX_WAYLAND)

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002

/*
  Notes:

  - A Wayland font is a glyph cache: fontconfig picks the font file, FreeType
    rasterizes the glyphs, and FXRaster blends them into the window buffer.
  - Font matching, sizing and metrics follow the Xft path in FXFont_x11.cpp, so
    a window that was laid out with Xft keeps its layout here; tests/glyphs
    compares the two directly.
  - The FXFont member "font" holds the FXGlyphs; FXDCWindow::setFont() hands it
    to the rasterizer through getGlyphCache().
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Try to find a matching font; the result is a glyph cache
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  FXGlyphs* glyphs;
  FXTRACE(TOPIC_DETAIL,"FXFont::match: wantfamily=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d res=%d\n",wantfamily.text(),wantsize,wantweight,wantslant,wantsetwidth,res);
  if(res<=0) res=96;
  glyphs=new FXGlyphs();
  if(!glyphs->open(wantfamily.text(),wantsize,wantweight,wantslant,wantsetwidth,res)){
    delete glyphs;
    return nullptr;
    }
  actualName=glyphs->getFontName();
  actualSize=(FXushort)((720.0*glyphs->getPixelSize())/res);
  actualWeight=(FXushort)wantweight;
  actualSlant=(FXushort)wantslant;
  actualSetwidth=(FXushort)wantsetwidth;
  actualEncoding=FONTENCODING_UNICODE;
  flags|=FXFont::Scalable;
  if(wanthints&FXFont::Mono){ flags|=FXFont::Mono; }
  if(wanthints&FXFont::Variable){ flags|=FXFont::Variable; }
  return glyphs;
  }


// Create font
void FXFont::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXString family=getFamily();
      FXint res;
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
      res=resolution;
      if(res<=0){
        res=(FXint)getApp()->reg().readUIntEntry("SETTINGS","screenres",100);
        }
      font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
      if(!font){ throw FXFontException("unable to create font"); }
      xid=1;
      }
    }
  }


// Detach font
void FXFont::detach(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    delete (FXGlyphs*)font;
    font=nullptr;
    xid=0;
    }
  }


// Destroy font
void FXFont::destroy(){
  detach();
  }


// List fonts matching hints; enumeration through fontconfig is not implemented
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  fonts=nullptr;
  numfonts=0;
  return false;
  }


/*******************************************************************************/

// Return true if the font has a glyph for the character
FXbool FXFont::hasChar(FXwchar ch) const {
  if(font){ return ((FXGlyphs*)font)->hasChar(ch); }
  return false;
  }


// Return the smallest character in the font
FXwchar FXFont::getMinChar() const {
  return 0;
  }


// Return the largest character in the font
FXwchar FXFont::getMaxChar() const {
  return 0xffff;
  }


// Get the font leading; as in the Xft path there is none on top of the height
FXint FXFont::getFontLeading() const {
  return 0;
  }


// Get the font line spacing
FXint FXFont::getFontSpacing() const {
  if(font){ return ((FXGlyphs*)font)->getHeight(); }
  return 1;
  }


// Left bearing of a character
FXint FXFont::leftBearing(FXwchar ch) const {
  if(font){
    const FXGlyph* glyph=((FXGlyphs*)font)->getGlyph(ch);
    if(glyph) return glyph->bearing;
    }
  return 0;
  }


// Right bearing of a character
FXint FXFont::rightBearing(FXwchar ch) const {
  if(font){
    const FXGlyph* glyph=((FXGlyphs*)font)->getGlyph(ch);
    if(glyph) return glyph->advance-glyph->width-glyph->bearing;
    }
  return 0;
  }


// Is it a mono space font
FXbool FXFont::isFontMono() const {
  if(font){ return ((FXGlyphs*)font)->getCharWidth('i')==((FXGlyphs*)font)->getCharWidth('M'); }
  return true;
  }


// Get font width
FXint FXFont::getFontWidth() const {
  if(font){ return ((FXGlyphs*)font)->getMaxAdvance(); }
  return 1;
  }


// Get font height
FXint FXFont::getFontHeight() const {
  if(font){ return ((FXGlyphs*)font)->getHeight(); }
  return 1;
  }


// Get font ascent
FXint FXFont::getFontAscent() const {
  if(font){ return ((FXGlyphs*)font)->getAscent(); }
  return 1;
  }


// Get font descent
FXint FXFont::getFontDescent() const {
  if(font){ return ((FXGlyphs*)font)->getDescent(); }
  return 0;
  }


// Get glyph cache, which is the font itself on this platform
void* FXFont::getGlyphCache() const {
  return font;
  }


// Calculate width of single wide character in this font
FXint FXFont::getCharWidth(const FXwchar ch) const {
  if(font){ return ((FXGlyphs*)font)->getCharWidth(ch); }
  return 1;
  }


// Text width
FXint FXFont::getTextWidth(const FXchar *string,FXuint length) const {
  if(font){ return ((FXGlyphs*)font)->getTextWidth(string,(FXint)length); }
  return 0;
  }


// Text height
FXint FXFont::getTextHeight(const FXchar *string,FXuint length) const {
  if(font){ return ((FXGlyphs*)font)->getHeight(); }
  return 1;
  }

}

#endif
