#include "ChartDetachedWindow.h"

namespace FXCHARTDEMO {

using namespace FX;
using namespace FXCHART;


FXIMPLEMENT(ChartDetachedWindow,FXMainWindow,nullptr,0)


ChartDetachedWindow::ChartDetachedWindow(FXApp* application,const FXString& windowTitle,ChartDemoWindow* ownerWindow,ChartDocument* chart):FXMainWindow(application,windowTitle,nullptr,nullptr,DECOR_ALL,40,40,1000,700){
  chartWindow=ownerWindow;
  document=chart;
  }


void ChartDetachedWindow::create(){
  FXMainWindow::create();
  chartWindow->updateDetachedChartFonts(document,this);
  }


void ChartDetachedWindow::applyTheme(FXTheme* theme){
  FXMainWindow::applyTheme(theme);
  chartWindow->updateDetachedChartFonts(document,this);
  }

}
