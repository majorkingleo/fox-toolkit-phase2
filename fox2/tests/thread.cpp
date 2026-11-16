/********************************************************************************
*                                                                               *
*                                Thread Pool Test                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"

#include <errno.h>

/*
  Notes:

  - Thread pool test.
  - With --test no keys are waited for; the producer runs a fixed number of jobs, the
    pool is stopped, and the number of jobs that ran is compared with the number
    queued.  Exit code is 0 only if all checks held.

*/

/*******************************************************************************/

static FXint failures=0;
static FXbool quiet=false;
static volatile FXint executed=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    failures++;
    printf("BAD: %s\n",what);
    }
  }


// Job runner
class Runner : public FXRunnable {
protected:
  FXdouble value;
  FXint    number;
  FXint    count;
public:
  Runner(FXint n,FXint c):value(1.0),number(n),count(c){}
  virtual FXint run();
  };


// Job producer
class Producer : public FXThread {
protected:
  FXThreadPool *pool;
  FXint         count;
  FXint         groups;
  FXint         queued;
public:
  Producer(FXThreadPool *p,FXint c,FXint g):pool(p),count(c),groups(g),queued(0){}
  FXint getQueued() const { return queued; }
  virtual FXint run();
  };


// Run jobs
FXint Runner::run(){
  if(!quiet) fxmessage("runner %d start\n",number);
  value=1.0;
  for(FXint i=0; i<count; i++){
    value=Math::cos(value);
    }
  if(!quiet) fxmessage("runner %d done\n",number);
  atomicAdd(&executed,1);
  delete this;
  return 1;
  }



// Generate jobs
FXint Producer::run(){
  FXint job=0;
  if(!quiet) fxmessage("producer start on cpus: %llb\n",affinity());
  FXRandom rng;
  for(FXint g=0; g<groups; ++g){
    for(FXint c=0; c<count; c++){
      if(!pool->execute(new Runner(job,quiet?1000:(rng.randLong()/4000000)/1000000))) goto x;
      if(!quiet) fxmessage("producer job %d\n",job);
      job++;
      queued++;
      }
    if(!quiet) fxmessage("producer waiting\n");
    pool->wait();
    if(!quiet) fxmessage("producer resumed\n");
    }
x:if(!quiet) fxmessage("producer done\n");
  return 1;
  }



// Run a fixed number of jobs on the pool and check that all of them ran
static void testpool(){
  FXuint cpus=FXThread::processors();
  FXThreadPool pool(256);
  Producer producer(&pool,20,5);
  FXuint started;
  quiet=true;
  started=pool.start(cpus);
  check(started==cpus,"pool starts one thread per processor");
  check(pool.getRunningThreads()>0,"pool has running threads");
  check(producer.start(),"producer thread starts");
  check(producer.join(),"producer thread ends");
  check(producer.getQueued()==100,"all 100 jobs were queued");
  check(pool.stop(),"pool stops");
  check(executed==producer.getQueued(),"every queued job ran");
  check(pool.getRunningThreads()==0,"no threads run after stop");
  check(!pool.stop(),"stopping a stopped pool fails");
  }


// Start
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FXuint cpus=FXThread::processors();
  int started;

  // Self-check without keyboard
  if(1<argc && strcmp(argv[1],"--test")==0){
    testpool();
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  // Trace
  setTraceLevel(150);

  // Make thread pool
  FXThreadPool pool(256);

  // Make producer thread
  Producer producer(&pool,100,10);

  fxmessage("Found %d processors\n",cpus);

  fxmessage("starting pool\n");
  started=pool.start(cpus);
  fxmessage("running: %d (%d)\n",pool.getRunningThreads(),started);
  getchar();
  fxmessage("started pool %d\n",started);
  getchar();

  fxmessage("running: %d\n",pool.getRunningThreads());
  fxmessage("starting jobs\n");
  producer.start();
  fxmessage("running jobs\n");
  fxmessage("running: %d\n",pool.getRunningThreads());

  getchar();
  fxmessage("stopping\n");
  pool.stop();
  fxmessage("running: %d\n",pool.getRunningThreads());
  fxmessage("stopped\n");

  getchar();
  producer.join();
  fxmessage("bye\n");
  return 0;
  }
