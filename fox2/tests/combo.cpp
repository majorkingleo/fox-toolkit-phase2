/********************************************************************************
*                                                                               *
*                        L i s t   a n d   C o m b o   T e s t                  *
*                                                                               *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>

/*
  Stage 4 smoke test of the calculator port: create the list, the combo box
  and the menu button, then open and close the popups they own.
*/

int main(int argc,char** argv){
  fxtestargs(argc,argv);

  printf("list and combo test\n");

  FXApp application("ListCombo","FoxTest");
  application.init(argc,argv);

  // The main window, and every widget below it, deletes itself when the window
  // is closed, so the whole tree lives on the heap
  FXMainWindow *main=new FXMainWindow(&application,"ListCombo",nullptr,nullptr,DECOR_ALL);
  FXVerticalFrame *frame=new FXVerticalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  FXList *list=new FXList(frame,nullptr,0,LIST_SINGLESELECT|LIST_BROWSESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  list->appendItem("alpha");
  list->appendItem("beta");

  FXComboBox *combo=new FXComboBox(frame,10,nullptr,0,COMBOBOX_NORMAL|LAYOUT_FILL_X);
  combo->appendItem("alpha");
  combo->appendItem("beta");
  combo->appendItem("gamma");

  FXMenuPane *pane=new FXMenuPane(frame);
  new FXMenuCommand(pane,"One");
  new FXMenuCommand(pane,"Two");
  FXMenuButton *button=new FXMenuButton(frame,"Menu",nullptr,pane,FRAME_RAISED|FRAME_THICK|MENUBUTTON_DOWN|MENUBUTTON_ATTACH_LEFT,0,0,0,0);

  application.create();
  main->show();

  int errors=0;

  // The list holds the items in order
  if(list->getNumItems()!=2){
    printf("list items %d\n",list->getNumItems());
    errors++;
    }
  if(list->getItemText(0)!="alpha" || list->getItemText(1)!="beta"){
    printf("list text %s, %s\n",list->getItemText(0).text(),list->getItemText(1).text());
    errors++;
    }
  list->removeItem(0);
  if(list->getNumItems()!=1 || list->getItemText(0)!="beta"){
    printf("list after remove %d, %s\n",list->getNumItems(),list->getItemText(0).text());
    errors++;
    }

  // The combo box holds its items and reflects the selected one in the field
  if(combo->getNumItems()!=3){
    printf("combo items %d\n",combo->getNumItems());
    errors++;
    }
  combo->setCurrentItem(1);
  if(combo->getCurrentItem()!=1 || combo->getText()!="beta"){
    printf("combo current %d, text %s\n",combo->getCurrentItem(),combo->getText().text());
    errors++;
    }

  // Open and close the combo popup
  combo->showMenu(true);
  if(!combo->isMenuShown()){
    printf("combo menu did not show\n");
    errors++;
    }
  combo->showMenu(false);
  if(combo->isMenuShown()){
    printf("combo menu did not hide\n");
    errors++;
    }

  // The menu button owns the pane and shows it on request
  if(button->getMenu()!=pane){
    printf("menu button has the wrong pane\n");
    errors++;
    }
  button->showMenu(true);
  if(!button->isMenuShown()){
    printf("menu button menu did not show\n");
    errors++;
    }
  button->showMenu(false);
  if(button->isMenuShown()){
    printf("menu button menu did not hide\n");
    errors++;
    }

  if(errors){
    printf("%d errors\n",errors);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }
