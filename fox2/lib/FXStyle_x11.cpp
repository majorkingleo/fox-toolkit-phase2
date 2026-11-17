/********************************************************************************
*                                                                               *
*                               Style   ( X 1 1 )                               *
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
#include "fxmath.h"
#include "fxkeys.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXVisual.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXTheme.h"
#include "FXFrame.h"
#include "FXStyle.h"

#if defined(FOX_X11)

/*
  Notes:
  - Platform part of the style: the antialiased rounded rectangle cache.
*/

#if defined(HAVE_XRENDER_H)
// Alpha mask for the four corners of a given radius
struct FXStyle::FXCornerMask {
  FXint    radius;              // Corner radius
  Pixmap   pixmap[4];           // Coverage bitmaps per corner
  Picture  picture[4];          // Render pictures per corner
  FXCornerMask *next;           // Next mask in cache
  };


// Alpha mask for a check mark of a given size
struct FXStyle::FXCheckMask {
  FXint      size;              // Indicator size
  Pixmap     pixmap;            // Coverage bitmap
  Picture    picture;           // Render picture
  FXCheckMask *next;            // Next mask in cache
  };
#else
// Placeholder when antialiased compositing is unavailable
struct FXStyle::FXCornerMask {
  FXint         radius;
  FXCornerMask *next;
  };
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {

// Drop cached chrome resources after a theme change
void FXStyle::invalidate(){
#if defined(HAVE_XRENDER_H)
  Display *display=(Display*)dpy;
  FXCornerMask *mask=masks;
  FXint c;
  while(mask){
    FXCornerMask *next=mask->next;
    for(c=0; c<4; c++){
      if(display && mask->picture[c]) XRenderFreePicture(display,mask->picture[c]);
      if(display && mask->pixmap[c]) XFreePixmap(display,mask->pixmap[c]);
      }
    delete mask;
    mask=next;
    }
#endif
  masks=nullptr;
#if defined(HAVE_XRENDER_H)
  FXCheckMask *check=checkmasks;
  while(check){
    FXCheckMask *next=check->next;
    if(display && check->picture) XRenderFreePicture(display,check->picture);
    if(display && check->pixmap) XFreePixmap(display,check->pixmap);
    delete check;
    check=next;
    }
#endif
  checkmasks=nullptr;

  // The pictures belonged to the display that was open when they were made, and
  // a closed display must not be asked to free anything; probe again next time
  dpy=nullptr;
  xrender=false;
  xrenderChecked=false;
  }


#if defined(HAVE_XRENDER_H)

// Build antialiased coverage for one corner of the given radius
static void buildCoverage(FXuchar* cov,FXint radius,FXuint corner){
  FXint i,j,x,y,inside;
  FXdouble px,py,dx,dy;
  FXdouble rr=(FXdouble)radius*radius;
  for(j=0; j<radius; j++){
    for(i=0; i<radius; i++){
      inside=0;
      for(y=0; y<4; y++){
        for(x=0; x<4; x++){
          px=i+(x+0.5)/4.0;
          py=j+(y+0.5)/4.0;
          dx=(corner&1)?px:(radius-px);
          dy=(corner&2)?py:(radius-py);
          if(dx*dx+dy*dy<=rr) inside++;
          }
        }
      cov[j*radius+i]=(FXuchar)(inside*255/16);
      }
    }
  }


// Composite an opaque color through a mask picture
static void compositeSolid(Display* display,Picture dst,Picture mask,FXColor clr,FXint dx,FXint dy,FXint radius){
  XRenderColor col;
  Picture src;
  col.red=FXREDVAL(clr)*257;
  col.green=FXGREENVAL(clr)*257;
  col.blue=FXBLUEVAL(clr)*257;
  col.alpha=0xffff;
  src=XRenderCreateSolidFill(display,&col);
  if(src){
    XRenderComposite(display,PictOpOver,src,mask,dst,0,0,0,0,dx,dy,radius,radius);
    XRenderFreePicture(display,src);
    }
  }

#endif


// Probe whether the drawable supports antialiased compositing
FXbool FXStyle::hasXRender(FXDCWindow& dc){
  if(!xrenderChecked){
    xrenderChecked=true;
#if defined(HAVE_XRENDER_H)
    Display *display=(Display*)dc.getApp()->getDisplay();
    FXDrawable *drawable=dc.drawable();
    if(display && drawable && drawable->getVisual() && 24<=drawable->getVisual()->getDepth()){
      int event,error;
      if(XRenderQueryExtension(display,&event,&error)){
        xrender=true;
        dpy=display;
        }
      }
#endif
    }
  return xrender;
  }


// Return cached corner mask for the given radius
FXStyle::FXCornerMask* FXStyle::getMask(FXint radius){
#if defined(HAVE_XRENDER_H)
  Display *display=(Display*)dpy;
  XRenderPictFormat *a8;
  FXCornerMask *mask;
  FXint c;
  for(mask=masks; mask; mask=mask->next){
    if(mask->radius==radius) return mask;
    }
  mask=new FXCornerMask;
  mask->radius=radius;
  mask->next=masks;
  masks=mask;
  a8=XRenderFindStandardFormat(display,PictStandardA8);
  for(c=0; c<4; c++){
    mask->pixmap[c]=0;
    mask->picture[c]=0;
    if(a8){
      FXuchar *cov=new FXuchar[radius*radius];
      buildCoverage(cov,radius,(FXuint)c);
      mask->pixmap[c]=XCreatePixmap(display,DefaultRootWindow(display),radius,radius,8);
      XImage *img=XCreateImage(display,DefaultVisual(display,DefaultScreen(display)),8,ZPixmap,0,(char*)cov,radius,radius,8,0);
      if(img){
        GC gc=XCreateGC(display,mask->pixmap[c],0,nullptr);
        XPutImage(display,mask->pixmap[c],gc,img,0,0,0,0,radius,radius);
        XFreeGC(display,gc);
        img->data=nullptr;
        XDestroyImage(img);
        }
      delete[] cov;
      mask->picture[c]=XRenderCreatePicture(display,mask->pixmap[c],a8,0,nullptr);
      }
    }
  return mask;
#else
  return nullptr;
#endif
  }


#if defined(HAVE_XRENDER_H)
// Distance from a point to a line segment
static FXdouble segmentDistance(FXdouble px,FXdouble py,FXdouble ax,FXdouble ay,FXdouble bx,FXdouble by){
  FXdouble dx=bx-ax;
  FXdouble dy=by-ay;
  FXdouble t=((px-ax)*dx+(py-ay)*dy)/(dx*dx+dy*dy);
  FXdouble qx,qy;
  if(t<0.0) t=0.0;
  else if(1.0<t) t=1.0;
  qx=ax+t*dx;
  qy=ay+t*dy;
  return sqrt((px-qx)*(px-qx)+(py-qy)*(py-qy));
  }


// Build antialiased coverage for a check mark of the given size
static void buildCheckCoverage(FXuchar* cov,FXint size){
  FXint i,j,x,y,inside;
  FXdouble ax,ay,bx,by,cx,cy,width;
  const FXint samples=4;
  ax=size*0.22;
  ay=size*0.50;
  bx=size*0.43;
  by=size*0.72;
  cx=size*0.78;
  cy=size*0.28;
  width=FXMAX(1.0,size/14.0);
  for(j=0; j<size; j++){
    for(i=0; i<size; i++){
      inside=0;
      for(y=0; y<samples; y++){
        for(x=0; x<samples; x++){
          FXdouble px=i+(x+0.5)/samples;
          FXdouble py=j+(y+0.5)/samples;
          if(segmentDistance(px,py,ax,ay,bx,by)<=width ||
             segmentDistance(px,py,bx,by,cx,cy)<=width) inside++;
          }
        }
      cov[j*size+i]=(FXuchar)((inside*255+samples*samples/2)/(samples*samples));
      }
    }
  }
#endif


// Return cached check-mark mask for the given size
FXStyle::FXCheckMask* FXStyle::getCheckMask(FXint size){
#if defined(HAVE_XRENDER_H)
  Display *display=(Display*)dpy;
  XRenderPictFormat *a8;
  FXCheckMask *mask;
  if(!display) return nullptr;
  for(mask=checkmasks; mask; mask=mask->next){
    if(mask->size==size) return mask;
    }
  mask=new FXCheckMask;
  mask->size=size;
  mask->pixmap=0;
  mask->picture=0;
  mask->next=checkmasks;
  checkmasks=mask;
  a8=XRenderFindStandardFormat(display,PictStandardA8);
  if(a8){
    FXuchar *cov=new FXuchar[size*size];
    buildCheckCoverage(cov,size);
    mask->pixmap=XCreatePixmap(display,DefaultRootWindow(display),size,size,8);
    XImage *img=XCreateImage(display,DefaultVisual(display,DefaultScreen(display)),8,ZPixmap,0,(char*)cov,size,size,8,0);
    if(img){
      GC gc=XCreateGC(display,mask->pixmap,0,nullptr);
      XPutImage(display,mask->pixmap,gc,img,0,0,0,0,size,size);
      XFreeGC(display,gc);
      img->data=nullptr;
      XDestroyImage(img);
      }
    delete[] cov;
    mask->picture=XRenderCreatePicture(display,mask->pixmap,a8,0,nullptr);
    }
  return mask;
#else
  return nullptr;
#endif
  }


// Draw an antialiased check mark through the XRender mask cache, or use lines
void FXStyle::drawCheckMark(FXDCWindow& dc,FXint x,FXint y,FXint size,FXColor color){
#if defined(HAVE_XRENDER_H)
  FXDrawable *drawable=dc.drawable();
  FXVisual *visual=drawable?drawable->getVisual():nullptr;
  Display *display;
  XRenderPictFormat *vf;
  FXCheckMask *mask;
  Picture dst;
  if(0<size && hasXRender(dc) && visual){
    display=(Display*)dpy;
    vf=XRenderFindVisualFormat(display,(Visual*)visual->getVisual());
    mask=getCheckMask(size);
    if(vf && mask && mask->picture){
      dst=XRenderCreatePicture(display,(Drawable)drawable->id(),vf,0,nullptr);
      if(dst){
        compositeSolid(display,dst,mask->picture,color,x,y,size);
        XRenderFreePicture(display,dst);
        return;
        }
      }
    }
#endif
  dc.setForeground(color);
  dc.setLineWidth(FXMAX(1,size/7));
  dc.drawLine(x+(size*22)/100,y+(size*50)/100,x+(size*43)/100,y+(size*72)/100);
  dc.drawLine(x+(size*43)/100,y+(size*72)/100,x+(size*78)/100,y+(size*28)/100);
  dc.setLineWidth(1);
  }


// Draw an antialiased rounded rectangle
// Margin a popup needs for its drop shadow.  Only a surface with an alpha
// channel can carry a translucent shadow, which the X11 window does not have,
// so no margin is reserved and the popups keep their size
// Fill the outside of the rounded corners; these surfaces have no antialiased
// compositing for a corner shape, so the cut is hard edged
void FXStyle::cutRoundedCorners(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor color){
  fillOutsideCorners(dc,x,y,w,h,radius,color);
  }


FXint FXStyle::shadowMargin(FXdouble){
  return 0;
  }


// Draw the drop shadow of a popup; nothing to draw without an alpha surface
void FXStyle::drawShadow(FXDCWindow&,FXint,FXint,FXint,FXint,FXint){
  }


FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
#if defined(HAVE_XRENDER_H)
  if(!hasXRender(dc)) return false;
  Display *display=(Display*)dpy;
  FXDrawable *drawable=dc.drawable();
  FXVisual *visual;
  XRenderPictFormat *vf;
  FXCornerMask *mask;
  Picture dst;
  FXint r,br,ir;
  static const FXint cx[4]={0,1,0,1};
  static const FXint cy[4]={0,0,1,1};
  FXint c;

  if(radius<=0 || display==nullptr || drawable==nullptr) return false;
  visual=drawable->getVisual();
  if(visual==nullptr) return false;
  vf=XRenderFindVisualFormat(display,(Visual*)visual->getVisual());
  if(vf==nullptr) return false;
  dst=XRenderCreatePicture(display,(Drawable)drawable->id(),vf,0,nullptr);
  if(!dst) return false;

  r=FXMIN(radius,FXMIN(w,h)/2);
  br=FXMIN(FXMAX(borderWidth,0),r);
  ir=FXMAX(r-br,0);

  // Outer rounded rectangle: border, or face when there is no border
  FXColor outer=(0<br)?border:face;
  dc.setForeground(outer);
  dc.fillRectangle(x+r,y,FXMAX(w-2*r,0),h);
  dc.fillRectangle(x,y+r,w,FXMAX(h-2*r,0));
  mask=getMask(r);
  if(mask){
    for(c=0; c<4; c++){
      compositeSolid(display,dst,mask->picture[c],outer,x+cx[c]*(w-r),y+cy[c]*(h-r),r);
      }
    }

  // Inner rounded rectangle in the face color; with no border the outer
  // shape is already the face
  if(0<br){
    if(2*br<w && 2*br<h){
      dc.setForeground(face);
      dc.fillRectangle(x+br+ir,y+br,FXMAX(w-2*br-2*ir,0),h-2*br);
      dc.fillRectangle(x+br,y+br+ir,w-2*br,FXMAX(h-2*br-2*ir,0));
      if(0<ir){
        mask=getMask(ir);
        if(mask){
          for(c=0; c<4; c++){
            compositeSolid(display,dst,mask->picture[c],face,x+br+cx[c]*(w-2*br-ir),y+br+cy[c]*(h-2*br-ir),ir);
            }
          }
        }
      }
    else{
      dc.setForeground(face);
      dc.fillRectangle(x+br,y+br,FXMAX(w-2*br,0),FXMAX(h-2*br,0));
      }
    }

  XRenderFreePicture(display,dst);
  return true;
#else
  return false;
#endif
  }

}

#endif
