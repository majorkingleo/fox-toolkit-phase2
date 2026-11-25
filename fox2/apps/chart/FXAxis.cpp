/********************************************************************************
*                                                                               *
*                          A x i s - D e f i n i t i o n                        *
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
#include "chartdefs.h"
#include "chartutils.h"
#include "FXAxis.h"
#include "FXChart.h"

/*
  Notes:
*/

#define FUDGE_FACTOR  0.001     // For tickmark computation


using namespace FXCHART;


/*******************************************************************************/

namespace FXCHART {


// Construct axis
FXAxis::FXAxis(FXChart* ch,const FXString& nm,FXuint lblsty,FXuint numsty,FXuint sty):chart(ch),label(nm),labelstyle(lblsty),numberstyle(numsty),style(sty){
  labelfont=chart->getApp()->getNormalFont();
  numberfont=chart->getApp()->getNormalFont();
  datarange.minimum=0.0;
  datarange.maximum=1.0;
  axisrange.minimum=0.0;
  axisrange.maximum=1.0;
  majorspace=0.1;
  minorspace=0.01;
  labelcolor=chart->getApp()->getForeColor();
  numbercolor=chart->getApp()->getForeColor();
  axiscolor=chart->getApp()->getForeColor();
  majortickcolor=chart->getApp()->getForeColor();
  minortickcolor=chart->getApp()->getForeColor();
  majorgridcolor=contrastColor(chart->getApp()->getBaseColor(),20);
  minorgridcolor=contrastColor(chart->getApp()->getBaseColor(),10);
  majorticksize=7;
  minorticksize=4;
  numberoffset=3;
  labeloffset=6;
  precision=3;
  fadeskip=false;
  fadehold=0;
  fadeon=true;
  }


// Create server-side resources
void FXAxis::create(){
  labelfont->create();
  numberfont->create();
  }


// Detach server-side resources
void FXAxis::detach(){
  labelfont->detach();
  numberfont->detach();
  }


// Return true if the list already holds a number with this text
static FXbool hasNumber(const FXArray<AxisNumber>& list,const FXString& text){
  FXint i;
  for(i=0; i<list.no(); i++){
    if(list[i].text==text) return true;
    }
  return false;
  }


// Format tick values with the precision implied by the major tick spacing
static FXString formatAxisNumber(FXdouble value,FXdouble spacing){
  FXString text;
  FXint decimals=0;
  FXdouble scaled=spacing;
  if(!Math::fpFinite(value) || !Math::fpFinite(spacing) || spacing<=0.0 || FXABS(value)>=1.0e9 || (value!=0.0 && FXABS(value)<1.0e-6)){
    text.format("%g",value);
    return text;
    }
  while(decimals<8 && FXABS(scaled-Math::round(scaled))>1.0e-7*FXMAX(1.0,FXABS(scaled))){
    scaled*=10.0;
    decimals++;
    }
  if(decimals==8 && FXABS(scaled-Math::round(scaled))>1.0e-7*FXMAX(1.0,FXABS(scaled))){
    text.format("%g",value);
    return text;
    }
  text.format("%.*f",decimals,value);
  return text;
  }


// Drop horizontal labels that do not fit beside the preceding label
static void fitHorizontalNumbers(FXArray<AxisNumber>& numbers,FXFont* font,FXuint style,FXint fx,FXint tx){
  FXint direction=(tx>=fx)?1:-1;
  FXint left,right,boundary=fx;
  FXival kept=0;
  for(FXival i=0; i<numbers.no(); i++){
    FXint width=textWidth(font,numbers[i].text);
    if(style&TEXT_ATTACH_LEFT){
      left=numbers[i].x;
      right=left+width;
      }
    else if(style&TEXT_ATTACH_RIGHT){
      right=numbers[i].x;
      left=right-width;
      }
    else{
      left=numbers[i].x-width/2;
      right=left+width;
      }
    if(left<FXMIN(fx,tx) || right>FXMAX(fx,tx)) continue;
    if(kept && (direction>0?left<boundary+2:right>boundary-2)) continue;
    if(kept) numbers[kept]=numbers[i];
    kept++;
    boundary=direction>0?right:left;
    }
  numbers.no(kept);
  }


// Drop vertical labels that do not fit beside the preceding label
static void fitVerticalNumbers(FXArray<AxisNumber>& numbers,FXFont* font,FXuint style,FXint fy,FXint ty){
  FXint direction=(ty>=fy)?1:-1;
  FXint top,bottom,boundary=fy;
  FXival kept=0;
  for(FXival i=0; i<numbers.no(); i++){
    FXint height=textHeight(font,numbers[i].text);
    if(style&TEXT_ATTACH_TOP){
      top=numbers[i].y;
      bottom=top+height;
      }
    else if(style&TEXT_ATTACH_BOTTOM){
      bottom=numbers[i].y;
      top=bottom-height;
      }
    else{
      top=numbers[i].y-height/2;
      bottom=top+height;
      }
    if(top<FXMIN(fy,ty) || bottom>FXMAX(fy,ty)) continue;
    if(kept && (direction>0?top<boundary+2:bottom>boundary-2)) continue;
    if(kept) numbers[kept]=numbers[i];
    kept++;
    boundary=direction>0?bottom:top;
    }
  numbers.no(kept);
  }


// Draw axis
void FXAxis::drawAxis(FXDC& dc,FXint fx,FXint fy,FXint tx,FXint ty,FXint dx,FXint dy) const {
  if(style&(AXIS_LABEL|AXIS_NUMBERS|AXIS_GRID_MAJOR|AXIS_GRID_MINOR|AXIS_TICKS_MAJOR|AXIS_TICKS_MINOR|AXIS_LINE)){
    FXint ex,ey,tin,tout,x,y,majh,majl,minh,minl,i,j;
    FXdouble mulx,muly;

    // Unit direction
    ex=0; if(dx) ex=dx/FXABS(dx);
    ey=0; if(dy) ey=dy/FXABS(dy);

    // Multipliers
    mulx=((FXdouble)(tx-fx))/(axisrange.maximum-axisrange.minimum);
    muly=((FXdouble)(ty-fy))/(axisrange.maximum-axisrange.minimum);

    // Minor grid lines
    if(style&AXIS_GRID_MINOR){
      dc.setForeground(minorgridcolor);
      minl=(FXint)Math::ceil((axisrange.minimum+FUDGE_FACTOR*minorspace)/minorspace);
      minh=(FXint)Math::floor((axisrange.maximum-FUDGE_FACTOR*minorspace)/minorspace);
      for(i=minl; i<=minh; i++){
        x=fx+Math::lrint(mulx*(i*minorspace-axisrange.minimum));
        y=fy+Math::lrint(muly*(i*minorspace-axisrange.minimum));
        dc.drawLine(x,y,x+dx,y+dy);
        }
      }

    // Major grid lines
    if(style&AXIS_GRID_MAJOR){
      dc.setForeground(majorgridcolor);
      majl=(FXint)Math::ceil((axisrange.minimum+FUDGE_FACTOR*majorspace)/majorspace);
      majh=(FXint)Math::floor((axisrange.maximum-FUDGE_FACTOR*majorspace)/majorspace);
      for(i=majl; i<=majh; i++){
        x=fx+Math::lrint(mulx*(i*majorspace-axisrange.minimum));
        y=fy+Math::lrint(muly*(i*majorspace-axisrange.minimum));
        dc.drawLine(x,y,x+dx,y+dy);
        }
      }

    // Axis line
    if(style&AXIS_LINE){
      dc.setForeground(axiscolor);
      dc.drawLine(fx,fy,tx,ty);
      }

    tin=tout=0;

    // Minor ticks
    if(style&AXIS_TICKS_MINOR){
      if(style&AXIS_TICKS_INSIDE) tin=minorticksize;
      if(style&AXIS_TICKS_OUTSIDE) tout=minorticksize;
      dc.setForeground(minortickcolor);
      minl=(FXint)Math::ceil(axisrange.minimum/minorspace);
      minh=(FXint)Math::floor(axisrange.maximum/minorspace);
      for(i=minl; i<=minh; i++){
        x=fx+Math::lrint(mulx*(i*minorspace-axisrange.minimum));
        y=fy+Math::lrint(muly*(i*minorspace-axisrange.minimum));
        dc.drawLine(x-tout*ex,y-tout*ey,x+tin*ex,y+tin*ey);
        }
      }

    // Major ticks
    if(style&AXIS_TICKS_MAJOR){
      if(style&AXIS_TICKS_INSIDE) tin=majorticksize;
      if(style&AXIS_TICKS_OUTSIDE) tout=majorticksize;
      dc.setForeground(majortickcolor);
      majl=(FXint)Math::ceil(axisrange.minimum/majorspace);
      majh=(FXint)Math::floor(axisrange.maximum/majorspace);
      for(i=majl; i<=majh; i++){
        x=fx+Math::lrint(mulx*(i*majorspace-axisrange.minimum));
        y=fy+Math::lrint(muly*(i*majorspace-axisrange.minimum));
        dc.drawLine(x-tout*ex,y-tout*ey,x+tin*ex,y+tin*ey);
        }
      }

    // Numbers; a number that appears fades in and one that goes fades out,
    // each on its own clock so a change does not disturb the others
    if(style&AXIS_NUMBERS){
      FXTime now=FXThread::steadytime();
      FXTime fadetime=chart->getFadeTime();
      FXdouble alpha;
      FXbool animated=false;
      FXint entering=0;
      FXint leaving=0;
      AxisNumber item;
      dc.setFont(numberfont);
      curnums.clear();
      if(style&AXIS_NUMBERS_INSIDE){
        tin+=numberoffset;
        majl=(FXint)Math::ceil((axisrange.minimum+FUDGE_FACTOR*majorspace)/majorspace);
        majh=(FXint)Math::floor((axisrange.maximum-FUDGE_FACTOR*majorspace)/majorspace);
        for(i=majl; i<=majh; i++){
          x=fx+Math::lrint(mulx*(i*majorspace-axisrange.minimum));
          y=fy+Math::lrint(muly*(i*majorspace-axisrange.minimum));
          item.text=formatAxisNumber(i*majorspace,majorspace);
          item.x=x+tin*ex;
          item.y=y+tin*ey;
          item.born=now;
          item.died=0;
          curnums.append(item);
          }
        }
      else{
        tout+=numberoffset;
        majl=(FXint)Math::ceil(axisrange.minimum/majorspace);
        majh=(FXint)Math::floor(axisrange.maximum/majorspace);
        for(i=majl; i<=majh; i++){
          x=fx+Math::lrint(mulx*(i*majorspace-axisrange.minimum));
          y=fy+Math::lrint(muly*(i*majorspace-axisrange.minimum));
          item.text=formatAxisNumber(i*majorspace,majorspace);
          item.x=x-tout*ex;
          item.y=y-tout*ey;
          item.born=now;
          item.died=0;
          curnums.append(item);
          }
        if(numberstyle&(TEXT_ATTACH_TOP|TEXT_ATTACH_BOTTOM)){
          tout+=textHeight(numberfont,"8.8");
          }
        else if(numberstyle&(TEXT_ATTACH_LEFT|TEXT_ATTACH_RIGHT)){
          tout+=textWidth(numberfont,"8.8");
          }
        }

      // Keep labels from overlapping as the axis is zoomed or resized
      if(dx==0 && curnums.no()>1) fitHorizontalNumbers(curnums,numberfont,numberstyle,fx,tx);
      else if(dy==0 && curnums.no()>1) fitVerticalNumbers(curnums,numberfont,numberstyle,fy,ty);

      // Count the numbers that come and go
      for(i=0; i<curnums.no(); i++){
        if(!hasNumber(prevnums,curnums[i].text)) entering++;
        }
      for(j=0; j<prevnums.no(); j++){
        if(!hasNumber(curnums,prevnums[j].text)) leaving++;
        }

      // A first draw, a zoom or a resize shows the numbers at once, and so
      // does a change too large or too quick to fade: a dragged viewport
      // moves the numbers faster than one can fade out
      if(fadeskip || prevnums.no()==0 || entering>1 || leaving>1 || ((entering || leaving) && (now<fadehold || oldnums.no()))){
        oldnums.clear();
        for(i=0; i<curnums.no(); i++){
          curnums[i].born=now-fadetime;
          }
        fadehold=now+fadetime;
        }
      else{

        // Numbers that stayed keep the time they appeared; numbers that went
        // away start fading out where they were
        for(i=0; i<curnums.no(); i++){
          for(j=0; j<prevnums.no(); j++){
            if(prevnums[j].text==curnums[i].text){
              curnums[i].born=prevnums[j].born;
              break;
              }
            }
          }
        for(j=0; j<prevnums.no(); j++){
          if(!hasNumber(curnums,prevnums[j].text) && !hasNumber(oldnums,prevnums[j].text)){
            item=prevnums[j];
            item.died=now;
            oldnums.append(item);
            }
          }
        }
      fadeskip=false;

      // Drop the numbers that faded out completely
      for(j=oldnums.no()-1; j>=0; j--){
        if((now-oldnums[j].died)>=fadetime) oldnums.erase(j);
        }

      // Fade the numbers that went away out against the chart background
      for(j=0; j<oldnums.no(); j++){
        alpha=1.0-(FXdouble)(now-oldnums[j].died)/(FXdouble)fadetime;
        dc.setForeground(makeBlendColor(numbercolor,chart->backColorAt(oldnums[j].x,oldnums[j].y),(FXint)(100.0*(1.0-alpha))));
        drawText(dc,oldnums[j].text,oldnums[j].x,oldnums[j].y,numberstyle);
        }

      // Draw the numbers, fading in the ones that just appeared
      for(i=0; i<curnums.no(); i++){
        alpha=(FXdouble)(now-curnums[i].born)/(FXdouble)fadetime;
        if(alpha<1.0){
          animated=true;
          dc.setForeground(makeBlendColor(chart->backColorAt(curnums[i].x,curnums[i].y),numbercolor,(FXint)(100.0*alpha)));
          }
        else{
          dc.setForeground(numbercolor);
          }
        drawText(dc,curnums[i].text,curnums[i].x,curnums[i].y,numberstyle);
        }

      // Keep the numbers drawn now for the next comparison
      prevnums=curnums;

      // Ask for repaints while anything is still on its way in or out
      if(animated || oldnums.no()) chart->startFade();
      }

    // Axis label
    if(style&AXIS_LABEL){
      dc.setFont(labelfont);
      dc.setForeground(labelcolor);
      tout+=labeloffset;

      // Horizontal attachment
      if(labelstyle&TEXT_ATTACH_LEFT) x=fx;
      else if(labelstyle&TEXT_ATTACH_RIGHT) x=tx;
      else x=(fx+tx)/2;

      // Vertical attachment
      if(labelstyle&TEXT_ATTACH_TOP) y=fy;
      else if(labelstyle&TEXT_ATTACH_BOTTOM) y=ty;
      else y=(fy+ty)/2;

//dc.drawLine(x-tout*ex-4,y-tout*ey,x-tout*ex+4,y-tout*ey);
//dc.drawLine(x-tout*ex,y-tout*ey-4,x-tout*ex,y-tout*ey+4);

      drawText(dc,label,x-tout*ex,y-tout*ey,labelstyle);
      }
    }
  }


// Set axis style
void FXAxis::setStyle(FXuint sty){
  if(style!=sty){
    style=sty;
    chart->recalc();
    }
  }


// Change axis label
void FXAxis::setLabel(const FXString& text){
  if(label!=text){
    label=text;
    chart->recalc();
    }
  }


// Change label text style
void FXAxis::setLabelStyle(FXuint sty){
  if(labelstyle!=sty){
    labelstyle=sty;
    chart->recalc();
    }
  }


// Change label text color
void FXAxis::setLabelColor(FXColor clr){
  if(labelcolor!=clr){
    labelcolor=clr;
    chart->recalc();
    }
  }


// Change axis line color
void FXAxis::setAxisColor(FXColor clr){
  if(axiscolor!=clr){
    axiscolor=clr;
    chart->invalidateChart();
    }
  }


// Change tickmark colors
void FXAxis::setTickColors(FXColor major,FXColor minor){
  if(majortickcolor!=major || minortickcolor!=minor){
    majortickcolor=major;
    minortickcolor=minor;
    chart->invalidateChart();
    }
  }


// Change gridline colors
void FXAxis::setGridColors(FXColor major,FXColor minor){
  if(majorgridcolor!=major || minorgridcolor!=minor){
    majorgridcolor=major;
    minorgridcolor=minor;
    chart->invalidateChart();
    }
  }


// Change label text style
void FXAxis::setLabelFont(FXFont* font){
  if(!font){ fxerror("FXAxis::setLabelFont: NULL font specified.\n"); }
  if(labelfont!=font){
    labelfont=font;
    chart->recalc();
    }
  }


// Change number text style
void FXAxis::setNumberStyle(FXuint sty){
  if(numberstyle!=sty){
    numberstyle=sty;
    chart->recalc();
    }
  }


// Change number color
void FXAxis::setNumberColor(FXColor clr){
  if(numbercolor!=clr){
    numbercolor=clr;
    chart->recalc();
    }
  }


// Change label text style
void FXAxis::setNumberFont(FXFont* font){
  if(!font){ fxerror("FXAxis::setNumberFont: NULL font specified.\n"); }
  if(numberfont!=font){
    numberfont=font;
    chart->recalc();
    }
  }


// Set data range
void FXAxis::setDataRange(FXdouble min,FXdouble max){
  if(min>max){ fxerror("FXAxis::setDataRange: bad range argument.\n"); }
  if(datarange.minimum!=min || datarange.maximum!=max){
    datarange.minimum=min;
    datarange.maximum=max;
    chart->invalidateChart();
    }
  }


// Set axis range
void FXAxis::setAxisRange(FXdouble min,FXdouble max){
  if(min>max){ fxerror("FXAxis::setAxisRange: bad range argument.\n"); }
  if(axisrange.minimum!=min || axisrange.maximum!=max){
    if(min==max){
      if(0.0<min){ min=0.0; }
      else if(max<0.0){ max=0.0; }
      else{ min=0.0; max=1.0; }
      }
    if(style&AXIS_TICKS_AUTO){
      getTickSpacing(majorspace,minorspace,min,max);
      }
    if(style&AXIS_ROUND){
      min=majorspace*Math::floor(min/majorspace);
      max=majorspace*Math::ceil(max/majorspace);
      }
    FXASSERT(min<max);
    axisrange.minimum=min;
    axisrange.maximum=max;
    chart->invalidateChart();
    }
  }


// Change major tick size
void FXAxis::setMajorTickSize(FXint sz){
  if(majorticksize!=sz){
    majorticksize=sz;
    chart->recalc();
    }
  }


// Change minor tick size
void FXAxis::setMinorTickSize(FXint sz){
  if(minorticksize!=sz){
    minorticksize=sz;
    chart->recalc();
    }
  }


// Change major tick space
void FXAxis::setMajorTickSpace(FXdouble space){
  if(space<=0.0){ fxerror("FXAxis::setMajorTickSpace: tick space should be > 0.\n"); }
  if(majorspace!=space){
    majorspace=space;
    chart->recalc();
    }
  }


// Change minor tick space
void FXAxis::setMinorTickSpace(FXdouble space){
  if(space<=0.0){ fxerror("FXAxis::setMinorTickSpace: tick space should be > 0.\n"); }
  if(minorspace!=space){
    minorspace=space;
    chart->recalc();
    }
  }


// Change number offset
void FXAxis::setNumberOffset(FXint off){
  if(numberoffset!=off){
    numberoffset=off;
    chart->recalc();
    }
  }


// Change label offset
void FXAxis::setLabelOffset(FXint off){
  if(labeloffset!=off){
    labeloffset=off;
    chart->recalc();
    }
  }


// Draw the numbers without fading on the next draw
void FXAxis::skipFade(){
  fadeskip=true;
  }


// Allow or disallow fading numbers that change
void FXAxis::setFadeEnabled(FXbool on){
  fadeon=on;
  }


// Reset axis to data range
void FXAxis::resetAxisRange(){
  setAxisRange(datarange.minimum,datarange.maximum);
  }


// Destroy axis
FXAxis::~FXAxis(){
  chart=(FXChart*)-1L;
  labelfont=(FXFont*)-1L;
  numberfont=(FXFont*)-1L;
  }


}
