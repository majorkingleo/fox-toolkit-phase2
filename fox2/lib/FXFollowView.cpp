/********************************************************************************
*                                                                               *
*                         F o l l o w   V i e w   S t a t e                      *
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
#include "FXFollowView.h"

#include <cmath>

using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {


// Construct default following view
FXFollowView::FXFollowView():windowlength(30.0),viewleft(0.0),latesttime(0.0),following(true),havetime(false){
  }


// Change visible window length
void FXFollowView::setWindowLength(FXdouble seconds){
  if(!Math::fpFinite(seconds)) seconds=30.0;
  windowlength=FXCLAMP(1.0,seconds,7200.0);
  if(following && havetime) viewleft=latesttime-windowlength;
  }


// Update latest time and follow it when enabled
void FXFollowView::update(FXdouble latestTime){
  if(!Math::fpFinite(latestTime)) return;
  latesttime=latestTime;
  havetime=true;
  if(following) viewleft=latesttime-windowlength;
  }


// Map data and view to scrollbar range, page, and position
void FXFollowView::getScrollBar(FXdouble dataStart,FXdouble latestTime,FXint& range,FXint& page,FXint& position) const {
  range=SCROLLBAR_RANGE;
  page=range;
  position=0;
  if(!Math::fpFinite(dataStart) || !Math::fpFinite(latestTime) || latestTime<=dataStart) return;
  FXdouble datarange=latestTime-dataStart;
  if(datarange<=windowlength) return;
  page=FXCLAMP(1,(FXint)std::floor(windowlength/datarange*range+0.5),range);
  FXint maxposition=range-page;
  if(maxposition<=0) return;
  FXdouble maxleft=latestTime-windowlength;
  FXdouble left=FXCLAMP(dataStart,viewleft,maxleft);
  FXdouble fraction=(left-dataStart)/(maxleft-dataStart);
  position=FXCLAMP(0,(FXint)std::floor(fraction*maxposition+0.5),maxposition);
  }


// Apply user scroll; reaching the far end resumes follow mode
void FXFollowView::scrollTo(FXint position,FXint range,FXdouble dataStart,FXdouble latestTime){
  if(!Math::fpFinite(dataStart) || !Math::fpFinite(latestTime)) return;
  update(latestTime);
  if(range<=0 || latestTime<=dataStart || latestTime-dataStart<=windowlength){
    following=true;
    viewleft=latestTime-windowlength;
    return;
    }
  FXint page=FXCLAMP(1,(FXint)std::floor(windowlength/(latestTime-dataStart)*range+0.5),range);
  FXint maxposition=range-page;
  if(maxposition<=0 || position>=maxposition){
    following=true;
    viewleft=latestTime-windowlength;
    return;
    }
  position=FXCLAMP(0,position,maxposition);
  FXdouble maxleft=latestTime-windowlength;
  viewleft=dataStart+(maxleft-dataStart)*(FXdouble)position/maxposition;
  following=false;
  }

}
