/********************************************************************************
*                                                                              *
*                            C S S   P a r s e r                               *
*                                                                              *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                              *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXFile.h"
#include "FXColors.h"
#include "FXTheme.h"
#include "FXCSS.h"


/*
  Notes:
  - Only the CSS subset that GTK themes actually use is handled; anything
    else is counted as a warning instead of failing the import.
  - @define-color values may refer to other named colors, so lookup follows
    the chain, with a bound to survive cyclic themes.
  - Lengths are converted to pixels using the nominal 96 dpi, which keeps the
    import independent of the machine it runs on.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Nominal resolution for converting physical units to pixels
#define CSS_DPI    96.0

// Longest chain of color indirections we follow
#define CSS_DEPTH  8

// A few names GTK themes use without defining them
struct FXCSSName { const FXchar* name; FXuint rgb; };

static const FXCSSName cssnames[]={
  { "white",       0xffffff },
  { "black",       0x000000 },
  { "red",         0xff0000 },
  { "green",       0x00ff00 },
  { "blue",        0x0000ff },
  { "gray",        0x808080 },
  { "grey",        0x808080 },
  { "silver",      0xc0c0c0 },
  { "transparent", 0x000000 }
  };


// Strip comments from a style sheet
static FXString stripComments(const FXString& text){
  FXString result;
  FXint i=0,n=text.length();
  while(i<n){
    if(text[i]=='/' && (i+1)<n && text[i+1]=='*'){
      i+=2;
      while(i<n && !(text[i]=='*' && (i+1)<n && text[i+1]=='/')) i++;
      i+=2;
      }
    else{
      result.append(text[i++]);
      }
    }
  return result;
  }


// Parse a number with an optional unit into pixels
static FXbool parseLength(const FXString& text,FXint& result){
  FXbool ok=false;
  FXString number;
  FXint i=0,n=text.length();
  FXdouble value;
  while(i<n && (text[i]==' ' || text[i]=='\t')) i++;
  while(i<n && ((text[i]>='0' && text[i]<='9') || text[i]=='.' || text[i]=='-' || text[i]=='+')) number.append(text[i++]);
  if(number.empty()) return false;
  value=number.toDouble(&ok);
  if(!ok) return false;
  FXString unit=text.mid(i,text.length()-i).trim();
  if(unit=="px" || unit.empty()){ result=FXint(value+0.5); return true; }
  if(unit=="mm"){ result=FXint(value*CSS_DPI/25.4+0.5); return true; }
  if(unit=="cm"){ result=FXint(value*CSS_DPI/2.54+0.5); return true; }
  if(unit=="pt"){ result=FXint(value*CSS_DPI/72.0+0.5); return true; }
  if(unit=="em"){ result=FXint(value*12.0+0.5); return true; }           // Assume a 12 pixel em
  return false;
  }


// Construct reader
FXCSS::FXCSS(){
  warnings=0;
  ruleno=0;
  }


// Destructor
FXCSS::~FXCSS(){
  }


// Count an unsupported construct
void FXCSS::warn(const FXchar* format,...) const {
  va_list args;
  va_start(args,format);
  vfprintf(stderr,format,args);
  va_end(args);
  ((FXCSS*)this)->warnings++;
  }


// Remember a named color
FXbool FXCSS::addColor(const FXString& name,const FXString& value){
  if(name.empty()) return false;
  colors.insert(name,value);
  return true;
  }


// Resolve a named color, following @name chains
FXColor FXCSS::lookupColor(const FXString& name,FXint depth) const {
  FXString text=name;
  FXbool ok=false;
  FXint i;

  // Follow @name indirections
  for(i=0; i<depth; i++){
    text=text.trim();
    if(text.empty()) return 0;
    if(text[0]!='@') break;
    if(!colors.has(text.mid(1,text.length()-1))) return 0;
    text=colors.at(text.mid(1,text.length()-1));
    }

  // Named color
  if(text[0]!='#' && text[0]!='r' && text[0]!='R'){
    for(i=0; i<(FXint)ARRAYNUMBER(cssnames); i++){
      if(FXString::comparecase(text,cssnames[i].name)==0){
        return FXRGB(cssnames[i].rgb>>16,(cssnames[i].rgb>>8)&0xff,cssnames[i].rgb&0xff);
        }
      }
    }

  // #rgb, #rrggbb or #rrggbbaa
  if(text[0]=='#'){
    FXString digits=text.mid(1,text.length()-1);
    if(digits.length()==3){
      FXint r=digits.mid(0,1).toInt(16,&ok); if(!ok) return 0;
      FXint g=digits.mid(1,1).toInt(16,&ok); if(!ok) return 0;
      FXint b=digits.mid(2,1).toInt(16,&ok); if(!ok) return 0;
      return FXRGB(r*17,g*17,b*17);
      }
    if(digits.length()==6 || digits.length()==8){
      FXint r=digits.mid(0,2).toInt(16,&ok); if(!ok) return 0;
      FXint g=digits.mid(2,2).toInt(16,&ok); if(!ok) return 0;
      FXint b=digits.mid(4,2).toInt(16,&ok); if(!ok) return 0;
      return FXRGB(r,g,b);
      }
    return 0;
    }

  // rgb() and rgba(); alpha is ignored because themes are opaque
  if(FXString::comparecase(text.left(4),"rgb(")==0 || FXString::comparecase(text.left(5),"rgba(")==0){
    FXint open=text.find('(');
    FXint close=text.find(')',open+1);
    if(open<0 || close<0) return 0;
    FXString args=text.mid(open+1,close-open-1);
    FXString part=args.section(',',0).trim();
    FXint r=part.toInt(10,&ok); if(!ok) return 0;
    FXint g=args.section(',',1).trim().toInt(10,&ok); if(!ok) return 0;
    FXint b=args.section(',',2).trim().toInt(10,&ok); if(!ok) return 0;
    return FXRGB(FXCLAMP(0,r,255),FXCLAMP(0,g,255),FXCLAMP(0,b,255));
    }

  return 0;
  }


// Store one declaration under the given selector
void FXCSS::addDeclaration(const FXString& selector,const FXString& key,const FXString& value){
  FXString entry;
  if(selector.empty() || key.empty() || value.empty()) return;
  entry=rules.at(selector);
  if(!entry.empty()) entry.append(';');
  entry.append(key);
  entry.append(':');
  entry.append(value);
  rules.insert(selector,entry);
  }


// Store the declarations of one rule
void FXCSS::parseDeclarations(const FXString& selector,const FXString& body){
  FXString clean=selector;
  FXString key;
  FXString value;
  FXint i=0,n=body.length();

  // Normalize the selector: drop comments and squeeze whitespace
  clean=clean.simplify();
  clean=clean.trim();
  if(clean.empty()) return;

  // Keep only declarations, in the form prop:value
  while(i<n){
    FXint start=i;
    while(i<n && body[i]!=';') i++;
    FXString decl=body.mid(start,i-start).trim();
    i++;
    if(decl.empty()) continue;
    FXint colon=decl.find(':');
    if(colon<0) continue;
    key=decl.left(colon).trim();
    value=decl.mid(colon+1,decl.length()-colon-1).trim();
    if(key.empty() || value.empty()) continue;
    addDeclaration(clean,key,value);
    }
  if(!rules.at(clean).empty()) ruleno++;
  }


// Parse one @define-color statement
void FXCSS::parseDefine(const FXString& text,FXint& pos){
  FXint n=text.length();
  FXint start=pos;
  while(pos<n && text[pos]!=';') pos++;
  FXString stmt=text.mid(start,pos-start);
  pos++;
  stmt=stmt.mid(stmt.find(' '),stmt.length()-stmt.find(' ')).trim();   // Skip the keyword
  FXint space=stmt.find(' ');
  if(space<0) return;
  addColor(stmt.left(space).trim(),stmt.mid(space+1,stmt.length()-space-1).trim());
  }


// At-rules that carry no color or metric information for us
static FXbool knownAtRule(const FXString& name){
  return name=="@keyframes" || name=="@import" || name=="@binding-set" || name=="@font-face" || name=="@define-color";
  }


// Parse the style sheet body
void FXCSS::parseBlock(const FXString& text,FXint& pos){
  FXint n=text.length();
  FXint start=pos;
  while(pos<n){
    if(text[pos]=='{'){
      FXString selector=text.mid(start,pos-start);
      FXint begin=++pos;
      while(pos<n && text[pos]!='}') pos++;
      FXString body=text.mid(begin,pos-begin);
      pos++;
      parseDeclarations(selector,body);
      start=pos;
      }
    else if(text[pos]=='@'){
      FXint keyword=pos;
      while(pos<n && text[pos]!=' ' && text[pos]!=';' && text[pos]!='{') pos++;
      FXString name=text.mid(keyword,pos-keyword);
      if(name=="@define-color" || name=="@define_color"){
        pos=keyword;
        parseDefine(text,pos);
        start=pos;
        }
      else{
        // Skip at-rules we do not understand, but quietly skip the ones that
        // carry no color information at all
        if(!knownAtRule(name)) warn("fxtheme: skipping unsupported at-rule '%s'\n",name.text());
        while(pos<n && text[pos]==' ') pos++;
        if(pos<n && text[pos]=='{'){
          FXint depth=0;
          while(pos<n){
            if(text[pos]=='{') depth++;
            if(text[pos]=='}' && --depth==0){ pos++; break; }
            pos++;
            }
          }
        else{
          while(pos<n && text[pos]!=';') pos++;
          if(pos<n) pos++;
          }
        start=pos;
        }
      }
    else{
      pos++;
      }
    }
  }


// Read a GTK 3/4 style sheet
FXbool FXCSS::loadCSS(const FXString& path){
  FXFile file(path,FXIO::Reading);
  FXString text;
  FXint pos=0;
  if(!file.isOpen()) return false;
  text.length(file.size());
  file.readBlock(text.text(),text.length());
  source=path;
  parseBlock(stripComments(text),pos);
  return true;
  }


// Test if a file can be opened for reading
static FXbool readable(const FXString& path){
  FXFile file(path,FXIO::Reading);
  return file.isOpen();
  }


// Return how often the given character occurs in the text
static FXint countChar(const FXString& text,FXchar ch){
  FXint count=0;
  FXint i;
  for(i=0; i<text.length(); i++){
    if(text[i]==ch) count++;
    }
  return count;
  }


// Read a GTK 2 gtkrc
FXbool FXCSS::loadGtkrc(const FXString& path){
  FXFile file(path,FXIO::Reading);
  FXString text;
  FXString line;
  FXint start=0;
  FXint stop;
  FXint depth=0;
  if(!file.isOpen()) return false;
  text.length(file.size());
  file.readBlock(text.text(),text.length());
  file.close();
  source=path;
  while(start<text.length()){
    stop=text.find('\n',start);
    if(stop<0) stop=text.length();
    line=text.mid(start,stop-start).trim();
    start=stop+1;
    if(line.empty() || line[0]=='#') continue;

    // Inside a style block only the braces are tracked; colors are ignored
    if(0<depth){
      depth+=(countChar(line,'{')-countChar(line,'}'));
      continue;
      }
    if(line.find('{')>=0){
      depth+=(countChar(line,'{')-countChar(line,'}'));
      continue;
      }

    // gtk-color-scheme = "name:#rrggbb"
    if(FXString::comparecase(line.left(16),"gtk-color-scheme")==0){
      FXint open=line.find('"');
      FXint close=line.rfind('"');
      if(0<=open && open<close){
        FXString entry=line.mid(open+1,close-open-1);
        FXint colon=entry.find(':');
        if(0<colon){
          addColor(entry.left(colon).trim(),entry.mid(colon+1,entry.length()-colon-1).trim());
          }
        }
      continue;
      }

    // font_name = "Sans 10"
    if(FXString::comparecase(line.left(9),"font_name")==0){
      FXint open=line.find('"');
      FXint close=line.rfind('"');
      if(0<=open && open<close){
        rules.insert("gtk-font",line.mid(open+1,close-open-1));
        ruleno++;
        }
      continue;
      }
    }
  return true;
  }


// Read a KDE color scheme; sections become selectors, keys become declarations
FXbool FXCSS::loadKdeColors(const FXString& path){
  FXFile file(path,FXIO::Reading);
  FXString text;
  FXString line;
  FXString section;
  FXint start=0;
  FXint stop;
  FXint equal;
  if(!file.isOpen()) return false;
  text.length(file.size());
  file.readBlock(text.text(),text.length());
  file.close();
  source=path;
  while(start<text.length()){
    stop=text.find('\n',start);
    if(stop<0) stop=text.length();
    line=text.mid(start,stop-start).trim();
    start=stop+1;
    if(line.empty() || line[0]=='#' || line[0]==';') continue;

    // Section header; the brackets stay part of the selector
    if(line[0]=='['){
      FXint close=line.find(']');
      section=(0<close)?line.mid(0,close+1):line;
      continue;
      }

    // Key=Value
    equal=line.find('=');
    if(equal<=0) continue;
    addDeclaration(section,line.left(equal).trim(),line.mid(equal+1,line.length()-equal-1).trim());
    }
  ruleno++;
  return true;
  }


// Read either format
FXbool FXCSS::load(const FXString& path){
  // A theme directory: prefer GTK 3, then GTK 4, then GTK 2
  if(readable(path+"/gtk-3.0/gtk.css")) return loadCSS(path+"/gtk-3.0/gtk.css");
  if(readable(path+"/gtk-4.0/gtk.css")) return loadCSS(path+"/gtk-4.0/gtk.css");
  if(readable(path+"/gtk-2.0/gtkrc"))  return loadGtkrc(path+"/gtk-2.0/gtkrc");
  if(path.right(5)=="gtkrc") return loadGtkrc(path);
  if(path.right(7)==".colors") return loadKdeColors(path);
  return loadCSS(path);
  }


// Return true if the given color name was defined
FXbool FXCSS::hasColor(const FXString& name) const {
  return colors.has(name);
  }


// Return named color
FXColor FXCSS::color(const FXString& name) const {
  if(!colors.has(name)) return 0;
  return lookupColor(FXString("@")+name,CSS_DEPTH);
  }


// Find a declaration in the rule that best matches the given element
FXString FXCSS::property(const FXString& match,const FXString& prop) const {
  FXival pos;
  FXString found;
  FXint pass;

  // First prefer a rule whose selector list names the element exactly, then
  // accept any rule that merely mentions it
  for(pass=0; pass<2; pass++){
    for(pos=0; pos<rules.no(); pos++){
      FXString selector;
      if(rules.empty(pos)) continue;
      selector=rules.key(pos);
      if(pass==0 && selector!=match) continue;
      if(pass==1 && selector.find(match)<0) continue;
      FXString body=rules.data(pos);
      FXint start=0;
      while(start<body.length()){
        FXint stop=body.find(';',start);
        if(stop<0) stop=body.length();
        FXString decl=body.mid(start,stop-start);
        FXint colon=decl.find(':');
        if(0<colon && FXString::comparecase(decl.left(colon).trim(),prop)==0){
          return decl.mid(colon+1,decl.length()-colon-1).trim();
          }
        start=stop+1;
        }
      }
    }
  return found;
  }


// Return a length declaration converted to pixels; zero values are skipped
// because GTK often resets a property to zero in a more specific rule
FXint FXCSS::length(const FXString& match,const FXString& prop,FXint def) const {
  FXival pos;
  FXint pass;
  for(pass=0; pass<2; pass++){
    for(pos=0; pos<rules.no(); pos++){
      FXString body;
      FXint start=0;
      if(rules.empty(pos)) continue;
      if(pass==0 && rules.key(pos)!=match) continue;
      if(pass==1 && rules.key(pos).find(match)<0) continue;
      body=rules.data(pos);
      while(start<body.length()){
        FXint stop=body.find(';',start);
        FXint result;
        FXString decl;
        FXint colon;
        if(stop<0) stop=body.length();
        decl=body.mid(start,stop-start);
        start=stop+1;
        colon=decl.find(':');
        if(colon<=0) continue;
        if(FXString::comparecase(decl.left(colon).trim(),prop)!=0) continue;
        if(parseLength(decl.mid(colon+1,decl.length()-colon-1).trim(),result) && 0<result) return result;
        }
      }
    }
  return def;
  }


// Resolve an "r,g,b" triplet, as KDE color schemes write them
FXColor FXCSS::lookupTriplet(const FXString& text) const {
  FXbool ok=false;
  FXint r=text.section(',',0).trim().toInt(10,&ok);
  FXint g;
  FXint b;
  if(!ok) return 0;
  g=text.section(',',1).trim().toInt(10,&ok);
  if(!ok) return 0;
  b=text.section(',',2).trim().toInt(10,&ok);
  if(!ok) return 0;
  return FXRGB(FXCLAMP(0,r,255),FXCLAMP(0,g,255),FXCLAMP(0,b,255));
  }


// Map a KDE color scheme onto the theme
void FXCSS::applyKde(FXTheme& theme) const {
  FXString value;
  FXColor clr;

  // Window and view colors
  value=property("[Colors:Window]","BackgroundNormal");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_SURFACE,clr); }
  value=property("[Colors:View]","BackgroundNormal");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_WINDOW_BG,clr); }
  value=property("[Colors:Window]","ForegroundNormal");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_TEXT,clr); }
  value=property("[Colors:Window]","ForegroundInactive");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_TEXT_DISABLED,clr); }

  // A hairline drawn as the alternate surface color, or derived below
  value=property("[Colors:Window]","BackgroundAlternate");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_BORDER,clr); }

  // Selection, accent and the focus decoration
  value=property("[Colors:Selection]","BackgroundNormal");
  if(!value.empty()){
    clr=lookupTriplet(value);
    if(clr){
      theme.setColor(ROLE_SELECTION_BG,clr);
      theme.setColor(ROLE_MENU_SEL_BG,clr);
      theme.setColor(ROLE_ACCENT,clr);
      }
    }
  value=property("[Colors:Selection]","ForegroundNormal");
  if(!value.empty()){
    clr=lookupTriplet(value);
    if(clr){
      theme.setColor(ROLE_SELECTION_TEXT,clr);
      theme.setColor(ROLE_MENU_SEL_TEXT,clr);
      }
    }
  value=property("[Colors:Button]","DecorationFocus");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_FOCUS_RING,clr); }

  // Tooltips
  value=property("[Colors:Tooltip]","BackgroundNormal");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_TOOLTIP_BG,clr); }
  value=property("[Colors:Tooltip]","ForegroundNormal");
  if(!value.empty()){ clr=lookupTriplet(value); if(clr) theme.setColor(ROLE_TOOLTIP_TEXT,clr); }

  // Fonts are written as QFont strings: family,pointsize,pixelsize,stylehint,weight,italic,...
  value=property("[General]","font");
  if(!value.empty()){
    FXFontDesc desc;
    FXString face=value.section(',',0).trim();
    FXint points=value.section(',',1).trim().toInt(10);
    FXint weight=value.section(',',4).trim().toInt(10);
    FXint italic=value.section(',',5).trim().toInt(10);
    desc.setFont(FXString::value("%s,%d",face.text(),(0<points)?points*10:0));
    if(600<=weight) desc.weight=FXFont::Bold;
    if(0<italic) desc.slant=FXFont::Italic;
    theme.setFont(FONT_UI,desc);
    }

  // Raised and sunken edges follow the imported surface
  theme.setColor(ROLE_SURFACE_HILITE,makeHiliteColor(theme.color(ROLE_SURFACE)));
  theme.setColor(ROLE_SURFACE_SHADOW,makeShadowColor(theme.color(ROLE_SURFACE)));
  theme.updateDerived();
  }


// Map the GTK colors and metrics onto the theme
void FXCSS::apply(FXTheme& theme) const {
  static const FXchar* surfaces[]={ "theme_bg_color","bg_color","theme_bg" };
  static const FXchar* bases[]={ "theme_base_color","base_color" };
  static const FXchar* texts[]={ "theme_text_color","theme_fg_color","fg_color","text_color" };
  static const FXchar* sels[]={ "theme_selected_bg_color","selected_bg_color" };
  static const FXchar* selfgs[]={ "theme_selected_fg_color","selected_fg_color" };
  static const FXchar* disfg[]={ "insensitive_fg_color","theme_unfocused_fg_color" };
  static const FXchar* bords[]={ "borders","border_color","theme_border_color" };
  static const FXchar* tips[]={ "theme_tooltip_bg_color","tooltip_bg_color" };
  static const FXchar* tipfg[]={ "theme_tooltip_fg_color","tooltip_fg_color" };
  static const FXchar* accents[]={ "theme_selected_bg_color","accent_bg_color","accent_color" };
  FXColor clr;

  // Base colors, in order of preference
  if((clr=color(surfaces[0]))==0 && (clr=color(surfaces[1]))==0) clr=color(surfaces[2]);
  if(clr) theme.setColor(ROLE_SURFACE,clr);
  if((clr=color(bases[0]))==0) clr=color(bases[1]);
  if(clr) theme.setColor(ROLE_WINDOW_BG,clr);
  if((clr=color(texts[0]))==0 && (clr=color(texts[1]))==0 && (clr=color(texts[2]))==0) clr=color(texts[3]);
  if(clr) theme.setColor(ROLE_TEXT,clr);
  if((clr=color(sels[0]))==0) clr=color(sels[1]);
  if(clr){
    theme.setColor(ROLE_SELECTION_BG,clr);
    theme.setColor(ROLE_MENU_SEL_BG,clr);
    }
  if((clr=color(selfgs[0]))==0) clr=color(selfgs[1]);
  if(clr){
    theme.setColor(ROLE_SELECTION_TEXT,clr);
    theme.setColor(ROLE_MENU_SEL_TEXT,clr);
    }
  if((clr=color(disfg[0]))==0) clr=color(disfg[1]);
  if(clr) theme.setColor(ROLE_TEXT_DISABLED,clr);
  if((clr=color(bords[0]))==0 && (clr=color(bords[1]))==0) clr=color(bords[2]);
  if(clr) theme.setColor(ROLE_BORDER,clr);
  if((clr=color(tips[0]))==0) clr=color(tips[1]);
  if(clr) theme.setColor(ROLE_TOOLTIP_BG,clr);
  if((clr=color(tipfg[0]))==0) clr=color(tipfg[1]);
  if(clr) theme.setColor(ROLE_TOOLTIP_TEXT,clr);
  if((clr=color(accents[0]))==0 && (clr=color(accents[1]))==0) clr=color(accents[2]);
  if(clr) theme.setColor(ROLE_ACCENT,clr);

  // Metrics taken from the button rule, falling back on the theme defaults
  theme.setMetric(METRIC_CORNER_RADIUS,length("button","border-radius",theme.metric(METRIC_CORNER_RADIUS)));
  theme.setMetric(METRIC_BORDER_WIDTH,length("button","border-width",theme.metric(METRIC_BORDER_WIDTH)));
  theme.setMetric(METRIC_PADDING,length("button","padding",theme.metric(METRIC_PADDING)));
  theme.setMetric(METRIC_FOCUS_WIDTH,length("button","outline-width",theme.metric(METRIC_FOCUS_WIDTH)));

  // Raised and sunken edges follow the imported surface
  theme.setColor(ROLE_SURFACE_HILITE,makeHiliteColor(theme.color(ROLE_SURFACE)));
  theme.setColor(ROLE_SURFACE_SHADOW,makeShadowColor(theme.color(ROLE_SURFACE)));

  // Font from the GTK 2 gtkrc, when present
  FXString font=rules.at("gtk-font");
  if(!font.empty()){
    FXFontDesc desc;
    desc.setFont(font);
    theme.setFont(FONT_UI,desc);
    }

  // Recompute the colors derived from the base roles
  theme.updateDerived();

  // KDE color schemes carry their colors under section/key names
  applyKde(theme);
  }

}
