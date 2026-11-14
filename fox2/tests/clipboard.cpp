/********************************************************************************
*                                                                               *
*                      C l i p b o a r d   T e s t                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
********************************************************************************/
#include "fx.h"
#include "testargs.h"

/*
  Clipboard test.

  Two buttons: one puts a text on the clipboard, the other reads whatever the
  clipboard holds and prints it.  scripts/gate-wayland-clipboard.sh uses it with
  scripts/wlclip.c as the other application, so that the copy goes to a peer
  that is not FOX and the paste comes from one.

  The text that is copied is in the environment, so a gate can match it.
*/

class ClipWindow : public FXMainWindow {
  FXDECLARE(ClipWindow)
protected:
  FXButton* copier;
  FXButton* paster;
  FXString  clipped;
public:
  enum { ID_COPY = FXMainWindow::ID_LAST, ID_PASTE, ID_LAST };
public:
  ClipWindow(){ copier=nullptr; paster=nullptr; }
  ClipWindow(FXApp* a);
  virtual void create();
  long onCopy(FXObject*,FXSelector,void*);
  long onPaste(FXObject*,FXSelector,void*);
  long onClipboardRequest(FXObject*,FXSelector,void*);
  };


FXDEFMAP(ClipWindow) ClipWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,ClipWindow::ID_COPY,ClipWindow::onCopy),
  FXMAPFUNC(SEL_COMMAND,ClipWindow::ID_PASTE,ClipWindow::onPaste),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,ClipWindow::onClipboardRequest)
  };

FXIMPLEMENT(ClipWindow,FXMainWindow,ClipWindowMap,ARRAYNUMBER(ClipWindowMap))


// Make the window
ClipWindow::ClipWindow(FXApp* a):FXMainWindow(a,"Clipboard",nullptr,nullptr,DECOR_ALL,0,0,320,120){
  copier=new FXButton(this,"&Copy",nullptr,this,ID_COPY,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,48);
  paster=new FXButton(this,"&Paste",nullptr,this,ID_PASTE,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT,0,0,0,48);
  }


// Create it
void ClipWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


// Put a text on the clipboard: the window takes the board and answers the
// request for the text when a peer asks for it
long ClipWindow::onCopy(FXObject*,FXSelector,void*){
  const FXchar* text=getenv("FOX_CLIPBOARD_TEXT");
  FXDragType types[2];
  types[0]=FXWindow::utf8Type;
  types[1]=FXWindow::textType;
  clipped=text?text:"Hello from FOX";
  if(!acquireClipboard(types,2)){
    fxmessage("copy failed\n");
    return 1;
    }
  fxmessage("copied \"%s\"\n",clipped.text());
  return 1;
  }


// Somebody wants the text we offered; the answer is the same on every platform
long ClipWindow::onClipboardRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(FXMainWindow::onClipboardRequest(sender,sel,ptr)) return 1;
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){
    setDNDData(FROM_CLIPBOARD,event->target,clipped);
    return 1;
    }
  return 0;
  }


// Read whatever the clipboard holds and report it
long ClipWindow::onPaste(FXObject*,FXSelector,void*){
  FXDragType* types=nullptr;
  FXuint numtypes=0;
  FXString body;
  FXuint i;
  if(!inquireDNDTypes(FROM_CLIPBOARD,types,numtypes)){
    fxmessage("pasted nothing, the clipboard is empty\n");
    return 1;
    }
  fxmessage("clipboard offers %u types\n",numtypes);
  for(i=0; i<numtypes; i++){
    if(getDNDData(FROM_CLIPBOARD,types[i],body)){
      fxmessage("pasted \"%s\"\n",body.text());
      break;
      }
    }
  if(i==numtypes) fxmessage("pasted nothing, none of the types could be read\n");
  freeElms(types);
  return 1;
  }


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("ClipboardTest","FoxTest");
  application.init(argc,argv);

  new ClipWindow(&application);

  application.create();

  return application.run();
  }
