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

#if !defined(WIN32)

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


// Draw an antialiased rounded rectangle
FXbool FXStyle::drawRoundRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint radius,FXColor face,FXColor border,FXint borderWidth){
#if defined(HAVE_XRENDER_H)
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

  // Outer rounded rectangle in the border color
  dc.setForeground(border);
  dc.fillRectangle(x+r,y,FXMAX(w-2*r,0),h);
  dc.fillRectangle(x,y+r,w,FXMAX(h-2*r,0));
  mask=getMask(r);
  if(mask){
    for(c=0; c<4; c++){
      compositeSolid(display,dst,mask->picture[c],border,x+cx[c]*(w-r),y+cy[c]*(h-r),r);
      }
    }

  // Inner rounded rectangle in the face color
  if(0<br && 2*br<w && 2*br<h){
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

  XRenderFreePicture(display,dst);
  return true;
#else
  return false;
#endif
  }

}

#endif
