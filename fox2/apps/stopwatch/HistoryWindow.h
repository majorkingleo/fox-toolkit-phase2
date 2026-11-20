#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include "fx.h"
#include "History.h"

class StopwatchHistoryWindow : public FXDialogBox {
private:
  const History* history;
  FXTable* table;
  FXbool sized;
private:
  StopwatchHistoryWindow();
  StopwatchHistoryWindow(const StopwatchHistoryWindow&);
  StopwatchHistoryWindow& operator=(const StopwatchHistoryWindow&);
public:
  StopwatchHistoryWindow(FXWindow* owner,const History& data,FXObject* target,FXSelector selector);
  void showHistory();
  void refresh();
  };

#endif
