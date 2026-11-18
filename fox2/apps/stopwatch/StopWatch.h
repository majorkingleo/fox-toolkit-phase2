#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "fx.h"

class StopWatch : public FXMainWindow {
  FXDECLARE(StopWatch)
private:
  FXLabel* clockLabel;
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
  long onKeyPress(FXObject*,FXSelector,void*);
  };

#endif
