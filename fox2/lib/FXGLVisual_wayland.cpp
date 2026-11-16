/********************************************************************************
*                                                                               *
*                     G L   V i s u a l   ( W a y l a n d )                     *
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

#if defined(FOX_WAYLAND)

/*
  Notes:

  - OpenGL is not supported by the Wayland platform yet (it needs EGL), so no
    GL visual can be created.
*/

#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {

// Test if OpenGL is possible
FXbool FXGLVisual::hasOpenGL(FXApp*){
  return false;
  }


// Initialize
void FXGLVisual::create(){
  throw FXWindowException("OpenGL is not supported on Wayland.");
  }


// Destroy visual
void FXGLVisual::destroy(){
  xid=0;
  }

}

#endif
