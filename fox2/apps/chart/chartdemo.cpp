#include "fx.h"
#include "FXSVGIcon.h"
#include "chart.h"
#include "CSVReader.h"
#include "ChartDemoWindow.h"
#include "ChartDemoUtils.h"
#include "ChartTabItem.h"
#include "ChartDetachedWindow.h"
#include "icons.h"
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstring>

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


FXDEFMAP(ChartDemoWindow) ChartDemoWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_OPEN,ChartDemoWindow::onCmdOpen),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_QUIT,ChartDemoWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XCOLUMN,ChartDemoWindow::onXColumn),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_X_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_Y_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_RESET_ZOOM,ChartDemoWindow::onResetZoom),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_PLOT_CHANGED,ChartDemoWindow::onPlotChanged),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_XSCROLL,ChartDemoWindow::onXScroll),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XSCROLL,ChartDemoWindow::onXScroll),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES,ChartDemoWindow::onSeries),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_ALL,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_NONE,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_TIMEOUT,ChartDemoWindow::ID_LOAD_POLL,ChartDemoWindow::onLoadPoll),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_SPLITTER_CHANGED,ChartDemoWindow::onSplitterChanged),
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
  FXint pananchorx;
  FXint pananchory;
  FXdouble panrange[4];
protected:
  ChartDemoPlot():panmode(false),panning(false),pananchorx(0),pananchory(0){ }
private:
  ChartDemoPlot(const ChartDemoPlot&);
  ChartDemoPlot& operator=(const ChartDemoPlot&);
public:
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  ChartDemoPlot(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FX2DPlot(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb),panmode(false),panning(false),pananchorx(0),pananchory(0){ }
  void setPanMode(FXbool mode){ panmode=mode; }
  };


FXDEFMAP(ChartDemoPlot) ChartDemoPlotMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,ChartDemoPlot::onLeftBtnPress),
  FXMAPFUNC(SEL_MOTION,0,ChartDemoPlot::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,ChartDemoPlot::onLeftBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,ChartDemoPlot::onUngrabbed),
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
  if(!panmode || !panning) return panmode?0:FX2DChart::onMotion(nullptr,0,ptr);
  FXEvent* event=(FXEvent*)ptr;
  const FXRectangle& area=getPlotArea();
  if(area.w<=0 || area.h<=0) return 1;
  FXdouble dx=(event->win_x-pananchorx)*(panrange[1]-panrange[0])/area.w;
  FXdouble dy=(event->win_y-pananchory)*(panrange[3]-panrange[2])/area.h;
  setViewRange(panrange[0]-dx,panrange[1]-dx,panrange[2]+dy,panrange[3]+dy);
  return 1;
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


static const FXColor seriesColors[]={
  FXRGB(220,50,47),FXRGB(38,139,210),FXRGB(133,153,0),FXRGB(211,54,130),
  FXRGB(42,161,152),FXRGB(203,75,22),FXRGB(108,113,196),FXRGB(181,137,0),
  FXRGB(88,110,117),FXRGB(0,121,107),FXRGB(156,39,176),FXRGB(63,81,181)
  };


static const FXchar detach_hover_svg[]="<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\"><rect x=\"0.5\" y=\"0.5\" width=\"15\" height=\"15\" rx=\"3\" fill=\"#dbeafe\" stroke=\"#60a5fa\"/><g fill=\"none\" stroke=\"#1d4ed8\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"1.4\"><path d=\"M9 3H4.5A1.5 1.5 0 0 0 3 4.5v7A1.5 1.5 0 0 0 4.5 13h7a1.5 1.5 0 0 0 1.5-1.5V7\"/><path d=\"M8 8l5-5m-4 0h4v4\"/></g></svg>";
static const FXchar detach_pressed_svg[]="<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\"><rect x=\"0.5\" y=\"0.5\" width=\"15\" height=\"15\" rx=\"3\" fill=\"#bfdbfe\" stroke=\"#2563eb\"/><g fill=\"none\" stroke=\"#1e40af\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"1.4\"><path d=\"M9 3H4.5A1.5 1.5 0 0 0 3 4.5v7A1.5 1.5 0 0 0 4.5 13h7a1.5 1.5 0 0 0 1.5-1.5V7\"/><path d=\"M8 8l5-5m-4 0h4v4\"/></g></svg>";


ChartDemoWindow::ChartDemoWindow(FXApp* app):FXMainWindow(app,"FOX Chart Demo",nullptr,nullptr,DECOR_ALL,20,20,1100,720){
  loader=nullptr;
  fontDPI=0;
  fontScale=0.0;
  pixelScale=0.0;
  verticalLabelFont=new FXFont(app,"helvetica",14,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  verticalLabelFontDesc=verticalLabelFont->getFontDesc();
  verticalLabelFont->setAngle(90*64);
  numberFont=app->getNormalFont();
  FXColor background=app->getTheme()->color(ROLE_WINDOW_BG);
  FXbool dark=(FXREDVAL(background)*299+FXGREENVAL(background)*587+FXBLUEVAL(background)*114)<128000;
  detachIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_detach:chart_detach),FXRGB(192,192,192),0,16,16);
  detachHoverIcon=new FXSVGIcon(app,(const FXuchar*)detach_hover_svg,FXRGB(192,192,192),0,16,16);
  detachPressedIcon=new FXSVGIcon(app,(const FXuchar*)detach_pressed_svg,FXRGB(192,192,192),0,16,16);
  resetZoomIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_reset_zoom:chart_reset_zoom),FXRGB(192,192,192),0,24,24);
  xAxisIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_x_axis_label:chart_x_axis_label),FXRGB(192,192,192),0,20,20);
  yAxisIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_y_axis_label:chart_y_axis_label),FXRGB(192,192,192),0,20,20);
  seriesIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_data_series:chart_data_series),FXRGB(192,192,192),0,20,20);
  cropIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_crop:chart_crop),FXRGB(192,192,192),0,20,20);
  dragIcon=new FXSVGIcon(app,(const FXuchar*)(dark?chart_dark_drag:chart_drag),FXRGB(192,192,192),0,20,20);
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Open CSV...\tCtl-O\tOpen one or more delimited text files",nullptr,this,ID_OPEN);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  new FXMenuTitle(menubar,"&File",nullptr,filemenu);

  statusbar=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);
  statusbar->getStatusLine()->setNormalText("Ready");
  loadingBar=new FXProgressBar(statusbar,nullptr,0,PROGRESSBAR_PERCENTAGE|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,220);
  loadingBar->setTotal(100);
  loadingBar->hide();
  tabs=new FXTabBook(this,nullptr,0,TABBOOK_TOPTABS|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXTabItem(tabs,"Welcome");
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,12,12,12,12);
  welcome=new FXLabel(page,"Open one or more CSV files to plot their numeric columns.\n\nThe importer detects common delimiters, quoted fields, UTF encodings and Windows-1252 text.\nDate/time columns are detected and selected as the default X axis when available.",nullptr,JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  setTarget(this);
  setSelector(ID_MAIN_CLOSE);
  }


// Create the main window and realize its chart font for the current monitor
void ChartDemoWindow::create(){
  FXMainWindow::create();
  detachIcon->create();
  detachHoverIcon->create();
  detachPressedIcon->create();
  resetZoomIcon->create();
  xAxisIcon->create();
  yAxisIcon->create();
  seriesIcon->create();
  cropIcon->create();
  dragIcon->create();
  updateChartFonts();
  }


// Refresh chart fonts when the window moves to a monitor with another DPI
void ChartDemoWindow::applyTheme(FXTheme* theme){
  FXMainWindow::applyTheme(theme);
  updateChartFonts();
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
    documents[i]->plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
    documents[i]->plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
    documents[i]->plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
    scaleDocumentWidgets(documents[i],this);
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
  document->plot->setMarginLeft(view->scale(12));
  document->plot->setMarginRight(view->scale(12));
  document->plot->setMarginTop(view->scale(12));
  document->plot->setMarginBottom(view->scale(12));
  document->plot->getAxis(FX2DChart::YLO)->setLabelOffset(view->scale(40));
  document->plot->getAxis(FX2DChart::YHI)->setLabelOffset(view->scale(40));
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
  document->plot->getAxis(FX2DChart::XLO)->setLabelFont(document->detachedNumberFont);
  document->plot->getAxis(FX2DChart::XHI)->setLabelFont(document->detachedNumberFont);
  document->plot->getAxis(FX2DChart::YLO)->setLabelFont(document->detachedVerticalFont);
  document->plot->getAxis(FX2DChart::YHI)->setLabelFont(document->detachedVerticalFont);
  document->plot->getAxis(FX2DChart::XLO)->setNumberFont(document->detachedNumberFont);
  document->plot->getAxis(FX2DChart::XHI)->setNumberFont(document->detachedNumberFont);
  document->plot->getAxis(FX2DChart::YLO)->setNumberFont(document->detachedNumberFont);
  document->plot->getAxis(FX2DChart::YHI)->setNumberFont(document->detachedNumberFont);
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
  FXString* filenames=FXFileDialog::getOpenFilenames(this,"Open CSV files",FXSystem::getCurrentDirectory(),"CSV Files (*.csv,*.tsv,*.txt)\nAll Files (*)");
  if(!filenames) return 1;
  for(FXint i=0; !filenames[i].empty(); i++) openFile(filenames[i]);
  delete [] filenames;
  return 1;
  }


long ChartDemoWindow::onCmdQuit(FXObject*,FXSelector,void*){
  getApp()->exit(0);
  return 1;
  }


void ChartDemoWindow::openFile(const FXString& filename){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->filename==filename){
      tabs->setCurrent((FXint)i+1);
      return;
      }
    }
  if(loader && loader->filename==filename) return;
  for(FXuval i=0; i<pendingFiles.size(); i++) if(pendingFiles[i]==filename) return;
  pendingFiles.push_back(filename);
  if(!loader) startNextLoad();
  }


void ChartDemoWindow::startNextLoad(){
  if(loader || pendingFiles.empty()) return;
  FXString filename=pendingFiles[0];
  pendingFiles.erase(pendingFiles.begin());
  loader=new CSVLoadTask(filename);
  if(!loader->start()){
    delete loader;
    loader=nullptr;
    FXMessageBox::error(this,MBOX_OK,"CSV Import Failed","Could not start the CSV loading thread.");
    startNextLoad();
    return;
    }
  statusbar->getStatusLine()->setText("Loading "+displayFileName(filename));
  loadingBar->setProgress(0);
  loadingBar->show();
  statusbar->recalc();
  getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_LOAD_POLL),100000000);
  }


long ChartDemoWindow::onLoadPoll(FXObject*,FXSelector,void*){
  if(!loader) return 1;
  loadingBar->setProgress(loader->getProgress());
  if(loader->running()){
    getApp()->addTimeout(this,FXSEL(SEL_TIMEOUT,ID_LOAD_POLL),100000000);
    return 1;
    }
  loader->join();
  CSVLoadTask* task=loader;
  loader=nullptr;
  if(task->success){
    loadingBar->setProgress(100);
    addLoadedFile(task->filename,task->table);
    statusbar->getStatusLine()->setNormalText("Loaded "+displayFileName(task->filename));
    }
  else{
    FXMessageBox::error(this,MBOX_OK,"CSV Import Failed","%s",task->error.text());
    statusbar->getStatusLine()->setNormalText("Ready");
    }
  statusbar->getStatusLine()->setText("");
  delete task;
  if(!pendingFiles.empty()){
    startNextLoad();
    }
  else{
    loadingBar->hide();
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
  document->xscroll=nullptr;
  document->info=nullptr;
  document->series=nullptr;
  document->seriesfilter=nullptr;
  document->serieslist=nullptr;
  document->splitter=nullptr;
  document->seriesshow=nullptr;
  document->seriesclose=nullptr;
  document->serieswidth=205;
  document->seriesshown=true;
  document->filtertext="";
  document->selectedx=-1;
  for(FXint column=0; column<(FXint)document->table.columns.size(); column++){
    if(document->table.columns[column].numeric){
      document->xcolumns.push_back(column);
      document->seriesKeys.push_back(chartColumnKey(document->table,column));
      document->visible.push_back(true);
      }
    }
  if(document->xcolumns.empty()){
    FXMessageBox::error(this,MBOX_OK,"CSV Import Failed","No plottable columns were found.");
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
  document->ytoggle=new FXToggleButton(charttoolbar,"","",yAxisIcon,yAxisIcon,this,ID_Y_DESCRIPTION,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->ytoggle->setTipText("Y Description");
  document->ytoggle->setAltTipText("Y Description");
  document->xtoggle->setState(true);
  document->ytoggle->setState(true);
  document->resetzoom=new FXButton(charttoolbar,"",resetZoomIcon,this,ID_RESET_ZOOM,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->resetzoom->setTipText("Reset zoom");
  document->resetzoom->disable();
  document->seriesshow=new FXToggleButton(charttoolbar,"","",seriesIcon,seriesIcon,this,ID_SERIES_PANEL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->seriesshow->setTipText("Show/hide the series panel");
  document->cropbutton=new FXToggleButton(charttoolbar,"","",cropIcon,cropIcon,this,ID_PLOT_TOOL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->cropbutton->setTipText("Crop to a rectangular plot selection");
  document->dragbutton=new FXToggleButton(charttoolbar,"","",dragIcon,dragIcon,this,ID_PLOT_TOOL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  document->dragbutton->setTipText("Drag to pan the plot");
  document->cropbutton->setState(true);
  FXHorizontalFrame* controls=new FXHorizontalFrame(page,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,4,4,4,4,6,0);
  document->info=new FXLabel(controls,"",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);

  FXSplitter* content=new FXSplitter(page,this,ID_SPLITTER_CHANGED,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_TOP|SPLITTER_REVERSED|SPLITTER_TRACKING);
  document->splitter=content;
  FXVerticalFrame* chartarea=new FXVerticalFrame(content,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  FXVerticalFrame* chartcolumn=new FXVerticalFrame(chartarea,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  document->plot=new ChartDemoPlot(chartcolumn,this,ID_PLOT_CHANGED,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,12,12,12,12);
  document->xscroll=new FXScrollBar(chartcolumn,this,ID_XSCROLL,SCROLLBAR_HORIZONTAL|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
  document->xscroll->setLine(1000);
  document->plot->getAxis(FX2DChart::XHI)->setStyle(AXIS_OFF);
  document->plot->getAxis(FX2DChart::YHI)->setStyle(AXIS_OFF);
  document->plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
  document->plot->getAxis(FX2DChart::YLO)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_RIGHT);
  document->plot->getAxis(FX2DChart::YLO)->setLabelOffset(40);
  document->plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
  document->plot->getAxis(FX2DChart::YHI)->setLabelStyle(TEXT_JUSTIFY_CENTER|TEXT_ATTACH_LEFT);
  document->plot->getAxis(FX2DChart::YHI)->setLabelOffset(40);
  document->plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
  document->plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
  document->plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
  FXSettings& settings=getApp()->reg();
  FXbool xdescription=FXString(settings.readStringEntry(document->settingsSection.text(),"xdescription","1"))!="0";
  FXbool ydescription=FXString(settings.readStringEntry(document->settingsSection.text(),"ydescription","1"))!="0";
  document->xtoggle->setState(xdescription);
  document->ytoggle->setState(ydescription);
  document->filtertext=settings.readStringEntry(document->settingsSection.text(),"seriesfilter","");
  document->seriesshown=FXString(settings.readStringEntry(document->settingsSection.text(),"seriesshown","1"))!="0";
  if(!xdescription) document->plot->getAxis(FX2DChart::XLO)->setStyle(document->plot->getAxis(FX2DChart::XLO)->getStyle()&~AXIS_LABEL);
  if(!ydescription) document->plot->getAxis(FX2DChart::YLO)->setStyle(document->plot->getAxis(FX2DChart::YLO)->getStyle()&~AXIS_LABEL);
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
    swatch->setBackColor(seriesColors[i%(sizeof(seriesColors)/sizeof(seriesColors[0]))]);
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
  if(getApp()->reg().existingEntry(document->settingsSection.text(),"split")){
    FXint split=getApp()->reg().readIntEntry(document->settingsSection.text(),"split",205);
    if(split>0) document->serieswidth=FXCLAMP(180,split,400);
    }
  if(!document->seriesshown) document->series->hide();
  else content->setSplit(1,scale(document->serieswidth));
  document->seriesshow->setState(document->series->shown());
  applySeriesFilter(document);
  if(FXString(settings.readStringEntry(document->settingsSection.text(),"zoomed","0"))=="1" && settings.existingEntry(document->settingsSection.text(),"xminimum")){
    document->plot->setViewRange(settings.readRealEntry(document->settingsSection.text(),"xminimum"),settings.readRealEntry(document->settingsSection.text(),"xmaximum"),settings.readRealEntry(document->settingsSection.text(),"yminimum"),settings.readRealEntry(document->settingsSection.text(),"ymaximum"));
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
  for(FXuval i=0; i<document->seriesKeys.size(); i++){
    FXString entry="series-"+chartHash(document->seriesKeys[i]);
    document->visible[i]=FXString(settings.readStringEntry(document->settingsSection.text(),entry,"1")).toInt()!=0;
    }
  }


void ChartDemoWindow::saveDocumentSettings(ChartDocument* document){
  FXSettings& settings=getApp()->reg();
  settings.writeStringEntry(document->settingsSection.text(),"xaxis",chartColumnKey(document->table,document->selectedx).text());
  for(FXuval i=0; i<document->seriesKeys.size(); i++){
    FXString entry="series-"+chartHash(document->seriesKeys[i]);
    settings.writeStringEntry(document->settingsSection.text(),entry,document->visible[i]?"1":"0");
    }
  settings.writeStringEntry(document->settingsSection.text(),"xdescription",document->xtoggle && document->xtoggle->getState()?"1":"0");
  settings.writeStringEntry(document->settingsSection.text(),"ydescription",document->ytoggle && document->ytoggle->getState()?"1":"0");
  settings.writeStringEntry(document->settingsSection.text(),"seriesfilter",document->seriesfilter?document->seriesfilter->getText().text():document->filtertext.text());
  settings.writeStringEntry(document->settingsSection.text(),"seriesshown",document->series && document->series->shown()?"1":"0");
  if(document->plot){
    FXAxis* xaxis=document->plot->getAxis(FX2DChart::XLO);
    FXAxis* yaxis=document->plot->getAxis(FX2DChart::YLO);
    settings.writeStringEntry(document->settingsSection.text(),"zoomed",document->plot->isZoomed()?"1":"0");
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
  FXbool waszoomed=keepView && document->plot->isZoomed();
  FXdouble xminimum=document->plot->getAxis(FX2DChart::XLO)->getAxisRangeMin();
  FXdouble xmaximum=document->plot->getAxis(FX2DChart::XLO)->getAxisRangeMax();
  FXdouble yminimum=document->plot->getAxis(FX2DChart::YLO)->getAxisRangeMin();
  FXdouble ymaximum=document->plot->getAxis(FX2DChart::YLO)->getAxisRangeMax();
  for(FXuval i=0; i<document->xcolumns.size(); i++) if(document->xcolumns[i]==xcol) xindex=(FXint)i;
  if(xindex<0) return;
  document->plot->clearCurves();
  FXint visiblecount=0;
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    document->checks[i]->setCheck(document->visible[i] && col!=xcol);
    if(col==xcol) document->checks[i]->disable();
    else document->checks[i]->enable();
    if(col==xcol) continue;
    if(!document->visible[i]) continue;
    FXCurve* curve=new FXCurve(document->plot,document->table.columns[col].name);
    FXData* xdata=new FXData;
    FXData* ydata=new FXData;
    xdata->reserve(document->table.rows);
    ydata->reserve(document->table.rows);
    for(FXint row=0; row<document->table.rows; row++){
      xdata->append(document->table.columns[xcol].values[row]);
      ydata->append(document->table.columns[col].values[row]);
      }
    curve->setXData(xdata);
    curve->setYData(ydata);
    LineStyle line={seriesColors[i%(sizeof(seriesColors)/sizeof(seriesColors[0]))],1,LINESTYLE_SOLID};
    curve->setLineStyle(line);
    document->plot->appendCurve(curve);
    visiblecount++;
    }
  document->plot->getAxis(FX2DChart::XLO)->setLabel(document->table.columns[xcol].name+(document->table.columns[xcol].timestamp?" (seconds from start)":""));
  document->plot->getAxis(FX2DChart::YLO)->setLabel("Value");
  document->plot->fitData();
  if(waszoomed) document->plot->setViewRange(xminimum,xmaximum,yminimum,ymaximum);
  document->plot->invalidateChart();
  FXint ycount=0;
  for(FXuval i=0; i<document->xcolumns.size(); i++) if((FXint)i!=xindex) ycount++;
  document->info->setText(FXString::value(document->table.rows)+" rows; "+FXString::value(visiblecount)+" plotted / "+FXString::value(ycount)+" numeric series; "+document->table.delimiter+"; "+document->table.encoding);
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
        if(document->plot->isZoomed()) document->resetzoom->enable();
        else document->resetzoom->disable();
        saveDocumentSettings(document);
        }
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
      document->plot->invalidateChart();
      saveDocumentSettings(document);
      return 1;
      }
    if(FXSELID(sel)==ID_Y_DESCRIPTION && document->ytoggle==sender){
      FXAxis* axis=document->plot->getAxis(FX2DChart::YLO);
      if(shown) axis->setStyle(axis->getStyle()|AXIS_LABEL);
      else axis->setStyle(axis->getStyle()&~AXIS_LABEL);
      document->plot->invalidateChart();
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onResetZoom(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->resetzoom==sender){
      document->plot->resetZoom();
      document->resetzoom->disable();
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


long ChartDemoWindow::onPlotTool(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(sender!=document->cropbutton && sender!=document->dragbutton) continue;
    FXbool panmode=sender==document->dragbutton && document->dragbutton->getState();
    document->cropbutton->setState(!panmode);
    document->dragbutton->setState(panmode);
    static_cast<ChartDemoPlot*>(document->plot)->setPanMode(panmode);
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onPlotChanged(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->plot==sender){
      if(document->plot->isZoomed()) document->resetzoom->enable();
      else document->resetzoom->disable();
      updateXScroll(document);
      saveDocumentSettings(document);
      return 1;
      }
    }
  return 0;
  }


void ChartDemoWindow::updateXScroll(ChartDocument* document){
  FXAxis* axis=document->plot->getAxis(FX2DChart::XLO);
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
    FXAxis* axis=document->plot->getAxis(FX2DChart::XLO);
    FXdouble datamin=axis->getDataRangeMin();
    FXdouble datamax=axis->getDataRangeMax();
    FXdouble datarange=datamax-datamin;
    FXdouble viewrange=axis->getAxisRangeMax()-axis->getAxisRangeMin();
    if(datarange<=0.0 || viewrange<=0.0) return 1;
    FXint position=(FXint)(FXival)ptr;
    FXdouble xminimum=datamin+datarange*(FXdouble)position/document->xscroll->getRange();
    FXdouble xmaximum=xminimum+viewrange;
    FXAxis* yaxis=document->plot->getAxis(FX2DChart::YLO);
    document->plot->setViewRange(xminimum,xmaximum,yaxis->getAxisRangeMin(),yaxis->getAxisRangeMax());
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


long ChartDemoWindow::onSplitterChanged(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    if(documents[i]->splitter==sender){
      saveDocumentSettings(documents[i]);
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
  document->plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
  document->plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
  document->plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
  document->plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
  document->plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
  document->plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
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
  if(loader){
    loader->join();
    delete loader;
    loader=nullptr;
    }
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
  delete resetZoomIcon;
  delete xAxisIcon;
  delete yAxisIcon;
  delete seriesIcon;
  delete cropIcon;
  delete dragIcon;
  delete filemenu;
}

}















