/********************************************************************************
*                                                                               *
*                        T e x t   a n d   S p i n n e r                        *
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
  Stage 3 smoke test of the calculator port: create the text widgets and the
  spinner the preferences dialog uses, let the application create them, and
  check that the geometry, the content and the spinner range behave.
*/

int main(int argc,char** argv){
  fxtestargs(argc,argv);

  printf("text and spinner test\n");

  FXApp application("TextSpin","FoxTest");
  application.init(argc,argv);

  FXMainWindow main(&application,"TextSpin",nullptr,nullptr,DECOR_ALL);
  FXVerticalFrame frame(&main,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  FXText *text=new FXText(&frame,nullptr,0,TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  text->setVisibleRows(20);
  text->setVisibleColumns(60);
  text->setText("one\ttwo\nthree");

  FXTextField *field=new FXTextField(&frame,20,nullptr,0,TEXTFIELD_NORMAL|LAYOUT_FILL_X);
  field->setText("calculator");

  FXSpinner *spinner=new FXSpinner(&frame,2,nullptr,0,JUSTIFY_RIGHT);
  spinner->setRange(1,30);
  spinner->setValue(7);

  application.create();
  main.show();

  int errors=0;

  // The content of the text widgets survives the round trip
  if(text->getText()!="one\ttwo\nthree"){
    printf("text content is \"%s\"\n",text->getText().text());
    errors++;
    }
  if(field->getText()!="calculator"){
    printf("field content is \"%s\"\n",field->getText().text());
    errors++;
    }

  // Visible rows and columns come back as requested
  if(text->getVisibleRows()!=20){
    printf("visible rows %d\n",text->getVisibleRows());
    errors++;
    }
  if(text->getVisibleColumns()!=60){
    printf("visible columns %d\n",text->getVisibleColumns());
    errors++;
    }

  // The widget got a geometry and a content size after layout
  printf("text %dx%d, content %dx%d\n",text->getWidth(),text->getHeight(),text->getContentWidth(),text->getContentHeight());
  if(text->getWidth()<=0 || text->getHeight()<=0){
    printf("text has no size\n");
    errors++;
    }
  if(text->getContentWidth()<=0 || text->getContentHeight()<=0){
    printf("text has no content size\n");
    errors++;
    }

  // The spinner clamps to its range
  if(spinner->getValue()!=7){
    printf("spinner value %d\n",spinner->getValue());
    errors++;
    }
  spinner->setValue(99);
  if(spinner->getValue()!=30){
    printf("spinner high clamp %d\n",spinner->getValue());
    errors++;
    }
  spinner->setValue(0);
  if(spinner->getValue()!=1){
    printf("spinner low clamp %d\n",spinner->getValue());
    errors++;
    }

  // The text field is an editable single line widget with a cursor
  field->setCursorPos(field->getText().length());
  if(field->getCursorPos()!=10){
    printf("field cursor %d\n",field->getCursorPos());
    errors++;
    }

  if(errors){
    printf("%d errors\n",errors);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }
