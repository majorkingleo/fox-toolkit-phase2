/********************************************************************************
*                                                                               *
*                 P r o c e s s   S u p p o r t   ( W i n 3 2 )                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXProcess.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXProcess members which create, signal and wait for native processes; the constructor and id() stay in FXProcess.cpp.
*/

#define TOPIC_CONSTRUCT 1000

using namespace FX;

/*******************************************************************************/

namespace FX {

// Extra variables
static const FXchar* extravars[]={
  "PATH",
  "PATHEXT",
  "COMSPEC",
  "SYSTEMDRIVE",
  "SYSTEMROOT",
  "OS",
  "TMP",
  "TEMP",
  "HOMEDRIVE",
  "HOMEPATH",
  "PROGRAMFILES",
  "COMPUTERNAME",
  "USERNAME",
  "USERDOMAIN",
  "USERPROFILE",
  "ALLUSERSPROFILE",
  "COMMONPROGRAMFILES",
  "NUMBER_OF_PROCESSORS",
  "PROCESSOR_ARCHITECTURE",
  "PROCESSOR_IDENTIFIER",
  "PROCESSOR_LEVEL",
  "PROCESSOR_REVISION",
  "WINDIR",
  "LOGONSERVER"
  };

// Sort environment variables
static int CDECL comparison(const void *a1, const void *a2){
  return FXString::compare(*((const FXchar**)a1),*((const FXchar**)a2));
  }

// See if quotes are needed
static inline FXint needquotes(const FXchar* ptr){
  FXchar c;
  while((c=*ptr++)!='\0'){
    if(c==' ' || c=='\t') return 1;
    }
  return 0;
  }

#ifdef UNICODE

// Build command line for windows
static FXnchar* commandline(const FXchar *const *args){
  FXnchar *result=nullptr;
  if(args){
    const FXchar *const *arg;
    const FXchar *ptr;
    FXint size=0,quote=0,slash=0,w;
    arg=args;
    while((ptr=*arg++)!=nullptr){
      quote=needquotes(ptr);
      if(quote) size++;         // Open quote
      do{
        w=wcnxt(ptr);
        switch(w){
        case '\\':              // Backslash
          size++;
          slash++;
          break;
        case '"':               // Embedded '"'
          size+=slash;          // Double '\'
          size+=2;              // Escaped '\"'
          slash=0;
          break;
        case '\0':              // End of string
          if(quote) size+=slash;// Double '\'
          slash=0;
          break;
        case ' ':               // White space
        case '\t':
          size++;
          slash=0;
          break;
        default:                // Normal characters
          size+=wc2nc(w);
          slash=0;
          break;
          }
        }
      while(w);
      if(quote) size++;         // Close quote
      size++;                   // Separate with ' '
      }

    // Ultimate termination
    size++;

    // Allocate buffer for the command
    if(allocElms(result,size)){
      FXnchar *dst=result;
      arg=args;
      while((ptr=*arg++)!=nullptr){
        quote=needquotes(ptr);
        if(quote) *dst++='"';   // Open quote
        do{
          w=wcnxt(ptr);
          switch(w){
          case '\\':            // Backslash
            *dst++='\\';
            slash++;
            break;
          case '"':             // Embedded '"'
            while(slash){
              *dst++='\\';
              slash--;
              }
            *dst++='\\';
            *dst++='"';
            slash=0;
            break;
          case '\0':            // End of string
            if(quote){
              while(slash){
                *dst++='\\';
                slash++;
                }
              }
            slash=0;
            break;
          case ' ':             // White space
          case '\t':
            *dst++=w;
            slash=0;
            break;
          default:              // Normal characters
            dst+=wc2nc(dst,w);
            slash=0;
            break;
            }
          }
        while(w);
        if(quote) *dst++='"';   // Close quote
        *dst++=' ';             // Separate with ' '
        }
      *dst++='\0';              // Terminate
      FXASSERT(dst==result+size);
      }
    }
  return result;
  }

// Make windows environment block
static FXnchar* enviroblock(const FXchar *const *env){
  FXnchar* result=nullptr;
  if(env){
    const FXchar **tmp;
    FXint size=0,n=0,s,d;
    while(env[n]) n++;
    if(callocElms(tmp,n+1)){
      for(s=0; s<n; ++s){
        tmp[s]=env[s];
        size+=utf2ncs(env[s])+1;
        }
      qsort((void*)tmp,(size_t)s,sizeof(const FXchar*),comparison);
      if(allocElms(result,size+2)){
        for(s=d=0; s<n; ++s){
          d+=utf2ncs(&result[d],tmp[s],size-d)+1;
          }
        result[d+0]=0;
        result[d+1]=0;
        }
      FXASSERT(d<=size+2);
      freeElms(tmp);
      }
    }
  return result;
  }

#else

// Build command line for windows
static FXchar* commandline(const FXchar *const *args){
  FXchar *result=nullptr;
  if(args){
    const FXchar *const *arg;
    const FXchar *ptr;
    FXint size=0,quote=0,slash=0,w;
    arg=args;
    while((ptr=*arg++)!=nullptr){
      quote=needquotes(ptr);
      if(quote) size++;         // Open quote
      do{
        w=*ptr++;
        switch(w){
        case '\\':              // Backslash
          size++;
          slash++;
          break;
        case '"':               // Embedded '"'
          size+=slash;          // Double '\'
          size+=2;              // Escaped '\"'
          slash=0;
          break;
        case '\0':              // End of string
          if(quote) size+=slash;// Double '\'
          slash=0;
          break;
        case ' ':               // White space
        case '\t':
          size++;
          slash=0;
          break;
        default:                // Normal characters
          size++;
          slash=0;
          break;
          }
        }
      while(w);
      if(quote) size++;         // Close quote
      size++;                   // Separate with ' '
      }

    // Ultimate termination
    size++;

    // Allocate buffer for the command
    if(allocElms(result,size)){
      FXchar *dst=result;
      arg=args;
      while((ptr=*arg++)!=nullptr){
        quote=needquotes(ptr);
        if(quote) *dst++='"';   // Open quote
        do{
          w=*ptr++;
          switch(w){
          case '\\':            // Backslash
            *dst++='\\';
            slash++;
            break;
          case '"':             // Embedded '"'
            while(slash){
              *dst++='\\';
              slash--;
              }
            *dst++='\\';
            *dst++='"';
            slash=0;
            break;
          case '\0':            // End of string
            if(quote){
              while(slash){
                *dst++='\\';
                slash++;
                }
              }
            slash=0;
            break;
          case ' ':             // White space
          case '\t':
            *dst++=w;
            slash=0;
            break;
          default:              // Normal characters
            *dst++=w;
            slash=0;
            break;
            }
          }
        while(w);
        if(quote) *dst++='"';   // Close quote
        *dst++='\0';            // Separate with ' '
        }
      *dst++='\0';
      FXASSERT(dst==result+size);
      }
    }
  return result;
  }

// Make windows environment block
static FXchar* enviroblock(const FXchar *const *env){
  FXchar* result=nullptr;
  if(env){
    const FXchar **tmp;
    FXint size=0,n=0,s,d,l;
    while(env[n]) n++;
    if(callocElms(tmp,n+1)){
      for(s=0; s<n; ++s){
        tmp[s]=env[s];
        size+=strlen(env[s])+1;
        }
      qsort((void*)tmp,(size_t)s,sizeof(const FXchar*),comparison);
      if(allocElms(result,size+2)){
        for(s=d=0; s<n; ++s){
          l=strlen(tmp[s])+1;
          copyElms(&result[d],tmp[s],l);
          d+=l;
          }
        result[d+0]=0;
        result[d+1]=0;
        }
      FXASSERT(d<=size+2);
      freeElms(tmp);
      }
    }
  return result;
  }

#endif


// Start subprocess
FXbool FXProcess::start(const FXchar* exec,const FXchar *const *args,const FXchar *const *env){
  FXbool result=false;
  if(pid==0 && exec && args){
#if defined(UNICODE)
    FXnchar uniexec[MAXPATHLEN];
    utf2ncs(uniexec,exec,MAXPATHLEN);
    if(::GetFileAttributesW(uniexec)!=INVALID_FILE_ATTRIBUTES){
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      clearElms(&pi,1);
      clearElms(&si,1);

      // Init startup info
      si.cb=sizeof(si);
      si.dwFlags=STARTF_USESTDHANDLES;

      // Stdin was redirected
      si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
      if(input && input->isOpen()){
        si.hStdInput=input->handle();
        }

      // Stdout was redirected
      si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
      if(output && output->isOpen()){
        si.hStdOutput=output->handle();
        }

      // Stderr was redirected
      si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
      if(errors && errors->isOpen()){
        si.hStdError=errors->handle();
        }

      // Build wide-character command line
      FXnchar *command=commandline(args);

      // Build wide-character environment block
      FXnchar *envir=enviroblock(env);

      // Create process
      if(CreateProcessW(uniexec,command,nullptr,nullptr,true,CREATE_UNICODE_ENVIRONMENT,envir,nullptr,&si,&pi)){
        CloseHandle(pi.hThread);
        pid=pi.hProcess;
        result=true;
        }

      // Free command line and environment block
      freeElms(envir);
      freeElms(command);
      }
#else
    if(::GetFileAttributesA(exec)!=INVALID_FILE_ATTRIBUTES){
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      clearElms(&pi,1);
      clearElms(&si,1);

      // Init startup info
      si.cb=sizeof(si);
      si.dwFlags=STARTF_USESTDHANDLES;

      // Stdin was redirected
      si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
      if(input && input->isOpen()){
        si.hStdInput=input->handle();
        }

      // Stdout was redirected
      si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
      if(output && output->isOpen()){
        si.hStdOutput=output->handle();
        }

      // Stderr was redirected
      si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
      if(errors && errors->isOpen()){
        si.hStdError=output->handle();
        }

      // Build command line
      FXchar *command=commandline(args);

      // Build environment block
      FXchar *envir=enviroblock(env);

      // Create process
      if(CreateProcessA(exec,command,nullptr,nullptr,true,0,envir,nullptr,&si,&pi)){
        CloseHandle(pi.hThread);
        pid=pi.hProcess;
        result=true;
        }

      // Free command line and environment block
      freeElms(envir);
      freeElms(command);
      }
#endif
    }
  return result;
  }


// Get process id
FXint FXProcess::current(){
  return (FXint)::GetCurrentProcessId();
  }


// Get current process handle
FXInputHandle FXProcess::handle(){
  return ::GetCurrentProcess();
  }


// Exit calling process
void FXProcess::exit(FXint code){
  ::ExitProcess(code);
  }


// Suspend process
FXbool FXProcess::suspend(){
  if(pid){
/*
    HANDLE hThreadSnap=nullptr;
    HANDLE hThreadSnap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap!=INVALID_HANDLE_VALUE){
      THREADENTRY32  te32={0};
      te32.dwSize=sizeof(THREADENTRY32);
      if(Thread32First(hThreadSnap,&te32)){
        do{
          if(te32.th32OwnerProcessID==pid){
            HANDLE hThread=OpenThread(THREAD_SUSPEND_RESUME,false,te32.th32ThreadID);
            if(hThread){
              ResumeThread(hThread);
              CloseHandle(hThread);
              }
            }
          }
        while(Thread32Next(hThreadSnap,&te32));
        }
      CloseHandle(hThreadSnap);
      }
*/
    }
  return false;
  }


// Resume process
FXbool FXProcess::resume(){
  if(pid){
/*
    HANDLE hThreadSnap=nullptr;
    HANDLE hThreadSnap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap!=INVALID_HANDLE_VALUE){
      THREADENTRY32  te32={0};
      te32.dwSize=sizeof(THREADENTRY32);
      if(Thread32First(hThreadSnap,&te32)){
        do{
          if(te32.th32OwnerProcessID==pid){
            HANDLE hThread=OpenThread(THREAD_SUSPEND_RESUME,false,te32.th32ThreadID);
            if(hThread){
              SuspendThread(hThread);
              CloseHandle(hThread);
              }
            }
          }
        while(Thread32Next(hThreadSnap,&te32));
        }
      CloseHandle(hThreadSnap);
      }
*/
    }
  return false;
  }


// Kill process
FXbool FXProcess::kill(){
  if(pid){
    return ::TerminateProcess((HANDLE)pid,-1)!=0;
    }
  return false;
  }


// Wait for child process
FXbool FXProcess::wait(){
  FXbool result=false;
  if(pid){
    if(::WaitForSingleObject((HANDLE)pid,INFINITE)==WAIT_OBJECT_0){
      ::CloseHandle((HANDLE)pid);
      result=true;
      pid=0;
      }
    }
  return result;
  }


// Wait for child process, returning exit code
FXbool FXProcess::wait(FXint& code){
  FXbool result=false;
  if(pid){
    if(::WaitForSingleObject((HANDLE)pid,INFINITE)==WAIT_OBJECT_0){
      ::GetExitCodeProcess((HANDLE)pid,(ULONG*)&code);
      ::CloseHandle((HANDLE)pid);
      result=true;
      pid=0;
      }
    }
  return result;
  }


// Delete
FXProcess::~FXProcess(){
  FXTRACE(TOPIC_CONSTRUCT,"FXProcess::~FXProcess\n");
  if(pid){
    ::CloseHandle((HANDLE)pid);
    }
  }

}

#endif
