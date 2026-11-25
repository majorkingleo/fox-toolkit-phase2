#ifndef CHARTTABITEM_H
#define CHARTTABITEM_H

#include "fx.h"

namespace FXCHARTDEMO {

using namespace FX;


class ChartTabItem : public FXTabItem {
  FXDECLARE(ChartTabItem)
protected:
  FXIcon* normalIcon;
  FXIcon* hoverIcon;
  FXIcon* pressedIcon;
  FXbool iconHot;
  FXbool iconPressed;
protected:
  ChartTabItem(){ }
  FXbool overDetachIcon(FXint x,FXint y) const;
  void updateDetachIcon();
private:
  ChartTabItem(const ChartTabItem&);
  ChartTabItem& operator=(const ChartTabItem&);
public:
  long onEnter(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  ChartTabItem(FXTabBar* p,const FXString& text,FXIcon* normal,FXIcon* hover,FXIcon* pressed,FXObject* target,FXSelector selector);
    void setDetachIcons(FXIcon* normal,FXIcon* hover,FXIcon* pressed);
  };

}

#endif
