#include "HistoryWindow.h"

StopwatchHistoryWindow::StopwatchHistoryWindow(FXWindow* owner,const History& data,FXObject* target,FXSelector selector):FXDialogBox(owner,"Stopwatch History",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE),history(&data),table(nullptr),sized(false){
  FXVerticalFrame* contents=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE,0,0,0,0,0,0,0,0);
  table=new FXTable(contents,nullptr,0,TABLE_READONLY|TABLE_COL_SIZABLE|TABLE_NO_COLSELECT|TABLE_NO_ROWSELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  table->setVisibleRows(1);
  table->setVisibleColumns(2);
  table->setRowRenumbering(FXHeader::decimalNumbering);
  table->getCornerButton()->hide();
  FXHorizontalFrame* buttons=new FXHorizontalFrame(contents,LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,2,2,2,2);
  new FXButton(buttons,"Clear history",nullptr,target,selector,BUTTON_NORMAL|LAYOUT_FILL_X);
  new FXButton(buttons,"Close",nullptr,this,FXSEL(SEL_COMMAND,FXDialogBox::ID_CANCEL),BUTTON_NORMAL|LAYOUT_FILL_X);
  refresh();
  }


void StopwatchHistoryWindow::showHistory(){
  refresh();
  if(!sized){
    resize(getDefaultWidth(),getDefaultHeight());
    sized=true;
    }
  if(shown()) raise();
  else show(PLACEMENT_OWNER);
  }


void StopwatchHistoryWindow::refresh(){
  FXint row;
  FXint padding;
  FXint widths[2];
  FXFont* cellFont;
  FXFont* headerFont;
  table->setTableSize(history->size(),2);
  table->setColumnText(0,"Stopped");
  table->setColumnText(1,"Elapsed");
  table->setVisibleRows(FXMAX(1,FXMIN(history->size(),10)));
  cellFont=table->getFont();
  headerFont=table->getColumnHeader()->getFont();
  padding=table->getMarginLeft()+table->getMarginRight()+16;
  widths[0]=headerFont->getTextWidth("Stopped")+padding;
  widths[1]=headerFont->getTextWidth("Elapsed")+padding;
  for(row=0;row<history->size();row++){
    const History::Entry& entry=history->at(row);
    FXString stopped=History::formatWhen(entry.when);
    FXString elapsed=History::formatElapsed(entry.elapsed);
    table->setItemText(row,0,stopped);
    table->setItemText(row,1,elapsed);
    widths[0]=FXMAX(widths[0],cellFont->getTextWidth(stopped)+padding);
    widths[1]=FXMAX(widths[1],cellFont->getTextWidth(elapsed)+padding);
    }
  table->setColumnWidth(0,widths[0]);
  table->setColumnWidth(1,widths[1]);
  table->setDefColumnWidth((widths[0]+widths[1]+1)/2);
  }
