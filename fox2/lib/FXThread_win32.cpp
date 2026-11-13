/********************************************************************************
*                                                                               *
*                T h r e a d   F u n c t i o n s   ( W i n 3 2 )                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXException.h"
#include "FXString.h"
#include "FXRunnable.h"
#include "FXAutoThreadStorageKey.h"
#include "FXThread.h"

#if defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXThread members which create, control, query and time threads; the thread bookkeeping stays in FXThread.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Start the thread; we associate the FXThread instance with this thread using thread-local
// storage accessed with self_key.
// A special FXThreadException may be used to unroll to this point and return a specific
// error code from the thread; any other FXException is silently caught here, and causes
// FXThread to return error-code of -1.
struct FXThreadEntry {
  static unsigned int CALLBACK function(void* ptr);
  };

unsigned int CALLBACK FXThreadEntry::function(void* ptr){
  FXThread *thread=(FXThread*)ptr;
  FXint code=-1;
  FXThread::self(thread);
  try{
    code=thread->run();
    }
  catch(const FXThreadException& e){    // Graceful thread exit
    code=e.code();
    }
  catch(const FXException& e){          // Our kind of exceptions
    fxerror("FXThread: caught exception: %s.\n",e.what());
    }
  catch(...){                           // Other exceptions
    fxerror("FXThread: caught unknown exception.\n");
    }
  if(FXThread::self()){ FXThread::self()->busy=false; }
  return code;
  }


// Start thread
FXbool FXThread::start(FXuval stacksize){
  DWORD thd;
  if(busy){ fxerror("FXThread::start: thread already running.\n"); }
  if(tid){ fxerror("FXThread::start: thread still attached.\n"); }
  busy=true;
  if((tid=(FXThreadID)CreateThread(nullptr,stacksize,(LPTHREAD_START_ROUTINE)FXThreadEntry::function,this,0,&thd))==nullptr) busy=false;
//  if((tid=(FXThreadID)_beginthreadex(nullptr,stacksize,(LPTHREAD_START_ROUTINE)FXThreadEntry::function,this,0,&thd))==nullptr) busy=false;
  return busy;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(FXint& code){
  if(tid && WaitForSingleObject((HANDLE)tid,INFINITE)==WAIT_OBJECT_0){
    GetExitCodeThread((HANDLE)tid,(DWORD*)&code);
    CloseHandle((HANDLE)tid);
    tid=0;
    return true;
    }
  return false;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(){
  if(tid && WaitForSingleObject((HANDLE)tid,INFINITE)==WAIT_OBJECT_0){
    CloseHandle((HANDLE)tid);
    tid=0;
    return true;
    }
  return false;
  }


// Cancel the thread
FXbool FXThread::cancel(){
  if(tid){
    if(busy && TerminateThread((HANDLE)tid,0)) busy=false;
    if(CloseHandle((HANDLE)tid)){
      tid=0;
      return true;
      }
    }
  return false;
  }


// Detach thread
FXbool FXThread::detach(){
  if(tid && CloseHandle((HANDLE)tid)){
    tid=0;
    return true;
    }
  return false;
  }


// Exit calling thread
void FXThread::exit(FXint code){
  if(self()){ self()->busy=false; }
  ExitThread(code);
//  _endthreadex(code);
  }


// Yield the thread
void FXThread::yield(){
  Sleep(0);
  }


// Processor pause/back-off
void FXThread::pause(){
#if defined(_MSC_VER)
  YieldProcessor();
#endif
  }


// Get time in nanoseconds since Epoch
FXTime FXThread::time(){
  const FXTime BIAS=FXLONG(116444736000000000);
  const FXTime HIGH=FXLONG(4294967296);
  FILETIME now;
  GetSystemTimeAsFileTime(&now);
  return (now.dwHighDateTime*HIGH+now.dwLowDateTime-BIAS)*100;
  }


// Get steady time in nanoseconds since some arbitrary start time
FXTime FXThread::steadytime(){
  const FXTime seconds=1000000000;
  LARGE_INTEGER frq,clk;
  ::QueryPerformanceFrequency(&frq);
  ::QueryPerformanceCounter(&clk);
  FXASSERT(frq.QuadPart<FXLONG(9223372036));    // Overflow possible if CPU speed exceeds 9.2GHz
  FXTime s=clk.QuadPart/frq.QuadPart;
  FXTime f=clk.QuadPart%frq.QuadPart;
  return seconds*s+(seconds*f)/frq.QuadPart;
  }


// Return total process cpu time
FXTime FXThread::cputime(){
  FILETIME crt,ext,krt,ust;
  GetProcessTimes(GetCurrentProcess(),&crt,&ext,&krt,&ust);
  return (ust.dwHighDateTime*FXLONG(4294967296)+ust.dwLowDateTime)*FXLONG(100);
  }


// Return time in processor ticks.
FXTime FXThread::ticks(){
#if defined(_MSC_VER)
#if defined(_M_IX86) || defined(_M_X64)
  return __rdtsc();
#else
  FXTime value;
  FXSTATIC_ASSERT(sizeof(FXTime)==sizeof(LARGE_INTEGER));
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
#endif
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i386__)
  FXTime value;
  __asm__ __volatile__ ( "rdtsc" : "=A" (value) : : "memory");
  return value;
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__x86_64__)
  FXTime value;
  __asm__ __volatile__ ( "rdtsc              \n\t"
                         "salq	$32, %%rdx   \n\t"
                         "orq	%%rdx, %%rax \n\t" : "=q"(value) : : "memory");
  return value;
#elif (defined(__GNUC__) && defined(__aarch64__))
   FXTime value;
   __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(value) : : "memory");
   return value;
#elif defined(__GNUC__) && (defined(__powerpc64__) || defined(__ppc64__))
  FXTime value;
  __asm__ __volatile__("mfspr %0, 268" : "=r"(value) : : "memory");
  return value;
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
  FXuint tbl,tbu0,tbu1;
  __asm__ __volatile__( "mftbu %0 \n\t"
                        "mftb  %1 \n\t"
                        "mftbu %2 \n\t" : "=r"(tbu0), "=r"(tbl), "=r"(tbu1) : : "memory");
  tbl&=-(FXint)(tbu0==tbu1);
  return (((FXTime)tbu1) << 32) | tbl;
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);   // Monotonic: no jumps!
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
  }


// We want to use NtDelayExecution() because it allows for both absolute as well as
// interval wait times. In addition, it is more accurate than the other system calls.
// Unfortunately, this is an undocumented NTDLL API; we can use it anyway by using
// GetProcAddress() on the NTDLL module to dig up its location.

// Typedef a pointer to NtDelayExecution()
typedef DWORD (WINAPI *PFN_NTDELAYEXECUTION)(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval);

// Declare the stub function
static DWORD WINAPI NtDelayExecutionStub(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval);

// Pointer to NtDelayExecution, initially pointing to the stub function
static PFN_NTDELAYEXECUTION fxNtDelayExecution=NtDelayExecutionStub;

// The stub gets the address of actual function, sets the function pointer, then calls
// actual function; next time around actual function will be called directly.
static DWORD WINAPI NtDelayExecutionStub(BOOLEAN Alertable,LARGE_INTEGER* DelayInterval){
  if(fxNtDelayExecution==NtDelayExecutionStub){
    HMODULE ntdllDll=GetModuleHandleA("ntdll.dll");
    FXASSERT(ntdllDll);
    fxNtDelayExecution=(PFN_NTDELAYEXECUTION)(void*)GetProcAddress(ntdllDll,"NtDelayExecution");
    FXASSERT(fxNtDelayExecution);
    }
  return fxNtDelayExecution(Alertable,DelayInterval);
  }


// Make the calling thread sleep for a number of nanoseconds
void FXThread::sleep(FXTime nsec){
  if(100<=nsec){
    LARGE_INTEGER jiffies;
    jiffies.QuadPart=-nsec/100;
    fxNtDelayExecution((BOOLEAN)false,&jiffies);
    }
  }


// Wake at appointed absolute time
void FXThread::wakeat(FXTime nsec){
  LARGE_INTEGER jiffies;
  jiffies.QuadPart=FXLONG(116444736000000000)+nsec/100;
  if(0<=jiffies.QuadPart){
    fxNtDelayExecution((BOOLEAN)false,&jiffies);
    }
  }


// Return thread id of caller
FXThreadID FXThread::current(){
  return (FXThreadID)GetCurrentThreadId();
  }


// Return number of processors
FXint FXThread::processors(){
  SYSTEM_INFO info={{0}};
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
  return 1;
  }


// Return processor index of the calling thread
FXint FXThread::processor(){
  if(1<processors()){
#if (WINVER >= 0x0600)                  // Vista and up
    return GetCurrentProcessorNumber();
#endif
#if defined(HAVE_SCHED_GETCPU)
    return sched_getcpu();
#endif
    return -1;
    }
  return 0;
  }


// Generate new thread local storage key
FXThreadStorageKey FXThread::createStorageKey(){
  return (FXThreadStorageKey)TlsAlloc();
  }


// Dispose of thread local storage key
void FXThread::deleteStorageKey(FXThreadStorageKey key){
  TlsFree((DWORD)key);
  }


// Get thread local storage pointer using key
void* FXThread::getStorage(FXThreadStorageKey key){
  return TlsGetValue((DWORD)key);
  }


// Set thread local storage pointer using key
void FXThread::setStorage(FXThreadStorageKey key,void* ptr){
  TlsSetValue((DWORD)key,ptr);
  }


// Set thread priority
FXbool FXThread::priority(FXThread::Priority prio){
  if(tid){
    int pri;
    switch(prio){
      case PriorityMinimum:
        pri=THREAD_PRIORITY_LOWEST;
        break;
      case PriorityLower:
        pri=THREAD_PRIORITY_BELOW_NORMAL;
        break;
      case PriorityMedium:
        pri=THREAD_PRIORITY_NORMAL;
        break;
      case PriorityHigher:
        pri=THREAD_PRIORITY_ABOVE_NORMAL;
        break;
      case PriorityMaximum:
        pri=THREAD_PRIORITY_HIGHEST;
        break;
      default:
        pri=THREAD_PRIORITY_NORMAL;
        break;
      }
    return SetThreadPriority((HANDLE)tid,pri)!=0;
    }
  return false;
  }


// Return thread priority
FXThread::Priority FXThread::priority() const {
  FXThread::Priority result=PriorityError;
  if(tid){
    int pri=GetThreadPriority((HANDLE)tid);
    if(pri!=THREAD_PRIORITY_ERROR_RETURN){
      switch(pri){
        case THREAD_PRIORITY_IDLE:
          result=PriorityMinimum;
          break;
        case THREAD_PRIORITY_BELOW_NORMAL:
          result=PriorityLower;
          break;
        case THREAD_PRIORITY_NORMAL:
          result=PriorityMedium;
          break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
          result=PriorityHigher;
          break;
        case THREAD_PRIORITY_HIGHEST:
          result=PriorityMaximum;
          break;
        default:
          result=PriorityDefault;
          break;
        }
      }
    }
  return result;
  }


// Set thread scheduling policy
FXbool FXThread::policy(FXThread::Policy plcy){
  return false;
  }


// Get thread scheduling policy
FXThread::Policy FXThread::policy() const {
  return PolicyError;
  }


// Change thread's processor affinity
FXbool FXThread::affinity(FXulong mask){
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    mask&=((bit<<ncpus)-1);
    if(mask){
      return SetThreadAffinityMask((HANDLE)tid,(FXuval)mask)!=0;
      }
    }
  return false;
  }


// Get thread's processor affinity
FXulong FXThread::affinity() const {
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    FXuval cpuset=SetThreadAffinityMask((HANDLE)tid,(FXuval)((bit<<ncpus)-1));
    if(cpuset){
      SetThreadAffinityMask((HANDLE)tid,cpuset);
      return (FXulong)cpuset;
      }
    }
  return 0;
  }


// Declare the function signatures
typedef HRESULT (WINAPI *PFN_SETTHREADDESCRIPTION)(HANDLE hThread,const WCHAR* desc);
typedef HRESULT (WINAPI *PFN_GETTHREADDESCRIPTION)(HANDLE hThread,WCHAR** desc);

// Declare the stub functions
static HRESULT WINAPI SetThreadDescriptionStub(HANDLE hThread,const WCHAR* desc);
static HRESULT WINAPI GetThreadDescriptionStub(HANDLE hThread,WCHAR** desc);

// Pointers to functions, initially pointing to the stub functions
static PFN_SETTHREADDESCRIPTION fxSetThreadDescription=SetThreadDescriptionStub;
static PFN_GETTHREADDESCRIPTION fxGetThreadDescription=GetThreadDescriptionStub;

// Set thread name (needs late-model Windows 10)
static HRESULT WINAPI SetThreadDescriptionStub(HANDLE hThread,const WCHAR* desc){
  if(fxSetThreadDescription==SetThreadDescriptionStub){
    HMODULE hnddll=GetModuleHandleA("kernel32.dll");
    fxSetThreadDescription=(PFN_SETTHREADDESCRIPTION)(void*)GetProcAddress(hnddll,"SetThreadDescription");
    }
  if(fxSetThreadDescription){
    return fxSetThreadDescription(hThread,desc);
    }
  return -1;
  }

// Get thread name (needs late-model Windows 10)
static HRESULT WINAPI GetThreadDescriptionStub(HANDLE hThread,WCHAR** desc){
  if(fxGetThreadDescription==GetThreadDescriptionStub){
    HMODULE hnddll=GetModuleHandleA("kernel32.dll");
    fxGetThreadDescription=(PFN_GETTHREADDESCRIPTION)(void*)GetProcAddress(hnddll,"GetThreadDescription");
    }
  if(fxGetThreadDescription){
    return fxGetThreadDescription(hThread,desc);
    }
  return -1;
  }


// Change thread description
FXbool FXThread::description(const FXString& desc){
  if(tid){
    FXnchar udesc[256];
    utf2ncs(udesc,desc.text(),ARRAYNUMBER(udesc));
    return 0<=fxSetThreadDescription((HANDLE)tid,udesc);
    }
  return false;
  }


// Return thread description
FXString FXThread::description() const {
  if(tid){
    FXnchar* udesc;
    if(0<=fxGetThreadDescription((HANDLE)tid,&udesc)){
      FXchar desc[256];
      ncs2utf(desc,udesc,ARRAYNUMBER(desc));
      ::LocalFree(udesc);
      return desc;
      }
    }
  return FXString::null;
  }


// Suspend thread
FXbool FXThread::suspend(){
  return tid && (SuspendThread((HANDLE)tid)!=(DWORD)-1L);
  }


// Resume thread
FXbool FXThread::resume(){
  return tid && (ResumeThread((HANDLE)tid)!=(DWORD)-1L);
  }

}

#endif
