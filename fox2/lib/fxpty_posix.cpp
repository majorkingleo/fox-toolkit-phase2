/********************************************************************************
*                                                                               *
*                    P s e u d o   T e r m i n a l   ( P O S I X )              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#include "FXString.h"
#include "FXTerminal.h"
#include "fxpty.h"

#include <vector>

#if !defined(WIN32)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

using namespace FX;

/*******************************************************************************/

namespace FX {

// Platform state; no OS type crosses the seam header
struct FXPtyPrivate {
  FXint    master;                      // PTY master
  FXint    pid;                         // Child process
  FXint    exitcode;                    // Exit code, -1 while running
  FXbool   running;                     // Child is running
  FXString pending;                     // Input that did not fit in the PTY
  FXPtyPrivate():master(-1),pid(-1),exitcode(-1),running(false){ }
  };


// Split a command line using POSIX-like quote rules
static void splitArguments(const FXString& text,std::vector<FXString>& arguments){
  FXString current;
  FXchar quote=0;
  FXchar ch;
  FXbool started=false;
  FXint i;
  for(i=0; i<text.length(); i++){
    ch=text[i];
    if(quote==0 && (ch==' ' || ch=='\t')){
      if(started){
        arguments.push_back(current);
        current.clear();
        started=false;
        }
      continue;
      }
    if(quote==0 && (ch=='\'' || ch=='"')){
      quote=ch;
      started=true;
      continue;
      }
    if(quote==ch && quote!=0){
      quote=0;
      started=true;
      continue;
      }
    if(ch=='\\' && quote!='\'' && i+1<text.length()){
      current+=text[++i];
      started=true;
      continue;
      }
    current+=ch;
    started=true;
    }
  if(started) arguments.push_back(current);
  }


// Reap the child if it has exited
static void reapChild(FXPtyPrivate *priv){
  FXint status;
  pid_t result;
  if(!priv || priv->pid<=0) return;
  result=waitpid(priv->pid,&status,WNOHANG);
  if(result==priv->pid){
    priv->exitcode=WIFEXITED(status) ? WEXITSTATUS(status) : 128+WTERMSIG(status);
    priv->running=false;
    }
  }


// Construct
FXPty::FXPty(){
  priv=new FXPtyPrivate();
  }


// Destructor
FXPty::~FXPty(){
  close();
  delete priv;
  }


// Start a child on a new pseudo terminal
FXbool FXPty::start(const FXTerminalLaunch& launch,FXString& error){
  std::vector<FXString> arguments;
  std::vector<FXchar*> argv;
  FXString shell=launch.shell;
  FXString term=launch.term;
  FXString version;
  FXString text;
  pid_t pid;
  FXint master;
  FXint slave;
  FXint columns=launch.columns>0 ? launch.columns : 80;
  FXint rows=launch.rows>0 ? launch.rows : 24;
  FXchar *name;
  struct winsize size;
  FXint i;
  close();
  if(shell.empty()){
    const FXchar* envshell=getenv("SHELL");
    shell=(envshell && envshell[0]) ? envshell : "/bin/sh";
    }
  if(term.empty()) term=(launch.kind==TERMINAL_VT100) ? "vt100" : "xterm-256color";
  splitArguments(launch.arguments,arguments);
  argv.push_back(const_cast<FXchar*>(shell.text()));
  for(i=0; i<(FXint)arguments.size(); i++) argv.push_back(const_cast<FXchar*>(arguments[i].text()));
  argv.push_back(nullptr);
  master=posix_openpt(O_RDWR|O_NOCTTY);
  if(master<0){
    error.format("posix_openpt: %s",strerror(errno));
    return false;
    }
  if(grantpt(master)<0){
    error.format("grantpt: %s",strerror(errno));
    ::close(master);
    return false;
    }
  if(unlockpt(master)<0){
    error.format("unlockpt: %s",strerror(errno));
    ::close(master);
    return false;
    }
  name=ptsname(master);
  if(!name){
    error.format("ptsname: %s",strerror(errno));
    ::close(master);
    return false;
    }
  memset(&size,0,sizeof(size));
  size.ws_col=(unsigned short)columns;
  size.ws_row=(unsigned short)rows;
  ioctl(master,TIOCSWINSZ,&size);
  pid=fork();
  if(pid<0){
    error.format("fork: %s",strerror(errno));
    ::close(master);
    return false;
    }
  if(pid==0){
    setsid();
    slave=open(name,O_RDWR);
    if(slave<0) _exit(126);
    ioctl(slave,TIOCSCTTY,0);
    dup2(slave,0);
    dup2(slave,1);
    dup2(slave,2);
    if(slave>2) ::close(slave);
    ::close(master);
    if(!launch.directory.empty()){
      if(chdir(launch.directory.text())<0){
        fprintf(stderr,"foxterm: cannot change directory: %s\n",strerror(errno));
        _exit(126);
        }
      setenv("PWD",launch.directory.text(),1);
      }
    version.format("%d.%d.%d",FOX_MAJOR,FOX_MINOR,FOX_LEVEL);
    text.format("%d",columns);
    setenv("COLUMNS",text.text(),1);
    text.format("%d",rows);
    setenv("LINES",text.text(),1);
    setenv("TERM",term.text(),1);
    setenv("TERM_PROGRAM","foxterm",1);
    setenv("TERM_PROGRAM_VERSION",version.text(),1);
    if(launch.kind==TERMINAL_XTERM) setenv("COLORTERM","truecolor",1);
    else unsetenv("COLORTERM");
    execvp(shell.text(),argv.data());
    fprintf(stderr,"foxterm: cannot run %s: %s\n",shell.text(),strerror(errno));
    _exit(127);
    }
  fcntl(master,F_SETFL,fcntl(master,F_GETFL,0)|O_NONBLOCK);
  priv->master=master;
  priv->pid=(FXint)pid;
  priv->exitcode=-1;
  priv->running=true;
  priv->pending.clear();
  error.clear();
  return true;
  }


// Reap without blocking and report whether the child is still running
FXbool FXPty::isRunning(){
  reapChild(priv);
  return priv->running;
  }


// Return the exit code
FXint FXPty::getExitCode(){
  reapChild(priv);
  return priv->exitcode;
  }


// Return the process id
FXint FXPty::getProcessID() const {
  return priv->pid;
  }


// Return the notification handle
FXInputHandle FXPty::getWakeHandle() const {
  return (FXInputHandle)priv->master;
  }


// No extra notification state on POSIX
void FXPty::clearWake(){
  }


// Read available output
FXival FXPty::read(FXchar* buffer,FXival size){
  ssize_t result;
  if(!buffer || size<=0 || priv->master<0) return -1;
  do{
    result=::read(priv->master,buffer,size);
    }
  while(result<0 && errno==EINTR);
  if(result>0) return (FXival)result;
  if(result==0){
    reapChild(priv);
    return -1;
    }
  if(errno==EAGAIN || errno==EWOULDBLOCK) return 0;
  reapChild(priv);
  return -1;
  }


// Queue input and try to write it
FXival FXPty::write(const FXchar* buffer,FXival size){
  FXival done;
  FXival result;
  if(!buffer || size<=0 || priv->master<0) return -1;
  if(priv->pending.length()+size>1024*1024) return -1;
  priv->pending.append(buffer,(FXint)size);
  done=0;
  while(done<priv->pending.length()){
    result=::write(priv->master,priv->pending.text()+done,priv->pending.length()-done);
    if(result<0){
      if(errno==EINTR) continue;
      if(errno==EAGAIN || errno==EWOULDBLOCK) break;
      return -1;
      }
    if(result==0) break;
    done+=result;
    }
  if(done>0) priv->pending.erase(0,(FXint)done);
  return size;
  }


// Flush pending input
FXbool FXPty::flushWrites(){
  FXival done=0;
  FXival result;
  while(done<priv->pending.length() && priv->master>=0){
    result=::write(priv->master,priv->pending.text()+done,priv->pending.length()-done);
    if(result<0){
      if(errno==EINTR) continue;
      if(errno==EAGAIN || errno==EWOULDBLOCK) break;
      return false;
      }
    if(result==0) break;
    done+=result;
    }
  if(done>0) priv->pending.erase(0,(FXint)done);
  return priv->pending.empty();
  }


// Return true if input remains queued
FXbool FXPty::hasPendingWrites() const {
  return !priv->pending.empty();
  }


// Resize the child terminal
void FXPty::resize(FXint columns,FXint rows){
  struct winsize size;
  if(priv->master<0) return;
  memset(&size,0,sizeof(size));
  size.ws_col=(unsigned short)FXMAX(1,columns);
  size.ws_row=(unsigned short)FXMAX(1,rows);
  ioctl(priv->master,TIOCSWINSZ,&size);
  }


// Signal the child
void FXPty::terminate(FXbool force){
  if(priv->pid>0 && priv->running){
    kill(-priv->pid,force ? SIGKILL : SIGHUP);
    }
  }


// Close the PTY and release the child
void FXPty::close(){
  FXint i;
  if(priv->master>=0){
    ::close(priv->master);
    priv->master=-1;
    }
  if(priv->pid>0){
    if(priv->running) kill(-priv->pid,SIGHUP);
    reapChild(priv);
    for(i=0; i<3 && priv->running; i++){
      usleep(10000);
      reapChild(priv);
      }
    if(priv->running){
      kill(-priv->pid,SIGKILL);
      for(i=0; i<3 && priv->running; i++){
        usleep(10000);
        reapChild(priv);
        }
      }
    }
  priv->pending.clear();
  priv->running=false;
  }


}

#endif
