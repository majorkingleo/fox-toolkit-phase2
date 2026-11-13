/********************************************************************************
*                                                                               *
*                 F i l e   S t a t i s t i c s   ( W i n 3 2 )                 *
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
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"
#include "FXPerformance.h"
#include "FXIO.h"
#include "FXPath.h"
#include "FXStat.h"
#include "FXFile.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXStat members which query and change native file attributes and disk space; the accessors and portable helpers stay in FXStat.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Convert 100ns since 01/01/1601 to ns since 01/01/1970
static inline FXTime fxunixtime(const FILETIME& ft){
  const FXTime BIAS=FXLONG(116444736000000000);
  const FXTime HIGH=FXLONG(4294967296);
  const FXTime MULT=FXLONG(100);
  return (ft.dwHighDateTime*HIGH+ft.dwLowDateTime-BIAS)*MULT;
  }

// Convert ns since 01/01/1970 to 100ns since 01/01/1601
static inline FILETIME fxwintime(FXTime ut){
  const FXTime BIAS=FXLONG(116444736000000000);
  const FXTime MULT=FXLONG(100);
  FILETIME ft;
  ut=BIAS+ut/MULT;
  ft.dwLowDateTime=(DWORD)ut;
  ft.dwHighDateTime=(DWORD)(ut>>32);
  return ft;
  }

PERFORMANCE_RECORDER(FXStat_statFile);

// Get statistics of given file
FXbool FXStat::statFile(const FXString& file,FXStat& info){
  PERFORMANCE_COUNTER(FXStat_statFile);
  FXbool result=false;
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    HANDLE hfile;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if((hfile=::CreateFile(unifile,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
          info.modeFlags|=FXIO::Directory;
          }
        else{
          info.modeFlags|=FXIO::File;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
          info.modeFlags|=FXIO::Hidden;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
          info.modeFlags&=~FXIO::AllWrite;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
          info.modeFlags|=FXIO::Device;
          }
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)){
          info.modeFlags&=~FXIO::AllExec;
          }
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(data.ftLastAccessTime);
        info.modifyTime=fxunixtime(data.ftLastWriteTime);
        info.createTime=fxunixtime(data.ftCreationTime);
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#else
    HANDLE hfile;
    if((hfile=::CreateFile(file.text(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
          info.modeFlags|=FXIO::Directory;
          }
        else{
          info.modeFlags|=FXIO::File;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
          info.modeFlags|=FXIO::Hidden;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
          info.modeFlags&=~FXIO::AllWrite;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
          info.modeFlags|=FXIO::Device;
          }
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)){
          info.modeFlags&=~FXIO::AllExec;
          }
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(data.ftLastAccessTime);
        info.modifyTime=fxunixtime(data.ftLastWriteTime);
        info.createTime=fxunixtime(data.ftCreationTime);
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#endif
    }
  return result;
  }

PERFORMANCE_RECORDER(FXStat_statLink);

// Get statistice of the linked file
FXbool FXStat::statLink(const FXString& file,FXStat& info){
  PERFORMANCE_COUNTER(FXStat_statLink);
  FXbool result=false;
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    HANDLE hfile;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if((hfile=::CreateFile(unifile,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
          info.modeFlags|=FXIO::Directory;
          }
        else{
          info.modeFlags|=FXIO::File;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
          info.modeFlags|=FXIO::Hidden;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
          info.modeFlags&=~FXIO::AllWrite;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
          info.modeFlags|=FXIO::Device;
          }
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)){
          info.modeFlags&=~FXIO::AllExec;
          }
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(data.ftLastAccessTime);
        info.modifyTime=fxunixtime(data.ftLastWriteTime);
        info.createTime=fxunixtime(data.ftCreationTime);
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#else
    HANDLE hfile;
    if((hfile=::CreateFile(file.text(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,nullptr))!=INVALID_HANDLE_VALUE){
      BY_HANDLE_FILE_INFORMATION data;
      if(::GetFileInformationByHandle(hfile,&data)){
        info.modeFlags=FXIO::AllFull;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
          info.modeFlags|=FXIO::Directory;
          }
        else{
          info.modeFlags|=FXIO::File;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
          info.modeFlags|=FXIO::Hidden;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
          info.modeFlags&=~FXIO::AllWrite;
          }
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
          info.modeFlags|=FXIO::Device;
          }
        if((info.modeFlags&FXIO::File) && !FXPath::hasExecExtension(file)){
          info.modeFlags&=~FXIO::AllExec;
          }
        info.userNumber=0;
        info.groupNumber=0;
        info.linkCount=data.nNumberOfLinks;
        info.accessTime=fxunixtime(data.ftLastAccessTime);
        info.modifyTime=fxunixtime(data.ftLastWriteTime);
        info.createTime=fxunixtime(data.ftCreationTime);
        info.fileVolume=data.dwVolumeSerialNumber;
        info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
        info.fileSize=(((FXlong)data.nFileSizeHigh)<<32)|((FXlong)data.nFileSizeLow);
        result=true;
        }
      ::CloseHandle(hfile);
      }
#endif
    }
  return result;
  }


PERFORMANCE_RECORDER(FXStat_stat);

// Get statistice of the already open file
FXbool FXStat::stat(const FXFile& file,FXStat& info){
  PERFORMANCE_COUNTER(FXStat_stat);
  info.modeFlags=0;
  info.userNumber=0;
  info.groupNumber=0;
  info.linkCount=0;
  info.createTime=0;
  info.accessTime=0;
  info.modifyTime=0;
  info.fileVolume=0;
  info.fileIndex=0;
  info.fileSize=0;
  BY_HANDLE_FILE_INFORMATION data;
  if(::GetFileInformationByHandle(file.handle(),&data)){
    info.modeFlags=FXIO::AllFull;
    if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
      info.modeFlags|=FXIO::Directory;
      }
    else{
      info.modeFlags|=FXIO::File;
      }
    if(data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN){
      info.modeFlags|=FXIO::Hidden;
      }
    if(data.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
      info.modeFlags&=~FXIO::AllWrite;
      }
    if(data.dwFileAttributes&FILE_ATTRIBUTE_DEVICE){
      info.modeFlags|=FXIO::Device;
      }
    info.userNumber=0;
    info.groupNumber=0;
    info.linkCount=data.nNumberOfLinks;
    info.accessTime=fxunixtime(data.ftLastAccessTime);
    info.modifyTime=fxunixtime(data.ftLastWriteTime);
    info.createTime=fxunixtime(data.ftCreationTime);
    info.fileVolume=data.dwVolumeSerialNumber;
    info.fileIndex=(((FXulong)data.nFileIndexHigh)<<32)|((FXulong)data.nFileIndexLow);
    info.fileSize=(((FXulong)data.nFileSizeHigh)<<32)|((FXulong)data.nFileSizeLow);
    return true;
    }
  return false;
  }


// Change the mode flags for this file
FXbool FXStat::mode(const FXString& file,FXuint perm){
  if(!file.empty()){
/*
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXuint flags=::GetFileAttributesW(unifile);
    if(flags!=INVALID_FILE_ATTRIBUTES){
      if(flags&FILE_ATTRIBUTE_DIRECTORY){
        }
      else{
        }

      if((flags&FILE_ATTRIBUTE_DIRECTORY) || (perm&FXIO::AllWrite)){
        flags&=~FILE_ATTRIBUTE_READONLY;
        }
      else{
        flags|=FILE_ATTRIBUTE_READONLY;
        }
      if(perm&FXIO::Hidden) flags|=FILE_ATTRIBUTE_HIDDEN; else flags&=~FILE_ATTRIBUTE_HIDDEN;
      flags&=FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY;
      return ::SetFileAttributesW(unifile,flags)!=0;
      }
    return false;
#else
    FXuint flags=::GetFileAttributesA(unifile);
    if(flags!=INVALID_FILE_ATTRIBUTES){
      if((flags&FILE_ATTRIBUTE_DIRECTORY) || (perm&FXIO::AllWrite)) flags&=~FILE_ATTRIBUTE_READONLY; else flags|=FILE_ATTRIBUTE_READONLY;
      if(perm&FXIO::Hidden) flags|=FILE_ATTRIBUTE_HIDDEN; else flags&=~FILE_ATTRIBUTE_HIDDEN;
      flags&=FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY;
      return ::SetFileAttributesA(file.text(),flags)!=0;
      }
    return false;
#endif
*/
    }
  return false;
  }


// Return true if file exists
FXbool FXStat::exists(const FXString& file){
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    return ::GetFileAttributesW(unifile)!=INVALID_FILE_ATTRIBUTES;
#else
    return ::GetFileAttributesA(file.text())!=INVALID_FILE_ATTRIBUTES;
#endif
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_modified);

// Change tiome when file was last modified
FXbool FXStat::modified(const FXString& file,FXTime ns){
  PERFORMANCE_COUNTER(FXStat_modified);
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime=fxwintime(ns);
      if(SetFileTime(hnd,nullptr,nullptr,&wintime)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_accessed);

// Change tiome when file was last accessed
FXbool FXStat::accessed(const FXString& file,FXTime ns){
  PERFORMANCE_COUNTER(FXStat_accessed);
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime=fxwintime(ns);
      if(SetFileTime(hnd,nullptr,&wintime,nullptr)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
    }
  return false;
  }


// Change time when file was last created
FXbool FXStat::created(const FXString& file,FXTime ns){
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    FXInputHandle hnd=CreateFileW(unifile,GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#else
    FXInputHandle hnd=CreateFileA(file.text(),GENERIC_READ|FILE_WRITE_ATTRIBUTES,0,nullptr,OPEN_EXISTING,0,nullptr);
#endif
    if(hnd!=INVALID_HANDLE_VALUE){
      FILETIME wintime=fxwintime(ns);
      if(SetFileTime(hnd,&wintime,nullptr,nullptr)!=0){
        CloseHandle(hnd);
        return true;
        }
      CloseHandle(hnd);
      }
    }
  return false;
  }


// Return true if file is accessible
FXbool FXStat::isAccessible(const FXString& file,FXuint m){
  if(!file.empty()){
#ifdef UNICODE
    FXnchar unifile[MAXPATHLEN];
    WIN32_FILE_ATTRIBUTE_DATA attr;
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if(GetFileAttributesExW(unifile,GetFileExInfoStandard,&attr)){
      return (attr.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || !(m&FXIO::WriteOnly) || !(attr.dwFileAttributes&FILE_ATTRIBUTE_READONLY);
      }
#else
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if(GetFileAttributesExA(file.text(),GetFileExInfoStandard,&attr)){
      return (attr.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || !(m&FXIO::WriteOnly) || !(attr.dwFileAttributes&FILE_ATTRIBUTE_READONLY);
      }
#endif
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_getTotalDiskSpace);

// Obtain total amount of space on disk
FXbool FXStat::getTotalDiskSpace(const FXString& path,FXulong& space){
  PERFORMANCE_COUNTER(FXStat_getTotalDiskSpace);
#ifdef UNICODE
  FXnchar unifile[MAXPATHLEN];
  utf2ncs(unifile,path.text(),MAXPATHLEN);
  if(GetDiskFreeSpaceExW(unifile,nullptr,(PULARGE_INTEGER)&space,nullptr)){
    return true;
    }
#else
  if(GetDiskFreeSpaceExA(path.text(),nullptr,(PULARGE_INTEGER)&space,nullptr)){
    return true;
    }
#endif
  return false;
  }


PERFORMANCE_RECORDER(FXStat_getAvailableDiskSpace);

// Obtain available amount of space on disk
FXbool FXStat::getAvailableDiskSpace(const FXString& path,FXulong& space){
  PERFORMANCE_COUNTER(FXStat_getAvailableDiskSpace);
#ifdef UNICODE
  FXnchar unifile[MAXPATHLEN];
  utf2ncs(unifile,path.text(),MAXPATHLEN);
  if(GetDiskFreeSpaceExW(unifile,(PULARGE_INTEGER)&space,nullptr,nullptr)){
    return true;
    }
#else
  if(GetDiskFreeSpaceExA(path.text(),(PULARGE_INTEGER)&space,nullptr,nullptr)){
    return true;
    }
#endif
  return false;
  }

}

#endif
