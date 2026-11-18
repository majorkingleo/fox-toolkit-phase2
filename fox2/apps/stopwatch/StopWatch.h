#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "fx.h"

class ClockCanvas : public FXCanvas {
  FXDECLARE(ClockCanvas)
private:
  FXString text;
private:
  ClockCanvas();
  ClockCanvas(const ClockCanvas&);
  ClockCanvas& operator=(const ClockCanvas&);
public:
  ClockCanvas(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=FRAME_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  void setText(const FXString& value);
  long onPaint(FXObject*,FXSelector,void*);
  };


class StopWatch : public FXMainWindow {
  FXDECLARE(StopWatch)
private:
  ClockCanvas* clockLabel;
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
  };

#endif
