/********************************************************************************
*                                                                               *
*                             F i l e   C l a s s                               *
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


/*
  Notes:

  - Implemented many functions in terms of FXFile and FXDir so we won't have to worry about
    unicode stuff.

*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct file and attach existing handle h
FXFile::FXFile(FXInputHandle h){
  attach(h);
  }


// Construct and open a file
FXFile::FXFile(const FXString& filename,FXuint m,FXuint perm){
  open(filename,m,perm);
  }


// Return true if serial access only
FXbool FXFile::isSerial() const {
  return false;
  }


// Test if we're at the end; -1 if error
FXint FXFile::eof(){
  if(device!=BadHandle){
    return !(position()<size());
    }
  return FXIO::Error;
  }

/*******************************************************************************/

// Return true if files are identical (identical node on disk)
FXbool FXFile::identical(const FXString& file1,const FXString& file2){
  if(file1!=file2){
    FXStat info1;
    FXStat info2;
    if(FXStat::statFile(file1,info1) && FXStat::statFile(file2,info2)){
      return info1.index()==info2.index() && info1.volume()==info2.volume();
      }
    return false;
    }
  return true;
  }

/*******************************************************************************/

// Copy srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::copy(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    FXFile src(srcfile,FXIO::Reading);
    if(src.isOpen()){
      FXStat stat;
      if(FXStat::stat(src,stat)){
        FXFile dst(dstfile,overwrite?FXIO::Writing:FXIO::Writing|FXIO::Exclusive,stat.mode());
        if(dst.isOpen()){
          FXuchar buffer[4096];
          FXival  nwritten;
          FXival  nread;
          while(1){
            nread=src.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<nread) return false;
            }
          return true;
          }
        }
      }
    }
  return false;
  }

/*******************************************************************************/

// Recursively copy or move files or directories from srcfile to dstfile, overwriting dstfile if allowed
FXbool FXFile::moveFiles(const FXString& srcfile,const FXString& dstfile,FXbool overwrite){
  if(srcfile!=dstfile){
    if(FXFile::move(srcfile,dstfile,overwrite)) return true;
    if(FXFile::copyFiles(srcfile,dstfile,overwrite)){
      return FXFile::removeFiles(srcfile,true);
      }
    }
  return false;
  }

/*******************************************************************************/

// Remove file or directory, recursively if allowed
FXbool FXFile::removeFiles(const FXString& path,FXbool recursive){
  FXStat stat;
  if(FXStat::statLink(path,stat)){
    if(stat.isDirectory()){
      if(recursive){
        FXDir dir(path);
        FXString name;
        while(dir.next(name)){
          if(name[0]=='.' && (name[1]=='\0' || (name[1]=='.' && name[2]=='\0'))) continue;
          if(!FXFile::removeFiles(path+PATHSEP+name,true)) return false;
          }
        }
      return FXDir::remove(path);
      }
    return FXFile::remove(path);
    }
  return false;
  }

/*******************************************************************************/

// Concatenate srcfile1 and srcfile2 to dstfile, overwriting dstfile if allowed
FXbool FXFile::concat(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite){
  FXuchar buffer[4096]; FXival nwritten,nread;
  if(srcfile1!=dstfile && srcfile2!=dstfile){
    FXFile src1(srcfile1,FXIO::Reading);
    if(src1.isOpen()){
      FXFile src2(srcfile2,FXIO::Reading);
      if(src2.isOpen()){
        FXFile dst(dstfile,overwrite?FXIO::Writing:FXIO::Writing|FXIO::Exclusive);
        if(dst.isOpen()){
          while(1){
            nread=src1.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<0) return false;
            }
          while(1){
            nread=src2.readBlock(buffer,sizeof(buffer));
            if(nread<0) return false;
            if(nread==0) break;
            nwritten=dst.writeBlock(buffer,nread);
            if(nwritten<0) return false;
            }
          return true;
          }
        }
      }
    }
  return false;
  }


}
