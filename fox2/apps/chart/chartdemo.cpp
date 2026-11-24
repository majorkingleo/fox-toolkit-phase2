#include "fx.h"
#include "chart.h"
#include "CSVReader.h"
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstring>

#if defined(WIN32)
#include <windows.h>
#endif

using namespace FX;
using namespace FXCHART;


struct ChartDocument {
  FXString filename;
  CSVTable table;
  FX2DPlot* plot;
  FXComboBox* xcolumn;
  FXToggleButton* xtoggle;
  FXToggleButton* ytoggle;
  FXButton* resetzoom;
  FXLabel* info;
  FXVerticalFrame* series;
  FXTextField* seriesfilter;
  FXVerticalFrame* serieslist;
  FXSplitter* splitter;
  FXToggleButton* seriesshow;
  FXButton* seriesclose;
  FXButton* selectAll;
  FXButton* selectNone;
  std::vector<FXint> xcolumns;
  std::vector<FXString> seriesKeys;
  std::vector<FXCheckButton*> checks;
  std::vector<FXHorizontalFrame*> seriesrows;
  std::vector<FXbool> visible;
  FXint selectedx;
  FXint serieswidth;
  FXbool seriesshown;
  FXString filtertext;
  FXString settingsSection;
  };


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


class ChartDemoWindow : public FXMainWindow {
  FXDECLARE(ChartDemoWindow)
protected:
  FXMenuBar* menubar;
  FXMenuPane* filemenu;
  FXTabBook* tabs;
  FXStatusBar* statusbar;
  FXProgressBar* loadingBar;
  FXFont* verticalLabelFont;
  FXFont* numberFont;
  FXFontDesc verticalLabelFontDesc;
  FXint fontDPI;
  FXdouble fontScale;
  FXLabel* welcome;
  std::vector<ChartDocument*> documents;
  std::vector<FXString> pendingFiles;
  CSVLoadTask* loader;
protected:
  ChartDemoWindow(){ }
private:
  ChartDemoWindow(const ChartDemoWindow&);
  ChartDemoWindow& operator=(const ChartDemoWindow&);
public:
  long onCmdOpen(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onXColumn(FXObject*,FXSelector,void*);
  long onAxisDescription(FXObject*,FXSelector,void*);
  long onResetZoom(FXObject*,FXSelector,void*);
  long onPlotChanged(FXObject*,FXSelector,void*);
  long onSeries(FXObject*,FXSelector,void*);
  long onSeriesSelection(FXObject*,FXSelector,void*);
  long onLoadPoll(FXObject*,FXSelector,void*);
  long onSplitterChanged(FXObject*,FXSelector,void*);
  long onSeriesFilter(FXObject*,FXSelector,void*);
  long onSeriesPanel(FXObject*,FXSelector,void*);
  void updateChartFonts();
  void applySeriesFilter(ChartDocument* document);
  void loadDocumentSettings(ChartDocument* document);
  void saveDocumentSettings(ChartDocument* document);
  enum{
    ID_OPEN=FXMainWindow::ID_LAST,
    ID_QUIT,
    ID_XCOLUMN,
    ID_X_DESCRIPTION,
    ID_Y_DESCRIPTION,
    ID_RESET_ZOOM,
    ID_PLOT_CHANGED,
    ID_SERIES,
    ID_SERIES_ALL,
    ID_SERIES_NONE,
    ID_LOAD_POLL,
    ID_SPLITTER_CHANGED,
    ID_SERIES_FILTER,
    ID_SERIES_PANEL,
    ID_LAST
    };
public:
  ChartDemoWindow(FXApp* app);
  virtual void create() override;
  virtual void applyTheme(FXTheme* theme) override;
  void openFile(const FXString& filename);
  void startNextLoad();
  void addLoadedFile(const FXString& filename,CSVTable& table);
  void rebuild(ChartDocument* document,FXbool keepView=false);
  virtual ~ChartDemoWindow();
  };


FXDEFMAP(ChartDemoWindow) ChartDemoWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_OPEN,ChartDemoWindow::onCmdOpen),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_QUIT,ChartDemoWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XCOLUMN,ChartDemoWindow::onXColumn),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_X_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_Y_DESCRIPTION,ChartDemoWindow::onAxisDescription),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_RESET_ZOOM,ChartDemoWindow::onResetZoom),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_PLOT_CHANGED,ChartDemoWindow::onPlotChanged),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES,ChartDemoWindow::onSeries),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_ALL,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_NONE,ChartDemoWindow::onSeriesSelection),
  FXMAPFUNC(SEL_TIMEOUT,ChartDemoWindow::ID_LOAD_POLL,ChartDemoWindow::onLoadPoll),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_SPLITTER_CHANGED,ChartDemoWindow::onSplitterChanged),
  FXMAPFUNC(SEL_CHANGED,ChartDemoWindow::ID_SERIES_FILTER,ChartDemoWindow::onSeriesFilter),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES_PANEL,ChartDemoWindow::onSeriesPanel),
  };

FXIMPLEMENT(ChartDemoWindow,FXMainWindow,ChartDemoWindowMap,ARRAYNUMBER(ChartDemoWindowMap))


static const FXColor seriesColors[]={
  FXRGB(220,50,47),FXRGB(38,139,210),FXRGB(133,153,0),FXRGB(211,54,130),
  FXRGB(42,161,152),FXRGB(203,75,22),FXRGB(108,113,196),FXRGB(181,137,0),
  FXRGB(88,110,117),FXRGB(0,121,107),FXRGB(156,39,176),FXRGB(63,81,181)
  };


static FXString chartHash(const FXString& text){
  FXulong hash=14695981039346656037ULL;
  for(FXint i=0; i<text.length(); i++){
    hash^=(FXuchar)text[i];
    hash*=1099511628211ULL;
    }
  return FXString::value(hash,16);
  }


static FXString chartColumnKey(const CSVTable& table,FXint column){
  FXint occurrence=0;
  for(FXint i=0; i<column; i++){
    if(table.columns[i].numeric && table.columns[i].name==table.columns[column].name && table.columns[i].timestamp==table.columns[column].timestamp) occurrence++;
    }
  return table.columns[column].name+"|"+(table.columns[column].timestamp?"T":"N")+"|"+FXString::value(occurrence);
  }


static FXString chartSchemaKey(const std::vector<FXString>& keys){
  std::vector<FXString> sorted=keys;
  FXString signature;
  std::sort(sorted.begin(),sorted.end());
  for(FXuval i=0; i<sorted.size(); i++) signature+=FXString::value(sorted[i].length())+":"+sorted[i]+";";
  return "Schema-"+chartHash(signature);
  }


static FXFont* makeChartFont(FXApp* app,const FXFontDesc& desc,FXint angle,FXint dpi,FXdouble scale){
  FXFontDesc scaled=desc;
  scaled.size=FXint(FXdouble(desc.size)*(scale/(FXdouble(dpi)/96.0))+0.5);
  FXFont* font=new FXFont(app,scaled);
  if(angle) font->setAngle(angle);
  font->setResolution(dpi);
  font->create();
  return font;
  }


static FXString displayFileName(const FXString& filename){
  FXint start=0;
  for(FXint i=0; i<filename.length(); i++){
    if(filename[i]=='/' || filename[i]=='\\') start=i+1;
    }
  return filename.mid(start,filename.length()-start);
  }

#if defined(WIN32)
static void attachConsole(){
  if(AttachConsole(ATTACH_PARENT_PROCESS)){
    std::freopen("CONOUT$","w",stdout);
    std::freopen("CONOUT$","w",stderr);
    }
  }
#endif


ChartDemoWindow::ChartDemoWindow(FXApp* app):FXMainWindow(app,"FOX Chart Demo",nullptr,nullptr,DECOR_ALL,20,20,1100,720){
  loader=nullptr;
  fontDPI=0;
  fontScale=0.0;
  verticalLabelFont=new FXFont(app,"helvetica",14,FXFont::Normal,FXFont::Straight,FONTENCODING_DEFAULT,FXFont::NonExpanded,FXFont::Scalable|FXFont::Rotatable);
  verticalLabelFontDesc=verticalLabelFont->getFontDesc();
  verticalLabelFont->setAngle(90*64);
  numberFont=app->getNormalFont();
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
  }


// Create the main window and realize its chart font for the current monitor
void ChartDemoWindow::create(){
  FXMainWindow::create();
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
  FXbool scalechanged=fontDPI>0 && windowdpi>0 && windowdpi!=fontDPI;
  FXFont* oldfont;
  if(windowdpi<=0) windowdpi=96;
  if(windowdpi==fontDPI && scale==fontScale) return;
  oldfont=verticalLabelFont;
  verticalLabelFont=makeChartFont(getApp(),verticalLabelFontDesc,oldfont->getAngle(),windowdpi,scale);
  numberFont=getApp()->getNormalFont(getWindowDPI());
  for(FXuval i=0; i<documents.size(); i++){
    documents[i]->plot->getAxis(FX2DChart::XLO)->setLabelFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::XHI)->setLabelFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YLO)->setLabelFont(verticalLabelFont);
    documents[i]->plot->getAxis(FX2DChart::YHI)->setLabelFont(verticalLabelFont);
    documents[i]->plot->getAxis(FX2DChart::XLO)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::XHI)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YLO)->setNumberFont(numberFont);
    documents[i]->plot->getAxis(FX2DChart::YHI)->setNumberFont(numberFont);
    if(scalechanged && documents[i]->series->shown()){
      documents[i]->splitter->setSplit(1,this->scale(documents[i]->serieswidth));
      documents[i]->splitter->recalc();
      documents[i]->splitter->layout();
      }
    }
  delete oldfont;
  fontDPI=windowdpi;
  fontScale=scale;
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
  document->plot=nullptr;
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

  for(FXint c=0; c<(FXint)document->table.columns.size(); c++){
    if(document->table.columns[c].numeric){
      document->xcolumns.push_back(c);
      document->seriesKeys.push_back(chartColumnKey(document->table,c));
      document->visible.push_back(true);
      }
    }
  if(document->xcolumns.empty()){
    FXMessageBox::error(this,MBOX_OK,"CSV Import Failed","No plottable columns were found.");
    delete document;
    return;
    }
  for(FXint i=0; i<(FXint)document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    if(document->table.columns[col].timestamp){ document->selectedx=col; break; }
    }
  if(document->selectedx<0) document->selectedx=document->xcolumns[0];
  document->settingsSection=chartSchemaKey(document->seriesKeys);
  loadDocumentSettings(document);

  FXString tabtitle=displayFileName(filename);
  FXTabItem* tabitem=new FXTabItem(tabs,tabtitle);
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,4,4,4,4);
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
  document->xtoggle=new FXToggleButton(charttoolbar,"X Description\tShow X axis description","X Description\tHide X axis description",nullptr,nullptr,this,ID_X_DESCRIPTION,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->ytoggle=new FXToggleButton(charttoolbar,"Y Description\tShow Y axis description","Y Description\tHide Y axis description",nullptr,nullptr,this,ID_Y_DESCRIPTION,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->xtoggle->setState(true);
  document->ytoggle->setState(true);
  document->resetzoom=new FXButton(charttoolbar,"Reset Zoom",nullptr,this,ID_RESET_ZOOM,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  document->resetzoom->disable();
  document->seriesshow=new FXToggleButton(charttoolbar,"Series","Series",nullptr,nullptr,this,ID_SERIES_PANEL,TOGGLEBUTTON_TOOLBAR|TOGGLEBUTTON_KEEPSTATE|FRAME_RAISED|LAYOUT_CENTER_Y|LAYOUT_SIDE_RIGHT);
  FXHorizontalFrame* controls=new FXHorizontalFrame(page,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,4,4,4,4,6,0);
  document->info=new FXLabel(controls,"",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);

  FXSplitter* content=new FXSplitter(page,this,ID_SPLITTER_CHANGED,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_TOP|SPLITTER_REVERSED|SPLITTER_TRACKING);
  document->splitter=content;
  FXVerticalFrame* chartarea=new FXVerticalFrame(content,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  document->plot=new FX2DPlot(chartarea,this,ID_PLOT_CHANGED,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,12,12,12,12);
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
  documents.push_back(document);
  rebuild(document);
  tabitem->create();
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
    if(width>0) document->serieswidth=(FXint)(width/getScale()+0.5);
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


long ChartDemoWindow::onPlotChanged(FXObject* sender,FXSelector,void*){
  for(FXuval i=0; i<documents.size(); i++){
    ChartDocument* document=documents[i];
    if(document->plot==sender){
      if(document->plot->isZoomed()) document->resetzoom->enable();
      else document->resetzoom->disable();
      saveDocumentSettings(document);
      return 1;
      }
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
      document->splitter->setSplit(1,scale(document->serieswidth));
      }
    else if(!shown && document->series->shown()){
      FXint width=document->splitter->getSplit(1);
      if(width>0) document->serieswidth=(FXint)(width/getScale()+0.5);
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


ChartDemoWindow::~ChartDemoWindow(){
  getApp()->removeTimeout(this,FXSEL(SEL_TIMEOUT,ID_LOAD_POLL));
  if(loader){
    loader->join();
    delete loader;
    loader=nullptr;
    }
  for(FXuval i=0; i<documents.size(); i++){
    saveDocumentSettings(documents[i]);
    delete documents[i];
    }
  documents.clear();
  pendingFiles.clear();
  delete tabs;
  getApp()->reg().write();
  delete verticalLabelFont;
  delete filemenu;
  }


int main(int argc,char* argv[]){
  FXint out=1;
  for(FXint i=1; i<argc; i++){
    if(std::strcmp(argv[i],"--no-dialogs")!=0) argv[out++]=argv[i];
    }
  argc=out;
  argv[argc]=nullptr;
  for(FXint i=1; i+1<argc; i++){
    if(std::strcmp(argv[i],"--check-import")==0){
#if defined(WIN32)
      attachConsole();
#endif
      CSVTable table;
      FXString error;
      if(!readCSV(argv[i+1],table,error)){
        std::fprintf(stderr,"%s\n",error.text());
        return 1;
        }
      FXint numeric=0,timestamps=0;
      for(FXuval c=0; c<table.columns.size(); c++){
        if(table.columns[c].numeric) numeric++;
        if(table.columns[c].timestamp) timestamps++;
        }
      std::printf("%d rows, %d columns, %d numeric, %d timestamp; delimiter=%s; encoding=%s\n",table.rows,(FXint)table.columns.size(),numeric,timestamps,table.delimiter.text(),table.encoding.text());
      return 0;
      }
    }
  FXApp application("ChartDemo","FOX Chart Demo");
  application.init(argc,argv);
  ChartDemoWindow* window=new ChartDemoWindow(&application);
  application.create();
  window->show(PLACEMENT_SCREEN);
  for(FXint i=1; i<argc; i++) window->openFile(argv[i]);
  return application.run();
  }
