/********************************************************************************
*                                                                               *
*                  T h r e a d   F u n c t i o n s   ( X 1 1 )                  *
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

#if !defined(WIN32)

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
void* FXThread::function(void* ptr){
  FXThread *thread=(FXThread*)ptr;
  FXint code=-1;
  self(thread);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,nullptr);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,nullptr);
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
  if(self()){ self()->busy=false; }
  return (void*)(FXival)code;
  }


// Start thread
FXbool FXThread::start(FXuval stacksize){
  pthread_attr_t attr;
  if(busy){ fxerror("FXThread::start: thread already running.\n"); }
  if(tid){ fxerror("FXThread::start: thread still attached.\n"); }
  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr,PTHREAD_INHERIT_SCHED);
  if(stacksize){ pthread_attr_setstacksize(&attr,stacksize); }
  busy=true;
#if defined(__USE_POSIX199506) || defined(__USE_UNIX98)
  sigset_t newset,oldset;
  sigfillset(&newset);
  pthread_sigmask(SIG_SETMASK,&newset,&oldset); // No signals except to main thread
  if(pthread_create((pthread_t*)&tid,&attr,FXThread::function,(void*)this)!=0) busy=false;
  pthread_sigmask(SIG_SETMASK,&oldset,nullptr);    // Restore old mask
#else
  if(pthread_create((pthread_t*)&tid,&attr,FXThread::function,(void*)this)!=0) busy=false;
#endif
  pthread_attr_destroy(&attr);
  return busy;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(FXint& code){
  void *trc=nullptr;
  if(tid && pthread_join((pthread_t)tid,&trc)==0){
    code=(FXint)(FXival)trc;
    tid=0;
    return true;
    }
  return false;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(){
  if(tid && pthread_join((pthread_t)tid,nullptr)==0){
    tid=0;
    return true;
    }
  return false;
  }


// Cancel the thread
FXbool FXThread::cancel(){
  if(tid){
    if(busy && pthread_cancel((pthread_t)tid)==0) busy=false;
    if(pthread_join((pthread_t)tid,nullptr)==0){
      tid=0;
      return true;
      }
    }
  return false;
  }


// Detach thread
FXbool FXThread::detach(){
  if(tid && pthread_detach((pthread_t)tid)==0){
    tid=0;
    return true;
    }
  return false;
  }


// Exit calling thread
void FXThread::exit(FXint code){
  if(self()){ self()->busy=false; }
  pthread_exit((void*)(FXival)code);
  }


// Yield the thread
void FXThread::yield(){
  sched_yield();                // More portable than pthread_yield()
  }


// Processor pause/back-off
void FXThread::pause(){
#if (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__("rep; nop\n" : : : "memory" );
//  _mm_pause();
#elif defined(__GNUC__) && defined(__aarch64__)
  __asm__ __volatile__("yield" ::: "memory");
#endif
  }


// Get time in nanoseconds since Epoch
FXTime FXThread::time(){
#if (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);            // Wallclock time!
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
  }


// Get steady time in nanoseconds since some arbitrary start time
FXTime FXThread::steadytime(){
#if (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);           // Monotonic: no jumps!
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,nullptr);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
  }


// Return total process cpu time
FXTime FXThread::cputime(){
#if (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  return 0;
#endif
  }


// Return time in processor ticks.
FXTime FXThread::ticks(){
#if (defined(__GNUC__) || defined(__INTEL_COMPILER)) && defined(__i386__)
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


// Make the calling thread sleep for a number of nanoseconds
void FXThread::sleep(FXTime nsec){
#if (_XOPEN_SOURCE >= 600) || (_POSIX_C_SOURCE >= 200112L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(clock_nanosleep(CLOCK_MONOTONIC,0,&value,&value)!=0){ }
    }
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(nanosleep(&value,&value)!=0){ }
    }
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  const FXTime milliseconds=1000000;
  struct timeval value;
  if(microseconds<=nsec){
    value.tv_usec=(nsec/microseconds)%milliseconds;
    value.tv_sec=nsec/seconds;
    select(0,nullptr,nullptr,nullptr,&value);
    }
#endif
  }


// Wake at appointed absolute time
void FXThread::wakeat(FXTime nsec){
#if (_XOPEN_SOURCE >= 600) || (_POSIX_C_SOURCE >= 200112L)
  const FXTime seconds=1000000000;
  struct timespec value;
  if(0<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&value,nullptr)!=0){ }
    }
#elif (_POSIX_C_SOURCE >= 199309L)
  const FXTime seconds=1000000000;
  struct timespec value;
  nsec-=FXThread::time();
  if(1<=nsec){
    value.tv_sec=nsec/seconds;
    value.tv_nsec=nsec%seconds;
    while(nanosleep(&value,&value)!=0){ }
    }
#else
  const FXTime seconds=1000000000;
  const FXTime microseconds=1000;
  const FXTime milliseconds=1000000;
  struct timeval value;
  nsec-=FXThread::time();
  if(microseconds<=nsec){
    value.tv_usec=(nsec/microseconds)%milliseconds;
    value.tv_sec=nsec/seconds;
    select(0,nullptr,nullptr,nullptr,&value);
    }
#endif
  }


// Return thread id of caller
FXThreadID FXThread::current(){
  return (FXThreadID)pthread_self();
  }


// Return number of processors
FXint FXThread::processors(){
#if defined(_SC_NPROCESSORS_ONLN)                             // Linux
  int result;
  if((result=sysconf(_SC_NPROCESSORS_ONLN))>0){
    return result;
    }
#elif defined(HAVE_SYS_SYSCTL_H)                                // FreeBSD/NetBSD/OpenBSD/MacOSX
  int mib[4];
  int result;
  size_t len;
  mib[0]=CTL_HW;
  mib[1]=HW_AVAILCPU;
  len=sizeof(result);
  if(sysctl(mib,2,&result,&len,nullptr,0)!=-1){
    return result;
    }
  mib[0]=CTL_HW;
  mib[1]=HW_NCPU;
  len=sizeof(result);
  if(sysctl(mib,2,&result,&len,nullptr,0)!=-1){
    return result;
    }
#elif defined(__IRIX__) && defined(_SC_NPROC_ONLN)              // IRIX
  int result;
  if((result=sysconf(_SC_NPROC_ONLN))>0){
    return result;
    }
#elif defined(hpux) || defined(__hpux) || defined(_hpux)        // HPUX
  struct pst_dynamic psd;
  if(!pstat_getdynamic(&psd,sizeof(psd),(size_t)1,0)){
    return (int)psd.psd_proc_cnt;
    }
#endif
  return 1;
  }


// Return processor index of the calling thread
FXint FXThread::processor(){
  if(1<processors()){
#if defined(HAVE_SCHED_GETCPU)
    return sched_getcpu();
#endif
    return -1;
    }
  return 0;
  }


// Generate new thread local storage key
FXThreadStorageKey FXThread::createStorageKey(){
  pthread_key_t key;
  return pthread_key_create(&key,nullptr)==0UL ? (FXThreadStorageKey)key : ~0UL;
  }


// Dispose of thread local storage key
void FXThread::deleteStorageKey(FXThreadStorageKey key){
  pthread_key_delete((pthread_key_t)key);
  }


// Get thread local storage pointer using key
void* FXThread::getStorage(FXThreadStorageKey key){
  return pthread_getspecific((pthread_key_t)key);
  }


// Set thread local storage pointer using key
void FXThread::setStorage(FXThreadStorageKey key,void* ptr){
  pthread_setspecific((pthread_key_t)key,ptr);
  }


// Set thread priority
FXbool FXThread::priority(FXThread::Priority prio){
#if defined(__APPLE__) || defined(__minix)
  return false;
#else
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
#if defined(_POSIX_PRIORITY_SCHEDULING)
      int priomax=sched_get_priority_max(plcy);         // Note that range may depend on scheduling policy!
      int priomin=sched_get_priority_min(plcy);
#elif defined(PTHREAD_MINPRIORITY) && defined(PTHREAD_MAX_PRIORITY)
      int priomin=PTHREAD_MIN_PRIORITY;
      int priomax=PTHREAD_MAX_PRIORITY;
#else
      int priomin=0;
      int priomax=20;
#endif
      if(priomax!=-1 && priomin!=-1){
        int priomed=(priomax+priomin)/2;
        switch(prio){
          case PriorityMinimum:
            sched.sched_priority=priomin;
            break;
          case PriorityLower:
            sched.sched_priority=(priomin+priomed)/2;
            break;
          case PriorityMedium:
            sched.sched_priority=priomed;
            break;
          case PriorityHigher:
            sched.sched_priority=(priomax+priomed)/2;
            break;
          case PriorityMaximum:
            sched.sched_priority=priomax;
            break;
          default:
            sched.sched_priority=priomed;
            break;
          }
        return pthread_setschedparam((pthread_t)tid,plcy,&sched)==0;
        }
      }
    }
  return false;
#endif
  }


// Return thread priority
FXThread::Priority FXThread::priority() const {
#if defined(__APPLE__) || defined(__minix)
  return PriorityError;
#else
  FXThread::Priority result=PriorityError;
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
#if defined(_POSIX_PRIORITY_SCHEDULING)
      int priomax=sched_get_priority_max(plcy);         // Note that range may depend on scheduling policy!
      int priomin=sched_get_priority_min(plcy);
#elif defined(PTHREAD_MINPRIORITY) && defined(PTHREAD_MAX_PRIORITY)
      int priomin=PTHREAD_MIN_PRIORITY;
      int priomax=PTHREAD_MAX_PRIORITY;
#else
      int priomin=0;
      int priomax=32;
#endif
      if(priomax!=-1 && priomin!=-1){
        int priomed=(priomax+priomin)/2;
        if(sched.sched_priority<priomed){
          if(sched.sched_priority<=priomin){
            result=PriorityMinimum;
            }
          else{
            result=PriorityLower;
            }
          }
        else if(sched.sched_priority>priomed){
          if(sched.sched_priority>=priomax){
            result=PriorityMaximum;
            }
          else{
            result=PriorityHigher;
            }
          }
        else{
          result=PriorityMedium;
          }
        }
      return result;
      }
    }
  return result;
#endif
  }


// Set thread scheduling policy
FXbool FXThread::policy(FXThread::Policy plcy){
#if defined(__APPLE__) || defined(__minix)
  return false;
#else
  if(tid){
    sched_param sched={0};
    int oldplcy=0;
    int newplcy=0;
    if(pthread_getschedparam((pthread_t)tid,&oldplcy,&sched)==0){
      switch(plcy){
        case PolicyFifo:
          newplcy=SCHED_FIFO;
          break;
        case PolicyRoundRobin:
          newplcy=SCHED_RR;
          break;
        default:
          newplcy=SCHED_OTHER;
          break;
        }
#if defined(_POSIX_PRIORITY_SCHEDULING)
      sched.sched_priority=sched_get_priority_min(newplcy);
#endif
      return pthread_setschedparam((pthread_t)tid,newplcy,&sched)==0;
      }
    }
  return false;
#endif
  }


// Get thread scheduling policy
FXThread::Policy FXThread::policy() const {
#if defined(__APPLE__) || defined(__minix)
  return PolicyError;
#else
  Policy result=PolicyError;
  if(tid){
    sched_param sched={0};
    int plcy=0;
    if(pthread_getschedparam((pthread_t)tid,&plcy,&sched)==0){
      switch(plcy){
        case SCHED_FIFO:
          result=PolicyFifo;
          break;
        case SCHED_RR:
          result=PolicyRoundRobin;
          break;
        default:
          result=PolicyDefault;
          break;
        }
      }
    }
  return result;
#endif
  }


// Change thread's processor affinity
FXbool FXThread::affinity(FXulong mask){
#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    mask&=((bit<<ncpus)-1);
    if(mask){
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      for(FXint cpu=0; cpu<ncpus; ++cpu){
        if((bit<<cpu)&mask){ CPU_SET(cpu,&cpuset); }
        }
      return pthread_setaffinity_np((pthread_t)tid,sizeof(cpuset),&cpuset)==0;
      }
    }
#endif
  return false;
  }


// Get thread's processor affinity
FXulong FXThread::affinity() const {
#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
  const FXulong bit=1;
  if(tid){
    FXint ncpus=processors();
    cpu_set_t cpuset;
    if(pthread_getaffinity_np((pthread_t)tid,sizeof(cpuset),&cpuset)==0){
      FXulong mask=0;
      for(FXint cpu=0; cpu<ncpus; ++cpu){
        if(CPU_ISSET(cpu,&cpuset)){ mask|=(bit<<cpu); }
        }
      return mask;
      }
    }
#endif
  return 0;
  }


// Change thread description
FXbool FXThread::description(const FXString& desc){
  if(tid){
#if defined(__APPLE__)
    return pthread_setname_np(desc.text())==0;
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    pthread_setname_np((pthread_t)tid,desc.text());
    return true;
#elif defined(HAVE_PTHREAD_SETNAME_NP)
    return pthread_setname_np((pthread_t)tid,desc.text())==0;
#endif
    }
  return false;
  }


// Return thread description
FXString FXThread::description() const {
  if(tid){
#if defined(__APPLE__)
    FXchar desc[256];
    if(pthread_getname_np(*((pthread_t*)&tid),desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    FXchar desc[256];
    if(pthread_getname_np((pthread_t)tid,desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#elif defined(HAVE_PTHREAD_GETNAME_NP)
    FXchar desc[256];
    if(pthread_getname_np((pthread_t)tid,desc,ARRAYNUMBER(desc))==0){
      return desc;
      }
#endif
    }
  return FXString::null;
  }


// Suspend thread
FXbool FXThread::suspend(){
#if defined(_HPUX_SOURCE)
  return tid && (pthread_suspend((pthread_t)tid)==0);
#elif defined(SUNOS)
  return tid && (thr_suspend((pthread_t)tid)==0);
#else
  return false;
#endif
  }


// Resume thread
FXbool FXThread::resume(){
#if defined(_HPUX_SOURCE)
  return tid && (pthread_resume_np((pthread_t)tid,PTHREAD_COUNT_RESUME_NP)==0);
#elif defined(SUNOS)
  return tid && (thr_continue((pthread_t)tid)==0);
#else
  return false;
#endif
  }

}

#endif
