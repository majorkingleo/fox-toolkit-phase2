#include "fx.h"
#include "chart.h"
#include "CSVReader.h"
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
  FXLabel* info;
  FXVerticalFrame* series;
  std::vector<FXint> xcolumns;
  std::vector<FXCheckButton*> checks;
  std::vector<FXbool> visible;
  FXint selectedx;
  };


class ChartDemoWindow : public FXMainWindow {
  FXDECLARE(ChartDemoWindow)
protected:
  FXMenuBar* menubar;
  FXMenuPane* filemenu;
  FXTabBook* tabs;
  FXLabel* welcome;
  std::vector<ChartDocument*> documents;
protected:
  ChartDemoWindow(){ }
private:
  ChartDemoWindow(const ChartDemoWindow&);
  ChartDemoWindow& operator=(const ChartDemoWindow&);
public:
  long onCmdOpen(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onXColumn(FXObject*,FXSelector,void*);
  long onSeries(FXObject*,FXSelector,void*);
  enum{
    ID_OPEN=FXMainWindow::ID_LAST,
    ID_QUIT,
    ID_XCOLUMN,
    ID_SERIES,
    ID_LAST
    };
public:
  ChartDemoWindow(FXApp* app);
  void openFile(const FXString& filename);
  void rebuild(ChartDocument* document);
  virtual ~ChartDemoWindow();
  };


FXDEFMAP(ChartDemoWindow) ChartDemoWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_OPEN,ChartDemoWindow::onCmdOpen),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_QUIT,ChartDemoWindow::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_XCOLUMN,ChartDemoWindow::onXColumn),
  FXMAPFUNC(SEL_COMMAND,ChartDemoWindow::ID_SERIES,ChartDemoWindow::onSeries),
  };

FXIMPLEMENT(ChartDemoWindow,FXMainWindow,ChartDemoWindowMap,ARRAYNUMBER(ChartDemoWindowMap))


static const FXColor seriesColors[]={
  FXRGB(220,50,47),FXRGB(38,139,210),FXRGB(133,153,0),FXRGB(211,54,130),
  FXRGB(42,161,152),FXRGB(203,75,22),FXRGB(108,113,196),FXRGB(181,137,0),
  FXRGB(88,110,117),FXRGB(0,121,107),FXRGB(156,39,176),FXRGB(63,81,181)
  };

#if defined(WIN32)
static void attachConsole(){
  if(AttachConsole(ATTACH_PARENT_PROCESS)){
    std::freopen("CONOUT$","w",stdout);
    std::freopen("CONOUT$","w",stderr);
    }
  }
#endif


ChartDemoWindow::ChartDemoWindow(FXApp* app):FXMainWindow(app,"FOX Chart Demo",nullptr,nullptr,DECOR_ALL,20,20,1100,720){
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Open CSV...\tCtl-O\tOpen one or more delimited text files",nullptr,this,ID_OPEN);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",nullptr,this,ID_QUIT);
  new FXMenuTitle(menubar,"&File",nullptr,filemenu);

  tabs=new FXTabBook(this,nullptr,0,TABBOOK_TOPTABS|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXTabItem(tabs,"Welcome");
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,12,12,12,12);
  welcome=new FXLabel(page,"Open one or more CSV files to plot their numeric columns.\n\nThe importer detects common delimiters, quoted fields, UTF encodings and Windows-1252 text.\nDate/time columns are detected and selected as the default X axis when available.",nullptr,JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_Y);
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
  ChartDocument* document=new ChartDocument;
  document->filename=filename;
  document->plot=nullptr;
  document->xcolumn=nullptr;
  document->info=nullptr;
  document->series=nullptr;
  document->selectedx=-1;
  FXString error;
  if(!readCSV(filename,document->table,error)){
    FXMessageBox::error(this,MBOX_OK,"CSV Import Failed","%s",error.text());
    delete document;
    return;
    }

  for(FXint c=0; c<(FXint)document->table.columns.size(); c++){
    if(document->table.columns[c].numeric){
      document->xcolumns.push_back(c);
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

  FXString tabtitle=FXPath::name(filename);
  new FXTabItem(tabs,tabtitle);
  FXVerticalFrame* page=new FXVerticalFrame(tabs,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED,0,0,0,0,4,4,4,4);
  FXHorizontalFrame* controls=new FXHorizontalFrame(page,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,4,4,4,4,6,0);
  new FXLabel(controls,"X axis:",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  document->xcolumn=new FXComboBox(controls,32,this,ID_XCOLUMN,COMBOBOX_STATIC|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y);
  document->xcolumn->setNumVisible(18);
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    document->xcolumn->appendItem(document->table.columns[col].name);
    if(col==document->selectedx) document->xcolumn->setCurrentItem((FXint)i);
    }
  document->info=new FXLabel(controls,"",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y|LAYOUT_FILL_X);

  FXHorizontalFrame* content=new FXHorizontalFrame(page,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,4,0);
  FXVerticalFrame* chartarea=new FXVerticalFrame(content,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  document->plot=new FX2DPlot(chartarea,nullptr,0,CAPTION_ABOVE|CAPTION_SHOWN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,12,12,12,12);
  document->series=new FXVerticalFrame(content,LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,205,0,5,5,5,5,1,1);
  new FXLabel(document->series,"Series",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  FXScrollWindow* scroll=new FXScrollWindow(document->series,LAYOUT_FILL_X|LAYOUT_FILL_Y|VSCROLLER_ALWAYS|VSCROLLING_ON);
  FXVerticalFrame* list=new FXVerticalFrame(scroll,LAYOUT_FILL_X|LAYOUT_TOP|FRAME_NONE,0,0,0,0,2,2,2,2,0,0);
  for(FXuval i=0; i<document->xcolumns.size(); i++){
    FXint col=document->xcolumns[i];
    FXCheckButton* check=new FXCheckButton(list,document->table.columns[col].name,this,ID_SERIES,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_FILL_X);
    check->setCheck(true);
    if(col==document->selectedx) check->disable();
    document->checks.push_back(check);
    }
  documents.push_back(document);
  rebuild(document);
  page->create();
  tabs->setCurrent((FXint)documents.size());
  }


void ChartDemoWindow::rebuild(ChartDocument* document){
  FXint xcol=document->selectedx;
  FXint xindex=-1;
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
    LineStyle line={seriesColors[visiblecount%(sizeof(seriesColors)/sizeof(seriesColors[0]))],1,LINESTYLE_SOLID};
    curve->setLineStyle(line);
    document->plot->appendCurve(curve);
    visiblecount++;
    }
  document->plot->setCaption(FXPath::name(document->filename));
  document->plot->getAxis(FX2DChart::XLO)->setLabel(document->table.columns[xcol].name+(document->table.columns[xcol].timestamp?" (seconds from start)":""));
  document->plot->getAxis(FX2DChart::YLO)->setLabel("Value");
  document->plot->fitData();
  document->plot->update();
  FXint ycount=0;
  for(FXuval i=0; i<document->xcolumns.size(); i++) if((FXint)i!=xindex) ycount++;
  document->info->setText(FXString::value(document->table.rows)+" rows; "+FXString::value(visiblecount)+" plotted / "+FXString::value(ycount)+" numeric series; "+document->table.delimiter+"; "+document->table.encoding);
  }


long ChartDemoWindow::onXColumn(FXObject* sender,FXSelector,void*){
  for(FXuval d=0; d<documents.size(); d++){
    ChartDocument* document=documents[d];
    if(document->xcolumn!=sender) continue;
    FXint selected=document->xcolumn->getCurrentItem();
    if(selected>=0 && selected<(FXint)document->xcolumns.size()){
      document->selectedx=document->xcolumns[selected];
      rebuild(document);
      }
    return 1;
    }
  return 0;
  }


long ChartDemoWindow::onSeries(FXObject* sender,FXSelector,void*){
  for(FXuval d=0; d<documents.size(); d++){
    ChartDocument* document=documents[d];
    for(FXuval i=0; i<document->checks.size(); i++){
      if(document->checks[i]==sender){
        FXCheckButton* check=(FXCheckButton*)sender;
        document->visible[i]=check->getCheck();
        rebuild(document);
        return 1;
        }
      }
    }
  return 0;
  }


ChartDemoWindow::~ChartDemoWindow(){
  for(FXuval i=0; i<documents.size(); i++) delete documents[i];
  documents.clear();
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
