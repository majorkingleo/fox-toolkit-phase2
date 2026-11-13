/********************************************************************************
*                                                                               *
*                       Platform helpers   ( W i n 3 2 )                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxpriv.h"

#if defined(WIN32)

/*
  Notes:

  - Windows data transfer for clipboard, selection and drag and drop: shared memory helpers and the FXApp transfer members.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Send data via shared memory
HANDLE fxsenddata(HWND window,FXuchar* data,FXuint size){
  HANDLE hMap=0,hMapCopy=0;
  FXuchar *ptr;
  DWORD processid;
  HANDLE process;

  if(data && size){
    hMap=CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,size+sizeof(FXuint),"_FOX_DDE");
    if(hMap){
      ptr=(FXuchar*)MapViewOfFile((HANDLE)hMap,FILE_MAP_WRITE,0,0,size+sizeof(FXuint));
      if(ptr){
        *((FXuint*)ptr)=size;
        memcpy(ptr+sizeof(FXuint),data,size);
        UnmapViewOfFile(ptr);
        }
      GetWindowThreadProcessId((HWND)window,&processid);
      process=OpenProcess(PROCESS_DUP_HANDLE,true,processid);
      DuplicateHandle(GetCurrentProcess(),hMap,process,&hMapCopy,FILE_MAP_ALL_ACCESS,true,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
      CloseHandle(process);
      }
    return hMapCopy;
    }
  return 0;
  }


// Receive data via shared memory
HANDLE fxrecvdata(HANDLE hMap,FXuchar*& data,FXuint& size){
  FXuchar *ptr;
  data=nullptr;
  size=0;
  if(hMap){
    ptr=(FXuchar*)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
    if(ptr){
      size=*((FXuint*)ptr);
      if(allocElms(data,size)){
        memcpy(data,ptr+sizeof(FXuint),size);
        }
      UnmapViewOfFile(ptr);
      }
    CloseHandle(hMap);
    return hMap;
    }
  return 0;
  }


// Send request for data
HANDLE fxsendrequest(HWND window,HWND requestor,WPARAM type){
  FXuint loops=100;
  MSG msg;
  PostMessage((HWND)window,WM_DND_REQUEST,type,(LPARAM)requestor);
  while(!PeekMessage(&msg,nullptr,WM_DND_REPLY,WM_DND_REPLY,PM_REMOVE)){
    if(loops==0){ fxwarning("timed out\n"); return 0; }
    FXThread::sleep(10000000);  // Don't burn too much CPU here:- the other guy needs it more....
    loops--;
    }
  return (HANDLE)msg.wParam;
  }


/*******************************************************************************/


// Change PRIMARY selection data
void FXApp::selectionSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve PRIMARY selection data
void FXApp::selectionGetData(const FXWindow*,FXDragType type,FXuchar*& data,FXuint& size){
  data=nullptr;
  size=0;
  if(selectionWindow){
    event.type=SEL_SELECTION_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    selectionWindow->handle(this,FXSEL(SEL_SELECTION_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  }


// Retrieve PRIMARY selection types
void FXApp::selectionGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  types=nullptr;
  numtypes=0;
  if(selectionWindow){
    dupElms(types,xselTypeList,xselNumTypes);
    numtypes=xselNumTypes;
    }
  }

/*******************************************************************************/


// Change CLIPBOARD selection data
void FXApp::clipboardSetData(const FXWindow*,FXDragType type,FXuchar* data,FXuint size){
  HGLOBAL hGlobalMemory=GlobalAlloc(GMEM_MOVEABLE,size);
  if(hGlobalMemory){
    void *pGlobalMemory=GlobalLock(hGlobalMemory);
    FXASSERT(pGlobalMemory);
    memcpy((FXchar*)pGlobalMemory,data,size);
    GlobalUnlock(hGlobalMemory);
    SetClipboardData(type,hGlobalMemory);
    freeElms(data);
    }
  }


// Retrieve CLIPBOARD selection data
void FXApp::clipboardGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  data=nullptr;
  size=0;
  if(IsClipboardFormatAvailable(type)){
    if(OpenClipboard((HWND)window->id())){
      HANDLE hClipMemory=GetClipboardData(type);
      if(hClipMemory){
        size=(FXuint)GlobalSize(hClipMemory);
        if(allocElms(data,size)){
          void *pClipMemory=GlobalLock(hClipMemory);
          FXASSERT(pClipMemory);
          memcpy((void*)data,pClipMemory,size);
          GlobalUnlock(hClipMemory);
          CloseClipboard();
          }
        }
      }
    }
  }


// Retrieve CLIPBOARD selection types
void FXApp::clipboardGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes){
  FXuint count;
  types=nullptr;
  numtypes=0;
  if(OpenClipboard((HWND)window->id())){
    count=CountClipboardFormats();
    if(count){
      allocElms(types,count);
      UINT format=0;
      while(numtypes<count && (format=EnumClipboardFormats(format))!=0){
        types[numtypes++]=format;
        }
      }
    CloseClipboard();
    }
  }

/*******************************************************************************/


// Change DND selection data
void FXApp::dragdropSetData(const FXWindow*,FXDragType,FXuchar* data,FXuint size){
  freeElms(ddeData);
  ddeData=data;
  ddeSize=size;
  }


// Retrieve DND selection data
void FXApp::dragdropGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size){
  HANDLE answer;
  data=nullptr;
  size=0;
  if(dragWindow){
    event.type=SEL_DND_REQUEST;
    event.target=type;
    ddeData=nullptr;
    ddeSize=0;
    dragWindow->handle(this,FXSEL(SEL_DND_REQUEST,0),&event);
    data=ddeData;
    size=ddeSize;
    ddeData=nullptr;
    ddeSize=0;
    }
  else{
    answer=fxsendrequest((HWND)xdndSource,(HWND)window->id(),(WPARAM)type);
    fxrecvdata(answer,data,size);
    }
  }


// Retrieve DND selection types
void FXApp::dragdropGetTypes(const FXWindow*,FXDragType*& types,FXuint& numtypes){
  dupElms(types,ddeTypeList,ddeNumTypes);
  numtypes=ddeNumTypes;
  }


/*******************************************************************************/

}

#endif
