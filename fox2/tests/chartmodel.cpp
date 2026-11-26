// No-display tests for chart curve scaling and axis label formatting.
#include "fx.h"
#include "FXChartDefs.h"
#include "FXAxis.h"
#include "FXCurve.h"
#include "FXChartUtils.h"
#include "FXData.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

using namespace FXCHART;

namespace {

bool scaledRangeSupportsPositiveScaleAndOffset(){
  FXCurve curve(nullptr);
  auto data=std::make_shared<FXData>();
  data->append(1.0);
  data->append(3.0);
  curve.setYData(data);
  curve.setScale(2.0,1.0);
  FXdouble lo=0.0,hi=0.0;
  return curve.getScale()==2.0 && curve.getYRange(lo,hi) && lo==3.0 && hi==7.0;
  }

bool scaledRangeReordersNegativeScale(){
  FXCurve curve(nullptr);
  auto data=std::make_shared<FXData>();
  data->append(1.0);
  data->append(3.0);
  curve.setYData(data);
  curve.setScale(-2.0,3.0);
  FXdouble lo=0.0,hi=0.0;
  return curve.getYRange(lo,hi) && lo==-3.0 && hi==1.0;
  }

bool emptyAndDefaultRangesArePreserved(){
  FXCurve curve(nullptr);
  FXdouble lo=0.0,hi=0.0;
  if(curve.getScale()!=1.0 || curve.getOffset()!=0.0 || curve.getYRange(lo,hi)) return false;
  auto data=std::make_shared<FXData>();
  data->append(-4.0);
  data->append(2.0);
  curve.setYData(data);
  return curve.getYRange(lo,hi) && lo==-4.0 && hi==2.0;
  }

bool customFormatterReceivesTickValue(){
  FXdouble seen=0.0;
  FXint calls=0;
  const std::function<FXString(FXdouble)> formatter=[&](FXdouble value){
    seen=value;
    ++calls;
    return FXString("custom");
    };
  const FXString text=FXAxis::formatLabel(12.5,0.5,formatter);
  return text=="custom" && calls==1 && seen==12.5;
  }

bool emptyFormatterUsesDefault(){
  return FXAxis::formatLabel(2.5,0.5,{})=="2.5";
  }

bool pickNearestFindsClosestPointOnSegment(){
  FXPointF points[3];
  points[0].x=0.0f; points[0].y=0.0f;
  points[1].x=10.0f; points[1].y=0.0f;
  points[2].x=10.0f; points[2].y=10.0f;
  FXdouble distance=-1.0;
  FXint index=pickNearest(points,3,5,1,1,&distance);
  return index==0 && distance==1.0;
  }

bool pickNearestFindsSinglePointAndHonorsRadius(){
  FXPointF point;
  point.x=4.0f;
  point.y=7.0f;
  FXdouble distance=-1.0;
  if(pickNearest(&point,1,7,11,5,&distance)!=0 || distance!=25.0) return false;
  if(pickNearest(&point,1,7,11,4,&distance)>=0) return false;
  return pickNearest(nullptr,0,0,0,8)==-1 && pickNearest(&point,1,0,0,-1)==-1;
  }

bool pickNearestDoesNotBridgeNonFiniteGaps(){
  FXPointF points[3];
  points[0].x=0.0f; points[0].y=0.0f;
  points[1].x=std::numeric_limits<FXfloat>::quiet_NaN();
  points[1].y=std::numeric_limits<FXfloat>::quiet_NaN();
  points[2].x=10.0f; points[2].y=0.0f;
  return pickNearest(points,3,5,0,1)==-1;
  }

}

int main(){
  struct Test { const char* name; bool (*run)(); };
  const Test tests[]={
    {"scaled range with offset",scaledRangeSupportsPositiveScaleAndOffset},
    {"negative scale range ordering",scaledRangeReordersNegativeScale},
    {"default and empty ranges",emptyAndDefaultRangesArePreserved},
    {"custom tick label formatter",customFormatterReceivesTickValue},
    {"default tick label formatter",emptyFormatterUsesDefault},
    {"curve picking segment distance",pickNearestFindsClosestPointOnSegment},
    {"curve picking point and radius",pickNearestFindsSinglePointAndHonorsRadius},
    {"curve picking respects non-finite gaps",pickNearestDoesNotBridgeNonFiniteGaps}
    };
  int failures=0;
  for(const Test& test : tests){
    if(test.run()) std::cout << "PASS " << test.name << '\n';
    else{
      std::cerr << "FAIL " << test.name << '\n';
      ++failures;
      }
    }
  return failures==0 ? 0 : 1;
  }