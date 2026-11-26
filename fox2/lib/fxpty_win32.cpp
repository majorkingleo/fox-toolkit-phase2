/********************************************************************************
*                                                                               *
*                    P s e u d o   T e r m i n a l   ( W i n 3 2 )              *
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

#if defined(WIN32)

#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE (0x00020016)
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

typedef HRESULT (WINAPI *PFNCREATEPSEUDOCONSOLE)(COORD,HANDLE,HANDLE,DWORD,HPCON*);
typedef HRESULT (WINAPI *PFNRESIZEPSEUDOCONSOLE)(HPCON,COORD);
typedef void (WINAPI *PFNCLOSEPSEUDOCONSOLE)(HPCON);


// Platform state; no OS type crosses the seam header
struct FXPtyPrivate {
  HPCON       hpc;                     // Pseudo console
  HANDLE      inputwrite;              // Input pipe write side
  HANDLE      outputread;              // Output pipe read side
  HANDLE      process;                 // Child process
  HANDLE      job;                     // Kill-on-close job
  HANDLE      wake;                    // Manual reset, set when output changes
  HANDLE      inputevent;             // Auto reset, wakes the writer
  HANDLE      reader;
  HANDLE      writer;
  DWORD       pid;
  CRITICAL_SECTION lock;
  std::string output;
  std::string input;
  FXbool      eof;
  FXbool      stop;
  FXint       exitcode;
  PFNCLOSEPSEUDOCONSOLE closeconsole;
  PFNRESIZEPSEUDOCONSOLE resizeconsole;
  FXPtyPrivate():hpc(nullptr),inputwrite(nullptr),outputread(nullptr),process(nullptr),job(nullptr),wake(nullptr),inputevent(nullptr),reader(nullptr),writer(nullptr),pid(0),eof(false),stop(false),exitcode(-1),closeconsole(nullptr),resizeconsole(nullptr){
    InitializeCriticalSection(&lock);
    }
  ~FXPtyPrivate(){
    DeleteCriticalSection(&lock);
    }
  };


// Convert UTF-8 to a wide string
static std::wstring utf8ToWide(const FXString& text){
  std::wstring result;
  FXint length;
  if(text.empty()) return result;
  length=MultiByteToWideChar(CP_UTF8,0,text.text(),text.length(),nullptr,0);
  if(length<=0) return result;
  result.resize(length);
  MultiByteToWideChar(CP_UTF8,0,text.text(),text.length(),&result[0],length);
  return result;
  }


// Quote one Windows command line argument
static std::wstring quoteArgument(const std::wstring& text){
  std::wstring result;
  FXint backslashes=0;
  FXint i;
  bool quote=false;
  for(i=0; i<(FXint)text.length(); i++){
    if(text[i]=='\\'){
      backslashes++;
      continue;
      }
    if(text[i]=='"'){
      result.append(backslashes*2+1,'\\');
      result+=L'"';
      backslashes=0;
      quote=true;
      continue;
      }
    result.append(backslashes,'\\');
    backslashes=0;
    result+=text[i];
    }
  result.append(backslashes*2,'\\');
  if(quote || text.find(L' ' )!=std::wstring::npos || text.find(L'\t')!=std::wstring::npos){
    return L"\""+result+L"\"";
    }
  return result;
  }


// Resolve a function pointer without a function-pointer cast warning
template<typename T> static T loadFunction(HMODULE module,const FXchar* name){
  union {
    FARPROC proc;
    T func;
    } u;
  u.proc=GetProcAddress(module,name);
  return u.func;
  }


// Format a Windows error
static FXString formatError(const FXchar* what,DWORD code){
  FXString result;
  result.format("%s: error %lu",what,(unsigned long)code);
  return result;
  }


// Build the environment block for the child
static std::vector<wchar_t> buildEnvironment(const FXTerminalLaunch& launch,const FXString& term,FXint columns,FXint rows){
  std::vector<std::wstring> entries;
  std::vector<wchar_t> block;
  std::wstring setterm=std::wstring(L"TERM=")+utf8ToWide(term);
  std::wstring setcolumns=L"COLUMNS="+std::to_wstring(columns);
  std::wstring setlines=L"LINES="+std::to_wstring(rows);
  std::wstring setprogram=L"TERM_PROGRAM=foxterm";
  std::wstring setversion=L"TERM_PROGRAM_VERSION="+std::to_wstring(FOX_MAJOR)+L"."+std::to_wstring(FOX_MINOR)+L"."+std::to_wstring(FOX_LEVEL);
  LPWSTR source=GetEnvironmentStringsW();
  LPWSTR entry;
  if(source){
    for(entry=source; *entry; entry+=wcslen(entry)+1){
      std::wstring item(entry);
      if(item.compare(0,5,L"TERM=")==0) continue;
      if(item.compare(0,10,L"COLORTERM=")==0) continue;
      if(item.compare(0,13,L"TERM_PROGRAM=")==0) continue;
      if(item.compare(0,21,L"TERM_PROGRAM_VERSION=")==0) continue;
      if(item.compare(0,8,L"COLUMNS=")==0) continue;
      if(item.compare(0,6,L"LINES=")==0) continue;
      if(item.compare(0,4,L"PWD=")==0) continue;
      entries.push_back(item);
      }
    FreeEnvironmentStringsW(source);
    }
  entries.push_back(setterm);
  entries.push_back(setcolumns);
  entries.push_back(setlines);
  entries.push_back(setprogram);
  entries.push_back(setversion);
  if(launch.kind==TERMINAL_XTERM) entries.push_back(L"COLORTERM=truecolor");
  if(!launch.directory.empty()) entries.push_back(L"PWD="+utf8ToWide(launch.directory));
  std::sort(entries.begin(),entries.end(),[](const std::wstring& a,const std::wstring& b){ return _wcsicmp(a.c_str(),b.c_str())<0; });
  for(size_t i=0; i<entries.size(); i++){
    block.insert(block.end(),entries[i].begin(),entries[i].end());
    block.push_back(0);
    }
  block.push_back(0);
  return block;
  }


// Reader thread: move ConPTY output into the queue and wake the UI
static DWORD WINAPI readerThread(LPVOID data){
  FXPtyPrivate *priv=(FXPtyPrivate*)data;
  FXchar buffer[8192];
  DWORD count;
  for(;;){
    if(!ReadFile(priv->outputread,buffer,sizeof(buffer),&count,nullptr) || count==0) break;
    EnterCriticalSection(&priv->lock);
    while(priv->output.size()>8*1024*1024 && !priv->stop){
      LeaveCriticalSection(&priv->lock);
      Sleep(5);
      EnterCriticalSection(&priv->lock);
      }
    priv->output.append(buffer,count);
    SetEvent(priv->wake);
    LeaveCriticalSection(&priv->lock);
    }
  EnterCriticalSection(&priv->lock);
  priv->eof=true;
  SetEvent(priv->wake);
  LeaveCriticalSection(&priv->lock);
  return 0;
  }


// Writer thread: drain the input queue to ConPTY
static DWORD WINAPI writerThread(LPVOID data){
  FXPtyPrivate *priv=(FXPtyPrivate*)data;
  std::string chunk;
  DWORD written;
  for(;;){
    WaitForSingleObject(priv->inputevent,INFINITE);
    EnterCriticalSection(&priv->lock);
    if(priv->input.empty() && priv->stop){
      LeaveCriticalSection(&priv->lock);
      break;
      }
    chunk.swap(priv->input);
    LeaveCriticalSection(&priv->lock);
    if(!chunk.empty()){
      if(!WriteFile(priv->inputwrite,chunk.data(),(DWORD)chunk.size(),&written,nullptr) || written!=chunk.size()){
        EnterCriticalSection(&priv->lock);
        priv->stop=true;
        SetEvent(priv->wake);
        LeaveCriticalSection(&priv->lock);
        break;
        }
      }
    }
  return 0;
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


// Release all native resources
void FXPty::close(){
  EnterCriticalSection(&priv->lock);
  priv->stop=true;
  priv->eof=true;
  LeaveCriticalSection(&priv->lock);
  if(priv->closeconsole && priv->hpc){
    priv->closeconsole(priv->hpc);
    priv->hpc=nullptr;
    }
  if(priv->job){
    TerminateJobObject(priv->job,1);
    CloseHandle(priv->job);
    priv->job=nullptr;
    }
  if(priv->inputevent) SetEvent(priv->inputevent);
  if(priv->reader){
    if(WaitForSingleObject(priv->reader,500)!=WAIT_OBJECT_0){
      CancelSynchronousIo(priv->reader);
      WaitForSingleObject(priv->reader,500);
      }
    CloseHandle(priv->reader);
    priv->reader=nullptr;
    }
  if(priv->writer){
    SetEvent(priv->inputevent);
    WaitForSingleObject(priv->writer,500);
    CloseHandle(priv->writer);
    priv->writer=nullptr;
    }
  if(priv->inputwrite){ CloseHandle(priv->inputwrite); priv->inputwrite=nullptr; }
  if(priv->outputread){ CloseHandle(priv->outputread); priv->outputread=nullptr; }
  if(priv->process){
    WaitForSingleObject(priv->process,1000);
    CloseHandle(priv->process);
    priv->process=nullptr;
    }
  if(priv->wake){ CloseHandle(priv->wake); priv->wake=nullptr; }
  if(priv->inputevent){ CloseHandle(priv->inputevent); priv->inputevent=nullptr; }
  EnterCriticalSection(&priv->lock);
  priv->output.clear();
  priv->input.clear();
  priv->pid=0;
  priv->exitcode=-1;
  LeaveCriticalSection(&priv->lock);
  priv->closeconsole=nullptr;
  priv->resizeconsole=nullptr;
  }


// Start a child on ConPTY
FXbool FXPty::start(const FXTerminalLaunch& launch,FXString& error){
  HMODULE kernel;
  PFNCREATEPSEUDOCONSOLE createconsole;
  PFNRESIZEPSEUDOCONSOLE resizeconsole;
  PFNCLOSEPSEUDOCONSOLE closeconsole;
  HANDLE inputread=nullptr;
  HANDLE outputwrite=nullptr;
  STARTUPINFOEXW si;
  PROCESS_INFORMATION pi;
  LPPROC_THREAD_ATTRIBUTE_LIST attributes=nullptr;
  SIZE_T attributesize=0;
  FXbool attributesready=false;
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
  COORD size;
  std::wstring shell;
  std::wstring command;
  std::wstring directory;
  std::wstring args;
  std::vector<wchar_t> environment;
  FXString term=launch.term;
  FXint columns=launch.columns>0 ? launch.columns : 80;
  FXint rows=launch.rows>0 ? launch.rows : 24;
  ZeroMemory(&pi,sizeof(pi));
  close();
  kernel=GetModuleHandleW(L"kernel32.dll");
  createconsole=kernel ? loadFunction<PFNCREATEPSEUDOCONSOLE>(kernel,"CreatePseudoConsole") : nullptr;
  resizeconsole=kernel ? loadFunction<PFNRESIZEPSEUDOCONSOLE>(kernel,"ResizePseudoConsole") : nullptr;
  closeconsole=kernel ? loadFunction<PFNCLOSEPSEUDOCONSOLE>(kernel,"ClosePseudoConsole") : nullptr;
  if(!createconsole || !resizeconsole || !closeconsole){
    error=FXString("ConPTY is not available; Windows 10 1809 or newer is required");
    return false;
    }
  if(term.empty()) term=(launch.kind==TERMINAL_VT100) ? "vt100" : "xterm-256color";
  if(launch.shell.empty()){
    wchar_t buffer[MAX_PATH];
    if(GetEnvironmentVariableW(L"COMSPEC",buffer,MAX_PATH)>0) shell=buffer;
    else shell=L"cmd.exe";
    }
  else{
    shell=utf8ToWide(launch.shell);
    }
  args=utf8ToWide(launch.arguments);
  command=quoteArgument(shell);
  if(!args.empty()){
    command+=L' ';
    command+=args;
    }
  directory=utf8ToWide(launch.directory);
  size.X=(SHORT)columns;
  size.Y=(SHORT)rows;
  if(!CreatePipe(&inputread,&priv->inputwrite,nullptr,0)) goto fail;
  if(!CreatePipe(&priv->outputread,&outputwrite,nullptr,0)) goto fail;
  if(FAILED(createconsole(size,inputread,outputwrite,0,&priv->hpc))) goto fail;
  priv->closeconsole=closeconsole;
  priv->resizeconsole=resizeconsole;
  CloseHandle(inputread);
  inputread=nullptr;
  CloseHandle(outputwrite);
  outputwrite=nullptr;
  ZeroMemory(&si,sizeof(si));
  si.StartupInfo.cb=sizeof(si);
  InitializeProcThreadAttributeList(nullptr,1,0,&attributesize);
  attributes=(LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(),0,attributesize);
  if(!attributes) goto fail;
  if(!InitializeProcThreadAttributeList(attributes,1,0,&attributesize)){
    HeapFree(GetProcessHeap(),0,attributes);
    attributes=nullptr;
    goto fail;
    }
  attributesready=true;
  if(!UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,priv->hpc,sizeof(HPCON),nullptr,nullptr)) goto fail;
  si.lpAttributeList=attributes;
  environment=buildEnvironment(launch,term,columns,rows);
  ZeroMemory(&pi,sizeof(pi));
  if(!CreateProcessW(nullptr,&command[0],nullptr,nullptr,FALSE,EXTENDED_STARTUPINFO_PRESENT|CREATE_UNICODE_ENVIRONMENT,environment.data(),directory.empty()?nullptr:&directory[0],&si.StartupInfo,&pi)) goto fail;
  if(attributesready) DeleteProcThreadAttributeList(attributes);
  HeapFree(GetProcessHeap(),0,attributes);
  attributes=nullptr;
  attributesready=false;
  CloseHandle(pi.hThread);
  pi.hThread=nullptr;
  priv->process=pi.hProcess;
  pi.hProcess=nullptr;
  priv->pid=pi.dwProcessId;
  priv->exitcode=-1;
  priv->eof=false;
  priv->stop=false;
  priv->closeconsole=closeconsole;
  priv->job=CreateJobObjectW(nullptr,nullptr);
  if(priv->job){
    ZeroMemory(&limits,sizeof(limits));
    limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(priv->job,JobObjectExtendedLimitInformation,&limits,sizeof(limits));
    AssignProcessToJobObject(priv->job,priv->process);
    }
  priv->wake=CreateEventW(nullptr,TRUE,FALSE,nullptr);
  priv->inputevent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
  if(!priv->wake || !priv->inputevent) goto fail;
  priv->reader=CreateThread(nullptr,0,readerThread,priv,0,nullptr);
  priv->writer=CreateThread(nullptr,0,writerThread,priv,0,nullptr);
  if(!priv->reader || !priv->writer) goto fail;
  error.clear();
  return true;
fail:
  {
  DWORD code=GetLastError();
  if(attributes){
    if(attributesready) DeleteProcThreadAttributeList(attributes);
    HeapFree(GetProcessHeap(),0,attributes);
    }
  if(inputread) CloseHandle(inputread);
  if(outputwrite) CloseHandle(outputwrite);
  if(pi.hThread) CloseHandle(pi.hThread);
  if(pi.hProcess) CloseHandle(pi.hProcess);
  error=formatError("ConPTY start failed",code);
  close();
  return false;
  }
  }


// Return true while the child is running
FXbool FXPty::isRunning(){
  if(!priv->process) return false;
  return WaitForSingleObject(priv->process,0)==WAIT_TIMEOUT;
  }


// Return the child exit code
FXint FXPty::getExitCode(){
  DWORD code;
  if(!priv->process) return -1;
  if(!GetExitCodeProcess(priv->process,&code)) return -1;
  if(code==STILL_ACTIVE) return -1;
  priv->exitcode=(FXint)code;
  return priv->exitcode;
  }


// Return the child process id
FXint FXPty::getProcessID() const {
  return (FXint)priv->pid;
  }


// Return the notification event
FXInputHandle FXPty::getWakeHandle() const {
  return (FXInputHandle)priv->wake;
  }


// Clear the notification when there is no output left
void FXPty::clearWake(){
  EnterCriticalSection(&priv->lock);
  if(priv->output.empty()) ResetEvent(priv->wake);
  LeaveCriticalSection(&priv->lock);
  }


// Read queued output
FXival FXPty::read(FXchar* buffer,FXival size){
  FXival count;
  if(!buffer || size<=0) return -1;
  EnterCriticalSection(&priv->lock);
  if(!priv->output.empty()){
    count=FXMIN(size,(FXival)priv->output.size());
    memcpy(buffer,priv->output.data(),(size_t)count);
    priv->output.erase(0,(size_t)count);
    if(priv->output.empty()) ResetEvent(priv->wake);
    LeaveCriticalSection(&priv->lock);
    return count;
    }
  if(priv->eof){
    LeaveCriticalSection(&priv->lock);
    return -1;
    }
  LeaveCriticalSection(&priv->lock);
  return 0;
  }


// Queue input for the writer thread
FXival FXPty::write(const FXchar* buffer,FXival size){
  if(!buffer || size<=0) return -1;
  EnterCriticalSection(&priv->lock);
  if(priv->stop || !priv->process || priv->input.size()+(size_t)size>1024*1024){
    LeaveCriticalSection(&priv->lock);
    return -1;
    }
  priv->input.append(buffer,(size_t)size);
  LeaveCriticalSection(&priv->lock);
  SetEvent(priv->inputevent);
  return size;
  }


// The writer thread owns the actual write
FXbool FXPty::flushWrites(){
  return true;
  }


// Return true if input is queued
FXbool FXPty::hasPendingWrites() const {
  FXbool pending;
  EnterCriticalSection(&priv->lock);
  pending=!priv->input.empty();
  LeaveCriticalSection(&priv->lock);
  return pending;
  }


// Resize the pseudo console
void FXPty::resize(FXint columns,FXint rows){
  COORD size;
  if(!priv->hpc) return;
  size.X=(SHORT)FXMAX(1,columns);
  size.Y=(SHORT)FXMAX(1,rows);
  if(priv->hpc && priv->resizeconsole) priv->resizeconsole(priv->hpc,size);
  }


// Terminate the child
void FXPty::terminate(FXbool force){
  EnterCriticalSection(&priv->lock);
  priv->stop=true;
  LeaveCriticalSection(&priv->lock);
  if(priv->job) TerminateJobObject(priv->job,force ? 1 : 0);
  else if(priv->process) TerminateProcess(priv->process,force ? 1 : 0);
  if(priv->inputevent) SetEvent(priv->inputevent);
  }


}

#endif
