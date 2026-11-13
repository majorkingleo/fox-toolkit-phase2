/********************************************************************************
*                                                                               *
*                   D i r e c t o r y   C l a s s   ( X 1 1 )                   *
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
#include "FXElement.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXPath.h"
#include "FXDir.h"

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXDir members which touch the native directory handle; the recursive helper and the destructor stay in FXDir.cpp.
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

struct SPACE {
  DIR*            handle;
  struct dirent*  dp;
  };


// Construct directory enumerator
FXDir::FXDir(){
  // If this fails on your machine, determine what sizeof(SPACE) is
  // on your machine and mail it to: jeroen@fox-toolkit.net!
  //FXTRACE(TOPIC_DETAIL,"sizeof(SPACE)=%ld\n",sizeof(SPACE));
  FXASSERT(sizeof(SPACE)<=sizeof(space));
  alias_cast<SPACE>(space)->handle=nullptr;
  }


// Construct directory enumerator
FXDir::FXDir(const FXString& path){
  // If this fails on your machine, determine what sizeof(SPACE) is
  // on your machine and mail it to: jeroen@fox-toolkit.net!
  //FXTRACE(TOPIC_DETAIL,"sizeof(SPACE)=%ld\n",sizeof(SPACE));
  FXASSERT(sizeof(SPACE)<=sizeof(space));
  alias_cast<SPACE>(space)->handle=nullptr;
  open(path);
  }


// Open directory to path, return true if ok.
FXbool FXDir::open(const FXString& path){
  if(!path.empty()){
    alias_cast<SPACE>(space)->handle=opendir(path.text());
    if(alias_cast<SPACE>(space)->handle){
      return true;
      }
    }
  return false;
  }


// Returns true if the directory is open
FXbool FXDir::isOpen() const {
  return alias_cast<const SPACE>(space)->handle!=nullptr;
  }


// Go to next directory entry and return its name
FXbool FXDir::next(FXString& name){
  if(isOpen()){
    if((alias_cast<SPACE>(space)->dp=readdir(alias_cast<SPACE>(space)->handle))!=nullptr){
      name.assign(alias_cast<SPACE>(space)->dp->d_name);
      return true;
      }
    }
  name.clear();
  return false;
  }


// Go to next directory entry and return its name and mode flags
FXbool FXDir::next(FXString& name,FXuint& mode){
  if(isOpen()){
    if((alias_cast<SPACE>(space)->dp=readdir(alias_cast<SPACE>(space)->handle))!=nullptr){
      name.assign(alias_cast<SPACE>(space)->dp->d_name);
      mode=0;
#if defined(_DIRENT_HAVE_D_TYPE)
      switch(alias_cast<SPACE>(space)->dp->d_type){
        case DT_REG: mode|=FXIO::File; break;
        case DT_DIR: mode|=FXIO::Directory; break;
        case DT_LNK: mode|=FXIO::SymLink; break;
        case DT_FIFO:mode|=FXIO::Fifo; break;
        case DT_SOCK:mode|=FXIO::Socket; break;
        case DT_BLK: mode|=FXIO::Block; break;
        case DT_CHR: mode|=FXIO::Character; break;
        case DT_UNKNOWN: break;
        }
#endif
      return true;
      }
    }
  name=FXString::null;
  mode=0;
  return false;
  }


// Close directory
void FXDir::close(){
  if(isOpen()){
    closedir(alias_cast<SPACE>(space)->handle);
    alias_cast<SPACE>(space)->handle=nullptr;
    }
  }


// Create new directory
FXbool FXDir::create(const FXString& path,FXuint perm){
  if(!path.empty()){
    return ::mkdir(path.text(),perm)==0;
    }
  return false;
  }


// Remove directory
FXbool FXDir::remove(const FXString& path){
  if(!path.empty()){
    return ::rmdir(path.text())==0;
    }
  return false;
  }


// List all the files in directory
FXint FXDir::listFiles(FXString*& filelist,const FXString& path,const FXString& pattern,FXuint flags){
  FXDir dir(path);

  // Initialize to empty
  filelist=nullptr;

  // Get directory stream pointer
  if(dir.isOpen()){
    FXuint    matching=(flags&CaseFold)?(FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold):(FXPath::PathName|FXPath::NoEscape);
    FXString *newlist;
    FXint     size=0;
    FXint     count=0;
    FXString  pathname;
    FXString  name;
    FXStat    data;

    // Loop over directory entries
    while(dir.next(name)){

      // Build full pathname
      pathname=path;
      if(!ISPATHSEP(pathname.tail())) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Get info on file
      if(!FXStat::statFile(pathname,data)) continue;


      // Filter out files; a bit tricky...
      if(!data.isDirectory() && ((flags&NoFiles) || (!(flags&HiddenFiles) && name[0]=='.') || (!(flags&AllFiles) && !FXPath::match(name,pattern,matching)))) continue;

      // Filter out directories; even more tricky!
      if(data.isDirectory() && ((flags&NoDirs) || ((flags&NoParent) && (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))) || (!(flags&HiddenDirs) && name[0]=='.') || (!(flags&AllDirs) && !FXPath::match(name,pattern,matching)))) continue;


      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString [size];
        for(FXint i=0; i<count; i++){
          newlist[i].adopt(filelist[i]);
          }
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++].adopt(name);
      }
    return count;
    }
  return 0;
  }


// List all the files in directory
FXint FXDir::removeFiles(const FXString& path,const FXString& pattern,FXuint flags){
  FXDir dir(path);
  if(dir.isOpen()){
    FXuint    matching=(flags&CaseFold)?(FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold):(FXPath::PathName|FXPath::NoEscape);
    FXint     count=0;
    FXString  pathname;
    FXString  name;
    FXStat    data;

    // Loop over directory entries
    while(dir.next(name)){

      // Build full pathname
      pathname=path;
      if(!ISPATHSEP(pathname.tail())) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Get info on file
      if(!FXStat::statFile(pathname,data)) continue;


      // Filter out files; a bit tricky...
      if(!data.isDirectory() && ((flags&NoFiles) || (!(flags&HiddenFiles) && name[0]=='.') || (!(flags&AllFiles) && !FXPath::match(name,pattern,matching)))) continue;

      // Filter out directories; even more tricky!
      if(data.isDirectory() && ((flags&NoDirs) || ((flags&NoParent) && (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0)))) || (!(flags&HiddenDirs) && name[0]=='.') || (!(flags&AllDirs) && !FXPath::match(name,pattern,matching)))) continue;


      // Remove file or folder, recursively
      if(FXFile::removeFiles(pathname,true)){
        count++;
        }
      }
    return count;
    }
  return 0;
  }


// List drives, i.e. roots of directory trees.
FXint FXDir::listDrives(FXString*& drivelist){
  FXint count=0;
  drivelist=new FXString[2];
  drivelist[count++].assign(PATHSEP);
  return count;
  }


// List file shares
FXint FXDir::listShares(FXString*& sharelist){
  sharelist=new FXString [2];
  sharelist[0].assign(PATHSEP);
  return 1;
  }


#define EXPERIMENTAL 1

// Check for subdirectories
FXbool FXDir::hasSubDirectories(const FXString& path){
  FXbool result=false;
  if(!path.empty()){
#if defined(EXPERIMENTAL)
    struct stat info;
    if(stat(path.text(),&info)==0){
      // If number of hard-links is >2, then besides parent and current
      // directory (.), it also has another link (..) from a subdirectory.
      // However it may depend on type of file system so mileage may vary.
      result=S_ISDIR(info.st_mode) && (2<info.st_nlink);
      }
#else
    DIR *dir=opendir(path.text());
    if(dir){
      struct dirent *ent;
      struct stat info;
      while((ent=readdir(dir))!=nullptr){
        if(ent->d_name[0]=='.' && (ent->d_name[1]=='\0' || (ent->d_name[1]=='.' && ent->d_name[2]=='\0'))) continue;
        if(ent->d_type==DT_DIR){ result=true; break; }
        if(stat(ent->d_name,&info)==0 && S_ISDIR(info.st_mode)){ result=true; break; }
        }
      closedir(dir);
      }
#endif
    }
  return result;
  }

}

#endif
