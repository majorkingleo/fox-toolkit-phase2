/********************************************************************************
*                                                                               *
*                   P r o c e s s   S u p p o r t   ( X 1 1 )                   *
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

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXProcess members which create, signal and wait for native processes; the constructor and id() stay in FXProcess.cpp.
*/

#define TOPIC_CONSTRUCT 1000

using namespace FX;

/*******************************************************************************/

namespace FX {

// Start subprocess
FXbool FXProcess::start(const FXchar* exec,const FXchar *const *args,const FXchar *const *env){
  FXbool result=false;
  if(pid==0 && exec && args){
    struct stat status;
    if(::stat(exec,&status)==0){
      FXProcessID hnd=::fork();
      if(0<=hnd){
        if(hnd==0){

          // Stdin was redirected
          if(input && input->isOpen()){
            dup2(input->handle(),STDIN_FILENO);
            }

          // Stdout was redirected
          if(output && output->isOpen()){
            dup2(output->handle(),STDOUT_FILENO);
            }

          // Stderr was redirected
          if(errors && errors->isOpen()){
            dup2(errors->handle(),STDERR_FILENO);
            }

          // Close all other handles
          FXint fd=::sysconf(_SC_OPEN_MAX);
          while(--fd>STDERR_FILENO){
            close(fd);
            }

          //setsid();

          // Kick off with arguments and environment
          if(env){
            ::execve(exec,const_cast<char* const*>(args),const_cast<char* const*>(env));
            }

          // Kick off with just arguments
          else{
            ::execv(exec,const_cast<char* const*>(args));
            }

          // Failed to kick off child
          fxwarning("failed to exec: %s\n",exec);

          // Child exits
          FXProcess::exit(-1);
          }
        pid=hnd;
        result=true;
        }
      }
    }
  return result;
  }


// Get process id
FXint FXProcess::current(){
  return (FXint)::getpid();
  }


// Get current process handle
FXInputHandle FXProcess::handle(){
  return ::getpid();
  }


// Exit calling process
void FXProcess::exit(FXint code){
  ::exit(code);
  }


// Suspend process
FXbool FXProcess::suspend(){
  if(pid){
    return ::kill(pid,SIGSTOP)==0;
    }
  return false;
  }


// Resume process
FXbool FXProcess::resume(){
  if(pid){
    return ::kill(pid,SIGCONT)==0;
    }
  return false;
  }


// Kill process
FXbool FXProcess::kill(){
  if(pid){
    return ::kill(pid,SIGKILL)==0;
    }
  return false;
  }


// Wait for child process
FXbool FXProcess::wait(){
  FXbool result=false;
  if(pid){
    FXint code;
    if(0<waitpid(pid,&code,0)){
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
    if(0<waitpid(pid,&code,0)){
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
    //// Zombie ////
    }
  }

}

#endif
