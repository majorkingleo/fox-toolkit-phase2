/********************************************************************************
*                                                                               *
*                  I / O   D e v i c e   C l a s s   ( X 1 1 )                  *
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

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXIODevice members which operate on the
    native handle; the portable members stay in FXIODevice.cpp.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {


// Return access mode
FXuint FXIODevice::mode() const {
  if(device!=BadHandle){
    FXuint result=FXIO::Inheritable;
    FXint flags=::fcntl(device,F_GETFL,0);
    if(flags!=-1){
#if defined(O_NOATIME)
      if(flags&O_NOATIME){ result|=FXIO::NoAccessTime; }
#endif
      if(flags&O_APPEND){ result|=FXIO::Append; }
      if(flags&O_CREAT){ result|=FXIO::Create; }
      if(flags&O_EXCL){ result|=FXIO::Exclusive; }
      if(flags&O_RDONLY){ result|=FXIO::ReadOnly; }
      if(flags&O_WRONLY){ result|=FXIO::WriteOnly; }
      if(flags&O_TRUNC){ result|=FXIO::Truncate; }
      if(flags&O_NONBLOCK){ result|=FXIO::NonBlocking; }
#if defined(O_CLOEXEC)
      flags=::fcntl(device,F_GETFD,0);
      if(flags!=-1){
        if(flags&O_CLOEXEC){ result&=~FXIO::Inheritable; }
        }
#endif
      return result;
      }
    }
  return FXIO::Error;
  }


// Change access mode of open device
FXbool FXIODevice::mode(FXuint m){
  if(device!=BadHandle){
    FXint flags=0;
    if(m&FXIO::Append){ flags|=O_APPEND; }
    if(m&FXIO::Truncate){ flags|=O_TRUNC; }
    if(m&FXIO::NonBlocking){ flags|=O_NONBLOCK; }
#if defined(O_NOATIME)
    if(m&FXIO::NoAccessTime){ flags|=O_NOATIME; }
#endif
    if(::fcntl(device,F_SETFL,flags)==0){
#if defined(O_CLOEXEC)
      flags=O_CLOEXEC;
      if(m&FXIO::Inheritable) flags=0;
      if(::fcntl(device,F_SETFD,flags)==0){
        return true;
        }
#else
      return true;
#endif
      }
    }
  return false;
  }


// Return permissions
FXuint FXIODevice::perms() const {
  if(device!=BadHandle){
    struct stat data;
    if(::fstat(device,&data)==0){
      FXuint result=(data.st_mode&FXIO::AllFull);
      if(S_ISDIR(data.st_mode)){ result|=FXIO::Directory; }
      if(S_ISREG(data.st_mode)){ result|=FXIO::File; }
      if(S_ISLNK(data.st_mode)){ result|=FXIO::SymLink; }
      if(S_ISCHR(data.st_mode)){ result|=FXIO::Character; }
      if(S_ISBLK(data.st_mode)){ result|=FXIO::Block; }
      if(S_ISFIFO(data.st_mode)){ result|=FXIO::Fifo; }
      if(S_ISSOCK(data.st_mode)){ result|=FXIO::Socket; }
      if(data.st_mode&S_ISUID){ result|=FXIO::SetUser; }
      if(data.st_mode&S_ISGID){ result|=FXIO::SetGroup; }
      if(data.st_mode&S_ISVTX){ result|=FXIO::Sticky; }
      return result;
      }
    }
  return FXIO::Error;
  }


// Set permissions
FXbool FXIODevice::perms(FXuint p){
  if(device!=BadHandle){
    FXint bits=p&0777;
    if(p&FXIO::SetUser){ bits|=S_ISUID; }
    if(p&FXIO::SetGroup){ bits|=S_ISGID; }
    if(p&FXIO::Sticky){ bits|=S_ISVTX; }
    return (::fchmod(device,bits)==0);
    }
  return false;
  }


// Return true if handle is valid
FXbool FXIODevice::valid(FXInputHandle hnd){
  if(hnd!=BadHandle){
    return ::fcntl(hnd,F_GETFD,0)>=0;
    }
  return false;
  }


// Read block
FXival FXIODevice::readBlock(void* ptr,FXival count){
  if(device!=BadHandle){
    FXuchar* pointer=(FXuchar*)ptr;
    FXival nread=0;
    FXival nr,ct;
    while(0<count){
      ct=FXMIN(count,1073741824);
      nr=::read(device,pointer,ct);
      if(nr<0){
        if(errno==EINTR) continue;
        if(errno==EAGAIN) return FXIO::Again;
        if(errno==EWOULDBLOCK) return FXIO::Again;
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
    FXival nw,ct;
    while(0<count){
      ct=FXMIN(count,1073741824);
      nw=::write(device,pointer,ct);
      if(nw<0){
        if(errno==EINTR) continue;
        if(errno==EAGAIN) return FXIO::Again;
        if(errno==EWOULDBLOCK) return FXIO::Again;
        if(errno==EPIPE) return FXIO::Broken;
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
    if(::close(device)==0){
      device=BadHandle;
      return true;
      }
    }
  return false;
  }


}

#endif
