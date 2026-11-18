#include "ClockView.h"

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

// The face is drawn three times too large and averaged down, because the
// raster backend draws lines and arcs one pixel at a time and has no coverage
static const FXint SUPERSAMPLE=3;

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
  big=nullptr;
  face=nullptr;
  }


ClockView::ClockView(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts):FXCanvas(p,tgt,sel,opts){
  hour=0;
  minute=0;
  second=0;
  milli=0;
  clockmode=MODE_CLOCK;
  progress=-1.0;
  big=nullptr;
  face=nullptr;
  }


ClockView::~ClockView(){
  delete big;
  delete face;
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


// Make an image that owns a zeroed pixel buffer, or return null
FXImage* ClockView::makeImage(FXint w,FXint h){
  FXColor *pixels=nullptr;
  FXImage *image=nullptr;
  if(callocElms(pixels,(FXuval)w*h)){
    image=new FXImage(getApp());
    image->setData(pixels,IMAGE_OWNED,w,h);
    image->create();
    // X11 and Win32 drop the pixel buffer in create() unless IMAGE_KEEP is
    // set, which the constructor does not accept; restore() brings it back
    image->restore();
    return image;
    }
  return nullptr;
  }


// Draw the whole face; s is the supersample factor the coordinates are in
void ClockView::drawFace(FXDCWindow& dc,FXint w,FXint h,FXint s){
  FXint cx=w/2;
  FXint cy=h/2;
  FXint r=FXMIN(w,h)/2-6*s;
  FXint i,a;

  dc.setForeground(getApp()->getBaseColor());
  dc.fillRectangle(0,0,w,h);
  if(r<8*s) return;

  // Face
  dc.setForeground(getApp()->getBackColor());
  dc.fillArc(cx-r,cy-r,2*r,2*r,0,64*360);
  dc.setForeground(getApp()->getShadowColor());
  dc.setLineWidth(s);
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
    FXint x1=cx+(FXint)((r-2*s)*sin(ang));
    FXint y1=cy-(FXint)((r-2*s)*cos(ang));
    FXint x2=cx+(FXint)((r-2*s-len)*sin(ang));
    FXint y2=cy-(FXint)((r-2*s-len)*cos(ang));
    dc.setLineWidth((i%5==0)?2*s:s);
    dc.setForeground((i%5==0)?getApp()->getForeColor():getApp()->getShadowColor());
    dc.drawLine(x1,y1,x2,y2);
    }
  dc.setLineWidth(s);

  // Hand angles in radians from twelve o'clock, clockwise
  FXdouble secs=second+milli/1000.0;
  FXdouble mins=minute+secs/60.0;
  FXdouble hours=hour+mins/60.0;
  FXdouble ha=hours*30.0*PI/180.0;
  FXdouble ma=mins*6.0*PI/180.0;
  FXdouble sa=secs*6.0*PI/180.0;

  dc.setForeground(getApp()->getForeColor());
  if(clockmode==MODE_CLOCK){
    dc.setLineWidth(3*s);
    dc.drawLine(cx,cy,cx+(FXint)(r*0.5*sin(ha)),cy-(FXint)(r*0.5*cos(ha)));
    }
  dc.setLineWidth(2*s);
  dc.drawLine(cx,cy,cx+(FXint)(r*0.75*sin(ma)),cy-(FXint)(r*0.75*cos(ma)));
  dc.setLineWidth(s);
  dc.setForeground(getApp()->getSelbackColor());
  dc.drawLine(cx,cy,cx+(FXint)(r*0.85*sin(sa)),cy-(FXint)(r*0.85*cos(sa)));

  // Hub
  a=FXMAX(2*s,r/24);
  dc.setForeground(getApp()->getForeColor());
  dc.fillArc(cx-a,cy-a,2*a,2*a,0,64*360);
  }


// Average s x s blocks of the big image into the face image
void ClockView::shrink(const FXColor* src,FXint sw,FXColor* dst,FXint w,FXint h,FXint s){
  FXint x,y,i,j,n=s*s;
  for(y=0; y<h; y++){
    for(x=0; x<w; x++){
      FXuint red=0,green=0,blue=0;
      const FXColor* row=src+(FXuval)(y*s)*sw+x*s;
      for(j=0; j<s; j++){
        for(i=0; i<s; i++){
          FXColor pixel=row[j*sw+i];
          red+=FXREDVAL(pixel);
          green+=FXGREENVAL(pixel);
          blue+=FXBLUEVAL(pixel);
          }
        }
      dst[(FXuval)y*w+x]=FXRGB(red/n,green/n,blue/n);
      }
    }
  }


long ClockView::onPaint(FXObject*,FXSelector,void* ptr){
  FXint sw=FXMAX(width,1)*SUPERSAMPLE;
  FXint sh=FXMAX(height,1)*SUPERSAMPLE;

  // Buffers follow the widget size; a resize makes them again
  if(big==nullptr || big->getWidth()!=sw || big->getHeight()!=sh){
    delete big;
    delete face;
    big=makeImage(sw,sh);
    face=makeImage(FXMAX(width,1),FXMAX(height,1));
    }

  if(big && face && big->getData() && face->getData()){
    {
    FXDCWindow dc(big);
    drawFace(dc,sw,sh,SUPERSAMPLE);
    dc.end();
    }
    big->restore();
    shrink(big->getData(),sw,face->getData(),FXMAX(width,1),FXMAX(height,1),SUPERSAMPLE);
    face->render();
    FXDCWindow dc(this,(FXEvent*)ptr);
    dc.drawImage(face,0,0);
    return 1;
    }

  // No buffers: draw aliased rather than not at all
  FXDCWindow dc(this,(FXEvent*)ptr);
  drawFace(dc,FXMAX(width,1),FXMAX(height,1),1);
  return 1;
  }
