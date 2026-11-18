#include "StopWatch.h"
#include "ClockView.h"
#include "icons.h"
#include "fxkeys.h"

#include <stdio.h>

FXDEFMAP(StopWatch) StopWatchMap[]={
  FXMAPFUNC(SEL_TIMEOUT,StopWatch::ID_TICK,StopWatch::onTick),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_CLOCK,StopWatch::onModeClock),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_STOPWATCH,StopWatch::onModeStopwatch),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_MODE_TIMER,StopWatch::onModeTimer),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_CLOCK,StopWatch::onUpdMode),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_STOPWATCH,StopWatch::onUpdMode),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_MODE_TIMER,StopWatch::onUpdMode),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_STYLE,StopWatch::onStyle),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_STYLE,StopWatch::onUpdStyle),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_STARTSTOP,StopWatch::onStartStop),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_STARTSTOP,StopWatch::onUpdStartStop),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_STOPWATCH_RESET,StopWatch::onStopwatchReset),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_STOPWATCH_RESET,StopWatch::onUpdStopwatchReset),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_TIMER_SPIN,StopWatch::onTimerSpin),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_TIMER_START,StopWatch::onTimerStart),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_TIMER_START,StopWatch::onUpdTimerStart),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_TIMER_RESET,StopWatch::onTimerReset),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_TIMER_RESET,StopWatch::onUpdTimerReset),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_CLEAR_HISTORY,StopWatch::onClearHistory),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_QUIT,StopWatch::onQuit),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,StopWatch::onMiddlePress),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,StopWatch::ID_RIGHT_CLICK,StopWatch::onMiddlePress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,StopWatch::onMiddleRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,StopWatch::ID_RIGHT_CLICK,StopWatch::onMiddleRelease),
  FXMAPFUNC(SEL_MOTION,0,StopWatch::onMotion),
  FXMAPFUNC(SEL_MOTION,StopWatch::ID_RIGHT_CLICK,StopWatch::onMotion),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,StopWatch::onRightPress),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,StopWatch::ID_MODE_CLOCK,StopWatch::onRightPress),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,StopWatch::ID_MODE_STOPWATCH,StopWatch::onRightPress),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,StopWatch::ID_MODE_TIMER,StopWatch::onRightPress),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,StopWatch::ID_RIGHT_CLICK,StopWatch::onRightPress),
  FXMAPFUNC(SEL_COMMAND,StopWatch::ID_SHOW_TITLEBAR,StopWatch::onShowTitlebar),
  FXMAPFUNC(SEL_UPDATE,StopWatch::ID_SHOW_TITLEBAR,StopWatch::onUpdShowTitlebar),
  FXMAPFUNC(SEL_CONFIGURE,0,StopWatch::onConfigure),
  FXMAPFUNC(SEL_PAINT,0,StopWatch::onPaint),
  };

FXIMPLEMENT(StopWatch,FXMainWindow,StopWatchMap,ARRAYNUMBER(StopWatchMap))


// Split a wall clock time into local hour, minute, second and millisecond.
static void splitLocalTime(FXTime now,FXint& h,FXint& m,FXint& s,FXint& ms){
  FXString text=FXSystem::localTime(now,"%H %M %S");
  h=text.mid(0,2).toInt();
  m=text.mid(3,2).toInt();
  s=text.mid(6,2).toInt();
  ms=(FXint)((now%1000000000)/1000000);
  }


// "HH:MM:SS", no fraction, for the timer
static FXString formatDuration(FXTime ns){
  FXchar text[64];
  FXTime secs=ns/1000000000;
  snprintf(text,sizeof(text),"%02d:%02d:%02d",(FXint)(secs/3600),(FXint)((secs/60)%60),(FXint)(secs%60));
  return FXString(text);
  }


// Design pixels to the display scale of this machine
static FXint scaledPixels(FXApp* app,FXint value){
  return FXMAX((FXint)(value*app->getWindowScale(0)+0.5),1);
  }


StopWatch::StopWatch(){
  mode=MODE_CLOCK;
  style=STYLE_DIGITAL;
  }


StopWatch::StopWatch(FXApp* a):FXMainWindow(a,"Stopwatch",nullptr,nullptr,DECOR_NONE,0,0,scaledPixels(a,260),scaledPixels(a,200),0,0,0,0){
  setBackColor(getApp()->getBaseColor());
  mode=MODE_CLOCK;
  style=STYLE_DIGITAL;
  running=false;
  accum=0;
  started=0;
  duration=0;
  remaining=0;
  deadline=0;
  timerRunning=false;
  timerDone=false;
  blinkOn=false;
  titlebar=false;
  clientWidth=scaledPixels(a,260);
  clientHeight=scaledPixels(a,200);
  dragging=false;
  dragx=0;
  dragy=0;
  faceSwitcher=nullptr;
  seven=nullptr;
  clockView=nullptr;
  controlSwitcher=nullptr;
  readout=nullptr;
  startStopButton=nullptr;
  stopwatchResetButton=nullptr;
  spinHours=nullptr;
  spinMinutes=nullptr;
  spinSeconds=nullptr;
  timerStartButton=nullptr;
  timerResetButton=nullptr;
  styleButton=nullptr;
  styleIcons[0]=nullptr;
  styleIcons[1]=nullptr;
  contextMenu=nullptr;
  titlebarCheck=nullptr;
  historyPopup=nullptr;
  historyTable=nullptr;
  for(FXint i=0;i<3;i++){
    modeIcons[i]=nullptr;
    modeButtons[i]=nullptr;
    }

  // The window has no decoration, so it needs its own one pixel border
  FXVerticalFrame* main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,0,0,0,0);

  // Face: digital seven segment or the analog canvas
  faceSwitcher=new FXSwitcher(main,LAYOUT_FILL_X|LAYOUT_FILL_Y|SWITCHER_HCOLLAPSE,0,0,0,0,0,0,0,0);
  seven=new FX7Segment(faceSwitcher,"--:--:--",SEVENSEGMENT_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  FXVerticalFrame* analogFrame=new FXVerticalFrame(faceSwitcher,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
  clockView=new ClockView(analogFrame,nullptr,0,FRAME_NONE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  // One readout under the face: empty in clock mode, so it is never shown
  // or hidden, which would need a fresh layout pass
  readout=new FXLabel(analogFrame,"",nullptr,LAYOUT_FILL_X|JUSTIFY_CENTER_X);

  // Controls: one page per mode, all of the same height
  controlSwitcher=new FXSwitcher(main,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,scaledPixels(getApp(),36));
  new FXFrame(controlSwitcher,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  FXHorizontalFrame* stopwatchPage=new FXHorizontalFrame(controlSwitcher,LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,0,0,0,0,2,2,2,2);
  startStopButton=new FXButton(stopwatchPage,"Start",nullptr,this,ID_STARTSTOP,BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  stopwatchResetButton=new FXButton(stopwatchPage,"Reset",nullptr,this,ID_STOPWATCH_RESET,BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  FXHorizontalFrame* timerPage=new FXHorizontalFrame(controlSwitcher,LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,0,0,0,0,2,2,2,2);
  spinHours=new FXSpinner(timerPage,2,this,ID_TIMER_SPIN,SPIN_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  spinHours->setRange(0,99);
  spinMinutes=new FXSpinner(timerPage,2,this,ID_TIMER_SPIN,SPIN_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  spinMinutes->setRange(0,59);
  spinSeconds=new FXSpinner(timerPage,2,this,ID_TIMER_SPIN,SPIN_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  spinSeconds->setRange(0,59);
  timerStartButton=new FXButton(timerPage,"Start",nullptr,this,ID_TIMER_START,BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  timerResetButton=new FXButton(timerPage,"Reset",nullptr,this,ID_TIMER_RESET,BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);

  // Bottom: mode switch and the style toggle
  FXHorizontalFrame* bottom=new FXHorizontalFrame(main,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|PACK_UNIFORM_WIDTH,0,0,0,scaledPixels(getApp(),40),4,4,4,4);
  modeIcons[0]=new FXSVGIcon(getApp(),clock_svg,FXRGB(32,33,36),0,24,24);
  modeIcons[1]=new FXSVGIcon(getApp(),stopwatch_svg,FXRGB(32,33,36),0,24,24);
  modeIcons[2]=new FXSVGIcon(getApp(),timer_svg,FXRGB(32,33,36),0,24,24);
  modeButtons[0]=new FXButton(bottom,"",modeIcons[0],this,FXSEL(SEL_COMMAND,ID_MODE_CLOCK),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modeButtons[1]=new FXButton(bottom,"",modeIcons[1],this,FXSEL(SEL_COMMAND,ID_MODE_STOPWATCH),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modeButtons[2]=new FXButton(bottom,"",modeIcons[2],this,FXSEL(SEL_COMMAND,ID_MODE_TIMER),BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  styleIcons[0]=new FXSVGIcon(getApp(),digital_clock_svg,FXRGB(32,33,36),0,24,24);
  styleIcons[1]=new FXSVGIcon(getApp(),analog_clock_svg,FXRGB(32,33,36),0,24,24);
  styleButton=new FXButton(bottom,"",styleIcons[0],this,ID_STYLE,BUTTON_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y);
  modeButtons[0]->setTipText("Show the clock");
  modeButtons[1]->setTipText("Show the stopwatch");
  modeButtons[2]->setTipText("Show the timer");

  // Context menu for the clock and timer modes
  contextMenu=new FXMenuPane(this);
  titlebarCheck=new FXMenuCheck(contextMenu,"&Show title bar",this,ID_SHOW_TITLEBAR);
  new FXMenuSeparator(contextMenu);
  new FXMenuCommand(contextMenu,"&Quit",nullptr,this,ID_QUIT);

  // Stopwatch history popup
  historyPopup=new FXPopup(this,POPUP_VERTICAL|FRAME_RAISED|FRAME_THICK);
  historyTable=new FXTable(historyPopup,nullptr,0,TABLE_READONLY|TABLE_COL_SIZABLE|TABLE_NO_COLSELECT|TABLE_NO_ROWSELECT|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,320,180);
  FXHorizontalFrame* popupButtons=new FXHorizontalFrame(historyPopup,LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,2,2,2,2);
  new FXButton(popupButtons,"Clear history",nullptr,this,ID_CLEAR_HISTORY,BUTTON_NORMAL|LAYOUT_FILL_X);
  new FXButton(popupButtons,"Quit",nullptr,this,ID_QUIT,BUTTON_NORMAL|LAYOUT_FILL_X);
  new FXButton(popupButtons,"Close",nullptr,historyPopup,FXSEL(SEL_COMMAND,FXWindow::ID_UNPOST),BUTTON_NORMAL|LAYOUT_FILL_X);

  // Keyboard: no title bar means no close button
  getAccelTable()->addAccel(MKUINT(KEY_Escape,0),this,FXSEL(SEL_COMMAND,ID_QUIT));
  getAccelTable()->addAccel(MKUINT(KEY_q,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_QUIT));

  routeContextEvents();
  }


StopWatch::~StopWatch(){
  getApp()->removeTimeout(this,ID_TICK);
  for(FXint i=0;i<3;i++) delete modeIcons[i];
  for(FXint i=0;i<2;i++) delete styleIcons[i];
  }


// Let the widgets that cover the window deliver middle and right clicks
// and motion to the main window, so the drag and the popup work everywhere.
void StopWatch::routeContextEvents(){
  FXWindow* widgets[]={faceSwitcher,seven,clockView,readout,controlSwitcher,nullptr};
  for(FXint i=0;widgets[i];i++){
    widgets[i]->setTarget(this);
    widgets[i]->setSelector(ID_RIGHT_CLICK);
    }
  }


void StopWatch::create(){
  FXMainWindow::create();
  for(FXint i=0;i<3;i++){
    if(modeIcons[i]){
      modeIcons[i]->create();
      modeIcons[i]->render();
      }
    }
  for(FXint i=0;i<2;i++){
    if(styleIcons[i]){
      styleIcons[i]->create();
      styleIcons[i]->render();
      }
    }
  history.load(getApp()->reg());
  loadSettings();
  show();
  raise();
  setFocus();
  onTick(nullptr,0,nullptr);
  }


void StopWatch::loadSettings(){
  FXSettings& reg=getApp()->reg();
  FXString xs=reg.readStringEntry("SETTINGS","x","");
  FXString ys=reg.readStringEntry("SETTINGS","y","");
  FXint x=xs.empty()?-1000000:xs.toInt();
  FXint y=ys.empty()?-1000000:ys.toInt();
  // FXSettings answers a stored 0 with the default, so read these as text
  mode=FXCLAMP(0,FXString(reg.readStringEntry("SETTINGS","mode","0")).toInt(),2);
  style=FXCLAMP(0,FXString(reg.readStringEntry("SETTINGS","style","0")).toInt(),1);
  titlebar=FXString(reg.readStringEntry("SETTINGS","titlebar","0")).toInt()!=0;
  accum=reg.readLongEntry("STOPWATCH","elapsed",0);
  duration=reg.readLongEntry("TIMER","duration",0);
  remaining=reg.readLongEntry("TIMER","remaining",0);
  timerRunning=false;
  timerDone=false;
  if(remaining<=0) remaining=duration;
  if(accum<0) accum=0;
  if(duration<0) duration=0;

  faceSwitcher->setCurrent(style);
  controlSwitcher->setCurrent(mode);
  clockView->setTime(0,0,0,0,ClockView::MODE_CLOCK);
  writeSpinners();
  if(titlebarCheck) titlebarCheck->setCheck(titlebar);
  setDecorations(titlebar?(DECOR_ALL|DECOR_RESIZE):DECOR_NONE);
  clientWidth=FXCLAMP(scaledPixels(getApp(),180),reg.readIntEntry("SETTINGS","w",scaledPixels(getApp(),260)),2000);
  clientHeight=FXCLAMP(scaledPixels(getApp(),120),reg.readIntEntry("SETTINGS","h",scaledPixels(getApp(),200)),2000);
  resize(clientWidth,clientHeight);
  FXbool validPosition=(x!=-1000000) && (y!=-1000000) &&
                       (x>=-32) && (y>=-32) &&
                       (x<getRoot()->getWidth()) && (y<getRoot()->getHeight());
  if(validPosition){
    move(x,y);
    }
  else{
    show(PLACEMENT_SCREEN);
    }
  updateDisplay();
  }


void StopWatch::saveSettings(){
  FXRegistry& reg=getApp()->reg();
  reg.writeIntEntry("SETTINGS","x",getX());
  reg.writeIntEntry("SETTINGS","y",getY());
  reg.writeIntEntry("SETTINGS","w",getWidth());
  reg.writeIntEntry("SETTINGS","h",getHeight());
  reg.writeIntEntry("SETTINGS","titlebar",titlebar?1:0);
  reg.writeIntEntry("SETTINGS","mode",(FXint)mode);
  reg.writeIntEntry("SETTINGS","style",(FXint)style);
  reg.writeLongEntry("STOPWATCH","elapsed",elapsed());
  history.save(reg);
  readSpinners();
  reg.writeLongEntry("TIMER","duration",duration);
  reg.writeLongEntry("TIMER","remaining",timerRunning?(deadline-FXThread::time()):remaining);
  reg.write();
  }


FXbool StopWatch::close(FXbool notify){
  getApp()->removeTimeout(this,ID_TICK);
  if(running) stopStopwatch();
  if(timerRunning){
    timerRunning=false;
    timerDone=false;
    blinkOn=false;
    }
  saveSettings();
  return FXMainWindow::close(notify);
  }


// --------------------------------------------------------------------------
// Model

FXTime StopWatch::elapsed() const{
  return running ? accum+(FXThread::time()-started) : accum;
  }


FXTime StopWatch::timerValue() const{
  FXTime value=timerRunning ? (deadline-FXThread::time()) : remaining;
  return FXMAX((FXTime)0,value);
  }


void StopWatch::startStopwatch(){
  started=FXThread::time();
  running=true;
  rearm();
  }


void StopWatch::stopStopwatch(){
  accum+=FXThread::time()-started;
  running=false;
  history.add(FXThread::time(),accum);
  history.save(getApp()->reg());
  getApp()->reg().writeLongEntry("STOPWATCH","elapsed",accum);
  getApp()->reg().write();
  rearm();
  }


void StopWatch::readSpinners(){
  if(spinHours && spinMinutes && spinSeconds){
    duration=(((FXTime)spinHours->getValue())*3600+
              ((FXTime)spinMinutes->getValue())*60+
              ((FXTime)spinSeconds->getValue()))*1000000000;
    }
  }


void StopWatch::writeSpinners(){
  if(spinHours && spinMinutes && spinSeconds){
    FXTime total=duration/1000000000;
    spinHours->setValue((FXint)(total/3600));
    spinMinutes->setValue((FXint)((total/60)%60));
    spinSeconds->setValue((FXint)(total%60));
    }
  }


// --------------------------------------------------------------------------
// Display

void StopWatch::updateDisplay(){
  if(showAnalog()){
    FXint h=0,m=0,s=0,ms=0;
    FXdouble progress=-1.0;
    if(mode==MODE_CLOCK){
      splitLocalTime(FXThread::time(),h,m,s,ms);
      clockView->setTime(h,m,s,ms,ClockView::MODE_CLOCK);
      readout->setText("");
      }
    else if(mode==MODE_STOPWATCH){
      FXTime value=elapsed();
      FXTime secs=value/1000000000;
      h=(FXint)(secs/3600);
      m=(FXint)((secs/60)%60);
      s=(FXint)(secs%60);
      ms=(FXint)((value%1000000000)/1000000);
      clockView->setTime(h,m,s,ms,ClockView::MODE_STOPWATCH);
      readout->setText(History::formatElapsed(value));
      }
    else{
      FXTime value=timerValue();
      FXTime secs=value/1000000000;
      h=(FXint)(secs/3600);
      m=(FXint)((secs/60)%60);
      s=(FXint)(secs%60);
      ms=(FXint)((value%1000000000)/1000000);
      progress=(duration>0)?((FXdouble)value/(FXdouble)duration):0.0;
      clockView->setTime(h,m,s,ms,ClockView::MODE_TIMER,progress);
      readout->setText(formatDuration(value));
      }
    }
  else{
    if(mode==MODE_CLOCK){
      seven->setText(FXSystem::localTime(FXThread::time(),"%H:%M:%S"));
      }
    else if(mode==MODE_STOPWATCH){
      seven->setText(History::formatElapsed(elapsed()));
      }
    else{
      seven->setText(formatDuration(timerValue()));
      }
    }
  }


// Arm the one tick timer with the shortest delay anything needs right now
void StopWatch::rearm(){
  FXTime now=FXThread::time();
  FXTime delay=0;
  getApp()->removeTimeout(this,ID_TICK);
  if(mode==MODE_STOPWATCH && running){
    delay=50000000;
    }
  else if(mode==MODE_TIMER && timerRunning){
    delay=50000000;
    }
  else if(mode==MODE_TIMER && timerDone){
    delay=500000000;
    }
  else if(mode==MODE_CLOCK || (mode==MODE_STOPWATCH && showAnalog())){
    delay=1000000000-(now%1000000000);
    }
  if(delay>0) getApp()->addTimeout(this,ID_TICK,delay);
  }


long StopWatch::onTick(FXObject*,FXSelector,void*){
  // The model runs whatever mode is shown (R10)
  if(timerRunning){
    remaining=deadline-FXThread::time();
    if(remaining<=0){
      remaining=0;
      timerRunning=false;
      timerDone=true;
      blinkOn=true;
      getApp()->beep();
      }
    }
  else if(timerDone){
    blinkOn=!blinkOn;
    }
  updateDisplay();
  rearm();
  return 1;
  }


// --------------------------------------------------------------------------
// Commands

long StopWatch::onQuit(FXObject*,FXSelector,void*){
  // The first Escape only closes an open history popup
  if(historyPopup && historyPopup->shown()){
    historyPopup->popdown();
    return 1;
    }
  return close(true)?1:0;
  }


long StopWatch::onModeClock(FXObject*,FXSelector,void*){
  mode=MODE_CLOCK;
  controlSwitcher->setCurrent(0);
  updateDisplay();
  rearm();
  return 1;
  }


long StopWatch::onModeStopwatch(FXObject*,FXSelector,void*){
  mode=MODE_STOPWATCH;
  controlSwitcher->setCurrent(1);
  updateDisplay();
  rearm();
  return 1;
  }


long StopWatch::onModeTimer(FXObject*,FXSelector,void*){
  mode=MODE_TIMER;
  controlSwitcher->setCurrent(2);
  updateDisplay();
  rearm();
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


long StopWatch::onStyle(FXObject*,FXSelector,void*){
  style=(style==STYLE_DIGITAL)?STYLE_ANALOG:STYLE_DIGITAL;
  faceSwitcher->setCurrent(style);
  updateDisplay();
  rearm();
  return 1;
  }


long StopWatch::onUpdStyle(FXObject* sender,FXSelector,void*){
  FXButton* button=(FXButton*)sender;
  button->setIcon(styleIcons[style==STYLE_DIGITAL?0:1]);
  button->setTipText(style==STYLE_DIGITAL?"Switch to the round clock":"Switch to the digital display");
  button->setState(style==STYLE_ANALOG?STATE_DOWN:STATE_UP);
  return 1;
  }


long StopWatch::onStartStop(FXObject*,FXSelector,void*){
  if(running) stopStopwatch();
  else startStopwatch();
  updateDisplay();
  return 1;
  }


long StopWatch::onUpdStartStop(FXObject* sender,FXSelector,void*){
  ((FXButton*)sender)->setText(running?"Stop":"Start");
  return 1;
  }


long StopWatch::onStopwatchReset(FXObject*,FXSelector,void*){
  if(!running){
    accum=0;
    getApp()->reg().writeLongEntry("STOPWATCH","elapsed",0);
    getApp()->reg().write();
    updateDisplay();
    rearm();
    }
  return 1;
  }


long StopWatch::onUpdStopwatchReset(FXObject* sender,FXSelector,void*){
  FXButton* button=(FXButton*)sender;
  if(running) button->disable();
  else button->enable();
  return 1;
  }


long StopWatch::onTimerSpin(FXObject*,FXSelector,void*){
  if(!timerRunning && !timerDone){
    readSpinners();
    remaining=duration;
    }
  return 1;
  }


long StopWatch::onTimerStart(FXObject*,FXSelector,void*){
  if(timerRunning){
    remaining=deadline-FXThread::time();
    timerRunning=false;
    }
  else{
    if(timerDone){
      remaining=duration;
      timerDone=false;
      }
    if(remaining<=0) remaining=duration;
    if(remaining<=0) return 1;
    deadline=FXThread::time()+remaining;
    timerRunning=true;
    timerDone=false;
    blinkOn=false;
    }
  updateDisplay();
  rearm();
  return 1;
  }


long StopWatch::onUpdTimerStart(FXObject* sender,FXSelector,void*){
  ((FXButton*)sender)->setText(timerRunning?"Pause":"Start");
  return 1;
  }


long StopWatch::onTimerReset(FXObject*,FXSelector,void*){
  timerRunning=false;
  timerDone=false;
  blinkOn=false;
  readSpinners();
  remaining=duration;
  updateDisplay();
  rearm();
  return 1;
  }


long StopWatch::onUpdTimerReset(FXObject* sender,FXSelector,void*){
  FXButton* button=(FXButton*)sender;
  if(timerRunning) button->disable();
  else button->enable();
  return 1;
  }


long StopWatch::onClearHistory(FXObject*,FXSelector,void*){
  history.clear();
  history.save(getApp()->reg());
  getApp()->reg().write();
  if(historyPopup->shown()) historyPopup->popdown();
  return 1;
  }


// --------------------------------------------------------------------------
// Window moving and the context popup

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


long StopWatch::onRightPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(mode==MODE_STOPWATCH){
    if(!historyPopup->id()) historyPopup->create();
    // No row header: the row number is the first column
    historyTable->getRowHeader()->hide();
    historyTable->setRowHeaderWidth(0);
    historyTable->setTableSize(history.size(),3);
    historyTable->setColumnText(0,"#");
    historyTable->setColumnText(1,"When");
    historyTable->setColumnText(2,"Elapsed");
    for(FXint i=0;i<history.size();i++){
      FXString number;
      number.format("%d",i+1);
      historyTable->setItemText(i,0,number);
      historyTable->setItemText(i,1,History::formatWhen(history.at(i).when));
      historyTable->setItemText(i,2,History::formatElapsed(history.at(i).elapsed));
      }
    historyTable->fitColumnsToContents(0,3);
    historyTable->setColumnWidth(0,40);
    FXint w=historyPopup->getDefaultWidth();
    FXint h=historyPopup->getDefaultHeight();
    FXint x=event->root_x;
    FXint y=event->root_y;
    if(x+w>getRoot()->getWidth()) x=FXMAX(0,x-w);
    if(y+h>getRoot()->getHeight()) y=FXMAX(0,y-h);
    historyPopup->popup(nullptr,x,y);
    }
  else{
    if(!contextMenu->id()) contextMenu->create();
    if(titlebarCheck) titlebarCheck->setCheck(titlebar);
    contextMenu->popup(this,event->root_x,event->root_y);
    }
  return 1;
  }


long StopWatch::onConfigure(FXObject* sender,FXSelector sel,void* ptr){
  FXShell::onConfigure(sender,sel,ptr);
  clientWidth=getWidth();
  clientHeight=getHeight();
  layout();
  update();
  return 1;
  }


long StopWatch::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(getApp()->getBaseColor());
  dc.fillRectangle(0,0,width,height);
  return 1;
  }


long StopWatch::onShowTitlebar(FXObject*,FXSelector,void*){
  FXint oldWidth=getWidth();
  FXint oldHeight=getHeight();
  titlebar=!titlebar;
  setDecorations(titlebar?(DECOR_ALL|DECOR_RESIZE):DECOR_NONE);
  resize(oldWidth,oldHeight);
  if(titlebarCheck) titlebarCheck->setCheck(titlebar);
  place(PLACEMENT_VISIBLE);
  layout();
  update();
  getApp()->reg().writeIntEntry("SETTINGS","w",oldWidth);
  getApp()->reg().writeIntEntry("SETTINGS","h",oldHeight);
  getApp()->reg().writeIntEntry("SETTINGS","titlebar",titlebar?1:0);
  getApp()->reg().write();
  return 1;
  }


long StopWatch::onUpdShowTitlebar(FXObject* sender,FXSelector,void*){
  ((FXMenuCheck*)sender)->setCheck(titlebar);
  return 1;
  }
