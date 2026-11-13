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

#if !defined(WIN32)

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

    // Access modes
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

    // Open file
    device=::open(filename.text(),flags,bits);
    if(device!=BadHandle){

      // Prior file size
      FXlong filesize=size();

      // Make it bigger?
      if(filesize<off+len){
        if(::ftruncate(device,off+len)!=0) goto fail;
        filesize=off+len;
        }

      // Whole whole thing?
      if(len==0){
        len=filesize-off;
        }

      // Non-zero map?
      if(0<len){

        // Protection bits
        FXint protect=PROT_NONE;
        switch(m&(FXIO::ReadOnly|FXIO::WriteOnly|FXIO::Executable)){
          case FXIO::ReadOnly: protect=PROT_READ; break;
          case FXIO::ReadOnly|FXIO::Executable: protect=PROT_READ|PROT_EXEC; break;
          case FXIO::WriteOnly: protect=PROT_WRITE; break;
          case FXIO::WriteOnly|FXIO::Executable: protect=PROT_WRITE|PROT_EXEC; break;
          case FXIO::ReadWrite: protect=PROT_READ|PROT_WRITE; break;
          case FXIO::ReadWrite|FXIO::Executable: protect=PROT_READ|PROT_WRITE|PROT_EXEC; break;
          }

        // Open map
        FXptr ptr=::mmap(nullptr,len,protect,MAP_SHARED,device,off);
        if(ptr!=MAP_FAILED){
#if defined(F_ADD_SEALS)
          //::fcntl(device,F_ADD_SEALS,F_SEAL_SHRINK|F_SEAL_GROW);   // Prevent size changes from here on
#endif
          mempointer=ptr;
          memlength=len;
          memoffset=off;
          return mempointer;
          }
        }
fail: ::close(device);
      device=BadHandle;
      }
    }
  return nullptr;
  }



// Return file size
FXlong FXMappedFile::size(){
  if(device!=BadHandle){
    struct stat data;
    if(::fstat(device,&data)==0) return data.st_size;
    }
  return FXIO::Error;
  }


// Synchronize disk
FXbool FXMappedFile::flush(){
  if(mempointer){
    if(::msync(mempointer,memlength,MS_SYNC|MS_INVALIDATE)==0){
      return ::fsync(device)==0;
      }
    }
  return false;
  }


// Close file, and also the map
FXbool FXMappedFile::close(){
  if(mempointer){
    if(::munmap(mempointer,memlength)==0){
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
  return sysconf(_SC_PAGE_SIZE);
  }


}

#endif
