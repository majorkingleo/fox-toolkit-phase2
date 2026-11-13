/********************************************************************************
*                                                                               *
*             P a t h   N a m e   F u n c t i o n s   ( W i n 3 2 )             *
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
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "fxpathpriv.h"

#if defined(WIN32)
#include <shellapi.h>

/*
  Notes:

  - Platform implementation of the FXPath members which parse, convert, quote and test path names according to the conventions of the platform; the portable path functions stay in FXPath.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Return root of absolute path.
// On Windows, this is of the form:
//
//     "\",
//     "C:\",
//     "\\Server\Share\",
//
// for paths starting with these strings.
// The empty string is returned if the path is not absolute.
FXString FXPath::root(const FXString& file){
  FXint p=0,s;
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    p=2;
    }
  else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    if(file[s]==PATHSEP){
      s++;
      if(file[s] && file[s]!=PATHSEP){
        s++;
        while(file[s] && file[s]!=PATHSEP) s++;
        p=s;
        }
      }
    }
  if(file[p]==PATHSEP){
    return FXString(&file[0],p+1);
    }
  return FXString::null;
  }

// Return share name part from Windows UNC filename.
// For example, the share() of "\\Server\Share\Document.doc"
// yields "\\Server\Share".
// The empty string is returned if path is not a network share.
FXString FXPath::share(const FXString& file){
  if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    FXint s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    if(file[s]==PATHSEP){
      s++;
      if(file[s] && file[s]!=PATHSEP){
        s++;
        while(file[s] && file[s]!=PATHSEP) s++;
        return FXString(&file[0],s);
        }
      }
    }
  return FXString::null;
  }

// Return server part from Windows UNC filename.
// For example, the server() of "\\Server\Share\Document.doc"
// yields "\\Server".
// The empty string is returned if path is not a network share.
FXString FXPath::server(const FXString& file){
  if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    FXint s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    return FXString(&file[0],s);
    }
  return FXString::null;
  }

// Return the drive letter in front of file name.
// Returns empty string if no drive letter present.
FXString FXPath::drive(const FXString& file){
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    return FXString(&file[0],2);
    }
  return FXString::null;
  }

// Return true if path is the form "\\Server\Share".
FXbool FXPath::isShare(const FXString& file){
  if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    FXint s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    if(file[s]==PATHSEP){
      s++;
      if(file[s] && file[s]!=PATHSEP){
        s++;
        while(file[s] && file[s]!=PATHSEP) s++;
        return (file[s]=='\0');
        }
      }
    }
  return false;
  }

// Return true if path is of the form "\\Server".
FXbool FXPath::isServer(const FXString& file){
  if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    FXint s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    return (file[s]=='\0');
    }
  return false;
  }


// Return directory part of pathname, assuming full pathname.
// Note that directory("/bla/bla/") is "/bla/bla" and NOT "/bla".
// However, directory("/bla/bla") is "/bla" as we expect!
// The "\\Server\Share\", "C:\" or "\" ("/" for Linux) are the
// top of that path's file system, and will not be dropped.
FXString FXPath::directory(const FXString& file){
  if(!file.empty()){
    FXint p=0,s;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    if(file[p]==PATHSEP) p++;
    s=p;
    while(file[p]){
      if(file[p]==PATHSEP){ s=p++; continue; }
      p++;
      }
    return FXString(&file[0],s);
    }
  return FXString::null;
  }


// Return name and extension part of pathname.
// Note that name("/bla/bla/") is "" and NOT "bla".
// However, name("/bla/bla") is "bla" as we expect!
FXString FXPath::name(const FXString& file){
  if(!file.empty()){
    FXint p=0,s;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    return FXString(&file[p],s-p);
    }
  return FXString::null;
  }


// Return file stem, i.e. document name only:
//
//  /path/aa        -> aa
//  /path/aa.bb     -> aa
//  /path/aa.bb.cc  -> aa.bb
//  /path/.aa       -> .aa
FXString FXPath::stem(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,b,e;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    b=p;
    e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ e=s; break; }
      }
    return FXString(&file[b],e-b);
    }
  return FXString::null;
  }


// Return extension, if there is one:
//
//  /path/aa        -> ""
//  /path/aa.bb     -> bb
//  /path/aa.bb.cc  -> cc
//  /path/.aa       -> ""
FXString FXPath::extension(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,b,e;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    b=e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ b=s+1; break; }
      }
    return FXString(&file[b],e-b);
    }
  return FXString::null;
  }


// Return file name less the extension
//
//  /path/aa        -> /path/aa
//  /path/aa.bb     -> /path/aa
//  /path/aa.bb.cc  -> /path/aa.bb
//  /path/.aa       -> /path/.aa
FXString FXPath::stripExtension(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,e;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ e=s; break; }
      }
    return FXString(&file[0],e);
    }
  return FXString::null;
  }


// Expand environment variables recursively, stopping when recursion goes too deep
FXString expandEnvironmentVariables(const FXString& file,FXint level){
  FXint s=0,r=0,b=0,e=0;
  FXString result;
  while(e<file.length()){
    r=e;
    if(file[e++]=='%'){       // %VAR%
      b=e;
      while(Ascii::isWord(file[e])) e++;
      if(file[e]=='%' && b<e && 0<level){
        result.append(&file[s],r-s);
        result.append(expandEnvironmentVariables(FXSystem::getEnvironment(file.mid(b,e-b)),level-1));
        s=++e;
        }
      }
    }
  result.append(&file[s],e-s);
  return result;
  }


// Convert a path to local conventions
FXString FXPath::convert(const FXString& path){
  return FXPath::convertToWindows(path);
  }


// Convert a path-list to local conventions
FXString FXPath::convertPathList(const FXString& path){
  FXString result;
  if(!path.empty()){
    FXint b=0,e=0;
    while(e<path.length()){
      while(e<path.length() && path[e]!=':') e++;
      result+=FXPath::convertToWindows(path.mid(b,e-b));
      if(path[e]==':'){ result+=';'; e++; }
      b=e;
      }
    }
  return result;
  }


// Contract path based on an environment variable.
//
// For example, on UNIX:
//
//   "/home/jeroen/junk"                -> "~/junk"
//   "/home/someoneelse/junk"           -> "~someoneelse/junk"
//   "/usr/local/ACE_wrappers/TAO"      -> "$ACE_ROOT/TAO"
//
// On Windows:
//
//   "C:\usr\local\ACE_wrappers\TAO"    -> "%ACE_ROOT%\TAO"
//
FXString FXPath::contract(const FXString& file,const FXString& user,const FXString& var){
  const FXchar legalcharacters[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
  if(!file.empty()){
    FXString result(file);
    if(FXPath::isAbsolute(result)){
      FXString dir=user.empty()?FXSystem::getHomeDirectory():FXSystem::getUserDirectory(user);
      if(!dir.empty()){
        if(FXString::compare(result,dir,dir.length())==0 && (dir.length()==result.length() || result[dir.length()]==PATHSEP)){
          result.replace(0,dir.length(),"~"+user);
          }
        }
      }
    if(!var.empty() && var.find_first_not_of(legalcharacters)<0){
      FXString val=FXSystem::getEnvironment(var);
      if(!val.empty()){
        FXint pos=result.find(val);
        if(0<=pos){
          if((pos==0 || result[pos-1]==PATHSEP) && (pos+val.length()==result.length() || result[pos+val.length()]==PATHSEP)){
            result.replace(pos,val.length(),"%"+var+"%");
            }
          }
        }
      }
    return result;
    }
  return FXString::null;
  }


// Simplify a file path; the path will remain relative if it was relative,
// or absolute if it was absolute.  Also, a trailing "/" will be preserved
// as this is important in other functions.  Finally, returned path should
// be non-empty unless the input path was empty, and pathological paths
// will be fixed.
// The result is a shorter, canonical form of the original path.
//
// Examples:
//
//    /aa/bb/../cc      -> /aa/cc
//    /aa/bb/../cc/     -> /aa/cc/
//    /aa/bb/../..      -> /
//    /..               -> /
//    ./aa/bb/../../    -> ./
//    ./aa/bb/../..     -> .
//    ./aa/bb/../../../ -> ../
//    ./aa/bb/../../..  -> ..
//    /aa/bb/../../../  -> /
//    /aa/bb/../../..   -> /
//    a/..              -> .
//    a/../..           -> ..
//    a/../             -> ./
//    /aa/ccc/../../bb  -> /bb
//    ./a               -> a
//    /////./././       -> /
//    /.                -> /
//    ./                -> ./
//    /a/b/./           -> /a/b/
//    /a/b/.            -> /a/b
//    /a/./b/.          -> /a/b
//    /a/./b/./         -> /a/b/
//    ./..              -> ..
//    /aa/bb/..         -> /aa
//    ../..             -> ../..
//    ../a/../b         -> ../b
//    a/../b            -> b
//    a/b/../../../     -> ../
//    /a/b/../../../    -> /
//    ../a/./b          -> ../a/b
//    /a/.              -> /a
//    /a../c/.          -> /a../c
//    /..a/c/.          -> /..a/c
//    ../../c/.         -> ../../c
//
FXString FXPath::simplify(const FXString& file){
  if(!file.empty()){
    FXString result(file);
    FXint p=0,q=0,s=0,r=0;
    if(Ascii::isLetter(result[0]) && result[1]==':'){
      result[0]=result[0];
      result[1]=':';
      p=2;
      q=2;
      }
    else if(result[0]==PATHSEP && result[1]==PATHSEP){
      result[0]=PATHSEP;
      result[1]=PATHSEP;
      p=2;
      q=2;
      if(result[2] && result[2]!=PATHSEP){
        while(result[q] && result[q]!=PATHSEP){
          result[p++]=result[q++];
          }
        if(result[q]==PATHSEP){
          result[p++]=PATHSEP;
          q++;
          if(result[q] && result[q]!=PATHSEP){
            while(result[q] && result[q]!=PATHSEP){
              result[p++]=result[q++];
              }
            }
          }
        }
      }
    if(result[q]==PATHSEP){
      while(result[q]==PATHSEP) q++;
      result[p++]=PATHSEP;
      }
    s=r=p;
    while(result[q]){
      if(result[q]=='.'){
        q++;
        if(!result[q]){                 // '.'
          if(s<p && result[p-1]==PATHSEP) p--;
          if(!p){
            result[p++]='.';
            }
          continue;
          }
        if(result[q]==PATHSEP){        // './'
          while(result[q]==PATHSEP) q++;
          if(!result[q] && !p){
            result[p++]='.';
            result[p++]=PATHSEP;
            }
          continue;
          }
        if(result[q]=='.'){
          q++;
          if(!result[q]){               // '..'
            if(s<p && result[p-1]==PATHSEP){
              p--;
              while(s<p && result[p-1]!=PATHSEP) p--;
              if(s<p && result[p-1]==PATHSEP) p--;
              if(!p){
                result[p++]='.';
                }
              continue;
              }
            if(!r){                     // Can't go up from root
              result[p++]='.';
              result[p++]='.';
              s=p;
              }
            continue;
            }
          if(result[q]==PATHSEP){       // '../'
            while(result[q]==PATHSEP) q++;
            if(s<p && result[p-1]==PATHSEP){
              p--;
              while(s<p && result[p-1]!=PATHSEP) p--;
              if(!result[q] && !p){
                result[p++]='.';
                result[p++]=PATHSEP;
                }
              continue;
              }
            if(!r){                     // Can't go up from root
              result[p++]='.';
              result[p++]='.';
              result[p++]=PATHSEP;
              s=p;
              }
            continue;
            }
          q--;
          }
        q--;
        }
      while(result[q] && result[q]!=PATHSEP){
        result[p++]=result[q++];
        }
      if(result[q]==PATHSEP){
        while(result[q]==PATHSEP) q++;
        result[p++]=PATHSEP;
        }
      }
    return result.trunc(p);
    }
  return FXString::null;
  }


// Check if file represents absolute pathname
//
// On Windows, isAbsolute() returns true for paths of the form:
//
//   "\"
//   "C:\"
//   "\\Server\Share\"
//
FXbool FXPath::isAbsolute(const FXString& file){
  FXint p=0,s;
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    p=2;
    }
  else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    if(file[s]==PATHSEP){
      s++;
      if(file[s] && file[s]!=PATHSEP){
        s++;
        while(file[s] && file[s]!=PATHSEP) s++;
        p=s;
        }
      }
    }
  return (file[p]==PATHSEP);
  }

// Fully qualified absolute, i.e. has "C:\" or "\\Server\Share\"
static inline FXbool fq(const FXString& file){
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    return (file[2]==PATHSEP);
    }
  if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
    FXint s=3;
    while(file[s] && file[s]!=PATHSEP) s++;
    if(file[s]==PATHSEP){
      s++;
      if(file[s] && file[s]!=PATHSEP){
        s++;
        while(file[s] && file[s]!=PATHSEP) s++;
        return (file[s]==PATHSEP);
        }
      }
    }
  return false;
  }

// Return absolute path name.
// If file started with "\" but is not fully qualified i.e. has
// no drive letter ["C:"] or file share prefix ["\\Server\Share"],
// then just prefix the current directory's root in front of the
// file to complete it.
// Otherwise, place the current directory in front, and simplify
// the result to obtain canonical path.
FXString FXPath::absolute(const FXString& file){
  if(!fq(file)){
    FXString result(FXSystem::getCurrentDirectory());
    if(file[0]==PATHSEP && file[1]!=PATHSEP){
      if(Ascii::isLetter(result[0]) && result[1]==':'){
        result.trunc(2);
        }
      else if(result[0]==PATHSEP && result[1]==PATHSEP && result[2]!=PATHSEP && result[2]){
        FXint p=3;
        while(result[p] && result[p]!=PATHSEP) p++;
        if(result[p]==PATHSEP){
          p++;
          if(result[p] && result[p]!=PATHSEP){
            p++;
            while(result[p] && result[p]!=PATHSEP) p++;
            result.trunc(p);
            }
          }
        }
      }
    if(!file.empty()){
      if(result.tail()!=PATHSEP) result.append(PATHSEP);
      result.append(file);
      }
    return FXPath::simplify(result);
    }
  return FXPath::simplify(file);
  }

// Return absolute path from base directory and file name.
// If file started with "\" but is not fully qualified i.e. has
// no drive letter ["C:"] or file share prefix ["\\Server\Share"],
// then just prefix the base-directory's root in front of the
// file to complete it.
// Otherwise, place the base-directory in front, and simplify
// the result to obtain canonical path.
FXString FXPath::absolute(const FXString& base,const FXString& file){
  if(!fq(file)){
    FXString result(FXPath::absolute(base));
    if(file[0]==PATHSEP && file[1]!=PATHSEP){
      if(Ascii::isLetter(result[0]) && result[1]==':'){
        result.trunc(2);
        }
      else if(result[0]==PATHSEP && result[1]==PATHSEP && result[2]!=PATHSEP && result[2]){
        FXint p=3;
        while(result[p] && result[p]!=PATHSEP) p++;
        if(result[p]==PATHSEP){
          p++;
          if(result[p] && result[p]!=PATHSEP){
            p++;
            while(result[p] && result[p]!=PATHSEP) p++;
            result.trunc(p);
            }
          }
        }
      }
    if(!file.empty()){
      if(result.tail()!=PATHSEP) result.append(PATHSEP);
      result.append(file);
      }
    return FXPath::simplify(result);
    }
  return FXPath::simplify(file);
  }


// Return relative path of file to given absolute base directory
//
// Examples:
//
//  Base       File         Result      Comment
//  a          /b           /b          Base is relative but file is not
//  /a         b            b           Base is absolute but file is not
//
//  /a/b/c     /a/b/c/d     d           Branch point is /a/b/c
//  /a/b/c/    /a/b/c/d     d           Branch point is /a/b/c
//
//  a          b            ../b        Branch point is assumed ..
//  ./a        ./b          ../b        Branch point is assumed ..
//
//  /a/b/c     /a/b/c       ../c        Branch point is /a/b
//  /a/b/c/    /a/b/c       ../c        Branch point is /a/b
//  /a/b/c/    /a/b/c/      .           Branch point is /a/b/c
//  /a/b/c     /a/b/c/      .           Branch point is /a/b/c
//
//  ../a/b/c   ../a/b/c/d   d           Branch point is ../a/b/c
//
//  /a/b/c/d   /a/b/c       ../         Branch point is /a/b/c
//
//  /a/b/c/d   /a/b/q       ../../q     Branch point is /a/b
//
//  /          /a           a           Branch point is /
//  /a         /b           ../b        Branch point is /
//  /a/b       /c           ../../c     Branch point is /
//  /          /a/b         a/b         Branch point is /
//  /p/q       /a/b         ../../a/b   Branch point is /
//
FXString FXPath::relative(const FXString& base,const FXString& file){
  if(!base.empty() && !file.empty()){
    FXint p=0,q=0,bp=0,bq=0;

    // Drive letter same?
    if(Ascii::isLetter(base[0]) && base[1]==':'){
      if(Ascii::toUpper(file[0])!=Ascii::toUpper(base[0])) return file;
      if(file[1]!=':') return file;
      p=2;
      q=2;
      }

    // Server and share same?
    else if(base[0]==PATHSEP && base[1]==PATHSEP){
      if(file[0]!=PATHSEP || file[1]!=PATHSEP) return file;
      p=2;
      q=2;
      while(base[p] && base[p]!=PATHSEP){
        if(Ascii::toLower(base[p])!=Ascii::toLower(file[q])) return file;
        p++;
        q++;
        }
      if(base[p]==PATHSEP){
        if(file[q]!=PATHSEP) return file;
        p++;
        q++;
        while(base[p] && base[p]!=PATHSEP){
          if((Ascii::toLower(base[p])!=Ascii::toLower(file[q]))) return file;
          p++;
          q++;
          }
        }
      }

    // Find branch point
    while(base[p] && file[q]){
      if(base[p]==PATHSEP && file[q]==PATHSEP){
        bp=p; while(base[p]==PATHSEP) p++;  // Eat multiple slashes
        bq=q; while(file[q]==PATHSEP) q++;
        continue;
        }
      if(Ascii::toLower(base[p])==Ascii::toLower(file[q])){
        p++;
        q++;
        continue;
        }
      break;
      }

   // Base and file should be either both absolute or both relative.
   // The branch point MUST be at a proper head, i.e. at least a
   // "C:\" or "\\Server\Share\" on Windows ["/" on Linux] if it is
   // an absolute path.
   if((base[bq]==PATHSEP) == (file[bq]==PATHSEP)){

      // Check if common prefix extends to the end of the base-path.
      // If the file-path ends in "/", the final component is a directory,
      // in which case it is OK to return ".".  Otherwise we prefer
      // to prefix the file-part with "../".
      if((base[p]=='\0' || base[p]==PATHSEP) && file[q]==PATHSEP){
        bp=p;
        bq=q;
        }

      // Strip leading path character off, if any
      while(file[bq]==PATHSEP) bq++;

      // Non trivial
      if(file[bq]){
        FXString result;

        // Up to branch point
        while(base[bp]){
          while(base[bp]==PATHSEP) bp++;
          if(base[bp]){
            while(base[bp] && base[bp]!=PATHSEP) bp++;
            result.append(".." PATHSEPSTRING);
            }
          }

        // Append tail end
        result.append(&file[bq]);
        return result;
        }
      return ".";
      }
    }
  return file;
  }


// Return true if file is inside base directory
//
// Examples:
//
//  Base       File         Result      Comment
//  /a/b/c     /a/b/c/d      yes        /a/b/c/d is under directory /a/b/c
//  /a/b/c     /a/b          no         /a/b is NOT under directory /a/b/c
//  ./a/b      ./a/b/c       yes        ./a/b/c is under directory ./a/b
//  a          b             no         ./b is NOT under ./a
//  a/b        a/c           no         ./a/c is NOT under ./a/b
//  /a/b/c     c             no         ./c is NOT (necessarily) under /a/b/c
//  a          /a/b          no         /a/b is NOT under ./a
//  .          b             yes        ./b is under .
//  ..         b             yes        ./b is under ./..
//  ../a       b             no         ./b is NOT under ../a
//  ./a/b      a/b           yes        ./a/b is under ./a/b
//  a/b        ./a/b/c       yes        ./a/b/c is under ./a/b
//  ./a/b      a/b           yes        ./a/b is under ./a/b
//  .          .             yes        . is under .
//  ..         .             yes        . is under ./..
//  ..         ..            yes        .. is under ..
//  .          ..            no         ./.. is NOT under .
//  ../a/b     ../a/b/c      yes        ../a/b/c is under ../a/b
//  ../a/b     ../d          no         ../d is NOT under ../a/b
//  (empty)    (something)   no         ./something is NOT under empty
FXbool FXPath::isInside(const FXString& base,const FXString& file){
  if(!base.empty() && !file.empty()){
    FXint p=0,q=0,v=0;
    // Drive letter same?
    if(Ascii::isLetter(base[0]) && base[1]==':'){
      if(Ascii::toUpper(file[0])!=Ascii::toUpper(base[0])) return false;
      if(file[1]!=':') return false;
      p=2;
      q=2;
      }

    // Server and share same?
    else if(base[0]==PATHSEP && base[1]==PATHSEP){
      if(file[0]!=PATHSEP) return false;
      if(file[1]!=PATHSEP) return false;
      p=2;
      q=2;
      while(base[p] && base[p]!=PATHSEP){
        if(Ascii::toLower(base[p])!=Ascii::toLower(file[q])) return false;
        p++;
        q++;
        }
      if(base[p]==PATHSEP){
        if(file[q]!=PATHSEP) return false;
        p++;
        q++;
        while(base[p] && base[p]!=PATHSEP){
          if((Ascii::toLower(base[p])!=Ascii::toLower(file[q]))) return false;
          p++;
          q++;
          }
        }
      }

    // Process potentially non-canonical, case-insensitive paths
    while(base[p]){
      if(base[p]==PATHSEP){
        if(file[q]!=PATHSEP) return false;
        while(base[p]==PATHSEP) p++;                            // Eat '/'
        while(file[q]==PATHSEP) q++;                            // Eat '/'
        }
a:    if(base[p]=='.'){
        if(base[p+1]=='\0'){ p+=1; goto a; }                    // Eat '.'
        if(base[p+1]==PATHSEP){ p+=2; goto a; }                 // Eat './'
        if(base[p+1]=='.'){
          if(base[p+2]=='\0'){ p+=2; v++; goto a; }             // Eat '..'
          if(base[p+2]==PATHSEP){ p+=3; v++; goto a; }          // Eat '../'
          }
        }
b:    if(file[q]=='.'){
        if(file[q+1]=='\0'){ q+=1; goto b; }                    // Eat '.'
        if(file[q+1]==PATHSEP){ q+=2; goto b; }                 // Eat './'
        if(file[q+1]=='.'){
          if(file[q+2]=='\0'){ q+=2; v--; goto b; }             // Eat '..'
          if(file[q+2]==PATHSEP){ q+=3; v--; goto b; }          // Eat '../'
          }
        }
      if(v<0) return false;
      while(base[p] && base[p]!=PATHSEP){
        if(Ascii::toUpper(base[p])!=Ascii::toUpper(file[q])) return false;
        p++;
        q++;
        }
      }
    return true;
    }
  return false;
  }
 


// Does file represent topmost directory
//
// On Windows, isTopDirectory() returns true for paths of the form:
//
//   "\"
//   "C:\"
//   "\\Server\Share\"
//
// On Linux, isTopDirectory() returns true for paths of the form:
//
//   "/"
//
FXbool FXPath::isTopDirectory(const FXString& file){
  if(!file.empty()){
    FXint p=0,s;
    if(Ascii::isLetter(file[0]) && file[1]==':'){
      p=2;
      }
    else if(file[0]==PATHSEP && file[1]==PATHSEP && file[2]!=PATHSEP && file[2]){
      s=3;
      while(file[s] && file[s]!=PATHSEP) s++;
      if(file[s]==PATHSEP){
        s++;
        if(file[s] && file[s]!=PATHSEP){
          s++;
          while(file[s] && file[s]!=PATHSEP) s++;
          p=s;
          }
        }
      }
    return file[p]==PATHSEP && file[p+1]=='\0';
    }
  return false;
  }


// Return true if input path is a hidden file or directory
FXbool FXPath::isHidden(const FXString& file){
  if(!file.empty()){
#ifdef UNICODE
    WIN32_FILE_ATTRIBUTE_DATA attr;
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,file.text(),MAXPATHLEN);
    if(GetFileAttributesExW(unifile,GetFileExInfoStandard,&attr)){
      return (attr.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)!=0;
      }
#else
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if(GetFileAttributesExA(file.text(),GetFileExInfoStandard,&attr)){
      return (attr.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)!=0;
      }
#endif
    }
  return false;
  }


// Enquote filename to make safe for shell
//
// Enclosing with double quotes if needed:
//
//   - If force is true.
//   - If filename is empty.
//   - If any whitespace in file.
//   - If special shell characters encountered.
//
// Backslashes are doubled if:
//
//   - Followed by closing quote '"'.
//   - Followed by escaped quote '\"'.
//
// Possible alternative for '"', if '\"' proves troublesome:
//
//   - Replace by '""' if inside quotes and not preceeded by '\'.
//   - Replace by '""""' if outside quotes and not preceeded by '\'.
//
// Presence of shell characters forces quotes; this way, special
// symbols pass through cmd shell unmolested.
// We could also escape shell characters with the shell escape symbol
// '^'; however, this means other programs which only process quoted
// strings will have problems.  So quotation is best here.
FXString FXPath::enquote(const FXString& file,FXbool force){
  if(!file.empty()){
    FXint p,q,c,n;
    p=q=n=0;
    do{
      c=file[q++];
      switch(c){
      case '\\':                        // Backslash
        p++;
        n++;
        break;
      case '"':                         // Embedded '"'
        p+=n;                           // Double '\'
        p+=2;                           // Plus escaped '"'
        n=0;
        break;
      case '\0':                        // End of string
        if(!force) break;
        p+=n;                           // Double '\'
        n=0;
        break;
      case ' ':                         // White space
      case '\t':
        force=true;
        p++;
        n=0;
        break;
      case '^':                         // Escape character
      case '<':                         // Redirection
      case '>':
      case '(':
      case ')':
      case '|':                         // Pipe
      case '%':                         // Environment variables
      case '!':                         // Wildcard
      case '&':                         // Command separators
      case '*':                         // Wildcard
      case '?':
      case '@':
       force=true;                      // Must quote shell characters
        /*FALL*/
      default:                          // Normal characters
        p++;
        n=0;
        break;
        }
      }
    while(c);

    // Space for quotes or space for escapes
    if(force) p+=2;

    // File must be escaped or quoted
    if(file.length()<p){
      FXString result('\0',p);
      p=q=n=0;
      if(force) result[p++]='"';        // Opening quote
      do{
        c=file[q++];
        switch(c){
        case '\\':                      // Backslash
          result[p++]='\\';
          n++;
          break;
        case '"':                       // Embedded '"'
          while(n){
            result[p++]='\\';           // Double the '\'
            n--;
            }
          result[p++]='\\';             // Escaped '\"'
          result[p++]='"';
          n=0;
          break;
        case '\0':                      // End of string
          if(!force) break;
          while(n){
            result[p++]='\\';           // Double the '\'
            n--;
            }
          n=0;
          break;
        case ' ':                       // White space
        case '\t':
          result[p++]=c;
          n=0;
          break;
        case '^':                       // Escape character
        case '<':                       // Redirection
        case '>':
        case '(':
        case ')':
        case '|':                       // Pipe
        case '%':                       // Environment variables
        case '!':                       // Wildcard
        case '&':                       // Command separators
        case '*':                       // Wildcards
        case '?':
        case '@':
          /*FALL*/
        default:                        // Normal characters
          result[p++]=c;
          n=0;
          break;
          }
        }
      while(c);
      if(force) result[p++]='"';        // Closing quote
      FXASSERT(p==result.length());
      return result;
      }

    // Unchanged filename
    return file;
    }
  return "\"\"";
  }

// Dequote filename that was enquoted for the command line.
//
//  Input       Result          Comment
//
//  ""          (empty)         To pass empty string.
//
//  "a b"       a b             Use quotes for embedded spaces.
//  a^ b        a b             Escape for embedded spaces to cmd.exe.
//  a b         a               Only first parameter will be returned.
//
//
//  \"          "               Escape with '\'' to embed quotes.
// """"         "               Inside quotes, '""' yields one '"'.
//
//  \\\"        \"              Odd number of '\' followed by '"' yields half that
//  \\\\\"      \\"             number of '\' followed by a '"'.
//
//  a\\"b c"    a\b c           Even number of '\' followed by a '"' yields to half that
//  a\\\\"b c"  a\\b c          number of '\', with quotes serving as delimiter.
//
//  a\b         a\b             When not followed by a '"', a '\' is copied literally.
//  "a\\b"      a\\b
//
//  ^<file      <file           Caret ^ serves as escape ONLY in non-quoted string.
//  ^^          ^               Embed the escape charecter itself.
//
//  ^<LF>abc    abc             Line continuation; first character on next line
//  ^<CR><LF>"  "               is treated as having been escaped!
//
// Here we do handle special shell characters via standard shell escape '^'.
// Line-continuation is performen when '^' followed by a CR, LF, or CRLF.
// Be careful with the first character after line-continuation, it will be
// interpreted as having been escaped!
FXString FXPath::dequote(const FXString& file){
  if(!file.empty()){
    FXString result(file);
    FXint quote=0,param=0,slash=0,p=0,q=0;
    FXchar c;
    while(c=file[q++]!='\0'){
      switch(c){
      case '\\':                        // Backslashes get special handling
        result[p++]=c;
        slash++;
        param=1;
        break;
      case '"':                         // Double quoted
        p-=(slash>>1);                  // Back of over half the '\'
        if((slash&1)==0){               // Even number of '\'
          if(file[q]=='"' && quote){    // Second '"' after '"' in quote
            result[p++]='"';            // Embedded quote
            q++;
            }
          else{
            quote^=1;                   // Unescaped '"'
            }
          }
        else{                           // Odd number of '\'
          result[p-1]='"';              // Overwrite '\' with '"'
          }
        slash=0;
        param=1;
        break;
      case ' ':                         // White space
      case '\t':
        if(quote) goto nrm;             // Embedded whitespace
        if(param) goto fin;             // Finished parameter
        break;
      case '^':                         // Shell escape if not quoted
        if(quote) goto nrm;
        if(file[q]){
          c=file[q++];                  // Escaped character
          if(c=='\r' && file[q]){       // CR line continuation
            c=file[q++];                // Escape character after continuation
            }
          if(c=='\n' && file[q]){       // LF or CRLF line continuation
            c=file[q++];                // Escape character after continuation
            }
          }
        /*FALL*/
      default:                          // Normal characters
nrm:    result[p++]=c;                  // Append it
        slash=0;
        param=1;
        break;
        }
      }
    FXASSERT(p<=result.length());
fin:result.trunc(p);
    return result;
    }
  return FXString::null;
  }


// Parse command to argc and argv, according to os-native rules
//
// 2N+1 backslashes + '"' : N backslashes + literal '"'
// 2N backslashes + '"'   : N backslashes and begin/end of quoted text
// N backslashes          : N backslashes
//
// '""' inside quote      : Inserts one '"'
//
FXint FXPath::parseArgs(FXchar**& argv,const FXchar* command){
  argv=nullptr;
  if(command){
    const FXchar* p;
    FXint token=0;
    FXint quote=0;
    FXint slash=0;
    FXint count=0;
    FXint space=0;
    FXchar* buffer;
    FXchar** ptr;
    FXchar* arg;
    FXchar* a;
    FXchar c;

    // First pass: measure output
    p=command;
    while((c=*p++)!='\0'){
      switch(c){
      case '\\':                        // Backslashes get special handling
        token=1;
        slash++;
        space++;
        break;
      case '"':                         // Double quotes
        token=1;                        // NO back up by slash/2
        if((slash&1)==0){               // Even number of '\'
          if(*p=='"' && quote){         // Double '"'
            space++;
            p++;
            }
          else{
            quote^=1;                   // Unescaped '"'
            }
          }
        slash=0;
        break;
      case ' ':                         // White space
      case '\t':
        if(quote) goto nrm1;            // White space in parameter
        /*FALL*/
      case '\v':
      case '\r':
      case '\f':
      case '\n':
        slash=0;
        if(!token) break;               // No preceeding token
        token=0;
        count++;
        space++;
        break;
      default:                          // Normal characters
nrm1:   token=1;
        space++;
        slash=0;
        break;
        }
      }

    // Final closeout
    count+=token;
    space+=token;

    // We got at least one argument
    if(count){

      // Allocate one buffer for the whole thing
      if(allocElms(buffer,(count+1)*sizeof(FXchar*)+space)){

        // First part of buffer contains pointers
        argv=ptr=(FXchar**)buffer;

        // Point to where the characters start
        arg=a=(FXchar*)&argv[count+1];

        quote=token=slash=0;

        // Second pass: generate output
        p=command;
        while((c=*p++)!='\0'){
          switch(c){
          case '\\':                    // Backslashes get special handling
            token=1;
            slash++;
            *a++=c;
            break;
          case '"':                     // Double quoted
            token=1;
            a-=(slash>>1);              // Back up by slash/2
            if((slash&1)==0){           // Even number of '\'
              if(*p=='"' && quote){     // Double '"'
                *a++='"';
                p++;
                }
              else{
                quote^=1;               // Unescaped '"'
                }
              }
            else{                       // Odd number of '\'
              *(a-1)='"';               // Overwrite '\' with '"'
              }
            slash=0;
            break;
          case ' ':                     // White space
          case '\t':
            if(quote) goto nrm2;        // White space in parameter
            /*FALL*/
          case '\v':
          case '\r':
          case '\f':
          case '\n':
            slash=0;
            if(!token) break;           // No preceeding token
            token=0;
            *ptr++=arg;
            *a++='\0';
            arg=a;
            break;
          default:                      // Normal characters
nrm2:       token=1;
            slash=0;
            *a++=c;
            break;
            }
          }

        // Final closeout
        if(token){
          *ptr++=arg;
          *a++='\0';
          }

        // Argv closeout
        *ptr=nullptr;

        // Make sure it all fitted
        FXASSERT(a<=buffer+(count+1)*sizeof(FXchar*)+space);
        return count;
        }
      }
    }
  return 0;
  }


// Check if file has executable extension
FXbool FXPath::hasExecExtension(const FXString& file){
  if(!file.empty()){
    FXString pathext(FXSystem::getExecExtensions());
    FXint beg=0;
    FXint end=0;
    do{
      end=beg;
      while(end<pathext.length() && pathext[end]!=PATHLISTSEP) end++;
      if((end-beg)<=file.length()){
        if(FXString::comparecase(&file[file.length()-(end-beg)],&pathext[beg],(end-beg))==0) return true;
        }
      beg=end+1;
      }
    while(end<pathext.length());
    }
  return false;
  }

// Check if given name is controversial
FXbool FXPath::isReservedName(const FXString& file){
  if(!file.empty()){
    static const FXchar reserved3[]="CON\nPRN\nAUX\nNUL\n";
    static const FXchar reserved4[]="COM1\nCOM2\nCOM3\nCOM4\nCOM5\nCOM6\nCOM7\nCOM8\nCOM9\nLPT1\nLPT2\nLPT3\nLPT4\nLPT5\nLPT6\nLPT7\nLPT8\nLPT9\n";
    if(file.length()==3){
      return (fxstrcasestr(reserved3,file.text())!=nullptr);
      }
    if(file.length()==4){
      return (fxstrcasestr(reserved4,file.text())!=nullptr);
      }
    }
  return false;
  }

}

#endif
