/********************************************************************************
*                                                                               *
*                     G L   V i s u a l   ( W a y l a n d )                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
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
#include "fxglwayland.h"

#if defined(FOX_WAYLAND)

#if defined(HAVE_EGL_H)
#include <EGL/egl.h>

// Not every EGL header spells out this config attribute
#ifndef EGL_DOUBLE_BUFFER
#define EGL_DOUBLE_BUFFER 0x3035
#endif
#endif

/*
  Notes:

  - OpenGL on Wayland comes from EGL: the visual picks an EGLConfig with the
    requested color, depth and stencil sizes.  There is no X Visual or colormap,
    so the config itself is what FXGLContext gets from the visual.
  - The drawing surface and its wl_egl_window belong to the canvas subsurface,
    not to the visual; see fxglwayland.cpp.
*/

#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Test if OpenGL is possible
FXbool FXGLVisual::hasOpenGL(FXApp* app){
  return fxglwaylandOpen(app);
  }


// Initialize
void FXGLVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
#if defined(HAVE_EGL_H)
      {
      EGLDisplay dpy;
      EGLConfig config=nullptr;
      EGLConfig configs[64];
      EGLint attrs[40];
      EGLint nconfigs=0;
      EGLint value;
      FXint i=0;
      if(!fxglwaylandOpen(getApp())) throw FXWindowException("OpenGL is not supported on Wayland.");
      dpy=(EGLDisplay)fxglwaylandDisplay(getApp());
      attrs[i++]=EGL_SURFACE_TYPE;    attrs[i++]=EGL_WINDOW_BIT;
      attrs[i++]=EGL_RENDERABLE_TYPE; attrs[i++]=EGL_OPENGL_BIT;
      attrs[i++]=EGL_RED_SIZE;        attrs[i++]=redSize;
      attrs[i++]=EGL_GREEN_SIZE;      attrs[i++]=greenSize;
      attrs[i++]=EGL_BLUE_SIZE;       attrs[i++]=blueSize;
      if(alphaSize){ attrs[i++]=EGL_ALPHA_SIZE; attrs[i++]=alphaSize; }
      if(depthSize){ attrs[i++]=EGL_DEPTH_SIZE; attrs[i++]=depthSize; }
      if(stencilSize){ attrs[i++]=EGL_STENCIL_SIZE; attrs[i++]=stencilSize; }
      if(multiSamples){
        attrs[i++]=EGL_SAMPLE_BUFFERS; attrs[i++]=1;
        attrs[i++]=EGL_SAMPLES;        attrs[i++]=multiSamples;
        }
      attrs[i++]=EGL_NONE;
      if(!eglChooseConfig(dpy,attrs,configs,ARRAYNUMBER(configs),&nconfigs) || nconfigs<1){
        throw FXWindowException("no matching GL configuration.");
        }

      // Prefer a double buffered config; a single buffered one never shows a
      // swap, and an application that only swaps when the visual says so (as
      // gltest does) would draw into nothing
      for(i=0; i<nconfigs; i++){
        value=0;
        if(eglGetConfigAttrib(dpy,configs[i],EGL_DOUBLE_BUFFER,&value) && value){
          config=configs[i];
          break;
          }
        }
      if(!config) config=configs[0];

      xid=(FXID)config;
      visual=nullptr;
      if(eglGetConfigAttrib(dpy,config,EGL_RED_SIZE,&value))     actualRedSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_GREEN_SIZE,&value))   actualGreenSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_BLUE_SIZE,&value))    actualBlueSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_ALPHA_SIZE,&value))   actualAlphaSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_DEPTH_SIZE,&value))   actualDepthSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_STENCIL_SIZE,&value)) actualStencilSize=value;
      if(eglGetConfigAttrib(dpy,config,EGL_SAMPLES,&value))      actualMultiSamples=value;
      // A Wayland surface always hands a buffer to the compositor on commit,
      // so the canvas presents a swap even when the EGL config itself is
      // single buffered; report double, or gltest and glviewer, which only
      // swap when the visual is double buffered, would draw into nothing
      doubleBuffer=true;
      depth=actualRedSize+actualGreenSize+actualBlueSize;
      }
#else
      throw FXWindowException("OpenGL is not supported on Wayland.");
#endif
      }
    }
  if(!xid){
    throw FXWindowException("no matching GL configuration.");
    }
  }


// Destroy visual
void FXGLVisual::destroy(){
  xid=0;
  }

}

#endif
