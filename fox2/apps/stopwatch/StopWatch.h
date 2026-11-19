#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "fx.h"
#include "History.h"

class ClockView;

class StopWatch : public FXMainWindow {
  FXDECLARE(StopWatch)
private:
  enum{ MODE_CLOCK,MODE_STOPWATCH,MODE_TIMER };
  enum{ STYLE_DIGITAL,STYLE_ANALOG };
private:
  // View
  FXuint mode;
  FXuint style;
  FXSwitcher* faceSwitcher;
  FX7Segment* seven;
  ClockView* clockView;
  FXSwitcher* controlSwitcher;
  FXLabel* readout;            // value under the analog face
  FXButton* startStopButton;
  FXButton* stopwatchResetButton;
  FXSpinner* spinHours;
  FXSpinner* spinMinutes;
  FXSpinner* spinSeconds;
  FXButton* timerStartButton;
  FXButton* timerResetButton;
  FXIcon* modeIcons[3];
  FXButton* modeButtons[3];
  FXIcon* styleIcons[2];
  FXButton* styleButton;
  // Model, independent of the shown mode
  FXbool running;
  FXTime accum;
  FXTime started;
  FXTime duration;
  FXTime remaining;
  FXTime deadline;
  FXbool timerRunning;
  FXbool timerDone;
  FXbool blinkOn;
  History history;
  // Window
  FXbool titlebar;
  FXint clientWidth;
  FXint clientHeight;
  FXbool dragging;
  FXint dragx;
  FXint dragy;
  FXMenuPane* contextMenu;
  FXMenuCheck* titlebarCheck;
  FXPopup* historyPopup;
  FXTable* historyTable;
private:
  StopWatch();
  StopWatch(const StopWatch&);
  StopWatch& operator=(const StopWatch&);
  FXTime elapsed() const;
  FXTime timerValue() const;
  void loadSettings();
  void saveSettings();
  void updateDisplay();
  void rearm();
  void startStopwatch();
  void stopStopwatch();
  void readSpinners();
  void writeSpinners();
  FXbool showAnalog() const { return style==STYLE_ANALOG; }
public:
  enum{
    ID_TICK=FXMainWindow::ID_LAST,
    ID_MODE_CLOCK,
    ID_MODE_STOPWATCH,
    ID_MODE_TIMER,
    ID_STYLE,
    ID_STARTSTOP,
    ID_STOPWATCH_RESET,
    ID_TIMER_START,
    ID_TIMER_RESET,
    ID_TIMER_SPIN,
    ID_CLEAR_HISTORY,
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
  long onStyle(FXObject*,FXSelector,void*);
  long onUpdStyle(FXObject*,FXSelector,void*);
  long onStartStop(FXObject*,FXSelector,void*);
  long onUpdStartStop(FXObject*,FXSelector,void*);
  long onStopwatchReset(FXObject*,FXSelector,void*);
  long onUpdStopwatchReset(FXObject*,FXSelector,void*);
  long onTimerSpin(FXObject*,FXSelector,void*);
  long onTimerStart(FXObject*,FXSelector,void*);
  long onUpdTimerStart(FXObject*,FXSelector,void*);
  long onTimerReset(FXObject*,FXSelector,void*);
  long onUpdTimerReset(FXObject*,FXSelector,void*);
  long onClearHistory(FXObject*,FXSelector,void*);
  long onQuit(FXObject*,FXSelector,void*);
  long onUnpost(FXObject*,FXSelector,void*);
  long onMiddlePress(FXObject*,FXSelector,void*);
  long onMiddleRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onConfigure(FXObject*,FXSelector,void*);
  long onPaint(FXObject*,FXSelector,void*);
  long onRightPress(FXObject*,FXSelector,void*);
  long onShowTitlebar(FXObject*,FXSelector,void*);
  long onUpdShowTitlebar(FXObject*,FXSelector,void*);
  void routeContextEvents();
  };

#endif
