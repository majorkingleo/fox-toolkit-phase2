#include "StopWatch.h"
#include "icons.h"

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_CLOCK,StopWatch::onModeClock),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_STOPWATCH,StopWatch::onModeStopwatch),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_TIMER,StopWatch::onModeTimer),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_CLOCK,StopWatch::onUpdMode),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_STOPWATCH,StopWatch::onUpdMode),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_TIMER,StopWatch::onUpdMode),
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


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_NONE,0,0,260,180,0,0,0,0){
  setBackColor(getApp()->getBaseColor());
  mode=MODE_CLOCK;
  dragging=false;
  dragx=0;
  dragy=0;
  switcher=nullptr;
  for(FXint i=0;i<3;i++){
    modePages[i]=nullptr;
    modeIcons[i]=nullptr;
    modeButtons[i]=nullptr;
    }
  FXVerticalFrame* main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
  switcher=new FXSwitcher(main,LAYOUT_FILL_X|LAYOUT_FILL_Y|SWITCHER_HCOLLAPSE|SWITCHER_VCOLLAPSE,0,0,0,0,0,0,0,0);
  modePages[0]=new FXLabel(switcher,"",nullptr,LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modePages[1]=new FXLabel(switcher,"00:00.00",nullptr,LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modePages[2]=new FXLabel(switcher,"00:00:00",nullptr,LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  FXHorizontalFrame* bottom=new FXHorizontalFrame(main,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|PACK_UNIFORM_WIDTH,0,0,0,40,4,4,4,4);
  modeIcons[0]=new FXSVGIcon(getApp(),clock_svg,FXRGB(32,33,36),0,24,24);
  modeIcons[1]=new FXSVGIcon(getApp(),stopwatch_svg,FXRGB(32,33,36),0,24,24);
  modeIcons[2]=new FXSVGIcon(getApp(),timer_svg,FXRGB(32,33,36),0,24,24);
  modeButtons[0]=new FXButton(bottom,"",modeIcons[0],this,FXSEL(SEL_COMMAND,ID_MODE_CLOCK),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modeButtons[1]=new FXButton(bottom,"",modeIcons[1],this,FXSEL(SEL_COMMAND,ID_MODE_STOPWATCH),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modeButtons[2]=new FXButton(bottom,"",modeIcons[2],this,FXSEL(SEL_COMMAND,ID_MODE_TIMER),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  }


StopWatch::~StopWatch(){
  getApp()->removeTimeout(this,ID_TICK);
  for(FXint i=0;i<3;i++) delete modeIcons[i];
  }


void StopWatch::create(){
  FXMainWindow::create();
  for(FXint i=0;i<3;i++){
    if(modeIcons[i]){
      modeIcons[i]->create();
      modeIcons[i]->render();
      }
    }
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
  FXString text;
  if(mode==MODE_CLOCK) text=FXSystem::localTime(FXThread::time(),"%H:%M:%S");
  else if(mode==MODE_STOPWATCH) text="00:00.00";
  else text="00:00:00";
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


long StopWatch::onModeClock(FXObject*,FXSelector,void*){
  mode=MODE_CLOCK;
  switcher->setCurrent(MODE_CLOCK);
  update();
  return 1;
  }


long StopWatch::onModeStopwatch(FXObject*,FXSelector,void*){
  mode=MODE_STOPWATCH;
  switcher->setCurrent(MODE_STOPWATCH);
  update();
  return 1;
  }


long StopWatch::onModeTimer(FXObject*,FXSelector,void*){
  mode=MODE_TIMER;
  switcher->setCurrent(MODE_TIMER);
  update();
  return 1;
  }


long StopWatch::onUpdMode(FXObject* sender,FXSelector sel,void*){
  FXButton* button=(FXButton*)sender;
  FXuint id=FXSELID(sel);
  if(id==ID_MODE_CLOCK) button->setState(mode==MODE_CLOCK?STATE_DOWN:STATE_UP);
  if(id==ID_MODE_STOPWATCH) button->setState(mode==MODE_STOPWATCH?STATE_DOWN:STATE_UP);
  if(id==ID_MODE_TIMER) button->setState(mode==MODE_TIMER?STATE_DOWN:STATE_UP);
  return 1;
  }
