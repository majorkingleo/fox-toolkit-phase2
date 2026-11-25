#ifndef CHARTASSIGNMENTDIALOG_H
#define CHARTASSIGNMENTDIALOG_H

#include "fx.h"
#include "CSVReader.h"
#include <vector>

namespace FXCHARTDEMO {

using namespace FX;
using namespace FXCHART;


class ChartAssignmentDialog : public FXDialogBox {
  FXDECLARE(ChartAssignmentDialog)
protected:
  FXList* series;
  FXTextField* seriesFilter;
  FXVerticalFrame* chartContainer;
  FXScrollWindow* chartScroll;
  std::vector<FXGroupBox*> chartGroups;
  std::vector<FXTextField*> chartFilters;
  std::vector<FXList*> chartLists;
  std::vector<FXButton*> chartAddButtons;
  std::vector<FXButton*> chartClearButtons;
  std::vector<FXButton*> chartRemoveButtons;
  std::vector<FXint> columns;
  std::vector<FXint> assignment;
  const CSVTable* table;
  FXDragType seriesDragType;
  FXint xcolumn;
  FXint plotCount;
  std::vector<FXint> draggedSeries;
  void refreshSeries();
  void updateChartLists();
  void ensureChartViews();
private:
  ChartAssignmentDialog(){ }
  ChartAssignmentDialog(const ChartAssignmentDialog&);
  ChartAssignmentDialog& operator=(const ChartAssignmentDialog&);
public:
  long onSeriesFilter(FXObject*,FXSelector,void*);
  long onChartFilter(FXObject*,FXSelector,void*);
  long onBeginDrag(FXObject*,FXSelector,void*);
  long onDragged(FXObject*,FXSelector,void*);
  long onEndDrag(FXObject*,FXSelector,void*);
  long onDNDRequest(FXObject*,FXSelector,void*);
  long onDNDEnter(FXObject*,FXSelector,void*);
  long onDNDMotion(FXObject*,FXSelector,void*);
  long onDNDDrop(FXObject*,FXSelector,void*);
  long onAddPlot(FXObject*,FXSelector,void*);
  long onAddAll(FXObject*,FXSelector,void*);
  long onClearPlot(FXObject*,FXSelector,void*);
  long onRemovePlot(FXObject*,FXSelector,void*);
  long onAccept(FXObject*,FXSelector,void*);
  enum{
    ID_ADD_PLOT=FXDialogBox::ID_LAST,
    ID_REMOVE_PLOT,
    ID_CLEAR_PLOT,
    ID_ADD_ALL,
    ID_SERIES_FILTER,
    ID_CHART_FILTER,
    ID_SERIES_LIST,
    ID_CHART_LIST,
    ID_LAST
    };
  ChartAssignmentDialog(FXWindow* owner,const CSVTable& source,const std::vector<FXint>& seriesColumns,FXint xaxis,const std::vector<FXint>& currentAssignment,FXint currentPlotCount);
  void getAssignment(std::vector<FXint>& result,FXint& count) const;
  };

}

#endif
