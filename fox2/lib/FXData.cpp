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
#include "fx.h"
#include "FXChartDefs.h"
#include "FXData.h"

/*
  Notes:

  - FXData stores one channel.  A curve owns an x channel and a y channel.
  - With a limit set, the array is a ring: appending beyond the limit drops
    the oldest sample in O(1), which is what streaming charts need.
*/

using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {


// Construct empty sample array
FXData::FXData():data(nullptr),size(0),capacity(0),limit(0),head(0),minimum(0.0),maximum(0.0),valid(false){
  }


// Grow storage to at least n slots
void FXData::grow(FXint n){
  FXint newcap;
  FXint i;
  if(n<=capacity) return;
  newcap=capacity?capacity:16;
  while(newcap<n) newcap*=2;
  if(limit && newcap>limit) newcap=limit;
  FXdouble *nd=new FXdouble[newcap];
  for(i=0; i<size; i++){
    nd[i]=data[(head+i)%capacity];
    }
  delete [] data;
  data=nd;
  capacity=newcap;
  head=0;
  }


// Move ring contents to the start of the array
void FXData::normalize(){
  FXint i;
  if(head==0) return;
  FXdouble *nd=new FXdouble[capacity];
  for(i=0; i<size; i++){
    nd[i]=data[(head+i)%capacity];
    }
  delete [] data;
  data=nd;
  head=0;
  }


// Update cached minimum and maximum
void FXData::updateRange(FXdouble value){
  if(!Math::fpFinite(value)) return;
  if(!valid){
    minimum=value;
    maximum=value;
    valid=true;
    }
  else{
    if(value<minimum) minimum=value;
    if(value>maximum) maximum=value;
    }
  }


// Recompute cached minimum and maximum over finite samples
void FXData::rebuildRange(){
  FXint i;
  valid=false;
  if(size==0){
    minimum=maximum=0.0;
    return;
    }
  for(i=0; i<size; i++) updateRange(get(i));
  if(!valid) minimum=maximum=0.0;
  }


// Set ring capacity; zero means unlimited
void FXData::setLimit(FXint n){
  FXint i;
  FXint keep;
  FXint start;
  if(n<=0){
    if(limit){
      normalize();
      limit=0;
      }
    return;
    }
  if(n==limit) return;
  FXdouble *nd=new FXdouble[n];
  keep=FXMIN(size,n);
  start=size-keep;
  for(i=0; i<keep; i++){
    nd[i]=data[(head+start+i)%capacity];
    }
  delete [] data;
  data=nd;
  capacity=n;
  size=keep;
  limit=n;
  head=0;
  if(start) valid=false;
  }


// Remove all samples
void FXData::clear(){
  size=0;
  head=0;
  minimum=maximum=0.0;
  valid=false;
  }


// Reserve room for at least n samples
void FXData::reserve(FXint n){
  grow(n);
  }


// Append one sample
void FXData::append(FXdouble value){
  if(limit==0){
    if(size==capacity) grow(size+1);
    data[size++]=value;
    updateRange(value);
    }
  else{
    if(size<capacity){
      data[(head+size)%capacity]=value;
      size++;
      updateRange(value);
      }
    else{
      FXdouble old=data[head];
      data[head]=value;
      head=(head+1)%capacity;
      if(valid && (old<=minimum || old>=maximum)){
        valid=false;
        }
      if(valid) updateRange(value);
      }
    }
  }


// Append a block of samples
void FXData::append(const FXdouble* values,FXint count){
  FXint i;
  if(count<=0) return;
  if(limit==0 && size+count<=capacity){
    for(i=0; i<count; i++){
      data[size++]=values[i];
      updateRange(values[i]);
      }
    return;
    }
  for(i=0; i<count; i++){
    append(values[i]);
    }
  }


// Replace samples with a contiguous block
void FXData::assign(const FXdouble* values,FXint count){
  FXint i;
  FXdouble lo=0.0,hi=0.0;
  FXbool any=false;
  if(count<0) count=0;
  if(limit){
    clear();
    for(i=0; i<count; i++) append(values[i]);
    return;
    }
  if(count>capacity) grow(count);
  for(i=0; i<count; i++){
    FXdouble value=values[i];
    data[i]=value;
    if(Math::fpFinite(value)){
      if(!any){ lo=hi=value; any=true; }
      else{
        if(value<lo) lo=value;
        if(value>hi) hi=value;
        }
      }
    }
  size=count;
  head=0;
  minimum=lo;
  maximum=hi;
  valid=any;
  }


// Remove count samples starting at index
void FXData::remove(FXint index,FXint count){
  FXint i;
  if(index<0 || index>size) fxerror("FXData::remove: bad index.\n");
  if(count<0) count=0;
  if(count>size-index) count=size-index;
  for(i=index; i+count<size; i++){
    data[(head+i)%capacity]=data[(head+i+count)%capacity];
    }
  size-=count;
  valid=false;
  }


// Return sample at index
FXdouble FXData::get(FXint index) const {
  if(index<0 || index>=size) fxerror("FXData::get: bad index.\n");
  return data[(head+index)%capacity];
  }


// Change sample at index
void FXData::set(FXint index,FXdouble value){
  if(index<0 || index>=size) fxerror("FXData::set: bad index.\n");
  data[(head+index)%capacity]=value;
  valid=false;
  }


// Return smallest sample
FXdouble FXData::getMinimum() const {
  if(!valid) const_cast<FXData*>(this)->rebuildRange();
  return minimum;
  }


// Return largest sample
FXdouble FXData::getMaximum() const {
  if(!valid) const_cast<FXData*>(this)->rebuildRange();
  return maximum;
  }


// Return finite minimum and maximum; false when no finite samples exist
FXbool FXData::getRange(FXdouble& lo,FXdouble& hi) const {
  if(!valid) const_cast<FXData*>(this)->rebuildRange();
  if(!valid) return false;
  lo=minimum;
  hi=maximum;
  return true;
  }


// Return index of sample at or before x
FXint FXData::findIndexAt(FXdouble x) const {
  FXint lo=0;
  FXint hi=size-1;
  FXint mid;
  if(size==0) return -1;
  if(x<=get(0)) return 0;
  if(x>=get(hi)) return hi;
  while(lo<hi){
    mid=(lo+hi+1)/2;
    if(get(mid)<=x) lo=mid;
    else hi=mid-1;
    }
  return lo;
  }


// Destroy
FXData::~FXData(){
  delete [] data;
  }

}

