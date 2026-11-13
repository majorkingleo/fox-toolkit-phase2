/********************************************************************************
*                                                                               *
*                  S y s t e m   F u n c t i o n s   ( X 1 1 )                  *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXFile.h"
#include "FXPath.h"
#include "FXProcess.h"
#include "FXSystem.h"
#include "FXStat.h"

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXSystem members which query the user, environment, directories and commands of the running system; the portable helpers stay in FXSystem.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Get effective user id
FXuint FXSystem::user(){
  return geteuid();
  }


// Get effective group id
FXuint FXSystem::group(){
  return getegid();
  }


// Return owner name from uid
FXString FXSystem::userName(FXuint uid){
  FXchar result[64];
#ifdef HAVE_GETPWUID_R
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(uid,&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
    return FXString(pwd->pw_name);
    }
#else
  struct passwd *pwd=getpwuid(uid);
  if(pwd){
    return FXString(pwd->pw_name);
    }
#endif
  __snprintf(result,sizeof(result),"%u",uid);
  return FXString(result);
  }


// Return group name from gid
FXString FXSystem::groupName(FXuint gid){
  FXchar result[64];
#ifdef HAVE_GETGRGID_R
  ::group grpresult;
  ::group *grp;
  char buffer[1024];
  if(getgrgid_r(gid,&grpresult,buffer,sizeof(buffer),&grp)==0 && grp){
    return FXString(grp->gr_name);
    }
#else
  ::group *grp=getgrgid(gid);
  if(grp){
    return FXString(grp->gr_name);
    }
#endif
  __snprintf(result,sizeof(result),"%u",gid);
  return FXString(result);
  }


// Get current user name
FXString FXSystem::currentUserName(){
#if defined(HAVE_GETPWUID_R)
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(geteuid(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
    return FXString(pwd->pw_name);
    }
#else
  struct passwd *pwd=getpwuid(geteuid());
  if(pwd){
    return FXString(pwd->pw_name);
    }
#endif
  return FXString::null;
  }


// Get current effective group name
FXString FXSystem::currentGroupName(){
#ifdef HAVE_GETGRGID_R
  ::group grpresult;
  ::group *grp;
  char buffer[1024];
  if(getgrgid_r(getegid(),&grpresult,buffer,sizeof(buffer),&grp)==0 && grp){
    return FXString(grp->gr_name);
    }
#else
  ::group *grp=getgrgid(getegid());
  if(grp){
    return FXString(grp->gr_name);
    }
#endif
  return FXString::null;
  }


// Return value of environment variable name
FXString FXSystem::getEnvironment(const FXString& name){
  if(!name.empty()){
    const FXchar* value=getenv(name.text());
    if(value){
      return FXString(value);
      }
    }
  return FXString::null;
  }


// Change value of environment variable name
FXbool FXSystem::setEnvironment(const FXString& name,const FXString& value){
  if(!name.empty()){
#if defined(__GNU_LIBRARY__)
    if(!value.empty()){
      return setenv(name.text(),value.text(),true)==0;
      }
    unsetenv(name.text());
    return true;
#endif
    }
  return false;
  }


// Change current directory
FXbool FXSystem::setCurrentDirectory(const FXString& path){
  if(!path.empty()){
    return chdir(path.text())==0;
    }
  return false;
  }


// Get current working directory
FXString FXSystem::getCurrentDirectory(){
  FXchar buffer[MAXPATHLEN];
  if(getcwd(buffer,MAXPATHLEN)){
    return FXString(buffer);
    }
  return FXString::null;
  }


// Change current drive prefix "a:"
FXbool FXSystem::setCurrentDrive(const FXString&){
  return true;
  }

// Get current drive prefix "a:", if any
// This is the same method as used in VC++ CRT.
FXString FXSystem::getCurrentDrive(){
  return FXString::null;
  }


// Return known executable file extensions (Windows)
// Default list is:
//
//   .COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC
//
FXString FXSystem::getExecExtensions(){
  return FXString::null;
  }


// Get name of calling executable
FXString FXSystem::getExecFilename(){
#if defined(__linux__)
  FXint pid=FXProcess::current();
  FXString filename=FXString::value("/proc/%d/exe",pid);
  return FXFile::symlink(filename);
#else
  return FXString::null;
#endif
  }


// Return the home directory for the current user.
FXString FXSystem::getHomeDirectory(){
  const FXchar* str(getenv("HOME"));
  if(str!=nullptr){
    return FXString(str);
    }
  return FXString::null;
  }


// Return temporary directory.
FXString FXSystem::getTempDirectory(){
  const FXchar* dir;
  if((dir=getenv("TMPDIR"))!=nullptr){
    return FXString(dir);
    }
  return FXString("/tmp");
  }


// Return system directory
FXString FXSystem::getSystemDirectory(){
  return FXString("/bin");
  }

// Return the default directory for a given user
// On Windows, first obtain the SID string from the user account, via LookupAccountName().
// Next, we need to open registry key "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\<SID>".
// Under this key, we'll find the ProfileImagePath, which is the user's home directory.
// On Linux, things are substantially easier.
FXString FXSystem::getUserDirectory(const FXString& user){
  if(!user.empty()){
#if defined(HAVE_GETPWNAM_R)
    struct passwd pwdresult;
    struct passwd *pwd;
    FXchar buffer[1024];
    if(getpwnam_r(user.text(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
      return FXString(pwd->pw_dir);
      }
#else
    struct passwd *pwd;
    if((pwd=getpwnam(user.text()))!=nullptr){
      return FXString(pwd->pw_dir);
      }
#endif
    }
  return FXString::null;
  }


// Kick off a command to the system
FXint FXSystem::system(const FXString& cmd){
  if(!cmd.empty()){
    return ::system(cmd.text());
    }
  return -1;
  }


// Determine if UTF8 locale in effect
FXbool FXSystem::localeIsUTF8(){
  const FXchar* str;
  if((str=getenv("LC_CTYPE"))!=nullptr || (str=getenv("LC_ALL"))!=nullptr || (str=getenv("LANG"))!=nullptr){
    return (strstr(str,"utf")!=nullptr || strstr(str,"UTF")!=nullptr);
    }
  return false;
  }


// Get DLL name for given base name
FXString FXSystem::dllName(const FXString& name){
#if defined(_HPUX_) || defined(_HPUX_SOURCE)
  return "lib"+name+".sl";
#elif defined(__APPLE__)
  return "lib"+name+".dylib";
#else
  return "lib"+name+".so";
#endif
  }

}

#endif
