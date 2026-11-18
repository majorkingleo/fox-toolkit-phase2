#include "StopWatch.h"

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_QUIT,StopWatch::onQuit),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,StopWatch::onMiddlePress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,StopWatch::onMiddleRelease),
  FXMAPFUNC(SEL_MOTION,0,StopWatch::onMotion),
  FXMAPFUNC(SEL_CONFIGURE,0,StopWatch::onConfigure),
  FXMAPFUNC(SEL_PAINT,0,StopWatch::onPaint),
  };

FXIMPLEMENT(StopWatch,FXMainWindow,StopWatchMap,ARRAYNUMBER(StopWatchMap))


StopWatch::StopWatch(){
  }


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_ALL|DECOR_RESIZE,0,0,260,180,0,0,0,0){
  setBackColor(getApp()->getBaseColor());
  dragging=false;
  dragx=0;
  dragy=0;
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
  update();
  FXTime now=FXThread::time();
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


long StopWatch::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(getApp()->getBaseColor());
  dc.fillRectangle(0,0,width,height);
  FXString text=FXSystem::localTime(FXThread::time(),"%H:%M:%S");
  FXFont* font=getApp()->getNormalFont();
  if(font && font->id()){
    dc.setFont(font);
    dc.setForeground(getApp()->getForeColor());
    FXint tw=font->getTextWidth(text);
    FXint th=font->getFontHeight();
    dc.drawText((width-tw)/2,(height-th)/2+th/2,text);
    }
  return 1;
  }
