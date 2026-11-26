/********************************************************************************
*                                                                               *
*                  I n t e r a c t i v e   P l o t   W i d g e t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXINTERACTIVEPLOT_H
#define FXINTERACTIVEPLOT_H

// Interactive two-dimensional plot with pan, crosshair, and curve picking.
#ifndef FXCHARTDEFS_H
#include "FXChartDefs.h"
#endif
#ifndef FX2DPLOT_H
#include "FX2DPlot.h"
#endif

namespace FXCHART {


/// Interactive two-dimensional plot with curve picking and crosshair readout
class FXCHARTAPI FXInteractivePlot : public FX2DPlot {
  FXDECLARE(FXInteractivePlot)
protected:
  FXbool panmode;
  FXbool panning;
  FXbool cursorinside;
  FXint cursorx;
  FXint cursory;
  FXint pananchorx;
  FXint pananchory;
  FXdouble panrange[4];
protected:
  FXInteractivePlot();
private:
  FXInteractivePlot(const FXInteractivePlot&);
  FXInteractivePlot& operator=(const FXInteractivePlot&);
  void notifyCurvePicked(FXint curve);
public:
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  virtual void drawOverlay(FXDC& dc) const override;

  /// Construct interactive two-dimensional plot
  FXInteractivePlot(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=FRAME_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=4,FXint pr=4,FXint pt=4,FXint pb=4);

  /// Enable or disable drag-to-pan mode
  void setPanMode(FXbool mode);

  /// Return nearest visible curve within radius pixels, or -1; does not send a message
  /// A successful click sends SEL_CHANGED using the constructor selector and FXuval(curve index+1) as data
  FXint pickCurve(FXint x,FXint y,FXint radius=8) const;

  /// Destructor
  virtual ~FXInteractivePlot();
  };

}

#endif
