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
  FXComboBox* destination;
  std::vector<FXint> columns;
  std::vector<FXint> assignment;
  const CSVTable* table;
  FXint xcolumn;
  FXint plotCount;
  void refreshSeries();
  void updatePlotChoices();
private:
  ChartAssignmentDialog(){ }
  ChartAssignmentDialog(const ChartAssignmentDialog&);
  ChartAssignmentDialog& operator=(const ChartAssignmentDialog&);
public:
  long onAssign(FXObject*,FXSelector,void*);
  long onAddPlot(FXObject*,FXSelector,void*);
  long onRemovePlot(FXObject*,FXSelector,void*);
  long onAccept(FXObject*,FXSelector,void*);
  enum{
    ID_ASSIGN=FXDialogBox::ID_LAST,
    ID_ADD_PLOT,
    ID_REMOVE_PLOT,
    ID_LAST
    };
  ChartAssignmentDialog(FXWindow* owner,const CSVTable& source,const std::vector<FXint>& seriesColumns,FXint xaxis,const std::vector<FXint>& currentAssignment);
  void getAssignment(std::vector<FXint>& result,FXint& count) const;
  };

}

#endif
