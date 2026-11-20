#include "fx.h"
#include "fxkeys.h"
#include "StopWatch.h"
#include <stdio.h>

#if defined(WIN32)
#include <windows.h>

static FXbool traceRequested(FXint argc,char* argv[]){
  for(FXint i=1;i<argc;i++){
    if((FXString::compare(argv[i],"-tracelevel")==0 || FXString::compare(argv[i],"-tracetopics")==0) && i+1<argc) return true;
    }
  const FXchar* level=getenv("FOX_TRACE_LEVEL");
  const FXchar* topics=getenv("FOX_TRACE_TOPICS");
  return (level && *level) || (topics && *topics);
  }


static void openTraceConsole(){
  if(GetConsoleWindow()==nullptr){
    if(!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
    }
  if(GetConsoleWindow()){
    freopen("CONOUT$","w",stdout);
    freopen("CONOUT$","w",stderr);
    }
  }
#endif


int main(int argc,char* argv[]){
#if defined(WIN32)
  if(traceRequested(argc,argv)) openTraceConsole();
#endif
  if(fxversion[0]!=FOX_MAJOR || fxversion[1]!=FOX_MINOR || fxversion[2]!=FOX_LEVEL){
    fxerror("FOX Library mismatch; expected version: %d.%d.%d, but found version: %d.%d.%d.\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL,fxversion[0],fxversion[1],fxversion[2]);
    }

  FXApp application("Stopwatch");
  application.init(argc,argv);
  new StopWatch(&application);
  new FXToolTip(&application,0);
  application.create();
  return application.run();
  }
