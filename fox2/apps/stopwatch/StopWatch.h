#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "fx.h"

class StopWatch : public FXMainWindow {
  FXDECLARE(StopWatch)
private:
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
  long onQuit(FXObject*,FXSelector,void*);
  long onMiddlePress(FXObject*,FXSelector,void*);
  long onMiddleRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onConfigure(FXObject*,FXSelector,void*);
  long onPaint(FXObject*,FXSelector,void*);
  };

#endif
