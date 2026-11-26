// No-display tests for chart curve scaling and axis label formatting.
#include "fx.h"
#include "FXChartDefs.h"
#include "FXAxis.h"
#include "FXCurve.h"
#include "FXData.h"

#include <cmath>
#include <functional>
#include <iostream>
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

}

int main(){
  struct Test { const char* name; bool (*run)(); };
  const Test tests[]={
    {"scaled range with offset",scaledRangeSupportsPositiveScaleAndOffset},
    {"negative scale range ordering",scaledRangeReordersNegativeScale},
    {"default and empty ranges",emptyAndDefaultRangesArePreserved},
    {"custom tick label formatter",customFormatterReceivesTickValue},
    {"default tick label formatter",emptyFormatterUsesDefault}
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