/********************************************************************************
*                                                                               *
*                       G L   V i s u a l   ( W i n 3 2 )                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLVisual.h"
#include "fxglpriv.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of FXGLVisual for Win32: pixel format selection.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002
#define TOPIC_DEBUG     1003

using namespace FX;

/*******************************************************************************/

namespace FX {

// Test if OpenGL is possible
FXbool FXGLVisual::hasOpenGL(FXApp* application){
  if(application->isInitialized()){
#if defined(HAVE_GL_H)
    return true;
#endif
    }
  return false;
  }



#ifndef PFD_SUPPORT_COMPOSITION
#define PFD_SUPPORT_COMPOSITION         0x00008000
#endif

#ifdef HAVE_GL_H

// Palette struct
struct LOGPALETTE256 {
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[257];
  };


// System colors to match against
static FXuchar defSysClr[20][3] = {
    {  0,  0,  0},
    {128,  0,  0},
    {  0,128,  0},
    {128,128,  0},
    {  0,  0,128},
    {128,  0,128},
    {  0,128,128},
    {192,192,192},

    {192,220,192},
    {166,202,240},
    {255,251,240},
    {160,160,164},

    {128,128,128},
    {255,  0,  0},
    {  0,255,  0},
    {255,255,  0},
    {  0,  0,255},
    {255,  0,255},
    {  0,255,255},
    {255,255,255}
    };

static int defaultOverride[13] = {
  0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
  };


// Make palette
static HPALETTE makeOpenGLPalette(PIXELFORMATDESCRIPTOR* info){
  LOGPALETTE256 palette;
  int num,i,j,rr,gg,bb;
  int rmax,gmax,bmax;
  HPALETTE hPalette;

  // Size of palette array
  num=1<<((PIXELFORMATDESCRIPTOR*)info)->cColorBits;

  FXASSERT(num<=256);

  // Maximum values each color
  rmax=(1 << info->cRedBits)-1;
  gmax=(1 << info->cGreenBits)-1;
  bmax=(1 << info->cBlueBits)-1;

  // Build palette
  for(rr=0; rr<=rmax; rr++){
    for(gg=0; gg<=gmax; gg++){
      for(bb=0; bb<=bmax; bb++){
        i = (rr << info->cRedShift) | (gg << info->cGreenShift) | (bb << info->cBlueShift);
        palette.palPalEntry[i].peRed = (255*rr)/rmax;
        palette.palPalEntry[i].peGreen = (255*gg)/gmax;
        palette.palPalEntry[i].peBlue = (255*bb)/bmax;
        palette.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
        }
      }
    }

  // For 8-bit palette
  if((info->cColorBits==8) && (info->cRedBits==3) && (info->cRedShift==0) && (info->cGreenBits==3) && (info->cGreenShift==3) && (info->cBlueBits==2) && (info->cBlueShift==6)){
    for(j=1; j<=12; j++){
      palette.palPalEntry[defaultOverride[j]].peRed=defSysClr[j][0];
      palette.palPalEntry[defaultOverride[j]].peGreen=defSysClr[j][1];
      palette.palPalEntry[defaultOverride[j]].peBlue=defSysClr[j][1];
      palette.palPalEntry[defaultOverride[j]].peFlags=0;
      }
    }

  // Fill in the rest
  palette.palVersion=0x300;
  palette.palNumEntries=num;

  // Make palette
  hPalette=CreatePalette((const LOGPALETTE*)&palette);

  return hPalette;
  }

#endif


// Initialize
void FXGLVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
#ifdef HAVE_GL_H
      PIXELFORMATDESCRIPTOR pfd;
      int bestmatch=1000000000;
      int best=-1;
      int match;
      int npf;
      HDC hdc;

      // Get some window handle
      hdc=GetDC(GetDesktopWindow());

      // Get number of supported pixel formats
      pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
      pfd.nVersion=1;

      // Get number of supported pixel formats
      if(0<(npf=DescribePixelFormat(hdc,1,sizeof(PIXELFORMATDESCRIPTOR),&pfd))){

        FXTRACE(TOPIC_DETAIL,"Found %d OpenGL configs\n",npf);

        // Try to find the best
        for(int v=1; v<=npf; v++){
          FXGLSpecs specs;

          // Get info about this visual
          DescribePixelFormat(hdc,v,sizeof(PIXELFORMATDESCRIPTOR),&pfd);

          // Make sure this visual is valid
//          if(ChoosePixelFormat(hdc,&pfd)!=v) continue;

          // Get supported render type; we don't want index mode
          if(pfd.iPixelType==PFD_TYPE_COLORINDEX) continue;

          // OpenGL support is required
          if(!(pfd.dwFlags&PFD_SUPPORT_OPENGL)) continue;

          // Draw to window is required
          if(!(pfd.dwFlags&PFD_DRAW_TO_WINDOW)) continue;

          // Get planes
          specs.redsize=pfd.cRedBits;
          specs.greensize=pfd.cGreenBits;
          specs.bluesize=pfd.cBlueBits;
          specs.alphasize=pfd.cAlphaBits;
          specs.depthsize=pfd.cDepthBits;
          specs.stencilsize=pfd.cStencilBits;
          specs.accumredsize=pfd.cAccumRedBits;
          specs.accumgreensize=pfd.cAccumGreenBits;
          specs.accumbluesize=pfd.cAccumBlueBits;
          specs.accumalphasize=pfd.cAccumAlphaBits;

          // Double buffer capable
          specs.doublebuffer=(pfd.dwFlags&PFD_DOUBLEBUFFER)!=0;

          // Stereo capable
          specs.stereobuffer=(pfd.dwFlags&PFD_STEREO)!=0;

          // Multisample
          specs.multisamples=0;

          // Windows Vista and Windows 7 composition support
          specs.composition=(pfd.dwFlags&PFD_SUPPORT_COMPOSITION)!=0;

          // Bitmap drawing would be nice
          specs.image=(pfd.dwFlags&PFD_DRAW_TO_BITMAP)!=0;

          // Hardware accelerated format
          specs.accel=(pfd.dwFlags&PFD_GENERIC_FORMAT)==0;

          // Swap buffers by copying
          specs.copy=(pfd.dwFlags&PFD_SWAP_COPY)!=0;

          // Match specs
          match=matchSpecs(specs);

          // Trace
          FXTRACE(TOPIC_DEBUG,"Config: #%d: match=%d\n",v,match);
          FXTRACE(TOPIC_DEBUG,"  drawables  = %s%s\n",(pfd.dwFlags&PFD_DRAW_TO_WINDOW)?"PFD_DRAW_TO_WINDOW ":"",(pfd.dwFlags&PFD_DRAW_TO_BITMAP)?"PFD_DRAW_TO_BITMAP ":"");
          FXTRACE(TOPIC_DEBUG,"  render     = %s\n",(pfd.iPixelType==PFD_TYPE_COLORINDEX)?"PFD_TYPE_COLORINDEX":(pfd.iPixelType==PFD_TYPE_RGBA)?"PFD_TYPE_RGBA":"");
          FXTRACE(TOPIC_DEBUG,"  red size   = %d\n",specs.redsize);
          FXTRACE(TOPIC_DEBUG,"  green size = %d\n",specs.greensize);
          FXTRACE(TOPIC_DEBUG,"  blue size  = %d\n",specs.bluesize);
          FXTRACE(TOPIC_DEBUG,"  alpha size = %d\n",specs.alphasize);
          FXTRACE(TOPIC_DEBUG,"  depth size = %d\n",specs.depthsize);
          FXTRACE(TOPIC_DEBUG,"  stencil    = %d\n",specs.stencilsize);
          FXTRACE(TOPIC_DEBUG,"  acc red    = %d\n",specs.accumredsize);
          FXTRACE(TOPIC_DEBUG,"  acc green  = %d\n",specs.accumgreensize);
          FXTRACE(TOPIC_DEBUG,"  acc blue   = %d\n",specs.accumbluesize);
          FXTRACE(TOPIC_DEBUG,"  acc alpha  = %d\n",specs.accumalphasize);
          FXTRACE(TOPIC_DEBUG,"  double buf = %d\n",specs.doublebuffer);
          FXTRACE(TOPIC_DEBUG,"  stereo buf = %d\n",specs.stereobuffer);
          FXTRACE(TOPIC_DEBUG,"  samples    = %d\n",specs.multisamples);
          FXTRACE(TOPIC_DEBUG,"  accel      = %d\n",specs.accel);

          // May the best visual win
          if(match<=bestmatch){
            actualRedSize=specs.redsize;
            actualGreenSize=specs.greensize;
            actualBlueSize=specs.bluesize;
            actualAlphaSize=specs.alphasize;
            actualDepthSize=specs.depthsize;
            actualStencilSize=specs.stencilsize;
            actualMultiSamples=specs.multisamples;
            actualAccumRedSize=specs.accumredsize;
            actualAccumGreenSize=specs.accumgreensize;
            actualAccumBlueSize=specs.accumbluesize;
            actualAccumAlphaSize=specs.accumalphasize;
            doubleBuffer=specs.doublebuffer;
            stereoBuffer=specs.stereobuffer;
            accelerated=specs.accel;
            copying=specs.copy;
            bestmatch=match;
            best=v;
            }
          }
        }

      // Hopefully, we got one
      if(0<=best){
        FXTRACE(TOPIC_DETAIL,"Best Config: #%d: match=%d\n",best,bestmatch);

        // Fill in visual data
        depth=actualRedSize+actualGreenSize+actualBlueSize;
        numred=(1<<actualRedSize);
        numgreen=(1<<actualGreenSize);
        numblue=(1<<actualBlueSize);
        numcolors=numred*numgreen*numblue;

        // Make a palette for it if needed
        if(pfd.dwFlags&PFD_NEED_PALETTE){
          colormap=makeOpenGLPalette(&pfd);
          freemap=true;
          }

        // Remember best config
        xid=(void*)(FXival)best;
        }

      // Done with that window
      ::ReleaseDC(GetDesktopWindow(),hdc);

      // Make non-current
      wglMakeCurrent(nullptr,nullptr);
#endif
      }

    // Test if successful
    if(!xid){
      throw FXWindowException("unable to create GL context.");
      }
    }
  }


// Destroy visual
void FXGLVisual::destroy(){
#if defined(HAVE_GL_H)
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      if(colormap){ DeleteObject(colormap); }
      }
    colormap=0;
    freemap=false;
    xid=0;
    }
#endif
  }

}

#endif
