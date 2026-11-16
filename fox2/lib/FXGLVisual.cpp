/********************************************************************************
*                                                                               *
*                            V i s u a l   C l a s s                            *
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
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLVisual.h"
#include "fxglpriv.h"


/*
  Notes:

  - FXGLVisual builds a visual/pixelformat suitable for GL drawing.

  - Selection of visual/pixelformat is based on a best match to a
    given set of hints, according to some heuristics:

    1) Prefer color depth close to asked ones; really like to
       get a bit MORE color, rather than LESS, however.

    2) If we wanted Z-buffer, it is STRONGLY preferred; If there's
       a choice, we prefer more color depth over more Z-depth; if
       we already have more colors than requested, we prefer to meet
       requested Z depth.

    3) If we wanted double buffer, we strongly prefer it over color and
       Z depth, but HAVING a Z-buffer is still more important.

    4) If we wanted alpha buffer, it is preferred, but Z-buffering
       and double buffering are considered more important.
       If there's a choice, we prefer to receive a few MORE bits of
       alpha buffer than we asked for, rather than LESS.

    5) If we wanted stereo, we prefer it, but almost everything except
       the color-, alpha-, and Z-depths are more important.

  - Some further tuning may be desired, but I think this should satisfy
    most cases....

  - Note that as long as OpenGL is in any way supported, you should ALWAYS
    be able to get at least some visual/pixelformat you can draw on.

  - As far as hardware acceleration goes, H/W acceleration should be
    enabled, possibly at the expense of color-, alpha-, and Z-depth;
    but NEVER at the expense of presence or absence of a requested feature.
    We only drop FEATURES which are requested if there is neither hardware
    nor software support.

    For example, we may trade in some Z-depth, but not the entire Z-buffer,
    to get a hardware accelerated visual/pixelformat.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_DETAIL    1002
#define TOPIC_DEBUG     1003

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGLVisual,FXVisual,nullptr,0)


// Deserialization
FXGLVisual::FXGLVisual(){
  redSize=0;
  greenSize=0;
  blueSize=0;
  alphaSize=0;
  depthSize=0;
  stencilSize=0;
  multiSamples=0;
  accumRedSize=0;
  accumGreenSize=0;
  accumBlueSize=0;
  accumAlphaSize=0;
  actualRedSize=0;
  actualGreenSize=0;
  actualBlueSize=0;
  actualAlphaSize=0;
  actualDepthSize=0;
  actualStencilSize=0;
  actualMultiSamples=0;
  actualAccumRedSize=0;
  actualAccumGreenSize=0;
  actualAccumBlueSize=0;
  actualAccumAlphaSize=0;
  doubleBuffer=false;
  stereoBuffer=false;
  accelerated=false;
  copying=false;
  }


// Construct
FXGLVisual::FXGLVisual(FXApp* a,FXuint flgs):FXVisual(a,flgs,0){
  FXTRACE(TOPIC_CONSTRUCT,"FXGLVisual::FXGLVisual %p\n",this);
  redSize=8;
  greenSize=8;
  blueSize=8;
  alphaSize=0;
  depthSize=24;
  stencilSize=0;
  multiSamples=0;
  accumRedSize=0;
  accumGreenSize=0;
  accumBlueSize=0;
  accumAlphaSize=0;
  actualRedSize=0;
  actualGreenSize=0;
  actualBlueSize=0;
  actualAlphaSize=0;
  actualDepthSize=0;
  actualStencilSize=0;
  actualMultiSamples=0;
  actualAccumRedSize=0;
  actualAccumGreenSize=0;
  actualAccumBlueSize=0;
  actualAccumAlphaSize=0;
  doubleBuffer=false;
  stereoBuffer=false;
  accelerated=false;
  copying=false;
  }


// Match actual GL depth to desired one
FXint FXGLVisual::matchSpecs(const FXGLSpecs& s){
  FXint match,dred,dgreen,dblue,dalpha,ddepth,dstencil,dsamples,daccred,daccgreen,daccblue,daccalpha;

  // We prefer to get a few MORE bits in RGBA than we asked for
  dred   = s.redsize-redSize;     if(dred<0)   dred   *= -100;
  dgreen = s.greensize-greenSize; if(dgreen<0) dgreen *= -100;
  dblue  = s.bluesize-blueSize;   if(dblue<0)  dblue  *= -100;
  dalpha = s.alphasize-alphaSize; if(dalpha<0) dalpha *= -100;

  // Prefer better Z than asked, but colors more important
  ddepth = s.depthsize-depthSize; if(ddepth<0) ddepth *= -10;

  // We care about colors and Z depth more than stencil depth
  dstencil = s.stencilsize-stencilSize; if(dstencil<0) dstencil *= -1;

  // Accumulation buffers
  daccred=s.accumredsize-accumRedSize;       if(daccred<0)   daccred   *= -1;
  daccgreen=s.accumgreensize-accumGreenSize; if(daccgreen<0) daccgreen *= -1;
  daccblue=s.accumbluesize-accumBlueSize;    if(daccblue<0)  daccblue  *= -1;
  daccalpha=s.accumalphasize-accumAlphaSize; if(daccalpha<0) daccalpha *= -1;

  // Want the best colors, of course
  match=dred+dgreen+dblue+dalpha;

  // Accumulation buffers
  match+=daccred+daccgreen+daccblue+daccalpha;

  // Hardware accelerated is normally a plus
  if(!s.accel && !(flags&VISUAL_NO_ACCEL)){
    match+=10000;
    }

  // Extra penalty for no alpha if we asked for alpha, but no
  // penalty at all if there is alpha and we didn't ask for it.
  if(alphaSize>0){
    if(s.alphasize<1) match+=100000;
    }

  // Wanted Z-buffer
  if(depthSize>0){
    if(s.depthsize<1) match+=10000000;
    else match+=ddepth;
    }
  else{
    if(s.depthsize>0) match+=10000000;
    }

  // Stencil buffers desired
  if(stencilSize>0){
    if(s.stencilsize<1) match+=10000;
    else match+=dstencil;
    }
  else{
    if(s.stencilsize>0) match+=1;
    }

  // Multisamples
  if(multiSamples>0){
    dsamples=s.multisamples-multiSamples;
    if(dsamples<0) dsamples*=-10;
    match+=dsamples;
    }
  else{
    if(s.multisamples>0) match+=100;
    }

  // Double buffering also quite strongly preferred
  if(flags&VISUAL_DOUBLE_BUFFER){
    if(!s.doublebuffer) match+=1000000;
    }
  else{
    if(s.doublebuffer) match+=1000000;
    }

  // Stereo not so important
  if(flags&VISUAL_STEREO){
    if(!s.stereobuffer) match+=10000;
    }
  else{
    if(s.stereobuffer) match+=10000;
    }

  // Swap copies also important
  if(flags&VISUAL_SWAP_COPY){
    if(!s.copy) match+=10000000;
    }

  // Composition support would be nice to have
  if(!s.composition) match+=100;

  // Off-screen drawing would be nice
  if(!s.image) match+=1;

  return match;
  }


// Detach visual
void FXGLVisual::detach(){
#ifdef HAVE_GL_H
  if(xid){
    FXTRACE(TOPIC_CREATION,"%s::detach %p\n",getClassName(),this);
    colormap=0;
    freemap=false;
    xid=0;
    }
#endif
  }


// Save to stream
void FXGLVisual::save(FXStream& store) const {
  FXVisual::save(store);
  store << redSize;
  store << greenSize;
  store << blueSize;
  store << alphaSize;
  store << depthSize;
  store << stencilSize;
  store << multiSamples;
  store << accumRedSize;
  store << accumGreenSize;
  store << accumBlueSize;
  store << accumAlphaSize;
  }


// Load from stream
void FXGLVisual::load(FXStream& store){
  FXVisual::load(store);
  store >> redSize;
  store >> greenSize;
  store >> blueSize;
  store >> alphaSize;
  store >> depthSize;
  store >> stencilSize;
  store >> multiSamples;
  store >> accumRedSize;
  store >> accumGreenSize;
  store >> accumBlueSize;
  store >> accumAlphaSize;
  }


// Destroy
FXGLVisual::~FXGLVisual(){
  FXTRACE(TOPIC_CONSTRUCT,"FXGLVisual::~FXGLVisual %p\n",this);
  destroy();
  }


}
