/********************************************************************************
*                                                                               *
*                S y s t e m   F u n c t i o n s   ( W i n 3 2 )                *
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

#if defined(WIN32)

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
  return 0;
  }


// Get effective group id
FXuint FXSystem::group(){
  return 0;
  }


// Return owner name from uid
FXString FXSystem::userName(FXuint uid){
  FXchar result[64];
  __snprintf(result,sizeof(result),"%u",uid);
  return FXString(result);
  }


// Return group name from gid
FXString FXSystem::groupName(FXuint gid){
  FXchar result[64];
  __snprintf(result,sizeof(result),"%u",gid);
  return FXString(result);
  }


// Get current user name
FXString FXSystem::currentUserName(){
  TCHAR buffer[MAXPATHLEN];
  DWORD size=MAXPATHLEN;
  if(GetUserName(buffer,&size)){
    return FXString(buffer);
    }
  return FXString::null;
  }


// Get current effective group name
FXString FXSystem::currentGroupName(){
  return FXString::null;
  }


// Return value of environment variable name
FXString FXSystem::getEnvironment(const FXString& name){
  if(!name.empty()){
#ifdef UNICODE
    // Windows limits total process environment space to 32767 bytes
    FXnchar variable[256],buffer[32767]; DWORD len;
    utf2ncs(variable,name.text(),ARRAYNUMBER(variable));
    if((len=GetEnvironmentVariableW(variable,buffer,ARRAYNUMBER(buffer)))>0){
      return FXString(buffer,len);
      }
#else
    // Windows limits total process environment space to 32767 bytes
    FXchar buffer[32767]; DWORD len;
    if((len=GetEnvironmentVariableA(name.text(),buffer,ARRAYNUMBER(buffer)))>0){
      return FXString(buffer,len);
      }
#endif
    }
  return FXString::null;
  }


// Change value of environment variable name
FXbool FXSystem::setEnvironment(const FXString& name,const FXString& value){
  if(!name.empty()){
#if defined(UNICODE)
    // Windows limits total process environment space to 32767 bytes
    FXnchar variable[256],buffer[32767];
    utf2ncs(variable,name.text(),ARRAYNUMBER(variable));
    if(!value.empty()){
      utf2ncs(buffer,value.text(),ARRAYNUMBER(buffer));
      return SetEnvironmentVariableW(variable,buffer)!=0;
      }
    return SetEnvironmentVariableW(variable,nullptr)!=0;
#else
    if(!value.empty()){
      return SetEnvironmentVariableA(name.text(),value.text())!=0;
      }
    return SetEnvironmentVariableA(name.text(),nullptr)!=0;
#endif
    }
  return false;
  }


// Change current directory
FXbool FXSystem::setCurrentDirectory(const FXString& path){
  if(!path.empty()){
#if defined(UNICODE)
    FXnchar unipath[MAXPATHLEN];
    utf2ncs(unipath,path.text(),MAXPATHLEN);
    return SetCurrentDirectory(unipath)!=0;
#else
    return SetCurrentDirectory(path.text())!=0;
#endif
    }
  return false;
  }


// Get current working directory
FXString FXSystem::getCurrentDirectory(){
  TCHAR buffer[MAXPATHLEN];
  if(GetCurrentDirectory(MAXPATHLEN,buffer)){
    return FXString(buffer);
    }
  return FXString::null;
  }


// Change current drive prefix "a:"
// This is the same method as used in VC++ CRT.
FXbool FXSystem::setCurrentDrive(const FXString& prefix){
  FXchar buffer[3];
  if(!prefix.empty() && Ascii::isLetter(prefix[0]) && prefix[1]==':'){
    buffer[0]=prefix[0];
    buffer[1]=':';
    buffer[2]='\0';
    return SetCurrentDirectoryA(buffer)!=0;
    }
  return false;
  }

// Get current drive prefix "a:", if any
// This is the same method as used in VC++ CRT.
FXString FXSystem::getCurrentDrive(){
  FXchar buffer[MAXPATHLEN];
  if(GetCurrentDirectoryA(MAXPATHLEN,buffer) && Ascii::isLetter((FXuchar)buffer[0]) && buffer[1]==':'){
    return FXString(buffer,2);
    }
  return FXString::null;
  }


// Return known executable file extensions (Windows)
// Default list is:
//
//   .COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC
//
FXString FXSystem::getExecExtensions(){
  return FXSystem::getEnvironment("PATHEXT");
  }


// Get name of calling executable
FXString FXSystem::getExecFilename(){
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetModuleFileName(nullptr,buffer,MAXPATHLEN);
  return FXString(buffer,len);
  }


// Return the home directory for the current user.
FXString FXSystem::getHomeDirectory(){
#ifdef UNICODE
  FXnchar path[4096];
  DWORD drivelen;
  DWORD pathlen;

  // We try %USERPROFILE% first
  if((pathlen=GetEnvironmentVariableW(L"USERPROFILE",path,ARRAYNUMBER(path)))!=0){
    return FXString(path,pathlen);
    }

  // Otherwise, try %HOMEDRIVE%%HOMEPATH%; this should be good for WinNT, Win2K according to MSDN
  if((drivelen=GetEnvironmentVariableW(L"HOMEDRIVE",path,ARRAYNUMBER(path)))!=0){
    if((pathlen=GetEnvironmentVariableW(L"HOMEPATH",&path[drivelen],ARRAYNUMBER(path)-drivelen))!=0){
      return FXString(path,drivelen+pathlen);
      }
    }
  return FXString::null;
#else
  FXchar path[4096];
  DWORD drivelen;
  DWORD pathlen;

  // We try %USERPROFILE% first
  if((pathlen=GetEnvironmentVariableA("USERPROFILE",path,ARRAYNUMBER(path)))!=0){
    return FXString(path,pathlen);
    }

  // Otherwise, try %HOMEDRIVE%%HOMEPATH%; this should be good for WinNT, Win2K according to MSDN
  if((drivelen=GetEnvironmentVariableA("HOMEDRIVE",path,ARRAYNUMBER(path)))!=0){
    if((pathlen=GetEnvironmentVariableA("HOMEPATH",&path[drivelen],ARRAYNUMBER(path)-drivelen))!=0){
      return FXString(path,drivelen+pathlen);
      }
    }
  return FXString::null;
#endif
  }


// Return temporary directory.
FXString FXSystem::getTempDirectory(){
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetTempPath(MAXPATHLEN,buffer);
  if(1<len && ISPATHSEP(buffer[len-1]) && !ISPATHSEP(buffer[len-2])) len--;
  return FXString(buffer,len);
  }


// Return system directory
FXString FXSystem::getSystemDirectory(){
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetSystemDirectory(buffer,MAXPATHLEN);
  return FXString(buffer,len);
  }

#ifdef UNICODE

static FXString getUserSID(const FXString& user){
  FXnchar usr[512],sid[512],dom[512];
  DWORD sidsize=ARRAYNUMBER(sid);
  DWORD domsize=ARRAYNUMBER(dom);
  SID_NAME_USE sidnameuse;
  utf2ncs(usr,user.text(),ARRAYNUMBER(usr));
  if(LookupAccountNameW(nullptr,usr,(SID*)sid,&sidsize,dom,&domsize,&sidnameuse)){
    FXnchar* str=nullptr;
    if(ConvertSidToStringSidW((SID*)sid,&str)){
      FXString result(str);
      LocalFree(str);
      return result;
      }
    }
  return FXString::null;
  }

#else

static FXString getUserSID(const FXString& user){
  FXchar sid[512],dom[512];
  DWORD sidsize=ARRAYNUMBER(sid);
  DWORD domsize=ARRAYNUMBER(dom);
  SID_NAME_USE sidnameuse;
  if(LookupAccountNameA(nullptr,user.text(),(SID*)sid,&sidsize,dom,&domsize,&sidnameuse)){
    FXchar* str=nullptr;
    if(ConvertSidToStringSidA((SID*)sid,&str)){
      FXString result(str);
      LocalFree(str);
      return result;
      }
    }
  return FXString::null;
  }

#endif

// Return the default directory for a given user
// On Windows, first obtain the SID string from the user account, via LookupAccountName().
// Next, we need to open registry key "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\<SID>".
// Under this key, we'll find the ProfileImagePath, which is the user's home directory.
// On Linux, things are substantially easier.
FXString FXSystem::getUserDirectory(const FXString& user){
  if(!user.empty()){
    FXString sid=getUserSID(user);
    FXString profile("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\"+sid);
    HKEY     key;
    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,profile.text(),0,KEY_READ,&key)==ERROR_SUCCESS){
#ifdef UNICODE
      FXnchar path[4096]; DWORD pathlen=ARRAYNUMBER(path);
      if(RegQueryValueExW(key,L"ProfileImagePath",nullptr,nullptr,(LPBYTE)path,&pathlen)==ERROR_SUCCESS){
        RegCloseKey(key);
        return FXString(path,pathlen);
        }
#else
      FXchar path[4096]; DWORD pathlen=ARRAYNUMBER(path);
      if(RegQueryValueExA(key,"ProfileImagePath",nullptr,nullptr,(LPBYTE)path,&pathlen)==ERROR_SUCCESS){
        RegCloseKey(key);
        return FXString(path,pathlen);
        }
#endif
      RegCloseKey(key);
      }
    }
  return FXString::null;
  }


// Kick off a command to the system
// Note, this version launches cmd.exe w/o creating a terminal window.
FXint FXSystem::system(const FXString& cmd){
  FXint res=-1;
  if(!cmd.empty()){
#if defined(UNICODE)
    FXnchar* cmd_arg;
    if(callocElms(cmd_arg,cmd.length()+4)){
      FXnchar cmd_exe[MAXPATHLEN];
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      clearElms(&pi,1);
      clearElms(&si,1);

      // Init startup info
      si.cb=sizeof(si);

      // Also suggested; not sure if this is better than CREATE_NO_WINDOW
      // for dwCreationFlags...
//      si.dwFlags=STARTF_USESHOWWINDOW;
//      si.wShowWindow=SW_HIDE;

      // Option for cmd.exe
      cmd_arg[0]='/';
      cmd_arg[1]='c';
      cmd_arg[2]=' ';

      // Copy command
      utf2ncs(&cmd_arg[3],cmd.text(),cmd.length());

      // Path to cmd.exe
      GetEnvironmentVariableW(L"COMSPEC",cmd_exe,MAXPATHLEN);

      // Probably required
      _flushall();

      // Kick off command
      DWORD dwCreationFlags = !GetConsoleWindow() ? CREATE_NO_WINDOW : 0;
      if(CreateProcessW(cmd_exe,cmd_arg,nullptr,nullptr,false,dwCreationFlags,nullptr,nullptr,&si,&pi)){
        WaitForSingleObject(pi.hProcess,INFINITE);
        GetExitCodeProcess(pi.hProcess,(DWORD*)&res);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        }

      // Free argument
      freeElms(cmd_arg);
      }
#else
    FXchar* cmd_arg;
    if(callocElms(cmd_arg,cmd.length()+4)){
      FXchar cmd_exe[MAXPATHLEN];
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      clearElms(&pi,1);
      clearElms(&si,1);

      // Init startup info
      si.cb=sizeof(si);

      // Also suggested:
//      si.dwFlags=STARTF_USESHOWWINDOW;
//      si.wShowWindow=SW_HIDE;

      // Option for cmd.exe
      cmd_arg[0]='/';
      cmd_arg[1]='c';
      cmd_arg[2]=' ';

      // Copy command
      copyElms(&cmd_arg[3],cmd.text(),cmd.length());

      // Path to cmd.exe
      GetEnvironmentVariableA("COMSPEC",cmd_exe,MAXPATHLEN);

      // Probably required
     _flushall();

      // Kick off command
      DWORD dwCreationFlags = !GetConsoleWindow() ? CREATE_NO_WINDOW : 0;
      if(CreateProcessA(cmd_exe,cmd_arg,nullptr,nullptr,false,dwCreationFlags,nullptr,nullptr,&si,&pi)){
        WaitForSingleObject(pi.hProcess,INFINITE);
        GetExitCodeProcess(pi.hProcess,(DWORD*)&res);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        }

      // Free argument
      freeElms(cmd_arg);
      }
#endif
    }
  return res;
  }


// Determine if UTF8 locale in effect
FXbool FXSystem::localeIsUTF8(){
  return GetACP()==CP_UTF8;
  }


// Get DLL name for given base name
FXString FXSystem::dllName(const FXString& name){
  return name+".dll";
  }

}

#endif
