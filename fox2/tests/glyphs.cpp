/********************************************************************************
*                                                                               *
*                          G l y p h   C a c h e   T e s t                     *
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
#include "fx.h"
#include "testargs.h"
#include "../lib/fxglyphs.h"
#include <stdio.h>
#include <string.h>

/*
  Notes:

  - Phase 2 gate of docs/wayland-plan.md: the platform neutral glyph cache must
    give the same font, the same metrics and the same advances as the Xft path,
    because the widget layout depends on them.
  - On an X11 build with a display the two are compared directly: FXFont (Xft)
    against FXGlyphs (fontconfig plus FreeType) for the same font description.
    The tolerance is one pixel, which is the rounding of the metrics.
  - Without FreeType and fontconfig the test is skipped, so it builds on Win32.
*/

// Font description used by both paths
static const FXchar* fontfamily="sans";
static const FXuint fontsize=100;             // Deci-points
static const FXint  fontdpi=96;


#if defined(FOX_GLYPHS_AVAILABLE)

// Number of failed checks
static FXint failures=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    printf("  FAIL %s\n",what);
    failures++;
    }
  }


// Check the metrics and the widths of the cached font
static void checkMetrics(FXGlyphs& glyphs){
  const FXGlyph* g1;
  const FXGlyph* g2;
  const FXGlyph* space;
  FXint wabc;
  printf("  font    : %s\n",glyphs.getFontName().text());
  printf("  file    : %s\n",glyphs.getFontFile().text());
  printf("  size    : %d px\n",glyphs.getPixelSize());
  printf("  ascent  : %d\n",glyphs.getAscent());
  printf("  descent : %d\n",glyphs.getDescent());
  printf("  height  : %d\n",glyphs.getHeight());
  printf("  max adv : %d\n",glyphs.getMaxAdvance());
  check(glyphs.getAscent()>0,"ascent is positive");
  check(glyphs.getDescent()>=0,"descent is not negative");
  check(glyphs.getHeight()==glyphs.getAscent()+glyphs.getDescent(),"height is ascent plus descent");
  check(glyphs.getMaxAdvance()>0,"max advance is positive");
  g1=glyphs.getGlyph('A');
  g2=glyphs.getGlyph('A');
  check(g1!=nullptr && g1==g2,"a glyph is cached");
  check(g1!=nullptr && g1->advance>0,"glyph advance is positive");
  check(g1!=nullptr && g1->width>0 && g1->height>0,"glyph has ink");
  space=glyphs.getGlyph(' ');
  check(space!=nullptr && space->width==0 && space->height==0,"space has no ink");
  check(space!=nullptr && space->advance>0,"space has an advance");
  check(glyphs.getTextWidth("",0)==0,"empty string has no width");
  check(glyphs.getTextWidth("abc",3)==glyphs.getCharWidth('a')+glyphs.getCharWidth('b')+glyphs.getCharWidth('c'),"text width is the sum of the advances");
  check(glyphs.getTextWidth("ab")==glyphs.getCharWidth('a')+glyphs.getCharWidth('b'),"default string length is 0 terminated");
  check(glyphs.getTextWidth("\xc3\xa4",2)==glyphs.getCharWidth(0xe4),"two byte UTF-8 counts once");
  wabc=glyphs.getTextWidth("abc",3);
  check(glyphs.getCharAt("abc",3,0)==0,"character at the start");
  check(glyphs.getCharAt("abc",3,wabc-1)==2,"character at the last pixel");
  check(glyphs.getCharAt("abc",3,wabc)==3,"character past the end");
  }

#endif

/*******************************************************************************/

// Entry point
int main(int argc,char** argv){
  fxtestargs(argc,argv);
  printf("glyph cache test\n");

#if !defined(FOX_GLYPHS_AVAILABLE)
  printf("not built with FreeType and fontconfig; skipped\n");
  return 0;
#else
  {
    FXGlyphs glyphs;
    check(glyphs.open(fontfamily,fontsize,FXFont::Normal,FXFont::Straight,FXFont::NonExpanded,fontdpi),"open font");
    if(!glyphs.isOpen()){
      printf("FAILED: no font matched\n");
      return 1;
      }
    checkMetrics(glyphs);
  }

#if defined(FOX_X11)
  {
    static const FXchar* texts[]={
      "Hello, World!",
      "The quick brown fox jumps over the lazy dog",
      "iiiiiiii",
      "MMMMMMMM",
      "0123456789",
      "A quick brown fox, 0123",
      nullptr
      };
    FXApp application("Glyphs","FoxTest");
    FXFont* font;
    FXGlyphs ours;
    FXint res;
    FXint i;
    FXint delta;
    application.init(argc,argv);
    res=application.reg().readUIntEntry("SETTINGS","screenres",100);
    printf("\n  comparing with FXFont (Xft) at %d dpi\n",res);
    printf("  %-12s %6s %6s\n","metric","Xft","glyphs");
    font=new FXFont(&application,fontfamily,fontsize/10,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,0);
    font->create();
    ours.open(fontfamily,fontsize,FXFont::Normal,FXFont::Straight,FXFont::NonExpanded,res);
    check(ours.isOpen(),"open font for comparison");
    delta=font->getFontAscent()-ours.getAscent();
    printf("  %-12s %6d %6d %+d\n","ascent",font->getFontAscent(),ours.getAscent(),delta);
    check(delta>=-1 && delta<=1,"ascent matches");
    delta=font->getFontDescent()-ours.getDescent();
    printf("  %-12s %6d %6d %+d\n","descent",font->getFontDescent(),ours.getDescent(),delta);
    check(delta>=-1 && delta<=1,"descent matches");
    delta=font->getFontHeight()-ours.getHeight();
    printf("  %-12s %6d %6d %+d\n","height",font->getFontHeight(),ours.getHeight(),delta);
    check(delta>=-1 && delta<=1,"height matches");
    delta=font->getFontWidth()-ours.getMaxAdvance();
    printf("  %-12s %6d %6d %+d\n","max advance",font->getFontWidth(),ours.getMaxAdvance(),delta);
    check(delta>=-1 && delta<=1,"max advance matches");
    printf("\n  %-40s %6s %6s %6s\n","text","Xft","glyphs","delta");
    for(i=0; texts[i]; i++){
      FXint xft=font->getTextWidth(texts[i],(FXuint)strlen(texts[i]));
      FXint own=ours.getTextWidth(texts[i],(FXint)strlen(texts[i]));
      printf("  %-40s %6d %6d %+6d\n",texts[i],xft,own,own-xft);
      check(own-xft>=-1 && own-xft<=1,"text width matches");
      }
    delete font;
    }
#endif

  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
#endif
  }
