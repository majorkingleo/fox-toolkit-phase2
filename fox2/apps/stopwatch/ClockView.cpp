#include "ClockView.h"

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

FXDEFMAP(ClockView) ClockViewMap[]={
  FXMAPFUNC(SEL_PAINT,0,ClockView::onPaint),
  };

FXIMPLEMENT(ClockView,FXCanvas,ClockViewMap,ARRAYNUMBER(ClockViewMap))


ClockView::ClockView(){
  hour=0;
  minute=0;
  second=0;
  milli=0;
  clockmode=MODE_CLOCK;
  progress=-1.0;
  }


ClockView::ClockView(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts):FXCanvas(p,tgt,sel,opts){
  hour=0;
  minute=0;
  second=0;
  milli=0;
  clockmode=MODE_CLOCK;
  progress=-1.0;
  }


FXint ClockView::getDefaultWidth(){
  return 160;
  }


FXint ClockView::getDefaultHeight(){
  return 160;
  }


void ClockView::setTime(FXint h,FXint m,FXint s,FXint ms,FXuint mode,FXdouble frac){
  if(h!=hour || m!=minute || s!=second || ms!=milli || mode!=clockmode || frac!=progress){
    hour=h;
    minute=m;
    second=s;
    milli=ms;
    clockmode=mode;
    progress=frac;
    update();
    }
  }


long ClockView::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  FXint cx=width/2;
  FXint cy=height/2;
  FXint r=FXMIN(width,height)/2-6;
  FXint i,a;

  dc.setForeground(getApp()->getBaseColor());
  dc.fillRectangle(0,0,width,height);
  if(r<8) return 1;

  // Face
  dc.setForeground(getApp()->getBackColor());
  dc.fillArc(cx-r,cy-r,2*r,2*r,0,64*360);
  dc.setForeground(getApp()->getShadowColor());
  dc.drawArc(cx-r,cy-r,2*r,2*r,0,64*360);

  // Timer wedge shows the remaining part of the duration
  if(clockmode==MODE_TIMER && progress>0.0){
    FXdouble frac=FXCLAMP(0.0,progress,1.0);
    FXint sweep=(FXint)(frac*360.0*64.0);
    dc.setForeground(getApp()->getHiliteColor());
    dc.fillArc(cx-r,cy-r,2*r,2*r,90*64-sweep,sweep);
    }

  // Ticks, every fifth one longer and thicker
  for(i=0;i<60;i++){
    FXdouble ang=i*6.0*PI/180.0;
    FXint len=(i%5==0)?r/7:r/16;
    FXint x1=cx+(FXint)((r-2)*sin(ang));
    FXint y1=cy-(FXint)((r-2)*cos(ang));
    FXint x2=cx+(FXint)((r-2-len)*sin(ang));
    FXint y2=cy-(FXint)((r-2-len)*cos(ang));
    dc.setLineWidth((i%5==0)?2:1);
    dc.setForeground((i%5==0)?getApp()->getForeColor():getApp()->getShadowColor());
    dc.drawLine(x1,y1,x2,y2);
    }
  dc.setLineWidth(1);

  // Hand angles in radians from twelve o'clock, clockwise
  FXdouble secs=second+milli/1000.0;
  FXdouble mins=minute+secs/60.0;
  FXdouble hours=hour+mins/60.0;
  FXdouble ha=hours*30.0*PI/180.0;
  FXdouble ma=mins*6.0*PI/180.0;
  FXdouble sa=secs*6.0*PI/180.0;

  dc.setForeground(getApp()->getForeColor());
  if(clockmode==MODE_CLOCK){
    dc.setLineWidth(3);
    dc.drawLine(cx,cy,cx+(FXint)(r*0.5*sin(ha)),cy-(FXint)(r*0.5*cos(ha)));
    }
  dc.setLineWidth(2);
  dc.drawLine(cx,cy,cx+(FXint)(r*0.75*sin(ma)),cy-(FXint)(r*0.75*cos(ma)));
  dc.setLineWidth(1);
  dc.setForeground(getApp()->getSelbackColor());
  dc.drawLine(cx,cy,cx+(FXint)(r*0.85*sin(sa)),cy-(FXint)(r*0.85*cos(sa)));

  // Hub
  a=FXMAX(2,r/24);
  dc.setForeground(getApp()->getForeColor());
  dc.fillArc(cx-a,cy-a,2*a,2*a,0,64*360);
  return 1;
  }
