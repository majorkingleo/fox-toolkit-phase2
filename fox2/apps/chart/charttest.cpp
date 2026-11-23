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

/*
  Notes:

*/

/*******************************************************************************/


// Mini application object
class ChartWindow : public FXMainWindow {
  FXDECLARE(ChartWindow)
protected:
  FX2DPlot     *plot;
  FXScrollBar  *xscroll;
  FXMenuBar    *menubar;
  FXStatusBar  *statusbar;
  FXMenuPane   *filemenu;
  FXMenuPane   *viewmenu;
  FXMenuRadio  *staticRadio;
  FXMenuRadio  *liveRadio;
  FXVerticalFrame *livepanel;
  FXVerticalFrame *staticseries;
  FXVerticalFrame *liveseries;
  FXVerticalFrame *livesection;
  FXButton     *zoomreset;
  FXCheckButton *demochecks[16];
  FXCheckButton *livechecks[16];
  FXbool        demovis[16];
  FXbool        livevis[16];
  FXTextField  *ratefield;
  FXTextField  *windowfield;
  FXLabel      *livebuffer;
  FXLabel      *livetime;
  FXLabel      *livespeed;
  FXLabel      *liveaccel;
  FXLabel      *livecpu;
  FXLabel      *livememory;
  FXLabel      *liveframe;
  LiveSource   *live;
  FXCurveList   demoCurves;
  FillStyle     liveBackStyle;   // Solid theme fills for the live view
  FillStyle     liveChartStyle;
  FillStyle     savedbackstyle;
  FillStyle     savedchartstyle;
  FXuint        savedcaptionstyle;
  FXFont       *captionFont;    // Caption font
  FXFont       *horzLabelFont;  // Normal horizontal font
  FXFont       *vertLabelFont;  // Vertical font
  FXFont       *horzNumberFont; // Normal horizontal font
  FXFont       *vertNumberFont; // Vertical font
  FXFontDesc    captionFontDesc;
  FXFontDesc    horzLabelFontDesc;
  FXFontDesc    vertLabelFontDesc;
  FXFontDesc    horzNumberFontDesc;
  FXFontDesc    vertNumberFontDesc;
  FXint         fontDPI;
  FXdouble      fontScale;
protected:
  ChartWindow(){}
private:
  ChartWindow(const ChartWindow&);
  ChartWindow &operator=(const ChartWindow&);
public:
  long onCmdChart(FXObject*,FXSelector,void*);
  long onChartZoom(FXObject*,FXSelector,void*);
  long onCmdResetZoom(FXObject*,FXSelector,void*);
  long onCmdStatic(FXObject*,FXSelector,void*);
  long onCmdLive(FXObject*,FXSelector,void*);
  long onCmdRate(FXObject*,FXSelector,void*);
  long onCmdWindow(FXObject*,FXSelector,void*);
  long onScroll(FXObject*,FXSelector,void*);
  long onSeries(FXObject*,FXSelector,void*);
  long onLiveSample(FXObject*,FXSelector,void*);
public:
  enum {
    ID_CHART=FXMainWindow::ID_LAST,
    ID_STATIC,
    ID_LIVE,
    ID_RATE,
    ID_WINDOW,
    ID_SCROLL,
    ID_LIVE_SAMPLE,
    ID_RESETZOOM,
    ID_SERIES,
    ID_SERIES_LAST=ID_SERIES+15,
    ID_LAST
    };
public:
  ChartWindow(FXApp *a);
  virtual void create() override;
  virtual void applyTheme(FXTheme* theme) override;
  virtual void startLive();
  virtual void stopLive();
protected:
  void updateRadios();
  void updateScroll();
  void updateZoomButton();
  void updateChartFonts();
  void addSeriesRow(FXVerticalFrame* box,FXCurve* curve,FXint id,FXCheckButton*& check);
  void syncSeriesChecks();
  virtual ~ChartWindow();
  };



/*******************************************************************************/

// Map
FXDEFMAP(ChartWindow) ChartWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_CHART,     ChartWindow::onCmdChart),
  FXMAPFUNC(SEL_CHANGED,  ChartWindow::ID_CHART,     ChartWindow::onChartZoom),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_STATIC,    ChartWindow::onCmdStatic),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_LIVE,      ChartWindow::onCmdLive),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_RATE,      ChartWindow::onCmdRate),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_WINDOW,    ChartWindow::onCmdWindow),
  FXMAPFUNC(SEL_CHANGED,  ChartWindow::ID_SCROLL,    ChartWindow::onScroll),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_SCROLL,    ChartWindow::onScroll),
  FXMAPFUNCS(SEL_COMMAND, ChartWindow::ID_SERIES,    ChartWindow::ID_SERIES_LAST, ChartWindow::onSeries),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_RESETZOOM,ChartWindow::onCmdResetZoom),
  FXMAPFUNC(SEL_COMMAND,  ChartWindow::ID_LIVE_SAMPLE,ChartWindow::onLiveSample),
  };


// Object implementation
FXIMPLEMENT(ChartWindow,FXMainWindow,ChartWindowMap,ARRAYNUMBER(ChartWindowMap))


static FXFont* makeChartFont(FXApp* app,const FXFontDesc& desc,FXint angle,FXint dpi,FXdouble scale){
  FXFontDesc scaled=desc;
  scaled.size=FXint(FXdouble(desc.size)*(scale/(FXdouble(dpi)/96.0))+0.5);
  FXFont* font=new FXFont(app,scaled);
  if(angle) font->setAngle(angle);
  font->setResolution(dpi);
  font->create();
  return font;
  }



// Make some windows
ChartWindow::ChartWindow(FXApp* a):FXMainWindow(a,"Chart Test",nullptr,nullptr,DECOR_ALL,20,20,700,540){
  fontDPI=0;
  fontScale=0.0;

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

  // Right sidebar: series list and, in live mode, the live controls
  livepanel=new FXVerticalFrame(container,LAYOUT_FILL_Y|FRAME_RAISED,0,0,190,0,4,4,4,4);
  new FXLabel(livepanel,"Series",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  staticseries=new FXVerticalFrame(livepanel,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,0,0);
  liveseries=new FXVerticalFrame(livepanel,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,0,0);
  livesection=new FXVerticalFrame(livepanel,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,0,2);
  new FXLabel(livesection,"Live buffer",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  new FXLabel(livesection,"Sample rate (Hz)",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  ratefield=new FXTextField(livesection,6,this,ID_RATE,TEXTFIELD_REAL|TEXTFIELD_NORMAL|LAYOUT_FILL_X);
  ratefield->setText("20");
  ratefield->setJustify(JUSTIFY_RIGHT);
  new FXLabel(livesection,"Viewport (s)",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  windowfield=new FXTextField(livesection,6,this,ID_WINDOW,TEXTFIELD_REAL|TEXTFIELD_NORMAL|LAYOUT_FILL_X);
  windowfield->setText("10");
  windowfield->setJustify(JUSTIFY_RIGHT);
  livebuffer=new FXLabel(livesection,"Buffer: 0 samples",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livetime=new FXLabel(livesection,"Time: 0.0 s",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livespeed=new FXLabel(livesection,"Speed: 0.00 px/ms",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  liveaccel=new FXLabel(livesection,"Accel: 0.00 px/ms2",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livecpu=new FXLabel(livesection,"CPU: 0.0 %",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  livememory=new FXLabel(livesection,"Memory: 0.0 MB",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  liveframe=new FXLabel(livesection,"Frame: 0.00 ms",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  zoomreset=new FXButton(livepanel,"Reset zoom",nullptr,this,ID_RESETZOOM,BUTTON_NORMAL|LAYOUT_BOTTOM|LAYOUT_FILL_X);
  liveseries->hide();
  livesection->hide();

  // Chart, with a scrollbar underneath it to pan the visible time range
  FXVerticalFrame *chartcolumn=new FXVerticalFrame(chartwell,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  plot=new FX2DPlot(chartcolumn,this,ID_CHART,CAPTION_ABOVE|CAPTION_SHOWN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,20,20,20,20);
  xscroll=new FXScrollBar(chartcolumn,this,ID_SCROLL,SCROLLBAR_HORIZONTAL|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM,0,0,0,0);
  xscroll->setLine(1000);

  plot->setCaption("Multi-line\ncaptions are great!");

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
  captionFontDesc=captionFont->getFontDesc();
  horzLabelFontDesc=horzLabelFont->getFontDesc();
  vertLabelFontDesc=vertLabelFont->getFontDesc();
  horzNumberFontDesc=horzNumberFont->getFontDesc();
  vertNumberFontDesc=vertNumberFont->getFontDesc();

  // Fill styles follow the theme: the frame is the window background, the
  // plot area is the base color with a light gradient towards the top
  FXColor back=getApp()->getBackColor();
  FXColor base=getApp()->getBaseColor();
  FXColor hilite=getApp()->getHiliteColor();
  FXColor shadow=getApp()->getShadowColor();
  FillStyle bs={nullptr,back,FXRGBA(0,0,0,0),shadow,back,STIPPLE_NONE,FILLSTYLE_HORIZONTAL};
  FillStyle cs={nullptr,base,FXRGBA(0,0,0,0),base,hilite,STIPPLE_NONE,FILLSTYLE_VERTICAL};

  // Change fill styles
  plot->setBackStyle(bs);
  plot->setChartStyle(cs);

  // The live view fills solid, which is much cheaper than a gradient
  liveBackStyle.image=nullptr;
  liveBackStyle.forecolor=back;
  liveBackStyle.backcolor=FXRGBA(0,0,0,0);
  liveBackStyle.lower=liveBackStyle.upper=back;
  liveBackStyle.hatch=STIPPLE_NONE;
  liveBackStyle.style=FILLSTYLE_SOLID;
  liveChartStyle.image=nullptr;
  liveChartStyle.forecolor=base;
  liveChartStyle.backcolor=FXRGBA(0,0,0,0);
  liveChartStyle.lower=liveChartStyle.upper=base;
  liveChartStyle.hatch=STIPPLE_NONE;
  liveChartStyle.style=FILLSTYLE_SOLID;

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
      ls.color=readableColor(colors[s],base);
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
      ls.color=readableColor(colors[3],base);
      ls.weight=1;
      ls.style=LINESTYLE_NONE;
      ms.color=readableColor(colors[3],base);
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
    for(i=0; i<demoCurves.no(); i++){
      addSeriesRow(staticseries,demoCurves[i],i,demochecks[i]);
      }
    for(i=0; i<live->getNumSeries(); i++){
      addSeriesRow(liveseries,live->getCurve(i),demoCurves.no()+i,livechecks[i]);
      livevis[i]=true;
      }
    }
  }


// Create window and fonts
void ChartWindow::create(){
  FXint panelwidth;
  FXMainWindow::create();
  updateChartFonts();

  // Keep the sidebar wide enough for the widest series row and buffer readout
  staticseries->show();
  liveseries->show();
  livesection->show();
  livebuffer->setText("Buffer: 20000 samples");
  panelwidth=livepanel->getDefaultWidth();
  livebuffer->setText("Buffer: 0 samples");
  livepanel->setLayoutHints(LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y);
  livepanel->setWidth(panelwidth);
  liveseries->hide();
  livesection->hide();
  updateScroll();
  xscroll->hide();
  xscroll->disable();
  updateZoomButton();
  }


// Recreate application-owned fonts for this window's current display scale
void ChartWindow::updateChartFonts(){
  FXint windowdpi=getWindowDPI();
  FXdouble scale=getDisplayScale();
  FXFont *oldCaption,*oldHorzLabel,*oldVertLabel,*oldHorzNumber,*oldVertNumber;
  if(windowdpi<=0) windowdpi=96;
  if(windowdpi==fontDPI && scale==fontScale) return;
  oldCaption=captionFont;
  oldHorzLabel=horzLabelFont;
  oldVertLabel=vertLabelFont;
  oldHorzNumber=horzNumberFont;
  oldVertNumber=vertNumberFont;
  captionFont=makeChartFont(getApp(),captionFontDesc,oldCaption->getAngle(),windowdpi,scale);
  horzLabelFont=makeChartFont(getApp(),horzLabelFontDesc,oldHorzLabel->getAngle(),windowdpi,scale);
  vertLabelFont=makeChartFont(getApp(),vertLabelFontDesc,oldVertLabel->getAngle(),windowdpi,scale);
  horzNumberFont=makeChartFont(getApp(),horzNumberFontDesc,oldHorzNumber->getAngle(),windowdpi,scale);
  vertNumberFont=makeChartFont(getApp(),vertNumberFontDesc,oldVertNumber->getAngle(),windowdpi,scale);
  plot->setCaptionFont(captionFont);
  plot->getAxis(FX2DChart::XLO)->setLabelFont(horzLabelFont);
  plot->getAxis(FX2DChart::XHI)->setLabelFont(horzLabelFont);
  plot->getAxis(FX2DChart::YLO)->setLabelFont(vertLabelFont);
  plot->getAxis(FX2DChart::YHI)->setLabelFont(vertLabelFont);
  plot->getAxis(FX2DChart::XLO)->setNumberFont(horzNumberFont);
  plot->getAxis(FX2DChart::XHI)->setNumberFont(horzNumberFont);
  plot->getAxis(FX2DChart::YLO)->setNumberFont(vertNumberFont);
  plot->getAxis(FX2DChart::YHI)->setNumberFont(vertNumberFont);
  delete oldCaption;
  delete oldHorzLabel;
  delete oldVertLabel;
  delete oldHorzNumber;
  delete oldVertNumber;
  fontDPI=windowdpi;
  fontScale=scale;
  }


// Refresh chart fonts when the window moves to a monitor with another DPI
void ChartWindow::applyTheme(FXTheme* theme){
  FXMainWindow::applyTheme(theme);
  updateChartFonts();
  }


// Command from chart
long ChartWindow::onCmdChart(FXObject*,FXSelector,void*){
  return 1;
  }


// The chart zoomed; drive the live viewport from the new time range
long ChartWindow::onChartZoom(FXObject*,FXSelector,void*){
  if(live->isRunning()){
    FXdouble lo=plot->getAxis(FX2DChart::XLO)->getAxisRangeMin();
    FXdouble hi=plot->getAxis(FX2DChart::XLO)->getAxisRangeMax();
    live->setWindow(hi-lo);
    live->setViewPosition(lo);
    windowfield->setText(FXString::value("%.3g",live->getWindow()));
    updateScroll();
    }
  updateZoomButton();
  return 1;
  }


// Restore the ranges in effect before the first zoom
long ChartWindow::onCmdResetZoom(FXObject*,FXSelector,void*){
  plot->resetZoom();
  updateZoomButton();
  return 1;
  }


// Grey out the reset button while nothing is zoomed
void ChartWindow::updateZoomButton(){
  if(plot->isZoomed()) zoomreset->enable();
  else zoomreset->disable();
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
  updateScroll();
  return 1;
  }


// Update the scrollbar to the full time range and the visible window
void ChartWindow::updateScroll(){
  FXint range=1,page=1,pos=0;
  if(live->isRunning()){
    range=(FXint)(live->getTime()*1000.0+0.5);
    page=(FXint)(live->getWindow()*1000.0+0.5);
    pos=(FXint)(live->getViewLeft()*1000.0+0.5);
    if(range<1) range=1;
    if(page<1) page=1;
    }
  xscroll->setRange(range,false);
  xscroll->setPage(page,false);
  xscroll->setPosition(pos,false);
  }


// Drag or step the viewport through the retained history
long ChartWindow::onScroll(FXObject*,FXSelector,void* ptr){
  if(live->isRunning()){
    FXint pos=(FXint)(FXival)ptr;
    FXint end=xscroll->getRange()-xscroll->getPage();
    if(pos>=end-xscroll->getLine()){
      live->setViewPosition(live->getViewMax());
      }
    else{
      live->setViewPosition((FXdouble)pos/1000.0);
      }
    updateScroll();
    }
  return 1;
  }


// Add one series row: color swatch, label, visibility check
void ChartWindow::addSeriesRow(FXVerticalFrame* box,FXCurve* curve,FXint id,FXCheckButton*& check){
  if(curve==nullptr){
    check=nullptr;
    return;
    }
  FXHorizontalFrame *row=new FXHorizontalFrame(box,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,2,0);
  FXLabel *swatch=new FXLabel(row,"",nullptr,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,14,12);
  swatch->setBackColor(curve->getLineStyle().color);
  new FXLabel(row,curve->getLabel(),nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  check=new FXCheckButton(row,"",this,ID_SERIES+id,CHECKBUTTON_NORMAL);
  check->setCheck(curve->isVisible());
  }


// Show or hide one series
long ChartWindow::onSeries(FXObject* sender,FXSelector sel,void*){
  FXint index=FXSELID(sel)-ID_SERIES;
  FXint num=demoCurves.no();
  FXbool show=((FXCheckButton*)sender)->getCheck()!=0;
  if(0<=index && index<num){
    demoCurves[index]->setVisible(show);
    if(!live->isRunning() && !plot->isZoomed()) plot->fitData();
    }
  else if(num<=index && index<num+live->getNumSeries()){
    FXCurve *curve=live->getCurve(index-num);
    if(curve!=nullptr){
      curve->setVisible(show);
      if(live->isRunning()) plot->fitY();
      }
    }
  plot->invalidateChart();
  return 1;
  }


// Bring the visibility checks in sync with the curves
void ChartWindow::syncSeriesChecks(){
  FXint i;
  for(i=0; i<demoCurves.no(); i++){
    if(demochecks[i]) demochecks[i]->setCheck(demoCurves[i]->isVisible(),false);
    }
  for(i=0; i<live->getNumSeries(); i++){
    FXCurve *curve=live->getCurve(i);
    if(livechecks[i] && curve) livechecks[i]->setCheck(curve->isVisible(),false);
    }
  }


// Switch to live data
void ChartWindow::startLive(){
  FXint i;
  if(live->isRunning()){
    updateRadios();
    return;
    }
  for(i=0; i<demoCurves.no(); i++){
    demovis[i]=demoCurves[i]->isVisible();
    demoCurves[i]->setVisible(false);
    }
  staticseries->hide();
  liveseries->show();
  livesection->show();
  savedbackstyle=plot->getBackStyle();
  savedchartstyle=plot->getChartStyle();
  savedcaptionstyle=plot->getCaptionStyle();
  plot->setCaptionStyle(CAPTION_NONE);
  plot->setBackStyle(liveBackStyle);
  plot->setChartStyle(liveChartStyle);
  plot->skipFades();
  plot->getAxis(FX2DChart::YLO)->setLabel("Mouse (px/ms)");
  plot->getAxis(FX2DChart::YHI)->setLabel("CPU % / MB");
  livebuffer->setText("Buffer: 0 samples");
  livetime->setText("Time: 0.0 s");
  livespeed->setText("Speed: 0.00 px/ms");
  liveaccel->setText("Accel: 0.00 px/ms2");
  livecpu->setText("CPU: 0.0 %");
  livememory->setText("Memory: 0.0 MB");
  liveframe->setText("Frame: 0.00 ms");
  onCmdRate(nullptr,0,nullptr);
  onCmdWindow(nullptr,0,nullptr);
  live->clear();
  live->start();
  for(i=0; i<live->getNumSeries(); i++){
    live->getCurve(i)->setVisible(livevis[i]);
    }
  xscroll->show();
  xscroll->enable();
  updateScroll();
  updateZoomButton();
  syncSeriesChecks();
  updateRadios();
  }


// Switch back to static data
void ChartWindow::stopLive(){
  FXint i;
  if(!live->isRunning()){
    updateRadios();
    return;
    }
  for(i=0; i<live->getNumSeries(); i++){
    livevis[i]=live->getCurve(i)->isVisible();
    }
  live->stop();
  for(i=0; i<demoCurves.no(); i++){
    demoCurves[i]->setVisible(demovis[i]);
    }
  staticseries->show();
  liveseries->hide();
  livesection->hide();
  plot->getAxis(FX2DChart::YLO)->setLabel("Value");
  plot->getAxis(FX2DChart::YHI)->setLabel("Value");
  plot->setBackStyle(savedbackstyle);
  plot->setChartStyle(savedchartstyle);
  plot->setCaptionStyle(savedcaptionstyle);
  plot->fitData();
  xscroll->hide();
  xscroll->disable();
  updateScroll();
  updateZoomButton();
  syncSeriesChecks();
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
  if(sample->count<5 || (sample->count%5)==0) updateScroll();
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
