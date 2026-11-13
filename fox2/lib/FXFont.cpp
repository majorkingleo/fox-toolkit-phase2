/********************************************************************************
*                                                                               *
*                               F o n t   O b j e c t                           *
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

/*
  Notes:

  - Interpretation of the hints:

      0                    No preference for pitch
      FXFont::Mono         If specified, match for fixed pitch fonts are strongly preferred
      FXFont::Variable     If specified, match for variable pitch font strongly preferred
      FXFont::Dual         If specified, match for dual pitch font strongly preferred

      0                    No hints given
      FXFont::Decorative   Ye Olde Fonte
      FXFont::Modern       Monospace fonts such as courier and so on
      FXFont::Roman        Serif font such as times
      FXFont::Script       Cursive font/script
      FXFont::Swiss        Sans serif font such as swiss, helvetica, arial
      FXFont::System       Raster based fonts, typically monospaced

      FXFont::X11          Force X11 raw font specification

      FXFont::Scalable     Strong emphasis on scalable fonts; under Windows, this means
                           TrueType fonts are desired

      FXFont::Polymorphic  Strong emphasis on polymorphic fonts; under Windows, this means
                           TrueType fonts desired also

  - FONTENCODING_DEFAULT means we prefer the fonts for the current locale;
    currently, this is hardwired to iso8859-1 until we have some means of
    determining the preferred encoding from the locale.

  - FXFont::Italic is a cursive typeface for some fonts; FXFont::Oblique is the same
    basic font written at an angle; for many fonts, FXFont::Italic and FXFont::Oblique
    means pretty much the same thing.  When matching, FXFont::Italic and FXFont::Oblique
    are considered closer to each other than FXFont::Straight.

  - A weight value of 0 indicates preference for non-bold font.

  - XFontStruct.ascent+XFontStruct.descent is the height of the font, as far as line
    spacing goes.  XFontStruct.max_bounds.ascent+XFontStruct.max_bounds.descent is
    larger, as some characters can apparently extend beyond ascent or descent!!

  - Registry section FONTSUBSTITUTIONS can be used to map typeface names to platform
    specific typeface names:

        [FONTSUBSTITUTIONS]
        arial = helvetica
        swiss = helvetica

    This allows you to change fonts in programs with hard-wired fonts.

  - Text txfm matrix [a b c d] premultiplies.

  - Should we perhaps build our own tables of font metrics? This might make
    things simpler for the advanced stuff, and be conceivably a lot faster
    under MS-Windows [no need to SelectObject() all the time just to get some
    info; also, this could be useful in case the drawing surface is not a
    window].

  - FOR THE MOMENT we're creating a dummy DC to keep the font locked into the GDI
    for MUCH quicker access to text metrics.  Soon however we want to just build
    our own font metrics tables and determine the metrics entirely with client-side
    code.  This will [predictably] be the fastest possible method as it will not
    involve context switches...

  - Matching algorithm slightly favors bitmapped fonts over scalable ones [as the
    latter may not be optimized easthetically; also, the matching algorithm should
    not weight resolution as much.

  - UNICODE means registry and encoding are set to iso10646-1

  - More human-readable font strings (e.g. registry):

       family [foundry],size,weight,slant,setwidth,encoding,hints

    For example:

       times [urw],120,bold,i,normal,iso8859-1,0

    Note that the size is in decipoints!

  - Get encoding from locale (see X11).

*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1012

using namespace FX;

/*******************************************************************************/

namespace FX {


static const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
static const FXint TAIL_OFFSET=0xDC00;


/*******************************************************************************/

// Object implementation
FXIMPLEMENT(FXFont,FXId,nullptr,0)


// Deserialization
FXFont::FXFont(){
  FXTRACE(TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this);
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  flags=0;
  angle=0;
  resolution=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Construct font from given font description
FXFont::FXFont(FXApp* a,const FXString& string):FXId(a){
  FXTRACE(TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this);
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  flags=0;
  angle=0;
  resolution=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  setFont(string);
  }


// Construct a font with given family name, size in points, weight, slant, character set encoding, setwidth, and hints
FXFont::FXFont(FXApp* a,const FXString& face,FXuint size,FXuint weight,FXuint slant,FXuint encoding,FXuint setwidth,FXuint h):FXId(a),wantedName(face){
  FXTRACE(TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this);
  wantedSize=10*size;
  wantedWeight=weight;
  wantedSlant=slant;
  wantedSetwidth=setwidth;
  wantedEncoding=encoding;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=(h&~FXFont::X11);          // System-independent method
  flags=0;
  angle=0;
  resolution=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Construct font from font description
FXFont::FXFont(FXApp* a,const FXFontDesc& fontdesc):FXId(a),wantedName(fontdesc.face){
  FXTRACE(TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this);
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=fontdesc.flags;
  flags=0;
  angle=0;
  resolution=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Return family part of name
FXString FXFont::getFamily() const {
  return wantedName.before('[').trimEnd();
  }


// Return foundry part of name
FXString FXFont::getFoundry() const {
  return wantedName.section("[]",1);
  }


/*******************************************************************************/


// Set to new angle, in degrees*64 relative to positive x axis
void FXFont::setAngle(FXint ang){
  if(xid){ fxerror("%s::setAngle: font has already been created.\n",getClassName()); }
  angle=(ang+34560)%23040-11520;
  if(angle!=ang){
    angle=ang;
    }
  }

/*
WINGDIAPI DWORD WINAPI GetGlyphIndices(
  HDC hdc,       // handle to DC
  LPCTSTR lpstr, // string to convert
  int c,         // number of characters in string
  LPWORD pgi,    // array of glyph indices
  DWORD fl       // glyph options
);
Parameters
hdc
[in] Handle to the device context.
lpstr
[in] Pointer to the string to be converted.
c
[in] Number of characters in pgi.
pgi
[out] Array of glyph indices corresponding to the characters in the string.
fl
[in] Specifies how glyphs should be handled if they are not supported. This parameter can be the following value. Value Meaning
GGI_MARK_NONEXISTING_GLYPHS Marks unsupported glyphs with the hexadecimal value 0xffff.
*/

// Does font have given character glyph?
FXbool FXFont::hasChar(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    // FIXME may want to use GetGlyphIndices()
    return ((TEXTMETRIC*)font)->tmFirstChar<=ch && ch<=((TEXTMETRIC*)font)->tmLastChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return XftCharExists(DISPLAY(getApp()),(XftFont*)font,ch);
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    const XCharStruct *cm;
    FXuchar row=ch>>8;
    FXuchar col=ch&255;
    if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
      if(!fs->per_char) return true;
      cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
      if(cm->width || cm->ascent || cm->descent || cm->rbearing || cm->lbearing) return true;
      }
#endif
    }
  return false;
  }


// Get first character glyph in font
FXwchar FXFont::getMinChar() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmFirstChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    return (((XFontStruct*)font)->min_byte1<<8)|((XFontStruct*)font)->min_char_or_byte2;
#endif
    }
  return 0;
  }


// Get last character glyph in font
FXwchar FXFont::getMaxChar() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmLastChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0x10ffff;                                    // FIXME
#else                           ///// XLFD /////
    return (((XFontStruct*)font)->max_byte1<<8)|((XFontStruct*)font)->max_char_or_byte2;
#endif
    }
  return 0;
  }


// Get font leading [that is lead-ing as in Pb!]
FXint FXFont::getFontLeading() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmExternalLeading;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent-((XFontStruct*)font)->max_bounds.ascent-((XFontStruct*)font)->max_bounds.descent;
#endif
    }
  return 0;
  }


// Get font line spacing [height+leading]
FXint FXFont::getFontSpacing() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmHeight;               // Includes font point size plus internal leading
#elif defined(HAVE_XFT_H)       ///// XFT /////
    //return ((XftFont*)font)->height;
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Left bearing
FXint FXFont::leftBearing(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return 0;                                           // FIXME
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    XftTextExtents32(DISPLAY(getApp()),(XftFont*)font,(const FcChar32*)&ch,1,&extents);
    return extents.x;
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    if(fs->per_char){
      FXuchar row=ch>>8;
      FXuchar col=ch&255;
      if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
        const XCharStruct *cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
        if(cm->width || cm->ascent || cm->descent) return cm->lbearing;
        }
      return fs->per_char[((fs->default_char>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+((fs->default_char&255)-fs->min_char_or_byte2)].lbearing;
      }
    return fs->min_bounds.lbearing;
#endif
    }
  return 0;
  }


// Right bearing
FXint FXFont::rightBearing(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return 0;                                           // FIXME
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    XftTextExtents32(DISPLAY(getApp()),(XftFont*)font,(const FcChar32*)&ch,1,&extents);
    return extents.xOff-extents.width-extents.x;
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    if(fs->per_char){
      FXuchar row=ch>>8;
      FXuchar col=ch&255;
      if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
        const XCharStruct *cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
        if(cm->width || cm->ascent || cm->descent) return cm->rbearing;
        }
      return fs->per_char[((fs->default_char>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+((fs->default_char&255)-fs->min_char_or_byte2)].rbearing;
      }
    return fs->min_bounds.rbearing;
#endif
    }
  return 0;
  }


// Is it a mono space font
FXbool FXFont::isFontMono() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return !(((TEXTMETRIC*)font)->tmPitchAndFamily&TMPF_FIXED_PITCH);
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo i_extents,m_extents;
    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)"i",1,&i_extents); // FIXME better than before but no cigar yet
    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)"M",1,&m_extents);
    return i_extents.xOff==m_extents.xOff;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->min_bounds.width == ((XFontStruct*)font)->max_bounds.width;
#endif
    }
  return true;
  }


// Get font width
FXint FXFont::getFontWidth() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmMaxCharWidth;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    //FXTRACE(TOPIC_DETAIL,"FXFont::getFontWidth() : \n");
    //FXTRACE(TOPIC_DETAIL,"  angle : %.2f\n",angle*(1.0/64.0));
    //FXTRACE(TOPIC_DETAIL,"  result: %d\n",((XftFont*)font)->max_advance_width);
    return ((XftFont*)font)->max_advance_width;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->max_bounds.width;
#endif
    }
  return 1;
  }


// Get font height
FXint FXFont::getFontHeight() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
//    XGlyphInfo extent;
//    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)"W",1,&extent);
//    return extent.height+extent.y;
    //FXTRACE(TOPIC_DETAIL,"FXFont::getFontHeight() : \n");
    //FXTRACE(TOPIC_DETAIL,"  angle  : %.2f\n",angle*(1.0/64.0));
    //FXTRACE(TOPIC_DETAIL,"  result : %d\n",((XftFont*)font)->ascent+((XftFont*)font)->descent);
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Get font ascent
FXint FXFont::getFontAscent() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmAscent;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    //FXTRACE(TOPIC_DETAIL,"FXFont::getFontAscent() : \n");
    //FXTRACE(TOPIC_DETAIL,"  angle  : %.2f\n",angle*(1.0/64.0));
    //FXTRACE(TOPIC_DETAIL,"  result : %d\n",((XftFont*)font)->ascent);
    return ((XftFont*)font)->ascent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent;
#endif
    }
  return 1;
  }


// Get font descent
FXint FXFont::getFontDescent() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmDescent;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    //FXTRACE(TOPIC_DETAIL,"FXFont::getFontDescent() : \n");
    //FXTRACE(TOPIC_DETAIL,"  angle  : %.2f\n",angle*(1.0/64.0));
    //FXTRACE(TOPIC_DETAIL,"  result : %d\n",((XftFont*)font)->descent);
    return ((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->descent;
#endif
    }
  return 0;
  }


// Calculate width of single wide character in this font
FXint FXFont::getCharWidth(const FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
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
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    XftTextExtents32(DISPLAY(getApp()),(XftFont*)font,(const FcChar32*)&ch,1,&extents);
    return extents.xOff;
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    FXint width,size;
    FXuchar r,c;
    if(fs->per_char){
      r=ch>>8;
      c=ch&255;
      size=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        width=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        if(width) return width;
        }
      r=fs->default_char>>8;
      c=fs->default_char&255;
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        return fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        }
      }
    return fs->min_bounds.width;
#endif
    }
  return 1;
  }


// Text width
FXint FXFont::getTextWidth(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextWidth: NULL string argument\n",getClassName()); }
  if(font){
#if defined(WIN32)              ///// WIN32 /////
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
#elif defined(HAVE_XFT_H)       ///// XFT /////
    const FXchar *ptr=string;
    const FXchar *end=string+length;
    FXint width=0;
    FXwchar ch;
    XGlyphInfo extents;
    while(ptr<end){
      ch=wcnxt(ptr);
      XftTextExtents32(DISPLAY(getApp()),(XftFont*)font,(const FcChar32*)&ch,1,&extents);
      width+=extents.xOff;
      }
    return width;
/*
    // This returns rotated metrics; FOX likes to work with unrotated metrics, so if angle
    // is not 0, we calculate the unrotated baseline; note however that the calculation is
    // not 100% pixel exact when the angle is not a multiple of 90 degrees.
    XftTextExtentsUtf8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)string,length,&extents);
    if(angle){
      extents.xOff=(FXint)(0.5+Math::sqrt((FXdouble)(extents.xOff*extents.xOff+extents.yOff*extents.yOff)));
      }
    //FXTRACE(TOPIC_DETAIL,"FXFont::getTextWidth(\"%.*s\") : \n",length,string);
    //FXTRACE(TOPIC_DETAIL,"  angle : %.2f\n",angle*(1.0/64.0));
    //FXTRACE(TOPIC_DETAIL,"  width:  %d\n",extents.width);
    //FXTRACE(TOPIC_DETAIL,"  height: %d\n",extents.height);
    //FXTRACE(TOPIC_DETAIL,"  x:      %d\n",extents.x);
    //FXTRACE(TOPIC_DETAIL,"  y:      %d\n",extents.y);
    //FXTRACE(TOPIC_DETAIL,"  xOff:   %d\n",extents.xOff);
    //FXTRACE(TOPIC_DETAIL,"  yOff:   %d\n",extents.yOff);
    //FXTRACE(TOPIC_DETAIL,"  result: %d\n",extents.xOff);
    return extents.xOff;
*/
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    const FXchar *ptr=string;
    const FXchar *end=string+length;
    FXint defwidth=fs->min_bounds.width;
    FXint width=0,ww;
    FXuint s;
    FXuchar r;
    FXuchar c;
    FXwchar w;
    if(fs->per_char){
      r=fs->default_char>>8;
      c=fs->default_char&255;
      s=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        defwidth=fs->per_char[(r-fs->min_byte1)*s+(c-fs->min_char_or_byte2)].width;
        }
      while(ptr<end){
        w=wcnxt(ptr);
        r=w>>8;
        c=w&255;
        if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
          if((ww=fs->per_char[(r-fs->min_byte1)*s+(c-fs->min_char_or_byte2)].width)!=0){
            width+=ww;
            continue;
            }
          }
        width+=defwidth;
        }
      }
    else{
      while(ptr<end){
        wcnxt(ptr);
        width+=defwidth;
        }
      }
    return width;
#endif
    }
  return length;
  }


// Text width
FXint FXFont::getTextWidth(const FXString& string) const {
  return getTextWidth(string.text(),string.length());
  }


// Text height
FXint FXFont::getTextHeight(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextHeight: NULL string argument\n",getClassName()); }
  if(font){
#if defined(WIN32)              ///// WIN32 /////
//    SIZE size;
//    FXASSERT(dc);
//    GetTextExtentPoint32((HDC)dc,string,length,&size);
//    return size.cy;
    return ((TEXTMETRIC*)font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
//    XGlyphInfo extents;
//    XftTextExtentsUtf8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)string,length,&extents);
//    return extents.height; // TODO: Is this correct?
//    FXTRACE(TOPIC_DETAIL,"FXFont::getTextHeight(\"%.*s\") : \n",length,string);
//    FXTRACE(TOPIC_DETAIL,"  width:  %d\n",extents.width);
//    FXTRACE(TOPIC_DETAIL,"  height: %d\n",extents.height);
//    FXTRACE(TOPIC_DETAIL,"  x:      %d\n",extents.x);
//    FXTRACE(TOPIC_DETAIL,"  y:      %d\n",extents.y);
//    FXTRACE(TOPIC_DETAIL,"  xOff:   %d\n",extents.xOff);
//    FXTRACE(TOPIC_DETAIL,"  yOff:   %d\n",extents.yOff);
//    FXTRACE(TOPIC_DETAIL,"  angle : %.2f\n",angle*(1.0/64.0));
//    FXTRACE(TOPIC_DETAIL,"  result: %d\n",((XftFont*)font)->ascent+((XftFont*)font)->descent);
    // Patch from ivan.markov@wizcom.bg
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
//    XCharStruct chst; int dir,asc,desc;
//    XTextExtents((XFontStruct*)font,string,length,&dir,&asc,&desc,&chst);
//    return asc+desc;
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Text height
FXint FXFont::getTextHeight(const FXString& string) const {
  return getTextHeight(string.text(),string.length());
  }


/*
static FX88591Codec codec_8859_1;
static FX88592Codec codec_8859_2;
static FX88593Codec codec_8859_3;
static FX88594Codec codec_8859_4;
static FX88595Codec codec_8859_5;
static FX88596Codec codec_8859_6;
static FX88597Codec codec_8859_7;
static FX88598Codec codec_8859_8;
static FX88599Codec codec_8859_9;
static FX885910Codec codec_8859_10;
static FX885911Codec codec_8859_11;
static FX885913Codec codec_8859_13;
static FX885914Codec codec_8859_14;
static FX885915Codec codec_8859_15;
static FX885916Codec codec_8859_16;

  FXchar sbuffer[4096];
  switch(actualEncoding){
    case FONTENCODING_ISO_8859_1: count=codec_8859_1.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_2: count=codec_8859_2.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_3: count=codec_8859_3.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_4: count=codec_8859_4.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_5: count=codec_8859_5.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_6: count=codec_8859_6.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_7: count=codec_8859_7.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_8: count=codec_8859_8.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_9: count=codec_8859_9.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_10: count=codec_8859_10.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_11: count=codec_8859_11.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_13: count=codec_8859_13.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_14: count=codec_8859_14.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_15: count=codec_8859_15.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_16: count=codec_8859_16.utf2mb(sbuffer,4096,string,length); break;
    default: count=0; break;
    }
  XDrawString(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,x,y,sbuffer,count);
*/


/*
  /// Draw text starting at x,y
  virtual void drawText(FXDC* dc,FXint x,FXint y,const FXString& string) const;

  /// Draw text starting at x,y
  virtual void drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const;

  /// Draw text starting at x,y over filled background
  virtual void drawImageText(FXDC* dc,FXint x,FXint y,const FXString& string) const;

  /// Draw text starting at x,y over filled background
  virtual void drawImageText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const;


#if defined(WIN32)              ///// WIN32 /////

// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  FXnchar sbuffer[4096];
  FXint iBkMode=SetBkMode((HDC)dc->ctx,TRANSPARENT);
  FXint count=utf2ncs(sbuffer,string,ARRAYNUMBER(sbuffer),FXMIN(length,4096));
  FXASSERT(count<=length);
  TextOutW((HDC)dc->ctx,x,y,sbuffer,count);
  SetBkMode((HDC)dc->ctx,iBkMode);
  }

#elif defined(HAVE_XFT_H)       ///// XFT /////

// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  XftColor color;
  color.pixel=((FXDCWindow*)dc)->devfg;
  color.color.red=FXREDVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.green=FXGREENVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.blue=FXBLUEVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.alpha=FXALPHAVAL(((FXDCWindow*)dc)->fg)*257;
  XftDrawStringUtf8((XftDraw*)((FXDCWindow*)dc)->xftDraw,&color,(XftFont*)((FXDCWindow*)dc)->font->font,x,y,(const FcChar8*)string,length);
  }

#else                           ///// XLFD /////

static FXint utf2db(XChar2b *dst,const FXchar *src,FXint n){
  FXint len,p;
  FXwchar w;
  for(p=len=0; p<n; p+=wclen(src+p),len++){
    w=wc(src+p);
    dst[len].byte1=(w>>8);
    dst[len].byte2=(w&255);
    }
  return len;
  }


// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  const XFontStruct *fs=(XFontStruct*)font;
  FXint count,escapement,defwidth,ww,size,i;
  FXdouble ang,ux,uy;
  FXuchar r,c;
  XChar2b sbuffer[4096];
  count=utf2db(sbuffer,string,FXMIN(length,4096));
  FXASSERT(count<=length);
  if(angle){
    ang=angle*0.00027270769562411399179;
    defwidth=fs->min_bounds.width;
    ux=cos(ang);
    uy=sin(ang);
    if(fs->per_char){
      r=fs->default_char>>8;
      c=fs->default_char&255;
      size=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        defwidth=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        }
      for(i=escapement=0; i<count; i++){
        XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,(FXint)(x+escapement*ux),(FXint)(y-escapement*uy),&sbuffer[i],1);
        r=sbuffer[i].byte1;
        c=sbuffer[i].byte2;
        escapement+=defwidth;
        if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
          if((ww=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width)!=0) escapement+=ww-defwidth;
          }
        }
      }
    else{
      for(i=escapement=0; i<count; i++){
        XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,(FXint)(x+escapement*ux),(FXint)(y-escapement*uy),&sbuffer[i],1);
        escapement+=defwidth;
        }
      }
    }
  else{
    XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,x,y,sbuffer,count);
    }
  }


#endif


// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXString& string) const {
  drawText(dc,x,y,string.text(),string.length());
  }



// Draw text starting at x,y over filled background
void FXFont::drawImageText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  // ...
  }


// Draw text starting at x,y over filled background
void FXFont::drawImageText(FXDC* dc,FXint x,FXint y,const FXString& string) const {
  drawImageText(dc,x,y,string.text(),string.length());
  }
*/


/*******************************************************************************/


// For tables
struct ENTRY { const FXchar *name; FXuint value; };


// Character set encodings
static const ENTRY encodingtable[]={
  {"",FONTENCODING_DEFAULT},
  {"iso10646-1",FONTENCODING_UNICODE},
  {"iso8859-1",FONTENCODING_ISO_8859_1},
  {"iso8859-2",FONTENCODING_ISO_8859_2},
  {"iso8859-3",FONTENCODING_ISO_8859_3},
  {"iso8859-4",FONTENCODING_ISO_8859_4},
  {"iso8859-5",FONTENCODING_ISO_8859_5},
  {"iso8859-6",FONTENCODING_ISO_8859_6},
  {"iso8859-7",FONTENCODING_ISO_8859_7},
  {"iso8859-8",FONTENCODING_ISO_8859_8},
  {"iso8859-9",FONTENCODING_ISO_8859_9},
  {"iso8859-10",FONTENCODING_ISO_8859_10},
  {"iso8859-11",FONTENCODING_ISO_8859_11},
  {"iso8859-13",FONTENCODING_ISO_8859_13},
  {"iso8859-14",FONTENCODING_ISO_8859_14},
  {"iso8859-15",FONTENCODING_ISO_8859_15},
  {"iso8859-16",FONTENCODING_ISO_8859_16},
  {"koi8",FONTENCODING_KOI8},
  {"koi8-r",FONTENCODING_KOI8_R},
  {"koi8-u",FONTENCODING_KOI8_U},
  {"koi8-unified",FONTENCODING_KOI8_UNIFIED},
  {"cp437",FONTENCODING_CP437},
  {"cp850",FONTENCODING_CP850},
  {"cp851",FONTENCODING_CP851},
  {"cp852",FONTENCODING_CP852},
  {"cp855",FONTENCODING_CP855},
  {"cp856",FONTENCODING_CP856},
  {"cp857",FONTENCODING_CP857},
  {"cp860",FONTENCODING_CP860},
  {"cp861",FONTENCODING_CP861},
  {"cp862",FONTENCODING_CP862},
  {"cp863",FONTENCODING_CP863},
  {"cp864",FONTENCODING_CP864},
  {"cp865",FONTENCODING_CP865},
  {"cp866",FONTENCODING_CP866},
  {"cp869",FONTENCODING_CP869},
  {"cp870",FONTENCODING_CP870},
  {"cp1250",FONTENCODING_CP1250},
  {"cp1251",FONTENCODING_CP1251},
  {"cp1252",FONTENCODING_CP1252},
  {"cp1253",FONTENCODING_CP1253},
  {"cp1254",FONTENCODING_CP1254},
  {"cp1255",FONTENCODING_CP1255},
  {"cp1256",FONTENCODING_CP1256},
  {"cp1257",FONTENCODING_CP1257},
  {"cp1258",FONTENCODING_CP1258},
  {"cp874",FONTENCODING_CP874},
  {"ascii",FONTENCODING_ISO_8859_1}
  };


// Font style table
static const ENTRY styletable[]={
  {"",0},
  {"decorative",FXFont::Decorative},
  {"modern",FXFont::Modern},
  {"roman",FXFont::Roman},
  {"script",FXFont::Script},
  {"swiss",FXFont::Swiss},
  {"system",FXFont::System}
  };


// Font pitch table
static const ENTRY pitchtable[]={
  {"",0},
  {"mono",FXFont::Mono},
  {"dual",FXFont::Dual},
  {"cell",FXFont::Cell},
  {"variable",FXFont::Variable},
  {"proportional",FXFont::Variable},
  {"c",FXFont::Cell},
  {"m",FXFont::Mono},
  {"p",FXFont::Variable}
  };


// Font text angles
static const ENTRY slanttable[]={
  {"",0},
  {"regular",FXFont::Straight},
  {"italic",FXFont::Italic},
  {"oblique",FXFont::Oblique},
  {"normal",FXFont::Straight},
  {"reverse italic",FXFont::ReverseItalic},
  {"reverse oblique",FXFont::ReverseOblique},
  {"r",FXFont::Straight},
  {"n",FXFont::Straight},
  {"i",FXFont::Italic},
  {"o",FXFont::Oblique},
  {"ri",FXFont::ReverseItalic},
  {"ro",FXFont::ReverseOblique}
  };


// Set width table
static const ENTRY setwidthtable[]={
  {"",0},
  {"ultracondensed",FXFont::UltraCondensed},
  {"extracondensed",FXFont::ExtraCondensed},
  {"condensed",FXFont::Condensed},
  {"narrow",FXFont::Condensed},
  {"compressed",FXFont::Condensed},
  {"semicondensed",FXFont::SemiCondensed},
  {"medium",FXFont::NonExpanded},
  {"normal",FXFont::NonExpanded},
  {"regular",FXFont::NonExpanded},
  {"semiexpanded",FXFont::SemiExpanded},
  {"expanded",FXFont::Expanded},
  {"wide",FXFont::ExtraExpanded},
  {"extraexpanded",FXFont::ExtraExpanded},
  {"ultraexpanded",FXFont::UltraExpanded},
  {"n",FXFont::Condensed},
  {"r",FXFont::NonExpanded},
  {"c",FXFont::Condensed},
  {"w",FXFont::ExtraExpanded},
  {"m",FXFont::NonExpanded},
  {"x",FXFont::Expanded}
  };


// Weight table
static const ENTRY weighttable[]={
  {"",0},
  {"thin",FXFont::Thin},
  {"extralight",FXFont::ExtraLight},
  {"light",FXFont::Light},
  {"normal",FXFont::Normal},
  {"regular",FXFont::Normal},
  {"medium",FXFont::Medium},
  {"demibold",FXFont::DemiBold},
  {"bold",FXFont::Bold},
  {"extrabold",FXFont::ExtraBold},
  {"heavy",FXFont::Black},
  {"black",FXFont::Black},
  {"b",FXFont::Bold},
  {"l",FXFont::Light},
  {"n",FXFont::Normal},
  {"r",FXFont::Normal},
  {"m",FXFont::Medium},
  };


// Search for value and return name
static FXString findbyvalue(const ENTRY* table,FXint n,FXuint value){
  for(int i=0; i<n; i++){ if(table[i].value==value) return table[i].name; }
  return FXString::value(value);
  }


// Search for name and return value
static FXuint findbyname(const ENTRY* table,FXint n,const FXString& name){
  for(int i=0; i<n; i++){ if(FXString::comparecase(table[i].name,name)==0) return table[i].value; }
  return name.toUInt();
  }


// Get style from string
FXuint FXFont::styleFromString(const FXString& str){
  return str.toUInt();
  }


// Get string from style
FXString FXFont::stringFromStyle(FXuint style){
   return FXString::value(style);
  }


// Get slant from string
FXuint FXFont::slantFromString(const FXString& str){
  return findbyname(slanttable,ARRAYNUMBER(slanttable),str);
  }


// Get string from slant
FXString FXFont::stringFromSlant(FXuint slant){
  return findbyvalue(slanttable,ARRAYNUMBER(slanttable),slant);
  }


// Get weight from string
FXuint FXFont::weightFromString(const FXString& str){
  return findbyname(weighttable,ARRAYNUMBER(weighttable),str);
  }


// Get string from weight
FXString FXFont::stringFromWeight(FXuint weight){
  return findbyvalue(weighttable,ARRAYNUMBER(weighttable),weight);
  }


// Get setwidth from string
FXuint FXFont::setWidthFromString(const FXString& str){
  return findbyname(setwidthtable,ARRAYNUMBER(setwidthtable),str);
  }


// Get string from setwidth
FXString FXFont::stringFromSetWidth(FXuint setwidth){
  return findbyvalue(setwidthtable,ARRAYNUMBER(setwidthtable),setwidth);
  }


// Get encoding from string
FXuint FXFont::encodingFromString(const FXString& str){
  return findbyname(encodingtable,ARRAYNUMBER(encodingtable),str);
  }


// Get string from encoding
FXString FXFont::stringFromEncoding(FXuint encoding){
  return findbyvalue(encodingtable,ARRAYNUMBER(encodingtable),encoding);
  }


// Change font description
void FXFont::setFontDesc(const FXFontDesc& fontdesc){
  wantedName=fontdesc.face;
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  hints=fontdesc.flags;
  }


// Get font description
FXFontDesc FXFont::getFontDesc() const {
  FXFontDesc result;
  fxstrlcpy(result.face,wantedName.text(),sizeof(result.face));
  result.size=wantedSize;
  result.weight=wantedWeight;
  result.slant=wantedSlant;
  result.setwidth=wantedSetwidth;
  result.encoding=wantedEncoding;
  result.flags=hints;
  return result;
  }


// Get actual font description
FXFontDesc FXFont::getActualFontDesc() const {
  FXFontDesc result;
  fxstrlcpy(result.face,actualName.text(),sizeof(result.face));
  result.size=actualSize;
  result.weight=actualWeight;
  result.slant=actualSlant;
  result.setwidth=actualSetwidth;
  result.encoding=actualEncoding;
  result.flags=flags;
  return result;
  }

/*******************************************************************************/

// Set font description from a string
void FXFontDesc::setFont(const FXString& string){
  FXint len=string.find(',');
  clearElms(face,sizeof(face));
  if(0<=len){
    memcpy(face,string.text(),Math::imin(len,(FXint)sizeof(face)-1));
    size=string.section(',',1).toUInt();
    weight=FXFont::weightFromString(string.section(',',2));
    slant=FXFont::slantFromString(string.section(',',3));
    setwidth=FXFont::setWidthFromString(string.section(',',4));
    encoding=FXFont::encodingFromString(string.section(',',5));
    flags=FXFont::styleFromString(string.section(',',6));
    }
  else{
    fxstrlcpy(face,string.text(),sizeof(face));
    size=0;
    weight=0;
    slant=0;
    setwidth=0;
    encoding=0;
    flags=FXFont::X11;
    }
  }


// Get string of font description
FXString FXFontDesc::getFont() const {
  FXString string=face;
  if(!(flags&FXFont::X11)){
    string.append(',');
    string.append(FXString::value(size));
    if(weight || slant || setwidth || encoding || flags){
      string.append(',');
      string.append(FXFont::stringFromWeight(weight));
      if(slant || setwidth || encoding || flags){
        string.append(',');
        string.append(FXFont::stringFromSlant(slant));
        if(setwidth || encoding || flags){
          string.append(',');
          string.append(FXFont::stringFromSetWidth(setwidth));
          if(encoding || flags){
            string.append(',');
            string.append(FXFont::stringFromEncoding(encoding));
            if(flags){
              string.append(',');
              string.append(FXFont::stringFromStyle(flags));
              }
            }
          }
        }
      }
    }
  return string;
  }


/*******************************************************************************/


// Change font description from a string
void FXFont::setFont(const FXString& string){
  FXint len;

  // Raw X11 font is only the name
  wantedName=string;
  wantedSize=0;
  wantedWeight=0;
  wantedSlant=0;
  wantedSetwidth=0;
  wantedEncoding=0;
  hints=FXFont::X11;

  // Normal font description
  len=string.find(',');
  if(0<=len){

    // Name and foundry
    wantedName.trunc(len);

    // Point size
    wantedSize=string.section(',',1).toUInt();

    // Weight
    wantedWeight=weightFromString(string.section(',',2));

    // Slant
    wantedSlant=slantFromString(string.section(',',3));

    // Set width
    wantedSetwidth=setWidthFromString(string.section(',',4));

    // Encoding
    wantedEncoding=encodingFromString(string.section(',',5));

    // Flags
    hints=styleFromString(string.section(',',6));
    }
  }



// Return the font description as a string; keep it as simple
// as possible by dropping defaulted fields at the end.
FXString FXFont::getFont() const {
  FXString string=wantedName;

  // Raw X11 font is only the name
  if(!(hints&FXFont::X11)){

    // Append size
    string.append(',');
    string.append(FXString::value(wantedSize));

    // Weight and other stuff
    if(wantedWeight || wantedSlant || wantedSetwidth || wantedEncoding || hints){

      // Append weight
      string.append(',');
      string.append(stringFromWeight(wantedWeight));

      // Slant and other stuff
      if(wantedSlant || wantedSetwidth || wantedEncoding || hints){

        // Append slant
        string.append(',');
        string.append(stringFromSlant(wantedSlant));

        // Setwidth and other stuff
        if(wantedSetwidth || wantedEncoding || hints){

          // Append set width
          string.append(',');
          string.append(stringFromSetWidth(wantedSetwidth));

          // Encoding and other stuff
          if(wantedEncoding || hints){

            // Append encoding
            string.append(',');
            string.append(stringFromEncoding(wantedEncoding));

            // Hints
            if(hints){

              // Append hint flags
              string.append(',');
              string.append(stringFromStyle(hints));
              }
            }
          }
        }
      }
    }
  return string;
  }


/*******************************************************************************/


// Save font to stream
void FXFont::save(FXStream& store) const {
  FXId::save(store);
  store << wantedName;
  store << wantedSize;
  store << wantedWeight;
  store << wantedSlant;
  store << wantedSetwidth;
  store << wantedEncoding;
  store << angle;
  store << hints;
  }


// Load font from stream; create() should be called later
void FXFont::load(FXStream& store){
  FXId::load(store);
  store >> wantedName;
  store >> wantedSize;
  store >> wantedWeight;
  store >> wantedSlant;
  store >> wantedSetwidth;
  store >> wantedEncoding;
  store >> angle;
  store >> hints;
  }


// Clean up
FXFont::~FXFont(){
  FXTRACE(TOPIC_CONSTRUCT,"FXFont::~FXFont %p\n",this);
  destroy();
  }

}
