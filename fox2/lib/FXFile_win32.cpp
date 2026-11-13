/********************************************************************************
*                                                                               *
*                      F i l e   C l a s s   ( W i n 3 2 )                      *
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
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXPipe.h"
#include "FXDir.h"
#include "fxiopriv.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXFile members which touch native file handles; the constructors and portable helpers stay in FXFile.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Open file
FXbool FXFile::open(const FXString& filename,FXuint m,FXuint perm){
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

    // Do it
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,filename.text(),MAXPATHLEN);
    device=::CreateFileW(unifile,access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#else
    device=::CreateFileA(filename.text(),access,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,creation,attributes,nullptr);
#endif
    return (device!=BadHandle);
    }
  return false;
  }


// Get position
FXlong FXFile::position() const {
  if(device!=BadHandle){
    LARGE_INTEGER off,pos;
    off.QuadPart=pos.QuadPart=0;
    if(SetFilePointerEx(device,off,&pos,FILE_CURRENT)!=0){
      return pos.QuadPart;
      }
    }
  return FXIO::Error;
  }


// Move to position
FXlong FXFile::position(FXlong offset,FXuint from){
  if(device!=BadHandle){
    LARGE_INTEGER off,pos;
    off.QuadPart=pos.QuadPart=offset;
    if(SetFilePointerEx(device,off,&pos,from)!=0){
      return pos.QuadPart;
      }
    }
  return FXIO::Error;
  }


// Truncate file
FXlong FXFile::truncate(FXlong sz){
  if(device!=BadHandle){
    LARGE_INTEGER off,pos;
    off.QuadPart=pos.QuadPart=sz;
    if(SetFilePointerEx(device,off,&pos,FILE_BEGIN)!=0){
      if(::SetEndOfFile(device)!=0){
        return sz;
        }
      }
    }
  return FXIO::Error;
  }


// Flush to disk
FXbool FXFile::flush(){
  if(device!=BadHandle){
    return ::FlushFileBuffers(device)!=0;
    }
  return false;
  }


// Return file size
FXlong FXFile::size(){
  if(device!=BadHandle){
    LARGE_INTEGER result;
    if(::GetFileSizeEx(device,&result)) return result.QuadPart;
    }
  return FXIO::Error;
  }


// Create new (empty) file; fails if file already exists
FXbool FXFile::create(const FXString& file,FXuint perm){
  if(!file.empty()){
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    DWORD attributes=FILE_ATTRIBUTE_NORMAL;
    if(!(perm&FXIO::AllWrite)){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_READONLY; }
    if(perm&FXIO::Hidden){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_HIDDEN; }
    FXInputHandle h=::CreateFileW(unifile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,attributes,nullptr);
    if(h!=BadHandle){ ::CloseHandle(h); return true; }
#else
    DWORD attributes=FILE_ATTRIBUTE_NORMAL;
    if(!(perm&FXIO::AllWrite)){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_READONLY; }
    if(perm&FXIO::Hidden){ attributes=(attributes&~FILE_ATTRIBUTE_NORMAL)|FILE_ATTRIBUTE_HIDDEN; }
    FXInputHandle h=::CreateFileA(file.text(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,attributes,nullptr);
    if(h!=BadHandle){ ::CloseHandle(h); return true; }
#endif
    }
  return false;
  }


// Link file
FXbool FXFile::link(const FXString& srcfile,const FXString& dstfile){
  if(srcfile!=dstfile){
#if defined(UNICODE)
    FXnchar srcname[MAXPATHLEN];
    FXnchar dstname[MAXPATHLEN];
    utf2ncs(srcname,srcfile.text(),MAXPATHLEN);
    utf2ncs(dstname,dstfile.text(),MAXPATHLEN);
    return CreateHardLinkW(dstname,srcname,nullptr)!=0;
#else
    return CreateHardLinkA(dstfile.text(),srcfile.text(),nullptr)!=0;
#endif
    }
  return false;
  }


// Read symbolic link
FXString FXFile::symlink(const FXString& file){
  if(!file.empty()){
    }
  return FXString::null;
  }


// Symbolic Link file
FXbool FXFile::symlink(const FXString& srcfile,const FXString& dstfile){
  if(dstfile!=srcfile){
    }
  return false;
  }


// Recursively copy files or directories from srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::copyFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    FXStat srcstat;
    FXStat dststat;

    // Source file information
    if(FXStat::statLink(srcfile,srcstat)){

      // Destination file information
      if(FXStat::statLink(dstfile,dststat)){

        // Destination is a directory?
        if(!dststat.isDirectory()){
          if(!overwrite) return false;
          if(!FXFile::remove(dstfile)) return false;
          }
        }

      // Source is a directory
      if(srcstat.isDirectory()){
        FXString name;
        FXDir dir;

        // Open source directory
        if(!dir.open(srcfile)) return false;

        // Make destination directory if needed
        if(!dststat.isDirectory()){

          // Make directory
          if(!FXDir::create(dstfile,srcstat.mode()|FXIO::OwnerWrite)) return false;
          }

        // Copy contents of source directory
        while(dir.next(name)){

          // Skip '.' and '..'
          if(name[0]=='.' && (name[1]=='\0' || (name[1]=='.' && name[2]=='\0'))) continue;

          // Recurse
          if(!FXFile::copyFiles(srcfile+PATHSEP+name,dstfile+PATHSEP+name,overwrite)) return false;
          }

        // OK
        return true;
        }

      // Source is a file
      if(srcstat.isFile()){

        // Simply copy
        if(!FXFile::copy(srcfile,dstfile,overwrite)) return false;

        // OK
        return true;
        }

      // Source is symbolic link: make a new one
      if(srcstat.isLink()){
        FXString lnkfile=FXFile::symlink(srcfile);

        // New symlink to whatever old one referred to
        if(!FXFile::symlink(lnkfile,dstfile)) return false;

        // OK
        return true;
        }

      }
    }
  return false;
  }


// Move or rename srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::move(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
#if defined(UNICODE)
    FXnchar srcname[MAXPATHLEN];
    FXnchar dstname[MAXPATHLEN];
    utf2ncs(srcname,srcfile.text(),MAXPATHLEN);
    utf2ncs(dstname,dstfile.text(),MAXPATHLEN);
    return ::MoveFileExW(srcname,dstname,overwrite?MOVEFILE_REPLACE_EXISTING:0)!=0;
#else
    return ::MoveFileExA(srcfile.text(),dstfile.text(),overwrite?MOVEFILE_REPLACE_EXISTING:0)!=0;
#endif
    }
  return false;
  }


// Remove a file
FXbool FXFile::remove(const FXString& file){
  if(!file.empty()){
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    return ::DeleteFileW(unifile)!=0;
#else
    return ::DeleteFileA(file.text())!=0;
#endif
    }
  return false;
  }

}

#endif
