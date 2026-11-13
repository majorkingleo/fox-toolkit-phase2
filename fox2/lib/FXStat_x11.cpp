/********************************************************************************
*                                                                               *
*                   F i l e   S t a t i s t i c s   ( X 1 1 )                   *
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

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXStat members which query and change native file attributes and disk space; the accessors and portable helpers stay in FXStat.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

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
    const FXTime seconds=1000000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      info.modeFlags=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
      if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
      if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
      if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
      if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
      if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
      if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
      if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
      if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
      if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
      info.userNumber=data.st_uid;
      info.groupNumber=data.st_gid;
      info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
      info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
      info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
      info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
      info.accessTime=data.st_atime*seconds;
      info.modifyTime=data.st_mtime*seconds;
      info.createTime=data.st_ctime*seconds;
#endif
      info.fileVolume=(FXlong)data.st_dev;
      info.fileIndex=(FXlong)data.st_ino;
      info.fileSize=(FXlong)data.st_size;
      result=true;
      }
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
    const FXTime seconds=1000000000;
    struct stat data;
    if(::lstat(file.text(),&data)==0){
      info.modeFlags=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
      if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
      if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
      if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
      if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
      if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
      if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
      if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
      if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
      if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
      info.userNumber=data.st_uid;
      info.groupNumber=data.st_gid;
      info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
      info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
      info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
      info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
      info.accessTime=data.st_atime*seconds;
      info.modifyTime=data.st_mtime*seconds;
      info.createTime=data.st_ctime*seconds;
#endif
      info.fileVolume=(FXlong)data.st_dev;
      info.fileIndex=(FXlong)data.st_ino;
      info.fileSize=(FXlong)data.st_size;
      result=true;
      }
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
  const FXTime seconds=1000000000;
  struct stat data;
  if(::fstat(file.handle(),&data)==0){
    info.modeFlags=(data.st_mode&FXIO::AllFull);
    if(S_ISDIR(data.st_mode)) info.modeFlags|=FXIO::Directory;
    if(S_ISREG(data.st_mode)) info.modeFlags|=FXIO::File;
    if(S_ISLNK(data.st_mode)) info.modeFlags|=FXIO::SymLink;
    if(S_ISCHR(data.st_mode)) info.modeFlags|=FXIO::Character;
    if(S_ISBLK(data.st_mode)) info.modeFlags|=FXIO::Block;
    if(S_ISFIFO(data.st_mode)) info.modeFlags|=FXIO::Fifo;
    if(S_ISSOCK(data.st_mode)) info.modeFlags|=FXIO::Socket;
    if(data.st_mode&S_ISUID) info.modeFlags|=FXIO::SetUser;
    if(data.st_mode&S_ISGID) info.modeFlags|=FXIO::SetGroup;
    if(data.st_mode&S_ISVTX) info.modeFlags|=FXIO::Sticky;
    info.userNumber=data.st_uid;
    info.groupNumber=data.st_gid;
    info.linkCount=data.st_nlink;
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
    info.accessTime=data.st_atim.tv_sec*seconds+data.st_atim.tv_nsec;
    info.modifyTime=data.st_mtim.tv_sec*seconds+data.st_mtim.tv_nsec;
    info.createTime=data.st_ctim.tv_sec*seconds+data.st_ctim.tv_nsec;
#else
    info.accessTime=data.st_atime*seconds;
    info.modifyTime=data.st_mtime*seconds;
    info.createTime=data.st_ctime*seconds;
#endif
    info.fileVolume=(FXlong)data.st_dev;
    info.fileIndex=(FXlong)data.st_ino;
    info.fileSize=(FXlong)data.st_size;
    return true;
    }
  return false;
  }


// Change the mode flags for this file
FXbool FXStat::mode(const FXString& file,FXuint perm){
  if(!file.empty()){
    FXuint bits=perm&0777;
    if(perm&FXIO::SetUser) bits|=S_ISUID;
    if(perm&FXIO::SetGroup) bits|=S_ISGID;
    if(perm&FXIO::Sticky) bits|=S_ISVTX;
    return ::chmod(file.text(),bits)==0;
    }
  return false;
  }


// Return true if file exists
FXbool FXStat::exists(const FXString& file){
  if(!file.empty()){
    struct stat status;
    return ::stat(file.text(),&status)==0;
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_modified);

// Change tiome when file was last modified
FXbool FXStat::modified(const FXString& file,FXTime ns){
  PERFORMANCE_COUNTER(FXStat_modified);
  if(!file.empty()){
#if (defined(_ATFILE_SOURCE) && defined(UTIME_OMIT))
    const FXTime seconds=1000000000;
    struct timespec values[2];
    values[0].tv_sec=UTIME_OMIT;
    values[0].tv_nsec=UTIME_OMIT;
    values[1].tv_sec=ns/seconds;
    values[1].tv_nsec=ns%seconds;
    return utimensat(AT_FDCWD,file.text(),values,0)==0;
#else
    const FXTime seconds=1000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      struct timeval values[2];
      values[0].tv_sec=data.st_atime;
      values[0].tv_usec=0;
      values[1].tv_sec=ns/seconds;
      values[1].tv_usec=ns%seconds;
      return utimes(file.text(),values)==0;
      }
#endif
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_accessed);

// Change tiome when file was last accessed
FXbool FXStat::accessed(const FXString& file,FXTime ns){
  PERFORMANCE_COUNTER(FXStat_accessed);
  if(!file.empty()){
#if (defined(_ATFILE_SOURCE) && defined(UTIME_OMIT))
    const FXTime seconds=1000000000;
    struct timespec values[2];
    values[0].tv_sec=ns/seconds;
    values[0].tv_nsec=ns%seconds;
    values[1].tv_sec=UTIME_OMIT;
    values[1].tv_nsec=UTIME_OMIT;
    return utimensat(AT_FDCWD,file.text(),values,0)==0;
#else
    const FXTime seconds=1000000;
    struct stat data;
    if(::stat(file.text(),&data)==0){
      struct timeval values[2];
      values[0].tv_sec=ns/seconds;
      values[0].tv_usec=ns%seconds;
      values[1].tv_sec=data.st_mtime;
      values[1].tv_usec=0;
      return utimes(file.text(),values)==0;
      }
#endif
    }
  return false;
  }


// Change time when file was last created
FXbool FXStat::created(const FXString&,FXTime){
  return false;
  }


// Return true if file is accessible
FXbool FXStat::isAccessible(const FXString& file,FXuint m){
  if(!file.empty()){
    FXuint mode=F_OK;
    if(m&FXIO::ReadOnly) mode|=R_OK;
    if(m&FXIO::WriteOnly) mode|=W_OK;
    if(m&FXIO::Executable) mode|=X_OK;
    return access(file.text(),mode)==0;
    }
  return false;
  }


PERFORMANCE_RECORDER(FXStat_getTotalDiskSpace);

// Obtain total amount of space on disk
FXbool FXStat::getTotalDiskSpace(const FXString& path,FXulong& space){
  PERFORMANCE_COUNTER(FXStat_getTotalDiskSpace);
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
  struct statvfs info;
  if(statvfs(path.text(),&info)==0){
    space=info.f_bsize*info.f_blocks;
    return true;
    }
#endif
  return false;
  }


PERFORMANCE_RECORDER(FXStat_getAvailableDiskSpace);

// Obtain available amount of space on disk
FXbool FXStat::getAvailableDiskSpace(const FXString& path,FXulong& space){
  PERFORMANCE_COUNTER(FXStat_getAvailableDiskSpace);
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
  struct statvfs info;
  if(statvfs(path.text(),&info)==0){
    space=info.f_bsize*info.f_bfree;
    return true;
    }
#endif
  return false;
  }

}

#endif
