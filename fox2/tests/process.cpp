/********************************************************************************
*                                                                               *
*                          P r o c e s s  -  T e s t                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2010,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"


/*
  Notes:

  - Test process creation.
  - With --test, the program starts itself as a child (--child, --sleep), and checks
    exit codes, captured output, the environment, kill and failure to start; the exit
    code is 0 only if all checks held.
  - Without arguments it starts an external editor, which only works on a machine
    where that is installed.

*/

/*******************************************************************************/



static FXint failures=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    failures++;
    printf("BAD: %s\n",what);
    }
  }


// Remove carriage returns, the Windows C library adds them to text output
static FXString unixtext(const FXString& text){
  FXString result;
  for(FXint i=0; i<text.length(); i++){
    if(text[i]!='\r') result.append(text[i]);
    }
  return result;
  }


// Read everything from the pipe until the other side is closed
static FXString readall(FXPipe& pipe){
  FXString result;
  FXchar c;
  while(pipe.readBlock(&c,1)==1) result.append(c);
  return result;
  }


// Run this program as a child with mode and argument; collect what it wrote to stdout and stderr
static FXbool runchild(const FXString& exec,const FXchar* mode,const FXchar* arg,const FXchar *const *env,FXString& out,FXString& err,FXint& code){
  const FXchar *const args[]={exec.text(),mode,arg,nullptr};
  FXPipe outread,outwrite,errread,errwrite;
  FXProcess process;
  FXbool started;
  if(!outread.open(outwrite,FXIO::Reading|FXIO::Inheritable)) return false;
  if(!errread.open(errwrite,FXIO::Reading|FXIO::Inheritable)) return false;
  process.setOutputStream(&outwrite);
  process.setErrorStream(&errwrite);
  started=process.start(exec.text(),args,env);

  // Our copies of the write sides must be closed to see the end of the data
  outwrite.close();
  errwrite.close();
  if(!started) return false;
  out=unixtext(readall(outread));
  err=unixtext(readall(errread));
  return process.wait(code);
  }


// Test creating, waiting for and killing processes; the program starts itself
static void testprocess(const FXString& exec){
  FXString envpath("PATH="+FXSystem::getEnvironment("PATH"));
  FXString envroot("SYSTEMROOT="+FXSystem::getEnvironment("SYSTEMROOT"));
  const FXchar *const env[]={"FOX_PROCESS_TEST=hello world",envpath.text(),envroot.text(),nullptr};
  FXString out,err;
  FXint code;

  // Exit code zero, output on both streams
  code=-1;
  check(runchild(exec,"--child","0",nullptr,out,err,code),"child with exit code 0 runs");
  check(code==0,"exit code 0 is returned");
  check(out=="child stdout\nchild env=(none)\n","stdout of the child is captured");
  check(err=="child stderr\n","stderr of the child is captured");

  // Other exit codes
  code=-1;
  check(runchild(exec,"--child","42",nullptr,out,err,code),"child with exit code 42 runs");
  check(code==42,"exit code 42 is returned");
  code=-1;
  check(runchild(exec,"--child","3",nullptr,out,err,code),"child with exit code 3 runs");
  check(code==3,"exit code 3 is returned");

  // Environment given to the child
  code=-1;
  check(runchild(exec,"--child","0",env,out,err,code),"child with an environment runs");
  check(code==0,"exit code 0 is returned with an environment");
  check(out=="child stdout\nchild env=hello world\n","the child sees the given environment");

  // Kill a child which would run for a minute
  {
  const FXchar *const args[]={exec.text(),"--sleep",nullptr};
  FXProcess process;
  FXTime beg=FXThread::time();
  code=0;
  check(process.start(exec.text(),args),"sleeping child starts");
  check(process.kill(),"sleeping child can be killed");
  check(process.wait(code),"killed child can be waited for");
  check(code!=0,"killed child does not report success");
  check((FXThread::time()-beg)<30000000000LL,"killing does not wait for the child to end");
  }

  // Program which does not exist
  {
  const FXchar *const args[]={"no-such-program-for-fox",nullptr};
  FXProcess process;
  check(!process.start("no-such-program-for-fox",args),"program which does not exist does not start");
  check(!process.wait(),"waiting without a started process fails");
  }
  }


// Start
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FXchar osver[100];
  FXint code;

  // Child of the test: write to both streams, show the test variable, exit with given code
  if(2<argc && strcmp(argv[1],"--child")==0){
    const FXchar* value=getenv("FOX_PROCESS_TEST");
    printf("child stdout\n");
    printf("child env=%s\n",value?value:"(none)");
    fflush(stdout);
    fprintf(stderr,"child stderr\n");
    fflush(stderr);
    return atoi(argv[2]);
    }

  // Child of the test: wait for the parent to kill it
  if(1<argc && strcmp(argv[1],"--sleep")==0){
    FXThread::sleep(60000000000LL);
    return 0;
    }

  // Start itself as child
  if(1<argc && strcmp(argv[1],"--test")==0){
    FXString exec=FXSystem::getExecFilename();
    if(exec.empty()) exec=argv[0];
    testprocess(exec);
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  // Trace
  setTraceLevel(151);

  // Get version
  fxosversion(osver,100);
  fxmessage("os version: %s\n",osver);

  // Make thread pool
  FXProcess process;

#if defined(WIN32)
  const FXchar command[]="C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe";
  const FXchar *const args[]={command,"c:\\WINDOWS\\win.ini",nullptr};
#else
//  const FXchar command[]="/bin/ls";
//  const FXchar *const args[]={command,"-l","/usr/bin",nullptr};
  const FXchar command[]="/usr/local/bin/adie";
  const FXchar *const args[]={command,"process.cpp",nullptr};
#endif

  // Kick off
  if(!process.start(command,args)){
    fxwarning("failed to start: %s\n",command);
    return 1;
    }

  // Wait
  if(process.wait(code)){
    fxwarning("chiled exited with code: %d\n",code);
    }

  return 0;
  }
