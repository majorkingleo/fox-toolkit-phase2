/********************************************************************************
*                                                                               *
*                           S V G   I c o n   O b j e c t                       *
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
#ifndef FXSVGICON_H
#define FXSVGICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/// Scalable Vector Graphics (SVG) Icon class
class FXAPI FXSVGIcon : public FXIcon {
  FXDECLARE(FXSVGIcon)
protected:
  FXSVGIcon(){}
private:
  FXSVGIcon(const FXSVGIcon&);
  FXSVGIcon &operator=(const FXSVGIcon&);
public:
  static const FXchar fileExt[];
  static const FXchar mimeType[];
public:

  /// Construct an icon from memory stream formatted in SVG format
  FXSVGIcon(FXApp *a,const FXuchar *pix=nullptr,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// True if format is supported
  static const FXbool supported;

  /// Saving SVG is not supported
  virtual FXbool savePixels(FXStream& store) const override;

  /// Load pixels from stream in SVG format
  virtual FXbool loadPixels(FXStream& store) override;

  /// Destroy
  virtual ~FXSVGIcon();
  };

}

#endif
