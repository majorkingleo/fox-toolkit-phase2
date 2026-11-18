#include "StopWatch.h"

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_QUIT,StopWatch::onQuit),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,StopWatch::onMiddlePress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,StopWatch::onMiddleRelease),
  FXMAPFUNC(SEL_MOTION,0,StopWatch::onMotion),
  };

FXIMPLEMENT(StopWatch,FXMainWindow,StopWatchMap,ARRAYNUMBER(StopWatchMap))


StopWatch::StopWatch(){
  }


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_NONE,0,0,260,180,0,0,0,0){
  clockLabel=nullptr;
  dragging=false;
  dragx=0;
  dragy=0;
  FXVerticalFrame* frame=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,12,12,12,12);
  clockLabel=new FXLabel(frame,"",nullptr,LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|ICON_BEFORE_TEXT|LABEL_NORMAL);
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
  if((x!=-1000000) && (y!=-1000000)){
    x=FXCLAMP(0,x,getRoot()->getWidth()-FXMIN(getWidth(),32)-32);
    y=FXCLAMP(0,y,getRoot()->getHeight()-FXMIN(getHeight(),32)-32);
    move(x,y);
    }
  else{
    place(PLACEMENT_SCREEN);
    }
  show();
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
