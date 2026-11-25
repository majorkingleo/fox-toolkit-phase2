/********************************************************************************
*                                                                               *
*                          C u r v e   D a t a   S a m p l e s                  *
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
#ifndef FXDATA_H
#define FXDATA_H

namespace FXCHART {


/// Storage for one channel of curve samples
class FXCHARTAPI FXData {
protected:
  FXdouble *data;               // Sample values
  FXint     size;               // Number of samples
  FXint     capacity;           // Allocated slots
  FXint     limit;              // Ring capacity, zero for unlimited
  FXint     head;               // First sample in ring
  FXdouble  minimum;            // Cached minimum
  FXdouble  maximum;            // Cached maximum
  FXbool    valid;              // Cached range is valid
protected:
  void grow(FXint n);
  void normalize();
  void updateRange(FXdouble value);
  void rebuildRange();
private:
  FXData(const FXData&);
  FXData& operator=(const FXData&);
public:

  /// Construct empty sample array
  FXData();

  /// Return number of samples
  FXint no() const { return size; }

  /// Return true if there are no samples
  FXbool empty() const { return size==0; }

  /// Return ring capacity, or zero when unlimited
  FXint getLimit() const { return limit; }

  /// Set ring capacity; zero means unlimited
  void setLimit(FXint n);

  /// Remove all samples
  void clear();

  /// Reserve room for at least n samples
  void reserve(FXint n);

  /// Append one sample
  void append(FXdouble value);

  /// Append count samples
  void append(const FXdouble* values,FXint count);

  /// Remove count samples starting at index
  void remove(FXint index,FXint count=1);

  /// Return sample at index
  FXdouble get(FXint index) const;

  /// Return sample at index
  FXdouble operator[](FXint index) const { return get(index); }

  /// Change sample at index
  void set(FXint index,FXdouble value);

  /// Return smallest sample, or zero when empty
  FXdouble getMinimum() const;

  /// Return largest sample, or zero when empty
  FXdouble getMaximum() const;

  /// Return finite minimum and maximum; false when no finite samples exist
  FXbool getRange(FXdouble& lo,FXdouble& hi) const;

  /// Return index of sample at or before x; samples must be sorted
  FXint findIndexAt(FXdouble x) const;

  /// Destructor
  ~FXData();
  };

}

#endif
