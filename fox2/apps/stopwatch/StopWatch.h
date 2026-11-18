#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "fx.h"

class StopWatch : public FXMainWindow {
  FXDECLARE(StopWatch)
private:
  enum{MODE_CLOCK,MODE_STOPWATCH,MODE_TIMER};
  FXuint mode;
  FXSwitcher* switcher;
  FXLabel* modePages[3];
  FXIcon* modeIcons[3];
  FXButton* modeButtons[3];
  FXbool dragging;
  FXint dragx;
  FXint dragy;
private:
  StopWatch();
  StopWatch(const StopWatch&);
  StopWatch& operator=(const StopWatch&);
public:
  enum{
    ID_TICK=FXMainWindow::ID_LAST,
    ID_MODE_CLOCK,
    ID_MODE_STOPWATCH,
    ID_MODE_TIMER,
    ID_SHOW_TITLEBAR,
    ID_RIGHT_CLICK,
    ID_QUIT,
    ID_LAST
    };
public:
  StopWatch(FXApp* a);
  virtual ~StopWatch();
  virtual void create() override;
  virtual FXbool close(FXbool notify=true) override;
public:
  long onTick(FXObject*,FXSelector,void*);
  long onModeClock(FXObject*,FXSelector,void*);
  long onModeStopwatch(FXObject*,FXSelector,void*);
  long onModeTimer(FXObject*,FXSelector,void*);
  long onUpdMode(FXObject*,FXSelector,void*);
  long onQuit(FXObject*,FXSelector,void*);
  long onMiddlePress(FXObject*,FXSelector,void*);
  long onMiddleRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onConfigure(FXObject*,FXSelector,void*);
  long onPaint(FXObject*,FXSelector,void*);
  long onRightPress(FXObject*,FXSelector,void*);
  long onShowTitlebar(FXObject*,FXSelector,void*);
  long onUpdShowTitlebar(FXObject*,FXSelector,void*);
  };

#endif
