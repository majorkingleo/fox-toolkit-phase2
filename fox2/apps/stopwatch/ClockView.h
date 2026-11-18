#ifndef CLOCKVIEW_H
#define CLOCKVIEW_H

#include "fx.h"

// Analog face: draws a round clock from three time values and a mode.
// It does not know about clocks, timers or the application.
class ClockView : public FXCanvas {
  FXDECLARE(ClockView)
public:
  enum{ MODE_CLOCK,MODE_STOPWATCH,MODE_TIMER };
private:
  FXint hour;
  FXint minute;
  FXint second;
  FXint milli;
  FXuint clockmode;
  FXdouble progress;    // timer wedge, 0..1; negative = no wedge
  FXImage* big;         // face drawn at SUPERSAMPLE times the widget size
  FXImage* face;        // averaged face that is drawn to the widget
private:
  ClockView();
  ClockView(const ClockView&);
  ClockView& operator=(const ClockView&);
public:
  ClockView(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=FRAME_NORMAL);
  virtual FXint getDefaultWidth() override;
  virtual FXint getDefaultHeight() override;
  // Set the shown time; update() only when a value changed
  void setTime(FXint h,FXint m,FXint s,FXint ms,FXuint mode,FXdouble frac=-1.0);
private:
  FXImage* makeImage(FXint w,FXint h);
  void drawFace(FXDCWindow& dc,FXint w,FXint h,FXint s);
  void shrink(const FXColor* src,FXint sw,FXColor* dst,FXint w,FXint h,FXint s);
public:
  virtual ~ClockView() override;
  long onPaint(FXObject*,FXSelector,void*);
  };

#endif
