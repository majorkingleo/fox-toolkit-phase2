/********************************************************************************
*                                                                               *
*                      D r a g   a n d   D r o p   T e s t                      *
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
  Drag and drop test.

  The upper half of the window is a drop disabled label, the lower half a drop
  target: press in the upper half and the label starts a drag of a text, release
  over the lower half and the target prints what it received; see
  scripts/gate-wayland-dnd.sh.

  Both halves are plain FXLabel subclasses, because the widgets of FOX that drag
  and drop (FXTextField, FXText, the file lists) are not part of this port yet.
  The target answers SEL_DND_ENTER/MOTION with acceptDrop(DRAG_COPY) and reads
  the data in SEL_DND_DROP, which is exactly what the ported widgets will do.
*/

// The place where a drag starts
class DragSource : public FXLabel {
  FXDECLARE(DragSource)
public:
  enum { ID_SOURCE = FXLabel::ID_LAST, ID_LAST };
public:
  DragSource(){}
  DragSource(FXComposite* p,const FXString& text):FXLabel(p,text){}
  long onPress(FXObject*,FXSelector,void*);
  long onRelease(FXObject*,FXSelector,void*);
  long onDNDRequest(FXObject*,FXSelector,void*);
  };


FXDEFMAP(DragSource) DragSourceMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,DragSource::onPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,DragSource::onRelease),
  FXMAPFUNC(SEL_DND_REQUEST,0,DragSource::onDNDRequest)
  };

FXIMPLEMENT(DragSource,FXLabel,DragSourceMap,ARRAYNUMBER(DragSourceMap))


// Pressing here starts a drag of a text
long DragSource::onPress(FXObject*,FXSelector,void*){
  const FXchar* text=getenv("FOX_DRAG_TEXT");
  FXDragType types[2];
  types[0]=FXWindow::utf8Type;
  types[1]=FXWindow::textType;
  fxmessage("drag of \"%s\" started\n",text?text:"drag text");
  beginDrag(types,2);
  return 1;
  }


// The release ends the drag; the target has had the chance to take the drop
long DragSource::onRelease(FXObject*,FXSelector,void*){
  if(isDragging()){
    FXDragAction action=endDrag(true);
    fxmessage("drag ended with action %d\n",(FXint)action);
    }
  return 1;
  }


// Somebody wants the text we drag; the answer is the same as for the clipboard
long DragSource::onDNDRequest(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  const FXchar* text=getenv("FOX_DRAG_TEXT");
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){
    setDNDData(FROM_DRAGNDROP,event->target,FXString(text?text:"drag text"));
    return 1;
    }
  return 0;
  }


// The place a drag can be dropped on
class DropTarget : public FXLabel {
  FXDECLARE(DropTarget)
public:
  enum { ID_TARGET = FXLabel::ID_LAST, ID_LAST };
public:
  DropTarget(){}
  DropTarget(FXComposite* p,const FXString& text):FXLabel(p,text){ dropEnable(); }
  long onDNDEnter(FXObject*,FXSelector,void*);
  long onDNDMotion(FXObject*,FXSelector,void*);
  long onDNDDrop(FXObject*,FXSelector,void*);
  long onDNDLeave(FXObject*,FXSelector,void*);
  };


FXDEFMAP(DropTarget) DropTargetMap[]={
  FXMAPFUNC(SEL_DND_ENTER,0,DropTarget::onDNDEnter),
  FXMAPFUNC(SEL_DND_MOTION,0,DropTarget::onDNDMotion),
  FXMAPFUNC(SEL_DND_DROP,0,DropTarget::onDNDDrop),
  FXMAPFUNC(SEL_DND_LEAVE,0,DropTarget::onDNDLeave)
  };

FXIMPLEMENT(DropTarget,FXLabel,DropTargetMap,ARRAYNUMBER(DropTargetMap))


// A drag arrived
long DropTarget::onDNDEnter(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(offeredDNDType(FROM_DRAGNDROP,utf8Type) || offeredDNDType(FROM_DRAGNDROP,textType)){
    event->target=utf8Type;
    acceptDrop(DRAG_COPY);
    fxmessage("drag entered the target\n");
    return 1;
    }
  return 0;
  }


// It moved over us; keep accepting it with a copy
long DropTarget::onDNDMotion(FXObject*,FXSelector,void*){
  if(offeredDNDType(FROM_DRAGNDROP,utf8Type) || offeredDNDType(FROM_DRAGNDROP,textType)){
    acceptDrop(DRAG_COPY);
    return 1;
    }
  return 0;
  }


// It was dropped: read the text and report it
long DropTarget::onDNDDrop(FXObject*,FXSelector,void*){
  FXString text;
  acceptDrop(DRAG_COPY);
  if(getDNDData(FROM_DRAGNDROP,utf8Type,text) || getDNDData(FROM_DRAGNDROP,textType,text)){
    fxmessage("dropped \"%s\"\n",text.text());
    setText("dropped: "+text);
    return 1;
    }
  fxmessage("dropped nothing, no type could be read\n");
  return 0;
  }


// It left again
long DropTarget::onDNDLeave(FXObject*,FXSelector,void*){
  fxmessage("drag left the target\n");
  return 1;
  }


// Start the program
int main(int argc,char** argv){
  fxtestargs(argc,argv);

  FXApp application("DNDTest","FoxTest");
  application.init(argc,argv);

  FXMainWindow* main=new FXMainWindow(&application,"Drag and drop",nullptr,nullptr,DECOR_ALL,0,0,320,200);
  DragSource* source=new DragSource(main,"Drag from here");
  source->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT);
  source->setHeight(80);
  DropTarget* target=new DropTarget(main,"Drop here");
  target->setLayoutHints(LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT);
  target->setHeight(80);

  application.create();
  main->show(PLACEMENT_SCREEN);

  return application.run();
  }
