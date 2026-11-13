/********************************************************************************
*                                                                               *
*                      M a p p e d   F i l e   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2023,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "FXString.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXMappedFile.h"
#include "fxiopriv.h"

#if defined(WIN32)

/*
  Notes:
  - Platform implementation of the FXMappedFile members; the portable parts stay in FXMappedFile.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Open file and map it
FXptr FXMappedFile::open(const FXString& filename,FXuint m,FXuint perm,FXlong len,FXlong off){
  if(device==BadHandle){

    // Basic access mode
    DWORD access=0;
    switch(m&(FXIO::ReadOnly|FXIO::WriteOnly)){
      case FXIO::ReadOnly: access=GENERIC_READ; break;
      case FXIO::WriteOnly: access=GENERIC_WRITE; break;
      case FXIO::ReadWrite: access=GENERIC_READ|GENERIC_WRITE; break;
      }

    // Creation and truncation mode
    DWORD creation=OPEN_EXISTING;
    switch(m&(FXIO::Create|FXIO::Truncate|FXIO::Exclusive)){
      case FXIO::Create: creation=OPEN_ALWAYS; break;
      case FXIO::Exclusive: creation=CREATE_NEW; break;
      case FXIO::Truncate: creation=TRUNCATE_EXISTING; break;
      case FXIO::Create|FXIO::Truncate: creation=CREATE_ALWAYS; break;
      case FXIO::Create|FXIO::Exclusive: creation=CREATE_NEW; break;
      case FXIO::Create|FXIO::Truncate|FXIO::Exclusive: creation=CREATE_NEW; break;
      }

    // Attributes (hidden, read-only)
    DWORD attributes=FILE_ATTRIBUTE_NORMAL;
    if(!(perm&FXIO::AllWrite)){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_READONLY; }
    if(perm&FXIO::Hidden){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_HIDDEN; }

    // Inheritable
    SECURITY_ATTRIBUTES security;
    security.nLength=sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle=((m&FXIO::Inheritable)==0);
    security.lpSecurityDescriptor=nullptr;

    // Open file
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,filename.text(),MAXPATHLEN);
    device=::CreateFileW(unifile,access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#else
    device=::CreateFileA(filename.text(),access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#endif
    if(device!=BadHandle){

      // Prior file size
      FXlong filesize=size();

      // Make it bigger?
      if(filesize<off+len){
        filesize=off+len;
        }

      // Whole thing?
      if(len==0){
        len=filesize-off;
        }

      // Set access flags
      DWORD protect=0;
      switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
        case FXIO::ReadOnly: protect=PAGE_READONLY; break;
        case FXIO::ReadOnly|FXIO::Executable: protect=PAGE_EXECUTE_READ; break;
        case FXIO::WriteOnly: protect=PAGE_READWRITE; break;
        case FXIO::WriteOnly|FXIO::Executable: protect=PAGE_EXECUTE_READWRITE; break;
        case FXIO::ReadOnly|FXIO::WriteOnly: protect=PAGE_READWRITE; break;
        case FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable: protect=PAGE_EXECUTE_READWRITE; break;
        }

      // Create mapping
      FXInputHandle hnd=::CreateFileMapping(device,nullptr,protect,(DWORD)((off+len)>>32),(DWORD)((off+len)&0xFFFFFFFF),nullptr);
      if(hnd!=BadHandle){

        // Protection bits
        DWORD bits=0;
        switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
          case FXIO::ReadOnly: bits=FILE_MAP_READ; break;
          case FXIO::ReadOnly|FXIO::Executable: bits=FILE_MAP_READ|FILE_MAP_EXECUTE; break;
          case FXIO::WriteOnly: bits=FILE_MAP_ALL_ACCESS; break;
          case FXIO::WriteOnly|FXIO::Executable: bits=FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE; break;
          case FXIO::ReadWrite: bits=FILE_MAP_ALL_ACCESS; break;
          case FXIO::ReadWrite|FXIO::Executable: bits=FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE; break;
          }

        // Open map
        FXptr ptr=::MapViewOfFile(hnd,bits,(DWORD)(off>>32),(DWORD)(off&0xFFFFFFFF),(DWORD)len);

        if(ptr){
          // MEMORY_BASIC_INFORMATION mbi;
          // if(VirtualQuery(ptr,&mbi,sizeof(mbi))!=0){ memlength=mbi.RegionSize; }
          memhandle=hnd;
          mempointer=ptr;
          memlength=len;
          memoffset=off;
          return mempointer;
          }
        ::CloseHandle(hnd);
        }
      ::CloseHandle(device);
      device=BadHandle;
      }
    }
  return nullptr;
  }



// Return file size
FXlong FXMappedFile::size(){
  if(device!=BadHandle){
    LARGE_INTEGER result;
    if(::GetFileSizeEx(device,&result)) return result.QuadPart;
    }
  return FXIO::Error;
  }


// Synchronize disk
FXbool FXMappedFile::flush(){
  if(mempointer){
    if(::FlushViewOfFile(mempointer,memlength)!=0){
      return ::FlushFileBuffers(device)!=0;
      }
    }
  return false;
  }


// Close file, and also the map
FXbool FXMappedFile::close(){
  if(mempointer){
    if(::UnmapViewOfFile(mempointer)!=0){
      ::CloseHandle(memhandle);
      memhandle=BadHandle;
      mempointer=nullptr;
      memlength=0;
      memoffset=0;
      return FXIODevice::close();
      }
    }
  return false;
  }


// Return memory mapping granularity
FXival FXMappedFile::granularity(){
  SYSTEM_INFO SystemInfo;
  GetSystemInfo(&SystemInfo);
  return SystemInfo.dwAllocationGranularity;
  }


}

#endif
