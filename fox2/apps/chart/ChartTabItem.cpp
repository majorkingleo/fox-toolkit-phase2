#include "ChartTabItem.h"

namespace FXCHARTDEMO {

using namespace FX;


FXDEFMAP(ChartTabItem) ChartTabItemMap[]={
  FXMAPFUNC(SEL_ENTER,0,ChartTabItem::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,ChartTabItem::onLeave),
  FXMAPFUNC(SEL_UNGRABBED,0,ChartTabItem::onUngrabbed),
  FXMAPFUNC(SEL_MOTION,0,ChartTabItem::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,ChartTabItem::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,ChartTabItem::onLeftBtnRelease),
  };

FXIMPLEMENT(ChartTabItem,FXTabItem,ChartTabItemMap,ARRAYNUMBER(ChartTabItemMap))


ChartTabItem::ChartTabItem(FXTabBar* p,const FXString& text,FXIcon* normal,FXIcon* hover,FXIcon* pressed,FXObject* tgt,FXSelector selector):FXTabItem(p,text,normal,(TAB_TOP_NORMAL&~ICON_BEFORE_TEXT)|ICON_AFTER_TEXT){
  normalIcon=normal;
  hoverIcon=hover;
  pressedIcon=pressed;
  iconHot=false;
  iconPressed=false;
  setTarget(tgt);
  setSelector(selector);
  setTipText("Detach chart into a new window; close it to return to tabs");
  }


void ChartTabItem::setDetachIcons(FXIcon* normal,FXIcon* hover,FXIcon* pressed){
  normalIcon=normal;
  hoverIcon=hover;
  pressedIcon=pressed;
  updateDetachIcon();
  }


FXbool ChartTabItem::overDetachIcon(FXint x,FXint y) const {
  return x>=width-scale(22) && x<width && y>=0 && y<height;
  }


void ChartTabItem::updateDetachIcon(){
  FXIcon* nextIcon=iconPressed?pressedIcon:(iconHot?hoverIcon:normalIcon);
  if(getIcon()!=nextIcon) setIcon(nextIcon);
  }


long ChartTabItem::onEnter(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  iconHot=overDetachIcon(event->win_x,event->win_y);
  updateDetachIcon();
  return 1;
  }


long ChartTabItem::onLeave(FXObject*,FXSelector,void*){
  iconHot=false;
  updateDetachIcon();
  return 1;
  }


long ChartTabItem::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  if(iconPressed){
    iconPressed=false;
    iconHot=false;
    updateDetachIcon();
    return 1;
    }
  return FXTabItem::onUngrabbed(sender,sel,ptr);
  }


long ChartTabItem::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  iconHot=overDetachIcon(event->win_x,event->win_y);
  updateDetachIcon();
  return iconHot || iconPressed;
  }


long ChartTabItem::onLeftBtnPress(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled() && overDetachIcon(event->win_x,event->win_y)){
    grab();
    iconHot=true;
    iconPressed=true;
    updateDetachIcon();
    return 1;
    }
  return FXTabItem::onLeftBtnPress(sender,sel,ptr);
  }


long ChartTabItem::onLeftBtnRelease(FXObject* sender,FXSelector sel,void* ptr){
  if(iconPressed){
    FXEvent* event=(FXEvent*)ptr;
    FXbool click=overDetachIcon(event->win_x,event->win_y);
    ungrab();
    iconPressed=false;
    iconHot=click;
    updateDetachIcon();
    if(click && target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),nullptr);
    return 1;
    }
  return FXTabItem::onLeftBtnRelease(sender,sel,ptr);
  }

}
