/********************************************************************************
*                                                                               *
*        P r i v a t e   G L   V i s u a l   S e l e c t i o n   D a t a        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#ifndef FXGLPRIV_H
#define FXGLPRIV_H

/*
  Notes:

  - Shared by FXGLVisual.cpp and its platform files.
*/

namespace FX {

// Describes particular FB configuration
struct FXGLVisual::FXGLSpecs {
  int redsize;
  int greensize;
  int bluesize;
  int alphasize;
  int depthsize;
  int stencilsize;
  int accumredsize;
  int accumgreensize;
  int accumbluesize;
  int accumalphasize;
  int doublebuffer;
  int stereobuffer;
  int multisamples;
  int composition;
  int image;
  int accel;
  int copy;
  };

}

#endif
