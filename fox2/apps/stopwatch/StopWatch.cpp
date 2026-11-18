#include "StopWatch.h"

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_QUIT,StopWatch::onQuit),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,StopWatch::onMiddlePress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,StopWatch::onMiddleRelease),
  FXMAPFUNC(SEL_MOTION,0,StopWatch::onMotion),
  FXMAPFUNC(SEL_CONFIGURE,0,StopWatch::onConfigure),
  };

FXIMPLEMENT(StopWatch,FXMainWindow,StopWatchMap,ARRAYNUMBER(StopWatchMap))


StopWatch::StopWatch(){
  }


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_ALL|DECOR_RESIZE,0,0,260,180,0,0,0,0){
  clockLabel=nullptr;
  setBackColor(getApp()->getBaseColor());
  dragging=false;
  dragx=0;
  dragy=0;
  FXVerticalFrame* frame=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,12,12,12,12);
  frame->setBackColor(getApp()->getBaseColor());
  clockLabel=new ClockCanvas(frame,this,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE);
  clockLabel->setBackColor(getApp()->getBaseColor());
  clockLabel->setTarget(this);
  clockLabel->setSelector(0);
  getAccelTable()->addAccel(MKUINT(KEY_Escape,0),this,FXSEL(SEL_COMMAND,ID_QUIT));
  getAccelTable()->addAccel(MKUINT(KEY_Q,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_QUIT));
  }


StopWatch::~StopWatch(){
  getApp()->removeTimeout(this,ID_TICK);
  }


void StopWatch::create(){
  FXMainWindow::create();
  FXint x=getApp()->reg().readIntEntry("SETTINGS","x",-1000000);
  FXint y=getApp()->reg().readIntEntry("SETTINGS","y",-1000000);
  FXbool validPosition=(x!=-1000000) && (y!=-1000000) &&
                       (x>=-32) && (y>=-32) &&
                       (x<getRoot()->getWidth()) && (y<getRoot()->getHeight());
  if(validPosition){
    move(x,y);
    show();
    }
  else{
    show(PLACEMENT_SCREEN);
    }
  raise();
  setFocus();
  onTick(nullptr,0,nullptr);
  }


FXbool StopWatch::close(FXbool notify){
  getApp()->removeTimeout(this,ID_TICK);
  getApp()->reg().writeIntEntry("SETTINGS","x",getX());
  getApp()->reg().writeIntEntry("SETTINGS","y",getY());
  getApp()->reg().writeIntEntry("SETTINGS","mode",0);
  getApp()->reg().writeIntEntry("SETTINGS","style",0);
  getApp()->reg().write();
  return FXMainWindow::close(notify);
  }


long StopWatch::onTick(FXObject*,FXSelector,void*){
  FXTime now=FXThread::time();
  FXString text=FXSystem::localTime(now,"%H:%M:%S");
  if(clockLabel){
    clockLabel->setText(text);
    clockLabel->update();
    }
  FXTime remainder=1000000000-(now%1000000000);
  getApp()->addTimeout(this,ID_TICK,remainder);
  return 1;
  }


long StopWatch::onQuit(FXObject*,FXSelector,void*){
  return close(true)?1:0;
  }


long StopWatch::onMiddlePress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  dragging=true;
  dragx=event->root_x-getX();
  dragy=event->root_y-getY();
  startDrag();
  return 1;
  }


long StopWatch::onMiddleRelease(FXObject*,FXSelector,void*){
  if(dragging){
    dragging=false;
    getApp()->reg().writeIntEntry("SETTINGS","x",getX());
    getApp()->reg().writeIntEntry("SETTINGS","y",getY());
    getApp()->reg().write();
    }
  return 1;
  }


long StopWatch::onMotion(FXObject*,FXSelector,void* ptr){
  if(dragging){
    FXEvent* event=(FXEvent*)ptr;
    move(event->root_x-dragx,event->root_y-dragy);
    return 1;
    }
  return 0;
  }


long StopWatch::onConfigure(FXObject* sender,FXSelector sel,void* ptr){
  FXShell::onConfigure(sender,sel,ptr);
  layout();
  update();
  return 1;
  }


FXDEFMAP(ClockCanvas) ClockCanvasMap[]={
  FXMAPFUNC(SEL_PAINT,0,ClockCanvas::onPaint),
  };

FXIMPLEMENT(ClockCanvas,FXCanvas,ClockCanvasMap,ARRAYNUMBER(ClockCanvasMap))


ClockCanvas::ClockCanvas(){
  }


ClockCanvas::ClockCanvas(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXCanvas(p,tgt,sel,opts,x,y,w,h){
  setBackColor(getApp()->getBaseColor());
  }


void ClockCanvas::setText(const FXString& value){
  if(text!=value){
    text=value;
    update();
    }
  }


long ClockCanvas::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(getApp()->getBaseColor());
  dc.fillRectangle(0,0,width,height);
  dc.setFont(getApp()->getNormalFont());
  dc.setForeground(getApp()->getForeColor());
  if(!text.empty()){
    FXFont* font=getApp()->getNormalFont();
    FXint tw=font->getTextWidth(text);
    FXint th=font->getFontHeight();
    dc.drawText((width-tw)/2,(height+th)/2-th/4,text);
    }
  return 1;
  }
