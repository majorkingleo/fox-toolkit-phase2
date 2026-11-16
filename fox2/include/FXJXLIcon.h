/********************************************************************************
*                                                                               *
*                       J P E G - X L   I c o n   O b j e c t                   *
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
#ifndef FXJXLICON_H
#define FXJXLICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/// JPEG-XL Icon class
class FXAPI FXJXLIcon : public FXIcon {
  FXDECLARE(FXJXLIcon)
protected:
  FXJXLIcon(){}
private:
  FXJXLIcon(const FXJXLIcon&);
  FXJXLIcon &operator=(const FXJXLIcon&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an icon from memory stream formatted in JPEG format
  FXJXLIcon(FXApp *a,const FXuchar *pix=nullptr,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// True if format is supported
  static const FXbool supported;

  /// Save pixels into stream in JPEG format
  virtual FXbool savePixels(FXStream& store) const override;

  /// Load pixels from stream in JPEG format
  virtual FXbool loadPixels(FXStream& store) override;

  /// Destroy
  virtual ~FXJXLIcon();
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
