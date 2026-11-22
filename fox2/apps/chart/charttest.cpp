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
  FXMenuRadio  *staticRadio;
  FXMenuRadio  *liveRadio;
  FXVerticalFrame *livepanel;
  FXTextField  *ratefield;
  FXTextField  *windowfield;
  FXLabel      *livebuffer;
  FXLabel      *livetime;
  FXLabel      *livespeed;
  FXLabel      *liveaccel;
  FXLabel      *livecpu;
  FXLabel      *livememory;
  FXLabel      *liveframe;
  FXBMPImage   *image;
  LiveSource   *live;
  FXCurveList   demoCurves;
  FillStyle     savedbackstyle;
  FillStyle     savedchartstyle;
  FXuint        savedcaptionstyle;
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
  long onCmdStatic(FXObject*,FXSelector,void*);
  long onCmdLive(FXObject*,FXSelector,void*);
  long onCmdRate(FXObject*,FXSelector,void*);
  long onCmdWindow(FXObject*,FXSelector,void*);
  long onLiveSample(FXObject*,FXSelector,void*);
public:
  enum {
    ID_CHART=FXMainWindow::ID_LAST,
    ID_STATIC,
    ID_LIVE,
    ID_RATE,
    ID_WINDOW,
    ID_LIVE_SAMPLE,
    ID_LAST
    };
public:
  ChartWindow(FXApp *a);
  virtual void create() override;
  virtual void startLive();
  virtual void stopLive();
protected:
  void updateRadios();
  virtual ~ChartWindow();
  };



/*******************************************************************************/

// Map
FXDEFMAP(ChartWindow) ChartWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_CHART,     ChartWindow::onCmdChart),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_STATIC,    ChartWindow::onCmdStatic),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_LIVE,      ChartWindow::onCmdLive),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_RATE,      ChartWindow::onCmdRate),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_WINDOW,    ChartWindow::onCmdWindow),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_LIVE_SAMPLE,ChartWindow::onLiveSample),
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

  // View menu: one radio group for the two data modes
  viewmenu=new FXMenuPane(this);
  staticRadio=new FXMenuRadio(viewmenu,"&Static data",this,ID_STATIC);
  liveRadio=new FXMenuRadio(viewmenu,"&Live data",this,ID_LIVE);
  staticRadio->setCheck(true);
  new FXMenuTitle(menubar,"&View",nullptr,viewmenu);

  // Container
  FXHorizontalFrame *container=new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED);

  // Well for chart
  FXHorizontalFrame *chartwell=new FXHorizontalFrame(container,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 0,0,0,0, 0,0);

  // Live buffer readout, shown only in live mode
  livepanel=new FXVerticalFrame(container,LAYOUT_FILL_Y|FRAME_RAISED,0,0,190,0,4,4,4,4);
  new FXLabel(livepanel,"Live buffer",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  new FXLabel(livepanel,"Sample rate (Hz)",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  ratefield=new FXTextField(livepanel,6,this,ID_RATE,TEXTFIELD_REAL|TEXTFIELD_NORMAL|LAYOUT_FILL_X);
  ratefield->setText("20");
  ratefield->setJustify(JUSTIFY_RIGHT);
  new FXLabel(livepanel,"Viewport (s)",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  windowfield=new FXTextField(livepanel,6,this,ID_WINDOW,TEXTFIELD_REAL|TEXTFIELD_NORMAL|LAYOUT_FILL_X);
  windowfield->setText("10");
  windowfield->setJustify(JUSTIFY_RIGHT);
  livebuffer=new FXLabel(livepanel,"Buffer: 0 samples",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livetime=new FXLabel(livepanel,"Time: 0.0 s",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livespeed=new FXLabel(livepanel,"Speed: 0.00 px/ms",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  liveaccel=new FXLabel(livepanel,"Accel: 0.00 px/ms2",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livecpu=new FXLabel(livepanel,"CPU: 0.0 %",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livememory=new FXLabel(livepanel,"Memory: 0.0 MB",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  liveframe=new FXLabel(livepanel,"Frame: 0.00 ms",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livepanel->hide();

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
    live->setInterval(50000000);
    live->setWindow(10.0);
    live->setTarget(this,FXSEL(SEL_COMMAND,ID_LIVE_SAMPLE));
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

  // Keep the sidebar wide enough for the largest buffer readout
  livebuffer->setText("Buffer: 20000 samples");
  livepanel->setLayoutHints(LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y);
  livepanel->setWidth(livebuffer->getDefaultWidth()+2*4+2*2+2);
  livebuffer->setText("Buffer: 0 samples");
  }


// Solid fills for the live view; gradients cost more than the data update
static const FillStyle liveBackStyle={nullptr,FXRGB(235,235,235),FXRGBA(0,0,0,0),FXRGB(255,255,255),FXRGB(255,255,255),STIPPLE_NONE,FILLSTYLE_SOLID};
static const FillStyle liveChartStyle={nullptr,FXRGB(255,255,255),FXRGBA(0,0,0,0),FXRGB(255,255,255),FXRGB(255,255,255),STIPPLE_NONE,FILLSTYLE_SOLID};


// Command from chart
long ChartWindow::onCmdChart(FXObject*,FXSelector,void*){
  return 1;
  }


// Switch to the static data mode
long ChartWindow::onCmdStatic(FXObject*,FXSelector,void*){
  if(live->isRunning()){
    stopLive();
    }
  else{
    updateRadios();
    }
  return 1;
  }


// Switch to the live data mode
long ChartWindow::onCmdLive(FXObject*,FXSelector,void*){
  if(!live->isRunning()){
    startLive();
    }
  else{
    updateRadios();
    }
  return 1;
  }


// Change the sample rate from the sidebar
long ChartWindow::onCmdRate(FXObject*,FXSelector,void*){
  FXbool ok=false;
  FXdouble hz=ratefield->getText().toDouble(&ok);
  if(!ok || hz<0.1 || hz>1000.0){
    ratefield->setText(FXString::value("%g",1000000000.0/live->getInterval()));
    return 1;
    }
  live->setInterval((FXTime)(1000000000.0/hz));
  ratefield->setText(FXString::value("%g",hz));
  return 1;
  }


// Change the viewport length from the sidebar
long ChartWindow::onCmdWindow(FXObject*,FXSelector,void*){
  FXbool ok=false;
  FXdouble seconds=windowfield->getText().toDouble(&ok);
  if(!ok || seconds<1.0 || seconds>3600.0){
    windowfield->setText(FXString::value("%g",live->getWindow()));
    return 1;
    }
  live->setWindow(seconds);
  windowfield->setText(FXString::value("%g",seconds));
  return 1;
  }


// Switch to live data
void ChartWindow::startLive(){
  FXint i;
  if(live->isRunning()){
    updateRadios();
    return;
    }
  for(i=0; i<demoCurves.no(); i++){
    demoCurves[i]->setVisible(false);
    }
  savedbackstyle=plot->getBackStyle();
  savedchartstyle=plot->getChartStyle();
  savedcaptionstyle=plot->getCaptionStyle();
  plot->setCaptionStyle(CAPTION_NONE);
  plot->setBackStyle(liveBackStyle);
  plot->setChartStyle(liveChartStyle);
  plot->getAxis(FX2DChart::YLO)->setLabel("Mouse (px/ms)");
  plot->getAxis(FX2DChart::YHI)->setLabel("CPU % / MB");
  livebuffer->setText("Buffer: 0 samples");
  livetime->setText("Time: 0.0 s");
  livespeed->setText("Speed: 0.00 px/ms");
  liveaccel->setText("Accel: 0.00 px/ms2");
  livecpu->setText("CPU: 0.0 %");
  livememory->setText("Memory: 0.0 MB");
  liveframe->setText("Frame: 0.00 ms");
  livepanel->show();
  onCmdRate(nullptr,0,nullptr);
  onCmdWindow(nullptr,0,nullptr);
  live->clear();
  live->start();
  updateRadios();
  }


// Switch back to static data
void ChartWindow::stopLive(){
  FXint i;
  if(!live->isRunning()){
    updateRadios();
    return;
    }
  live->stop();
  for(i=0; i<demoCurves.no(); i++){
    demoCurves[i]->setVisible(true);
    }
  plot->getAxis(FX2DChart::YLO)->setLabel("Value");
  plot->getAxis(FX2DChart::YHI)->setLabel("Value");
  plot->setBackStyle(savedbackstyle);
  plot->setChartStyle(savedchartstyle);
  plot->setCaptionStyle(savedcaptionstyle);
  plot->fitData();
  livepanel->hide();
  updateRadios();
  }


// Show the current sample and buffer size
long ChartWindow::onLiveSample(FXObject*,FXSelector,void* ptr){
  LiveSample *sample=(LiveSample*)ptr;
  livebuffer->setText(FXString::value("Buffer: %d samples",sample->count));
  livetime->setText(FXString::value("Time: %.1f s",sample->time));
  livespeed->setText(FXString::value("Speed: %.2f px/ms",sample->speed));
  liveaccel->setText(FXString::value("Accel: %.2f px/ms2",sample->accel));
  livecpu->setText(FXString::value("CPU: %.1f %%",sample->cpu));
  livememory->setText(FXString::value("Memory: %.1f MB",sample->memory));
  liveframe->setText(FXString::value("Frame: %.2f ms",sample->frame));
  return 1;
  }


// Keep the two view-mode radios mutually exclusive
void ChartWindow::updateRadios(){
  FXbool running=live->isRunning();
  staticRadio->setCheck(!running);
  liveRadio->setCheck(running);
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
