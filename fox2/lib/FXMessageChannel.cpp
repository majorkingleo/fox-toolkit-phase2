/********************************************************************************
*                                                                               *
*         I n t e r - T h r e a d    M e s s a g i n g    C h a n n e l         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXException.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXMutex.h"
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
#include "FXMessageChannel.h"

#if defined(WIN32)
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Maximum payload size in bytes
static const FXint MAXMESSAGE=8192;


// Message header passed through the pipe
struct FXMessage {
  FXObject* target;
  FXSelector message;
  FXint size;
  };


// Message data buffer, aligned for arbitrary payloads
struct FXDataMessage {
  FXMessage message;
  FXlong data[MAXMESSAGE/sizeof(FXlong)];
  };


#if defined(WIN32)

// Read exactly size bytes from a pipe handle
static FXbool readAll(FXInputHandle handle,void* data,FXuint size){
  FXuchar* bytes=(FXuchar*)data;
  while(size){
    DWORD count=0;
    if(!::ReadFile(handle,bytes,size,&count,nullptr) || count==0) return false;
    bytes+=count;
    size-=count;
    }
  return true;
  }


// Write exactly size bytes to a pipe handle
static FXbool writeAll(FXInputHandle handle,const void* data,FXuint size){
  const FXuchar* bytes=(const FXuchar*)data;
  while(size){
    DWORD count=0;
    if(!::WriteFile(handle,bytes,size,&count,nullptr) || count==0) return false;
    bytes+=count;
    size-=count;
    }
  return true;
  }

#else

// Read exactly size bytes from a pipe descriptor
static FXbool readAll(FXInputHandle handle,void* data,FXuint size){
  FXuchar* bytes=(FXuchar*)data;
  while(size){
    ssize_t count=::read(handle,bytes,size);
    if(count<0 && errno==EINTR) continue;
    if(count<=0) return false;
    bytes+=count;
    size-=(FXuint)count;
    }
  return true;
  }


// Write exactly size bytes to a pipe descriptor
static FXbool writeAll(FXInputHandle handle,const void* data,FXuint size){
  const FXuchar* bytes=(const FXuchar*)data;
  while(size){
    ssize_t count=::write(handle,bytes,size);
    if(count<0 && errno==EINTR) continue;
    if(count<=0) return false;
    bytes+=count;
    size-=(FXuint)count;
    }
  return true;
  }

#endif


// Message map
FXDEFMAP(FXMessageChannel) FXMessageChannelMap[]={
  FXMAPFUNC(SEL_IO_READ,FXMessageChannel::ID_IO_READ,FXMessageChannel::onMessage)
  };


// Object implementation
FXIMPLEMENT(FXMessageChannel,FXObject,FXMessageChannelMap,ARRAYNUMBER(FXMessageChannelMap))


// Construct uninitialized channel
FXMessageChannel::FXMessageChannel():app(nullptr){
  h[0]=h[1]=h[2]=BadHandle;
  }


// Create the message pipe and register its read end with the application
FXMessageChannel::FXMessageChannel(FXApp* application):app(application){
  h[0]=h[1]=h[2]=BadHandle;
  if(!app) throw FXResourceException("unable to create message channel without an application.");
#if defined(WIN32)
  h[2]=::CreateSemaphore(nullptr,0,2147483647,nullptr);
  if(h[2]==nullptr) throw FXResourceException("unable to create message semaphore.");
  if(!::CreatePipe(&h[0],&h[1],nullptr,0)){
    ::CloseHandle(h[2]);
    h[2]=BadHandle;
    throw FXResourceException("unable to create message pipe.");
    }
#else
  if(::pipe(h)!=0) throw FXResourceException("unable to create message pipe.");
  ::fcntl(h[0],F_SETFD,FD_CLOEXEC);
  ::fcntl(h[1],F_SETFD,FD_CLOEXEC);
#endif
  if(!app->addInput(this,ID_IO_READ,
#if defined(WIN32)
      h[2],
#else
      h[0],
#endif
      INPUT_READ,nullptr)){
#if defined(WIN32)
    ::CloseHandle(h[0]);
    ::CloseHandle(h[1]);
    ::CloseHandle(h[2]);
#else
    ::close(h[0]);
    ::close(h[1]);
#endif
    h[0]=h[1]=h[2]=BadHandle;
    throw FXResourceException("unable to register message channel with application.");
    }
  }


// Dispatch one message from the worker-thread pipe
long FXMessageChannel::onMessage(FXObject*,FXSelector,void*){
  FXDataMessage packet;
#if defined(WIN32)
  if(!readAll(h[0],&packet.message,sizeof(FXMessage))) return 0;
#else
  if(!readAll(h[0],&packet.message,sizeof(FXMessage))) return 0;
#endif
  if(packet.message.size<0 || packet.message.size>MAXMESSAGE) return 0;
  if(packet.message.size>0 && !readAll(h[0],packet.data,(FXuint)packet.message.size)) return 0;
  return packet.message.target ? packet.message.target->tryHandle(this,packet.message.message,
    packet.message.size>0 ? packet.data : nullptr) : 0;
  }


// Send a message and optional payload to an object in the application thread
FXbool FXMessageChannel::message(FXObject* target,FXSelector msg,const void* data,FXint size){
  if(!target || size<0 || (size>0 && !data)) return false;
  FXMessage packet;
  packet.target=target;
  packet.message=msg;
  packet.size=FXMIN(size,MAXMESSAGE);
  FXScopedMutex locker(m);
  if(!writeAll(h[1],&packet,sizeof(FXMessage))) return false;
  if(packet.size>0 && !writeAll(h[1],data,(FXuint)packet.size)) return false;
#if defined(WIN32)
  return ::ReleaseSemaphore(h[2],1,nullptr)!=0;
#else
  return true;
#endif
  }


// Unregister and close the channel
FXMessageChannel::~FXMessageChannel(){
  if(!app) return;
#if defined(WIN32)
  if(h[2]!=BadHandle) app->removeInput(h[2],INPUT_READ);
  if(h[0]!=BadHandle) ::CloseHandle(h[0]);
  if(h[1]!=BadHandle) ::CloseHandle(h[1]);
  if(h[2]!=BadHandle) ::CloseHandle(h[2]);
#else
  if(h[0]!=BadHandle) app->removeInput(h[0],INPUT_READ);
  if(h[0]!=BadHandle) ::close(h[0]);
  if(h[1]!=BadHandle) ::close(h[1]);
#endif
  app=nullptr;
  }

}
