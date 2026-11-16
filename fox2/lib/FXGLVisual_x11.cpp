/********************************************************************************
*                                                                               *
*                         G L   V i s u a l   ( X 1 1 )                         *
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

#if defined(FOX_X11)

/*
  Notes:

  - Platform implementation of FXGLVisual for X11: GLX visual selection.
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
    return glXQueryExtension((Display*)application->getDisplay(),nullptr,nullptr);
#endif
    }
  return false;
  }




#ifndef GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB             100000
#define GLX_SAMPLES_ARB                    100001
#endif
#ifndef GLX_EXT_visual_rating
#define GLX_VISUAL_CAVEAT_EXT              0x20
#define GLX_SLOW_VISUAL_EXT                0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT      0x800D
#endif


// Initialize
void FXGLVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::create %p\n",getClassName(),this);
#ifdef HAVE_GL_H
      int majoropcode,errorbase,eventbase;

      // Assume the default
      visual=DefaultVisual((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()));
      depth=DefaultDepth((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()));

      // OpenGL is available if we're talking to an OpenGL-capable X-Server
      if(XQueryExtension((Display*)getApp()->getDisplay(),"GLX",&majoropcode,&errorbase,&eventbase)){
        if(glXQueryExtension((Display*)getApp()->getDisplay(),&errorbase,&eventbase)){
          int major,minor;

          // Try get OpenGL version info
          if(glXQueryVersion((Display*)getApp()->getDisplay(),&major,&minor)){
            XVisualInfo vitemplate,*vi; int nvi;

            // Report version found
            FXTRACE(TOPIC_DETAIL,"Found GLX version: %d.%d (Major: %d, Error: %d, Event: %d)\n",major,minor,majoropcode,errorbase,eventbase);

            // Scan for all visuals of given screen
            vitemplate.screen=DefaultScreen((Display*)getApp()->getDisplay());
            vi=XGetVisualInfo((Display*)getApp()->getDisplay(),VisualScreenMask,&vitemplate,&nvi);
            if(vi){
              VisualID defvisualid=XVisualIDFromVisual(DefaultVisual((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay())));
              int samplebuffers=0;
              int bestmatch=1000000000;
              int best=-1;
              int value;
              int match;

              FXTRACE(TOPIC_DEBUG,"Found %d configs\n",nvi);
              FXTRACE(TOPIC_DEBUG,"Default visualid=0x%02x\n",defvisualid);

              // Find the best one
              for(int v=0; v<nvi; v++){
                FXGLSpecs specs;

                // GL support is requested
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_USE_GL,&value)!=Success || (value==0)) continue;

                // Get supported drawable targets
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_RGBA,&value)!=Success || (value==0)) continue;

                // Get overlay level
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_LEVEL,&value)!=Success || (value!=0)) continue;

                // Get plane depths
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_RED_SIZE,&specs.redsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_GREEN_SIZE,&specs.greensize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_BLUE_SIZE,&specs.bluesize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ALPHA_SIZE,&specs.alphasize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_DEPTH_SIZE,&specs.depthsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_STENCIL_SIZE,&specs.stencilsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_RED_SIZE,&specs.accumredsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_GREEN_SIZE,&specs.accumgreensize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_BLUE_SIZE,&specs.accumbluesize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_ALPHA_SIZE,&specs.accumalphasize)!=Success) continue;

                // Get stereo and double buffer support
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_DOUBLEBUFFER,&specs.doublebuffer)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_STEREO,&specs.stereobuffer)!=Success) continue;

                // Get multisample support (if we don't succeed, set it to zero)
                specs.multisamples=0;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_SAMPLE_BUFFERS_ARB,&samplebuffers)==Success && samplebuffers){
                  if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_SAMPLES_ARB,&specs.multisamples)!=Success) continue;
                  }

                // Check if accelerated or not (assume yes)
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_VISUAL_CAVEAT_EXT,&specs.accel)==Success){
                  specs.accel=(specs.accel!=GLX_SLOW_VISUAL_EXT);        // Penalize if any caveats present
                  }
                else{
                  specs.accel=1;
                  }

                // Composition and swap copy
                specs.composition=false;
                specs.image=false;
                specs.copy=false;

                // Match specs
                match=matchSpecs(specs);

                // Trace
                FXTRACE(TOPIC_DEBUG,"Config: #%d: match=%d\n",v,match);
                FXTRACE(TOPIC_DEBUG,"  visualid   = 0x%02lx\n",vi[v].visualid);
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

                // May the best config win
                if(match<=bestmatch){

                  // All other things being equal, we prefer default visual!
                  if((match<bestmatch) || (vi[v].visualid==defvisualid)){
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

              // We should have one now
              if(0<=best){
                FXTRACE(TOPIC_DETAIL,"Best Config: #%d: match=%d\n",best,bestmatch);

                // Remember visual, depth, visualid
                visual=vi[best].visual;
                depth=vi[best].depth;

                // Initialize colormap
                setupcolormap();

                // Make GC's for this visual
                scrollgc=setupgc(true);
                gc=setupgc(false);

                // Remember best config
                xid=((Visual*)visual)->visualid;
                }

              // Free visual info
              XFree((char*)vi);
              }
            }
          }
        }
#endif
      }

    // Test if successful
    if(!xid){
      throw FXWindowException("no matching GL configuration.");
      }
    }
  }


// Destroy visual
void FXGLVisual::destroy(){
#if defined(HAVE_GL_H)
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE(TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this);
      if(freemap){ XFreeColormap((Display*)getApp()->getDisplay(),colormap); }
      if(scrollgc){ XFreeGC((Display*)getApp()->getDisplay(),(GC)scrollgc); }
      if(gc){ XFreeGC((Display*)getApp()->getDisplay(),(GC)gc); }
      }
    scrollgc=nullptr;
    gc=nullptr;
    colormap=0;
    freemap=false;
    xid=0;
    }
#endif
  }

}

#endif
