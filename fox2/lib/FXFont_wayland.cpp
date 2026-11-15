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

#if defined(HAVE_XFT_H) || defined(FOX_HAVE_FREETYPE)
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#endif

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


#if defined(HAVE_XFT_H) || defined(FOX_HAVE_FREETYPE)


// Function to sort by name, weight, slant, and size
static int CDECL comparefont(const void *a,const void *b){
  const FXFontDesc *fa=(const FXFontDesc*)a;
  const FXFontDesc *fb=(const FXFontDesc*)b;
  FXint cmp=strcmp(fa->face,fb->face);
  return cmp ? cmp : (fa->weight!=fb->weight) ? fa->weight-fb->weight : (fa->slant!=fb->slant) ? fa->slant-fb->slant : fa->size-fb->size;
  }


// From fontconfig weight to FOX weight
static FXint fcWeight2Weight(FXint fcWeight){
  switch(fcWeight){
    case FC_WEIGHT_THIN:      return FXFont::Thin;
    case FC_WEIGHT_EXTRALIGHT:return FXFont::ExtraLight;
    case FC_WEIGHT_LIGHT:     return FXFont::Light;
    case FC_WEIGHT_NORMAL:    return FXFont::Normal;
    case FC_WEIGHT_MEDIUM:    return FXFont::Medium;
    case FC_WEIGHT_DEMIBOLD:  return FXFont::DemiBold;
    case FC_WEIGHT_BOLD:      return FXFont::Bold;
    case FC_WEIGHT_EXTRABOLD: return FXFont::ExtraBold;
    case FC_WEIGHT_BLACK:     return FXFont::Black;
    }
  return FXFont::Normal;
  }


// From fontconfig setwidth to FOX setwidth
static FXint fcSetWidth2SetWidth(FXint fcSetWidth){
  switch(fcSetWidth){
    case FC_WIDTH_ULTRACONDENSED:return FXFont::UltraCondensed;
    case FC_WIDTH_EXTRACONDENSED:return FXFont::ExtraCondensed;
    case FC_WIDTH_CONDENSED:     return FXFont::Condensed;
    case FC_WIDTH_SEMICONDENSED: return FXFont::SemiCondensed;
    case FC_WIDTH_NORMAL:        return FXFont::NonExpanded;
    case FC_WIDTH_SEMIEXPANDED:  return FXFont::SemiExpanded;
    case FC_WIDTH_EXPANDED:      return FXFont::Expanded;
    case FC_WIDTH_EXTRAEXPANDED: return FXFont::ExtraExpanded;
    case FC_WIDTH_ULTRAEXPANDED: return FXFont::UltraExpanded;
    }
  return FXFont::NonExpanded;
  }


// From fontconfig slant to FOX slant
static FXint fcSlant2Slant(FXint fcSlant){
  switch(fcSlant){
    case FC_SLANT_ROMAN:  return FXFont::Straight;
    case FC_SLANT_ITALIC: return FXFont::Italic;
    case FC_SLANT_OBLIQUE:return FXFont::Oblique;
    }
  return FXFont::Straight;
  }


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


// List all fonts that match the passed requirements; the same fontconfig
// query the Xft path uses, so the selector lists the same faces
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  int          encoding,setwidth,weight,slant,size,pitch,scalable,res,i,j;
  FXchar       fullname[256];
  FcPattern   *pattern,*p;
  FcObjectSet *objset;
  FcFontSet   *fontset;
  FcChar8     *fam,*fdy;
  FcBool       scale;
  FXdouble     points;
  FXString     family;
  FXString     foundry;

  FXUNUSED(en);

  fonts=nullptr;
  numfonts=0;

  // Need to have application
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }

  // Get family part of name
  family=face.before('[').trimEnd();

  // Get foundry part of name
  foundry=face.section("[]",1);

  FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: family=\"%s\" foundry=\"%s\" weight=%d slant=%d setwidth=%d encoding=%d hints=%x\n",family.text(),foundry.text(),wt,sl,sw,en,h);

  // Screen resolution may be overidden by registry
  res=FXApp::instance()->reg().readUIntEntry("SETTINGS","screenres",100);

  // Build object set
  objset=FcObjectSetBuild(FC_FAMILY,FC_FOUNDRY,FC_SPACING,FC_SCALABLE,FC_WIDTH,FC_WEIGHT,FC_SLANT,FC_PIXEL_SIZE,nullptr);
  if(objset){

    // Create pattern object
    pattern=FcPatternCreate();
    if(pattern){

      // Set family
      if(!family.empty()){
        FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)family.text());
        }

      // Set foundry
      if(!foundry.empty()){
        FcPatternAddString(pattern,FC_FOUNDRY,(const FcChar8*)foundry.text());
        }

      // List fonts matching pattern
      fontset=FcFontList(0,pattern,objset);
      if(fontset && 0<fontset->nfont){

        // Allocate return array
        if(allocElms(fonts,fontset->nfont)){

          // Collect the info now...
          for(i=0; i<fontset->nfont; i++){
            p=fontset->fonts[i];

            // Get full face name
            fullname[0]=0;
            if(FcPatternGetString(p,FC_FAMILY,0,&fam)==FcResultMatch){
              fxstrlcpy(fullname,(const char*)fam,sizeof(fullname));
              if(FcPatternGetString(p,FC_FOUNDRY,0,&fdy)==FcResultMatch){
                fxstrlcat(fullname," [",sizeof(fullname));
                fxstrlcat(fullname,(const char*)fdy,sizeof(fullname));
                fxstrlcat(fullname,"]",sizeof(fullname));
                }
              }

            // Get setwidth
            setwidth=0;
            if(FcPatternGetInteger(p,FC_WIDTH,0,&setwidth)==FcResultMatch){
              setwidth=fcSetWidth2SetWidth(setwidth);
              }

            // Get weight
            weight=0;
            if(FcPatternGetInteger(p,FC_WEIGHT,0,&weight)==FcResultMatch){
              weight=fcWeight2Weight(weight);
              }

            // Get slant
            slant=0;
            if(FcPatternGetInteger(p,FC_SLANT,0,&slant)==FcResultMatch){
              slant=fcSlant2Slant(slant);
              }

            // Get pitch
            pitch=FXFont::Variable;
            if(FcPatternGetInteger(p,FC_SPACING,0,&pitch)==FcResultMatch){
              if(pitch==FC_PROPORTIONAL) pitch=FXFont::Variable;
              else if(pitch==FC_MONO) pitch=FXFont::Mono;
              else if(pitch==FC_CHARCELL) pitch=FXFont::Cell;
              else if(pitch==FC_DUAL) pitch=FXFont::Dual;
              }

            // Pixel size works for both bitmap and scalable fonts
            size=0;
            if(FcPatternGetDouble(p,FC_PIXEL_SIZE,0,&points)==FcResultMatch){
              size=(int)((720.0*points)/res);
              }

            // Get scalable flag
            scalable=0;
            if(FcPatternGetBool(p,FC_SCALABLE,0,&scale)==FcResultMatch){
              if(scale) scalable=FXFont::Scalable;
              }

            // Get the encoding
            encoding=FONTENCODING_UNICODE;

            // Skip if pitch does not match
            if((h&FXFont::Mono) && (pitch!=FXFont::Mono)) continue;
            if((h&FXFont::Dual) && (pitch!=FXFont::Dual)) continue;
            if((h&FXFont::Variable) && (pitch!=FXFont::Variable)) continue;

            // Skip if weight does not match
            if((wt!=0) && (wt!=(FXuint)weight)) continue;

            // Skip if slant does not match
            if((sl!=0) && (sl!=(FXuint)slant)) continue;

            // Skip if setwidth does not match
            if((sw!=0) && (sw!=(FXuint)setwidth)) continue;

            // Want scalable
            if((h&FXFont::Scalable) && (scalable!=FXFont::Scalable)) continue;

            // If NULL face name, just list one of each face
            if(family.empty()){
              for(j=numfonts-1; j>=0; j--){
                if(strcmp(fullname,fonts[j].face)==0) goto next;
                }
              }

            // Add this font
            fxstrlcpy(fonts[numfonts].face,fullname,116);
            fonts[numfonts].size=size;
            fonts[numfonts].weight=weight;
            fonts[numfonts].slant=slant;
            fonts[numfonts].encoding=encoding;
            fonts[numfonts].setwidth=setwidth;
            fonts[numfonts].flags=pitch|scalable;
            numfonts++;

            // Next font
next:       continue;
            }

          // Realloc to shrink the block
          resizeElms(fonts,numfonts);

          // Sort them by name, weight, slant, and size respectively
          ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);
          }
        FcFontSetDestroy(fontset);
        }
      FcPatternDestroy(pattern);
      }
    FcObjectSetDestroy(objset);
    }
  return (0<numfonts);
  }


#else


// List fonts; without fontconfig there is nothing to enumerate
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString&,FXuint,FXuint,FXuint,FXuint,FXuint){
  fonts=nullptr;
  numfonts=0;
  return false;
  }


#endif


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
