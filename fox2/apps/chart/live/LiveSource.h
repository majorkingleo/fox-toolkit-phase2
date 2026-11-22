/********************************************************************************
*                                                                               *
*                       L i v e   D a t a   S o u r c e                         *
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
#ifndef LIVESOURCE_H
#define LIVESOURCE_H

#ifndef CHARTDEFS_H
#include "../chartdefs.h"
#endif

namespace FXCHART {


class FX2DPlot;
class FXCurve;
class FXData;


/// Feed mouse and system values into a chart on a timer
class FXCHARTAPI LiveSource : public FXObject {
  FXDECLARE(LiveSource)
public:
  enum {
    ID_TIMEOUT=1,
    ID_LAST
    };
  enum {
    SERIES_SPEED,               /// Mouse speed
    SERIES_ACCEL,               /// Mouse acceleration
    SERIES_CPU,                 /// CPU load
    SERIES_MEMORY,              /// Resident memory
    SERIES_FRAME,               /// Chart draw time
    SERIES_COUNT
    };
protected:
  FX2DPlot *plot;               // Chart to feed
  FXCurve  *curves[SERIES_COUNT];       // Live curves
  FXData   *timedata[SERIES_COUNT];     // Time samples
  FXData   *values[SERIES_COUNT];       // Value samples
  FXTime    interval;           // Sampling interval, nanoseconds
  FXTime    starttime;          // Time of first sample
  FXTime    lasttime;           // Time of previous sample
  FXdouble  window;             // Visible history, seconds
  FXdouble  lastspeed;          // Previous mouse speed
  FXint     lastx;              // Previous pointer x
  FXint     lasty;              // Previous pointer y
  FXbool    lastvalid;          // Previous pointer position valid
  FXulong   lastcpu;            // Previous CPU total
  FXulong   lastidle;           // Previous CPU idle
  FXbool    running;            // Timer is running
protected:
  LiveSource();
  void addCurve(FXint index,const FXString& label,FXColor color,FXuchar axis);
  void showCurves(FXbool show);
  void samplePointer(FXdouble dt,FXdouble& speed,FXdouble& accel);
  FXdouble sampleCPU();
  FXdouble sampleMemory();
private:
  LiveSource(const LiveSource&);
  LiveSource& operator=(const LiveSource&);
public:

  /// Construct a live source feeding plot
  LiveSource(FX2DPlot* p,FXTime ns=50000000);

  /// Start sampling
  void start();

  /// Stop sampling
  void stop();

  /// Clear samples and restart the time base
  void clear();

  /// Change sampling interval in nanoseconds
  void setInterval(FXTime ns);

  /// Return sampling interval in nanoseconds
  FXTime getInterval() const { return interval; }

  /// Change visible history in seconds
  void setWindow(FXdouble seconds);

  /// Return visible history in seconds
  FXdouble getWindow() const { return window; }

  /// Return true if the timer is running
  FXbool isRunning() const { return running; }

  /// Timer callback
  long onTimeout(FXObject*,FXSelector,void*);

  /// Destructor
  virtual ~LiveSource();
  };

}

#endif
