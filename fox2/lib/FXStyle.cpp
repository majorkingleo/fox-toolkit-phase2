/********************************************************************************
*                                                                               *
*                        S t y l e    O b j e c t s                              *
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


/*
  Notes:
  - The classic style must reproduce the original FXButton and FXFrame drawing
    byte for byte; that is what makes the theme refactoring safe to land.
  - Colors come from the theme, layout stays in the widget.
*/


// Frame style mask, matching FXFrame
#define FRAME_MASK (FRAME_SUNKEN|FRAME_RAISED|FRAME_THICK)


#if !defined(WIN32) && defined(HAVE_XRENDER_H)
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

// Construct style for given theme
FXStyle::FXStyle(FXTheme* thm):theme(thm),xrender(false),xrenderChecked(false),dpy(nullptr),masks(nullptr){
  }


// Drop cached chrome resources after a theme change
void FXStyle::invalidate(){
#if !defined(WIN32) && defined(HAVE_XRENDER_H)
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


// Destructor
FXStyle::~FXStyle(){
  invalidate();
  dpy=nullptr;
  }


// Draw border rectangle
void FXStyle::drawBorderRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(theme->color(ROLE_BORDER));
  dc.drawRectangle(x,y,w-1,h-1);
  }


// Draw raised rectangle
void FXStyle::drawRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    }
  }


// Draw sunken rectangle
void FXStyle::drawSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    }
  }


// Draw double raised rectangle
void FXStyle::drawDoubleRaisedRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_BORDER));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE));
      dc.fillRectangle(x+1,y+1,w-2,1);
      dc.fillRectangle(x+1,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }


// Draw double sunken rectangle
void FXStyle::drawDoubleSunkenRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w-1,1);
    dc.fillRectangle(x,y,1,h-1);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_BORDER));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      dc.setForeground(theme->color(ROLE_SURFACE));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      }
    }
  }


// Draw keyboard focus ring
void FXStyle::drawFocusRing(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  dc.setForeground(theme->color(ROLE_TEXT));
  dc.drawFocusRectangle(x,y,w,h);
  }


// Draw ridge rectangle
void FXStyle::drawRidgeRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }


// Draw groove rectangle
void FXStyle::drawGrooveRectangle(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(0<w && 0<h){
    dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
    dc.fillRectangle(x,y,w,1);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x+w-1,y,1,h);
    if(1<w && 1<h){
      dc.setForeground(theme->color(ROLE_SURFACE_SHADOW));
      dc.fillRectangle(x+1,y+h-2,w-2,1);
      dc.fillRectangle(x+w-2,y+1,1,h-2);
      dc.setForeground(theme->color(ROLE_SURFACE_HILITE));
      dc.fillRectangle(x+1,y+1,w-3,1);
      dc.fillRectangle(x+1,y+1,1,h-3);
      }
    }
  }


// Draw frame chrome for the given frame options
void FXStyle::drawFrame(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint options){
  switch(options&FRAME_MASK){
    case FRAME_LINE: drawBorderRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN: drawSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED: drawRaisedRectangle(dc,x,y,w,h); break;
    case FRAME_GROOVE: drawGrooveRectangle(dc,x,y,w,h); break;
    case FRAME_RIDGE: drawRidgeRectangle(dc,x,y,w,h); break;
    case FRAME_SUNKEN|FRAME_THICK: drawDoubleSunkenRectangle(dc,x,y,w,h); break;
    case FRAME_RAISED|FRAME_THICK: drawDoubleRaisedRectangle(dc,x,y,w,h); break;
    }
  }


#if !defined(WIN32) && defined(HAVE_XRENDER_H)

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
#if !defined(WIN32) && defined(HAVE_XRENDER_H)
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
#if !defined(WIN32) && defined(HAVE_XRENDER_H)
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
#if !defined(WIN32) && defined(HAVE_XRENDER_H)
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


// Draw button chrome; returns rectangle available to icon and label
FXRectangle FXStyle::drawButton(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXButtonState& state){
  FXColor hilite=theme->color(ROLE_SURFACE_HILITE);
  FXColor edgeCol=theme->color(ROLE_BORDER);
  FXColor face=state.engaged?hilite:state.back;
  FXint borderWidth=theme->metric(METRIC_BORDER_WIDTH);
  FXint radius=theme->metric(METRIC_CORNER_RADIUS);
  FXint border=state.border;
  FXuint options=state.options;

  // Rounded chrome, when the theme asks for it
  if(0<radius){

    // Windows are created with a black background pixel, so the area outside
    // the rounded shape must be filled with the parent's color first
    dc.setForeground(state.parentBack);
    dc.fillRectangle(x,y,w,h);

    if(hasXRender(dc) && drawRoundRect(dc,x,y,w,h,radius,face,edgeCol,borderWidth)){
      return FXRectangle(x,y,w,h);
      }
    // Fallback: hard-edged rounded shape, corners stay transparent
    FXint rr=FXMIN(radius,FXMIN(w,h)/2);
    dc.setForeground(face);
    dc.fillRoundRectangle(x,y,w,h,rr*2,rr*2);
    if(0<borderWidth){
      dc.setForeground(edgeCol);
      dc.drawRoundRectangle(x,y,w,h,rr*2,rr*2);
      }
    return FXRectangle(x,y,w,h);
    }

  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(state.toolbar){

      // Enabled, cursor inside, and up
      if(state.enabled && state.hover && !state.down && !state.engaged){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x,y,w,h);
        else drawRaisedRectangle(dc,x,y,w,h);
        }

      // Enabled, cursor inside, and down
      else if(state.enabled && state.hover && state.down){
        dc.setForeground(state.back);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
        else drawSunkenRectangle(dc,x,y,w,h);
        }

      // Enabled and checked
      else if(state.enabled && state.engaged){
        dc.setForeground(hilite);
        dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
        else drawSunkenRectangle(dc,x,y,w,h);
        }

      // Disabled or unchecked or not under cursor
      else{
        dc.setForeground(state.back);
        dc.fillRectangle(x,y,w,h);
        }
      }

    // Normal style
    else{

      // Default
      if(state.defaulted){

        // Draw in up state if disabled or up
        if(!state.enabled || (!state.down && !state.engaged)){
          dc.setForeground(state.back);
          dc.fillRectangle(x+border+1,y+border+1,w-border*2-1,h-border*2-1);
          if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x+1,y+1,w-1,h-1);
          else drawRaisedRectangle(dc,x+1,y+1,w-1,h-1);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2-1,h-border*2-1);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w-1,h-1);
          else drawSunkenRectangle(dc,x,y,w-1,h-1);
          }

        // Black default border
        drawBorderRectangle(dc,x,y,w,h);
        }

      // Non-default
      else{

        // Draw in up state if disabled or up
        if(!state.enabled || (!state.down && !state.engaged)){
          dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,x,y,w,h);
          else drawRaisedRectangle(dc,x,y,w,h);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state.engaged) dc.setForeground(hilite); else dc.setForeground(state.back);
          dc.fillRectangle(x+border,y+border,w-border*2,h-border*2);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,x,y,w,h);
          else drawSunkenRectangle(dc,x,y,w,h);
          }
        }
      }
    }

  // No borders
  else{
    if(state.enabled && state.engaged){
      dc.setForeground(hilite);
      dc.fillRectangle(x,y,w,h);
      }
    else{
      dc.setForeground(state.back);
      dc.fillRectangle(x,y,w,h);
      }
    }
  return FXRectangle(x,y,w,h);
  }

}
