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
#include "fx.h"
#include "../chartdefs.h"
#include "../chartutils.h"
#include "../FXAxis.h"
#include "../FXChart.h"
#include "../FXData.h"
#include "../FXCurve.h"
#include "../FX2DChart.h"
#include "../FX2DPlot.h"
#include "LiveSource.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#include <unistd.h>
#endif

/*
  Notes:

  - Timers in FOX are one-shot; onTimeout() re-arms the timer itself.
  - The pointer is polled through FXWindow::getCursorPosition(), because
    Wayland has no global pointer query and this works on all platforms.
*/

using namespace FXCHART;

/*******************************************************************************/

namespace FXCHART {

// Map
FXDEFMAP(LiveSource) LiveSourceMap[]={
  FXMAPFUNC(SEL_TIMEOUT,LiveSource::ID_TIMEOUT,LiveSource::onTimeout),
  };


// Object implementation
FXIMPLEMENT(LiveSource,FXObject,LiveSourceMap,ARRAYNUMBER(LiveSourceMap))


// Make a live curve
void LiveSource::addCurve(FXint index,const FXString& label,FXColor color,FXuchar axis){
  FXData *xd=new FXData();
  FXData *yd=new FXData();
  FXCurve *curve=new FXCurve(plot,label);
  LineStyle style;
  xd->setLimit(20000);
  yd->setLimit(20000);
  style.color=color;
  style.weight=1;
  style.style=LINESTYLE_SOLID;
  curve->setLineStyle(style);
  curve->setYAxis(axis);
  curve->setXData(xd);
  curve->setYData(yd);
  curve->setVisible(false);
  plot->appendCurve(curve);
  curves[index]=curve;
  timedata[index]=xd;
  values[index]=yd;
  }


// Show or hide the live curves
void LiveSource::showCurves(FXbool show){
  FXint i;
  for(i=0; i<SERIES_COUNT; i++){
    curves[i]->setVisible(show);
    }
  plot->update();
  }


// Sample pointer speed and acceleration
void LiveSource::samplePointer(FXdouble dt,FXdouble& speed,FXdouble& accel){
  FXint x=0;
  FXint y=0;
  FXuint buttons=0;
  speed=0.0;
  accel=0.0;
  if(plot->getCursorPosition(x,y,buttons)){
    if(lastvalid){
      FXdouble dx=(FXdouble)(x-lastx);
      FXdouble dy=(FXdouble)(y-lasty);
      FXdouble distance=Math::sqrt(dx*dx+dy*dy);
      speed=distance/(dt*1000.0);
      accel=(speed-lastspeed)/(dt*1000.0);
      }
    lastx=x;
    lasty=y;
    lastspeed=speed;
    lastvalid=true;
    }
  else{
    lastvalid=false;
    }
  }


// Read CPU load in percent
FXdouble LiveSource::sampleCPU(){
#if defined(__linux__)
  FILE *fp;
  unsigned long long user,nice,system,idle,iowait,irq,softirq,steal;
  unsigned long long total,idletotal,dtotal,didle;
  FXdouble load=0.0;
  fp=fopen("/proc/stat","r");
  if(!fp) return 0.0;
  if(fscanf(fp,"cpu %llu %llu %llu %llu %llu %llu %llu %llu",
      &user,&nice,&system,&idle,&iowait,&irq,&softirq,&steal)!=8){
    fclose(fp);
    return 0.0;
    }
  fclose(fp);
  total=user+nice+system+idle+iowait+irq+softirq+steal;
  idletotal=idle+iowait;
  if(lastcpu && total>lastcpu){
    dtotal=total-lastcpu;
    didle=idletotal-lastidle;
    if(dtotal) load=100.0*((FXdouble)(dtotal-didle))/(FXdouble)dtotal;
    if(load<0.0) load=0.0;
    if(load>100.0) load=100.0;
    }
  lastcpu=total;
  lastidle=idletotal;
  return load;
#else
  return 0.0;
#endif
  }


// Read resident memory in megabytes
FXdouble LiveSource::sampleMemory(){
#if defined(__linux__)
  FILE *fp;
  unsigned long total,resident;
  long pagesize;
  fp=fopen("/proc/self/statm","r");
  if(!fp) return 0.0;
  if(fscanf(fp,"%lu %lu",&total,&resident)!=2){
    fclose(fp);
    return 0.0;
    }
  fclose(fp);
  pagesize=sysconf(_SC_PAGESIZE);
  if(pagesize<=0) return 0.0;
  return (FXdouble)resident*(FXdouble)pagesize/(1024.0*1024.0);
#else
  return 0.0;
#endif
  }


// Deserialization
LiveSource::LiveSource():plot(nullptr),interval(50000000),starttime(0),lasttime(0),window(60.0),lastspeed(0.0),lastx(0),lasty(0),lastvalid(false),lastcpu(0),lastidle(0),running(false){
  FXint i;
  for(i=0; i<SERIES_COUNT; i++){
    curves[i]=nullptr;
    timedata[i]=nullptr;
    values[i]=nullptr;
    }
  }


// Construct a live source feeding plot
LiveSource::LiveSource(FX2DPlot* p,FXTime ns):plot(p),interval(ns),starttime(0),lasttime(0),window(60.0),lastspeed(0.0),lastx(0),lasty(0),lastvalid(false),lastcpu(0),lastidle(0),running(false){
  FXint i;
  if(interval<1000000) interval=1000000;
  for(i=0; i<SERIES_COUNT; i++){
    curves[i]=nullptr;
    timedata[i]=nullptr;
    values[i]=nullptr;
    }
  addCurve(SERIES_SPEED,"Mouse speed",FXRGB(220,40,40),FX2DChart::YLO);
  addCurve(SERIES_ACCEL,"Mouse acceleration",FXRGB(230,140,20),FX2DChart::YLO);
  addCurve(SERIES_CPU,"CPU load",FXRGB(40,160,40),FX2DChart::YHI);
  addCurve(SERIES_MEMORY,"Memory",FXRGB(40,80,220),FX2DChart::YHI);
  addCurve(SERIES_FRAME,"Frame time",FXRGB(160,40,160),FX2DChart::YHI);
  }


// Start sampling
void LiveSource::start(){
  if(running) return;
  starttime=FXThread::steadytime();
  lasttime=starttime;
  lastspeed=0.0;
  lastvalid=false;
  lastcpu=0;
  lastidle=0;
  running=true;
  showCurves(true);
  plot->getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_TIMEOUT),interval);
  }


// Stop sampling
void LiveSource::stop(){
  if(!running) return;
  plot->getApp()->removeTimeout(this,FXSEL(SEL_TIMEOUT,ID_TIMEOUT));
  running=false;
  showCurves(false);
  }


// Clear samples and restart the time base
void LiveSource::clear(){
  FXint i;
  for(i=0; i<SERIES_COUNT; i++){
    timedata[i]->clear();
    values[i]->clear();
    }
  starttime=lasttime=FXThread::steadytime();
  lastspeed=0.0;
  lastvalid=false;
  lastcpu=0;
  lastidle=0;
  plot->update();
  }


// Change sampling interval
void LiveSource::setInterval(FXTime ns){
  if(ns<1000000) ns=1000000;
  interval=ns;
  }


// Change visible history
void LiveSource::setWindow(FXdouble seconds){
  if(seconds<1.0) seconds=1.0;
  window=seconds;
  }


// Timer callback
long LiveSource::onTimeout(FXObject*,FXSelector,void*){
  FXTime now;
  FXdouble t,dt,speed,accel,cpu,memory,frame,xmin;
  FXint i;
  if(!running) return 0;
  now=FXThread::steadytime();
  dt=(FXdouble)(now-lasttime)/1000000000.0;
  if(dt<=0.0) dt=(FXdouble)interval/1000000000.0;
  t=(FXdouble)(now-starttime)/1000000000.0;
  samplePointer(dt,speed,accel);
  cpu=sampleCPU();
  memory=sampleMemory();
  frame=(FXdouble)plot->getLastDrawTime()/1000000.0;
  if(getenv("CHART_LIVE_TRACE")){
    fprintf(stderr,"live t=%.3f speed=%.1f accel=%.1f cpu=%.1f mem=%.1f frame=%.3f\n",t,speed,accel,cpu,memory,frame);
    }
  for(i=0; i<SERIES_COUNT; i++){
    timedata[i]->append(t);
    }
  values[SERIES_SPEED]->append(speed);
  values[SERIES_ACCEL]->append(accel);
  values[SERIES_CPU]->append(cpu);
  values[SERIES_MEMORY]->append(memory);
  values[SERIES_FRAME]->append(frame);
  xmin=t>window ? t-window : 0.0;
  plot->getAxis(FX2DChart::XLO)->setAxisRange(xmin,t);
  plot->getAxis(FX2DChart::XHI)->setAxisRange(xmin,t);
  plot->fitY();
  plot->update();
  lasttime=now;
  plot->getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_TIMEOUT),interval);
  return 1;
  }


// Destroy
LiveSource::~LiveSource(){
  stop();
  plot=nullptr;
  }

}
