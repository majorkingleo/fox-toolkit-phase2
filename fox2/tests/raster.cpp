/********************************************************************************
*                                                                               *
*                        S o f t w a r e   R a s t e r   T e s t              *
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
#include "../lib/fxraster.h"
#include <stdio.h>
#include <string.h>

/*
  Notes:

  - Phase 1 gate of docs/wayland-plan.md: draw one fixed scene through FXRaster
    and, where the platform can draw, through FXDCWindow, then compare the two
    buffers cell by cell and report the differences.
  - Each cell of the scene holds one primitive; the scene is written as PPM so
    the output can be looked at, and the per-cell table tells which primitives
    still deviate.
  - --raster-only runs headless on any host: no FXApp, no display, no X11.
  - Exit code 0 when every exact cell matches and no tolerant cell exceeds its
    limit; 1 on pixel differences; 2 on usage or I/O errors.
*/

// Geometry of the test scene
const FXint CELLW=64;
const FXint CELLH=64;
const FXint COLS=5;
const FXint ROWS=6;
const FXint SCENEW=CELLW*COLS;
const FXint SCENEH=CELLH*ROWS;
const FXint NCELLS=COLS*ROWS;


// One cell of the scene, and how strictly it must match
struct FXCellDesc {
  const FXchar* name;           // Cell name
  FXbool        exact;          // Must match X11 pixel for pixel
  FXint         tolerance;      // Allowed differing pixels otherwise
  };


// The cells; a tolerant cell allows for the documented differences of the
// analytic rasterization of arcs, wide lines and polygons.  The limits are the
// measured difference against the X11 build plus a small margin, so that a
// regression is caught while the known deviations are tolerated.
static const FXCellDesc celldesc[NCELLS]={
  {"fill",       true,  0},
  {"tiled",      true,  0},
  {"stipple",    true,  0},
  {"opaque",     true,  0},
  {"points",     true,  0},
  {"line-hv",    true,  0},
  {"line-diag",  false, 20},
  {"lines",      false, 40},
  {"dash",       false, 40},
  {"width",      false, 60},
  {"rect",       true,  0},
  {"roundrect",  false, 70},
  {"fillround",  false, 30},
  {"arc",        false, 110},
  {"fillarc",    false, 30},
  {"chord",      false, 70},
  {"ellipse",    false, 170},
  {"fillellipse",false, 70},
  {"polygon",    false, 50},
  {"concave",    false, 50},
  {"complex",    false, 80},
  {"winding",    false, 80},
  {"gradv",      true,  0},
  {"gradh",      true,  0},
  {"hashbox",    true,  0},
  {"focus",      false, 10},
  {"image",      true,  0},
  {"stretch",    true,  0},
  {"area",       true,  0},
  {"clip",       true,  0}
  };


// Images used by the scene
struct FXAssets {
  FXImage*  tile;               // 8x8 tile
  FXImage*  image;              // 32x32 source image
  };


// Image without an application, holding client-side pixels only; FXImage's
// own default constructor is protected
class FXBareImage : public FXImage {
public:
  FXBareImage(){}
  };


/*******************************************************************************/

// Fill a pixel buffer with a pattern
static FXColor* makePixels(FXint w,FXint h,FXColor (*pattern)(FXint,FXint)){
  FXColor* pix;
  FXint x,y;
  if(!allocElms(pix,w*h)) return nullptr;
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      pix[y*w+x]=pattern(x,y);
      }
    }
  return pix;
  }


// An 8x8 checker tile
static FXColor tilePattern(FXint x,FXint y){
  return ((x^y)&8) ? FXRGB(0,120,200) : FXRGB(250,250,180);
  }


// A 32x32 image with a red border and a blue-green gradient inside
static FXColor imagePattern(FXint x,FXint y){
  if(x<2 || y<2 || x>=30 || y>=30) return FXRGB(200,40,40);
  return FXRGB(x*8,y*8,120);
  }


// Build the assets; an application, if given, gets server-side copies
static FXbool makeAssets(FXAssets& a,FXApp* app){
  FXColor* tilepix=makePixels(8,8,tilePattern);
  FXColor* imagepix=makePixels(32,32,imagePattern);
  if(!tilepix || !imagepix){
    freeElms(tilepix);
    freeElms(imagepix);
    return false;
    }
  a.tile=app?new FXImage(app):new FXBareImage();
  a.image=app?new FXImage(app):new FXBareImage();
  a.tile->setData(tilepix,IMAGE_OWNED|IMAGE_KEEP,8,8);
  a.image->setData(imagepix,IMAGE_OWNED|IMAGE_KEEP,32,32);
  if(app){
    a.tile->create();
    a.tile->render();
    a.image->create();
    a.image->render();
    }
  return true;
  }


// Release the assets
static void freeAssets(FXAssets& a){
  delete a.image;  a.image=nullptr;
  delete a.tile;   a.tile=nullptr;
  }


/*******************************************************************************/

// Reset the drawing state at the start of a cell; both DC's must agree
template<class DC>
static void resetCell(DC& dc,FXint x,FXint y){
  dc.setFunction(BLT_SRC);
  dc.setLineWidth(0);
  dc.setLineCap(CAP_BUTT);
  dc.setLineJoin(JOIN_MITER);
  dc.setLineStyle(LINE_SOLID);
  dc.setFillStyle(FILL_SOLID);
  dc.setFillRule(RULE_EVEN_ODD);
  dc.setForeground(FXRGB(0,0,0));
  dc.setBackground(FXRGB(255,255,255));
  dc.clearClipRectangle();
  dc.setClipRectangle(x,y,CELLW,CELLH);
  }


// Draw one primitive into cell x,y of the scene
template<class DC>
static void drawCell(DC& dc,FXint i,FXint x,FXint y,const FXAssets& a){
  FXPoint points[6];
  FXRectangle rectangles[2];
  FXuchar dashpat[2];
  FXRegion region;
  switch(i){

    // Solid fill
    case 0:
      dc.setForeground(FXRGB(200,60,60));
      dc.fillRectangle(x+6,y+6,CELLW-12,CELLH-12);
      break;

    // Tiled fill
    case 1:
      dc.setTile(a.tile,x,y);
      dc.setFillStyle(FILL_TILED);
      dc.fillRectangle(x+6,y+6,CELLW-12,CELLH-12);
      dc.setFillStyle(FILL_SOLID);
      break;

    // Stippled fill
    case 2:
      dc.setStipple(STIPPLE_GRAY);
      dc.setFillStyle(FILL_STIPPLED);
      dc.setForeground(FXRGB(0,0,0));
      dc.fillRectangle(x+6,y+6,CELLW-12,CELLH-12);
      break;

    // Opaque stippled fill
    case 3:
      dc.setStipple(STIPPLE_CROSS);
      dc.setForeground(FXRGB(20,120,40));
      dc.setBackground(FXRGB(240,240,250));
      dc.setFillStyle(FILL_OPAQUESTIPPLED);
      dc.fillRectangle(x+6,y+6,CELLW-12,CELLH-12);
      break;

    // Points, absolute and relative
    case 4:
      points[0].x=x+8;   points[0].y=y+8;
      points[1].x=x+20;  points[1].y=y+30;
      points[2].x=x+34;  points[2].y=y+12;
      points[3].x=x+48;  points[3].y=y+40;
      points[4].x=x+56;  points[4].y=y+20;
      dc.drawPoints(points,5);
      break;

    // Horizontal and vertical lines
    case 5:
      dc.drawLine(x+6,y+10,x+CELLW-7,y+10);
      dc.drawLine(x+10,y+6,x+10,y+CELLH-7);
      dc.drawLine(x+CELLW-7,y+16,x+CELLW-7,y+CELLH-7);
      dc.drawLine(x+20,y+CELLH-7,x+CELLW-7,y+CELLH-7);
      break;

    // Diagonal lines of different slopes
    case 6:
      dc.drawLine(x+6,y+6,x+CELLW-7,y+CELLH-7);
      dc.drawLine(x+6,y+CELLH-7,x+CELLW-7,y+6);
      dc.drawLine(x+6,y+20,x+CELLW-7,y+28);
      dc.drawLine(x+30,y+6,x+34,y+CELLH-7);
      break;

    // Polyline
    case 7:
      points[0].x=x+6;   points[0].y=y+50;
      points[1].x=x+20;  points[1].y=y+10;
      points[2].x=x+34;  points[2].y=y+50;
      points[3].x=x+48;  points[3].y=y+10;
      points[4].x=x+58;  points[4].y=y+50;
      dc.drawLines(points,5);
      break;

    // Dashed lines
    case 8:
      dashpat[0]=6;
      dashpat[1]=4;
      dc.setDashes(0,dashpat,2);
      dc.setLineStyle(LINE_ONOFF_DASH);
      dc.drawLine(x+6,y+14,x+CELLW-7,y+14);
      dc.drawLine(x+6,y+30,x+CELLW-7,y+30);
      dc.setForeground(FXRGB(30,30,200));
      dc.setBackground(FXRGB(240,220,120));
      dc.setLineStyle(LINE_DOUBLE_DASH);
      dc.drawLine(x+6,y+48,x+CELLW-7,y+48);
      break;

    // Line width and caps
    case 9:
      dc.setLineWidth(3);
      dc.drawLine(x+8,y+12,x+56,y+22);
      dc.setLineWidth(5);
      dc.setLineCap(CAP_ROUND);
      dc.drawLine(x+10,y+46,x+54,y+46);
      break;

    // Rectangle outline
    case 10:
      dc.drawRectangle(x+6,y+6,CELLW-12,CELLH-12);
      rectangles[0].x=x+14;  rectangles[0].y=y+14;
      rectangles[0].w=16;    rectangles[0].h=16;
      rectangles[1].x=x+34;  rectangles[1].y=y+34;
      rectangles[1].w=20;    rectangles[1].h=14;
      dc.drawRectangles(rectangles,2);
      break;

    // Rounded rectangle outline
    case 11:
      dc.drawRoundRectangle(x+6,y+14,CELLW-12,CELLH-28,10,8);
      break;

    // Filled rounded rectangle
    case 12:
      dc.setForeground(FXRGB(60,140,200));
      dc.fillRoundRectangle(x+6,y+14,CELLW-12,CELLH-28,10,8);
      break;

    // Arcs
    case 13:
      dc.drawArc(x+6,y+6,CELLW-12,CELLH-12,0,5760);
      dc.drawArc(x+6,y+6,CELLW-12,CELLH-12,5760,11520);
      break;

    // Filled arcs
    case 14:
      dc.setForeground(FXRGB(180,60,140));
      dc.fillArc(x+6,y+6,CELLW-12,CELLH-12,0,5760);
      dc.setForeground(FXRGB(240,200,60));
      dc.fillArc(x+6,y+6,CELLW-12,CELLH-12,11520,5760);
      break;

    // Filled chord
    case 15:
      dc.setForeground(FXRGB(60,180,140));
      dc.fillChord(x+6,y+6,CELLW-12,CELLH-12,2880,11520);
      break;

    // Ellipse outline
    case 16:
      dc.drawEllipse(x+6,y+12,CELLW-12,CELLH-24);
      break;

    // Filled ellipse
    case 17:
      dc.setForeground(FXRGB(120,120,220));
      dc.fillEllipse(x+6,y+12,CELLW-12,CELLH-24);
      break;

    // Convex polygon
    case 18:
      points[0].x=x+8;   points[0].y=y+52;
      points[1].x=x+32;  points[1].y=y+8;
      points[2].x=x+56;  points[2].y=y+52;
      dc.setForeground(FXRGB(90,160,60));
      dc.fillPolygon(points,3);
      break;

    // Concave polygon
    case 19:
      points[0].x=x+8;   points[0].y=y+10;
      points[1].x=x+54;  points[1].y=y+10;
      points[2].x=x+34;  points[2].y=y+30;
      points[3].x=x+54;  points[3].y=y+52;
      points[4].x=x+8;   points[4].y=y+52;
      points[5].x=x+26;  points[5].y=y+30;
      dc.setForeground(FXRGB(200,110,40));
      dc.fillConcavePolygon(points,6);
      break;

    // Complex polygon, even odd rule
    case 20:
      points[0].x=x+32;  points[0].y=y+6;
      points[1].x=x+40;  points[1].y=y+24;
      points[2].x=x+58;  points[2].y=y+24;
      points[3].x=x+44;  points[3].y=y+36;
      points[4].x=x+50;  points[4].y=y+54;
      points[5].x=x+32;  points[5].y=y+42;
      points[6].x=x+14;  points[6].y=y+54;
      points[7].x=x+20;  points[7].y=y+36;
      points[8].x=x+6;   points[8].y=y+24;
      points[9].x=x+24;  points[9].y=y+24;
      dc.setForeground(FXRGB(150,60,200));
      dc.fillComplexPolygon(points,10);
      break;

    // The same star with the winding rule
    case 21:
      points[0].x=x+32;  points[0].y=y+6;
      points[1].x=x+40;  points[1].y=y+24;
      points[2].x=x+58;  points[2].y=y+24;
      points[3].x=x+44;  points[3].y=y+36;
      points[4].x=x+50;  points[4].y=y+54;
      points[5].x=x+32;  points[5].y=y+42;
      points[6].x=x+14;  points[6].y=y+54;
      points[7].x=x+20;  points[7].y=y+36;
      points[8].x=x+6;   points[8].y=y+24;
      points[9].x=x+24;  points[9].y=y+24;
      dc.setFillRule(RULE_WINDING);
      dc.setForeground(FXRGB(60,60,160));
      dc.fillComplexPolygon(points,10);
      break;

    // Vertical gradient
    case 22:
      dc.fillVerticalGradient(x+6,y+6,CELLW-12,CELLH-12,FXRGB(250,250,120),FXRGB(40,60,160));
      break;

    // Horizontal gradient
    case 23:
      dc.fillHorizontalGradient(x+6,y+6,CELLW-12,CELLH-12,FXRGB(250,120,120),FXRGB(60,200,120));
      break;

    // Hash box
    case 24:
      dc.drawHashBox(x+8,y+8,CELLW-16,CELLH-16,2);
      break;

    // Focus rectangle over a dark background
    case 25:
      dc.setForeground(FXRGB(40,60,90));
      dc.fillRectangle(x+8,y+14,CELLW-16,CELLH-28);
      dc.drawFocusRectangle(x+8,y+14,CELLW-16,CELLH-28);
      break;

    // Copy an image
    case 26:
      dc.drawImage(a.image,x+16,y+16);
      break;

    // Stretched copy of an image
    case 27:
      dc.drawArea(a.image,0,0,32,32,x+6,y+8,CELLW-12,CELLH-16);
      break;

    // Copy part of an image
    case 28:
      dc.drawArea(a.image,8,8,24,24,x+20,y+20);
      break;

    // Fill clipped to a region
    case 29:
      region=FXRegion(x+10,y+10,CELLW-20,CELLH-20);
      region-=FXRegion(x+22,y+22,20,20);
      dc.setClipRegion(region);
      dc.setForeground(FXRGB(80,200,220));
      dc.fillRectangle(x,y,CELLW,CELLH);
      dc.clearClipRectangle();
      break;
    }
  }


// Draw the whole scene
template<class DC>
static void drawScene(DC& dc,const FXAssets& a){
  FXint i;
  dc.setForeground(FXRGB(240,240,240));
  dc.fillRectangle(0,0,SCENEW,SCENEH);
  for(i=0; i<NCELLS; i++){
    FXint x=(i%COLS)*CELLW;
    FXint y=(i/COLS)*CELLH;
    resetCell(dc,x,y);
    drawCell(dc,i,x,y,a);
    }
  }


/*******************************************************************************/

// Write a PPM file; the alpha channel is ignored
static FXbool writePPM(const FXchar* file,const FXColor* pixels,FXint w,FXint h){
  FILE* fp;
  FXint x,y;
  if(!(fp=fopen(file,"wb"))) return false;
  fprintf(fp,"P6\n%d %d\n255\n",w,h);
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      FXColor c=pixels[y*w+x];
      fputc(FXREDVAL(c),fp);
      fputc(FXGREENVAL(c),fp);
      fputc(FXBLUEVAL(c),fp);
      }
    }
  fclose(fp);
  return true;
  }


// Compare two buffers cell by cell; differences are written to diff.ppm
static FXint compare(const FXchar* dir,const FXColor* raster,const FXColor* x11){
  FXColor* diff;
  FXint i,x,y,failures=0,total=0;
  if(!allocElms(diff,SCENEW*SCENEH)) return 0;
  clearElms(diff,SCENEW*SCENEH);
  printf("\n%-14s %8s %8s %8s\n","cell","diff","pixels","allowed");
  printf("%-14s %8s %8s %8s\n","--------------","--------","--------","--------");
  for(i=0; i<NCELLS; i++){
    FXint x0=(i%COLS)*CELLW;
    FXint y0=(i/COLS)*CELLH;
    FXint count=0;
    for(y=y0; y<y0+CELLH; y++){
      for(x=x0; x<x0+CELLW; x++){
        FXColor a=raster[y*SCENEW+x]&0xffffff;
        FXColor b=x11[y*SCENEW+x]&0xffffff;
        if(a!=b){
          count++;
          diff[y*SCENEW+x]=FXRGB(255,0,255);
          }
        else{
          diff[y*SCENEW+x]=FXRGB(a>>17&0x7f,a>>9&0x7f,a>>1&0x7f);
          }
        }
      }
    total+=count;
    if(celldesc[i].exact ? (count!=0) : (count>celldesc[i].tolerance)){ failures++; }
    printf("%-14s %8d %8d %8d%s\n",celldesc[i].name,count,CELLW*CELLH,celldesc[i].exact?0:celldesc[i].tolerance,
           (celldesc[i].exact ? (count!=0) : (count>celldesc[i].tolerance)) ? "  FAIL" : "");
    }
  printf("%-14s %8d %8d\n","total",total,SCENEW*SCENEH);
  if(dir){
    FXString name=FXString(dir)+"/diff.ppm";
    writePPM(name.text(),diff,SCENEW,SCENEH);
    }
  freeElms(diff);
  return failures;
  }


// Print the usage
static void usage(){
  printf("usage: raster [--raster-only] [--out=DIR] [--no-dialogs]\n");
  printf("  --raster-only  draw with FXRaster only, no display needed\n");
  printf("  --out=DIR      write raster.ppm, x11.ppm and diff.ppm into DIR\n");
  }


/*******************************************************************************/

// Entry point
int main(int argc,char** argv){
  FXString outdir="tmp/raster";
  FXbool rasteronly=false;
  FXbool hasout=false;
  FXColor* rasterpix;
  FXColor* x11pix=nullptr;
  FXAssets assets;
  FXint failures=-1;
  FXint i;

  fxtestargs(argc,argv);

  // Options
  for(i=1; i<argc; i++){
    if(strcmp(argv[i],"--raster-only")==0){
      rasteronly=true;
      }
    else if(strncmp(argv[i],"--out=",6)==0){
      outdir=argv[i]+6;
      hasout=true;
      }
    else{
      fprintf(stderr,"raster: unknown option '%s'\n",argv[i]);
      usage();
      return 2;
      }
    }

  // Draw the scene into a plain buffer
  if(!allocElms(rasterpix,SCENEW*SCENEH)){
    fprintf(stderr,"raster: out of memory\n");
    return 2;
    }
  if(!makeAssets(assets,nullptr)){
    fprintf(stderr,"raster: out of memory\n");
    freeElms(rasterpix);
    return 2;
    }
  {
    FXRaster raster(rasterpix,SCENEW,SCENEH);
    drawScene(raster,assets);
  }
  freeAssets(assets);

  if(hasout){
    FXString name=outdir+"/raster.ppm";
    if(!writePPM(name.text(),rasterpix,SCENEW,SCENEH)){
      fprintf(stderr,"raster: cannot write '%s'\n",name.text());
      freeElms(rasterpix);
      return 2;
      }
    printf("wrote %s (%dx%d)\n",name.text(),SCENEW,SCENEH);
    }

#if defined(FOX_X11)
  if(!rasteronly){
    FXApp application("Raster","FoxTest");
    application.init(argc,argv);
    FXImage image(&application);
    image.setData(nullptr,0,SCENEW,SCENEH);
    image.create();
    image.render();
    if(!makeAssets(assets,&application)){
      fprintf(stderr,"raster: out of memory\n");
      image.destroy();
      freeElms(rasterpix);
      return 2;
      }
    {
      FXDCWindow dc(&image);
      drawScene(dc,assets);
      dc.end();
    }
    image.restore();
    x11pix=image.getData();
    if(hasout){
      FXString name=outdir+"/x11.ppm";
      if(!writePPM(name.text(),x11pix,SCENEW,SCENEH)){
        fprintf(stderr,"raster: cannot write '%s'\n",name.text());
        freeAssets(assets);
        image.destroy();
        freeElms(rasterpix);
        return 2;
        }
      printf("wrote %s (%dx%d)\n",name.text(),SCENEW,SCENEH);
      }
    failures=compare(hasout?outdir.text():nullptr,rasterpix,x11pix);
    freeAssets(assets);
    image.destroy();
    }
  else {
    printf("raster-only: X11 comparison skipped\n");
    }
#else
  printf("raster-only: no drawable platform in this build\n");
#endif

  freeElms(rasterpix);

  if(failures<0) return 0;
  printf("\n%d cell(s) outside their limit\n",failures);
  return failures?1:0;
  }
