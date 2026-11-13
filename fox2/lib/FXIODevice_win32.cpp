/********************************************************************************
*                                                                               *
*                I / O   D e v i c e   C l a s s   ( W i n 3 2 )                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "fxiopriv.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXIODevice members which operate on the
    native handle; the portable members stay in FXIODevice.cpp.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {


// Grab access attributes from the open handle.
// There doesn't seem to be a way to do that on Windows except
// through the use of the undocumented NtQueryObject() function.
// At the first call, dig up the function pointer and keep it
// for the next time.

// Declare NtQueryObject()
typedef NTSTATUS (WINAPI *PFN_NTQUERYOBJECT)(HANDLE,OBJECT_INFORMATION_CLASS,void*,ULONG,ULONG*);

// Declare the stub function
static NTSTATUS WINAPI StubNtQueryObject(HANDLE,OBJECT_INFORMATION_CLASS,void*,ULONG,ULONG*);

// Pointer to (stub for) NtQueryObject
static PFN_NTQUERYOBJECT fxNtQueryObject=StubNtQueryObject;

// The stub gets the address of actual function, sets the function pointer, then calls
// actual function; next time around actual function will be called directly.
static NTSTATUS WINAPI StubNtQueryObject(HANDLE hnd,OBJECT_INFORMATION_CLASS oic,void* oi,ULONG oilen,ULONG* len){
  if(fxNtQueryObject==StubNtQueryObject){
    HMODULE ntdllDll=GetModuleHandleA("ntdll.dll");
    FXASSERT(ntdllDll);
    fxNtQueryObject=(PFN_NTQUERYOBJECT)GetProcAddress(ntdllDll,"NtQueryObject");
    FXASSERT(fxNtQueryObject);
    }
  return fxNtQueryObject(hnd,oic,oi,oilen,len);
  }


// Return access mode
FXuint FXIODevice::mode() const {
  if(device!=BadHandle){
    FXuint result=FXIO::NoAccess;
    DWORD flags=0;
    if(GetHandleInformation(device,&flags)!=0){
      result=FXIO::ReadWrite;
      if(flags&HANDLE_FLAG_INHERIT){ result|=FXIO::Inheritable; }
/*
      PUBLIC_OBJECT_BASIC_INFORMATION obi;
      if(fxNtQueryObject(device,ObjectBasicInformation,&obi,sizeof(obi),nullptr)>=0){
        //FXTRACE(TOPIC_DETAIL,"obi.Attributes     = %08x\n",obi.Attributes);
        //FXTRACE(TOPIC_DETAIL,"obi.GrantedAccess  = %08x\n",obi.GrantedAccess);
        //FXTRACE(TOPIC_DETAIL,"obi.GrantedAccess  = %08x\n",obi.GrantedAccess);
        //FXTRACE(TOPIC_DETAIL,"obi.HandleCount    = %08x\n",obi.HandleCount);
        //FXTRACE(TOPIC_DETAIL,"obi.PointerCount   = %08x\n",obi.PointerCount);
        if(obi.GrantedAccess&GENERIC_READ){ result|=FXIO::ReadOnly; }
        if(obi.GrantedAccess&GENERIC_WRITE){ result|=FXIO::WriteOnly; }
        if(obi.GrantedAccess&GENERIC_ALL){ result|=FXIO::ReadWrite; }
        if(obi.GrantedAccess&GENERIC_EXECUTE){ result|=FXIO::Executable; }
        }
      FXTRACE(TOPIC_DETAIL,"result             = %08x\n",result);
*/
      return result;
      }
    }
  return FXIO::Error;
  }


// Change access mode of open device
FXbool FXIODevice::mode(FXuint m){
  if(device!=BadHandle){
    DWORD flags=0;
    if(m&FXIO::Inheritable) flags=HANDLE_FLAG_INHERIT;
    if(::SetHandleInformation(device,HANDLE_FLAG_INHERIT,flags)){
      return true;
      }
    }
  return false;
  }


// Return permissions
FXuint FXIODevice::perms() const {
  if(device!=BadHandle){
    BY_HANDLE_FILE_INFORMATION data;
    if(::GetFileInformationByHandle(device,&data)){
      FXuint result=FXIO::AllFull;
      DWORD bits=::GetFileType(device);
      if(bits&FILE_TYPE_CHAR){ result|=FXIO::Character; }
      if(bits&FILE_TYPE_DISK){ result|=FXIO::Block; }
      if(bits&FILE_TYPE_PIPE){ result|=FXIO::Fifo; }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
        result|=FXIO::Directory;
        }
      else{
        result|=FXIO::File;
        }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
        result|=FXIO::Hidden;
        }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
        result&=~FXIO::AllWrite;
        }
      if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
        result|=FXIO::Device;
        }
      return result;
      }
    }
  return FXIO::Error;
  }


// Set permissions
FXbool FXIODevice::perms(FXuint p){
  if(device!=BadHandle){
/*
    BY_HANDLE_FILE_INFORMATION data;
    if(::GetFileInformationByHandle(device,&data)){
      FILE_BASIC_INFO info;
      info.CreationTime.LowPart=data.ftCreationTime.dwLowDateTime;
      info.CreationTime.HighPart=data.ftCreationTime.dwHighDateTime;
      info.LastAccessTime.LowPart=data.ftLastAccessTime.dwLowDateTime;
      info.LastAccessTime.HighPart=data.ftLastAccessTime.dwHighDateTime;
      info.LastWriteTime.LowPart=data.ftLastWriteTime.dwLowDateTime;
      info.LastWriteTime.HighPart=data.ftLastWriteTime.dwHighDateTime;
      info.ChangeTime.LowPart=data.ftLastWriteTime.dwLowDateTime;       // ??
      info.ChangeTime.HighPart=data.ftLastWriteTime.dwHighDateTime;
      info.FileAttributes=data.dwFileAttributes;        // FIXME with changes
      if(p&FXIO::Hidden) info.FileAttributes|=FILE_ATTRIBUTE_HIDDEN; else info.FileAttributes&=~FILE_ATTRIBUTE_HIDDEN;
      if(!(p&FXIO::AllWrite)) info.FileAttributes|=FILE_ATTRIBUTE_READONLY; else info.FileAttributes&=~FILE_ATTRIBUTE_READONLY;
      return ::SetFileInformationByHandle(device,FileBasicInfo,&info,sizeof(info))!=0;
      }
*/
    }
  return false;
  }


// Return true if handle is valid
FXbool FXIODevice::valid(FXInputHandle hnd){
  if(hnd!=BadHandle){
    DWORD flags;
    return ::GetHandleInformation(hnd,&flags)!=0;
    }
  return false;
  }


// Read block
FXival FXIODevice::readBlock(void* ptr,FXival count){
  if(device!=BadHandle){
    FXuchar* pointer=(FXuchar*)ptr;
    FXival nread=0;
    DWORD nr,ct,er;
    while(0<count){
      ct=(DWORD)FXMIN(count,1073741824);
      if(::ReadFile(device,pointer,ct,&nr,nullptr)==0){
        er=GetLastError();
        if(er==ERROR_HANDLE_EOF) return nread;
        if(er==ERROR_IO_PENDING) return FXIO::Again;
        if(er==ERROR_BROKEN_PIPE) return FXIO::Broken;
        return FXIO::Error;
        }
      if(nr==0) return nread;
      pointer+=nr;
      nread+=nr;
      count-=nr;
      }
    return nread;
    }
  return FXIO::Error;
  }


// Write block
FXival FXIODevice::writeBlock(const void* ptr,FXival count){
  if(device!=BadHandle){
    const FXuchar* pointer=(const FXuchar*)ptr;
    FXival nwritten=0;
    DWORD nw,ct,er;
    while(0<count){
      ct=(DWORD)FXMIN(count,1073741824);
      if(::WriteFile(device,pointer,ct,&nw,nullptr)==0){
        er=GetLastError();
        if(er==ERROR_HANDLE_EOF) return nwritten;
        if(er==ERROR_IO_PENDING) return FXIO::Again;
        if(er==ERROR_BROKEN_PIPE) return FXIO::Broken;
        return FXIO::Error;
        }
      if(nw==0) return nwritten;
      pointer+=nw;
      nwritten+=nw;
      count-=nw;
      }
    return nwritten;
    }
  return FXIO::Error;
  }


// Close file
FXbool FXIODevice::close(){
  if(device!=BadHandle){
    if(::CloseHandle(device)!=0){
      device=BadHandle;
      return true;
      }
    }
  return false;
  }


}

#endif
