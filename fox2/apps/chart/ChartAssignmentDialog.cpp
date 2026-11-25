#include "fx.h"
#include "ChartAssignmentDialog.h"

namespace FXCHARTDEMO {


FXDEFMAP(ChartAssignmentDialog) ChartAssignmentDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_ASSIGN,ChartAssignmentDialog::onAssign),
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_ADD_PLOT,ChartAssignmentDialog::onAddPlot),
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_REMOVE_PLOT,ChartAssignmentDialog::onRemovePlot),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,ChartAssignmentDialog::onAccept),
  };

FXIMPLEMENT(ChartAssignmentDialog,FXDialogBox,ChartAssignmentDialogMap,ARRAYNUMBER(ChartAssignmentDialogMap))


ChartAssignmentDialog::ChartAssignmentDialog(FXWindow* owner,const CSVTable& source,const std::vector<FXint>& seriesColumns,FXint xaxis,const std::vector<FXint>& currentAssignment):FXDialogBox(owner,"Assign Series to Charts",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,620,440,12,12,12,12,8,6),columns(seriesColumns),assignment(currentAssignment),table(&source),xcolumn(xaxis),plotCount(1){
  for(FXuval i=0; i<columns.size(); i++){
    if(columns[i]==xcolumn) continue;
    if(i<assignment.size()) plotCount=FXMAX(plotCount,assignment[i]+1);
    }
  FXVerticalFrame* body=new FXVerticalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,2,2,2,2,6,6);
  new FXLabel(body,"Select one or more series, choose a chart, then assign them.",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  FXHorizontalFrame* targetrow=new FXHorizontalFrame(body,LAYOUT_FILL_X|LAYOUT_SIDE_TOP,0,0,0,0,0,0,0,0,6,0);
  new FXLabel(targetrow,"Assign to:",nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  destination=new FXComboBox(targetrow,16,nullptr,0,COMBOBOX_STATIC|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_CENTER_Y);
  FXHorizontalFrame* plotbuttons=new FXHorizontalFrame(targetrow,LAYOUT_SIDE_RIGHT|LAYOUT_CENTER_Y,0,0,0,0,0,0,0,0,4,0);
  new FXButton(plotbuttons,"Remove subplot",nullptr,this,ID_REMOVE_PLOT,FRAME_RAISED|LAYOUT_CENTER_Y);
  new FXButton(plotbuttons,"Add subplot",nullptr,this,ID_ADD_PLOT,FRAME_RAISED|LAYOUT_CENTER_Y);
  series=new FXList(body,nullptr,0,LIST_EXTENDEDSELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK);
  series->setNumVisible(18);
  FXHorizontalFrame* buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0,6,0);
  new FXButton(buttons,"&Assign",nullptr,this,ID_ASSIGN,FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  new FXButton(buttons,"&Apply",nullptr,this,ID_ACCEPT,BUTTON_DEFAULT|BUTTON_INITIAL|FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  new FXButton(buttons,"&Cancel",nullptr,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  updatePlotChoices();
  refreshSeries();
  }


void ChartAssignmentDialog::updatePlotChoices(){
  FXint current=destination->getCurrentItem();
  destination->clearItems();
  for(FXint i=0; i<plotCount; i++) destination->appendItem(FXString::value("Chart %d",i+1));
  destination->setCurrentItem(FXCLAMP(0,current,plotCount-1));
  }


void ChartAssignmentDialog::refreshSeries(){
  series->clearItems();
  for(FXuval i=0; i<columns.size(); i++){
    FXint column=columns[i];
    if(column==xcolumn) continue;
    FXint plot=(i<assignment.size())?FXCLAMP(0,assignment[i],plotCount-1):0;
    series->appendItem(table->columns[column].name+FXString::value("    [Chart %d]",plot+1),nullptr,(FXptr)(FXuval)(i+1));
    }
  }


long ChartAssignmentDialog::onAssign(FXObject*,FXSelector,void*){
  FXint plot=destination->getCurrentItem();
  if(plot<0 || plot>=plotCount) return 1;
  if(assignment.size()<columns.size()) assignment.resize(columns.size(),0);
  for(FXint i=0; i<series->getNumItems(); i++){
    if(!series->isItemSelected(i)) continue;
    FXuval index=(FXuval)series->getItemData(i);
    if(index>0 && index<=assignment.size()) assignment[index-1]=plot;
    }
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onAddPlot(FXObject*,FXSelector,void*){
  plotCount++;
  updatePlotChoices();
  destination->setCurrentItem(plotCount-1);
  return 1;
  }


long ChartAssignmentDialog::onRemovePlot(FXObject*,FXSelector,void*){
  if(plotCount<=1) return 1;
  FXint removed=plotCount-1;
  for(FXuval i=0; i<assignment.size(); i++) if(assignment[i]>=removed) assignment[i]=0;
  plotCount--;
  updatePlotChoices();
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onAccept(FXObject*,FXSelector,void*){
  for(FXint plot=0; plot<plotCount; plot++){
    FXbool found=false;
    for(FXuval i=0; i<columns.size(); i++){
      if(columns[i]!=xcolumn && i<assignment.size() && assignment[i]==plot){ found=true; break; }
      }
    if(!found){
      FXMessageBox::error(this,MBOX_OK,"Chart Assignment","Assign at least one series to each chart, or remove the empty subplot.");
      return 1;
      }
    }
  return FXDialogBox::onCmdAccept(this,FXSEL(SEL_COMMAND,ID_ACCEPT),nullptr);
  }


void ChartAssignmentDialog::getAssignment(std::vector<FXint>& result,FXint& count) const {
  result=assignment;
  if(result.size()<columns.size()) result.resize(columns.size(),0);
  count=plotCount;
  }

}
