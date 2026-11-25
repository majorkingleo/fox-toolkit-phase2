#include "fx.h"
#include "ChartAssignmentDialog.h"

namespace FXCHARTDEMO {


FXDEFMAP(ChartAssignmentDialog) ChartAssignmentDialogMap[]={
  FXMAPFUNC(SEL_CHANGED,ChartAssignmentDialog::ID_SERIES_FILTER,ChartAssignmentDialog::onSeriesFilter),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,ChartAssignmentDialog::ID_SERIES_LIST,ChartAssignmentDialog::onListPress),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onListPress),
  FXMAPFUNC(SEL_BEGINDRAG,ChartAssignmentDialog::ID_SERIES_LIST,ChartAssignmentDialog::onBeginDrag),
  FXMAPFUNC(SEL_BEGINDRAG,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onBeginDrag),
  FXMAPFUNC(SEL_DRAGGED,ChartAssignmentDialog::ID_SERIES_LIST,ChartAssignmentDialog::onDragged),
  FXMAPFUNC(SEL_DRAGGED,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onDragged),
  FXMAPFUNC(SEL_ENDDRAG,ChartAssignmentDialog::ID_SERIES_LIST,ChartAssignmentDialog::onEndDrag),
  FXMAPFUNC(SEL_ENDDRAG,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onEndDrag),
  FXMAPFUNC(SEL_DND_REQUEST,ChartAssignmentDialog::ID_SERIES_LIST,ChartAssignmentDialog::onDNDRequest),
  FXMAPFUNC(SEL_DND_REQUEST,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onDNDRequest),
  FXMAPFUNC(SEL_DND_ENTER,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onDNDEnter),
  FXMAPFUNC(SEL_DND_MOTION,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onDNDMotion),
  FXMAPFUNC(SEL_DND_DROP,ChartAssignmentDialog::ID_CHART_LIST,ChartAssignmentDialog::onDNDDrop),
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_ADD_PLOT,ChartAssignmentDialog::onAddPlot),
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_REMOVE_PLOT,ChartAssignmentDialog::onRemovePlot),
  FXMAPFUNC(SEL_COMMAND,ChartAssignmentDialog::ID_CLEAR_PLOT,ChartAssignmentDialog::onClearPlot),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,ChartAssignmentDialog::onAccept),
  };

FXIMPLEMENT(ChartAssignmentDialog,FXDialogBox,ChartAssignmentDialogMap,ARRAYNUMBER(ChartAssignmentDialogMap))


ChartAssignmentDialog::ChartAssignmentDialog(FXWindow* owner,const CSVTable& source,const std::vector<FXint>& seriesColumns,FXint xaxis,const std::vector<FXint>& currentAssignment):FXDialogBox(owner,"Assign Series to Charts",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,900,600,12,12,12,12,8,6),series(nullptr),seriesFilter(nullptr),chartContainer(nullptr),chartScroll(nullptr),columns(seriesColumns),assignment(currentAssignment),table(&source),seriesDragType(0),xcolumn(xaxis),plotCount(1),draggedSeries(-1){
  seriesDragType=getApp()->registerDragType("application/x-fox-chart-series");
  for(FXuval i=0; i<columns.size(); i++){
    if(columns[i]==xcolumn) continue;
    if(i<assignment.size()) plotCount=FXMAX(plotCount,assignment[i]+1);
    }
  FXVerticalFrame* body=new FXVerticalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,2,2,2,2,6,6);
  new FXLabel(body,"Drag a series onto a chart to assign it. Drag it to another chart to reassign it.",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  FXHorizontalFrame* workspace=new FXHorizontalFrame(body,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,8,0);
  FXVerticalFrame* seriesPane=new FXVerticalFrame(workspace,LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|FRAME_GROOVE,0,0,280,0,6,6,6,6,4,4);
  new FXLabel(seriesPane,"Series",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X);
  seriesFilter=new FXTextField(seriesPane,18,this,ID_SERIES_FILTER,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X);
  seriesFilter->setHelpText("Filter series by name");
  series=new FXList(seriesPane,this,ID_SERIES_LIST,LIST_SINGLESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK);
  series->setNumVisible(20);
  FXVerticalFrame* chartPane=new FXVerticalFrame(workspace,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,4,4);
  FXHorizontalFrame* chartHeader=new FXHorizontalFrame(chartPane,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0,0,0,0,0,4,0);
  new FXLabel(chartHeader,"Charts (drop series here)",nullptr,JUSTIFY_LEFT|LAYOUT_FILL_X|LAYOUT_CENTER_Y);
  new FXButton(chartHeader,"Add chart",nullptr,this,ID_ADD_PLOT,FRAME_RAISED|LAYOUT_CENTER_Y);
  chartScroll=new FXScrollWindow(chartPane,LAYOUT_FILL_X|LAYOUT_FILL_Y|VSCROLLING_ON|FRAME_SUNKEN|FRAME_THICK);
  chartContainer=new FXVerticalFrame(chartScroll,LAYOUT_FILL_X|LAYOUT_TOP,0,0,0,0,4,4,4,4,4,4);
  FXHorizontalFrame* buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0,6,0);
  new FXButton(buttons,"&Apply",nullptr,this,ID_ACCEPT,BUTTON_DEFAULT|BUTTON_INITIAL|FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  new FXButton(buttons,"&Cancel",nullptr,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  ensureChartViews();
  refreshSeries();
  }


void ChartAssignmentDialog::ensureChartViews(){
  while((FXint)chartGroups.size()>plotCount){
    chartLists.pop_back();
    chartClearButtons.pop_back();
    chartRemoveButtons.pop_back();
    delete chartGroups.back();
    chartGroups.pop_back();
    }
  while((FXint)chartGroups.size()<plotCount){
    FXint index=(FXint)chartGroups.size();
    FXGroupBox* group=new FXGroupBox(chartContainer,FXString::value("Chart %d",index+1),FRAME_GROOVE|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,160,6,6,14,6,4,4);
    FXHorizontalFrame* header=new FXHorizontalFrame(group,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0,0,0,0,0,0,0);
    new FXLabel(header,"",nullptr,LAYOUT_FILL_X);
    FXButton* clear=new FXButton(header,"Remove all",nullptr,this,ID_CLEAR_PLOT,FRAME_RAISED|LAYOUT_CENTER_Y);
    clear->setUserData((FXptr)(FXuval)(index+1));
    FXButton* remove=new FXButton(header,"X",nullptr,this,ID_REMOVE_PLOT,FRAME_RAISED|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_CENTER_Y,0,0,24,20);
    remove->setUserData((FXptr)(FXuval)(index+1));
    FXList* list=new FXList(group,this,ID_CHART_LIST,LIST_SINGLESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK);
    list->setNumVisible(6);
    list->dropEnable();
    if(chartContainer->id()) group->create();
    chartGroups.push_back(group);
    chartLists.push_back(list);
    chartClearButtons.push_back(clear);
    chartRemoveButtons.push_back(remove);
    }
  for(FXuval i=0; i<chartGroups.size(); i++){
    chartGroups[i]->setText(FXString::value("Chart %d",(FXint)i+1));
    chartClearButtons[i]->setUserData((FXptr)(FXuval)(i+1));
    chartRemoveButtons[i]->setUserData((FXptr)(FXuval)(i+1));
    if(i>0 && chartLists[i]->getNumItems()>0) chartClearButtons[i]->enable();
    else chartClearButtons[i]->disable();
    if(plotCount>1) chartRemoveButtons[i]->enable();
    else chartRemoveButtons[i]->disable();
    }
  chartContainer->recalc();
  chartContainer->layout();
  chartScroll->recalc();
  chartScroll->layout();
  }


void ChartAssignmentDialog::refreshSeries(){
  series->clearItems();
  for(FXuval p=0; p<chartLists.size(); p++) chartLists[p]->clearItems();
  if(assignment.size()<columns.size()) assignment.resize(columns.size(),0);
  FXString filter=seriesFilter->getText();
  filter.lower();
  for(FXuval i=0; i<columns.size(); i++){
    FXint column=columns[i];
    if(column==xcolumn) continue;
    FXString name=table->columns[column].name;
    FXString lowername=name;
    lowername.lower();
    if(filter.empty() || lowername.find(filter)>=0){
      series->appendItem(name,nullptr,(FXptr)(FXuval)(i+1));
      series->getItem(series->getNumItems()-1)->setDraggable(true);
      }
    FXint plot=FXCLAMP(0,assignment[i],plotCount-1);
    chartLists[plot]->appendItem(name,nullptr,(FXptr)(FXuval)(i+1));
    chartLists[plot]->getItem(chartLists[plot]->getNumItems()-1)->setDraggable(true);
    }
  series->recalc();
  series->update();
  for(FXuval p=0; p<chartLists.size(); p++){
    chartLists[p]->recalc();
    chartLists[p]->update();
    if(p>0 && chartLists[p]->getNumItems()>0) chartClearButtons[p]->enable();
    else chartClearButtons[p]->disable();
    }
  }


long ChartAssignmentDialog::onSeriesFilter(FXObject*,FXSelector,void*){
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onListPress(FXObject* sender,FXSelector,void* ptr){
  FXList* list=(FXList*)sender;
  FXEvent* event=(FXEvent*)ptr;
  FXint index=list->getItemAt(event->win_x,event->win_y);
  if(index>=0 && index<list->getNumItems()){
    list->setCurrentItem(index,false);
    if(!list->isItemSelected(index)){
      list->killSelection(false);
      list->selectItem(index,false);
      }
    list->getItem(index)->setDraggable(true);
    FXuval seriesIndex=(FXuval)list->getItemData(index);
    draggedSeries=seriesIndex>0?(FXint)seriesIndex-1:-1;
    }
  return 0;
  }


long ChartAssignmentDialog::onBeginDrag(FXObject* sender,FXSelector,void*){
  FXList* list=(FXList*)sender;
  FXint current=list->getCurrentItem();
  if(current>=0 && current<list->getNumItems()){
    FXuval index=(FXuval)list->getItemData(current);
    if(index==0 || index>columns.size()) return 0;
    draggedSeries=(FXint)index-1;
    }
  if(draggedSeries<0 || draggedSeries>=(FXint)columns.size()) return 0;
  FXDragType types[1]={seriesDragType};
  return list->beginDrag(types,1)?1:0;
  }


long ChartAssignmentDialog::onDragged(FXObject* sender,FXSelector,void* ptr){
  FXList* list=(FXList*)sender;
  FXEvent* event=(FXEvent*)ptr;
  list->handleDrag(event->root_x,event->root_y,DRAG_MOVE);
  return 1;
  }


long ChartAssignmentDialog::onEndDrag(FXObject* sender,FXSelector,void*){
  ((FXList*)sender)->endDrag(true);
  return 1;
  }


long ChartAssignmentDialog::onDNDRequest(FXObject* sender,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(event->target!=seriesDragType || draggedSeries<0) return 0;
  return ((FXWindow*)sender)->setDNDData(FROM_DRAGNDROP,seriesDragType,FXString::value("%d",draggedSeries))?1:0;
  }


long ChartAssignmentDialog::onDNDEnter(FXObject* sender,FXSelector,void* ptr){
  FXWindow* target=(FXWindow*)sender;
  if(!target->offeredDNDType(FROM_DRAGNDROP,seriesDragType)) return 0;
  ((FXEvent*)ptr)->target=seriesDragType;
  target->acceptDrop(DRAG_MOVE);
  return 1;
  }


long ChartAssignmentDialog::onDNDMotion(FXObject* sender,FXSelector,void*){
  FXWindow* target=(FXWindow*)sender;
  if(!target->offeredDNDType(FROM_DRAGNDROP,seriesDragType)) return 0;
  target->acceptDrop(DRAG_MOVE);
  return 1;
  }


long ChartAssignmentDialog::onDNDDrop(FXObject* sender,FXSelector,void*){
  FXList* target=(FXList*)sender;
  FXint destination=-1;
  for(FXuval i=0; i<chartLists.size(); i++) if(chartLists[i]==target) destination=(FXint)i;
  if(destination<0) return 0;
  target->acceptDrop(DRAG_MOVE);
  FXString text;
  if(!target->getDNDData(FROM_DRAGNDROP,seriesDragType,text)) return 0;
  FXint index=text.toInt();
  if(index<0 || index>=(FXint)assignment.size()) return 0;
  assignment[index]=destination;
  target->dropFinished(DRAG_MOVE);
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onAddPlot(FXObject*,FXSelector,void*){
  plotCount++;
  ensureChartViews();
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onClearPlot(FXObject* sender,FXSelector,void*){
  FXint cleared=(FXint)(FXuval)((FXButton*)sender)->getUserData()-1;
  if(cleared<=0 || cleared>=plotCount) return 1;
  for(FXuval i=0; i<assignment.size(); i++) if(assignment[i]==cleared) assignment[i]=0;
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onRemovePlot(FXObject* sender,FXSelector,void*){
  if(plotCount<=1) return 1;
  FXint removed=(FXint)(FXuval)((FXButton*)sender)->getUserData()-1;
  if(removed<0 || removed>=plotCount) return 1;
  for(FXuval i=0; i<assignment.size(); i++){
    if(assignment[i]==removed) assignment[i]=0;
    else if(assignment[i]>removed) assignment[i]--;
    }
  delete chartGroups[removed];
  chartGroups.erase(chartGroups.begin()+removed);
  chartLists.erase(chartLists.begin()+removed);
  chartClearButtons.erase(chartClearButtons.begin()+removed);
  chartRemoveButtons.erase(chartRemoveButtons.begin()+removed);
  plotCount--;
  ensureChartViews();
  refreshSeries();
  return 1;
  }


long ChartAssignmentDialog::onAccept(FXObject*,FXSelector,void*){
  for(FXint plot=0; plot<plotCount; plot++){
    if(chartLists[plot]->getNumItems()==0){
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
