/********************************************************************************
*                                                                               *
*                        F i l e   C l a s s   ( X 1 1 )                        *
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

#if !defined(WIN32)

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
    FXint flags=0;
    switch(m&(FXIO::ReadOnly|FXIO::WriteOnly)){
      case FXIO::ReadOnly: flags=O_RDONLY; break;
      case FXIO::WriteOnly: flags=O_WRONLY; break;
      case FXIO::ReadWrite: flags=O_RDWR; break;
      }

    // Truncate it
    if(m&FXIO::Truncate){ flags|=O_TRUNC; }

    // Change access time
#if defined(O_NOATIME)
    if(m&FXIO::NoAccessTime){ flags|=O_NOATIME; }
#endif

    // Inheritable
#if defined(O_CLOEXEC)
    if(!(m&FXIO::Inheritable)){ flags|=O_CLOEXEC; }
#endif

    // Appending and truncation
    if(m&FXIO::Append){ flags|=O_APPEND; }

    // Non-blocking mode
    if(m&FXIO::NonBlocking){ flags|=O_NONBLOCK; }

    // Creation mode
    if(m&FXIO::Create){
      flags|=O_CREAT;
      if(m&FXIO::Exclusive){ flags|=O_EXCL; }
      }

    // Permission bits
    FXint bits=perm&0777;
    if(perm&FXIO::SetUser){ bits|=S_ISUID; }
    if(perm&FXIO::SetGroup){ bits|=S_ISGID; }
    if(perm&FXIO::Sticky){ bits|=S_ISVTX; }

    // Do it
    device=::open(filename.text(),flags,bits);
    return (device!=BadHandle);
    }
  return false;
  }


// Get position
FXlong FXFile::position() const {
  if(device!=BadHandle){
    FXlong pos;
    if((pos=::lseek(device,0,SEEK_CUR))>=0){
      return pos;
      }
    }
  return FXIO::Error;
  }


// Move to position
FXlong FXFile::position(FXlong offset,FXuint from){
  if(device!=BadHandle){
    FXlong pos;
    if(0<=(pos=::lseek(device,offset,from))){
      return pos;
      }
    }
  return FXIO::Error;
  }


// Truncate file
FXlong FXFile::truncate(FXlong sz){
  if(device!=BadHandle){
    FXlong pos;
    if(::ftruncate(device,sz)==0){
      if(0<=(pos=::lseek(device,sz,SEEK_SET))){
        return pos;
        }
      }
    }
  return FXIO::Error;
  }


// Flush to disk
FXbool FXFile::flush(){
  if(device!=BadHandle){
    return ::fsync(device)==0;
    }
  return false;
  }


// Return file size
FXlong FXFile::size(){
  if(device!=BadHandle){
    struct stat data;
    if(::fstat(device,&data)==0) return data.st_size;
    }
  return FXIO::Error;
  }


// Create new (empty) file; fails if file already exists
FXbool FXFile::create(const FXString& file,FXuint perm){
  if(!file.empty()){
    FXInputHandle h=::open(file.text(),O_CREAT|O_RDWR|O_TRUNC|O_EXCL,perm&0777);
    if(h!=BadHandle){ ::close(h); return true; }
    }
  return false;
  }


// Link file
FXbool FXFile::link(const FXString& srcfile,const FXString& dstfile){
  if(srcfile!=dstfile){
    return ::link(srcfile.text(),dstfile.text())==0;
    }
  return false;
  }


// Read symbolic link
FXString FXFile::symlink(const FXString& file){
  if(!file.empty()){
    FXchar lnk[MAXPATHLEN+1];
    FXint len=::readlink(file.text(),lnk,MAXPATHLEN);
    if(0<=len){
      return FXString(lnk,len);
      }
    }
  return FXString::null;
  }


// Symbolic Link file
FXbool FXFile::symlink(const FXString& srcfile,const FXString& dstfile){
  if(dstfile!=srcfile){
    return ::symlink(srcfile.text(),dstfile.text())==0;
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

      // Source is fifo: make a new one
      if(srcstat.isFifo()){

        // Make named pipe
        if(!FXPipe::create(dstfile,srcstat.mode())) return false;

        // OK
        return true;
        }

      // Source is device/socket; only on UNIX
      if(srcstat.isDevice() || srcstat.isSocket()){
        struct stat data;
        if(::lstat(srcfile.text(),&data)==0){
          return ::mknod(dstfile.text(),data.st_mode,data.st_rdev)==0;
          }
        }

      }
    }
  return false;
  }


// Move or rename srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::move(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    if(overwrite || !FXStat::exists(dstfile)){
      return ::rename(srcfile.text(),dstfile.text())==0;
      }
    }
  return false;
  }


// Remove a file
FXbool FXFile::remove(const FXString& file){
  if(!file.empty()){
    return ::unlink(file.text())==0;
    }
  return false;
  }

}

#endif
