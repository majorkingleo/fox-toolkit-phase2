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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXFont.h"
#include "fxglyphs.h"

/*
  Notes:

  - The font is matched with the same fontconfig pattern the Xft path builds in
    FXFont_x11.cpp, and the face is sized the way Xft sizes it, so both give the
    same glyph advances and the same ascent and descent.
  - Glyphs are cached by character, not by glyph index, because the callers
    hand us UTF-8 text; a second lookup of the same character returns the same
    cached entry.
  - Without FreeType and fontconfig the whole implementation compiles away and
    open() fails; the rasterizer then simply has no text.
*/

#if defined(FOX_GLYPHS_AVAILABLE)

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

// Decode one UTF-8 character, advancing p; invalid bytes yield the
// replacement character
FXuint fxutf8decode(const FXchar* text,FXint len,FXint& p){
  FXuchar c=(FXuchar)text[p];
  FXuint ch;
  FXint n;
  if(c<0x80){ p+=1; return c; }
  if((c&0xe0)==0xc0){ ch=c&0x1f; n=1; }
  else if((c&0xf0)==0xe0){ ch=c&0x0f; n=2; }
  else if((c&0xf8)==0xf0){ ch=c&0x07; n=3; }
  else{ p+=1; return 0xfffd; }
  if(p+n>=len && len>0){ p+=1; return 0xfffd; }
  for(FXint i=1; i<=n; i++){
    FXuchar cc=(FXuchar)text[p+i];
    if((cc&0xc0)!=0x80){ p+=1; return 0xfffd; }
    ch=(ch<<6)|(cc&0x3f);
    }
  p+=n+1;
  return ch;
  }

}


/*******************************************************************************/

#if defined(FOX_GLYPHS_AVAILABLE)

namespace FX {

// From FOX weight to fontconfig weight
static FXint weight2FcWeight(FXint weight){
  switch(weight){
    case FXFont::Thin:      return FC_WEIGHT_THIN;
    case FXFont::ExtraLight:return FC_WEIGHT_EXTRALIGHT;
    case FXFont::Light:     return FC_WEIGHT_LIGHT;
    case FXFont::Normal:    return FC_WEIGHT_NORMAL;
    case FXFont::Medium:    return FC_WEIGHT_MEDIUM;
    case FXFont::DemiBold:  return FC_WEIGHT_DEMIBOLD;
    case FXFont::Bold:      return FC_WEIGHT_BOLD;
    case FXFont::ExtraBold: return FC_WEIGHT_EXTRABOLD;
    case FXFont::Black:     return FC_WEIGHT_BLACK;
    }
  return FC_WEIGHT_NORMAL;
  }


// From FOX slant to fontconfig slant
static FXint slant2FcSlant(FXint slant){
  switch(slant){
    case FXFont::Straight:      return FC_SLANT_ROMAN;
    case FXFont::Italic:        return FC_SLANT_ITALIC;
    case FXFont::Oblique:       return FC_SLANT_OBLIQUE;
    case FXFont::ReverseItalic: return FC_SLANT_ITALIC;
    case FXFont::ReverseOblique:return FC_SLANT_OBLIQUE;
    }
  return FC_SLANT_ROMAN;
  }


// From FOX setwidth to fontconfig setwidth
static FXint setWidth2FcSetWidth(FXint setwidth){
  switch(setwidth){
    case FXFont::UltraCondensed:return FC_WIDTH_ULTRACONDENSED;
    case FXFont::ExtraCondensed:return FC_WIDTH_EXTRACONDENSED;
    case FXFont::Condensed:     return FC_WIDTH_CONDENSED;
    case FXFont::SemiCondensed: return FC_WIDTH_SEMICONDENSED;
    case FXFont::NonExpanded:   return FC_WIDTH_NORMAL;
    case FXFont::SemiExpanded:  return FC_WIDTH_SEMIEXPANDED;
    case FXFont::Expanded:      return FC_WIDTH_EXPANDED;
    case FXFont::ExtraExpanded: return FC_WIDTH_EXTRAEXPANDED;
    case FXFont::UltraExpanded: return FC_WIDTH_ULTRAEXPANDED;
    }
  return FC_WIDTH_NORMAL;
  }


/*******************************************************************************/

// Construct cache without a font
FXGlyphs::FXGlyphs():library(nullptr),face(nullptr),glyphs(nullptr),count(0),alloced(0),ascent(0),descent(0),height(0),maxadvance(0),pixelsize(0),antialias(true),hinting(true),autohint(false){
  }


// Construct cache and open the font
FXGlyphs::FXGlyphs(const FXchar* family,FXuint size,FXuint weight,FXuint slant,FXuint setwidth,FXint dpi,FXbool antialias,FXbool hinting,FXbool autohint):FXGlyphs(){
  open(family,size,weight,slant,setwidth,dpi,antialias,hinting,autohint);
  }


// Destroy the cache
FXGlyphs::~FXGlyphs(){
  close();
  }


// Close font and drop the cache
void FXGlyphs::close(){
  FXuint i;
  for(i=0; i<count; i++){
    freeElms(glyphs[i].coverage);
    }
  freeElms(glyphs);
  count=0;
  alloced=0;
  if(face){ FT_Done_Face((FT_Face)face); face=nullptr; }
  if(library){ FT_Done_FreeType((FT_Library)library); library=nullptr; }
  ascent=0;
  descent=0;
  height=0;
  maxadvance=0;
  pixelsize=0;
  filename.clear();
  fontname.clear();
  }


// Open the font matching the description
FXbool FXGlyphs::open(const FXchar* family,FXuint size,FXuint weight,FXuint slant,FXuint setwidth,FXint dpi,FXbool antialias,FXbool hinting,FXbool autohint){
  FcPattern* pattern;
  FcPattern* match;
  FcChar8* file;
  FcChar8* fam;
  FcResult result;
  FT_Face f;
  FXdouble sz;
  close();
  if(!family) family="";
  if(dpi<=0) dpi=96;
  this->antialias=antialias;
  this->hinting=hinting;
  this->autohint=autohint;
  pattern=FcPatternCreate();
  FcPatternAddBool(pattern,FC_HINTING,hinting);
  FcPatternAddBool(pattern,FC_ANTIALIAS,antialias);
  FcPatternAddBool(pattern,FC_AUTOHINT,autohint);
  FcPatternAddInteger(pattern,FC_RGBA,FC_RGBA_UNKNOWN);
  FcPatternAddInteger(pattern,FC_HINT_STYLE,FC_HINT_FULL);
  if(family[0]){
    FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)family);
    }
  if(size){
    FcPatternAddDouble(pattern,FC_PIXEL_SIZE,(dpi*(FXdouble)size)/720.0);
    }
  FcPatternAddInteger(pattern,FC_WEIGHT,weight2FcWeight(weight));
  FcPatternAddInteger(pattern,FC_SLANT,slant2FcSlant(slant));
  FcPatternAddInteger(pattern,FC_WIDTH,setWidth2FcSetWidth(setwidth));
  FcPatternAddBool(pattern,FC_SCALABLE,true);
  FcConfigSubstitute(nullptr,pattern,FcMatchPattern);
  FcDefaultSubstitute(pattern);
  match=FcFontMatch(nullptr,pattern,&result);
  FcPatternDestroy(pattern);
  if(!match) return false;
  if(FcPatternGetString(match,FC_FILE,0,&file)!=FcResultMatch){
    FcPatternDestroy(match);
    return false;
    }
  if(FcPatternGetString(match,FC_FAMILY,0,&fam)==FcResultMatch){
    fontname=(const FXchar*)fam;
    }
  filename=(const FXchar*)file;
  if(FcPatternGetDouble(match,FC_PIXEL_SIZE,0,&sz)!=FcResultMatch){
    FcPatternDestroy(match);
    return false;
    }
  pixelsize=(FXint)(sz+0.5);
  FcPatternDestroy(match);
  if(FT_Init_FreeType((FT_Library*)&library)) return false;
  if(FT_New_Face((FT_Library)library,filename.text(),0,&f)){
    close();
    return false;
    }
  face=f;
  // Size the face the way Xft does, so the metrics agree
  if(FT_Set_Char_Size(f,0,(FT_F26Dot6)(sz*64.0+0.5),0,0)){
    close();
    return false;
    }
  // Metrics of the sized face; Xft rounds up, so do the same
  ascent=(f->size->metrics.ascender+63)>>6;
  descent=(-f->size->metrics.descender+63)>>6;
  height=ascent+descent;
  maxadvance=(f->size->metrics.max_advance+63)>>6;
  return true;
  }


/*******************************************************************************/

// Load and cache the glyph of a character
FXGlyph* FXGlyphs::load(FXuint ch){
  FT_Face f=(FT_Face)face;
  FT_GlyphSlot slot;
  FT_Int32 flags=FT_LOAD_DEFAULT;
  FT_Render_Mode mode=FT_RENDER_MODE_NORMAL;
  FXGlyph g;
  FXint x,y,pitch;
  if(!f) return nullptr;
  if(!hinting) flags|=FT_LOAD_NO_HINTING;
  if(autohint) flags|=FT_LOAD_FORCE_AUTOHINT;
  if(!antialias){ flags|=FT_LOAD_TARGET_MONO; mode=FT_RENDER_MODE_MONO; }
  if(FT_Load_Glyph(f,FT_Get_Char_Index(f,ch),flags)) return nullptr;
  slot=f->glyph;
  if(FT_Render_Glyph(slot,mode)) return nullptr;
  g.ch=ch;
  g.advance=(slot->metrics.horiAdvance+32)>>6;
  g.bearing=slot->bitmap_left;
  g.top=slot->bitmap_top;
  g.width=slot->bitmap.width;
  g.height=slot->bitmap.rows;
  g.coverage=nullptr;
  if(g.width>0 && g.height>0){
    pitch=slot->bitmap.pitch;
    if(!allocElms(g.coverage,g.width*g.height)) return nullptr;
    for(y=0; y<g.height; y++){
      const FXuchar* src=(pitch<0)?slot->bitmap.buffer+(g.height-1-y)*(-pitch):slot->bitmap.buffer+y*pitch;
      FXuchar* dst=g.coverage+y*g.width;
      if(slot->bitmap.pixel_mode==FT_PIXEL_MODE_MONO){
        for(x=0; x<g.width; x++){ dst[x]=((src[x>>3]>>(7-(x&7)))&1)?255:0; }
        }
      else if(slot->bitmap.pixel_mode==FT_PIXEL_MODE_GRAY){
        for(x=0; x<g.width; x++){ dst[x]=src[x]; }
        }
      else{
        clearElms(dst,g.width);
        }
      }
    }
  if(count==alloced){
    FXuint cap=alloced?alloced*2:128;
    if(!resizeElms(glyphs,cap)){ freeElms(g.coverage); return nullptr; }
    alloced=cap;
    }
  glyphs[count]=g;
  return &glyphs[count++];
  }


// Return the cached glyph of a character
const FXGlyph* FXGlyphs::getGlyph(FXuint ch){
  FXuint i;
  for(i=0; i<count; i++){
    if(glyphs[i].ch==ch) return &glyphs[i];
    }
  return load(ch);
  }


// Return true if the font has a glyph for the character
FXbool FXGlyphs::hasChar(FXuint ch){
  if(!face) return false;
  return FT_Get_Char_Index((FT_Face)face,ch)!=0;
  }


// Return the advance width of a character
FXint FXGlyphs::getCharWidth(FXuint ch){
  const FXGlyph* g=getGlyph(ch);
  return g?g->advance:0;
  }


// Return the width of a UTF-8 string
FXint FXGlyphs::getTextWidth(const FXchar* text,FXint len){
  FXint p=0;
  FXint width=0;
  if(!text) return 0;
  if(len<0) len=(FXint)strlen(text);
  while(p<len){
    width+=getCharWidth(fxutf8decode(text,len,p));
    }
  return width;
  }


// Return the character index at pixel position x
FXint FXGlyphs::getCharAt(const FXchar* text,FXint len,FXint x){
  FXint p=0;
  FXint index=0;
  FXint width=0;
  if(!text) return 0;
  if(len<0) len=(FXint)strlen(text);
  while(p<len){
    width+=getCharWidth(fxutf8decode(text,len,p));
    if(x<width) return index;
    index++;
    }
  return index;
  }

#else

/*******************************************************************************/

namespace FX {

// Without FreeType and fontconfig there is no glyph cache; the class keeps its
// interface so the rasterizer links, but no font can be opened

FXGlyphs::FXGlyphs():library(nullptr),face(nullptr),glyphs(nullptr),count(0),alloced(0),ascent(0),descent(0),height(0),maxadvance(0),pixelsize(0),antialias(true),hinting(true),autohint(false){
  }


FXGlyphs::FXGlyphs(const FXchar*,FXuint,FXuint,FXuint,FXuint,FXint,FXbool,FXbool,FXbool):FXGlyphs(){
  }


FXGlyphs::~FXGlyphs(){
  }


void FXGlyphs::close(){
  }


FXbool FXGlyphs::open(const FXchar*,FXuint,FXuint,FXuint,FXuint,FXint,FXbool,FXbool,FXbool){
  return false;
  }


FXGlyph* FXGlyphs::load(FXuint){
  return nullptr;
  }


const FXGlyph* FXGlyphs::getGlyph(FXuint){
  return nullptr;
  }


FXbool FXGlyphs::hasChar(FXuint){
  return false;
  }


FXint FXGlyphs::getCharWidth(FXuint){
  return 0;
  }


FXint FXGlyphs::getTextWidth(const FXchar*,FXint){
  return 0;
  }


FXint FXGlyphs::getCharAt(const FXchar*,FXint,FXint){
  return 0;
  }

#endif

}
