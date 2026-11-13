/********************************************************************************
*                                                                               *
*                     F o n t   O b j e c t   ( W i n 3 2 )                     *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXRegion.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"

#if defined(WIN32)

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1012

/*
  Notes:

  - Win32 (GDI) face matching, font realization, font enumeration and text
    metrics.  The public FXFont API stays in FXFont.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


static const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
static const FXint TAIL_OFFSET=0xDC00;



// Character set encoding
static BYTE FXFontEncoding2CharSet(FXuint encoding){
  switch(encoding){
    case FONTENCODING_USASCII: return ANSI_CHARSET;
    case FONTENCODING_ARABIC: return ARABIC_CHARSET;
    case FONTENCODING_BALTIC: return BALTIC_CHARSET;
    case FONTENCODING_EASTEUROPE: return EASTEUROPE_CHARSET;
    case FONTENCODING_GREEK: return GREEK_CHARSET;
    case FONTENCODING_HEBREW: return HEBREW_CHARSET;
    case FONTENCODING_CYRILLIC: return RUSSIAN_CHARSET;
    case FONTENCODING_TURKISH: return TURKISH_CHARSET;
    case FONTENCODING_THAI: return THAI_CHARSET;
    case FONTENCODING_CP1258: return VIETNAMESE_CHARSET;
    case FONTENCODING_DEFAULT: return DEFAULT_CHARSET;
    //case FONTENCODING_UNICODE: return ANSI_CHARSET;
    }
  return DEFAULT_CHARSET;
  }


// Character set encoding
static FXuint CharSet2FXFontEncoding(BYTE lfCharSet){
  switch(lfCharSet){
    case ANSI_CHARSET: return FONTENCODING_USASCII;
    case ARABIC_CHARSET: return FONTENCODING_ARABIC;
    case BALTIC_CHARSET: return FONTENCODING_BALTIC;
    case CHINESEBIG5_CHARSET: return FONTENCODING_DEFAULT;
    case DEFAULT_CHARSET: return FONTENCODING_DEFAULT;
    case EASTEUROPE_CHARSET: return FONTENCODING_EASTEUROPE;
    case GB2312_CHARSET: return FONTENCODING_DEFAULT;
    case GREEK_CHARSET: return FONTENCODING_GREEK;
    case HANGUL_CHARSET: return FONTENCODING_DEFAULT;
    case HEBREW_CHARSET: return FONTENCODING_HEBREW;
    case MAC_CHARSET: return FONTENCODING_DEFAULT;
    case OEM_CHARSET: return FONTENCODING_DEFAULT;
    case SYMBOL_CHARSET: return FONTENCODING_DEFAULT;
    case RUSSIAN_CHARSET: return FONTENCODING_CYRILLIC;
    case SHIFTJIS_CHARSET: return FONTENCODING_DEFAULT;
    case THAI_CHARSET: return FONTENCODING_THAI;
    case TURKISH_CHARSET: return FONTENCODING_TURKISH;
    case VIETNAMESE_CHARSET: return FONTENCODING_CP1258;
    }
  return FONTENCODING_DEFAULT;
  }


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  FXTRACE(TOPIC_DETAIL,"FXFont::match: wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res);
  TEXTMETRIC *font;
  LOGFONT lf;
  FXchar buffer[256];

  // Hang on to this for text metrics functions
  dc=CreateCompatibleDC(nullptr);

  // Font resolution; an explicit monitor DPI or the screen DC
  if(res<=0) res=GetDeviceCaps((HDC)dc,LOGPIXELSY);
  if(res<=0) res=96;

  // Now fill in the fields
  lf.lfHeight=-MulDiv(wantsize,res,720);
  lf.lfWidth=0;
  if(wanthints&FXFont::Rotatable){
    lf.lfEscapement=(angle*10)/64;
    lf.lfOrientation=(angle*10)/64;
    }
  else{
    lf.lfEscapement=0;
    lf.lfOrientation=0;
    }
  lf.lfWeight=wantweight*10;
  if((wantslant==FXFont::Italic) || (wantslant==FXFont::Oblique))
    lf.lfItalic=true;
  else
    lf.lfItalic=false;
  lf.lfUnderline=false;
  lf.lfStrikeOut=false;

  // Character set encoding
  lf.lfCharSet=FXFontEncoding2CharSet(wantencoding);

  // Other hints
  lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
  if(wanthints&FXFont::System) lf.lfOutPrecision=OUT_RASTER_PRECIS;
  if(wanthints&FXFont::Scalable) lf.lfOutPrecision=OUT_TT_PRECIS;
  if(wanthints&FXFont::Polymorphic) lf.lfOutPrecision=OUT_TT_PRECIS;

  // Clip precision
  lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;

  // Quality
  lf.lfQuality=DEFAULT_QUALITY;

  // Pitch and Family
  lf.lfPitchAndFamily=0;

  // Pitch
  if(wanthints&FXFont::Variable) lf.lfPitchAndFamily|=VARIABLE_PITCH;
  else if(wanthints&FXFont::Mono) lf.lfPitchAndFamily|=MONO_FONT;
  else if(wanthints&FXFont::Cell) lf.lfPitchAndFamily|=FIXED_PITCH;
  else lf.lfPitchAndFamily|=DEFAULT_PITCH;

  // Family
  if(wanthints&FXFont::Decorative) lf.lfPitchAndFamily|=FF_DECORATIVE;
  else if(wanthints&FXFont::Modern) lf.lfPitchAndFamily|=FF_MODERN;
  else if(wanthints&FXFont::Roman) lf.lfPitchAndFamily|=FF_ROMAN;
  else if(wanthints&FXFont::Script) lf.lfPitchAndFamily|=FF_SCRIPT;
  else if(wanthints&FXFont::Swiss) lf.lfPitchAndFamily|=FF_SWISS;
  else lf.lfPitchAndFamily|=FF_DONTCARE;

  // Font substitution
#ifdef UNICODE
  utf2ncs(lf.lfFaceName,wantfamily.text(),LF_FACESIZE);
#else
  fxstrlcpy(lf.lfFaceName,wantfamily.text(),LF_FACESIZE);
#endif

  // Here we go!
  xid=CreateFontIndirect(&lf);

  // Uh-oh, we failed
  if(!xid) return nullptr;

  // Obtain text metrics
  if(!callocElms(font,1)) return nullptr;

  SelectObject((HDC)dc,xid);
  GetTextMetrics((HDC)dc,(TEXTMETRIC*)font);

  // Get actual face name
  GetTextFaceA((HDC)dc,sizeof(buffer),buffer);
  actualName=buffer;
  actualSize=MulDiv(((TEXTMETRIC*)font)->tmHeight,720,res);
  actualWeight=((TEXTMETRIC*)font)->tmWeight/10;
  actualSlant=((TEXTMETRIC*)font)->tmItalic?FXFont::Italic:FXFont::Straight;
  actualSetwidth=0;
  actualEncoding=CharSet2FXFontEncoding(((TEXTMETRIC*)font)->tmCharSet);

  // Return it
  return font;
  }




// Create font
void FXFont::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);


      FXString family=getFamily();

      FXTRACE(TOPIC_CREATION,"%s::create: win32 font\n",getClassName());

      // Try to match with specified family and foundry
      if(!family.empty()){
        family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
        font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,resolution);
        }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }


      // What was really matched
      FXTRACE(TOPIC_CREATION,"wantedName=%s wantedSize=%d wantedWeight=%d wantedSlant=%d wantedSetwidth=%d wantedEncoding=%d\n",wantedName.text(),wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding);
      FXTRACE(TOPIC_CREATION,"actualName=%s actualSize=%d actualWeight=%d actualSlant=%d actualSetwidth=%d actualEncoding=%d\n",actualName.text(),actualSize,actualWeight,actualSlant,actualSetwidth,actualEncoding);
      }
    }
  }


// Detach font
void FXFont::detach(){
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);


    // Free font metrics
    freeElms(font);


    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=nullptr;
    xid=0;
    }
  }


// Destroy font
void FXFont::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);


      // Necessary to prevent resource leak
      SelectObject((HDC)dc,GetStockObject(SYSTEM_FONT));

      // Delete font
      DeleteObject((HFONT)xid);

      // Delete dummy DC
      DeleteDC((HDC)dc);

      // Free font metrics
      freeElms(font);

      }

    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=nullptr;
    xid=0;
    }
  }


// Function to sort by name, weight, slant, and size
static int CDECL comparefont(const void *a,const void *b){
  const FXFontDesc *fa=(const FXFontDesc*)a;
  const FXFontDesc *fb=(const FXFontDesc*)b;
  FXint cmp=strcmp(fa->face,fb->face);
  return cmp ? cmp : (fa->weight!=fb->weight) ? fa->weight-fb->weight : (fa->slant!=fb->slant) ? fa->slant-fb->slant : fa->size-fb->size;
  }



// Need to get some data into the callback function.
struct FXFontStore {
  HDC         hdc;
  FXFontDesc *fonts;
  FXuint      numfonts;
  FXuint      size;
  FXFontDesc  desc;
  };


// Callback function for EnumFontFamiliesEx()
static int CALLBACK EnumFontFamExProc(const LOGFONTA *lf,const TEXTMETRICA *lptm,DWORD FontType,LPARAM lParam){
  FXFontStore *pFontStore=(FXFontStore*)lParam;
  FXASSERT(lf);
  FXASSERT(lptm);
  FXASSERT(pFontStore);

  // Get pitch
  FXuint flags=0;
  if(lf->lfPitchAndFamily&MONO_FONT) flags|=FXFont::Mono;
  if(lf->lfPitchAndFamily&FIXED_PITCH) flags|=FXFont::Cell;
  if(lf->lfPitchAndFamily&VARIABLE_PITCH) flags|=FXFont::Variable;

  // Get hints
  if(lf->lfPitchAndFamily&FF_MODERN) flags|=FXFont::Modern;
  if(lf->lfPitchAndFamily&FF_ROMAN) flags|=FXFont::Roman;
  if(lf->lfPitchAndFamily&FF_SCRIPT) flags|=FXFont::Script;
  if(lf->lfPitchAndFamily&FF_DECORATIVE) flags|=FXFont::Decorative;
  if(lf->lfPitchAndFamily&FF_SWISS) flags|=FXFont::Swiss;
  if(lf->lfPitchAndFamily&FF_DONTCARE) flags&=~(FXFont::Modern|FXFont::Roman|FXFont::Script|FXFont::Decorative|FXFont::Swiss);

  // Skip if no match
  FXuint h=pFontStore->desc.flags;
  if((h&FXFont::Mono) && !(flags&FXFont::Mono)) return 1;
  if((h&FXFont::Variable) && !(flags&FXFont::Variable)) return 1;

  // Get weight (also guess from the name)
  FXuint weight=lf->lfWeight/10;
  if(!strstr(lf->lfFaceName,"Bold")) weight=FXFont::Bold;
  if(!strstr(lf->lfFaceName,"Black")) weight=FXFont::Black;
  if(!strstr(lf->lfFaceName,"Demi")) weight=FXFont::DemiBold;
  if(!strstr(lf->lfFaceName,"Light")) weight=FXFont::Light;
  if(!strstr(lf->lfFaceName,"Medium")) weight=FXFont::Medium;

  // Skip if weight doesn't match
  FXuint wt=pFontStore->desc.weight;
  if((wt!=0) && (wt!=weight)) return 1;

  // Get slant
  FXuint slant=FXFont::Straight;
  if(lf->lfItalic) slant=FXFont::Italic;
  if(!strstr(lf->lfFaceName,"Italic")) slant=FXFont::Italic;
  if(!strstr(lf->lfFaceName,"Roman")) slant=FXFont::Straight;

  // Skip if no match
  FXuint sl=pFontStore->desc.slant;
  if((sl!=0) && (sl!=slant)) return 1;

  // Get set width (also guess from the name)
  FXuint setwidth=0;
  if(!strstr(lf->lfFaceName,"Cond")) setwidth=FXFont::Condensed;
  if(!strstr(lf->lfFaceName,"Narrow")) setwidth=FXFont::Condensed;
  if(!strstr(lf->lfFaceName,"Ext Cond")) setwidth=FXFont::ExtraCondensed;

  // Skip if no match
  FXuint sw=pFontStore->desc.setwidth;
  if((sw!=0) && (sw!=setwidth)) return 1;

  // Get encoding
  FXuint encoding=CharSet2FXFontEncoding(lf->lfCharSet);

  // Skip if no match
  FXuint en=pFontStore->desc.encoding;
  if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) return 1;

  // Is it scalable?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FXFont::Scalable;
    }

  // Is it polymorphic?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FXFont::Polymorphic;
    }

  // Initial allocation of storage?
  if(pFontStore->numfonts==0){
    allocElms(pFontStore->fonts,50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size=50;
    }

  // Grow the array if needed
  if(pFontStore->numfonts>=pFontStore->size){
    resizeElms(pFontStore->fonts,pFontStore->size+50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size+=50;
    }

  FXFontDesc *fonts=pFontStore->fonts;
  FXuint numfonts=pFontStore->numfonts;

  fxstrlcpy(fonts[numfonts].face,lf->lfFaceName,116);
  if(lf->lfHeight<0){
    fonts[numfonts].size=-MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  else{
    fonts[numfonts].size=MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  fonts[numfonts].weight=weight;
  fonts[numfonts].slant=slant;
  fonts[numfonts].encoding=encoding;
  fonts[numfonts].setwidth=setwidth;
  fonts[numfonts].flags=flags;

  pFontStore->fonts=fonts;
  pFontStore->numfonts++;

  // Must return 1 to continue enumerating fonts
  return 1;
  }


// List all fonts matching hints
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  FXuint i,j;

  // Initialize return values
  fonts=nullptr;
  numfonts=0;

  // This data gets passed into the callback function
  FXFontStore fontStore;
  HDC hdc=GetDC(GetDesktopWindow());
  SaveDC(hdc);
  fontStore.hdc=hdc;
  fontStore.fonts=fonts;
  fontStore.numfonts=numfonts;
  fontStore.desc.weight=wt;
  fontStore.desc.slant=sl;
  fontStore.desc.setwidth=sw;
  fontStore.desc.encoding=en;
  fontStore.desc.flags=h;

  // Fill in the appropriate fields of the LOGFONT structure. Note that
  // EnumFontFamiliesEx() only examines the lfCharSet, lfFaceName and
  // lpPitchAndFamily fields of this struct.
  LOGFONTA lf;
  lf.lfHeight=0;
  lf.lfWidth=0;
  lf.lfEscapement=0;
  lf.lfOrientation=0;
  lf.lfWeight=0;
  lf.lfItalic=0;
  lf.lfUnderline=0;
  lf.lfStrikeOut=0;
  lf.lfCharSet=FXFontEncoding2CharSet(en);
  lf.lfOutPrecision=0;
  lf.lfClipPrecision=0;
  lf.lfQuality=0;
  lf.lfPitchAndFamily=0;                          // Should be MONO_FONT for Hebrew and Arabic?
  FXASSERT(face.length()<LF_FACESIZE);
  fxstrlcpy(lf.lfFaceName,face.text(),LF_FACESIZE);

  // Start enumerating!
  EnumFontFamiliesExA(hdc,&lf,EnumFontFamExProc,(LPARAM)&fontStore,0);
  RestoreDC(hdc,-1);
  ReleaseDC(GetDesktopWindow(),hdc);

  // Copy stuff back from the store
  fonts=fontStore.fonts;
  numfonts=fontStore.numfonts;

  // Any fonts found?
  if(numfonts==0){
    freeElms(fonts);
    return false;
    }

  // Sort them by name, weight, slant, and size respectively
  ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);

  // Weed out duplicates if we were just listing the face names
  if(lf.lfCharSet==DEFAULT_CHARSET && lf.lfFaceName[0]==0){
    i=j=1;
    while(j<numfonts){
      if(strcmp(fonts[i-1].face,fonts[j].face)!=0){
        fonts[i]=fonts[j];
        i++;
        }
      j++;
      }
    numfonts=i;
    }

  // Realloc to shrink the block
  resizeElms(fonts,numfonts);

//   FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: %d fonts:\n",numfonts);
//   for(FXuint f=0; f<numfonts; f++){
//     FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: Font=%s weight=%d slant=%d size=%3d setwidth=%d encoding=%d\n",fonts[f].face,fonts[f].weight,fonts[f].slant,fonts[f].size,fonts[f].setwidth,fonts[f].encoding);
//     }
//   FXTRACE(TOPIC_DETAIL,"\n\n");

  return true;
  }


// Does font have given character glyph?
FXbool FXFont::hasChar(FXwchar ch) const {
  if(font){
    // FIXME may want to use GetGlyphIndices()
    return ((TEXTMETRIC*)font)->tmFirstChar<=ch && ch<=((TEXTMETRIC*)font)->tmLastChar;
    }
  return false;
  }


// Get first character glyph in font
FXwchar FXFont::getMinChar() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmFirstChar;
    }
  return 0;
  }


// Get last character glyph in font
FXwchar FXFont::getMaxChar() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmLastChar;
    }
  return 0;
  }


// Get font leading [that is lead-ing as in Pb!]
FXint FXFont::getFontLeading() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmExternalLeading;
    }
  return 0;
  }


// Get font line spacing [height+leading]
FXint FXFont::getFontSpacing() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmHeight;               // Includes font point size plus internal leading
    }
  return 1;
  }


// Left bearing
FXint FXFont::leftBearing(FXwchar ch) const {
  if(font){
    return 0;                                           // FIXME
    }
  return 0;
  }


// Right bearing
FXint FXFont::rightBearing(FXwchar ch) const {
  if(font){
    return 0;                                           // FIXME
    }
  return 0;
  }


// Is it a mono space font
FXbool FXFont::isFontMono() const {
  if(font){
    return !(((TEXTMETRIC*)font)->tmPitchAndFamily&TMPF_FIXED_PITCH);
    }
  return true;
  }


// Get font width
FXint FXFont::getFontWidth() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmMaxCharWidth;
    }
  return 1;
  }


// Get font height
FXint FXFont::getFontHeight() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmHeight;
    }
  return 1;
  }


// Get font ascent
FXint FXFont::getFontAscent() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmAscent;
    }
  return 1;
  }


// Get font descent
FXint FXFont::getFontDescent() const {
  if(font){
    return ((TEXTMETRIC*)font)->tmDescent;
    }
  return 0;
  }


// Calculate width of single wide character in this font
FXint FXFont::getCharWidth(const FXwchar ch) const {
  if(font){
    FXnchar sbuffer[2]; SIZE size;
    if(ch<0xFFFF){
      sbuffer[0]=ch;
      GetTextExtentPoint32W((HDC)dc,sbuffer,1,&size);
      return size.cx;
      }
    sbuffer[0]=(ch>>10)+LEAD_OFFSET;    // Deal with surrogate pair
    sbuffer[1]=(ch&0x3FF)+TAIL_OFFSET;
    GetTextExtentPoint32W((HDC)dc,sbuffer,2,&size);
    return size.cx;
    }
  return 1;
  }


// Text width
FXint FXFont::getTextWidth(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextWidth: NULL string argument\n",getClassName()); }
  if(font){
    SIZE size;
    FXint count;
    if(length<=4096){
      FXnchar sbuffer[4096];
      count=utf2ncs(sbuffer,string,4096,length);
      GetTextExtentPoint32W((HDC)dc,sbuffer,count,&size);
      }
    else{
      FXnchar* dbuffer=nullptr;
      if(!allocElms(dbuffer,length)){ throw FXResourceException("unable allocate memory in FXFont."); }
      count=utf2ncs(dbuffer,string,length,length);
      GetTextExtentPoint32W((HDC)dc,dbuffer,count,&size);
      freeElms(dbuffer);
      }
    return size.cx;
    }
  return length;
  }


// Text height
FXint FXFont::getTextHeight(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextHeight: NULL string argument\n",getClassName()); }
  if(font){
//    SIZE size;
//    FXASSERT(dc);
//    GetTextExtentPoint32((HDC)dc,string,length,&size);
//    return size.cy;
    return ((TEXTMETRIC*)font)->tmHeight;
    }
  return 1;
  }


}

#endif
