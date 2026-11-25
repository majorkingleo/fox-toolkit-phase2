#include "fx.h"
#include "FXSVGIcon.h"
#include "FXCharts.h"
#include "CSVReader.h"
#include "ChartDemoWindow.h"
#include "ChartAssignmentDialog.h"
#include "ChartDemoUtils.h"
#include "ChartTabItem.h"
#include "ChartDetachedWindow.h"
#include "icons.h"
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>

#if defined(WIN32)
#include <windows.h>
#endif

namespace FXCHARTDEMO {

using namespace FX;
using namespace FXCHART;


class CSVLoadTask : public FXThread {
private:
  FXMutex progressMutex;
  FXuint progress;
  static void reportProgress(void* context,FXuint value){
    CSVLoadTask* task=(CSVLoadTask*)context;
    FXScopedMutex lock(task->progressMutex);
    task->progress=value;
    }
public:
  FXString filename;
  CSVTable table;
  FXString error;
  FXbool success;
  CSVLoadTask(const FXString& path):progress(0),filename(path),success(false){ }
  FXuint getProgress(){
    FXScopedMutex lock(progressMutex);
    return progress;
    }
  virtual FXint run() override {
    success=readCSV(filename,table,error,reportProgress,this);
    return success?0:1;
    }
  };


// Number of decimals needed to show a tick step without losing its value
static FXint tickDecimals(FXdouble step){
  FXdouble scaled=step;
  for(FXint decimals=0; decimals<3; decimals++){
    if(FXABS(scaled-Math::round(scaled))<=1.0e-7*FXMAX(1.0,FXABS(scaled))) return decimals;
    scaled*=10.0;
    }
  return 3;
  }


// Show elapsed time with the unit that fits the tick spacing
static FXString formatElapsedTick(FXdouble value,FXdouble spacing){
  FXdouble scale=1.0;
  const FXchar* suffix="s";
  if(spacing>=2.0*86400.0){ scale=86400.0; suffix="d"; }
  else if(spacing>=2.0*3600.0){ scale=3600.0; suffix="h"; }
  else if(spacing>=2.0*60.0){ scale=60.0; suffix="m"; }
  FXdouble step=spacing/scale;
  FXint decimals=tickDecimals(step);
  if(scale>1.0 && step>=10.0) decimals=0;
  return FXString::value("%.*f%s",decimals,value/scale,suffix);
  }


// Show an absolute timestamp as a date, month, year or clock time
static FXString formatDateTick(FXdouble value,FXdouble spacing,FXdouble origin){
  FXdouble absolute=value+origin;
  FXdouble day=std::floor(absolute/86400.0);
  FXdouble seconds=absolute-day*86400.0;
  FXint dayseconds=(FXint)std::floor(seconds+0.5);
  FXDate date;
  date.setTime((FXTime)Math::round(absolute*1.0e9));
  if(dayseconds>=86400){
    dayseconds-=86400;
    date.addDays(1);
    }
  FXint year=0,month=0,dayofmonth=0;
  date.getDate(year,month,dayofmonth);
  FXint hour=dayseconds/3600;
  FXint minute=(dayseconds/60)%60;
  FXint second=dayseconds%60;
  if(spacing>=86400.0*300.0) return FXString::value("%04d",year);
  if(spacing>=86400.0*20.0) return FXString::value("%04d-%02d",year,month);
  if(spacing>=86400.0) return FXString::value("%04d-%02d-%02d",year,month,dayofmonth);
  if(spacing>=60.0) return FXString::value("%02d-%02d %02d:%02d",month,dayofmonth,hour,minute);
  return FXString::value("%02d-%02d %02d:%02d:%02d",month,dayofmonth,hour,minute,second);
  }


// Format the x tick labels of a timestamp column
static FXString formatChartXAxisNumber(FXdouble value,FXdouble spacing,void* context){
  ChartDocument* document=(ChartDocument*)context;
  if(!document || !Math::fpFinite(value) || !Math::fpFinite(spacing) || spacing<=0.0) return FXString::value("%g",value);
  FXint column=document->selectedx;
  if(column<0 || column>=(FXint)document->table.columns.size() || !document->table.columns[column].timestamp) return FXString::value("%g",value);
  if(document->xdates) return formatDateTick(value,spacing,document->table.columns[column].timeOrigin);
  return formatElapsedTick(value,spacing);
  }


FXDEFMAP(ChartDemoWindow) ChartDemoWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_OPEN,ChartDemoWindow::onCmdOpen),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_QUIT,ChartDemoWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_THEME_CLASSIC,ChartDemoWindow::onTheme),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_THEME_DARK,ChartDemoWindow::onTheme),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_THEME_BREEZE,ChartDemoWindow::onTheme),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_THEME_BREEZE_DARK,ChartDemoWindow::onTheme),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XCOLUMN,ChartDemoWindow::onXColumn),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_X_AXIS_MODE,ChartDemoWindow::onXAxisMode),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_X_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_Y_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_AUTOFIT_Y,ChartDemoWindow::onAutoFitY),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_RESET_ZOOM,ChartDemoWindow::onResetZoom),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SPLIT_CHART,ChartDemoWindow::onSplitChart),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_PLOT_CHANGED,ChartDemoWindow::onPlotChanged),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_XSCROLL,ChartDemoWindow::onXScroll),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XSCROLL,ChartDemoWindow::onXScroll),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES,ChartDemoWindow::onSeries),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_ALL,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_NONE,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_TIMEOUT,ChartDemoWindow::ID_LOAD_POLL,ChartDemoWindow::onLoadPoll),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SPLITTER_CHANGED,ChartDemoWindow::onSplitterChanged),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_SERIES_FILTER,ChartDemoWindow::onSeriesFilter),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_PANEL,ChartDemoWindow::onSeriesPanel),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_DETACH,ChartDemoWindow::onDetach),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_PLOT_TOOL,ChartDemoWindow::onPlotTool),
  FXMAPFUNC(SEL_CLOSE,ChartDemoWindow::ID_DETACHED_CLOSE,ChartDemoWindow::onDetachedClose),
  FXMAPFUNC(SEL_CLOSE,ChartDemoWindow::ID_MAIN_CLOSE,ChartDemoWindow::onMainClose),
  };

FXIMPLEMENT(ChartDemoWindow,FXMainWindow,ChartDemoWindowMap,ARRAYNUMBER(ChartDemoWindowMap))


class ChartDemoPlot : public FX2DPlot {
  FXDECLARE(ChartDemoPlot)
protected:
  FXbool panmode;
  FXbool panning;
  FXbool cursorinside;
  FXint cursorx;
  FXint cursory;
  FXint pananchorx;
  FXint pananchory;
  FXdouble panrange[4];
protected:
  ChartDemoPlot():panmode(false),panning(false),cursorinside(false),cursorx(0),cursory(0),pananchorx(0),pananchory(0){ }
private:
  ChartDemoPlot(const ChartDemoPlot&);
  ChartDemoPlot& operator=(const ChartDemoPlot&);
public:
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  virtual void drawOverlay(FXDC& dc) const override;
  ChartDemoPlot(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FX2DPlot(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb),panmode(false),panning(false),cursorinside(false),cursorx(0),cursory(0),pananchorx(0),pananchory(0){ }
  void setPanMode(FXbool mode){ panmode=mode; }
  };


FXDEFMAP(ChartDemoPlot) ChartDemoPlotMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,ChartDemoPlot::onLeftBtnPress),
  FXMAPFUNC(SEL_MOTION,0,ChartDemoPlot::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,ChartDemoPlot::onLeftBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,ChartDemoPlot::onUngrabbed),
  FXMAPFUNC(SEL_LEAVE,0,ChartDemoPlot::onLeave),
  };

FXIMPLEMENT(ChartDemoPlot,FX2DPlot,ChartDemoPlotMap,ARRAYNUMBER(ChartDemoPlotMap))


long ChartDemoPlot::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  if(!panmode) return FX2DChart::onLeftBtnPress(nullptr,0,ptr);
  FXEvent* event=(FXEvent*)ptr;
  const FXRectangle& area=getPlotArea();
  if(!isEnabled() || event->win_x<area.x || event->win_x>=area.x+area.w || event->win_y<area.y || event->win_y>=area.y+area.h) return 0;
  if(event->click_count>1){
    resetZoom();
    return 1;
    }
  pananchorx=event->win_x;
  pananchory=event->win_y;
  panrange[0]=getAxis(FX2DChart::XLO)->getAxisRangeMin();
  panrange[1]=getAxis(FX2DChart::XLO)->getAxisRangeMax();
  panrange[2]=getAxis(FX2DChart::YLO)->getAxisRangeMin();
  panrange[3]=getAxis(FX2DChart::YLO)->getAxisRangeMax();
  panning=true;
  grab();
  return 1;
  }


long ChartDemoPlot::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(panmode && panning){
    const FXRectangle& area=getPlotArea();
    if(area.w<=0 || area.h<=0) return 1;
    FXdouble dx=(event->win_x-pananchorx)*(panrange[1]-panrange[0])/area.w;
    FXdouble dy=(event->win_y-pananchory)*(panrange[3]-panrange[2])/area.h;
    setViewRange(panrange[0]-dx,panrange[1]-dx,panrange[2]+dy,panrange[3]+dy);
    return 1;
    }
  const FXRectangle& area=getPlotArea();
  FXbool inside=event->win_x>=area.x && event->win_x<area.x+area.w && event->win_y>=area.y && event->win_y<area.y+area.h;
  if(cursorinside!=inside || (inside && (cursorx!=event->win_x || cursory!=event->win_y))){
    cursorinside=inside;
    cursorx=event->win_x;
    cursory=event->win_y;
    update();
    }
  if(panmode) return 0;
  return FX2DChart::onMotion(nullptr,0,ptr);
  }


long ChartDemoPlot::onLeftBtnRelease(FXObject*,FXSelector,void*){
  if(!panmode || !panning) return panmode?0:FX2DChart::onLeftBtnRelease(nullptr,0,nullptr);
  panning=false;
  ungrab();
  return 1;
  }


long ChartDemoPlot::onUngrabbed(FXObject*,FXSelector,void* ptr){
  if(!panmode || !panning) return panmode?0:FX2DChart::onUngrabbed(nullptr,0,ptr);
  panning=false;
  return 1;
  }


long ChartDemoPlot::onLeave(FXObject*,FXSelector,void*){
  if(cursorinside && !panning){
    cursorinside=false;
    update();
    }
  return 1;
  }


void ChartDemoPlot::drawOverlay(FXDC& dc) const {
  FX2DChart::drawOverlay(dc);
  if(!cursorinside) return;
  const FXRectangle& area=getPlotArea();
  if(area.w<=0 || area.h<=0) return;
  FXdouble xmin=getAxis(FX2DChart::XLO)->getAxisRangeMin();
  FXdouble xmax=getAxis(FX2DChart::XLO)->getAxisRangeMax();
  if(!(xmax>xmin)) return;
  FXdouble xvalue=xmin+(xmax-xmin)*(cursorx-area.x)/area.w;
  FXColor crosshair=readableColor(FXRGB(150,150,150),getChartStyle().forecolor);
  dc.setForeground(crosshair);
  dc.setLineWidth(1);
  dc.setLineStyle(LINE_ONOFF_DASH);
  dc.drawLine(cursorx,area.y,cursorx,area.y+area.h);
  dc.drawLine(area.x,cursory,area.x+area.w,cursory);
  dc.setLineStyle(LINE_SOLID);
  FXFont* font=getAxis(FX2DChart::XLO)->getNumberFont();
  dc.setFont(font);
  FXint textheight=font->getFontHeight();
  FXint textx=area.x+4;
  FXint texty=area.y+textheight+3;
  FXint rowheight=textheight+3;
  for(FXint i=0; i<numCurves(); i++){
    FXCurve* curve=getCurve(i);
    if(!curve->isVisible()) continue;
    FXData* xs=curve->getXData();
    FXData* ys=curve->getYData();
    if(!xs || !ys) continue;
    FXint count=FXMIN(xs->no(),ys->no());
    if(count<=0) continue;
    FXint index=xs->findIndexAt(xvalue);
    if(index<0) continue;
    FXdouble samplex=xs->get(index);
    FXdouble sampley=ys->get(index);
    if(index+1<count){
      FXdouble nextx=xs->get(index+1);
      FXdouble nexty=ys->get(index+1);
      if(Math::fpFinite(samplex) && Math::fpFinite(sampley) && Math::fpFinite(nextx) && Math::fpFinite(nexty) && nextx>samplex && xvalue>=samplex && xvalue<=nextx){
        sampley+=(nexty-sampley)*(xvalue-samplex)/(nextx-samplex);
        }
      }
    if(!Math::fpFinite(sampley)) continue;
    FXAxis* yaxis=getAxis(curve->getYAxis());
    FXdouble ymin=yaxis->getAxisRangeMin();
    FXdouble ymax=yaxis->getAxisRangeMax();
    if(!(ymax>ymin) || sampley<ymin || sampley>ymax) continue;
    FXint pointx=area.x+(FXint)Math::lrint((xvalue-xmin)*area.w/(xmax-xmin));
    FXint pointy=area.y+area.h-(FXint)Math::lrint((sampley-ymin)*area.h/(ymax-ymin));
    dc.setForeground(curve->getLineStyle().color);
    dc.fillEllipse(pointx-3,pointy-3,7,7);
    FXString text=FXString::value("%s: %.6g",curve->getLabel().text(),sampley);
    FXint width=textWidth(font,text);
    if(textx+width>area.x+area.w-4){
      textx=area.x+4;
      texty+=rowheight;
      }
    if(texty>area.y+area.h) break;
    dc.setForeground(curve->getLineStyle().color);
    drawText(dc,text,textx,texty,TEXT_ATTACH_LEFT|TEXT_ATTACH_BOTTOM);
    textx+=width+12;
    }
  }


static const FXColor seriesColors[]={
  FXRGB(220,50,47),FXRGB(38,139,210),FXRGB(133,153,0),FXRGB(211,54,130),
  FXRGB(42,161,152),FXRGB(203,75,22),FXRGB(108,113,196),FXRGB(181,137,0),
  FXRGB(88,110,117),FXRGB(0,121,107),FXRGB(156,39,176),FXRGB(63,81,181)
  };


static void applyPlotColors(FX2DPlot* plot,FXColor background,FXColor foreground,FXColor majorgrid,FXColor minorgrid);


static ChartDemoPlot* createChartPlot(FXComposite* parent,FXObject* target,FXSelector selector,FXFont* verticalFont,FXFont* numberFont,FXint plotHeight){
  ChartDemoPlot* plot=new ChartDemoPlot(parent,target,selector,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,plotHeight,12,12,12,12);
  plot->getAxis(FX2DChart::XHI)->setStyle(AXIS_OFF);
  plot->getAxis(FX2DChart::YHI)->setStyle(AXIS_OFF);
  plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalFont);
  plot->getAxis(FX2DChart::YLO)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_RIGHT);
  plot->getAxis(FX2DChart::YLO)->setLabelOffset(40);
  plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalFont);
  plot->getAxis(FX2DChart::YHI)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_LEFT);
  plot->getAxis(FX2DChart::YHI)->setLabelOffset(40);
  plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
  plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
  plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
  plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
  plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
  plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
  return plot;
  }


static void alignPlotAreas(ChartDocument* document);


static void updatePlotSizing(ChartDocument* document,FXint plotHeight){
  FXbool single=document->plots.size()==1;
  document->plotStack->setLayoutHints(single?(LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP):(LAYOUT_FILL_X|LAYOUT_TOP));
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    if(single) plot->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FILL_Y);
    else{
      plot->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT);
      plot->setHeight(plotHeight);
      }
    }
  document->plotStack->recalc();
  document->plotStack->layout();
  document->plotScroll->recalc();
  document->plotScroll->layout();
  }


static void ensurePlotCount(ChartDocument* document,FXint count,FXFont* verticalFont,FXFont* numberFont,FXint plotHeight){
  FXObject* target=document->plot->getTarget();
  FXSelector selector=document->plot->getSelector();
  while((FXint)document->plots.size()>count){
    delete document->plots.back();
    document->plots.pop_back();
    }
  while((FXint)document->plots.size()<count){
    ChartDemoPlot* plot=createChartPlot(document->plotStack,target,selector,verticalFont,numberFont,plotHeight);
    document->plots.push_back(plot);
    if(document->plotStack->id()) plot->create();
    FXTheme* theme=document->plot->getApp()->getTheme();
    FXColor surface=theme->color(ROLE_SURFACE);
    applyPlotColors(plot,theme->color(ROLE_WINDOW_BG),theme->color(ROLE_TEXT),contrastColor(surface,20),contrastColor(surface,10));
    }
  document->plotCount=(FXint)document->plots.size();
  document->plot=document->plots[0];
  updatePlotSizing(document,plotHeight);
  }


static void alignPlotAreas(ChartDocument* document){
  FXint anchor=-1;
  for(FXuval p=0; p<document->plots.size(); p++){
    document->plots[p]->setPlotLeftAnchor(-1);
    anchor=FXMAX(anchor,document->plots[p]->getNaturalPlotLeft());
    }
  for(FXuval p=0; p<document->plots.size(); p++) document->plots[p]->setPlotLeftAnchor(anchor);
  }


static void updateSplitChartLayout(ChartDocument* document){
  FXint count=(FXint)document->plots.size();
  for(FXint i=0; i<count; i++){
    FX2DPlot* plot=document->plots[i];
    FXAxis* xaxis=plot->getAxis(FX2DChart::XLO);
    FXuint xstyle=xaxis->getStyle()|AXIS_NUMBERS;
    if(i==count-1){
      if(document->xtoggle->getState()) xstyle|=AXIS_LABEL;
      else xstyle&=~AXIS_LABEL;
      }
    else xstyle&=~(AXIS_LABEL|AXIS_NUMBERS);
    xaxis->setStyle(xstyle);
    FXAxis* yaxis=plot->getAxis(FX2DChart::YLO);
    if(document->ytoggle->getState()) yaxis->setStyle(yaxis->getStyle()|AXIS_LABEL);
    else yaxis->setStyle(yaxis->getStyle()&~AXIS_LABEL);
    }
  document->plotStack->recalc();
  document->plotStack->layout();
  document->plotScroll->recalc();
  document->plotScroll->layout();
  alignPlotAreas(document);
  }


static void applyPlotColors(FX2DPlot* plot,FXColor background,FXColor foreground,FXColor majorgrid,FXColor minorgrid){
  FillStyle back=plot->getBackStyle();
  back.forecolor=background;
  back.backcolor=background;
  back.lower=background;
  back.upper=background;
  plot->setBackStyle(back);
  FillStyle chart=plot->getChartStyle();
  chart.forecolor=background;
  chart.backcolor=background;
  chart.lower=background;
  chart.upper=background;
  plot->setChartStyle(chart);
  plot->setCaptionColor(foreground);
  for(FXint axis=0; axis<4; axis++){
    FXAxis* item=plot->getAxis(axis);
    item->setAxisColor(foreground);
    item->setTickColors(foreground,foreground);
    item->setGridColors(majorgrid,minorgrid);
    item->setLabelColor(foreground);
    item->setNumberColor(foreground);
    }
  }


static const FXchar detach_hover_svg[]="<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\"><rect x=\"0.5\" y=\"0.5\" width=\"15\" height=\"15\" rx=\"3\" fill=\"#dbeafe\" stroke=\"#60a5fa\"/><g fill=\"none\" stroke=\"#1d4ed8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"1.4\"><path d=\"M9 3H4.5A1.5 1.5 0 0 0 3 4.5v7A1.5 1.5 0 0 0 4.5 13h7a1.5 1.5 0 0 0 1.5-1.5V7\"/><path d=\"M8 8l5-5m-4 0h4v4\"/></g></svg>";
static const FXchar detach_pressed_svg[]="<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\"><rect x=\"0.5\" y=\"0.5\" width=\"15\" height=\"15\" rx=\"3\" fill=\"#bfdbfe\" stroke=\"#2563eb\"/><g fill=\"none\" stroke=\"#1e40af\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"1.4\"><path d=\"M9 3H4.5A1.5 1.5 0 0 0 3 4.5v7A1.5 1.5 0 0 0 4.5 13h7a1.5 1.5 0 0 0 1.5-1.5V7\"/><path d=\"M8 8l5-5m-4 0h4v4\"/></g></svg>";


ChartDemoWindow::ChartDemoWindow(FXApp* app):FXMainWindow(app,"FOX Chart Demo",nullptr,nullptr,DECOR_ALL,20,20,1100,720){
    syncingPlots=false;
  fontDPI=0;
  fontScale=0.0;
  pixelScale=0.0;
  verticalLabelFont=new FXFont(app,"helvetica",14,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  verticalLabelFontDesc=verticalLabelFont->getFontDesc();
  verticalLabelFont->setAngle(90*64);
  numberFont=app->getNormalFont();
  FXColor background=app->getTheme()->color(ROLE_WINDOW_BG);
  FXbool dark=(FXREDVAL(background)*299+FXGREENVAL(background)*587+FXBLUEVAL(background)*114)<128000;
    iconsDark=dark;
  detachIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_detach:chart_detach),FXRGB(192,192,192),0,16,16);
  detachHoverIcon=new FXSVGIcon(app,(const FXuchar*)detach_hover_svg,FXRGB(192,192,192),0,16,16);
  detachPressedIcon=new FXSVGIcon(app,(const FXuchar*)detach_pressed_svg,FXRGB(192,192,192),0,16,16);
  autoFitYIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_autofix_y_axis:chart_autofix_y_axis),FXRGB(192,192,192),0,20,20);
  resetZoomIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_reset_zoom:chart_reset_zoom),FXRGB(192,192,192),0,24,24);
  splitChartIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_split_chart:chart_split_chart),FXRGB(192,192,192),0,20,20);
  xAxisIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_x_axis_label:chart_x_axis_label),FXRGB(192,192,192),0,20,20);
  dateAxisIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_date_axis:chart_date_axis),FXRGB(192,192,192),0,20,20);
  yAxisIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_y_axis_label:chart_y_axis_label),FXRGB(192,192,192),0,20,20);
  seriesIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_data_series:chart_data_series),FXRGB(192,192,192),0,20,20);
  zoomIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_zoom:chart_zoom),FXRGB(192,192,192),0,20,20);
  dragIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_drag:chart_drag),FXRGB(192,192,192),0,20,20);
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Open Data...\tCtl-O\tOpen delimited text, OpenDocument or Excel files",nullptr,this,ID_OPEN);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  new FXMenuTitle(menubar,"&File",nullptr,filemenu);
  FXMenuPane* thememenu=new FXMenuPane(this);
  classicTheme=new FXMenuRadio(thememenu,"FOX default",this,ID_THEME_CLASSIC);
  darkTheme=new FXMenuRadio(thememenu,"FOX dark",this,ID_THEME_DARK);
  breezeTheme=new FXMenuRadio(thememenu,"FOX Breeze Light",this,ID_THEME_BREEZE);
  breezeDarkTheme=new FXMenuRadio(thememenu,"FOX Breeze Dark",this,ID_THEME_BREEZE_DARK);
  FXColor surface=app->getTheme()->color(ROLE_SURFACE);
  if(surface==FXRGB(45,45,45)) themePreset=THEME_PRESET_DARK;
  else if(surface==FXRGB(239,240,241)) themePreset=THEME_PRESET_BREEZE;
  else if(surface==FXRGB(32,35,38)) themePreset=THEME_PRESET_BREEZE_DARK;
  else themePreset=THEME_PRESET_CLASSIC;
  classicTheme->setCheck(themePreset==THEME_PRESET_CLASSIC);
  darkTheme->setCheck(themePreset==THEME_PRESET_DARK);
  breezeTheme->setCheck(themePreset==THEME_PRESET_BREEZE);
  breezeDarkTheme->setCheck(themePreset==THEME_PRESET_BREEZE_DARK);
  new FXMenuTitle(menubar,"&Theme",nullptr,thememenu);

  statusbar=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);
  statusbar->getStatusLine()->setNormalText("Ready");
  loadingBar=new FXProgressBar(statusbar,nullptr,0,PROGRESSBAR_PERCENTAGE|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,220);
  loadingBar->setTotal(100);
  loadingBar->hide();
  tabs=new FXTabBook(this,nullptr,0,TABBOOK_TOPTABS|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXTabItem(tabs,"Welcome");
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,12,12,12,12);
  welcome=new FXLabel(page,"Open one or more CSV, OpenDocument (.ods) or Excel (.xlsx) files to plot their numeric columns.\n\nThe importer detects common delimiters, quoted fields, UTF encodings and Windows-1252 text.\nDate/time columns are detected and selected as the default X axis when available.",nullptr,JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  setTarget(this);
  setSelector(ID_MAIN_CLOSE);
  }


// Create the main window and realize its chart font for the current monitor
void ChartDemoWindow::create(){
  FXMainWindow::create();
  detachIcon->create();
  detachHoverIcon->create();
  detachPressedIcon->create();
  autoFitYIcon->create();
  resetZoomIcon->create();
  splitChartIcon->create();
  xAxisIcon->create();
  dateAxisIcon->create();
  yAxisIcon->create();
  seriesIcon->create();
  zoomIcon->create();
  dragIcon->create();
  updateChartFonts();
  }


// Refresh chart fonts when the window moves to a monitor with another DPI
void ChartDemoWindow::applyTheme(FXTheme* theme){
  FXMainWindow::applyTheme(theme);
  FXbool dark=(FXREDVAL(theme->color(ROLE_WINDOW_BG))*299+FXGREENVAL(theme->color(ROLE_WINDOW_BG))*587+FXBLUEVAL(theme->color(ROLE_WINDOW_BG))*114)<128000;
  if(dark!=iconsDark) updateChartIcons(dark);
  updateChartFonts();
  updateChartColors(theme);
  }


void ChartDemoWindow::updateChartIcons(FXbool dark){
  FXSVGIcon* oldDetach=detachIcon;
  FXSVGIcon* oldAutoFitY=autoFitYIcon;
  FXSVGIcon* oldResetZoom=resetZoomIcon;
  FXSVGIcon* oldSplitChart=splitChartIcon;
  FXSVGIcon* oldXAxis=xAxisIcon;
  FXSVGIcon* oldDateAxis=dateAxisIcon;
  FXSVGIcon* oldYAxis=yAxisIcon;
  FXSVGIcon* oldSeries=seriesIcon;
  FXSVGIcon* oldZoom=zoomIcon;
  FXSVGIcon* oldDrag=dragIcon;
  detachIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_detach:chart_detach),FXRGB(192,192,192),0,16,16);
  autoFitYIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_autofix_y_axis:chart_autofix_y_axis),FXRGB(192,192,192),0,20,20);
  resetZoomIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_reset_zoom:chart_reset_zoom),FXRGB(192,192,192),0,24,24);
  splitChartIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_split_chart:chart_split_chart),FXRGB(192,192,192),0,20,20);
  xAxisIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_x_axis_label:chart_x_axis_label),FXRGB(192,192,192),0,20,20);
  dateAxisIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_date_axis:chart_date_axis),FXRGB(192,192,192),0,20,20);
  yAxisIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_y_axis_label:chart_y_axis_label),FXRGB(192,192,192),0,20,20);
  seriesIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_data_series:chart_data_series),FXRGB(192,192,192),0,20,20);
  zoomIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_zoom:chart_zoom),FXRGB(192,192,192),0,20,20);
  dragIcon=new FXSVGIcon(getApp(),(const FXuchar*)(dark?chart_dark_drag:chart_drag),FXRGB(192,192,192),0,20,20);
  detachIcon->create();
  autoFitYIcon->create();
  resetZoomIcon->create();
  splitChartIcon->create();
  xAxisIcon->create();
  dateAxisIcon->create();
  yAxisIcon->create();
  seriesIcon->create();
  zoomIcon->create();
  dragIcon->create();
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    ((ChartTabItem*)document->tabitem)->setDetachIcons(detachIcon,detachHoverIcon,detachPressedIcon);
    document->xtoggle->setIcon(xAxisIcon);
    document->xtoggle->setAltIcon(xAxisIcon);
    document->xmode->setIcon(dateAxisIcon);
    document->xmode->setAltIcon(dateAxisIcon);
    document->ytoggle->setIcon(yAxisIcon);
    document->ytoggle->setAltIcon(yAxisIcon);
    document->autofity->setIcon(autoFitYIcon);
    document->resetzoom->setIcon(resetZoomIcon);
    document->splitbutton->setIcon(splitChartIcon);
    document->seriesshow->setIcon(seriesIcon);
    document->seriesshow->setAltIcon(seriesIcon);
    document->zoombutton->setIcon(zoomIcon);
    document->zoombutton->setAltIcon(zoomIcon);
    document->dragbutton->setIcon(dragIcon);
    document->dragbutton->setAltIcon(dragIcon);
    }
  delete oldDetach;
  delete oldAutoFitY;
  delete oldResetZoom;
  delete oldSplitChart;
  delete oldXAxis;
  delete oldDateAxis;
  delete oldYAxis;
  delete oldSeries;
  delete oldZoom;
  delete oldDrag;
  iconsDark=dark;
  }


void ChartDemoWindow::updateChartColors(FXTheme* theme){
  FXColor background=theme->color(ROLE_WINDOW_BG);
  FXColor surface=theme->color(ROLE_SURFACE);
  FXColor foreground=theme->color(ROLE_TEXT);
  FXColor majorgrid=contrastColor(surface,20);
  FXColor minorgrid=contrastColor(surface,10);
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    for(FXuval j=0; j<document->seriesrows.size(); j++){
      FXWindow* swatch=document->seriesrows[j]->getFirst();
      if(swatch) swatch->setBackColor(readableColor(seriesColors[j%(sizeof(seriesColors)/sizeof(seriesColors[0]))],background));
      }
    for(FXuval p=0; p<document->plots.size(); p++) applyPlotColors(document->plots[p],background,foreground,majorgrid,minorgrid);
    rebuild(document,true);
    }
  }


// Recreate application-owned fonts for this window's current display scale
void ChartDemoWindow::updateChartFonts(){
  FXint windowdpi=getWindowDPI();
  FXdouble scale=getDisplayScale();
  FXdouble pixelscale=getScale();
  FXFont* oldfont;
  if(windowdpi<=0) windowdpi=96;
  if(windowdpi==fontDPI && scale==fontScale && pixelscale==pixelScale) return;
  FXbool scalechanged=fontDPI>0 && (windowdpi!=fontDPI || pixelscale!=pixelScale);
  oldfont=verticalLabelFont;
  verticalLabelFont=makeChartFont(getApp(),verticalLabelFontDesc,oldfont->getAngle(),windowdpi,scale);
  numberFont=getApp()->getNormalFont(getWindowDPI());
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->detachWindow) continue;
    for(FXuval p=0; p<documents[i]->plots.size(); p++){
      FX2DPlot* plot=documents[i]->plots[p];
      plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
      plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
      plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
      plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
      plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
      plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
      plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
      plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
      }
    scaleDocumentWidgets(documents[i],this);
    if(documents[i]->splitter) documents[i]->splitter->setBarSize(this->scale(4));
    if(scalechanged && documents[i]->series->shown()){
      documents[i]->splitter->setSplit(1,this->scale(documents[i]->serieswidth));
      documents[i]->splitter->recalc();
      documents[i]->splitter->layout();
      }
    }
  delete oldfont;
  fontDPI=windowdpi;
  fontScale=scale;
  pixelScale=pixelscale;
  }


void ChartDemoWindow::scaleDocumentWidgets(ChartDocument* document,FXWindow* view){
  if(document->splitter) document->splitter->setBarSize(view->scale(4));
  updatePlotSizing(document,view->scale(280));
  if(document->plotStack){
    document->plotStack->setVSpacing(view->scale(4));
    document->plotStack->recalc();
    document->plotStack->layout();
    document->plotScroll->recalc();
    document->plotScroll->layout();
    }
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    plot->setMarginLeft(view->scale(48));
    plot->setMarginRight(view->scale(12));
    plot->setMarginTop(view->scale(12));
    plot->setMarginBottom(view->scale(12));
    plot->getAxis(FX2DChart::YLO)->setLabelOffset(view->scale(40));
    plot->getAxis(FX2DChart::YHI)->setLabelOffset(view->scale(40));
    }
  alignPlotAreas(document);
  document->xcolumn->setWidth(view->scale(280));
  document->series->setPadLeft(view->scale(5));
  document->series->setPadRight(view->scale(5));
  document->series->setPadTop(view->scale(5));
  document->series->setPadBottom(view->scale(5));
  for(FXuval i=0; i<document->seriesrows.size(); i++){
    FXWindow* swatch=document->seriesrows[i]->getFirst();
    if(swatch){
      swatch->setWidth(view->scale(12));
      swatch->setHeight(view->scale(12));
      }
    }
  }


void ChartDemoWindow::updateDetachedChartFonts(ChartDocument* document,FXTopWindow* view){
  FXint windowdpi=view->getWindowDPI();
  FXdouble scale=view->getDisplayScale();
  FXdouble pixelscale=view->getScale();
  FXFont* oldfont;
  if(windowdpi<=0) windowdpi=96;
  if(windowdpi==document->detachedFontDPI && scale==document->detachedFontScale && pixelscale==document->detachedPixelScale) return;
  oldfont=document->detachedVerticalFont;
  document->detachedVerticalFont=makeChartFont(getApp(),verticalLabelFontDesc,90*64,windowdpi,scale);
  document->detachedNumberFont=getApp()->getNormalFont(windowdpi);
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    plot->getAxis(FX2DChart::XLO)->setLabelFont(document->detachedNumberFont);
    plot->getAxis(FX2DChart::XHI)->setLabelFont(document->detachedNumberFont);
    plot->getAxis(FX2DChart::YLO)->setLabelFont(document->detachedVerticalFont);
    plot->getAxis(FX2DChart::YHI)->setLabelFont(document->detachedVerticalFont);
    plot->getAxis(FX2DChart::XLO)->setNumberFont(document->detachedNumberFont);
    plot->getAxis(FX2DChart::XHI)->setNumberFont(document->detachedNumberFont);
    plot->getAxis(FX2DChart::YLO)->setNumberFont(document->detachedNumberFont);
    plot->getAxis(FX2DChart::YHI)->setNumberFont(document->detachedNumberFont);
    }
  scaleDocumentWidgets(document,view);
  if(document->series->shown()) document->splitter->setSplit(1,view->scale(document->serieswidth));
  document->splitter->recalc();
  document->splitter->layout();
  delete oldfont;
  document->detachedFontDPI=windowdpi;
  document->detachedFontScale=scale;
  document->detachedPixelScale=pixelscale;
  }


long ChartDemoWindow::onCmdOpen(FXObject*,FXSelector,void*){
  FXString* filenames=FXFileDialog::getOpenFilenames(this,"Open data files",FXSystem::getCurrentDirectory(),"Data Files (*.csv,*.tsv,*.txt,*.ods,*.xlsx)\nCSV Files (*.csv,*.tsv,*.txt)\nSpreadsheets (*.ods,*.xlsx)\nOpenDocument Spreadsheets (*.ods)\nExcel Workbooks (*.xlsx)\nAll Files (*)");
  if(!filenames) return 1;
  for(FXint i=0; !filenames[i].empty(); i++) openFile(filenames[i]);
  delete [] filenames;
  return 1;
  }


long ChartDemoWindow::onCmdQuit(FXObject*,FXSelector,void*){
  getApp()->exit(0);
  return 1;
  }


long ChartDemoWindow::onTheme(FXObject*,FXSelector sel,void*){
  FXTheme theme=*getApp()->getTheme();
  FXString name;
  switch(FXSELID(sel)){
    case ID_THEME_DARK:
      theme.setDark();
      themePreset=THEME_PRESET_DARK;
      name="dark";
      break;
    case ID_THEME_BREEZE:
      theme.setBreezeLight();
      themePreset=THEME_PRESET_BREEZE;
      name="breeze";
      break;
    case ID_THEME_BREEZE_DARK:
      theme.setBreezeDark();
      themePreset=THEME_PRESET_BREEZE_DARK;
      name="breeze-dark";
      break;
    default:
      theme.setClassic();
      themePreset=THEME_PRESET_CLASSIC;
      name="classic";
      break;
    }
  classicTheme->setCheck(themePreset==THEME_PRESET_CLASSIC);
  darkTheme->setCheck(themePreset==THEME_PRESET_DARK);
  breezeTheme->setCheck(themePreset==THEME_PRESET_BREEZE);
  breezeDarkTheme->setCheck(themePreset==THEME_PRESET_BREEZE_DARK);
  getApp()->reg().writeStringEntry("SETTINGS","theme",name.text());
  getApp()->setTheme(theme);
  return 1;
  }


void ChartDemoWindow::openFile(const FXString& filename){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->filename==filename){
      tabs->setCurrent((FXint)i+1);
      return;
      }
    }
  for(FXuval i=0; i<loaders.size(); i++) if(loaders[i]->filename==filename) return;
  for(FXuval i=0; i<pendingFiles.size(); i++) if(pendingFiles[i]==filename) return;
  pendingFiles.push_back(filename);
  if(loaders.empty() && pendingFiles.size()==1){
    loadingBar->setProgress(0);
    loadingBar->show();
    statusbar->recalc();
    }
  startNextLoad();
  if(!loaders.empty()){
    FXSelector poll=FXSEL(SEL_TIMEOUT,ID_LOAD_POLL);
    if(!getApp()->hasTimeout(this,poll)) getApp()->addTimeout(this,poll,100000000);
    }
  else if(pendingFiles.empty()){
    loadingBar->hide();
    statusbar->getStatusLine()->setText("");
    statusbar->getStatusLine()->setNormalText("Ready");
    statusbar->recalc();
    }
  }


void ChartDemoWindow::startNextLoad(){
  FXint maxloaders=FXMAX(2,FXMIN(4,FXThread::processors()));
  while((FXint)loaders.size()<maxloaders && !pendingFiles.empty()){
    FXString filename=pendingFiles[0];
    pendingFiles.erase(pendingFiles.begin());
    CSVLoadTask* task=new CSVLoadTask(filename);
    if(!task->start()){
      delete task;
      FXMessageBox::error(this,MBOX_OK,"Import Failed","Could not start the CSV loading thread.");
      continue;
      }
    loaders.push_back(task);
    }
  FXint total=(FXint)(loaders.size()+pendingFiles.size());
  if(total>0) statusbar->getStatusLine()->setText(FXString::value("Importing %d file%s...",total,total==1?"":"s"));
  }


long ChartDemoWindow::onLoadPoll(FXObject*,FXSelector,void*){
  for(FXuval i=0; i<loaders.size();){
    CSVLoadTask* task=loaders[i];
    if(task->running()){
      i++;
      continue;
      }
    task->join();
    loaders.erase(loaders.begin()+i);
    if(task->success){
      addLoadedFile(task->filename,task->table);
      }
    else{
      FXMessageBox::error(this,MBOX_OK,"Import Failed","%s",task->error.text());
      }
    delete task;
    }
  startNextLoad();
  if(!loaders.empty() || !pendingFiles.empty()){
    FXuint progress=0;
    for(FXuval i=0; i<loaders.size(); i++) progress+=loaders[i]->getProgress();
    if(!loaders.empty()) loadingBar->setProgress(progress/(FXuint)loaders.size());
    getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_LOAD_POLL),100000000);
    }
  else{
    loadingBar->hide();
    statusbar->getStatusLine()->setText("");
    statusbar->getStatusLine()->setNormalText("Ready");
    statusbar->recalc();
    }
  return 1;
  }


void ChartDemoWindow::addLoadedFile(const FXString& filename,CSVTable& table){
  ChartDocument* document=new ChartDocument;
  document->filename=filename;
  document->table.columns.swap(table.columns);
  document->table.rows=table.rows;
  document->table.encoding=table.encoding;
  document->table.delimiter=table.delimiter;
  document->tabitem=nullptr;
  document->page=nullptr;
  document->placeholder=nullptr;
  document->detachWindow=nullptr;
  document->detachHost=nullptr;
  document->detachedVerticalFont=nullptr;
  document->detachedNumberFont=nullptr;
  document->detachedFontDPI=0;
  document->detachedFontScale=0.0;
  document->detachedPixelScale=0.0;
  document->plot=nullptr;
  document->plotCount=1;
  document->plots.clear();
  document->plotScroll=nullptr;
  document->plotStack=nullptr;
  document->xscroll=nullptr;
  document->info=nullptr;
  document->series=nullptr;
  document->seriesfilter=nullptr;
  document->serieslist=nullptr;
  document->splitter=nullptr;
  document->splitbutton=nullptr;
  document->seriesshow=nullptr;
  document->seriesclose=nullptr;
  document->serieswidth=205;
  document->xdates=false;
  document->xmode=nullptr;
  document->seriesshown=true;
  document->filtertext="";
  document->selectedx=-1;
  for(FXint column=0; column<(FXint)document->table.columns.size(); column++){
    if(document->table.columns[column].numeric){
      document->xcolumns.push_back(column);
      document->seriesKeys.push_back(chartColumnKey(document->table,column));
      document->visible.push_back(true);
      document->seriesPlot.push_back(0);
      }
    }
  if(document->xcolumns.empty()){
    FXMessageBox::error(this,MBOX_OK,"Import Failed","No plottable columns were found.");
    delete document;
    return;
    }
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint column=document->xcolumns[i];
    if(document->table.columns[column].timestamp){
      document->selectedx=column;
      break;
      }
    }
  if(document->selectedx<0) document->selectedx=document->xcolumns[0];
  document->settingsSection=chartSchemaKey(document->seriesKeys);
  loadDocumentSettings(document);

  FXString tabtitle=displayFileName(filename);
  document->tabitem=new ChartTabItem(tabs,tabtitle,detachIcon,detachHoverIcon,detachPressedIcon,this,ID_DETACH);
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,4,4,4,4);
  document->page=page;
  FXDockSite* docksite=new FXDockSite(page,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  FXToolBarShell* axisshell=new FXToolBarShell(this,FRAME_RAISED);
  FXToolBar* axistoolbar=new FXToolBar(docksite,axisshell,LAYOUT_DOCK_NEXT|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED);
  new FXToolBarGrip(axistoolbar,axistoolbar,FXToolBar::ID_TOOLBARGRIP,TOOLBARGRIP_DOUBLE);
  new FXLabel(axistoolbar,"X axis:",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  document->xcolumn=new FXComboBox(axistoolbar,32,this,ID_XCOLUMN,COMBOBOX_STATIC|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y|LAYOUT_FIX_WIDTH,0,0,280);
  for(FXuval i=0; i<document->xcolumns.size(); i++) document->xcolumn->appendItem(document->table.columns[document->xcolumns[i]].name);
  for(FXuval i=0; i<document->xcolumns.size(); i++) if(document->xcolumns[i]==document->selectedx) document->xcolumn->setCurrentItem((FXint)i);
  document->xcolumn->setNumVisible(FXMIN((FXint)document->xcolumns.size(),8));

  FXToolBarShell* charttoolshell=new FXToolBarShell(this,FRAME_RAISED);
  FXToolBar* charttoolbar=new FXToolBar(docksite,charttoolshell,LAYOUT_DOCK_NEXT|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED);
  new FXToolBarGrip(charttoolbar,charttoolbar,FXToolBar::ID_TOOLBARGRIP,TOOLBARGRIP_DOUBLE);
  document->xtoggle=new FXToggleButton(charttoolbar,"","",xAxisIcon,xAxisIcon,this,ID_X_DESCRIPTION,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->xtoggle->setTipText("X Description");
  document->xtoggle->setAltTipText("X Description");
  document->xmode=new FXToggleButton(charttoolbar,"","",dateAxisIcon,dateAxisIcon,this,ID_X_AXIS_MODE,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->xmode->setTipText("Switch X axis between time since start and dates");
  document->xmode->setAltTipText("Switch X axis between time since start and dates");
  document->ytoggle=new FXToggleButton(charttoolbar,"","",yAxisIcon,yAxisIcon,this,ID_Y_DESCRIPTION,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->ytoggle->setTipText("Y Description");
  document->ytoggle->setAltTipText("Y Description");
  document->xtoggle->setState(true);
  document->ytoggle->setState(true);
  document->autofity=new FXButton(charttoolbar,"",autoFitYIcon,this,ID_AUTOFIT_Y,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->autofity->setTipText("Auto-fit Y axis; preserve X view");
  document->resetzoom=new FXButton(charttoolbar,"",resetZoomIcon,this,ID_RESET_ZOOM,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->resetzoom->setTipText("Reset zoom");
  document->resetzoom->disable();
  document->splitbutton=new FXButton(charttoolbar,"",splitChartIcon,this,ID_SPLIT_CHART,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->splitbutton->setTipText("Assign series to subplots");
  if(document->xcolumns.size()<3) document->splitbutton->disable();
  document->seriesshow=new FXToggleButton(charttoolbar,"","",seriesIcon,seriesIcon,this,ID_SERIES_PANEL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->seriesshow->setTipText("Show/hide the series panel");
  document->zoombutton=new FXToggleButton(charttoolbar,"","",zoomIcon,zoomIcon,this,ID_PLOT_TOOL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->zoombutton->setTipText("Zoom to a rectangular plot selection");
  document->dragbutton=new FXToggleButton(charttoolbar,"","",dragIcon,dragIcon,this,ID_PLOT_TOOL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->dragbutton->setTipText("Drag to pan the plot");
  document->zoombutton->setState(true);
  FXHorizontalFrame* controls=new FXHorizontalFrame(page,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,4,4,4,4,6,0);
  document->info=new FXLabel(controls,"",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);

  FXSplitter* content=new FXSplitter(page,this,ID_SPLITTER_CHANGED,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_TOP|SPLITTER_REVERSED|SPLITTER_TRACKING);
  content->setBarSize(scale(4));
  document->splitter=content;
  FXVerticalFrame* chartarea=new FXVerticalFrame(content,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  FXVerticalFrame* chartcolumn=new FXVerticalFrame(chartarea,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  document->plotScroll=new FXScrollWindow(chartcolumn,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|HSCROLLING_OFF);
  document->plotStack=new FXVerticalFrame(document->plotScroll,LAYOUT_FILL_X|LAYOUT_TOP|FRAME_NONE,0,0,0,0,0,0,0,0,0,scale(4));
  document->plot=createChartPlot(document->plotStack,this,ID_PLOT_CHANGED,verticalLabelFont,numberFont,scale(280));
  document->plots.push_back(document->plot);
  FXint plotcount=document->plotCount;
  for(FXuval i=0; i<document->seriesPlot.size(); i++) plotcount=FXMAX(plotcount,document->seriesPlot[i]+1);
  ensurePlotCount(document,plotcount,verticalLabelFont,numberFont,scale(280));
  document->xscroll=new FXScrollBar(chartcolumn,this,ID_XSCROLL,SCROLLBAR_HORIZONTAL|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  document->xscroll->setLine(1000);
  FXSettings& settings=getApp()->reg();
  FXbool xdescription=FXString(settings.readStringEntry(document->settingsSection.text(),"xdescription","1"))!="0";
  FXbool ydescription=FXString(settings.readStringEntry(document->settingsSection.text(),"ydescription","1"))!="0";
  document->xtoggle->setState(xdescription);
  document->ytoggle->setState(ydescription);
  document->filtertext=settings.readStringEntry(document->settingsSection.text(),"seriesfilter","");
  document->seriesshown=FXString(settings.readStringEntry(document->settingsSection.text(),"seriesshown","1"))!="0";
  updateSplitChartLayout(document);
  FXTheme* currentTheme=getApp()->getTheme();
  FXColor chartbackground=currentTheme->color(ROLE_WINDOW_BG);
  FXColor chartforeground=currentTheme->color(ROLE_TEXT);
  FXColor chartsurface=currentTheme->color(ROLE_SURFACE);
  for(FXuval p=0; p<document->plots.size(); p++) applyPlotColors(document->plots[p],chartbackground,chartforeground,contrastColor(chartsurface,20),contrastColor(chartsurface,10));
  document->series=new FXVerticalFrame(content,LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK,0,0,205,0,5,5,5,5,1,1);
  FXHorizontalFrame* seriesheader=new FXHorizontalFrame(document->series,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,2,2,2,2,0,0);
  new FXLabel(seriesheader,"Series",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);
  document->seriesclose=new FXButton(seriesheader,"x",nullptr,this,ID_SERIES_PANEL,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  FXHorizontalFrame* filterframe=new FXHorizontalFrame(document->series,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,2,2,2,2,0,0);
  document->seriesfilter=new FXTextField(filterframe,18,this,ID_SERIES_FILTER,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X);
  document->seriesfilter->setText(document->filtertext);
  document->seriesfilter->setHelpText("Filter series by name");
  FXHorizontalFrame* seriesbuttons=new FXHorizontalFrame(document->series,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,3,0);
  document->selectAll=new FXButton(seriesbuttons,"Select All\tSelect all series",nullptr,this,ID_SERIES_ALL,FRAME_RAISED|LAYOUT_FILL_X);
  document->selectNone=new FXButton(seriesbuttons,"Select None\tSelect no series",nullptr,this,ID_SERIES_NONE,FRAME_RAISED|LAYOUT_FILL_X);
  FXComposite* listparent=document->series;
  if(document->xcolumns.size()>8) listparent=new FXScrollWindow(document->series,LAYOUT_FILL_X|LAYOUT_FILL_Y|VSCROLLING_ON);
  document->serieslist=new FXVerticalFrame(listparent,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|FRAME_NONE,0,0,0,0,2,2,2,2,0,0);
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    FXHorizontalFrame* row=new FXHorizontalFrame(document->serieslist,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,2,1,2,1,4,0);
    FXFrame* swatch=new FXFrame(row,FRAME_SUNKEN|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_CENTER_Y,0,0,12,12);
    swatch->setBackColor(readableColor(seriesColors[i%(sizeof(seriesColors)/sizeof(seriesColors[0]))],getApp()->getTheme()->color(ROLE_WINDOW_BG)));
    FXCheckButton* check=new FXCheckButton(row,document->table.columns[col].name,this,ID_SERIES,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);
    check->setCheck(true);
    if(col==document->selectedx) check->disable();
    document->checks.push_back(check);
    document->seriesrows.push_back(row);
    }
  scaleDocumentWidgets(document,this);
  documents.push_back(document);
  rebuild(document);
  document->tabitem->create();
  page->create();
  tabs->setCurrent((FXint)documents.size());
  document->plotStack->layout();
  document->plotScroll->layout();
  alignPlotAreas(document);
  if(getApp()->reg().existingEntry(document->settingsSection.text(),"split")){
    FXint split=getApp()->reg().readIntEntry(document->settingsSection.text(),"split",205);
    if(split>0) document->serieswidth=FXCLAMP(180,split,400);
    }
  if(!document->seriesshown) document->series->hide();
  else content->setSplit(1,scale(document->serieswidth));
  document->seriesshow->setState(document->series->shown());
  applySeriesFilter(document);
  if(FXString(settings.readStringEntry(document->settingsSection.text(),"zoomed","0"))=="1" && settings.existingEntry(document->settingsSection.text(),"xminimum")){
    for(FXuval p=0; p<document->plots.size(); p++){
      FXString prefix=FXString::value("plot-%d-",(FXint)p);
      FXString zoomkey=prefix+"zoomed";
      FXString zoomdefault=p==0?"1":(p==1?settings.readStringEntry(document->settingsSection.text(),"lowerzoomed","0"):"0");
      if(FXString(settings.readStringEntry(document->settingsSection.text(),zoomkey.text(),zoomdefault.text()))!="1") continue;
      FXString ymin=prefix+"yminimum";
      FXString ymax=prefix+"ymaximum";
      const FXchar* oldymin=p==1?"loweryminimum":"yminimum";
      const FXchar* oldymax=p==1?"lowerymaximum":"ymaximum";
      document->plots[p]->setViewRange(settings.readRealEntry(document->settingsSection.text(),"xminimum"),settings.readRealEntry(document->settingsSection.text(),"xmaximum"),settings.readRealEntry(document->settingsSection.text(),ymin.text(),settings.readRealEntry(document->settingsSection.text(),oldymin)),settings.readRealEntry(document->settingsSection.text(),ymax.text(),settings.readRealEntry(document->settingsSection.text(),oldymax)));
      }
    document->resetzoom->enable();
    }
  saveDocumentSettings(document);
  }


void ChartDemoWindow::loadDocumentSettings(ChartDocument* document){
  FXSettings& settings=getApp()->reg();
  FXString xkey=settings.readStringEntry(document->settingsSection.text(),"xaxis","");
  if(!xkey.empty()){
    for(FXuval i=0; i<document->xcolumns.size(); i++){
      if(document->seriesKeys[i]==xkey){
        document->selectedx=document->xcolumns[i];
        break;
        }
      }
    }
  document->xdates=FXString(settings.readStringEntry(document->settingsSection.text(),"xaxisdates","0"))!="0";
  for(FXuval i=0; i<document->seriesKeys.size(); i++){
    FXString entry="series-"+chartHash(document->seriesKeys[i]);
    document->visible[i]=FXString(settings.readStringEntry(document->settingsSection.text(),entry,"1")).toInt()!=0;
    entry="plot-"+chartHash(document->seriesKeys[i]);
    FXString legacy="lower-"+chartHash(document->seriesKeys[i]);
    FXint defaultplot=FXString(settings.readStringEntry(document->settingsSection.text(),legacy.text(),"0")).toInt()!=0?1:0;
    document->seriesPlot[i]=FXString(settings.readStringEntry(document->settingsSection.text(),entry.text(),FXString::value("%d",defaultplot).text())).toInt();
    }
  document->plotCount=FXMAX(1,settings.readIntEntry(document->settingsSection.text(),"plotcount",1));
  for(FXuval i=0; i<document->seriesPlot.size(); i++) document->plotCount=FXMAX(document->plotCount,document->seriesPlot[i]+1);
  }


void ChartDemoWindow::saveDocumentSettings(ChartDocument* document){
  FXSettings& settings=getApp()->reg();
  settings.writeStringEntry(document->settingsSection.text(),"xaxis",chartColumnKey(document->table,document->selectedx).text());
  for(FXuval i=0; i<document->seriesKeys.size(); i++){
    FXString entry="series-"+chartHash(document->seriesKeys[i]);
    settings.writeStringEntry(document->settingsSection.text(),entry,document->visible[i]?"1":"0");
    entry="plot-"+chartHash(document->seriesKeys[i]);
    settings.writeIntEntry(document->settingsSection.text(),entry,document->seriesPlot[i]);
    }
  settings.writeIntEntry(document->settingsSection.text(),"plotcount",document->plotCount);
  settings.writeStringEntry(document->settingsSection.text(),"xaxisdates",document->xdates?"1":"0");
  settings.writeStringEntry(document->settingsSection.text(),"xdescription",document->xtoggle && document->xtoggle->getState()?"1":"0");
  settings.writeStringEntry(document->settingsSection.text(),"ydescription",document->ytoggle && document->ytoggle->getState()?"1":"0");
  settings.writeStringEntry(document->settingsSection.text(),"seriesfilter",document->seriesfilter?document->seriesfilter->getText().text():document->filtertext.text());
  settings.writeStringEntry(document->settingsSection.text(),"seriesshown",document->series && document->series->shown()?"1":"0");
  if(document->plot){
    FX2DPlot* reference=document->plot;
    for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]->numCurves()>0){ reference=document->plots[p]; break; }
    FXAxis* xaxis=reference->getAxis(FX2DChart::XLO);
    FXAxis* yaxis=document->plot->getAxis(FX2DChart::YLO);
    FXbool zoomed=false;
    for(FXuval p=0; p<document->plots.size(); p++){
      FX2DPlot* plot=document->plots[p];
      FXString prefix=FXString::value("plot-%d-",(FXint)p);
      FXString zoomkey=prefix+"zoomed";
      FXString ymin=prefix+"yminimum";
      FXString ymax=prefix+"ymaximum";
      settings.writeStringEntry(document->settingsSection.text(),zoomkey.text(),plot->isZoomed()?"1":"0");
      settings.writeRealEntry(document->settingsSection.text(),ymin.text(),plot->getAxis(FX2DChart::YLO)->getAxisRangeMin());
      settings.writeRealEntry(document->settingsSection.text(),ymax.text(),plot->getAxis(FX2DChart::YLO)->getAxisRangeMax());
      if(plot->isZoomed()) zoomed=true;
      }
    settings.writeStringEntry(document->settingsSection.text(),"zoomed",zoomed?"1":"0");
    settings.writeRealEntry(document->settingsSection.text(),"xminimum",xaxis->getAxisRangeMin());
    settings.writeRealEntry(document->settingsSection.text(),"xmaximum",xaxis->getAxisRangeMax());
    settings.writeRealEntry(document->settingsSection.text(),"yminimum",yaxis->getAxisRangeMin());
    settings.writeRealEntry(document->settingsSection.text(),"ymaximum",yaxis->getAxisRangeMax());
    }
  if(document->splitter && document->series && document->series->shown()){
    FXint width=document->splitter->getSplit(1);
    FXdouble viewscale=document->detachWindow?document->detachWindow->getScale():getScale();
    if(width>0) document->serieswidth=(FXint)(width/viewscale+0.5);
    }
  settings.writeIntEntry(document->settingsSection.text(),"split",document->serieswidth);
  }


void ChartDemoWindow::applySeriesFilter(ChartDocument* document){
  FXString filter=document->seriesfilter?document->seriesfilter->getText():document->filtertext;
  filter.lower();
  document->filtertext=document->seriesfilter?document->seriesfilter->getText():document->filtertext;
  for(FXuval i=0; i<document->seriesrows.size(); i++){
    FXint column=document->xcolumns[i];
    FXString name=document->table.columns[column].name;
    name.lower();
    if(filter.empty() || name.find(filter)>=0) document->seriesrows[i]->show();
    else document->seriesrows[i]->hide();
    }
  if(document->serieslist){
    document->serieslist->recalc();
    document->serieslist->layout();
    document->serieslist->getParent()->recalc();
    document->serieslist->getParent()->layout();
    }
  }


void ChartDemoWindow::rebuild(ChartDocument* document,FXbool keepView){
  FXint xcol=document->selectedx;
  FXint xindex=-1;
  std::vector<FXbool> zoomed(document->plots.size(),false);
  std::vector<FXdouble> xmin(document->plots.size()),xmax(document->plots.size()),ymin(document->plots.size()),ymax(document->plots.size());
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    zoomed[p]=keepView && plot->isZoomed();
    xmin[p]=plot->getAxis(FX2DChart::XLO)->getAxisRangeMin();
    xmax[p]=plot->getAxis(FX2DChart::XLO)->getAxisRangeMax();
    ymin[p]=plot->getAxis(FX2DChart::YLO)->getAxisRangeMin();
    ymax[p]=plot->getAxis(FX2DChart::YLO)->getAxisRangeMax();
    }
  for(FXuval i=0; i<document->xcolumns.size(); i++) if(document->xcolumns[i]==xcol) xindex=(FXint)i;
  if(xindex<0) return;
  for(FXuval p=0; p<document->plots.size(); p++) document->plots[p]->clearCurves();
  std::vector<FXData*> sharedx(document->plots.size(),nullptr);
  FXint visiblecount=0;
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    document->checks[i]->setCheck(document->visible[i] && col!=xcol);
    if(col==xcol) document->checks[i]->disable();
    else document->checks[i]->enable();
    if(col==xcol) continue;
    if(document->seriesPlot[i]<0) continue;
    if(!document->visible[i]) continue;
    FXint plotindex=FXCLAMP(0,document->seriesPlot[i],(FXint)document->plots.size()-1);
    FX2DPlot* target=document->plots[plotindex];
    FXCurve* curve=new FXCurve(target,document->table.columns[col].name);
    FXbool ownsx=false;
    FXData* xdata=sharedx[plotindex];
    if(xdata==nullptr){
      xdata=new FXData;
      xdata->assign(document->table.columns[xcol].values.data(),document->table.rows);
      sharedx[plotindex]=xdata;
      ownsx=true;
      }
    FXData* ydata=new FXData;
    ydata->assign(document->table.columns[col].values.data(),document->table.rows);
    curve->setXData(xdata,ownsx);
    curve->setYData(ydata);
    LineStyle line={readableColor(seriesColors[i%(sizeof(seriesColors)/sizeof(seriesColors[0]))],getApp()->getTheme()->color(ROLE_WINDOW_BG)),1,LINESTYLE_SOLID};
    curve->setLineStyle(line);
    target->appendCurve(curve);
    visiblecount++;
    }
  FXbool timeaxis=document->table.columns[xcol].timestamp;
  FXString xlabel=document->table.columns[xcol].name+(timeaxis&&!document->xdates?" (time since start)":"");
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    FXAxis* xaxis=plot->getAxis(FX2DChart::XLO);
    xaxis->setNumberFormat(timeaxis?formatChartXAxisNumber:nullptr,timeaxis?document:nullptr);
    xaxis->setLabel(xlabel);
    plot->getAxis(FX2DChart::YLO)->setLabel("Value");
    plot->fitData();
    if(zoomed[p]) plot->setViewRange(xmin[p],xmax[p],ymin[p],ymax[p]);
    }
  if(document->xmode){
    document->xmode->setState(timeaxis && document->xdates);
    if(timeaxis){
      document->xmode->enable();
      document->xmode->setTipText("Switch X axis between time since start and dates");
      document->xmode->setAltTipText("Switch X axis between time since start and dates");
      }
    else{
      document->xmode->disable();
      document->xmode->setTipText("Date axis needs a date/time X column");
      document->xmode->setAltTipText("Date axis needs a date/time X column");
      }
    }
  updateSplitChartLayout(document);
  alignPlotAreas(document);
  FXbool anyzoomed=false;
  for(FXuval p=0; p<document->plots.size(); p++){
    document->plots[p]->invalidateChart();
    if(document->plots[p]->isZoomed()) anyzoomed=true;
    }
  if(anyzoomed) document->resetzoom->enable();
  else document->resetzoom->disable();
  FXint ycount=0;
  for(FXuval i=0; i<document->xcolumns.size(); i++) if((FXint)i!=xindex) ycount++;
  document->info->setText(FXString::value(document->table.rows)+" rows; "+chartMemorySize(chartDataBufferBytes(document->table))+"; "+FXString::value(visiblecount)+" plotted / "+FXString::value(ycount)+" numeric series; "+document->table.delimiter+"; "+document->table.encoding);
  updateXScroll(document);
  }


long ChartDemoWindow::onXColumn(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->xcolumn==sender){
      FXint selected=document->xcolumn->getCurrentItem();
      if(selected>=0 && selected<(FXint)document->xcolumns.size()){
        document->selectedx=document->xcolumns[selected];
        rebuild(document);
        FXbool zoomed=false;
        for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]->isZoomed()) zoomed=true;
        if(zoomed) document->resetzoom->enable();
        else document->resetzoom->disable();
        saveDocumentSettings(document);
        }
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onXAxisMode(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->xmode==sender){
      document->xdates=document->xmode->getState();
      rebuild(document,true);
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onAxisDescription(FXObject* sender,FXSelector sel,void*){
  FXbool shown=((FXToggleButton*)sender)->getState();
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(FXSELID(sel)==ID_X_DESCRIPTION && document->xtoggle==sender){
      FXAxis* axis=document->plot->getAxis(FX2DChart::XLO);
      if(shown) axis->setStyle(axis->getStyle()|AXIS_LABEL);
      else axis->setStyle(axis->getStyle()&~AXIS_LABEL);
      updateSplitChartLayout(document);
      for(FXuval p=0; p<document->plots.size(); p++) document->plots[p]->invalidateChart();
      saveDocumentSettings(document);
      return 1;
      }
    if(FXSELID(sel)==ID_Y_DESCRIPTION && document->ytoggle==sender){
      for(FXuval p=0; p<document->plots.size(); p++){
        FXAxis* axis=document->plots[p]->getAxis(FX2DChart::YLO);
        if(shown) axis->setStyle(axis->getStyle()|AXIS_LABEL);
        else axis->setStyle(axis->getStyle()&~AXIS_LABEL);
        document->plots[p]->invalidateChart();
        }
      alignPlotAreas(document);
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onAutoFitY(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->autofity==sender){
      for(FXuval p=0; p<document->plots.size(); p++){
        document->plots[p]->fitY();
        document->plots[p]->invalidateChart();
        }
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onSplitChart(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->splitbutton!=sender) continue;
    ChartAssignmentDialog dialog(this,document->table,document->xcolumns,document->selectedx,document->seriesPlot,document->plotCount);
    if(!dialog.execute(PLACEMENT_OWNER)) return 1;
    FXint count=1;
    dialog.getAssignment(document->seriesPlot,count);
    FXWindow* view=document->detachWindow?static_cast<FXWindow*>(document->detachWindow):this;
    ensurePlotCount(document,count,document->detachWindow?document->detachedVerticalFont:verticalLabelFont,document->detachWindow?document->detachedNumberFont:numberFont,view->scale(280));
    scaleDocumentWidgets(document,document->detachWindow?static_cast<FXWindow*>(document->detachWindow):this);
    updateSplitChartLayout(document);
    rebuild(document,false);
    document->plotStack->layout();
    document->plotScroll->layout();
    document->resetzoom->disable();
    saveDocumentSettings(document);
    getApp()->reg().write();
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onResetZoom(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->resetzoom==sender){
      syncingPlots=true;
      for(FXuval p=0; p<document->plots.size(); p++) document->plots[p]->resetZoom();
      syncingPlots=false;
      document->resetzoom->disable();
      updateXScroll(document);
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onPlotTool(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(sender!=document->zoombutton && sender!=document->dragbutton) continue;
    FXbool panmode=sender==document->dragbutton && document->dragbutton->getState();
    document->zoombutton->setState(!panmode);
    document->dragbutton->setState(panmode);
    for(FXuval p=0; p<document->plots.size(); p++) static_cast<ChartDemoPlot*>(document->plots[p])->setPanMode(panmode);
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onPlotChanged(FXObject* sender,FXSelector,void*){
  if(syncingPlots) return 1;
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    FX2DPlot* source=nullptr;
    for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]==sender) source=document->plots[p];
    if(source){
      syncingPlots=true;
      for(FXuval p=0; p<document->plots.size(); p++){
        FX2DPlot* other=document->plots[p];
        if(other==source) continue;
        if(source->isZoomed()){
          FXdouble yminimum=other->getAxis(FX2DChart::YLO)->getAxisRangeMin();
          FXdouble ymaximum=other->getAxis(FX2DChart::YLO)->getAxisRangeMax();
          other->setViewRange(source->getAxis(FX2DChart::XLO)->getAxisRangeMin(),source->getAxis(FX2DChart::XLO)->getAxisRangeMax(),yminimum,ymaximum);
          }
        else other->resetZoom();
        other->invalidateChart();
        }
      syncingPlots=false;
      FXbool zoomed=false;
      for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]->isZoomed()) zoomed=true;
      if(zoomed) document->resetzoom->enable();
      else document->resetzoom->disable();
      updateXScroll(document);
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


void ChartDemoWindow::updateXScroll(ChartDocument* document){
  FX2DPlot* reference=document->plot;
  for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]->numCurves()>0){ reference=document->plots[p]; break; }
  FXAxis* axis=reference->getAxis(FX2DChart::XLO);
  FXdouble datamin=axis->getDataRangeMin();
  FXdouble datamax=axis->getDataRangeMax();
  FXdouble viewmin=axis->getAxisRangeMin();
  FXdouble viewmax=axis->getAxisRangeMax();
  FXdouble datarange=datamax-datamin;
  FXdouble viewrange=viewmax-viewmin;
  FXint range=1000000;
  FXint page=range;
  FXint position=0;
  if(datarange>0.0 && viewrange>0.0){
    page=FXCLAMP(1,(FXint)(viewrange/datarange*range+0.5),range);
    position=(FXint)((viewmin-datamin)/datarange*range+0.5);
    position=FXCLAMP(0,position,range-page);
    }
  document->xscroll->setRange(range,false);
  document->xscroll->setPage(page,false);
  document->xscroll->setPosition(position,false);
  }


long ChartDemoWindow::onXScroll(FXObject* sender,FXSelector,void* ptr){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->xscroll!=sender) continue;
    FX2DPlot* reference=document->plot;
    for(FXuval p=0; p<document->plots.size(); p++) if(document->plots[p]->numCurves()>0){ reference=document->plots[p]; break; }
    FXAxis* axis=reference->getAxis(FX2DChart::XLO);
    FXdouble datamin=axis->getDataRangeMin();
    FXdouble datamax=axis->getDataRangeMax();
    FXdouble datarange=datamax-datamin;
    FXdouble viewrange=axis->getAxisRangeMax()-axis->getAxisRangeMin();
    if(datarange<=0.0 || viewrange<=0.0) return 1;
    FXint position=(FXint)(FXival)ptr;
    FXdouble xminimum=datamin+datarange*(FXdouble)position/document->xscroll->getRange();
    FXdouble xmaximum=xminimum+viewrange;
    syncingPlots=true;
    for(FXuval p=0; p<document->plots.size(); p++){
      FXAxis* yaxis=document->plots[p]->getAxis(FX2DChart::YLO);
      document->plots[p]->setViewRange(xminimum,xmaximum,yaxis->getAxisRangeMin(),yaxis->getAxisRangeMax());
      }
    syncingPlots=false;
    updateXScroll(document);
    saveDocumentSettings(document);
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onSeries(FXObject* sender,FXSelector,void* ptr){
  for(FXuval d=0; d<documents.size(); d++){
    ChartDocument* document=documents[d];
    for(FXuval i=0; i<document->checks.size(); i++){
      if(document->checks[i]==sender){
        document->visible[i]=(FXbool)(FXuval)ptr;
        rebuild(document,true);
        saveDocumentSettings(document);
        return 1;
        }
      }
    }
  return 0;
  }


long ChartDemoWindow::onSeriesSelection(FXObject* sender,FXSelector sel,void*){
  FXbool selected=FXSELID(sel)==ID_SERIES_ALL;
  for(FXuval d=0; d<documents.size(); d++){
    ChartDocument* document=documents[d];
    if(sender!=document->selectAll && sender!=document->selectNone) continue;
    for(FXuval i=0; i<document->visible.size(); i++) document->visible[i]=selected;
    rebuild(document,true);
    saveDocumentSettings(document);
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onSplitterChanged(FXObject* sender,FXSelector sel,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->splitter==sender){
      saveDocumentSettings(documents[i]);
      if(FXSELTYPE(sel)==SEL_COMMAND) getApp()->reg().write();
      return 1;
    }
  }
  return 0;
}


long ChartDemoWindow::onSeriesFilter(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->seriesfilter==sender){
      applySeriesFilter(documents[i]);
      saveDocumentSettings(documents[i]);
      return 1;
    }
  }
  return 0;
}


long ChartDemoWindow::onSeriesPanel(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(sender!=document->seriesshow && sender!=document->seriesclose) continue;
    FXbool shown=sender==document->seriesclose?false:document->seriesshow->getState();
    if(shown && !document->series->shown()){
      document->series->show();
      FXMainWindow* view=document->detachWindow?document->detachWindow:this;
      document->splitter->setSplit(1,view->scale(document->serieswidth));
      }
    else if(!shown && document->series->shown()){
      FXint width=document->splitter->getSplit(1);
      FXdouble viewscale=document->detachWindow?document->detachWindow->getScale():getScale();
      if(width>0) document->serieswidth=(FXint)(width/viewscale+0.5);
      document->series->hide();
      }
    document->seriesshow->setState(shown);
    document->splitter->recalc();
    document->splitter->layout();
    document->series->getParent()->recalc();
    document->series->getParent()->layout();
    saveDocumentSettings(document);
    return 1;
    }
  return 0;
}


long ChartDemoWindow::onDetach(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->tabitem==(FXTabItem*)sender){
      detachDocument(documents[i]);
      return 1;
    }
  }
  return 0;
}


void ChartDemoWindow::detachDocument(ChartDocument* document){
  if(!document || document->detachWindow) return;
  for(FXuval i=0; i<documents.size(); i++) if(documents[i]==document){
    tabs->setCurrent((FXint)i+1);
    break;
    }
  document->placeholder=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,4,4,4,4);
  document->placeholder->create();
  document->placeholder->reparent(tabs,document->page);
  FXString windowTitle="FOX Chart Demo - "+displayFileName(document->filename);
  ChartDetachedWindow* window=new ChartDetachedWindow(getApp(),windowTitle,this,document);
  FXVerticalFrame* host=new FXVerticalFrame(window,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE);
  window->setTarget(this);
  window->setSelector(ID_DETACHED_CLOSE);
  window->create();
  document->detachWindow=window;
  document->detachHost=host;
  document->page->reparent(host);
  document->splitter->setSplit(1,window->scale(document->serieswidth));
  document->page->show();
  document->tabitem->hide();
  document->placeholder->hide();
  host->recalc();
  host->layout();
  window->show(PLACEMENT_SCREEN);
  tabs->recalc();
  tabs->layout();
  saveDocumentSettings(document);
}


void ChartDemoWindow::attachDocument(ChartDocument* document,FXbool select){
  if(!document || !document->detachWindow) return;
  saveDocumentSettings(document);
  document->page->reparent(tabs,document->placeholder);
  delete document->placeholder;
  document->placeholder=nullptr;
  document->detachWindow=nullptr;
  document->detachHost=nullptr;
  document->tabitem->show();
  document->page->show();
  document->splitter->setSplit(1,scale(document->serieswidth));
  for(FXuval p=0; p<document->plots.size(); p++){
    FX2DPlot* plot=document->plots[p];
    plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
    plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
    plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
    plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
    plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
    plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
    plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
    plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
    }
  delete document->detachedVerticalFont;
  document->detachedVerticalFont=nullptr;
  document->detachedNumberFont=nullptr;
  document->detachedFontDPI=0;
  document->detachedFontScale=0.0;
  document->detachedPixelScale=0.0;
  if(select){
    for(FXuval i=0; i<documents.size(); i++) if(documents[i]==document){
      tabs->setCurrent((FXint)i+1);
      break;
      }
    }
  tabs->recalc();
  tabs->layout();
  tabs->getParent()->recalc();
  tabs->getParent()->layout();
  saveDocumentSettings(document);
}


long ChartDemoWindow::onDetachedClose(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->detachWindow==(FXMainWindow*)sender){
      attachDocument(documents[i]);
      return 0;
      }
    }
  return 0;
}


long ChartDemoWindow::onMainClose(FXObject*,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->detachWindow){
      FXMainWindow* window=documents[i]->detachWindow;
      attachDocument(documents[i],false);
      window->close(false);
      }
    }
  return 0;
}


ChartDemoWindow::~ChartDemoWindow(){
  getApp()->removeTimeout(this,FXSEL(SEL_TIMEOUT,ID_LOAD_POLL));
  for(FXuval i=0; i<loaders.size(); i++){
    loaders[i]->join();
    delete loaders[i];
    }
  loaders.clear();
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->detachWindow){
      FXMainWindow* window=documents[i]->detachWindow;
      attachDocument(documents[i],false);
      delete window;
      }
    saveDocumentSettings(documents[i]);
    delete documents[i];
    }
  documents.clear();
  pendingFiles.clear();
  delete tabs;
  getApp()->reg().write();
  delete verticalLabelFont;
  delete detachPressedIcon;
  delete detachHoverIcon;
  delete detachIcon;
  delete autoFitYIcon;
  delete resetZoomIcon;
  delete splitChartIcon;
  delete xAxisIcon;
  delete dateAxisIcon;
  delete yAxisIcon;
  delete seriesIcon;
  delete zoomIcon;
  delete dragIcon;
  delete filemenu;
}

}















