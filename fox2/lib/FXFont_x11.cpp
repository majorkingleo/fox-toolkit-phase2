/********************************************************************************
*                                                                               *
*                       F o n t   O b j e c t   ( X 1 1 )                       *
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

#if defined(FOX_X11)

#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1012

/*
  Notes:

  - X11 (Xft or XLFD) face matching, font realization, font enumeration and text
    metrics.  The public FXFont API stays in FXFont.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);

#if defined(HAVE_XFT_H) /////////////////////// XFT ///////////////////////////


// Access to display
#define DISPLAY(app)      ((Display*)((app)->display))


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


// From FOX setwidth to fontconfig setwidth
static FXint setWidth2FcSetWidth(FXint setwidth){
  switch(setwidth){
    case FXFont::UltraCondensed:return FC_WIDTH_ULTRACONDENSED;
    case FXFont::ExtraCondensed:return FC_WIDTH_EXTRACONDENSED;
    case FXFont::Condensed:     return FC_WIDTH_CONDENSED;
    case FXFont::SemiCondensed: return FC_WIDTH_SEMICONDENSED;
    case FXFont::NonExpanded:        return FC_WIDTH_NORMAL;
    case FXFont::SemiExpanded:  return FC_WIDTH_SEMIEXPANDED;
    case FXFont::Expanded:      return FC_WIDTH_EXPANDED;
    case FXFont::ExtraExpanded: return FC_WIDTH_EXTRAEXPANDED;
    case FXFont::UltraExpanded: return FC_WIDTH_ULTRAEXPANDED;
    }
  return FC_WIDTH_NORMAL;
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


// From FOX slant to fontconfig slant
static FXint slant2FcSlant(FXint slant){
  switch(slant){
    case FXFont::Straight: return FC_SLANT_ROMAN;
    case FXFont::Italic: return FC_SLANT_ITALIC;
    case FXFont::Oblique: return FC_SLANT_OBLIQUE;
    case FXFont::ReverseItalic: return FC_SLANT_ITALIC;         // No equivalent FC value
    case FXFont::ReverseOblique: return FC_SLANT_OBLIQUE;       // No equivalent FC value
    }
  return FC_SLANT_ROMAN;
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


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  const FXchar *rgba=getApp()->reg().readStringEntry("Xft","rgba","unknown");
  const FXchar *hs=getApp()->reg().readStringEntry("Xft","hintstyle","full");
  FXbool     fc_hinting=getApp()->reg().readBoolEntry("Xft","hinting",true);
  FXbool     fc_autohint=getApp()->reg().readBoolEntry("Xft","autohint",false);
  FXbool     fc_antialias=getApp()->reg().readBoolEntry("Xft","antialias",true);
  FXint      fc_rgba=FC_RGBA_UNKNOWN;
  FXint      fc_hintstyle=FC_HINT_FULL;
  FXint      pp,sw,wt,sl;
  FXdouble   a,s,c,sz;
  FcPattern *pattern,*p;
  FcChar8   *fam,*fdy;
  FcCharSet *charset;
  XftFont   *fnt;
  FcResult   result;
  FcBool     sc;
  FcMatrix   matrix;

  FXTRACE(TOPIC_DETAIL,"FXFont::match: wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res);

  // Create pattern object
  pattern=FcPatternCreate();

  // RGBA color order hint (unknown, rgb, bgr, vrgb, vbgr, none)
  if(rgba[0]=='u') fc_rgba=FC_RGBA_UNKNOWN;
  else if(rgba[0]=='r') fc_rgba=FC_RGBA_RGB;
  else if(rgba[0]=='b') fc_rgba=FC_RGBA_BGR;
  else if(rgba[0]=='v' && rgba[1]=='r') fc_rgba=FC_RGBA_VRGB;
  else if(rgba[0]=='v' && rgba[1]=='b') fc_rgba=FC_RGBA_VBGR;
  else if(rgba[0]=='n') fc_rgba=FC_RGBA_NONE;

  if(hs[0]=='s') fc_hintstyle=FC_HINT_SLIGHT;
  else if(hs[0]=='m') fc_hintstyle=FC_HINT_MEDIUM;
  else if(hs[0]=='f') fc_hintstyle=FC_HINT_FULL;
  else if(hs[0]=='n') fc_hintstyle=FC_HINT_NONE;

  // Set additional hints
  FcPatternAddBool(pattern,FC_HINTING,fc_hinting);
  FcPatternAddBool(pattern,FC_ANTIALIAS,fc_antialias);
  FcPatternAddBool(pattern,FC_AUTOHINT,fc_autohint);
  FcPatternAddInteger(pattern,FC_RGBA,fc_rgba);
  FcPatternAddInteger(pattern,FC_HINT_STYLE,fc_hintstyle);

  // Set family
  if(!wantfamily.empty()){
    FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)wantfamily.text());
    }

  // Set foundry
  if(!wantforge.empty()){
    FcPatternAddString(pattern,FC_FOUNDRY,(const FcChar8*)wantforge.text());
    }

  // Set pixel size, based on given screen res and desired point size
  if(wantsize!=0){
    FcPatternAddDouble(pattern,FC_PIXEL_SIZE,(res*wantsize)/720.0);
    }

  // Set font weight
  if(wantweight!=0){
    FcPatternAddInteger(pattern,FC_WEIGHT,weight2FcWeight(wantweight));
    }

  // Set slant
  if(wantslant!=0){
    FcPatternAddInteger(pattern,FC_SLANT,slant2FcSlant(wantslant));
    }

  // Set setwidth
  if(wantsetwidth!=0){
    FcPatternAddInteger(pattern,FC_WIDTH,setWidth2FcSetWidth(wantsetwidth));
    }

  // Set encoding
  if(wantencoding!=FONTENCODING_DEFAULT){                                       // FIXME
//    FcCharSet* charSet=FcCharSetCreate();
//    encoding2FcCharSet((void*)charSet, (FXFontEncoding)encoding);
//    FcPatternAddCharSet(pattern,FC_CHARSET,charSet);
//    FcCharSetDestroy(charSet);
    }

  // Set pitch
  if(wanthints&FXFont::Variable){
    FcPatternAddInteger(pattern,FC_SPACING,FC_PROPORTIONAL);
    }
  else if(wanthints&FXFont::Mono){
    FcPatternAddInteger(pattern,FC_SPACING,FC_MONO);
    }
  else if(wanthints&FXFont::Cell){
    FcPatternAddInteger(pattern,FC_SPACING,FC_CHARCELL);
    }
  else if(wanthints&FXFont::Dual){
    FcPatternAddInteger(pattern,FC_SPACING,FC_DUAL);
    }

  // Scalable font hint; also set if we want rotation
  if(wanthints&(FXFont::Scalable|FXFont::Rotatable)){
    FcPatternAddBool(pattern,FC_SCALABLE,true);
    }

  // Always set matrix if rotatable
  if(wanthints&FXFont::Rotatable){
    a=0.00027270769562411399179*angle;
    c=Math::cos(a);
    s=Math::sin(a);
    matrix.xx=c; matrix.xy=-s;
    matrix.yx=s; matrix.yy=c;
    FcPatternAddMatrix(pattern,FC_MATRIX,&matrix);
    }

  // Pattern substitutions
  FcConfigSubstitute(0,pattern,FcMatchPattern);
  FcDefaultSubstitute(pattern);

  // Find pattern matching a font
  p=FcFontMatch(0,pattern,&result);
  if(!p) return nullptr;

  // Get name and foundry
  if(FcPatternGetString(p,FC_FAMILY,0,&fam)==FcResultMatch){
    actualName=(const FXchar*)fam;
    if(FcPatternGetString(p,FC_FOUNDRY,0,&fdy)==FcResultMatch){
      actualName.append(" [");
      actualName.append((const FXchar*)fdy);
      actualName.append("]");
      }
    }

  // Get setwidth
  if(FcPatternGetInteger(p,FC_WIDTH,0,&sw)==FcResultMatch){
    actualSetwidth=fcSetWidth2SetWidth(sw);
    }

  // Get weight
  if(FcPatternGetInteger(p,FC_WEIGHT,0,&wt)==FcResultMatch){
    actualWeight=fcWeight2Weight(wt);                                   // FIXME
    }

  // Get slant
  if(FcPatternGetInteger(p,FC_SLANT,0,&sl)==FcResultMatch){
    actualSlant=fcSlant2Slant(sl);
    }

  // Get pitch
  if(FcPatternGetInteger(p,FC_SPACING,0,&pp)==FcResultMatch){
    flags&=~(FXFont::Mono|FXFont::Variable|FXFont::Dual);
    if(pp==FC_MONO) flags|=FXFont::Mono;
    else if(pp==FC_DUAL) flags|=FXFont::Dual;
    else if(pp==FC_PROPORTIONAL) flags|=FXFont::Variable;
    else if(pp==FC_CHARCELL) flags|=FXFont::Mono;
    }

  // Get scalable flag
  if(FcPatternGetBool(p,FC_SCALABLE,0,&sc)==FcResultMatch){
    flags=sc?(flags|FXFont::Scalable):(flags&~FXFont::Scalable);
    }

  // Get pixel size and work it back to deci-points using given screen res
  if(FcPatternGetDouble(p,FC_PIXEL_SIZE,0,&sz)==FcResultMatch){
    actualSize=(FXushort)((720.0*sz)/res);
    }

  // Get charset
  if(FcPatternGetCharSet(p,FC_CHARSET,0,&charset)==FcResultMatch){      // FIXME
    }

  // Get the encoding
  actualEncoding=FONTENCODING_UNICODE;

  // Open font
  fnt=XftFontOpenPattern(DISPLAY(getApp()),p);
  xid=(FXID)fnt;

  FXTRACE(TOPIC_DETAIL,"FXFont::match: fnt(%p)->ascent            = %d\n",fnt,fnt->ascent);
  FXTRACE(TOPIC_DETAIL,"FXFont::match: fnt(%p)->descent           = %d\n",fnt,fnt->descent);
  FXTRACE(TOPIC_DETAIL,"FXFont::match: fnt(%p)->height            = %d\n",fnt,fnt->height);
  FXTRACE(TOPIC_DETAIL,"FXFont::match: fnt(%p)->max_advance_width = %d\n",fnt,fnt->max_advance_width);

  // Destroy pattern
  FcPatternDestroy(pattern);

  return fnt;
  }


#else ///////////////////////////////// XLFD ////////////////////////////////////


#define SGN(x)        ((x)<-0.0005?"~":"")
#define DISPLAY(app)  ((Display*)((app)->display))


// Convert text to font weight
static FXuint xlfdWeight(const FXchar* text){
  FXchar c1=Ascii::toLower(text[0]);
  FXchar c2=Ascii::toLower(text[1]);
  if(c1=='l' && c2=='i') return FXFont::Light;
  if(c1=='o' && c2=='u') return FXFont::Light;
  if(c1=='s' && c2=='h') return FXFont::Light;
  if(c1=='n' && c2=='o') return FXFont::Normal;
  if(c1=='r' && c2=='e') return FXFont::Normal;
  if(c1=='m' && c2=='e') return FXFont::Medium;
  if(c1=='d' && c2=='e') return FXFont::DemiBold;
  if(c1=='s' && c2=='e') return FXFont::DemiBold;
  if(c1=='b' && c2=='o') return FXFont::Bold;
  if(c1=='b' && c2=='l') return FXFont::Black;
  return 0;
  }


// Convert text to slant
static FXuint xlfdSlant(const FXchar* text){
  FXchar c1=Ascii::toLower(text[0]);
  FXchar c2=Ascii::toLower(text[1]);
  if(c1=='i') return FXFont::Italic;
  if(c1=='o') return FXFont::Oblique;
  if(c1=='r' && c2=='i') return FXFont::ReverseItalic;
  if(c1=='r' && c2=='o') return FXFont::ReverseOblique;
  if(c1=='r') return FXFont::Straight;
  return 0;
  }


// Convert text to setwidth
static FXuint xlfdSetwidth(const FXchar* text){
  if(text[0]=='m') return FXFont::NonExpanded;
  if(text[0]=='w') return FXFont::ExtraExpanded;
  if(text[0]=='r') return FXFont::NonExpanded;
  if(text[0]=='c') return FXFont::Condensed;
  if(text[0]=='n'){
    if(text[1]=='a') return FXFont::Condensed;
    if(text[1]=='o') return FXFont::NonExpanded;
    return 0;
    }
  if(text[0]=='e' && text[1]=='x' && text[2]=='p') return FXFont::Expanded;
  if(text[0]=='e' && text[1]=='x' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FXFont::ExtraCondensed;
    if(text[5]=='e') return FXFont::ExtraExpanded;
    return 0;
    }
  if(text[0]=='u' && text[1]=='l' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FXFont::UltraCondensed;
    if(text[5]=='e') return FXFont::UltraExpanded;
    return 0;
    }
  if((text[0]=='s' || text[0]=='d') && text[1]=='e' && text[2]=='m' && text[3]=='i'){
    if(text[5]=='c') return FXFont::SemiCondensed;
    if(text[5]=='e') return FXFont::SemiExpanded;
    return 0;
    }
  return 0;
  }


// Convert pitch to flags
static FXuint xlfdPitch(const FXchar* text){
  FXchar c=Ascii::toLower(text[0]);
  if(c=='p') return FXFont::Variable;
  if(c=='m') return FXFont::Mono;
  if(c=='c') return FXFont::Cell;
  return 0;
  }


// Convert fields to scalable flag
static FXuint xlfdScalable(const FXchar *pixelsize,const FXchar *pointsize,const FXchar* average){
  if(pixelsize[0]=='[' && pointsize[0]=='[' && average[0]=='0') return FXFont::Scalable;
  if(pixelsize[0]=='0' && pointsize[0]=='0' && average[0]=='0') return FXFont::Scalable;
  return 0;
  }


// Convert fields to rotatable flag
static FXuint xlfdRotatable(const FXchar *pixelsize,const FXchar *pointsize){
  if(pixelsize[0]=='[' && pointsize[0]=='[') return FXFont::Rotatable;
  return 0;
  }


// Convert fields to rotatable flag
static FXuint xlfdPolymorph(const FXchar *weight,const FXchar* slant,const FXchar *setwidth,const FXchar* addstyle){
  if(weight[0]=='0' || slant[0]=='0' || setwidth[0]=='0' || addstyle[0]=='0') return FXFont::Polymorphic;
  return 0;
  }


// Determine encoding; these codes should tie into the codec list in some way
static FXuint xlfdEncoding(const FXchar* text){
  if((text[0]=='i' || text[0]=='I') && (text[1]=='s' || text[1]=='S') && (text[2]=='o' || text[2]=='O')){
    if(text[3]=='8' && text[4]=='8' && text[5]=='5' && text[6]=='9' && text[7]=='-'){
      return FONTENCODING_ISO_8859_1+atoi(text+8)-1;    // iso8859-XX
      }
    if(text[3]=='1' && text[4]=='0' &&text[5]=='6' && text[6]=='4' && text[7]=='6' && text[8]=='-' && text[9]=='1'){
      return FONTENCODING_UNICODE;                      // iso10646-1
      }
    return FONTENCODING_DEFAULT;
    }
  if((text[0]=='k' || text[0]=='K') && (text[1]=='o' || text[1]=='O') && (text[2]=='i' || text[2]=='I') && text[3]=='8'){
    if(text[4]=='r' || text[4]=='R'){
      return FONTENCODING_KOI8_R;                       // koi8r
      }
    if(text[4]=='u' || text[4]=='U'){                   // koi8u
      return FONTENCODING_KOI8_U;
      }
    return FONTENCODING_KOI8;
    }
  if((text[0]=='m' || text[0]=='M') && (text[1]=='i' || text[1]=='I') && (text[2]=='c' || text[2]=='C') && (text[3]=='r' || text[3]=='R') && (text[4]=='o' || text[4]=='O') && (text[5]=='s' || text[5]=='S') && (text[6]=='o' || text[6]=='O') && (text[7]=='f' || text[7]=='F') && (text[8]=='t' || text[8]=='T') && text[9]=='-'){
    if((text[10]=='c' || text[10]=='C') && (text[11]=='p' || text[11]=='P')){
      return atoi(text+12);                             // microsoft-cpXXXX
      }
    return FONTENCODING_DEFAULT;
    }
  if((text[0]=='j' || text[0]=='J') && (text[1]=='i' || text[1]=='I') && (text[2]=='s' || text[2]=='S') && (text[3]=='x' || text[3]=='X')){
    return FONTENCODING_DEFAULT;                        // jisx.XXXX.YYYY-Z FIXME
    }
  if((text[0]=='b' || text[0]=='B') && (text[1]=='i' || text[1]=='I') && (text[2]=='g' || text[2]=='G') && text[3]=='5'){
    return FONTENCODING_DEFAULT;                        // big5XXX FIXME
    }
  if((text[0]=='k' || text[0]=='K') && (text[1]=='s' || text[1]=='S') && (text[2]=='c' || text[2]=='C')){
    return FONTENCODING_DEFAULT;                        // kscXXX FIXME
    }
  if((text[0]=='g' || text[0]=='G') && (text[1]=='b' || text[1]=='B')){
    return FONTENCODING_DEFAULT;                        // gbXXXX FIXME
    }
  return FONTENCODING_DEFAULT;
  }


// Split XLFD into pieces
static void xlfdSplit(const char *fld[],char* font){
  fld[0]=fld[1]=fld[2]=fld[3]=fld[4]=fld[5]=fld[6]=fld[7]=fld[8]=fld[9]=fld[10]=fld[11]=fld[12]="";
  if(*font=='-'){
    fld[0]=++font;
    for(int f=1; f<=12; f++){
      while(*font && *font!='-') font++;
      if(*font=='-') *font++='\0';
      fld[f]=font;
      }
    }
  else{
    fld[1]=font;
    }
  }


// Return font name from a font possibly containing wildcards
static FXString xlfdFont(Display *dpy,const FXString& font){
  char **fontnames; int nfontnames;
  FXString fontname(font);
  if((fontnames=XListFonts(dpy,font.text(),1,&nfontnames))!=nullptr){
    fontname=fontnames[0];
    XFreeFontNames(fontnames);
    }
  return fontname;
  }


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  FXuint   encoding,weight,slant,setwidth,pitch,scalable,rotatable,polymorph,xres,yres,points,size;
  FXint    bencoding,bweight,bslant,bsetwidth,bpitch,bscalable,brotatable,bpolymorph,bsize,bxres,byres;
  FXint    dencoding,dweight,dslant,dsetwidth,dpitch,dscalable,drotatable,dpolymorph,dsize;
  FXchar   candidate[256],xlfd[256];
  FXchar **fontnames;
  FXint    nfontnames,b,f;
  const FXchar *field[13];
  FXdouble c,s,a;
  XFontStruct *fs;

  FXTRACE(TOPIC_DETAIL,"FXFont::match: wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res);

  // Get fonts matching the pattern
  __snprintf(candidate,sizeof(candidate),"-%s-%s-*-*-*-*-*-%s-*-*-*-*-*-*",wantforge.empty()?"*":wantforge.text(),wantfamily.empty()?"*":wantfamily.text(),(hints&FXFont::Rotatable)?"[1 0 0 1]":"*");
  fontnames=XListFonts(DISPLAY(getApp()),candidate,65535,&nfontnames);
  if(fontnames && 0<nfontnames){

    b=-1;
    bencoding=100000;
    bweight=100000;
    bslant=100000;
    bsetwidth=100000;
    bpitch=100000;
    bscalable=100000;
    brotatable=100000;
    bpolymorph=100000;
    bsize=100000;
    bxres=res;
    byres=res;

    // Match them
    for(f=0; f<nfontnames; f++){

      //FXTRACE(TOPIC_DETAIL,"FXFont::match: font=%s\n",fontnames[f]);

      // Break apart into fields
      fxstrlcpy(candidate,fontnames[f],sizeof(candidate));
      xlfdSplit(field,candidate);

      // Get info
      weight=xlfdWeight(field[2]);
      slant=xlfdSlant(field[3]);
      setwidth=xlfdSetwidth(field[4]);
      scalable=xlfdScalable(field[6],field[7],field[11]);
      polymorph=xlfdPolymorph(field[2],field[3],field[4],field[5]);
      rotatable=xlfdRotatable(field[6],field[7]);
      points=atoi(field[7]);    // Deci-points
      xres=atoi(field[8]);
      yres=atoi(field[9]);
      pitch=xlfdPitch(field[10]);
      encoding=xlfdEncoding(field[12]);

      // The font can be rendered at any resolution, so render at actual device resolution
      if(xres==0 && yres==0){
        xres=res;
        yres=res;
        }

      // Encoding
      if(wantencoding==FONTENCODING_DEFAULT){
        dencoding=(encoding!=FONTENCODING_UNICODE);     // Want unicode if nothing else given
        }
      else{
        dencoding=(encoding!=wantencoding);
        }

      // Weight
      if(wantweight){
        dweight=Math::iabs(FXint(weight-wantweight));
        }
      else{
        dweight=Math::iabs(FXint(weight-FXFont::Normal));
        }

      // Slant
      if(wantslant){
        dslant=Math::iabs(FXint(slant-wantslant));
        }
      else{
        dslant=Math::iabs(FXint(slant-FXFont::Straight));
        }

      // Set width
      if(wantsetwidth){
        dsetwidth=Math::iabs(FXint(setwidth-wantsetwidth));
        }
      else{
        dsetwidth=Math::iabs(FXint(setwidth-FXFont::NonExpanded));
        }

      // Pitch
      dpitch=0;
      if(wanthints&FXFont::Variable){
        if(!(pitch&FXFont::Variable)) dpitch+=3;
        }
      else if(wanthints&FXFont::Mono){
        if(!(pitch&FXFont::Mono)) dpitch+=2;
        if(!(pitch&FXFont::Cell)) dpitch+=1;
        }
      else if(wanthints&FXFont::Cell){
        if(!(pitch&FXFont::Cell)) dpitch+=2;
        if(!(pitch&FXFont::Mono)) dpitch+=1;
        }

      // Scalable
      if(wanthints&FXFont::Scalable){
        dscalable=(FXFont::Scalable!=scalable);
        }
      else{
        dscalable=0;
        }

      // Rotatable
      if(wanthints&FXFont::Rotatable){
        drotatable=(FXFont::Rotatable!=rotatable);
        }
      else{
        drotatable=0;
        }

      // Polymorphic
      if(wanthints&FXFont::Polymorphic){
        dpolymorph=(FXFont::Polymorphic!=polymorph);
        }
      else{
        dpolymorph=0;
        }

      // If scalable, we can of course get the exact size we want
      // We do not set dsize to 0, as we prefer a bitmapped font that gets within
      // 10% over a scalable one that's exact, as the bitmapped fonts look much better
      // at small sizes than scalable ones...
      if(scalable){
        dsize=wantsize/10;
        size=wantsize;
        }

      // We correct for the actual screen resolution; if the font is rendered at a
      // 100 dpi, and we have a screen with 90dpi, the actual point size of the font
      // should be multiplied by (100/90).
      else{
        size=(yres*points)/res;
        dsize=Math::iabs(FXint(size-wantsize));
        }

      FXTRACE(TOPIC_DETAIL,"FXFont::match: %4d: dweight=%-3d dsize=%3d dslant=%d dsetwidth=%d dscalable=%d dpolymorph=%d xres=%-3d yres=%-3d xlfd=\"%s\"\n",f,dweight,dsize,dslant,dsetwidth,dscalable,dpolymorph,xres,yres,fontnames[f]);

      // But I'm NOT drinking any fucking Merlot!
      if((dencoding<bencoding) || ((dencoding==bencoding) && ((drotatable<brotatable) || ((drotatable==brotatable) && ((dpitch<bpitch) || ((dpitch==bpitch) && ((dsize<bsize) || ((dsize==bsize) && ((dweight<bweight) || ((dweight==bweight) && ((dslant<bslant) || ((dslant==bslant) && ((dsetwidth<bsetwidth) || ((dsetwidth==bsetwidth) && ((dscalable<bscalable) || ((dscalable==bscalable) && (dpolymorph<bpolymorph))))))))))))))))){

        // Best match
        actualName=field[1];
        actualName.append(" [");
        actualName.append(field[0]);
        actualName.append("]");
        actualSize=size;
        actualWeight=weight;
        actualSlant=slant;
        actualSetwidth=setwidth;
        actualEncoding=encoding;
        flags=pitch|scalable|polymorph|rotatable;

        // Closeness
        bencoding=dencoding;
        brotatable=drotatable;
        bpitch=dpitch;
        bsize=dsize;
        bweight=dweight;
        bslant=dslant;
        bsetwidth=dsetwidth;
        bscalable=dscalable;
        bpolymorph=dpolymorph;
        bxres=xres;
        byres=yres;
        b=f;
        }
      }

    // Got a font?
    if(0<=b){

      FXTRACE(TOPIC_DETAIL,"FXFont::match: bweight=%-3d bsize=%3d bslant=%d bsetwidth=%d bscalable=%d bpolymorph=%d bxres=%-3d byres=%-3d xlfd=\"%s\"\n",bweight,bsize,bslant,bsetwidth,bscalable,bpolymorph,bxres,byres,fontnames[b]);

      // Keep desired font name
      fxstrlcpy(xlfd,fontnames[b],sizeof(xlfd));

      // Free the list
      XFreeFontNames(fontnames);

/*
      // If font is scaled or rotated, build custom xlfd
      if(flags&(FXFont::Scalable|FXFont::Rotatable)){

        // Bust up XLFD into parts
        fxstrlcpy(candidate,xlfd,sizeof(candidate));
        xlfdSplit(field,candidate);

        // Create rotated font
        if(flags&FXFont::Rotatable){
          a=0.00027270769562411399179*angle;
          c=(cos(a)*res*actualSize)/(10.0*byres);
          s=(sin(a)*res*actualSize)/(10.0*byres);
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-[%s%.3f %s%.3f %s%.3f %s%.3f]-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],SGN(c),fabs(c),SGN(s),fabs(s),SGN(-s),fabs(s),SGN(c),fabs(c),bxres,byres,field[10],field[12]);
          }

        // Create scaled font
        else{
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],(res*actualSize)/byres,bxres,byres,field[10],field[12]);
          }
        }

      // Try load it
      font=XLoadQueryFont(DISPLAY(getApp()),xlfd);
*/

      // If font is scaled or rotated, build custom xlfd
      if(flags&(FXFont::Scalable|FXFont::Rotatable)){

        // Bust up XLFD into parts
        fxstrlcpy(candidate,xlfd,sizeof(candidate));
        xlfdSplit(field,candidate);

        // Create scaled font
        __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],(res*actualSize)/byres,bxres,byres,field[10],field[12]);

        FXTRACE(TOPIC_DETAIL,"FXFont::match: XLoadQueryFont(\"%s\")\n",xlfd);

        // Load normal scaled font
        fs=XLoadQueryFont(DISPLAY(getApp()),xlfd);

        // This is an ugly workaround:- according to docs, the X11 server is supposed to
        // fill XCharStruct's in the XFontStruct such that the attribute data represents
        // the escapement amount; however, this appears to be total garbage for all but a
        // few of the installed fonts; moreover, the width member of the XCharStruct is
        // also total garbage for these rotated fonts.
        // The fix here is very tricky, but it works:- first, we grab the normal horizontal
        // font's XFontStruct using XLoadQueryFont().  Then, we XUnloadFont() the font while
        // keeping the XFontStruct.  Next, we load the rotated font using XLoadFont() and
        // plug the font id into the XFontStruct.  This works, since while the XCharStruct's
        // may sometimes be located in read-only shared memory, the XFontStruct is always
        // allocated in the client by Xlib.
        if(fs && (flags&FXFont::Rotatable)){
          a=0.00027270769562411399179*angle;
          c=(Math::cos(a)*res*actualSize)/(10.0*byres);
          s=(Math::sin(a)*res*actualSize)/(10.0*byres);
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-[%s%.3f %s%.3f %s%.3f %s%.3f]-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],SGN(c),Math::fabs(c),SGN(s),Math::fabs(s),SGN(-s),Math::fabs(s),SGN(c),Math::fabs(c),bxres,byres,field[10],field[12]);
          XUnloadFont(DISPLAY(getApp()),((XFontStruct*)fs)->fid);
          ((XFontStruct*)fs)->fid=XLoadFont(DISPLAY(getApp()),xlfd);
          }
        }

      // Simple case for horizontally drawn fonts
      else{

        FXTRACE(TOPIC_DETAIL,"FXFont::match: XLoadQueryFont(\"%s\")\n",xlfd);

        // Load normal scaled font
        fs=XLoadQueryFont(DISPLAY(getApp()),xlfd);
        }

/*
      if(font){
        for(b=0; b<((XFontStruct*)font)->n_properties; b++){
          if(((XFontStruct*)font)->properties[b].name==XA_FONT){
            char *fn=XGetAtomName(DISPLAY(getApp()),((XFontStruct*)font)->properties[b].card32);
            fxstrlcpy(candidate,fn,sizeof(candidate));
            FXTRACE(TOPIC_DETAIL,"FXFont::match: FONT = %s\n",candidate);
            xlfdSplit(field,candidate);
            XFree(fn);
            FXTRACE(TOPIC_DETAIL,"FXFont::match: mat  = %s\n",field[6]);
            }
          else{
            FXTRACE(TOPIC_DETAIL,"FXFont::match: %d (%s) = %d\n",((XFontStruct*)font)->properties[b].name,XGetAtomName(DISPLAY(getApp()),((XFontStruct*)font)->properties[b].name),((XFontStruct*)font)->properties[b].card32);
            }
          }
        }
*/
      return fs;
      }
    }
  return nullptr;
  }


#endif //////////////////////////////////////////////////////////////////////////


// Create font
void FXFont::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);

#if defined(HAVE_XFT_H)       ///// XFT /////

      FXString family=getFamily();
      FXString foundry=getFoundry();
      FXint    res;

      // Override screen resolution via registry
      res=getApp()->reg().readUIntEntry("SETTINGS","screenres",100);

      FXTRACE(TOPIC_CREATION,"%s::create: xft font\n",getClassName());

      // Try to match with specified family and foundry
      if(!family.empty()){
        family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
        if(!foundry.empty()){
          foundry=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",foundry.text(),foundry.text());
          font=match(family,foundry,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
          }
        if(!font){
          font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
          }
        }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }

#else                           ///// XLFD /////

      FXString family=getFamily();
      FXString foundry=getFoundry();
      FXint    res;

      // Override screen resolution via registry
      res=getApp()->reg().readUIntEntry("SETTINGS","screenres",100);

      FXTRACE(TOPIC_CREATION,"%s::create: xlfd font\n",getClassName());

      // X11 font specification
      if(hints&FXFont::X11){

        // Resolve font name
        actualName=xlfdFont(DISPLAY(getApp()),wantedName);

        // Try load the font
        font=XLoadQueryFont(DISPLAY(getApp()),actualName.text());
        }

      // Platform independent specification
      if(!font){

        // First we try to match with specified family and foundry
        if(!family.empty()){
          family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
          if(!foundry.empty()){
            foundry=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",foundry.text(),foundry.text());
            font=match(family,foundry,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          if(!font){
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          }

        // Try based on hints
        if(!font){

          // Try swiss if we want swiss or indicated no preference
          if((hints&(FXFont::Swiss|FXFont::System)) || !(hints&(FXFont::Decorative|FXFont::Modern|FXFont::Roman|FXFont::Script|FXFont::Swiss|FXFont::System))){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","helvetica","helvetica");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","lucida","lucida");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try roman
          else if(hints&FXFont::Roman){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","times","times");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","charter","charter");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try modern
          else if(hints&FXFont::Modern){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","courier","courier");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","lucidatypewriter","lucidatypewriter");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try decorative
          else if(hints&FXFont::Decorative){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","gothic","gothic");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }

          // Try anything
          if(!font){
            font=match(FXString::null,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          }
        }

      // If we still don't have a font yet, use fixed font
      if(!font){

        // Resolve font name
        actualName="fixed";

        // Try load the font
        font=XLoadQueryFont(DISPLAY(getApp()),actualName.text());
        }

      // Remember font id
      if(font){ xid=((XFontStruct*)font)->fid; }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }

#endif

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

#if defined(HAVE_XFT_H)       ///// XFT /////

    XftFontClose(DISPLAY(getApp()),(XftFont*)font);

#else                           ///// XLFD /////

    XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);

#endif

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

#if defined(HAVE_XFT_H)       ///// XFT /////

      // Free font
      XftFontClose(DISPLAY(getApp()),(XftFont*)font);

#else                           ///// XLFD /////

      // Free font
      XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);

#endif
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


#if defined(HAVE_XFT_H)       ///////  X Freetype ///////


// List all fonts that match the passed requirements
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  int          encoding,setwidth,weight,slant,size,pitch,scalable,res,i,j;
  FXchar       fullname[256];
  FcPattern   *pattern,*p;
  FcObjectSet *objset;
  FcFontSet   *fontset;
  FcChar8     *fam,*fdy;
  FcBool       scale;
  FcCharSet   *charset;
  FXString     family;
  FXString     foundry;
  double       points;

  FXUNUSED(en);

  fonts=nullptr;
  numfonts=0;

  // Need to have application
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }

  // Need to have display open
  if(!DISPLAY(FXApp::instance())){ fxerror("FXFont::listFonts: trying to list fonts before opening display.\n"); }

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

      // If we set this we get no fonts
//      if(h&FXFont::Rotatable){
//        const FcMatrix matrix={1.0,0.0,0.0,1.0};
//        FcPatternAddMatrix(pattern,FC_MATRIX,&matrix);
//        }

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
              weight=fcWeight2Weight(weight);   // FIXME
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

            // Get charset
            if(FcPatternGetCharSet(p,FC_CHARSET,0,&charset)==FcResultMatch){    // FIXME
              }

            // Get the encoding
            encoding=FONTENCODING_UNICODE;

            FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: wt=%2d sl=%d sw=%3d en=%5d sz=%3d sc=%4x pi=%d name=%s\n",weight,slant,setwidth,encoding,size,scalable,pitch,fullname);

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


#else                           ///////  X XLFD ///////


// Try find matching font
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  FXuint   encoding,weight,slant,setwidth,pitch,scalable,rotatable,polymorph,xres,yres,points,size,res;
  FXchar   candidate[256],fullname[256];
  const FXchar  *field[13];
  FXchar **fontnames;
  FXint    nfontnames,f,j;
  FXString family;
  FXString foundry;

  fonts=nullptr;
  numfonts=0;

  // Need to have application
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }

  // Need to have display open
  if(!DISPLAY(FXApp::instance())){ fxerror("FXFont::listFonts: trying to list fonts before opening display.\n"); }

  // Screen resolution may be overidden by registry
  res=FXApp::instance()->reg().readUIntEntry("SETTINGS","screenres",100);

  // Get family part of name
  family=face.before('[').trimEnd();

  // Get foundry part of name
  foundry=face.section("[]",1);

  FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: family=\"%s\" foundry=\"%s\" weight=%d slant=%d setwidth=%d encoding=%d hints=%x\n",family.text(),foundry.text(),wt,sl,sw,en,h);

  // Match RAW X11
  if(h&FXFont::X11){
    __snprintf(candidate,sizeof(candidate),"%s",face.empty()?"*":face.text());
    }

  // Match XLFD
  else{
    __snprintf(candidate,sizeof(candidate),"-%s-%s-*-*-*-*-*-%s-*-*-*-*-*-*",foundry.empty() ? "*" : foundry.text(),family.empty() ? "*" : family.text(),(h&FXFont::Rotatable) ? "[1 0 0 1]" : "*");
    }

  // Get fonts matching the pattern
  fontnames=XListFonts(DISPLAY(FXApp::instance()),candidate,65535,&nfontnames);
  if(fontnames && 0<nfontnames){

    // Allocate return array
    if(!allocElms(fonts,nfontnames)){ XFreeFontNames(fontnames); return false; }

    // List them
    for(f=0; f<nfontnames; f++){

      // Break apart into fields
      fxstrlcpy(candidate,fontnames[f],sizeof(candidate));
      xlfdSplit(field,candidate);

      // Get info
      weight=xlfdWeight(field[2]);
      slant=xlfdSlant(field[3]);
      setwidth=xlfdSetwidth(field[4]);
      scalable=xlfdScalable(field[6],field[7],field[11]);
      polymorph=xlfdPolymorph(field[2],field[3],field[4],field[5]);
      rotatable=xlfdRotatable(field[6],field[7]);
      points=atoi(field[7]);
      xres=atoi(field[8]);
      yres=atoi(field[9]);
      pitch=xlfdPitch(field[10]);
      encoding=xlfdEncoding(field[12]);

      FXTRACE(TOPIC_DETAIL,"FXFont::listFonts: wt=%2d sl=%d sw=%3d en=%5d pt=%3d sc=%4x po=%4x ro=%4x xlfd=%s\n",weight,slant,setwidth,encoding,points,scalable,polymorph,rotatable,fontnames[f]);

      // The font can be rendered at any resolution
      if(xres==0 && yres==0){ xres=res; yres=res; }

      // Skip if encoding does not match
      if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) continue;

      // Skip if pitch does not match
      if((h&FXFont::Variable) && !(pitch&FXFont::Variable)) continue;
      if((h&(FXFont::Mono|FXFont::Cell)) && !(pitch&(FXFont::Mono|FXFont::Cell))) continue;

      // Skip if weight does not match
      if((wt!=0) && (wt!=weight)) continue;

      // Skip if slant does not match
      if((sl!=0) && (sl!=slant)) continue;

      // Skip if setwidth does not match
      if((sw!=0) && (sw!=setwidth)) continue;

      // Want rotatable
      if((h&FXFont::Rotatable) && (rotatable!=FXFont::Rotatable)) continue;

      // Want scalable
      if((h&FXFont::Scalable) && (scalable!=FXFont::Scalable)) continue;

      // If scalable, we can of course get the exact size we want
      if(scalable){
        size=0;
        }

      // Correct for the actual screen resolution
      else{
        size=(yres*points)/res;
        }

      // Get full face name
      fxstrlcpy(fullname,field[1],sizeof(fullname));
      if(field[0][0]){
        fxstrlcat(fullname," [",sizeof(fullname));
        fxstrlcat(fullname,field[0],sizeof(fullname));
        fxstrlcat(fullname,"]",sizeof(fullname));
        }

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
      fonts[numfonts].flags=pitch|scalable|polymorph|rotatable;
      numfonts++;

      // Next font
next: continue;
      }

    // Free the list
    XFreeFontNames(fontnames);

    // Realloc to shrink the block
    resizeElms(fonts,numfonts);

    // Sort them by name, weight, slant, and size respectively
    ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);
    }
  return (0<numfonts);
  }


#endif


// Does font have given character glyph?
FXbool FXFont::hasChar(FXwchar ch) const {
  if(font){
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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
#if defined(HAVE_XFT_H)       ///// XFT /////
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


// Text height
FXint FXFont::getTextHeight(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextHeight: NULL string argument\n",getClassName()); }
  if(font){
#if defined(HAVE_XFT_H)       ///// XFT /////
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


}

#endif
