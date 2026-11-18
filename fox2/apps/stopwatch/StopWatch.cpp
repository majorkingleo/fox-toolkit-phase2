#include "StopWatch.h"

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_QUIT,StopWatch::onQuit),
  };

FXIMPLEMENT(StopWatch,FXMainWindow,StopWatchMap,ARRAYNUMBER(StopWatchMap))


StopWatch::StopWatch(){
  }


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_NONE,0,0,260,180,0,0,0,0){
  clockLabel=nullptr;
  FXVerticalFrame* frame=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,12,12,12,12);
  clockLabel=new FXLabel(frame,"",nullptr,LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|ICON_BEFORE_TEXT|LABEL_NORMAL);
  getAccelTable()->addAccel(MKUINT(KEY_Escape,0),this,FXSEL(SEL_COMMAND,ID_QUIT));
  getAccelTable()->addAccel(MKUINT(KEY_Q,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_QUIT));
  }


StopWatch::~StopWatch(){
  getApp()->removeTimeout(this,ID_TICK);
  }


void StopWatch::create(){
  FXMainWindow::create();
  place(PLACEMENT_SCREEN);
  show();
  onTick(nullptr,0,nullptr);
  }


FXbool StopWatch::close(FXbool notify){
  getApp()->removeTimeout(this,ID_TICK);
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
