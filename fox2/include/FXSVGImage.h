/********************************************************************************
*                                                                               *
*                          S V G   I m a g e   O b j e c t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#ifndef FXSVGIMAGE_H
#define FXSVGIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {


/// Scalable Vector Graphics (SVG) Image class
class FXAPI FXSVGImage : public FXImage {
  FXDECLARE(FXSVGImage)
protected:
  FXSVGImage(){}
private:
  FXSVGImage(const FXSVGImage&);
  FXSVGImage &operator=(const FXSVGImage&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an image from memory stream formatted in SVG format
  FXSVGImage(FXApp *a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1);

  /// True if format is supported
  static const FXbool supported;

  /// Saving SVG is not supported
  virtual FXbool savePixels(FXStream& store) const override;

  /// Load pixels from stream in SVG format
  virtual FXbool loadPixels(FXStream& store) override;

  /// Destroy
  virtual ~FXSVGImage();
  };



/**
* Check if stream contains an SVG, return true if so.
*/
extern FXAPI FXbool fxcheckSVG(FXStream& store);


/**
* Load an SVG (Scalable Vector Graphics) file from a stream, rasterized at the
* given size; pass 0 for a size to use the natural size, or to derive it from
* the other dimension.  Upon successful return, the pixel array and size are
* returned.  If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadSVG(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint w=0,FXint h=0);

}

#endif
