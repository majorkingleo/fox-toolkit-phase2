/********************************************************************************
*                                                                               *
*                             I m a g e    O b j e c t                          *
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
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXElement.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXImage.h"


/*
  Notes:
  - FXImage::create() renders rgb[a] data into X/GDI resident, device
    dependent pixmap.
  - Need to be able to re-render subpart of image.
  - We should implement shared pixmaps.
  - If IMAGE_KEEP, repeated rendering is usually desired; should we
    hang on to XImage, and the shared memory segment in that case?
    How about shared pixmaps...
  - Slight change in interpretation of IMAGE_OWNED flag:- if passed, the
    FXImage will own client-side pixel buffer, otherwise it will not; if
    no pixel-buffer has been passed and IMAGE_OWNED *is* passed, a pixel
    buffer will be allocated [and cleared to zero].
    No pixel buffer will be allocated if neither IMAGE_OWNED nor pixels
    are passed.
  - When using shared image/pixmaps, if IMAGE_KEEP is set, hang on to pixel buffer.
  - We need to speed up 16/15 bpp true color.
  - We need dither tables with 3bit and 2bit rounding for 5,6,5/5,5,5 modes
  - We need dither tables with 5bit, 6bit rounding for 3,3,2 mode.
  - We need to split true color from direct color, because direct color
    has random mapping, true has not.
  - Just because I always forget:

      StaticGray   0
      GrayScale    1
      StaticColor  2
      PseudoColor  3
      TrueColor    4
      DirectColor  5

  - The smooth scaling algorithm is based on the idea of keeping track which
    pixels of the source are contributing to each pixel in the destination.

    The smallest fraction of a pixel of interest is 1/(w*w'), where w is
    the old width, and w' is the new one.

    Consider scaling from 3->7 pixels, with 1/(w*w') being 1/21:

      Source Pixel 2 --------------+        Source Pixel is 7/21 units wide
      Source Pixel 1 --------+     |
      Source Pixel 0 -+      |     |
                      |      |     |
                      v      v     v
      Original:    000000011111112222222
      Scaled:      000111222333444555666
                    ^  ^  ^  ^  ^  ^  ^
                    |  |  |  |  |  |  |
      Dest Pixel 0 -+  |  |  |  |  |  |
      Dest Pixel 1 ----+  |  |  |  |  |
      Dest Pixel 2 -------+  |  |  |  |
      Dest Pixel 3 ----------+  |  |  |
      Dest Pixel 4 -------------+  |  |
      Dest Pixel 5 ----------------+  |
      Dest Pixel 6 -------------------+     Dest Pixel is 3/21 units wide

    As can be seen from the picture above, destination pixel 2 is comprised
    of 1/3 times source pixel 0, and 2/3 times source pixel 1.  Splitting
    into the individual fragments yields 9 fragments total:

      Fragment     0   1 2  3   4  5 6   7   8
      ========================================
      Original:  000 000 0 11 111 11 2 222 222
      Scaled:    000 111 2 22 333 44 4 555 666

    Note no fragment can be smaller than one unit, or 1/(w*w').

    The algorithm iterates over fragments; and for each fragment, it adds
    the source pixel multiplied by the length of the fragment.  It repeats
    until enough fragments have been collected to make one output pixel:

      fin  = w';                        The fractions fin and fout have
      fout = w;                         been multiplied by w*w'
      in   = 0;
      out  = 0;
      acc  = 0;
      while out < w' do
        if fin < fout then
          acc = acc + fin*source[in];   Add fin units of source pixel
          fout = fout - fin;
          fin = outw;
          in = in + 1;
        else
          acc = acc + fout*source[in];  Add fout (the remaining unfilled fraction) of source pixel
          dest[out] = acc / w;          Output one pixel
          acc = 0;
          fin = fin - fout;
          fout = inw;
          out = out + 1;
        end if
      end while

    Remember, you saw it here first!!!!

  - When compositing, out-of-image data behaves as if clear (0,0,0,0)
  - Absence of data behaves as if clear
  - Operations work on subrectangle of an image
  - Images and regions are at least 1x1 pixels
  - Operations of the form a = a op b, or a = op a.
  - Fast blend (0..255) colors:

      blue  = (ALPHA * (sb - db) >> 8) + db;
      green = (ALPHA * (sg - dg) >> 8) + dg;
      red   = (ALPHA * (sr - dr) >> 8) + dr;

  - Fast division by 255:

      r=(i+(i>>8)+1)>>8);

  - Rotate using 3 shear operations:

     { cos(phi) -sin(phi) }   { 1 -tan(phi/2) }   { 1         0 }   { 1  -tan(phi/2) }
     {                    } = {               } x {             } x {                }
     { sin(phi)  cos(phi) }   { 0      1      }   { sin(phi)  1 }   { 0      1       }

  - Need to move to BGRA components; 4x faster with OpenGL!
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

// Changable image options
#define IMAGE_MASK   (IMAGE_KEEP|IMAGE_NEAREST|IMAGE_OPAQUE|IMAGE_ALPHACOLOR|IMAGE_SHMI|IMAGE_SHMP|IMAGE_ALPHAGUESS|IMAGE_THRESGUESS)

using namespace FX;

/*******************************************************************************/

namespace FX {

// RGB Ordering code
enum {
  RGB = 7,   // RGB 111      > | R  G  B
  BGR = 0,   // BGR 000      --+--------
  RBG = 6,   // RBG 110      R | x  4  2
  GBR = 1,   // GBR 001      G |    x  1
  BRG = 4,   // BRG 100      B |       x
  GRB = 3    // GRB 011
  };


// Object implementation
FXIMPLEMENT(FXImage,FXDrawable,nullptr,0)


// For deserialization
FXImage::FXImage():data(nullptr),options(0){
  }


// Initialize
FXImage::FXImage(FXApp* a,const FXColor *pix,FXuint opts,FXint w,FXint h):FXDrawable(a,w,h){
  FXTRACE(TOPIC_CONSTRUCT,"FXImage::FXImage %p\n",this);
  FXASSERT((opts&~(IMAGE_OWNED|IMAGE_MASK))==0);
  visual=getApp()->getDefaultVisual();
  data=const_cast<FXColor*>(pix);
  options=opts;
  if(!data && (options&IMAGE_OWNED)){           // This is confusing use of IMAGE_OWNED
    if(!callocElms(data,width*height)){ throw FXMemoryException("unable to construct image"); }
    }
  }


// Change options
void FXImage::setOptions(FXuint opts){
  options=(options&~IMAGE_MASK) | (opts&IMAGE_MASK);
  }


// Set pixel data ownership flag
void FXImage::setOwned(FXbool owned){
  options^=((0-owned)^options)&IMAGE_OWNED;
  }


// Get pixel ownership flag
FXbool FXImage::isOwned() const {
  return (options&IMAGE_OWNED)!=0;
  }


// Attach pixel buffer to image, and assume ownership of it if IMAGE_OWNED is passed
void FXImage::setData(FXColor *pix,FXuint opts){

  // Free old data
  if(options&IMAGE_OWNED){ freeElms(data); }

  // Only own pixel buffer if one was passed
  if(pix && (opts&IMAGE_OWNED)){
    options|=IMAGE_OWNED;
    }
  else{
    options&=~IMAGE_OWNED;
    }

  // Set the pointer
  data=pix;
  }


// Populate the image with new pixel data
void FXImage::setData(FXColor *pix,FXuint opts,FXint w,FXint h){

  // Free old data
  if(options&IMAGE_OWNED){ freeElms(data); }

  // Resize pixmap
  resize(w,h);

  // Only own pixel buffer if one was passed
  if(pix && (opts&IMAGE_OWNED)){
    options|=IMAGE_OWNED;
    }
  else{
    options&=~IMAGE_OWNED;
    }

  // Set the pointer
  data=pix;
  }


// Scan the image and return false if fully opaque
FXbool FXImage::hasAlpha() const {
  if(data){
    FXint i=width*height-1;
    do{
      if(((const FXuchar*)(data+i))[3]<255) return true;
      }
    while(--i>=0);
    }
  return false;
  }


// Detach image
void FXImage::detach(){
  visual->detach();
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    xid=0;
    }
  }


// Release the client-side buffer, free it if it was owned.
void FXImage::release(){
  if(options&IMAGE_OWNED){
    options&=~IMAGE_OWNED;
    freeElms(data);
    }
  data=nullptr;
  }


// Gamma-corrected image scaling.
// In a nutshell: convert pixel value to intensities, scale, then convert
// them back to pixel value:
//
//      I = pow(P,2.2)
//      P = pow(I,1/2.2)
//
// Reference: http://www.ericbrasseur.org/gamma.html
// At some future time this should be based around the gamma used in
// FXVisual; this means generating the table at run time.

// Table lookup: y=pow(x,2.2)
static const FXuint gammatable[256]={
       0,     1,     4,    11,    21,    34,    51,    72,
      97,   125,   158,   195,   236,   282,   332,   386,
     445,   509,   577,   650,   728,   810,   898,   990,
    1087,  1189,  1297,  1409,  1526,  1649,  1776,  1909,
    2048,  2191,  2340,  2494,  2653,  2818,  2988,  3164,
    3346,  3532,  3725,  3923,  4126,  4335,  4550,  4771,
    4997,  5229,  5466,  5710,  5959,  6214,  6475,  6742,
    7014,  7293,  7577,  7868,  8164,  8466,  8775,  9089,
    9410,  9736, 10069, 10407, 10752, 11103, 11460, 11824,
   12193, 12569, 12951, 13339, 13733, 14134, 14541, 14954,
   15374, 15800, 16232, 16671, 17116, 17567, 18025, 18490,
   18961, 19438, 19922, 20412, 20908, 21412, 21922, 22438,
   22961, 23490, 24026, 24569, 25118, 25674, 26237, 26806,
   27382, 27965, 28554, 29150, 29753, 30362, 30978, 31601,
   32231, 32867, 33511, 34161, 34818, 35482, 36152, 36830,
   37514, 38205, 38903, 39608, 40320, 41039, 41765, 42497,
   43237, 43984, 44737, 45498, 46266, 47040, 47822, 48610,
   49406, 50209, 51019, 51836, 52660, 53491, 54329, 55174,
   56027, 56886, 57753, 58627, 59508, 60396, 61291, 62194,
   63103, 64020, 64944, 65876, 66815, 67760, 68714, 69674,
   70642, 71617, 72599, 73588, 74585, 75590, 76601, 77620,
   78646, 79680, 80721, 81769, 82825, 83888, 84958, 86036,
   87122, 88214, 89314, 90422, 91537, 92660, 93790, 94927,
   96072, 97224, 98384, 99552,100727,101909,103099,104297,
  105502,106715,107935,109163,110398,111641,112892,114150,
  115415,116689,117970,119259,120555,121859,123170,124490,
  125817,127151,128493,129843,131201,132566,133940,135320,
  136709,138105,139509,140921,142340,143768,145203,146646,
  148096,149555,151021,152495,153977,155466,156964,158469,
  159982,161503,163032,164569,166114,167666,169226,170795,
  172371,173955,175547,177147,178754,180370,181994,183625,
  185265,186912,188568,190231,191902,193582,195269,196964
  };


static FXuint gammaLookup(FXuint i){
  return gammatable[i];
  }


#if 0
static FXuint gammaInvertLookup(FXuint val){    // New
  FXint m,l=0,h=255;
  while(l<(m=(h+l)>>1)){
    if(gammatable[m]<=val) l=m; else h=m;
    }
  return l;
  }
#endif

static FXuint gammaInvertLookup(FXuint val){
  FXint mid,low=0,high=255;
  while((high-low)>1){
    mid=low+(high-low)/2;
    if(val<gammatable[mid])
      high=mid;
    else
      low=mid;
    }
  return (gammatable[high]==val) ? high : low;
  }


// Horizontal box-filtered, gamma-corrected
static void hscalergbagamma(FXuchar *dst,const FXuchar* src,FXint dw,FXint dh,FXint sw,FXint ){
  FXint fin,fout,ar,ag,ab,aa;
  FXint ss=4*sw;
  FXint ds=4*dw;
  FXuchar *end=dst+ds*dh;
  FXuchar *d;
  const FXuchar *s;
  do{
    s=src; src+=ss;
    d=dst; dst+=ds;
    fin=dw;
    fout=sw;
    ar=ag=ab=aa=0;
    while(1){
      if(fin<fout){
        aa+=fin*s[3];
        ar+=fin*gammaLookup(s[2]);
        ag+=fin*gammaLookup(s[1]);
        ab+=fin*gammaLookup(s[0]);
        fout-=fin;
        fin=dw;
        s+=4;
        }
      else{
        aa+=fout*s[3];              d[3]=aa/sw;
        ar+=fout*gammaLookup(s[2]); d[2]=gammaInvertLookup(ar/sw);
        ag+=fout*gammaLookup(s[1]); d[1]=gammaInvertLookup(ag/sw);
        ab+=fout*gammaLookup(s[0]); d[0]=gammaInvertLookup(ab/sw);
        ar=ag=ab=aa=0;
        fin-=fout;
        fout=sw;
        d+=4;
        if(d>=dst) break;
        }
      }
    }
  while(dst<end);
  }


// Vertical box-filtered, gamma-corrected
static void vscalergbagamma(FXuchar *dst,const FXuchar* src,FXint dw,FXint dh,FXint sw,FXint sh){
  FXint fin,fout,ar,ag,ab,aa;
  FXint ss=4*sw;
  FXint ds=4*dw;
  FXint dss=ds*dh;
  FXuchar *end=dst+ds;
  FXuchar *d,*dd;
  const FXuchar *s;
  do{
    s=src; src+=4;
    d=dst; dst+=4;
    dd=d+dss;
    fin=dh;
    fout=sh;
    ar=ag=ab=aa=0;
    while(1){
      if(fin<fout){
        aa+=fin*s[3];
        ar+=fin*gammaLookup(s[2]);
        ag+=fin*gammaLookup(s[1]);
        ab+=fin*gammaLookup(s[0]);
        fout-=fin;
        fin=dh;
        s+=ss;
        }
      else{
        aa+=fout*s[3];              d[3]=aa/sh;
        ar+=fout*gammaLookup(s[2]); d[2]=gammaInvertLookup(ar/sh);
        ag+=fout*gammaLookup(s[1]); d[1]=gammaInvertLookup(ag/sh);
        ab+=fout*gammaLookup(s[0]); d[0]=gammaInvertLookup(ab/sh);
        ar=ag=ab=aa=0;
        fin-=fout;
        fout=sh;
        d+=ds;
        if(d>=dd) break;
        }
      }
    }
  while(dst<end);
  }


// Horizontal box-filtered
static void hscalergba(FXuchar *dst,const FXuchar* src,FXint dw,FXint dh,FXint sw,FXint ){
  FXint fin,fout,ar,ag,ab,aa;
  FXint ss=4*sw;
  FXint ds=4*dw;
  FXuchar *end=dst+ds*dh;
  FXuchar *d;
  const FXuchar *s;
  do{
    s=src; src+=ss;
    d=dst; dst+=ds;
    fin=dw;
    fout=sw;
    ar=ag=ab=aa=0;
    while(1){
      if(fin<fout){
        aa+=fin*s[3];
        ar+=fin*s[2];
        ag+=fin*s[1];
        ab+=fin*s[0];
        fout-=fin;
        fin=dw;
        s+=4;
        }
      else{
        aa+=fout*s[3]; d[3]=aa/sw; aa=0;
        ar+=fout*s[2]; d[2]=ar/sw; ar=0;
        ag+=fout*s[1]; d[1]=ag/sw; ag=0;
        ab+=fout*s[0]; d[0]=ab/sw; ab=0;
        fin-=fout;
        fout=sw;
        d+=4;
        if(d>=dst) break;
        }
      }
    }
  while(dst<end);
  }


// Vertical box-filtered
static void vscalergba(FXuchar *dst,const FXuchar* src,FXint dw,FXint dh,FXint sw,FXint sh){
  FXint fin,fout,ar,ag,ab,aa;
  FXint ss=4*sw;
  FXint ds=4*dw;
  FXint dss=ds*dh;
  FXuchar *end=dst+ds;
  FXuchar *d,*dd;
  const FXuchar *s;
  do{
    s=src; src+=4;
    d=dst; dst+=4;
    dd=d+dss;
    fin=dh;
    fout=sh;
    ar=ag=ab=aa=0;
    while(1){
      if(fin<fout){
        aa+=fin*s[3];
        ar+=fin*s[2];
        ag+=fin*s[1];
        ab+=fin*s[0];
        fout-=fin;
        fin=dh;
        s+=ss;
        }
      else{
        aa+=fout*s[3]; d[3]=aa/sh; aa=0;
        ar+=fout*s[2]; d[2]=ar/sh; ar=0;
        ag+=fout*s[1]; d[1]=ag/sh; ag=0;
        ab+=fout*s[0]; d[0]=ab/sh; ab=0;
        fin-=fout;
        fout=sh;
        d+=ds;
        if(d>=dd) break;
        }
      }
    }
  while(dst<end);
  }


// Simple nearest neighbor scaling; fast but ugly
static void scalenearest(FXColor *dst,const FXColor* src,FXint dw,FXint dh,FXint sw,FXint sh){
  FXint xs=(sw<<16)/dw;
  FXint ys=(sh<<16)/dh;
  FXint i,j,x,y;
  const FXColor *q;
  FXColor *p;
  i=0;
  y=ys>>1;
  p=dst;
  do{
    j=0;
    x=xs>>1;
    q=src+(y>>16)*sw;
    do{
      p[j]=q[x>>16];
      x+=xs;
      }
    while(++j<dw);
    p+=dw;
    y+=ys;
    }
  while(++i<dh);
  }


// Resize drawable to the specified width and height
void FXImage::scale(FXint w,FXint h,FXint quality){
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE(TOPIC_CREATION,"%s::scale(%d,%d)\n",getClassName(),w,h);
  if(w!=width || h!=height){
    if(data){
      FXint ow=width;
      FXint oh=height;
      FXColor *interim;

      switch(quality){
        case 0:         // Fast but ugly scale

          // Copy to old buffer
          if(!dupElms(interim,data,ow*oh)){ throw FXMemoryException("unable to scale image"); }

          // Resize the pixmap and target buffer
          resize(w,h);

          // Fast nearest neighbor scale
          scalenearest(data,interim,w,h,ow,oh);

          // Free old buffer
          freeElms(interim);
          break;
        case 1:         // Slower box filtered scale

          // Allocate interim buffer
          if(!allocElms(interim,w*oh)){ throw FXMemoryException("unable to scale image"); }

          // Scale horizontally first, placing result into interim buffer
          if(w==ow){
            memcpy((FXuchar*)interim,(FXuchar*)data,w*oh*4);
            }
          else{
            hscalergba((FXuchar*)interim,(FXuchar*)data,w,oh,ow,oh);
            }

          // Resize the pixmap and target buffer
          resize(w,h);

          // Scale vertically from the interim buffer into target buffer
          if(h==oh){
            memcpy((FXuchar*)data,(FXuchar*)interim,w*h*4);
            }
          else{
            vscalergba((FXuchar*)data,(FXuchar*)interim,w,h,w,oh);
            }

          // Free interim buffer
          freeElms(interim);
          break;
        case 2:         // Slow gamma corrected scale
        default:

          // Allocate interim buffer
          if(!allocElms(interim,w*oh)){ throw FXMemoryException("unable to scale image"); }

          // Scale horizontally first, placing result into interim buffer
          if(w==ow){
            memcpy((FXuchar*)interim,(FXuchar*)data,w*oh*4);
            }
          else{
            hscalergbagamma((FXuchar*)interim,(FXuchar*)data,w,oh,ow,oh);
            }

          // Resize the pixmap and target buffer
          resize(w,h);

          // Scale vertically from the interim buffer into target buffer
          if(h==oh){
            memcpy((FXuchar*)data,(FXuchar*)interim,w*h*4);
            }
          else{
            vscalergbagamma((FXuchar*)data,(FXuchar*)interim,w,h,w,oh);
            }

          // Free interim buffer
          freeElms(interim);
          break;
        }
      render();
      }
    else{
      resize(w,h);
      }
    }
  }


// Mirror image horizontally and/or vertically
void FXImage::mirror(FXbool horizontal,FXbool vertical){
  FXTRACE(TOPIC_CREATION,"%s::mirror(%d,%d)\n",getClassName(),horizontal,vertical);
  if(horizontal || vertical){
    if(data){
      FXColor *paa,*pa,*pbb,*pb,t;
      if(vertical && height>1){     // Mirror vertically
        paa=data;
        pbb=data+width*(height-1);
        do{
          pa=paa; paa+=width;
          pb=pbb; pbb-=width;
          do{
            t=*pa; *pa++=*pb; *pb++=t;
            }
          while(pa<paa);
          }
        while(paa<pbb);
        }
      if(horizontal && width>1){    // Mirror horizontally
        paa=data;
        pbb=data+width*height;
        do{
          pa=paa; paa+=width;
          pb=paa;
          do{
            t=*--pb; *pb=*pa; *pa++=t;
            }
          while(pa<pb);
          }
        while(paa<pbb);
        }
      render();
      }
    }
  }


// Rotate image by degrees ccw
void FXImage::rotate(FXint degrees){
  FXTRACE(TOPIC_CREATION,"%s::rotate(%d)\n",getClassName(),degrees);
  degrees=(degrees+360)%360;
  if(degrees!=0 && width>1 && height>1){
    if(data){
      FXColor *paa,*pbb,*end,*pa,*pb;
      FXint size=width*height;
      FXColor *olddata;
      if(!dupElms(olddata,data,size)){ throw FXMemoryException("unable to rotate image"); }
      switch(degrees){
        case 90:
          resize(height,width);
          paa=data;
          pbb=olddata+(height-1);
          end=data+size;
          do{
            pa=paa; paa+=width;
            pb=pbb; pbb-=1;
            do{
              *pa=*pb;
              pa+=1;
              pb+=height;
              }
            while(pa<paa);
            }
          while(paa<end);
          break;
        case 180:
          paa=data;
          pbb=olddata+size;
          end=data+size;
          do{
            pa=paa; paa+=width;
            pb=pbb; pbb-=width;
            do{
              pb-=1;
              *pa=*pb;
              pa+=1;
              }
            while(pa<paa);
            }
          while(paa<end);
          break;
        case 270:
          resize(height,width);
          paa=data;
          pbb=olddata+height*(width-1);
          end=data+size;
          do{
            pa=paa; paa+=width;
            pb=pbb; pbb+=1;
            do{
              *pa=*pb;
              pa+=1;
              pb-=height;
              }
            while(pa<paa);
            }
          while(paa<end);
          break;
        default:
          fxwarning("%s::rotate: rotation by %d degrees not implemented.\n",getClassName(),degrees);
          break;
        }
      freeElms(olddata);
      render();
      }
    else{
      switch(degrees){
        case 90:
          resize(height,width);
          break;
        case 180:
          resize(width,height);
          break;
        case 270:
          resize(height,width);
          break;
        default:
          fxwarning("%s::rotate: rotation by %d degrees not implemented.\n",getClassName(),degrees);
          break;
        }
      }
    }
  }


// Crop image to given rectangle; must have at least one pixel overlap.
void FXImage::crop(FXint x,FXint y,FXint w,FXint h,FXColor color){
  if(w<1) w=1;
  if(h<1) h=1;
  if(x>=width || y>=height || x+w<=0 || y+h<=0){ fxerror("%s::crop: bad arguments.\n",getClassName()); }
  FXTRACE(TOPIC_CREATION,"%s::crop(%d,%d,%d,%d)\n",getClassName(),x,y,w,h);
  if(data){
    FXColor *pnn,*poo,*yyy,*pn,*po,*xx;
    FXint ow=width;
    FXint oh=height;
    FXint nw=w;
    FXint nh=h;
    FXint cw;
    FXint ch;
    FXColor *olddata;
    if(!dupElms(olddata,data,width*height)){ throw FXMemoryException("unable to crop image"); }
    resize(nw,nh);
    pnn=data;
    yyy=data+nw*nh;
    do{
      *pnn++=color;
      }
    while(pnn<yyy);
    if(x<0){
      cw=FXMIN(ow,x+nw);
      if(y<0){
        pnn=data-nw*y;
        poo=olddata;
        ch=FXMIN(oh,y+nh);
        }
      else{
        pnn=data;
        poo=olddata+ow*y;
        ch=FXMIN(oh,y+nh)-y;
        }
      pnn-=x;
      }
    else{
      cw=FXMIN(ow,x+nw)-x;
      if(y<0){
        pnn=data-nw*y;
        poo=olddata;
        ch=FXMIN(oh,y+nh);
        }
      else{
        pnn=data;
        poo=olddata+ow*y;
        ch=FXMIN(oh,y+nh)-y;
        }
      poo+=x;
      }
    FXASSERT(cw>0);
    FXASSERT(ch>0);
    yyy=pnn+nw*ch;
    do{
      pn=pnn;
      po=poo;
      xx=pnn+cw;
      do{
        *pn++=*po++;
        }
      while(pn<xx);
      pnn+=nw;
      poo+=ow;
      }
    while(pnn<yyy);
    freeElms(olddata);
    render();
    }
  else{
    resize(w,h);
    }
  }


/*
    FXuint r=FXREDVAL(color);
    FXuint g=FXGREENVAL(color);
    FXuint b=FXBLUEVAL(color);
    FXuint a=FXALPHAVAL(color);
    FXuchar *pix=data;
    FXuchar *end=pix+height*width*channels;
    FXuchar  tbl[512];

    // Fill table
    for(int i=0; i<256; i++){
      tbl[255+i]=-(i*factor+127)/255;
      tbl[255-i]= (i*factor+127)/255;
      }

    // Fade
    if(channels==4){
      do{
        pix[0]=pix[0]+tbl[255+pix[0]-r];
        pix[1]=pix[1]+tbl[255+pix[1]-g];
        pix[2]=pix[2]+tbl[255+pix[2]-b];
        pix[3]=pix[3]+tbl[255+pix[3]-a];
        pix+=4;
        }
      while(pix<end);
      }
    else{
      do{
        pix[0]=pix[0]+tbl[255+pix[0]-r];
        pix[1]=pix[1]+tbl[255+pix[1]-g];
        pix[2]=pix[2]+tbl[255+pix[2]-b];
        pix+=3;
        }
      while(pix<end);
      }
    }
*/

  // FXColor ____blend(FXColor fg,FXColor bg){
//   FXuint r,g,b,s,t,tmp;
//   s=FXALPHAVAL(fg);
//   t=~s;
//   tmp=FXREDVAL(fg)*s+FXREDVAL(bg)*t+127;     r=(tmp+(tmp>>8))>>8;
//   tmp=FXGREENVAL(fg)*s+FXGREENVAL(bg)*t+127; g=(tmp+(tmp>>8))>>8;
//   tmp=FXBLUEVAL(fg)*s+FXBLUEVAL(bg)*t+127;   b=(tmp+(tmp>>8))>>8;
//   return FXRGB(r,g,b);
//   }
/*
        s=pix[3];
        t=s^0xff;
        w=pix[0]*s+r*t; pix[0]=(w+(w>>8))>>8;
        w=pix[1]*s+g*t; pix[1]=(w+(w>>8))>>8;
        w=pix[2]*s+b*t; pix[2]=(w+(w>>8))>>8;
        s=pix[3];

*/


// Fill image with color
void FXImage::fill(FXColor color){
  if(data){
    FXColor *pix=data;
    FXColor *end=pix+height*width;
    do{ *pix++=color; }while(pix<end);
    }
  }


// Fade image to uniform color
void FXImage::fade(FXColor color,FXint factor){
  if(data){
    FXuint s=factor;
    FXuint t=~factor;
    FXuint r=FXREDVAL(color)*t;
    FXuint g=FXGREENVAL(color)*t;
    FXuint b=FXBLUEVAL(color)*t;
    FXuint a=FXALPHAVAL(color)*t;
    FXuint w;
    FXuchar *pix=(FXuchar*)data;
    FXuchar *end=pix+height*width*4;
    do{
      w=pix[3]*s+a; pix[3]=(w+(w>>8))>>8;
      w=pix[2]*s+r; pix[2]=(w+(w>>8))>>8;
      w=pix[1]*s+g; pix[1]=(w+(w>>8))>>8;
      w=pix[0]*s+b; pix[0]=(w+(w>>8))>>8;
      pix+=4;
      }
    while(pix<end);
    }
  }


// Shear in X
static void shearx(FXuchar *out,FXuchar* in,FXint nwidth,FXint owidth,FXint height,FXint shear,FXColor clr){
  FXuchar *ppp,*pp,*qq,*p,*q,*k;
  FXuint r=FXREDVAL(clr);
  FXuint g=FXGREENVAL(clr);
  FXuint b=FXBLUEVAL(clr);
  FXuint a=FXALPHAVAL(clr);
  FXint dp=owidth<<2;
  FXint dq=nwidth<<2;
  FXint s,z,y,d;
  if(shear){
    if(shear>0){ y=height-1; d=-1; } else { shear=-shear; y=0; d=1; }
    pp=in;
    ppp=pp+height*dp;
    qq=out;
    do{
      p=pp; pp+=dp;
      q=qq; qq+=dq;
      z=(y*shear-1)/(height-1); y+=d;
      s=z&255;
      k=q+(z>>8)*4;
      while(q<k){
        q[3]=a;
        q[2]=r;
        q[1]=g;
        q[0]=b;
        q+=4;
        }
      q[3]=((a-p[3])*s+(p[3]<<8)+127)>>8;
      q[2]=((r-p[2])*s+(p[2]<<8)+127)>>8;
      q[1]=((g-p[1])*s+(p[1]<<8)+127)>>8;
      q[0]=((b-p[0])*s+(p[0]<<8)+127)>>8;
      q+=4;
      p+=4;
      while(p<pp){
        q[3]=((p[3-4]-p[3])*s+(p[3]<<8)+127)>>8;
        q[2]=((p[2-4]-p[2])*s+(p[2]<<8)+127)>>8;
        q[1]=((p[1-4]-p[1])*s+(p[1]<<8)+127)>>8;
        q[0]=((p[0-4]-p[0])*s+(p[0]<<8)+127)>>8;
        q+=4;
        p+=4;
        }
      q[3]=((p[3-4]-a)*s+(a<<8)+127)>>8;
      q[2]=((p[2-4]-r)*s+(r<<8)+127)>>8;
      q[1]=((p[1-4]-g)*s+(g<<8)+127)>>8;
      q[0]=((p[0-4]-b)*s+(b<<8)+127)>>8;
      q+=4;
      while(q<qq){
        q[3]=a;
        q[2]=r;
        q[1]=g;
        q[0]=b;
        q+=4;
        }
      }
    while(pp!=ppp);
    }
  else{
    memcpy(out,in,owidth*height*4);
    }
  }


// Shear in Y
static void sheary(FXuchar *out,FXuchar* in,FXint width,FXint nheight,FXint oheight,FXint shear,FXColor clr){
  FXuchar *ppp,*pp,*qq,*p,*q,*k;
  FXuint r=FXREDVAL(clr);
  FXuint g=FXGREENVAL(clr);
  FXuint b=FXBLUEVAL(clr);
  FXuint a=FXALPHAVAL(clr);
  FXint dp=width<<2;
  FXint s,z,x,d;
  if(shear){
    if(shear>0){ x=width-1; d=-1; } else { shear=-shear; x=0; d=1; }
    pp=in+dp*oheight;
    ppp=pp+dp;
    qq=out+dp*nheight;
    do{
      p=pp-dp*oheight;
      q=qq-dp*nheight;
      z=(x*shear-1)/(width-1); x+=d;
      s=z&255;
      k=q+(z>>8)*dp;
      while(q<k){
        q[3]=r;
        q[2]=g;
        q[1]=b;
        q[0]=a;
        q+=dp;
        }
      q[3]=((a-p[3])*s+(p[3]<<8)+127)>>8;
      q[2]=((r-p[2])*s+(p[2]<<8)+127)>>8;
      q[1]=((g-p[1])*s+(p[1]<<8)+127)>>8;
      q[0]=((b-p[0])*s+(p[0]<<8)+127)>>8;
      q+=dp;
      p+=dp;
      while(p<pp){
        q[3]=((p[3-dp]-p[3])*s+(p[3]<<8)+127)>>8;
        q[2]=((p[2-dp]-p[2])*s+(p[2]<<8)+127)>>8;
        q[1]=((p[1-dp]-p[1])*s+(p[1]<<8)+127)>>8;
        q[0]=((p[0-dp]-p[0])*s+(p[0]<<8)+127)>>8;
        q+=dp;
        p+=dp;
        }
      q[3]=((p[3-dp]-a)*s+(a<<8)+127)>>8;
      q[2]=((p[2-dp]-r)*s+(r<<8)+127)>>8;
      q[1]=((p[1-dp]-g)*s+(g<<8)+127)>>8;
      q[0]=((p[0-dp]-b)*s+(b<<8)+127)>>8;
      q+=dp;
      while(q<qq){
        q[3]=a;
        q[2]=r;
        q[1]=g;
        q[0]=b;
        q+=dp;
        }
      pp+=4;
      qq+=4;
      }
    while(pp!=ppp);
    }
  else{
    memcpy(out,in,width*oheight*4);
    }
  }


// Shear image horizontally
void FXImage::xshear(FXint shear,FXColor clr){
  FXint neww=width+((FXABS(shear)+255)>>8);
  FXint oldw=width;
  FXTRACE(TOPIC_CREATION,"%s::xshear(%d)\n",getClassName(),shear);
  if(data){
    FXColor *olddata;
    if(!dupElms(olddata,data,width*height)){ throw FXMemoryException("unable to xshear image"); }
    resize(neww,height);
    shearx((FXuchar*)data,(FXuchar*)olddata,neww,oldw,height,shear,clr);
    freeElms(olddata);
    render();
    }
  else{
    resize(neww,height);
    }
  }


// Shear image vertically
void FXImage::yshear(FXint shear,FXColor clr){
  FXint newh=height+((FXABS(shear)+255)>>8);
  FXint oldh=height;
  FXTRACE(TOPIC_CREATION,"%s::yshear(%d)\n",getClassName(),shear);
  if(data){
    FXColor *olddata;
    if(!dupElms(olddata,data,width*height)){ throw FXMemoryException("unable to yshear image"); }
    resize(width,newh);
    sheary((FXuchar*)data,(FXuchar*)olddata,width,newh,oldh,shear,clr);
    freeElms(olddata);
    render();
    }
  else{
    resize(width,newh);
    }
  }


// Fill horizontal gradient
void FXImage::hgradient(FXColor left,FXColor right){
  FXint rr,gg,bb,aa,dr,dg,db,da,r1,g1,b1,a1,r2,g2,b2,a2,x;
  FXuchar *ptr=(FXuchar*)data;
  FXuchar *prv=(FXuchar*)data;
  if(ptr && width>1 && height>1){
    r1=FXREDVAL(left);
    r2=FXREDVAL(right);
    rr=(r1<<16)+32768;
    dr=((r2-r1)<<16)/(width-1);
    g1=FXGREENVAL(left);
    g2=FXGREENVAL(right);
    gg=(g1<<16)+32768;
    dg=((g2-g1)<<16)/(width-1);
    b1=FXBLUEVAL(left);
    b2=FXBLUEVAL(right);
    bb=(b1<<16)+32768;
    db=((b2-b1)<<16)/(width-1);
    a1=FXALPHAVAL(left);
    a2=FXALPHAVAL(right);
    aa=(a1<<16)+32768;
    da=((a2-a1)<<16)/(width-1);
    x=width;
    do{
      ptr[3]=aa>>16; aa+=da;
      ptr[2]=rr>>16; rr+=dr;
      ptr[1]=gg>>16; gg+=dg;
      ptr[0]=bb>>16; bb+=db;
      ptr+=4;
      }
    while(--x);
    x=width*(height-1);
    do{
      ptr[0]=prv[0];
      ptr[1]=prv[1];
      ptr[2]=prv[2];
      ptr[3]=prv[3];
      ptr+=4;
      prv+=4;
      }
    while(--x);
    }
  }


// Fill vertical gradient
void FXImage::vgradient(FXColor top,FXColor bottom){
  FXint rr,gg,bb,aa,dr,dg,db,da,r1,g1,b1,a1,r2,g2,b2,a2,x,y;
  FXuchar *ptr=(FXuchar*)data;
  if(ptr && width>1 && height>1){
    r1=FXREDVAL(top);
    r2=FXREDVAL(bottom);
    rr=(r1<<16)+32768;
    dr=((r2-r1)<<16)/(height-1);
    g1=FXGREENVAL(top);
    g2=FXGREENVAL(bottom);
    gg=(g1<<16)+32768;
    dg=((g2-g1)<<16)/(height-1);
    b1=FXBLUEVAL(top);
    b2=FXBLUEVAL(bottom);
    bb=(b1<<16)+32768;
    db=((b2-b1)<<16)/(height-1);
    a1=FXALPHAVAL(top);
    a2=FXALPHAVAL(bottom);
    aa=(a1<<16)+32768;
    da=((a2-a1)<<16)/(height-1);
    y=height;
    do{
      a1=aa>>16; aa+=da;
      r1=rr>>16; rr+=dr;
      g1=gg>>16; gg+=dg;
      b1=bb>>16; bb+=db;
      x=width;
      do{
        ptr[3]=a1;
        ptr[2]=r1;
        ptr[1]=g1;
        ptr[0]=b1;
        ptr+=4;
        }
      while(--x);
      }
    while(--y);
    }
  }


// Fill with gradient
void FXImage::gradient(FXColor topleft,FXColor topright,FXColor bottomleft,FXColor bottomright){
  FXint rl,gl,bl,al,rr,gr,br,ar,drl,dgl,dbl,dal,drr,dgr,dbr,dar,r,g,b,a,dr,dg,db,da,x,y;
  FXint rtl,gtl,btl,atl,rtr,gtr,btr,atr,rbl,gbl,bbl,abl,rbr,gbr,bbr,abr;
  FXuchar *ptr=(FXuchar*)data;
  if(ptr && width>1 && height>1){

    rtl=FXREDVAL(topleft);
    rbl=FXREDVAL(bottomleft);
    rl=(rtl<<16)+32768; drl=((rbl-rtl)<<16)/(height-1);

    gtl=FXGREENVAL(topleft);
    gbl=FXGREENVAL(bottomleft);
    gl=(gtl<<16)+32768; dgl=((gbl-gtl)<<16)/(height-1);

    btl=FXBLUEVAL(topleft);
    bbl=FXBLUEVAL(bottomleft);
    bl=(btl<<16)+32768; dbl=((bbl-btl)<<16)/(height-1);

    rtr=FXREDVAL(topright);
    rbr=FXREDVAL(bottomright);
    rr=(rtr<<16)+32768; drr=((rbr-rtr)<<16)/(height-1);

    gtr=FXGREENVAL(topright);
    gbr=FXGREENVAL(bottomright);
    gr=(gtr<<16)+32768; dgr=((gbr-gtr)<<16)/(height-1);

    btr=FXBLUEVAL(topright);
    bbr=FXBLUEVAL(bottomright);
    br=(btr<<16)+32768; dbr=((bbr-btr)<<16)/(height-1);

    atl=FXALPHAVAL(topleft);
    abl=FXALPHAVAL(bottomleft);
    al=(atl<<16)+32768; dal=((abl-atl)<<16)/(height-1);

    atr=FXALPHAVAL(topright);
    abr=FXALPHAVAL(bottomright);
    ar=(atr<<16)+32768; dar=((abr-atr)<<16)/(height-1);

    y=height;
    do{
      a=al; da=(ar-al)/(width-1);
      r=rl; dr=(rr-rl)/(width-1);
      g=gl; dg=(gr-gl)/(width-1);
      b=bl; db=(br-bl)/(width-1);
      x=width;
      do{
        ptr[3]=a>>16; a+=da;
        ptr[2]=r>>16; r+=dr;
        ptr[1]=g>>16; g+=dg;
        ptr[0]=b>>16; b+=db;
        ptr+=4;
        }
      while(--x);
      rl+=drl;
      gl+=dgl;
      bl+=dbl;
      al+=dal;
      rr+=drr;
      gr+=dgr;
      br+=dbr;
      ar+=dar;
      }
    while(--y);
    }
  }


// Blend image over uniform color
void FXImage::blend(FXColor color){
  if(data){
    FXuchar *pix=(FXuchar*)data;
    FXuchar *end=pix+height*width*4;
    FXint r=FXREDVAL(color);
    FXint g=FXGREENVAL(color);
    FXint b=FXBLUEVAL(color);
    FXint s,w;
    do{
      s=pix[3];
      w=(pix[2]-r)*s; pix[2]=r+((w+(w>>8)+128)>>8);
      w=(pix[1]-g)*s; pix[1]=g+((w+(w>>8)+128)>>8);
      w=(pix[0]-b)*s; pix[0]=b+((w+(w>>8)+128)>>8);             /* FIXME need to write new alpha also */
      pix+=4;
      }
    while(pix<end);
    }
  }


// Invert colors of an image
void FXImage::invert(){
  if(data){
    FXuchar *pix=(FXuchar*)data;
    FXuchar *end=pix+height*width*4;
    do{
      pix[1]=255-pix[1];
      pix[2]=255-pix[2];
      pix[3]=255-pix[3];
      pix+=4;
      }
    while(pix<end);
    }
  }


// Colorize image based on luminance
void FXImage::colorize(FXColor color){
  if(data){
    FXuchar *pix=(FXuchar*)data;
    FXuchar *end=pix+height*width*4;
    FXint r=FXREDVAL(color);
    FXint g=FXGREENVAL(color);
    FXint b=FXBLUEVAL(color);
    FXint lum,w;
    do{
      if(pix[3]){
        lum=(77*pix[2]+151*pix[1]+29*pix[0])>>8;
        w=r*lum; pix[2]=(w+(w>>8))>>8;
        w=g*lum; pix[1]=(w+(w>>8))>>8;
        w=b*lum; pix[0]=(w+(w>>8))>>8;
        }
      pix+=4;
      }
    while(pix<end);
    }
  }

/*


// Fill with diagonal gradient RGB
static void dgradientrgba(FXuchar *dst,FXint w,FXint h,FXint r1,FXint g1,FXint b1,FXint a1,FXint r2,FXint g2,FXint b2,FXint a2){
  FXint rr,gg,bb,aa,drx,dgx,dbx,dax,dry,dgy,dby,day,x,y;
  FXuchar *ptr=dst;
  FXuchar xtable[4][2048];
  FXuchar ytable[4][2048];
  FXASSERT(w>0 && h>0);
  FXASSERT(w<2048 && h<2048);
  drx=dry=((r2-r1)<<16);
  dgx=dgy=((g2-g1)<<16);
  dbx=dby=((b2-b1)<<16);
  dax=day=((a2-a1)<<16);
  rr=(r1<<16)+32768;
  gg=(g1<<16)+32768;
  bb=(b1<<16)+32768;
  aa=(a1<<16)+32768;
  drx/=(w-1)*2;
  dgx/=(w-1)*2;
  dbx/=(w-1)*2;
  dax/=(w-1)*2;
  x=w;
  do{
    --x;
    xtable[0][x]=rr>>16; rr+=drx;
    xtable[1][x]=gg>>16; gg+=dgx;
    xtable[2][x]=bb>>16; bb+=dbx;
    xtable[3][x]=aa>>16; aa+=dax;
    }
  while(x);
  rr=32768;
  gg=32768;
  bb=32768;
  aa=32768;
  dry/=(h-1)*2;
  dgy/=(h-1)*2;
  dby/=(h-1)*2;
  day/=(h-1)*2;
  y=h;
  do{
    --y;
    ytable[0][y]=rr>>16; rr+=dry;
    ytable[1][y]=gg>>16; gg+=dgy;
    ytable[2][y]=bb>>16; bb+=dby;
    ytable[3][y]=aa>>16; aa+=day;
    }
  while(y);
  y=h;
  do{
    --y;
    x=w;
    do{
      --x;
      ptr[0]=xtable[0][x]+ytable[0][y];
      ptr[1]=xtable[1][x]+ytable[1][y];
      ptr[2]=xtable[2][x]+ytable[2][y];
      ptr[3]=xtable[3][x]+ytable[3][y];
      ptr+=4;
      }
    while(x);
    }
  while(y);
  }
*/


// Save pixel data only
FXbool FXImage::savePixels(FXStream& store) const {
  FXuint size=width*height;
  store.save(data,size);
  return true;
  }


// Load pixel data only
FXbool FXImage::loadPixels(FXStream& store){
  FXuint size=width*height;
  if(options&IMAGE_OWNED){freeElms(data);}
  if(!allocElms(data,size)) return false;
  store.load(data,size);
  options|=IMAGE_OWNED;
  return true;
  }


// Save data
void FXImage::save(FXStream& store) const {
  FXuchar haspixels=(data!=nullptr);
  FXDrawable::save(store);
  store << options;
  store << haspixels;
  if(haspixels) savePixels(store);
  }


// Load data
void FXImage::load(FXStream& store){
  FXuchar haspixels;
  FXDrawable::load(store);
  store >> options;
  store >> haspixels;
  if(haspixels) loadPixels(store);
  }


// Clean up
FXImage::~FXImage(){
  FXTRACE(TOPIC_CONSTRUCT,"FXImage::~FXImage %p\n",this);
  destroy();
  if(options&IMAGE_OWNED){freeElms(data);}
  data=(FXColor*)-1L;
  }

}
