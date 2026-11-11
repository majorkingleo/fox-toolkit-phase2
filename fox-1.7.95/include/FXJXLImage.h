/********************************************************************************
*                                                                               *
*                     J P E G - X L   I m a g e   O b j e c t                   *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXJXLIMAGE_H
#define FXJXLIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {


/// JPEG-XL Image class
class FXAPI FXJXLImage : public FXImage {
  FXDECLARE(FXJXLImage)
protected:
  FXJXLImage(){}
private:
  FXJXLImage(const FXJXLImage&);
  FXJXLImage &operator=(const FXJXLImage&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an image from memory stream formatted in JPEG format
  FXJXLImage(FXApp *a,const FXuchar *pix=nullptr,FXuint opts=0,FXint w=1,FXint h=1);

  /// True if format is supported
  static const FXbool supported;

  /// Load pixels from stream in JPEG format
  virtual FXbool savePixels(FXStream& store) const override;

  /// Save pixels into stream in JPEG format
  virtual FXbool loadPixels(FXStream& store) override;

  /// Destroy
  virtual ~FXJXLImage();
  };


#ifndef FXLOADJXL
#define FXLOADJXL

/**
* Check if stream contains a JPEG-XL, return true if so.
*/
extern FXAPI FXbool fxcheckJXL(FXStream& store);


/**
* Load an JPEG-XL file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadJXL(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an JPEG-XL file to a stream.
*/
extern FXAPI FXbool fxsaveJXL(FXStream& store,const FXColor* data,FXint width,FXint height);

#endif

}

#endif
