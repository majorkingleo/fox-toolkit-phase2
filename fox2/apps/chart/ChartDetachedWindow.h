#ifndef CHARTDETACHEDWINDOW_H
#define CHARTDETACHEDWINDOW_H

#include "ChartDemoWindow.h"

namespace FXCHARTDEMO {

class ChartDetachedWindow : public FXMainWindow {
  FXDECLARE(ChartDetachedWindow)
protected:
  ChartDemoWindow* chartWindow;
  ChartDocument* document;
protected:
  ChartDetachedWindow(){ }
private:
  ChartDetachedWindow(const ChartDetachedWindow&);
  ChartDetachedWindow& operator=(const ChartDetachedWindow&);
public:
  ChartDetachedWindow(FXApp* app,const FXString& title,ChartDemoWindow* owner,ChartDocument* chart);
  virtual void create() override;
  virtual void applyTheme(FXTheme* theme) override;
  };

}

#endif
