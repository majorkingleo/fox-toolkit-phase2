#ifndef CHARTDEMOWINDOW_H
#define CHARTDEMOWINDOW_H

#include "fx.h"
#include "FXSVGIcon.h"
#include "chart.h"
#include "CSVReader.h"
#include <vector>

namespace FXCHARTDEMO {

using namespace FX;
using namespace FXCHART;


struct ChartDocument {
  FXString filename;
  CSVTable table;
  FXTabItem* tabitem;
  FXVerticalFrame* page;
  FXVerticalFrame* placeholder;
  FXMainWindow* detachWindow;
  FXVerticalFrame* detachHost;
  FXFont* detachedVerticalFont;
  FXFont* detachedNumberFont;
  FXint detachedFontDPI;
  FXdouble detachedFontScale;
  FXdouble detachedPixelScale;
  FX2DPlot* plot;
  FXScrollBar* xscroll;
  FXComboBox* xcolumn;
  FXToggleButton* xtoggle;
  FXToggleButton* ytoggle;
  FXButton* autofity;
  FXButton* resetzoom;
  FXToggleButton* zoombutton;
  FXToggleButton* dragbutton;
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


class CSVLoadTask;


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
  FXSVGIcon* detachIcon;
  FXSVGIcon* detachHoverIcon;
  FXSVGIcon* detachPressedIcon;
  FXSVGIcon* autoFitYIcon;
  FXSVGIcon* resetZoomIcon;
  FXSVGIcon* xAxisIcon;
  FXSVGIcon* yAxisIcon;
  FXSVGIcon* seriesIcon;
  FXSVGIcon* zoomIcon;
  FXSVGIcon* dragIcon;
  FXMenuRadio* classicTheme;
  FXMenuRadio* darkTheme;
  FXMenuRadio* breezeTheme;
  FXMenuRadio* breezeDarkTheme;
  FXuint themePreset;
  FXbool iconsDark;
  FXFontDesc verticalLabelFontDesc;
  FXint fontDPI;
  FXdouble fontScale;
  FXdouble pixelScale;
  FXLabel* welcome;
  std::vector<ChartDocument*> documents;
  std::vector<FXString> pendingFiles;
  std::vector<CSVLoadTask*> loaders;
protected:
  ChartDemoWindow(){ }
private:
  ChartDemoWindow(const ChartDemoWindow&);
  ChartDemoWindow& operator=(const ChartDemoWindow&);
public:
  long onCmdOpen(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onTheme(FXObject*,FXSelector sel,void*);
  long onXColumn(FXObject*,FXSelector,void*);
  long onAxisDescription(FXObject*,FXSelector,void*);
  long onAutoFitY(FXObject* sender,FXSelector,void*);
  long onResetZoom(FXObject*,FXSelector,void*);
  long onPlotTool(FXObject* sender,FXSelector,void*);
  long onPlotChanged(FXObject*,FXSelector,void*);
  long onXScroll(FXObject* sender,FXSelector,void* ptr);
  long onSeries(FXObject*,FXSelector,void*);
  long onSeriesSelection(FXObject*,FXSelector,void*);
  long onLoadPoll(FXObject*,FXSelector,void*);
  long onSplitterChanged(FXObject*,FXSelector,void*);
  long onSeriesFilter(FXObject*,FXSelector,void*);
  long onSeriesPanel(FXObject*,FXSelector,void*);
  long onDetach(FXObject*,FXSelector,void*);
  long onDetachedClose(FXObject*,FXSelector,void*);
  long onMainClose(FXObject*,FXSelector,void*);
  void updateChartFonts();
    void updateChartIcons(FXbool dark);
    void updateChartColors(FXTheme* theme);
  void scaleDocumentWidgets(ChartDocument* document,FXWindow* view);
  void updateDetachedChartFonts(ChartDocument* document,FXTopWindow* view);
  void detachDocument(ChartDocument* document);
  void attachDocument(ChartDocument* document,FXbool select=true);
  void applySeriesFilter(ChartDocument* document);
  void updateXScroll(ChartDocument* document);
  void loadDocumentSettings(ChartDocument* document);
  void saveDocumentSettings(ChartDocument* document);
  enum{
    ID_OPEN=FXMainWindow::ID_LAST,
    ID_QUIT,
    ID_THEME_CLASSIC,
    ID_THEME_DARK,
    ID_THEME_BREEZE,
    ID_THEME_BREEZE_DARK,
    ID_XCOLUMN,
    ID_X_DESCRIPTION,
    ID_Y_DESCRIPTION,
    ID_AUTOFIT_Y,
    ID_RESET_ZOOM,
    ID_PLOT_CHANGED,
    ID_SERIES,
    ID_SERIES_ALL,
    ID_SERIES_NONE,
    ID_LOAD_POLL,
    ID_XSCROLL,
    ID_SPLITTER_CHANGED,
    ID_SERIES_FILTER,
    ID_SERIES_PANEL,
    ID_DETACH,
    ID_PLOT_TOOL,
    ID_DETACHED_CLOSE,
    ID_MAIN_CLOSE,
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

}

#endif
