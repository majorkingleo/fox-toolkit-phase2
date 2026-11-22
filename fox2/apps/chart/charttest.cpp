/********************************************************************************
*                                                                               *
*                                  Chart  Test                                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "chart.h"
#include "live/LiveSource.h"
#include "icons.h"

/*
  Notes:

*/

/*******************************************************************************/


// Mini application object
class ChartWindow : public FXMainWindow {
  FXDECLARE(ChartWindow)
protected:
  FX2DPlot     *plot;
  FXMenuBar    *menubar;
  FXStatusBar  *statusbar;
  FXMenuPane   *filemenu;
  FXMenuPane   *viewmenu;
  FXMenuCheck  *livecheck;
  FXBMPImage   *image;
  LiveSource   *live;
  FXCurveList   demoCurves;
  FXFont       *captionFont;    // Caption font
  FXFont       *horzLabelFont;  // Normal horizontal font
  FXFont       *vertLabelFont;  // Vertical font
  FXFont       *horzNumberFont; // Normal horizontal font
  FXFont       *vertNumberFont; // Vertical font
protected:
  ChartWindow(){}
private:
  ChartWindow(const ChartWindow&);
  ChartWindow &operator=(const ChartWindow&);
public:
  long onCmdChart(FXObject*,FXSelector,void*);
  long onCmdFit(FXObject*,FXSelector,void*);
  long onCmdLive(FXObject*,FXSelector,void*);
public:
  enum {
    ID_CHART=FXMainWindow::ID_LAST,
    ID_FIT,
    ID_LIVE,
    ID_LAST
    };
public:
  ChartWindow(FXApp *a);
  virtual void create() override;
  virtual void startLive();
  virtual ~ChartWindow();
  };



/*******************************************************************************/

// Map
FXDEFMAP(ChartWindow) ChartWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_CHART,     ChartWindow::onCmdChart),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_FIT,       ChartWindow::onCmdFit),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_LIVE,      ChartWindow::onCmdLive),
  };


// Object implementation
FXIMPLEMENT(ChartWindow,FXMainWindow,ChartWindowMap,ARRAYNUMBER(ChartWindowMap))



// Make some windows
ChartWindow::ChartWindow(FXApp* a):FXMainWindow(a,"Chart Test",nullptr,nullptr,DECOR_ALL,20,20,700,460){

  // Menubar
  menubar=new FXMenuBar(this,FRAME_RAISED|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  statusbar=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|FRAME_RAISED|STATUSBAR_WITH_DRAGCORNER);

  // File menu
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",nullptr,getApp(),FXApp::ID_QUIT);
  new FXMenuTitle(menubar,"&File",nullptr,filemenu);

  // View menu
  viewmenu=new FXMenuPane(this);
  new FXMenuCommand(viewmenu,"&Fit data",nullptr,this,ID_FIT);
  livecheck=new FXMenuCheck(viewmenu,"&Live data",this,ID_LIVE);
  new FXMenuTitle(menubar,"&View",nullptr,viewmenu);

  // Container
  FXHorizontalFrame *container=new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED);

  // Well for chart
  FXHorizontalFrame *chartwell=new FXHorizontalFrame(container,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 0,0,0,0, 0,0);

  // Chart
  plot=new FX2DPlot(chartwell,this,ID_CHART,CAPTION_ABOVE|CAPTION_SHOWN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,20,20,20,20);

  plot->setCaption("Multi-line\ncaptions are great!");

  // Make tile pattern
  image=new FXBMPImage(getApp(),marble);

  // Make label fonts
  horzLabelFont=new FXFont(getApp(),"helvetica",14,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  vertLabelFont=new FXFont(getApp(),"helvetica",14,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  vertLabelFont->setAngle(90*64);
//vertLabelFont->setAngle(180*64);

  // Make number fonts
  horzNumberFont=new FXFont(getApp(),"helvetica",9,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  vertNumberFont=new FXFont(getApp(),"helvetica",9,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  vertNumberFont->setAngle(90*64);
//vertNumberFont->setAngle(270*64);

  // Make caption font
  captionFont=new FXFont(getApp(),"helvetica",18,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  //captionFont->setAngle(90*64);

  // Set up fill style
  FillStyle bs={image,FXRGB(128,255,255),FXRGB(128,128,255),FXRGB(200,200,200),FXRGB(128,128,128),STIPPLE_NONE,FILLSTYLE_HORIZONTAL};
  FillStyle cs={image,FXRGB(128,255,255),FXRGB(128,128,255),FXRGB(230,230,230),FXRGB(128,128,255),STIPPLE_NONE,FILLSTYLE_VERTICAL};

  // Change fill styles
  plot->setBackStyle(bs);
  plot->setChartStyle(cs);

  // Change axis labels
  plot->getAxis(FX2DChart::XLO)->setLabel("Time (s)");
  plot->getAxis(FX2DChart::XHI)->setLabel("Time (s)");
  plot->getAxis(FX2DChart::YLO)->setLabel("Value");
  plot->getAxis(FX2DChart::YHI)->setLabel("Value");

  // Set caption font
  plot->setCaptionFont(captionFont);

  // Set label fonts
  plot->getAxis(FX2DChart::XLO)->setLabelFont(horzLabelFont);
  plot->getAxis(FX2DChart::XHI)->setLabelFont(horzLabelFont);
  plot->getAxis(FX2DChart::YLO)->setLabelFont(vertLabelFont);
  plot->getAxis(FX2DChart::YHI)->setLabelFont(vertLabelFont);

  // Set number fonts
  plot->getAxis(FX2DChart::XLO)->setNumberFont(horzNumberFont);
  plot->getAxis(FX2DChart::XHI)->setNumberFont(horzNumberFont);
  plot->getAxis(FX2DChart::YLO)->setNumberFont(vertNumberFont);
  plot->getAxis(FX2DChart::YHI)->setNumberFont(vertNumberFont);

  plot->getAxis(FX2DChart::XLO)->setLabelOffset(0);
  plot->getAxis(FX2DChart::XHI)->setLabelOffset(0);
  plot->getAxis(FX2DChart::YLO)->setLabelOffset(0);
  plot->getAxis(FX2DChart::YHI)->setLabelOffset(0);

  // Change text styles
  plot->getAxis(FX2DChart::XLO)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_TOP);
  plot->getAxis(FX2DChart::XHI)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_BOTTOM);
  plot->getAxis(FX2DChart::YLO)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_RIGHT);
  plot->getAxis(FX2DChart::YHI)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_LEFT);

  // Deterministic data: three dense curves and one sparse marker curve
  {
    FXColor colors[4]={FXRGB(220,40,40),FXRGB(40,160,40),FXRGB(40,80,220),FXRGB(160,40,160)};
    FXint s,i;
    for(s=0; s<3; s++){
      FXData *xd=new FXData();
      FXData *yd=new FXData();
      FXCurve *curve=new FXCurve(plot,FXString::value("Series %d",s+1));
      LineStyle ls;
      for(i=0; i<10000; i++){
        FXdouble x=0.01*i;
        FXdouble y=50.0;
        if(s==0) y+=40.0*Math::sin(x/10.0);
        if(s==1) y+=30.0*Math::cos(x/15.0);
        if(s==2) y+=10.0*Math::sin(x/4.0)+8.0*Math::cos(x/7.0);
        xd->append(x);
        yd->append(y);
        }
      ls.color=colors[s];
      ls.weight=1;
      ls.style=LINESTYLE_SOLID;
      curve->setLineStyle(ls);
      curve->setXData(xd);
      curve->setYData(yd);
      plot->appendCurve(curve);
      demoCurves.append(curve);
      }
    {
      FXData *xd=new FXData();
      FXData *yd=new FXData();
      FXCurve *curve=new FXCurve(plot,"Markers");
      LineStyle ls;
      Marker ms;
      for(i=0; i<=20; i++){
        xd->append(5.0*i);
        yd->append(50.0+45.0*Math::sin(0.25*i));
        }
      ls.color=colors[3];
      ls.weight=1;
      ls.style=LINESTYLE_NONE;
      ms.color=colors[3];
      ms.size=7;
      ms.style=MARKER_DIAMOND|MARKER_SOLID;
      curve->setLineStyle(ls);
      curve->setMarkerStyle(ms);
      curve->setXData(xd);
      curve->setYData(yd);
      plot->appendCurve(curve);
      demoCurves.append(curve);
      }
    plot->fitData();
    live=new LiveSource(plot);
    }
  }


// Create image
void ChartWindow::create(){
  FXMainWindow::create();
  image->create();
  captionFont->create();
  horzLabelFont->create();
  vertLabelFont->create();
  horzNumberFont->create();
  vertNumberFont->create();
  }


// Command from chart
long ChartWindow::onCmdChart(FXObject*,FXSelector,void*){
  return 1;
  }


// Fit all data into the view
long ChartWindow::onCmdFit(FXObject*,FXSelector,void*){
  plot->fitData();
  plot->update();
  return 1;
  }


// Start or stop the live data mode
long ChartWindow::onCmdLive(FXObject*,FXSelector,void*){
  FXint i;
  if(live->isRunning()){
    live->stop();
    for(i=0; i<demoCurves.no(); i++){
      demoCurves[i]->setVisible(true);
      }
    plot->getAxis(FX2DChart::YLO)->setLabel("Value");
    plot->getAxis(FX2DChart::YHI)->setLabel("Value");
    plot->fitData();
    livecheck->setCheck(false);
    }
  else{
    startLive();
    }
  plot->update();
  return 1;
  }


// Switch to live data
void ChartWindow::startLive(){
  FXint i;
  for(i=0; i<demoCurves.no(); i++){
    demoCurves[i]->setVisible(false);
    }
  plot->getAxis(FX2DChart::YLO)->setLabel("Mouse (px/ms)");
  plot->getAxis(FX2DChart::YHI)->setLabel("CPU % / MB");
  live->clear();
  live->start();
  livecheck->setCheck(true);
  plot->update();
  }


// Clean up
ChartWindow::~ChartWindow(){
  delete live;
  delete filemenu;
  delete viewmenu;
  delete image;
  delete captionFont;
  delete horzLabelFont;
  delete vertLabelFont;
  delete horzNumberFont;
  delete vertNumberFont;
  }


/*******************************************************************************/


// Start the whole thing
int main(int argc,char *argv[]){
  FXbool live=false;
  FXint i,j;

  // Pull out --live before FXApp parses the options
  for(i=1,j=1; i<argc; i++){
    if(strcmp(argv[i],"--live")==0) live=true;
    else argv[j++]=argv[i];
    }
  argc=j;
  argv[argc]=nullptr;

  // Make application
  FXApp application("ChartWindow","FoxTest");

  // Open display
  application.init(argc,argv);

  // Main window
  ChartWindow* window=new ChartWindow(&application);

  // Create app
  application.create();

  // Show it
  window->show(PLACEMENT_SCREEN);

  // Start live mode when asked
  if(live) window->startLive();

  // Run
  return application.run();
  }
